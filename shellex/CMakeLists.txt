# shellex/CMakeLists.txt
cmake_minimum_required(VERSION 3.25)

project(wynpixshellex LANGUAGES C CXX)

set(CMAKE_AUTOMOC ON)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../cmake")
include(WynpixMsvcPolicy)

# Find Qt
find_package(Qt6 REQUIRED COMPONENTS Core Gui Svg)

add_library(wynpixshellex SHARED
    wynpixshellex.h
    wynpixshellex.cpp
    wynpixshellex.def
    "${CMAKE_CURRENT_SOURCE_DIR}/../src/3rdparty/djvudec/djvu.c"
)

# Standard C++ features
target_compile_features(wynpixshellex PRIVATE cxx_std_23)

if(MSVC)
    # djvu.c is part of this target, so the C++ switch is restricted to CXX.
    target_compile_options(wynpixshellex PRIVATE "$<$<COMPILE_LANGUAGE:CXX>:/std:c++latest>")
endif()

wynpix_apply_msvc_profile(wynpixshellex)
wynpix_suppress_crt_deprecation(wynpixshellex)

# Disable Qt version tagging to allow delay-loading (prevents data symbol imports)
target_compile_definitions(wynpixshellex PRIVATE QT_NO_VERSION_TAGGING)

# Target properties
set_target_properties(wynpixshellex PROPERTIES
    CXX_EXTENSIONS OFF
    PREFIX "" # No "lib" prefix on Windows (output as wynpixshellex.dll directly)
    RUNTIME_OUTPUT_DIRECTORY_RELEASE "${WYNPIX_RELEASE_DIR}"
)

# Link Qt, LibRaw, and standard Win32 libraries
target_include_directories(wynpixshellex PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/../formats/LibRaw"
    "${CMAKE_CURRENT_SOURCE_DIR}/../src/3rdparty/djvudec"
)
target_link_libraries(wynpixshellex PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Svg
    shlwapi
    "${CMAKE_CURRENT_SOURCE_DIR}/../formats/LibRaw/buildfiles/release-x86_64${WYNPIX_DEPS_SUFFIX}/libraw_static.lib"
)
target_compile_definitions(wynpixshellex PRIVATE LIBRAW_NODLL)

# Delay-loading for MSVC (the .def source already makes CMake pass /DEF)
if(MSVC)
    target_link_options(wynpixshellex PRIVATE
        "/DELAYLOAD:Qt6Core.dll"
        "/DELAYLOAD:Qt6Gui.dll"
        "/DELAYLOAD:Qt6Svg.dll"
    )
    target_link_libraries(wynpixshellex PRIVATE delayimp)
endif()
