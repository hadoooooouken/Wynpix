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

# Development build (wynpix-x64-dev): the Release optimization baseline without
# LTCG, whose whole-program link of wynpix.exe dominates rebuilds after an
# edit, and without hardening. It links the non-LTCG exiv2 of
# build_scripts/build_exiv2-dev.ps1 (the only prebuilt /GL library in
# wynpix.exe) and deploys to release-dev.
option(WYNPIX_DEV_BUILD "Release build without LTCG and hardening, deployed to release-dev (AVX2 only)" OFF)
if(WYNPIX_DEV_BUILD)
    if(NOT WYNPIX_ISA STREQUAL "AVX2")
        message(FATAL_ERROR "WYNPIX_DEV_BUILD supports only WYNPIX_ISA AVX2")
    endif()
    set(WYNPIX_RELEASE_DIR_SUFFIX "-dev")
else()
    set(WYNPIX_RELEASE_DIR_SUFFIX "${WYNPIX_DEPS_SUFFIX}")
endif()

# Sample Profile-Guided Optimization (SPGO).
# Requires MSVC 19.51+ and LTCG (/GL).  The first build with WYNPIX_SPGO=ON
# produces an empty Sample Profile Database (.spd) next to each linked binary.
# After collecting hardware performance counter samples with xperf and
# converting them (SPTAggregate -> SPDConvert), set WYNPIX_SPD_DIR to the
# directory holding the populated .spd files and rebuild.  The linker reads
# each <binary>.spd and applies hot/cold code layout, branch probability
# tuning and targeted inlining.
#
# Workflow:
#   1. cmake --preset wynpix-x64-release-spgo && cmake --build --preset wynpix-x64-release-spgo
#   2. build_scripts/collect-spgo-profile.ps1 -ExePath release/wynpix.exe
#   3. cmake --build --preset wynpix-x64-release-spgo          (re-link)
option(WYNPIX_SPGO "Enable Sample Profile-Guided Optimization (SPGO) link flags" OFF)
set(WYNPIX_SPD_DIR "" CACHE PATH "Directory containing populated .spd files for SPGO (empty = use linker defaults)")
if(WYNPIX_SPGO)
    if(WYNPIX_DEV_BUILD)
        message(FATAL_ERROR "WYNPIX_SPGO requires LTCG and is incompatible with WYNPIX_DEV_BUILD")
    endif()
    if(NOT CMAKE_BUILD_TYPE STREQUAL "Release")
        message(WARNING "WYNPIX_SPGO is only effective in Release builds (current: ${CMAKE_BUILD_TYPE})")
    endif()
    message(STATUS "wynpix SPGO: enabled (SPD dir: '${WYNPIX_SPD_DIR}')")
endif()

cmake_path(SET WYNPIX_RELEASE_DIR NORMALIZE "${CMAKE_CURRENT_LIST_DIR}/../release${WYNPIX_RELEASE_DIR_SUFFIX}")
message(STATUS "wynpix ISA: ${WYNPIX_ISA} (dependencies: ${WYNPIX_DEPS_INSTALL_NAME}, output: ${WYNPIX_RELEASE_DIR})")

# Compile-side hardening: Control Flow Guard, Spectre v1 mitigation and
# EH continuation metadata (consumed by CET shadow stacks).
set(WYNPIX_MSVC_SECURITY_COMPILE_OPTIONS /guard:cf /Qspectre /guard:ehcont)

# Link-side hardening for executables and DLLs.
set(WYNPIX_MSVC_SECURITY_LINK_OPTIONS /guard:cf /CETCOMPAT)
set(WYNPIX_MSVC_EHCONT_LINK_OPTIONS /guard:ehcont)

if(WYNPIX_DEV_BUILD)
    set(WYNPIX_MSVC_SECURITY_COMPILE_OPTIONS)
    set(WYNPIX_MSVC_SECURITY_LINK_OPTIONS)
    set(WYNPIX_MSVC_EHCONT_LINK_OPTIONS)
endif()

# SPGO link options, applied to linked binaries in wynpix_apply_msvc_profile().
# /spgo tells the linker to produce (or consume) a Sample Profile Database.
# /DEBUG generates the PDB with CodeView RSDS GUID required by xperf/perf_spt/SPDConvert.
# /OPT:REF and /OPT:ICF re-enable dead code elimination and identical COMDAT folding.
set(WYNPIX_MSVC_SPGO_LINK_OPTIONS)
if(WYNPIX_SPGO)
    list(APPEND WYNPIX_MSVC_SPGO_LINK_OPTIONS /spgo /DEBUG /OPT:REF /OPT:ICF)
endif()

# Dynamic CRT, debug flavour only in Debug.
set(WYNPIX_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")

# wynpix_apply_msvc_profile(<target>
#     [NO_ARCH]         Target owns its ISA strategy (e.g. ncnn runtime dispatch).
#     [NO_IPO]          Leave INTERPROCEDURAL_OPTIMIZATION to the target's project.
#     [NO_EHCONT_LINK]  Target links prebuilt C++ code without EH continuation
#                       metadata; compile-side /guard:ehcont is still emitted.
#     [NO_SPGO]         Exclude this target from SPGO profiling.
# )
function(wynpix_apply_msvc_profile target)
    if(NOT MSVC)
        return()
    endif()
    if(NOT TARGET ${target})
        message(FATAL_ERROR "wynpix_apply_msvc_profile: '${target}' is not a target")
    endif()

    cmake_parse_arguments(PROFILE "NO_ARCH;NO_IPO;NO_EHCONT_LINK;NO_SPGO" "" "" ${ARGN})
    if(PROFILE_UNPARSED_ARGUMENTS)
        message(FATAL_ERROR "wynpix_apply_msvc_profile: unknown arguments '${PROFILE_UNPARSED_ARGUMENTS}'")
    endif()

    # Optimization
    if(NOT PROFILE_NO_ARCH)
        target_compile_options(${target} PRIVATE
            "$<$<COMPILE_LANGUAGE:C,CXX>:${WYNPIX_MSVC_ISA_OPTIONS}>")
    endif()
    if(NOT PROFILE_NO_IPO AND NOT WYNPIX_DEV_BUILD)
        set_property(TARGET ${target} PROPERTY INTERPROCEDURAL_OPTIMIZATION_RELEASE ON)
    endif()
    if(WYNPIX_SPGO AND NOT PROFILE_NO_SPGO)
        # SPGO requires debug symbols (/Zi) and /DEBUG to generate a PDB with CodeView RSDS GUID,
        # which xperf/perf_spt uses to associate hardware performance samples with the binary.
        target_compile_options(${target} PRIVATE
            "$<$<COMPILE_LANGUAGE:C,CXX>:/Zi>")
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

        # SPGO: apply /spgo and optional /spd:<path> to linked binaries.
        if(WYNPIX_SPGO AND NOT PROFILE_NO_SPGO)
            target_link_options(${target} PRIVATE ${WYNPIX_MSVC_SPGO_LINK_OPTIONS})
            if(WYNPIX_SPD_DIR)
                # Resolve the output file name to locate the matching .spd file.
                get_target_property(_output_name ${target} OUTPUT_NAME)
                if(NOT _output_name)
                    set(_output_name ${target})
                endif()
                get_target_property(_suffix ${target} SUFFIX)
                if(NOT _suffix)
                    if(_type STREQUAL "EXECUTABLE")
                        set(_suffix ".exe")
                    else()
                        set(_suffix ".dll")
                    endif()
                endif()
                cmake_path(SET _spd_path NORMALIZE "${WYNPIX_SPD_DIR}/${_output_name}.spd")
                if(NOT EXISTS "${_spd_path}")
                    cmake_path(SET _spd_path NORMALIZE "${WYNPIX_SPD_DIR}/${_output_name}${_suffix}.spd")
                endif()
                if(EXISTS "${_spd_path}")
                    target_link_options(${target} PRIVATE "/spd:${_spd_path}")
                    message(STATUS "  SPGO ${target}: using ${_spd_path}")
                else()
                    message(STATUS "  SPGO ${target}: no SPD at ${_spd_path}, linker will use defaults")
                endif()
            endif()
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
        if(NOT WYNPIX_DEV_BUILD)
            set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELEASE ON)
        endif()
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
