# MKXP-Z iOS Build Configuration
# CMakeLists.txt for cross-compiling MKXP-Z engine for iOS
#
# Usage (from ios/build/):
#   cmake .. -DCMAKE_SYSTEM_NAME=iOS -DCMAKE_OSX_ARCHITECTURES=arm64
#   make -j$(sysctl -n hw.ncpu)
cmake_minimum_required(VERSION 3.20)

project(mkxp-z-ios VERSION 2.4.2 LANGUAGES C CXX OBJC OBJCXX)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ====================
# Configuration
# ====================

# Path to pre-built iOS dependencies (built by ios/Dependencies/Makefile)
set(DEPS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/Dependencies/build-iphoneos" CACHE PATH
    "Path to pre-built iOS dependencies (headers in include/, libs in lib/)")

# Platform detection for device vs simulator
if(CMAKE_OSX_SYSROOT MATCHES "iphonesimulator")
    set(IOS_PLATFORM "SIMULATOR")
else()
    set(IOS_PLATFORM "DEVICE")
endif()

message(STATUS "MKXP-Z iOS Build")
message(STATUS "  Platform: ${IOS_PLATFORM}")
message(STATUS "  Dependencies: ${DEPS_DIR}")

# ====================
# Build Definitions
# ====================

add_compile_definitions(
    MKXPZ_VERSION="${PROJECT_VERSION}"
    MKXPZ_BUILD_CMAKE
    MKXPZ_BUILD_XCODE
    MKXPZ_ALCDEVICE=ALCdevice
    AL_LIBTYPE_STATIC
    GLES2_HEADER
    HAVE_NANOSLEEP
    IOS_PLATFORM
    MKXPZ_MINIFFI
    _TTF_Font=TTF_Font
    MKXPZ_NO_SYSTEM_CALL
)

# ====================
# Include Directories
# ====================

set(MKXPZ_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../src")
set(MKXPZ_BINDING_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../binding")
set(MKXPZ_SHADER_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../shader")
set(MKXPZ_ASSETS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../assets")

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}              # iOS-specific stubs (TouchBar.h)
    ${CMAKE_CURRENT_SOURCE_DIR}/generated    # Generated xxd headers
    ${CMAKE_CURRENT_SOURCE_DIR}/..           # For 'src/util/util.h' style includes
    ${MKXPZ_SRC_DIR}
    ${MKXPZ_SRC_DIR}/audio
    ${MKXPZ_SRC_DIR}/crypto
    ${MKXPZ_SRC_DIR}/display
    ${MKXPZ_SRC_DIR}/display/gl
    ${MKXPZ_SRC_DIR}/display/libnsgif
    ${MKXPZ_SRC_DIR}/display/libnsgif/utils
    ${MKXPZ_SRC_DIR}/etc
    ${MKXPZ_SRC_DIR}/filesystem
    ${MKXPZ_SRC_DIR}/filesystem/ghc
    ${MKXPZ_SRC_DIR}/input
    ${MKXPZ_SRC_DIR}/net
    ${MKXPZ_SRC_DIR}/system
    ${MKXPZ_SRC_DIR}/util
    ${MKXPZ_SRC_DIR}/util/sigslot
    ${MKXPZ_SRC_DIR}/util/sigslot/adapter
    ${MKXPZ_SRC_DIR}/theoraplay
    ${MKXPZ_BINDING_DIR}
    # Dependencies (from ios/Dependencies/build-iphoneos)
    ${DEPS_DIR}/include
    ${DEPS_DIR}/include/AL
    ${DEPS_DIR}/include/SDL2
    ${DEPS_DIR}/include/pixman-1
    ${DEPS_DIR}/include/freetype2
    ${DEPS_DIR}/include/uchardet
    ${DEPS_DIR}/include/ruby-3.1.0
    ${DEPS_DIR}/include/ruby-3.1.0/arm-apple-darwin
)

# ====================
# Source Files
# ====================

set(MKXPZ_SOURCES
    # Core
    ${MKXPZ_SRC_DIR}/main.cpp
    ${MKXPZ_SRC_DIR}/config.cpp
    ${MKXPZ_SRC_DIR}/eventthread.cpp
    ${MKXPZ_SRC_DIR}/settingsmenu.cpp
    ${MKXPZ_SRC_DIR}/sharedstate.cpp

    # Audio
    ${MKXPZ_SRC_DIR}/audio/alstream.cpp
    ${MKXPZ_SRC_DIR}/audio/audio.cpp
    ${MKXPZ_SRC_DIR}/audio/audiostream.cpp
    ${MKXPZ_SRC_DIR}/audio/fluid-fun.cpp
    ${MKXPZ_SRC_DIR}/audio/midisource.cpp
    ${MKXPZ_SRC_DIR}/audio/sdlsoundsource.cpp
    ${MKXPZ_SRC_DIR}/audio/soundemitter.cpp
    ${MKXPZ_SRC_DIR}/audio/vorbissource.cpp
    ${MKXPZ_SRC_DIR}/theoraplay/theoraplay.c

    # Crypto
    ${MKXPZ_SRC_DIR}/crypto/rgssad.cpp

    # Display
    ${MKXPZ_SRC_DIR}/display/autotiles.cpp
    ${MKXPZ_SRC_DIR}/display/autotilesvx.cpp
    ${MKXPZ_SRC_DIR}/display/bitmap.cpp
    ${MKXPZ_SRC_DIR}/display/font.cpp
    ${MKXPZ_SRC_DIR}/display/graphics.cpp
    ${MKXPZ_SRC_DIR}/display/plane.cpp
    ${MKXPZ_SRC_DIR}/display/sprite.cpp
    ${MKXPZ_SRC_DIR}/display/tilemap.cpp
    ${MKXPZ_SRC_DIR}/display/tilemapvx.cpp
    ${MKXPZ_SRC_DIR}/display/viewport.cpp
    ${MKXPZ_SRC_DIR}/display/window.cpp
    ${MKXPZ_SRC_DIR}/display/windowvx.cpp
    ${MKXPZ_SRC_DIR}/display/libnsgif/libnsgif.c
    ${MKXPZ_SRC_DIR}/display/libnsgif/lzw.c

    # OpenGL
    ${MKXPZ_SRC_DIR}/display/gl/gl-debug.cpp
    ${MKXPZ_SRC_DIR}/display/gl/gl-fun.cpp
    ${MKXPZ_SRC_DIR}/display/gl/gl-meta.cpp
    ${MKXPZ_SRC_DIR}/display/gl/glstate.cpp
    ${MKXPZ_SRC_DIR}/display/gl/scene.cpp
    ${MKXPZ_SRC_DIR}/display/gl/shader.cpp
    ${MKXPZ_SRC_DIR}/display/gl/texpool.cpp
    ${MKXPZ_SRC_DIR}/display/gl/tileatlas.cpp
    ${MKXPZ_SRC_DIR}/display/gl/tileatlasvx.cpp
    ${MKXPZ_SRC_DIR}/display/gl/tilequad.cpp
    ${MKXPZ_SRC_DIR}/display/gl/vertex.cpp

    # Utilities
    ${MKXPZ_SRC_DIR}/util/iniconfig.cpp
    ${MKXPZ_SRC_DIR}/util/win-consoleutils.cpp

    # Etc
    ${MKXPZ_SRC_DIR}/etc/etc.cpp
    ${MKXPZ_SRC_DIR}/etc/table.cpp

    # Filesystem
    ${MKXPZ_SRC_DIR}/filesystem/filesystem.cpp
    ${MKXPZ_SRC_DIR}/filesystem/filesystemImpl.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/filesystemImplApple-ios.mm

    # Input
    ${MKXPZ_SRC_DIR}/input/input.cpp
    ${MKXPZ_SRC_DIR}/input/keybindings.cpp

    # Network
    ${MKXPZ_SRC_DIR}/net/LUrlParser.cpp
    ${MKXPZ_SRC_DIR}/net/net.cpp

    # System (use iOS-specific version)
    ${MKXPZ_SRC_DIR}/system/systemImpl.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/systemImplApple.mm

    # Ruby Bindings (use iOS-patched version for binding-mri)
    ${CMAKE_CURRENT_SOURCE_DIR}/binding-mri-ios.cpp
    ${MKXPZ_BINDING_DIR}/binding-util.cpp
    ${MKXPZ_BINDING_DIR}/table-binding.cpp
    ${MKXPZ_BINDING_DIR}/etc-binding.cpp
    ${MKXPZ_BINDING_DIR}/bitmap-binding.cpp
    ${MKXPZ_BINDING_DIR}/font-binding.cpp
    ${MKXPZ_BINDING_DIR}/graphics-binding.cpp
    ${MKXPZ_BINDING_DIR}/input-binding.cpp
    ${MKXPZ_BINDING_DIR}/sprite-binding.cpp
    ${MKXPZ_BINDING_DIR}/viewport-binding.cpp
    ${MKXPZ_BINDING_DIR}/plane-binding.cpp
    ${MKXPZ_BINDING_DIR}/window-binding.cpp
    ${MKXPZ_BINDING_DIR}/tilemap-binding.cpp
    ${MKXPZ_BINDING_DIR}/audio-binding.cpp
    ${MKXPZ_BINDING_DIR}/module_rpg.cpp
    ${MKXPZ_BINDING_DIR}/filesystem-binding.cpp
    ${MKXPZ_BINDING_DIR}/windowvx-binding.cpp
    ${MKXPZ_BINDING_DIR}/tilemapvx-binding.cpp
    ${MKXPZ_BINDING_DIR}/http-binding.cpp
    ${MKXPZ_BINDING_DIR}/miniffi-binding.cpp
    ${MKXPZ_BINDING_DIR}/miniffi.cpp
)

# ====================
# Libraries
# ====================

# Find static libraries built by ios/Dependencies/Makefile
set(DEP_LIBDIR "${DEPS_DIR}/lib")

find_library(SDL2_LIB        SDL2         PATHS ${DEP_LIBDIR} NO_DEFAULT_PATH)
find_library(SDL2_IMAGE_LIB  SDL2_image   PATHS ${DEP_LIBDIR} NO_DEFAULT_PATH)
find_library(SDL2_TTF_LIB    SDL2_ttf     PATHS ${DEP_LIBDIR} NO_DEFAULT_PATH)
find_library(SDL2_SOUND_LIB  SDL2_sound   PATHS ${DEP_LIBDIR} NO_DEFAULT_PATH)
find_library(OGG_LIB         ogg          PATHS ${DEP_LIBDIR} NO_DEFAULT_PATH)
find_library(VORBIS_LIB      vorbis       PATHS ${DEP_LIBDIR} NO_DEFAULT_PATH)
find_library(VORBISFILE_LIB  vorbisfile   PATHS ${DEP_LIBDIR} NO_DEFAULT_PATH)
find_library(THEORA_LIB      theora       PATHS ${DEP_LIBDIR} NO_DEFAULT_PATH)
find_library(THEORADEC_LIB   theoradec    PATHS ${DEP_LIBDIR} NO_DEFAULT_PATH)
find_library(OPENAL_LIB      openal       PATHS ${DEP_LIBDIR} NO_DEFAULT_PATH)
find_library(PHYSFS_LIB      physfs       PATHS ${DEP_LIBDIR} NO_DEFAULT_PATH)
find_library(PIXMAN_LIB      pixman-1     PATHS ${DEP_LIBDIR} NO_DEFAULT_PATH)
find_library(UCHARDET_LIB    uchardet     PATHS ${DEP_LIBDIR} NO_DEFAULT_PATH)
find_library(FREETYPE_LIB    freetype     PATHS ${DEP_LIBDIR} NO_DEFAULT_PATH)
find_library(PNG_LIB         png          PATHS ${DEP_LIBDIR} NO_DEFAULT_PATH)

# Ruby static library
find_library(RUBY_LIB ruby-static PATHS ${DEP_LIBDIR} NO_DEFAULT_PATH)
if(NOT RUBY_LIB)
    find_library(RUBY_LIB ruby PATHS ${DEP_LIBDIR} NO_DEFAULT_PATH)
endif()

# ====================
# Static Library Target
# ====================

add_library(mkxpz STATIC ${MKXPZ_SOURCES})

target_compile_options(mkxpz PRIVATE
    -Wno-non-virtual-dtor
    -Wno-reorder
    -Wno-uninitialized
    -Wno-unknown-pragmas
    -Wno-deprecated-declarations
    -Wno-incompatible-pointer-types
    -Wno-pointer-sign
    -Wno-incompatible-pointer-types-discards-qualifiers
    -fdeclspec
)

# Collect all found dependency libraries
set(DEP_LIBS "")
foreach(_lib
    SDL2_LIB SDL2_IMAGE_LIB SDL2_TTF_LIB SDL2_SOUND_LIB
    OPENAL_LIB PHYSFS_LIB PIXMAN_LIB FREETYPE_LIB PNG_LIB
    OGG_LIB VORBIS_LIB VORBISFILE_LIB
    THEORA_LIB THEORADEC_LIB
    UCHARDET_LIB RUBY_LIB)
    if(${_lib})
        list(APPEND DEP_LIBS ${${_lib}})
    else()
        message(WARNING "Dependency not found: ${_lib} (looked in ${DEP_LIBDIR})")
    endif()
endforeach()

target_link_libraries(mkxpz
    ${DEP_LIBS}
    # iOS System Frameworks
    "-framework Foundation"
    "-framework UIKit"
    "-framework OpenGLES"
    "-framework AudioToolbox"
    "-framework CoreAudio"
    "-framework CoreGraphics"
    "-framework CoreMotion"
    "-framework GameController"
    "-framework Metal"
    "-framework QuartzCore"
    "-framework AVFoundation"
    # System libraries
    bz2
    z
    iconv
)

# ====================
# Installation
# ====================

install(TARGETS mkxpz
    ARCHIVE DESTINATION lib
)

install(FILES
    ${MKXPZ_SRC_DIR}/config.h
    ${MKXPZ_SRC_DIR}/sharedstate.h
    ${MKXPZ_SRC_DIR}/binding.h
    DESTINATION include/mkxpz
)

message(STATUS "MKXP-Z iOS configuration complete")
message(STATUS "  Ruby library: ${RUBY_LIB}")
message(STATUS "  Build as static library for embedding in iOS app")
