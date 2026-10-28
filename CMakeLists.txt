# wynpix root
cmake_minimum_required(VERSION 3.25)
project(wynpix
    VERSION 1.2.2.0
    HOMEPAGE_URL "https://github.com/hadoooooouken/wynpix"
    LANGUAGES C CXX RC)

if(NOT DEFINED PROJECT_VERSION_TWEAK OR PROJECT_VERSION_TWEAK STREQUAL "")
    set(PROJECT_VERSION_TWEAK 0)
endif()

configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/src/wynpix_version.h.in"
    "${CMAKE_CURRENT_BINARY_DIR}/generated/wynpix_version.h"
    @ONLY
)

message(STATUS "Build configuration: " ${CMAKE_BUILD_TYPE})

# AUTOMOC
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

# OPTIMIZATIONS & HARDENING
# ISA, IPO, hardening and CRT policy is applied per target through
# wynpix_apply_msvc_profile() (cmake/WynpixMsvcPolicy.cmake). Only the Release
# optimization baseline is set here; it is shadowed as a normal variable so a
# stale cache entry (e.g. one carrying /MD) cannot leak into the build.
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(WynpixMsvcPolicy)
if(MSVC)
    set(CMAKE_C_FLAGS_RELEASE "${WYNPIX_MSVC_RELEASE_BASELINE_FLAGS}")
    set(CMAKE_CXX_FLAGS_RELEASE "${WYNPIX_MSVC_RELEASE_BASELINE_FLAGS}")
endif()

# USER OPTIONS
option(EXIV2 "For reading/writing exif tags" ON)
option(WYNPIX_BUILD_TESTS "Build the Qt Test and Qt Quick Test suites" ON)

# FIND PACKAGES
set(QT_VERSION_MAJOR 6)
# GuiPrivate (QRhi) is used deliberately by wynpix.render; see its CMakeLists.
set(QT_NO_PRIVATE_MODULE_WARNING ON)
find_package(Qt6 6.12 REQUIRED COMPONENTS
    # PrintSupport (QPrinter) brings Qt Widgets with it; nothing else uses
    # Widgets.
    Core Gui Network Svg PrintSupport Sql Pdf
    Qml Quick QuickControls2 QuickDialogs2 ShaderTools
    # QRhi headers (<rhi/qrhi.h>) for the wynpix.render module.
    GuiPrivate)
if(WYNPIX_BUILD_TESTS)
    find_package(Qt6 6.12 REQUIRED COMPONENTS Test QuickTest)
endif()

# Qt CMake policies are opted into individually instead of through
# qt_standard_project_setup(REQUIRES 6.12): on Windows that command also sets
# CMAKE_RUNTIME_OUTPUT_DIRECTORY, which would move wynpix and the DLLs it
# deploys next to itself out of their established output directories.
# QTP0006..QTP0008 cover Wayland and deploy scripts and do not apply here.
qt_policy(SET QTP0001 NEW)
qt_policy(SET QTP0002 NEW)
qt_policy(SET QTP0003 NEW)
qt_policy(SET QTP0004 NEW)
qt_policy(SET QTP0005 NEW)
# exiv2 is looked up in src/CMakeLists.txt, next to the zlib-ng install
# its static package config depends on.


##############################################################

# --- UPSCAYL NCNN INTEGRATION ---

if(DEFINED ENV{VULKAN_SDK})
    set(VULKAN_SDK_DEFAULT "$ENV{VULKAN_SDK}")
else()
    set(VULKAN_SDK_DEFAULT "E:/VULKAN")
endif()

set(VULKAN_SDK_PATH "${VULKAN_SDK_DEFAULT}" CACHE PATH "Path to Vulkan SDK")

if(NOT EXISTS "${VULKAN_SDK_PATH}")
    message(FATAL_ERROR "Vulkan SDK not found: ${VULKAN_SDK_PATH}")
endif()

message(STATUS "Configuring Upscayl NCNN with Vulkan SDK: ${VULKAN_SDK_PATH}")

list(APPEND CMAKE_PREFIX_PATH "${VULKAN_SDK_PATH}")

if(NOT DEFINED ENV{VULKAN_SDK})
    set(ENV{VULKAN_SDK} "${VULKAN_SDK_PATH}")
endif()

set(NCNN_VULKAN ON CACHE BOOL "Build NCNN with Vulkan support")
set(NCNN_VULKAN_ONLINE_SPIRV ON CACHE BOOL "Build NCNN with Vulkan online SPIRV compilation support")

# The glslang copy pinned by the ncnn submodule still declares
# cmake_minimum_required(VERSION 2.8.12), which CMake 4 rejects. Raise the
# floor for the upscayl-ncnn subtree only, instead of editing the submodule.
set(UPSCAYL_NCNN_POLICY_VERSION_MINIMUM 3.5)
set(CMAKE_POLICY_VERSION_MINIMUM ${UPSCAYL_NCNN_POLICY_VERSION_MINIMUM})
add_subdirectory(upscayl-ncnn/src)
unset(CMAKE_POLICY_VERSION_MINIMUM)

# upscayl-ncnn.dll is shipped by this project and gets the full profile.
# Its ncnn/glslang static libraries are LTCG IL (the upscayl subproject enables
# IPO for itself), so the DLL keeps Release IPO to link them without a restart.
wynpix_apply_msvc_profile(upscayl-ncnn)
wynpix_suppress_crt_deprecation(upscayl-ncnn)

# ncnn adds /arch:SSE2 to its base TUs and dispatches at runtime to TUs whose
# source flags end with /arch:AVX, /arch:AVX2 or /arch:AVX512; that strategy is
# left intact. The application baseline is still placed BEFORE ncnn's own
# /arch:SSE2 on purpose: MSVC lowers code generation to the last /arch, but
# keeps __AVX__/__AVX2__ predefined from an earlier /arch:AVX2, and ncnn's x86
# sources select their AVX/AVX2 intrinsic paths by those macros.
#   - AVX2 build: the base TUs keep their AVX/AVX2 paths (dropping the leading
#     flag took convolution_x86.cpp from 6691 ymm instructions to 0), so this
#     ordering reproduces the established build.
#   - SSE2 build: the leading flag is /arch:SSE2, no AVX macro is predefined
#     and the base TUs fall back to their SSE2 paths; the runtime-dispatched
#     AVX/AVX2/AVX512 TUs are unchanged and still selected by CPUID.
# glslang has no ISA strategy of its own and gets the regular baseline.
# All of them keep the IPO setting of the upscayl subproject; /guard:ehcont on
# them is required for the upscayl-ncnn.dll link to accept /guard:ehcont.
target_compile_options(ncnn BEFORE PRIVATE "$<$<COMPILE_LANGUAGE:C,CXX>:${WYNPIX_MSVC_ISA_OPTIONS}>")
wynpix_apply_msvc_profile(ncnn NO_ARCH NO_IPO)
foreach(_glslang_target glslang SPIRV MachineIndependent GenericCodeGen OGLCompiler OSDependent)
    if(TARGET ${_glslang_target})
        wynpix_apply_msvc_profile(${_glslang_target} NO_IPO)
    endif()
endforeach()

add_subdirectory(src)
add_subdirectory(shellex)

if(WYNPIX_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
