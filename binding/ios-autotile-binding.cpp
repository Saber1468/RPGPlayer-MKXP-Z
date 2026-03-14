/*
** ios-autotile-binding.cpp
**
** iOS-specific Ruby bindings for autotile animation optimization.
**
** This file exposes two utility subsystems to the Ruby scripting layer:
**
**  1. MKXPZ::IOS module
**     - tileset_fits_native?(bitmap) → bool
**         Returns true when the bitmap height is within the GPU texture limit,
**         meaning the native C++ Tilemap can render it without TileWrap.
**
**     - max_texture_size → Integer
**         Exposes the resolved GL_MAX_TEXTURE_SIZE so Ruby can replicate the
**         same limit check that TileWrap::MAX_TEX_SIZE already uses.
**
**     - platform → String ("ios" | "macos" | "other")
**         Lets Ruby conditionally apply iOS-only code paths.
**
**  2. NativeTilemapWrapper (Ruby class backed by the C++ Tilemap object)
**     The game's CustomTilemap is a pure-Ruby reimplementation that works
**     around the 1024-px GPU texture limit for mega-tilesets.  For standard
**     tilesets (height <= max_tex_size) the native C++ Tilemap is both faster
**     and avoids all Ruby-side bitmap.blt stalls.
**
**     This wrapper lets Ruby scripts instantiate and use the native Tilemap
**     through the same interface as CustomTilemap, so a thin Ruby adapter can
**     forward calls to whichever backend is appropriate.
**
**     Usage (Ruby side):
**       if MKXPZ::IOS.tileset_fits_native?(tileset_bmp)
**         @tm = NativeTilemapWrapper.new(viewport, tileset_bmp, map_data,
**                                        priorities, autotiles_array)
**       else
**         @tm = CustomTilemap.new(viewport)   # mega-tileset path
**       end
**
** Integration:
**   Add this file to the CMakeLists.txt source list alongside the other
**   binding/*.cpp files, then call  iosAutotileBindingInit()  from
**   binding-mri.cpp (or binding-mri-ios.cpp) at engine startup.
**
** NOTE: The Ruby-side patch (000001_IOSAutotilePatch.rb) is the primary
**   performance fix and works independently of this file.  This file adds
**   the optional native bypass path for non-mega tilesets.
*/

#include "binding-util.h"
#include "binding-types.h"
#include "bitmap.h"
#include "tilemap.h"
#include "viewport.h"
#include "table.h"
#include "sharedstate.h"
#include "gl-fun.h"

#include <ruby.h>
#include <string.h>

/* -------------------------------------------------------------------------
** Helper: resolve the per-device GL max texture size once and cache it.
** Mirrors TileWrap::MAX_TEX_SIZE logic in Ruby.
** ------------------------------------------------------------------------- */
static int resolveMaxTextureSize()
{
    static int cached = 0;
    if (cached > 0)
        return cached;

    GLint glMax = 0;
    gl.GetIntegerv(GL_MAX_TEXTURE_SIZE, &glMax);

    /* Clamp to a conservative 1024-aligned value (matches TileWrap) */
    int clamped = (glMax / 1024) * 1024;
    if (clamped < 1024)
        clamped = 1024;

    cached = clamped;
    return cached;
}

/* =========================================================================
** MKXPZ::IOS module bindings
** ========================================================================= */

/*
** MKXPZ::IOS.max_texture_size → Integer
** Returns the device's clamped GL_MAX_TEXTURE_SIZE (1024-aligned).
*/
RB_METHOD(iosMaxTextureSize)
{
    RB_UNUSED_PARAM;
    return rb_int_new(resolveMaxTextureSize());
}

/*
** MKXPZ::IOS.tileset_fits_native?(bitmap) → bool
** Returns true when bitmap.height <= max_texture_size,
** i.e. the native C++ Tilemap can render it without TileWrap wrapping.
*/
RB_METHOD(iosTilesetFitsNative)
{
    VALUE bitmapObj;
    rb_get_args(argc, argv, "o", &bitmapObj RB_ARG_END);

    Bitmap *bmp = getPrivateDataCheck<Bitmap>(bitmapObj, BitmapType);

    int maxSize = resolveMaxTextureSize();
    bool fits   = (bmp->height() <= maxSize);

    return fits ? Qtrue : Qfalse;
}

/*
** MKXPZ::IOS.platform → String
** Returns "ios", "macos", or "other".
*/
RB_METHOD(iosPlatform)
{
    RB_UNUSED_PARAM;
#if defined(__IPHONE_OS_VERSION_MIN_REQUIRED) || defined(TARGET_OS_IPHONE)
    return rb_str_new_cstr("ios");
#elif defined(__APPLE__)
    return rb_str_new_cstr("macos");
#else
    return rb_str_new_cstr("other");
#endif
}

/* =========================================================================
** NativeTilemapWrapper class
**
** Thin Ruby class that owns a native C++ Tilemap and exposes the same
** interface as CustomTilemap so Ruby adapters can use either backend.
**
** The wrapper stores the Tilemap* as private data (same pattern used by the
** existing tilemap-binding.cpp).  It deliberately does NOT inherit the RGSS
** Tilemap class to avoid confusion; it appears as its own class.
** ========================================================================= */

#if RAPI_FULL > 187
DEF_TYPE(NativeTilemapWrapper);
#else
DEF_ALLOCFUNC(NativeTilemapWrapper);
#endif

/*
** NativeTilemapWrapper.new(viewport)
*/
RB_METHOD(nativeWrapperInitialize)
{
    VALUE viewportObj = Qnil;
    rb_get_args(argc, argv, "|o", &viewportObj RB_ARG_END);

    Viewport *vp = nullptr;
    if (!NIL_P(viewportObj))
        vp = getPrivateDataCheck<Viewport>(viewportObj, ViewportType);

    GFX_LOCK;
    Tilemap *t = new Tilemap(vp);
    rb_iv_set(self, "viewport", viewportObj);
    setPrivateData(self, t);
    t->initDynAttribs();

    /* Mirror autotiles sub-object setup from tilemap-binding.cpp */
    VALUE autotilesObj = rb_iv_get(self, "autotiles");
    if (autotilesObj != Qnil)
        setPrivateData(autotilesObj, nullptr);

    /* Re-use the existing TilemapAutotiles type registered by tilemap-binding */
    wrapProperty(self, &t->getAutotiles(), "autotiles", TilemapAutotilesType);
    wrapProperty(self, &t->getColor(),     "color",     ColorType);
    wrapProperty(self, &t->getTone(),      "tone",      ToneType);

    autotilesObj = rb_iv_get(self, "autotiles");
    VALUE ary = rb_ary_new2(7);
    for (int i = 0; i < 7; ++i)
        rb_ary_push(ary, Qnil);
    rb_iv_set(autotilesObj, "array", ary);
    rb_iv_set(autotilesObj, "tilemap", self);

    GFX_UNLOCK;
    return self;
}

/*
** NativeTilemapWrapper#update
** Steps the native autotile animation counter — must be called each game frame.
** This is what advances water/flower animation at GPU level with zero CPU cost.
*/
RB_METHOD(nativeWrapperUpdate)
{
    RB_UNUSED_PARAM;
    Tilemap *t = getPrivateData<Tilemap>(self);
    GFX_LOCK;
    t->update();
    GFX_UNLOCK;
    return Qnil;
}

/*
** NativeTilemapWrapper#dispose
*/
RB_METHOD(nativeWrapperDispose)
{
    RB_UNUSED_PARAM;
    Tilemap *t = getPrivateDataNoRaise<Tilemap>(self);
    if (!t)
        return Qnil;
    GFX_LOCK;
    t->dispose();
    GFX_UNLOCK;
    return Qnil;
}

/*
** NativeTilemapWrapper#disposed?
*/
RB_METHOD(nativeWrapperDisposed)
{
    RB_UNUSED_PARAM;
    Tilemap *t = getPrivateDataNoRaise<Tilemap>(self);
    if (!t)
        return Qtrue;
    return t->isDisposed() ? Qtrue : Qfalse;
}

/*
** NativeTilemapWrapper#viewport
*/
RB_METHOD(nativeWrapperGetViewport)
{
    RB_UNUSED_PARAM;
    checkDisposed<Tilemap>(self);
    return rb_iv_get(self, "viewport");
}

/*
** NativeTilemapWrapper#autotiles
*/
RB_METHOD(nativeWrapperGetAutotiles)
{
    RB_UNUSED_PARAM;
    return rb_iv_get(self, "autotiles");
}

/* Property bindings — identical to the standard Tilemap class */
DEF_GFX_PROP_OBJ_REF(NativeTilemapWrapper, Bitmap, Tileset,   "tileset")
DEF_GFX_PROP_OBJ_REF(NativeTilemapWrapper, Table,  MapData,   "map_data")
DEF_GFX_PROP_OBJ_REF(NativeTilemapWrapper, Table,  FlashData, "flash_data")
DEF_GFX_PROP_OBJ_REF(NativeTilemapWrapper, Table,  Priorities,"priorities")
DEF_GFX_PROP_OBJ_VAL(NativeTilemapWrapper, Color,  Color,     "color")
DEF_GFX_PROP_OBJ_VAL(NativeTilemapWrapper, Tone,   Tone,      "tone")
DEF_GFX_PROP_B(NativeTilemapWrapper, Visible)
DEF_GFX_PROP_I(NativeTilemapWrapper, OX)
DEF_GFX_PROP_I(NativeTilemapWrapper, OY)

/* =========================================================================
** Registration entry point
**
** Call this from binding-mri.cpp or binding-mri-ios.cpp:
**   extern void iosAutotileBindingInit();
**   iosAutotileBindingInit();
** ========================================================================= */
void iosAutotileBindingInit()
{
    /* ------------------------------------------------------------------
    ** MKXPZ::IOS module
    ** ------------------------------------------------------------------ */
    VALUE mkxpzMod = rb_define_module("MKXPZ");
    VALUE iosMod   = rb_define_module_under(mkxpzMod, "IOS");

    rb_define_module_function(iosMod, "max_texture_size",    iosMaxTextureSize,    0);
    rb_define_module_function(iosMod, "tileset_fits_native?",iosTilesetFitsNative, 1);
    rb_define_module_function(iosMod, "platform",            iosPlatform,          0);

    /* ------------------------------------------------------------------
    ** NativeTilemapWrapper class
    ** ------------------------------------------------------------------ */
    VALUE klass = rb_define_class("NativeTilemapWrapper", rb_cObject);

#if RAPI_FULL > 187
    rb_define_alloc_func(klass, classAllocate<&NativeTilemapWrapperType>);
#else
    rb_define_alloc_func(klass, NativeTilemapWrapperAllocate);
#endif

    _rb_define_method(klass, "initialize", nativeWrapperInitialize);
    _rb_define_method(klass, "dispose",    nativeWrapperDispose);
    _rb_define_method(klass, "disposed?",  nativeWrapperDisposed);
    _rb_define_method(klass, "update",     nativeWrapperUpdate);
    _rb_define_method(klass, "viewport",   nativeWrapperGetViewport);
    _rb_define_method(klass, "autotiles",  nativeWrapperGetAutotiles);

    INIT_PROP_BIND(NativeTilemapWrapper, Tileset,    "tileset");
    INIT_PROP_BIND(NativeTilemapWrapper, MapData,    "map_data");
    INIT_PROP_BIND(NativeTilemapWrapper, FlashData,  "flash_data");
    INIT_PROP_BIND(NativeTilemapWrapper, Priorities, "priorities");
    INIT_PROP_BIND(NativeTilemapWrapper, Visible,    "visible");
    INIT_PROP_BIND(NativeTilemapWrapper, OX,         "ox");
    INIT_PROP_BIND(NativeTilemapWrapper, OY,         "oy");
    INIT_PROP_BIND(NativeTilemapWrapper, Color,      "color");
    INIT_PROP_BIND(NativeTilemapWrapper, Tone,       "tone");
}
