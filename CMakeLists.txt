# Copyright 2021, Collabora, Ltd.
# Copyright Moses Turner, 2021.
# Copyright Nick Klingensmith, 2021.
# Many thanks to Ryan Pavlik for showing the way
# SPDX-License-Identifier: MIT

# This build file has the following options
# - SK_LINUX_EGL
#     This tells the Linux build to use EGL as the OpenGL loader, 
#     instead of the default GLX.
# - SK_MULTITHREAD_BUILD_BY_DEFAULT
#     MSVC only, on by default. This forces projects here to
#     build with multi-threading (/MP)
# - SK_BUILD_TESTS
#     Build the StereoKitCTest project in addition to the StereoKitC
#     library. This is off by default.
# - SK_BUILD_SHARED_LIBS
#     Should StereoKit build as a shared, or static library?

cmake_minimum_required(VERSION 3.10)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project(StereoKit VERSION "0.3.2" LANGUAGES CXX C)

set(default_build_type "Release")
if(EXISTS "${CMAKE_SOURCE_DIR}/.git" AND NOT DEFINED ENV{CI})
  set(default_build_type "Debug")
endif()

###########################################
## Options                               ##
###########################################

set(SK_LINUX_EGL                    OFF CACHE BOOL "Force using EGL graphics backend on Linux")
set(SK_MULTITHREAD_BUILD_BY_DEFAULT ON  CACHE BOOL "MSVC only, on by default. This forces projects here to build with multi-threading (/MP)")
set(SK_BUILD_TESTS                  ON  CACHE BOOL "Build the StereoKitCTest project in addition to the StereoKitC library.")
set(SK_BUILD_SHARED_LIBS            ON  CACHE BOOL "Should StereoKit build as a shared, or static library?")

###########################################
## Dependencies                          ##
###########################################

# See: https://github.com/cpm-cmake/CPM.cmake
include("cmake/CPM.cmake")
if (UNIX)
  # Linux users/devs prefer shared system libraries rather than
  # static libraries. Linux OpenXR runtimes are responsible for
  # providing the shared OpenXR loader there. On Windows and
  # Android, static linking is necessary.
  set(CPM_USE_LOCAL_PACKAGES ON) # Tell CPM to prefer locally installed packages
endif()

#### ReactPhysics3D - https://www.reactphysics3d.com/ ####
CPMAddPackage(
  NAME reactphysics3d
  GITHUB_REPOSITORY DanielChappuis/reactphysics3d
  GIT_TAG bdc3153f552f6665ea9d2d13afd20ea871119e17 # v0.8.0
)
# Something in reactphysics3d seems to require -fPIC
set_property(TARGET reactphysics3d PROPERTY POSITION_INDEPENDENT_CODE ON)
# Disable warnings for reactphysics3d, it's a little horrifying
target_compile_options(reactphysics3d PRIVATE
  $<$<CXX_COMPILER_ID:MSVC>:/W0>
  $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-w> )

#### OpenXR - https://www.khronos.org/openxr/ ####
CPMAddPackage(
  NAME OpenXR
  GITHUB_REPOSITORY KhronosGroup/OpenXR-SDK
  GIT_TAG bf21ccb1007bb531b45d9978919a56ea5059c245 # v1.0.17
)

###########################################
## Shader Compiler                       ##
###########################################

set(SK_SRC_SHADERS_HLSL
  StereoKitC/shaders_builtin/shader_builtin_default.hlsl
  StereoKitC/shaders_builtin/shader_builtin_equirect.hlsl
  StereoKitC/shaders_builtin/shader_builtin_font.hlsl
  StereoKitC/shaders_builtin/shader_builtin_lines.hlsl
  StereoKitC/shaders_builtin/shader_builtin_pbr.hlsl
  StereoKitC/shaders_builtin/shader_builtin_skybox.hlsl
  StereoKitC/shaders_builtin/shader_builtin_ui.hlsl
  StereoKitC/shaders_builtin/shader_builtin_ui_box.hlsl
  StereoKitC/shaders_builtin/shader_builtin_ui_quadrant.hlsl
  StereoKitC/shaders_builtin/shader_builtin_unlit.hlsl
  StereoKitC/shaders_builtin/shader_builtin_unlit_clip.hlsl )

set(SK_SRC_SHADERS_H
  StereoKitC/shaders_builtin/shader_builtin.h )

set(SK_SHADER_COMPILER "${CMAKE_SOURCE_DIR}/Tools/skshaderc")
foreach(SHADER ${SK_SRC_SHADERS_HLSL})
  # Shader compile tool doesn't work on Linux yet, but pre-built
  # shaders should still be in the repository.
  if (WIN32)
    add_custom_command(
        OUTPUT ${CMAKE_SOURCE_DIR}/${SHADER}.h
        COMMAND ${SK_SHADER_COMPILER} -h -t xge -i ${CMAKE_SOURCE_DIR}/Tools/include -o ${CMAKE_SOURCE_DIR}/StereoKitC/shaders_builtin ${CMAKE_SOURCE_DIR}/${SHADER}
        DEPENDS ${SHADER} )
  endif()
  list(APPEND SK_SRC_SHADERS_H ${SHADER}.h)
endforeach(SHADER)

###########################################
## Platform Specific Library Linking     ##
###########################################

if (UNIX)
  set(LINUX_LIBS
    Threads::Threads
    GLX
    X11
    fontconfig )

  # Add pthreads
  set(THREADS_PREFER_PTHREAD_FLAG ON)
  find_package(Threads REQUIRED)

  # Add OpenGL, and account for the SK_LINUX_EGL option
  if(SK_LINUX_EGL)
    add_definitions("-DSKG_LINUX_EGL")
    list(APPEND LINUX_LIBS EGL)
  else()
    add_definitions("-DSKG_LINUX_GLX")
    find_package(GLEW REQUIRED)
    find_package(X11 REQUIRED)
    list(APPEND LINUX_LIBS GL GLEW)
  endif()
elseif(WIN32)
  add_definitions("-D_CRT_SECURE_NO_WARNINGS")
  set(WINDOWS_LIBS
    WindowsApp) # Required for ISAC spatial audio code
endif()

###########################################
## StereoKitC                            ##
###########################################

set(SK_SRC_CODE
  StereoKitC/stereokit.h
  StereoKitC/stereokit_ui.h

  StereoKitC/_stereokit.h
  StereoKitC/_stereokit_ui.h
  StereoKitC/color.cpp
  StereoKitC/hierarchy.h
  StereoKitC/hierarchy.cpp
  StereoKitC/intersect.cpp
  StereoKitC/log.h
  StereoKitC/log.cpp
  StereoKitC/rect_atlas.h
  StereoKitC/rect_atlas.cpp
  StereoKitC/sk_math.h
  StereoKitC/sk_math.cpp
  StereoKitC/sk_memory.h
  StereoKitC/sk_memory.cpp
  StereoKitC/spherical_harmonics.h
  StereoKitC/spherical_harmonics.cpp
  StereoKitC/stereokit.cpp
  StereoKitC/stereokit_ui.cpp )

set(SK_SRC_ASSET_TYPES
  StereoKitC/asset_types/assets.h
  StereoKitC/asset_types/assets.cpp
  StereoKitC/asset_types/font.h
  StereoKitC/asset_types/font.cpp
  StereoKitC/asset_types/material.h
  StereoKitC/asset_types/material.cpp
  StereoKitC/asset_types/mesh.h
  StereoKitC/asset_types/mesh.cpp
  StereoKitC/asset_types/model.h
  StereoKitC/asset_types/model.cpp
  StereoKitC/asset_types/model_gltf.cpp
  StereoKitC/asset_types/model_obj.cpp
  StereoKitC/asset_types/model_ply.cpp
  StereoKitC/asset_types/model_stl.cpp
  StereoKitC/asset_types/shader.h
  StereoKitC/asset_types/shader.cpp
  StereoKitC/asset_types/sound.h
  StereoKitC/asset_types/sound.cpp
  StereoKitC/asset_types/sprite.h
  StereoKitC/asset_types/sprite.cpp
  StereoKitC/asset_types/texture.h
  StereoKitC/asset_types/texture.cpp )

set(SK_SRC_LIBRARIES
  StereoKitC/libraries/array.h
  StereoKitC/libraries/cgltf.h
  StereoKitC/libraries/ferr_hash.h
  StereoKitC/libraries/ferr_hash.cpp
  StereoKitC/libraries/isac_spatial_sound.h
  StereoKitC/libraries/isac_spatial_sound.cpp
  StereoKitC/libraries/micro_ply.h
  StereoKitC/libraries/miniaudio.h
  StereoKitC/libraries/miniaudio.cpp
  StereoKitC/libraries/sk_gpu.h
  StereoKitC/libraries/sk_gpu.cpp
  StereoKitC/libraries/sokol_time.h
  StereoKitC/libraries/sokol_time.cpp
  StereoKitC/libraries/stb_image.h
  StereoKitC/libraries/stb_image_write.h
  StereoKitC/libraries/stb_rect_pack.h
  StereoKitC/libraries/stb_truetype.h
  StereoKitC/libraries/stref.h
  StereoKitC/libraries/stref.cpp
  StereoKitC/libraries/tinycthread.h
  StereoKitC/libraries/tinycthread.cpp )

set(SK_SRC_SYSTEMS
  StereoKitC/systems/audio.h
  StereoKitC/systems/audio.cpp
  StereoKitC/systems/defaults.h
  StereoKitC/systems/defaults.cpp
  StereoKitC/systems/input.h
  StereoKitC/systems/input.cpp
  StereoKitC/systems/input_keyboard.h
  StereoKitC/systems/input_keyboard.cpp
  StereoKitC/systems/line_drawer.h
  StereoKitC/systems/line_drawer.cpp
  StereoKitC/systems/physics.h
  StereoKitC/systems/physics.cpp
  StereoKitC/systems/render.h
  StereoKitC/systems/render.cpp
  StereoKitC/systems/render_sort.h
  StereoKitC/systems/render_sort.cpp
  StereoKitC/systems/sprite_drawer.h
  StereoKitC/systems/sprite_drawer.cpp
  StereoKitC/systems/system.h
  StereoKitC/systems/system.cpp
  StereoKitC/systems/text.h
  StereoKitC/systems/text.cpp  
  StereoKitC/systems/world.h
  StereoKitC/systems/world.cpp )

set(SK_SRC_HANDS
  StereoKitC/systems/hand/hand_mouse.h
  StereoKitC/systems/hand/hand_mouse.cpp
  StereoKitC/systems/hand/hand_override.h
  StereoKitC/systems/hand/hand_override.cpp
  StereoKitC/systems/hand/hand_oxr_articulated.h
  StereoKitC/systems/hand/hand_oxr_articulated.cpp
  StereoKitC/systems/hand/hand_oxr_controller.h
  StereoKitC/systems/hand/hand_oxr_controller.cpp
  StereoKitC/systems/hand/hand_poses.h
  StereoKitC/systems/hand/input_hand.h
  StereoKitC/systems/hand/input_hand.cpp )

set(SK_SRC_PLATFORM
  StereoKitC/systems/platform/android.cpp
  StereoKitC/systems/platform/android.h
  StereoKitC/systems/platform/flatscreen_input.h
  StereoKitC/systems/platform/flatscreen_input.cpp
  StereoKitC/systems/platform/linux.h
  StereoKitC/systems/platform/linux.cpp
  StereoKitC/systems/platform/openxr.h
  StereoKitC/systems/platform/openxr.cpp
  StereoKitC/systems/platform/openxr_extensions.h
  StereoKitC/systems/platform/openxr_input.h
  StereoKitC/systems/platform/openxr_input.cpp
  StereoKitC/systems/platform/openxr_view.h
  StereoKitC/systems/platform/openxr_view.cpp
  StereoKitC/systems/platform/platform.h
  StereoKitC/systems/platform/platform.cpp
  StereoKitC/systems/platform/platform_utils.h
  StereoKitC/systems/platform/platform_utils.cpp
  StereoKitC/systems/platform/uwp.h
  StereoKitC/systems/platform/uwp.cpp  
  StereoKitC/systems/platform/win32.h
  StereoKitC/systems/platform/win32.cpp )

set(SK_SRC_TOOLS
  StereoKitC/tools/file_picker.h
  StereoKitC/tools/file_picker.cpp )

# Switch between a static or shared library based on
# the cmake options provided.
if (SK_BUILD_SHARED_LIBS)
  set(LIBRARY_TYPE SHARED)
else()
  set(LIBRARY_TYPE STATIC)
endif()

add_library(StereoKitC ${LIBRARY_TYPE}
  ${SK_SRC_CODE}
  ${SK_SRC_ASSET_TYPES}
  ${SK_SRC_LIBRARIES}
  ${SK_SRC_SYSTEMS}
  ${SK_SRC_HANDS}
  ${SK_SRC_PLATFORM}
  ${SK_SRC_TOOLS}
  ${SK_SRC_SHADERS_H} )

# Set up Visual Studio folders/filters for a more organized
# workspace there.
source_group("code"        FILES ${SK_SRC_CODE})
source_group("asset_types" FILES ${SK_SRC_ASSET_TYPES})
source_group("libraries"   FILES ${SK_SRC_LIBRARIES})
source_group("systems"     FILES ${SK_SRC_SYSTEMS})
source_group("hands"       FILES ${SK_SRC_HANDS})
source_group("platform"    FILES ${SK_SRC_PLATFORM})
source_group("tools"       FILES ${SK_SRC_TOOLS})

if (UNIX)
  # DirectXMath on Linux needs a few extra files
  set(LINUX_INCLUDES
    StereoKitC/lib/include_no_win/)
endif()

target_include_directories(StereoKitC
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/StereoKitC>
    $<INSTALL_INTERFACE:include>
  PRIVATE
    ${CMAKE_CURRENT_BINARY_DIR}/StereoKitC/
    ${LINUX_INCLUDES}
)

target_link_libraries( StereoKitC
  PRIVATE
    reactphysics3d
  PUBLIC
    openxr_loader
    ${LINUX_LIBS}
    ${WINDOWS_LIBS}
    ${CMAKE_DL_LIBS}
)

###########################################
## StereoKitCTest                        ##
###########################################

if (SK_BUILD_TESTS)
  add_executable( StereoKitCTest
    Examples/StereoKitCTest/main.cpp
    Examples/StereoKitCTest/demo_basics.h
    Examples/StereoKitCTest/demo_basics.cpp
    Examples/StereoKitCTest/demo_picker.h
    Examples/StereoKitCTest/demo_picker.cpp
    Examples/StereoKitCTest/demo_mic.h
    Examples/StereoKitCTest/demo_mic.cpp
    Examples/StereoKitCTest/demo_sprites.h
    Examples/StereoKitCTest/demo_sprites.cpp
    Examples/StereoKitCTest/scene.h
    Examples/StereoKitCTest/scene.cpp
    Examples/StereoKitCTest/demo_ui.h
    Examples/StereoKitCTest/demo_ui.cpp
    Examples/StereoKitCTest/demo_world.h
    Examples/StereoKitCTest/demo_world.cpp
    Examples/StereoKitCTest/demo_lines.h
    Examples/StereoKitCTest/demo_lines.cpp
    Examples/StereoKitCTest/demo_lighting.h
    Examples/StereoKitCTest/demo_lighting.cpp
    Examples/StereoKitCTest/skt_lighting.h
    Examples/StereoKitCTest/skt_lighting.cpp
    Examples/StereoKitCTest/Shaders/skt_default_lighting.hlsl.h
    Examples/StereoKitCTest/Shaders/skt_light_only.hlsl.h
  )

  target_link_libraries( StereoKitCTest
    StereoKitC
  )

  if (MSVC AND SK_MULTITHREAD_BUILD_BY_DEFAULT)
    target_compile_options(StereoKitCTest PRIVATE "/MP")
  endif()
endif()

###########################################
## Multi-threaded build MSVC             ##
###########################################

# Setup Visual Studio to use multi-threaded compilation by
# default, this is mostly for when using Visual Studio as
# the IDE. CLI users can pass in command line arguments.
if (MSVC AND SK_MULTITHREAD_BUILD_BY_DEFAULT)
  target_compile_options(reactphysics3d PRIVATE "/MP")
  target_compile_options(openxr_loader PRIVATE "/MP")
  target_compile_options(StereoKitC PRIVATE "/MP")
endif()

###########################################
## Installation                          ##
###########################################

# For installing on Linux
# find_package(PkgConfig)

install(
  TARGETS StereoKitC 
  EXPORT  StereoKitC-targets
  RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
  LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
  ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
)

install(
  FILES       "StereoKitC/stereokit.h" "StereoKitC/stereokit_ui.h" 
  DESTINATION ${CMAKE_INSTALL_PREFIX}/include) 

install(
  EXPORT      StereoKitC-targets
  FILE        StereoKitCConfig.cmake
  NAMESPACE   StereoKitC::
  DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/cmake/StereoKitC"
)