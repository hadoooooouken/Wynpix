## WYNPIX TESTS
# wynpix_tests      Qt Test suite for UI-independent components.
# wynpix_qml_tests  Qt Quick Test suite for the QML modules.
# wynpix_render_tests  Qt Test pixel tests of the wynpix.render GPU renderer.

# Tests run from the build tree without windeployqt: Qt's bin directory
# provides the DLLs, and QLibraryInfo then resolves the Qt plugin and QML
# import paths from the Qt installation.
set(WYNPIX_TEST_QT_BIN_DIR "${QT6_INSTALL_PREFIX}/${QT6_INSTALL_BINS}")

# On Windows, Qt Test writes its log to the debugger (OutputDebugString)
# unless stderr has a console, and a CTest pipe is none: forcing stderr
# logging keeps the log in the captured output (--output-on-failure, skip
# detection).
set(WYNPIX_TEST_LOG_ENVIRONMENT "QT_FORCE_STDERR_LOGGING=set:1")

# wynpix_configure_test(<target>)
# Applies the project compiler profile to a test executable and registers it
# with CTest, headless (offscreen platform), with its log in the output.
function(wynpix_configure_test target)
    target_compile_features(${target} PRIVATE cxx_std_23)
    if(MSVC)
        # Same C++26 working-draft switch as wynpix.
        target_compile_options(${target} PRIVATE "$<$<COMPILE_LANGUAGE:CXX>:/std:c++latest>")
    endif()
    set_target_properties(${target} PROPERTIES CXX_EXTENSIONS OFF)
    wynpix_apply_msvc_profile(${target})
    wynpix_suppress_crt_deprecation(${target})

    add_test(NAME ${target} COMMAND ${target})
    set_tests_properties(${target} PROPERTIES
        ENVIRONMENT_MODIFICATION
            "PATH=path_list_prepend:${WYNPIX_TEST_QT_BIN_DIR};QT_QPA_PLATFORM=set:offscreen;${WYNPIX_TEST_LOG_ENVIRONMENT}"
    )
endfunction()

add_subdirectory(unit)
add_subdirectory(qml)
add_subdirectory(render)
