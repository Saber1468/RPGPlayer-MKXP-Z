#ifndef __gl2platform_h_
#define __gl2platform_h_

/*
 * Minimal GLES2 platform header for MKXP-Z iOS builds.
 *
 * On macOS build hosts, Homebrew's SDL2 SDL_opengles2.h includes
 * <GLES2/gl2platform.h> which doesn't exist outside the iOS SDK.
 * This shim provides the required platform defines so the build
 * can find the header without needing the iOS SDK sysroot in the
 * default include path.
 */

#ifndef GL_APICALL
#define GL_APICALL
#endif

#ifndef GL_APIENTRY
#define GL_APIENTRY
#endif

#ifndef GL_APIENTRYP
#define GL_APIENTRYP GL_APIENTRY *
#endif

#endif /* __gl2platform_h_ */
