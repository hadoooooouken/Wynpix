# MSVC build policy for wynpix targets.
#
# The policy is applied explicitly per target through wynpix_apply_msvc_profile()
# instead of being injected into CMAKE_<LANG>_FLAGS / CMAKE_<TYPE>_LINKER_FLAGS,
# so third-party subprojects (ncnn, glslang) keep control of their own ISA and
# IPO strategy and only receive what is requested for them.
#
# Toolchain facts this policy relies on (MSVC 19.51, x64), verified with probe
# builds rather than taken from documentation:
#   - /O2 already implies /Ob2 /Oi /Ot, so they are not repeated.
#   - /GS is on by default.
#   - link.exe sets /DYNAMICBASE /HIGHENTROPYVA /NXCOMPAT by default for x64.
#   - /CETCOMPAT is NOT a linker default.
#   - When several /arch switches are present, the last one silently wins.
#   - Linking /guard:ehcont fails (LNK2047 / LNK1386) if any C++ object with EH
#     metadata was compiled without /guard:ehcont, including /GL objects in
#     prebuilt static libraries.

include_guard(GLOBAL)

# Release optimization baseline shared by every target in the build.
set(WYNPIX_MSVC_RELEASE_BASELINE_FLAGS "/O2 /DNDEBUG")

# Application ISA baseline. AVX2 is the primary build; SSE2 is the
# compatibility build for x64 CPUs without AVX/AVX2. Each ISA is configured in
# its own build directory (see CMakePresets.json) and links its own dependency
# trees, because a single AVX2-compiled dependency breaks the SSE2 build.
set(WYNPIX_ISA_VALUES AVX2 SSE2)
set(WYNPIX_ISA "AVX2" CACHE STRING "Instruction set baseline of the build (AVX2 or SSE2)")
set_property(CACHE WYNPIX_ISA PROPERTY STRINGS ${WYNPIX_ISA_VALUES})
if(NOT WYNPIX_ISA IN_LIST WYNPIX_ISA_VALUES)
    message(FATAL_ERROR "WYNPIX_ISA must be one of '${WYNPIX_ISA_VALUES}', got '${WYNPIX_ISA}'")
endif()

# Every ISA-specific path is derived from WYNPIX_DEPS_SUFFIX, which the
# dependency build scripts append to the same folder names.
if(WYNPIX_ISA STREQUAL "AVX2")
    set(WYNPIX_MSVC_ISA_OPTIONS /arch:AVX2)
    set(WYNPIX_DEPS_SUFFIX "")
else()
    set(WYNPIX_MSVC_ISA_OPTIONS /arch:SSE2)
    set(WYNPIX_DEPS_SUFFIX "-sse2")
endif()
set(WYNPIX_DEPS_INSTALL_NAME "install${WYNPIX_DEPS_SUFFIX}")
set(WYNPIX_DEPS_BUILD_NAME "build${WYNPIX_DEPS_SUFFIX}")
cmake_path(SET WYNPIX_RELEASE_DIR NORMALIZE "${CMAKE_CURRENT_LIST_DIR}/../release${WYNPIX_DEPS_SUFFIX}")
message(STATUS "wynpix ISA: ${WYNPIX_ISA} (dependencies: ${WYNPIX_DEPS_INSTALL_NAME}, output: ${WYNPIX_RELEASE_DIR})")

# Compile-side hardening: Control Flow Guard, Spectre v1 mitigation and
# EH continuation metadata (consumed by CET shadow stacks).
set(WYNPIX_MSVC_SECURITY_COMPILE_OPTIONS /guard:cf /Qspectre /guard:ehcont)

# Link-side hardening for executables and DLLs.
set(WYNPIX_MSVC_SECURITY_LINK_OPTIONS /guard:cf /CETCOMPAT)
set(WYNPIX_MSVC_EHCONT_LINK_OPTIONS /guard:ehcont)

# Dynamic CRT, debug flavour only in Debug.
set(WYNPIX_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")

# wynpix_apply_msvc_profile(<target>
#     [NO_ARCH]         Target owns its ISA strategy (e.g. ncnn runtime dispatch).
#     [NO_IPO]          Leave INTERPROCEDURAL_OPTIMIZATION to the target's project.
#     [NO_EHCONT_LINK]  Target links prebuilt C++ code without EH continuation
#                       metadata; compile-side /guard:ehcont is still emitted.
# )
function(wynpix_apply_msvc_profile target)
    if(NOT MSVC)
        return()
    endif()
    if(NOT TARGET ${target})
        message(FATAL_ERROR "wynpix_apply_msvc_profile: '${target}' is not a target")
    endif()

    cmake_parse_arguments(PROFILE "NO_ARCH;NO_IPO;NO_EHCONT_LINK" "" "" ${ARGN})
    if(PROFILE_UNPARSED_ARGUMENTS)
        message(FATAL_ERROR "wynpix_apply_msvc_profile: unknown arguments '${PROFILE_UNPARSED_ARGUMENTS}'")
    endif()

    # Optimization
    if(NOT PROFILE_NO_ARCH)
        target_compile_options(${target} PRIVATE
            "$<$<COMPILE_LANGUAGE:C,CXX>:${WYNPIX_MSVC_ISA_OPTIONS}>")
    endif()
    if(NOT PROFILE_NO_IPO)
        set_property(TARGET ${target} PROPERTY INTERPROCEDURAL_OPTIMIZATION_RELEASE ON)
    endif()

    # Security
    target_compile_options(${target} PRIVATE
        "$<$<COMPILE_LANGUAGE:C,CXX>:${WYNPIX_MSVC_SECURITY_COMPILE_OPTIONS}>")

    get_target_property(_type ${target} TYPE)
    if(_type STREQUAL "EXECUTABLE" OR _type STREQUAL "SHARED_LIBRARY" OR _type STREQUAL "MODULE_LIBRARY")
        target_link_options(${target} PRIVATE ${WYNPIX_MSVC_SECURITY_LINK_OPTIONS})
        if(NOT PROFILE_NO_EHCONT_LINK)
            target_link_options(${target} PRIVATE ${WYNPIX_MSVC_EHCONT_LINK_OPTIONS})
        endif()
    endif()

    # Runtime
    set_property(TARGET ${target} PROPERTY MSVC_RUNTIME_LIBRARY "${WYNPIX_MSVC_RUNTIME_LIBRARY}")
endfunction()

# wynpix_apply_msvc_directory_profile()
#
# Applies the compile-side profile (ISA, hardening, Release IPO, CRT) to every
# target created afterwards in the calling directory and its subdirectories.
# Qt's CMake API generates helper targets for each QML module (static plugin,
# plugin init, resource and cache object libraries) that are not visible to
# the caller but are linked into wynpix; without /guard:ehcont on them the
# executable fails to link with /guard:ehcont. Link-side options stay with
# wynpix_apply_msvc_profile() on the linking executable. A macro, so that the
# CMAKE_* variables land in the caller's directory scope.
macro(wynpix_apply_msvc_directory_profile)
    if(MSVC)
        add_compile_options(
            "$<$<COMPILE_LANGUAGE:C,CXX>:${WYNPIX_MSVC_ISA_OPTIONS}>"
            "$<$<COMPILE_LANGUAGE:C,CXX>:${WYNPIX_MSVC_SECURITY_COMPILE_OPTIONS}>")
        set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELEASE ON)
        set(CMAKE_MSVC_RUNTIME_LIBRARY "${WYNPIX_MSVC_RUNTIME_LIBRARY}")
    endif()
endmacro()

# Silences C4996 CRT deprecation warnings for first-party targets only; it is
# deliberately not a directory-wide definition, because glslang defines the
# same macro without a guard and a global -D turns that into C4005.
function(wynpix_suppress_crt_deprecation target)
    if(MSVC)
        target_compile_definitions(${target} PRIVATE _CRT_SECURE_NO_WARNINGS)
    endif()
endfunction()
