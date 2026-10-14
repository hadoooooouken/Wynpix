## wynpix_qml_tests (Qt Quick Test)
# Loads the wynpix.* QML modules through the same static plugins as
# wynpix. Test files (tst_*.qml) are read from this source directory.
# The theme snapshots are built from the application's own colour schemes
# (ThemeStore) and the icon font is loaded from the source tree, so the style
# tests see the real light and dark themes.
# The executable is itself the wynpix.tests QML module, which provides the
# Fixture singleton (bridgetestfixture.h) the bridge tests drive.

set(WYNPIX_APP_SOURCE_DIR "${PROJECT_SOURCE_DIR}/src")

qt_add_executable(wynpix_qml_tests
    main.cpp
    "${WYNPIX_APP_SOURCE_DIR}/themestore.cpp"
    "${WYNPIX_APP_SOURCE_DIR}/gui/quick/adapters/themesnapshotbuilder.cpp"
)

qt_add_qml_module(wynpix_qml_tests
    URI wynpix.tests
    DEPENDENCIES wynpix.bridges wynpix.style wynpix.ui
    SOURCES
        bridgetestfixture.cpp
        bridgetestfixture.h
)

# In-memory stores of the settings editor (shared with wynpix_tests).
target_include_directories(wynpix_qml_tests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../fakes")

target_compile_definitions(wynpix_qml_tests PRIVATE
    QUICK_TEST_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}"
    WYNPIX_ICON_FONT_FILE="${WYNPIX_APP_SOURCE_DIR}/res/fonts/FluentSystemIcons-Custom.ttf"
)

target_link_libraries(wynpix_qml_tests PRIVATE
    Qt6::Qml
    Qt6::Quick
    Qt6::QuickTest
    # QTest key clicks into the dialog windows.
    Qt6::Test
    wynpix_bridges
    wynpix_style
    wynpix_ui
    wynpix_bridgesplugin
    wynpix_styleplugin
    wynpix_uiplugin
    wynpix_renderplugin
)

wynpix_configure_test(wynpix_qml_tests)
