## wynpix_render_tests (Qt Test)
# Pixel tests of ImageRenderItem / ImageRenderer. The item is rendered
# headless through QQuickRenderControl into a QRhi texture (Direct3D 11, 12 or
# Vulkan) and read back, so no window is shown.

qt_add_executable(wynpix_render_tests
    offscreenquick.cpp
    offscreenquick.h
    referenceimages.cpp
    referenceimages.h
    tst_imagerenderer.cpp
    # The HDR tests compare against the unchanged CPU tone mapper.
    "${PROJECT_SOURCE_DIR}/src/utils/hdrtonemapper.cpp"
)

target_link_libraries(wynpix_render_tests PRIVATE
    Qt6::Gui
    Qt6::GuiPrivate
    Qt6::Quick
    Qt6::Test
    wynpix_render
    # ImageViewportController's settle reporting runs on the real frame loop.
    wynpix_ui
)

wynpix_configure_test(wynpix_render_tests)

# A backend the machine cannot create skips the whole suite in initTestCase;
# CTest reports that run as skipped instead of passed. Single skipped test
# functions (a documented backend limitation) leave the run passed.
set(WYNPIX_RENDER_TEST_SKIP_PATTERN "SKIP   : [A-Za-z]+::initTestCase[(][)]")
set_tests_properties(wynpix_render_tests PROPERTIES
    SKIP_REGULAR_EXPRESSION "${WYNPIX_RENDER_TEST_SKIP_PATTERN}")

# The same tests on the other QRhi backends of the Quick UI, selected like the
# scene graph's through QSG_RHI_BACKEND (wynpix_render_tests itself runs on
# Direct3D 11). The offscreen QPA platform cannot create Vulkan instances, so
# the Vulkan run uses the windows platform; it still renders through
# QQuickRenderControl and never shows a window. A backend the machine cannot
# create is reported as skipped.
set(WYNPIX_RENDER_TEST_BACKENDS "d3d12=offscreen" "vulkan=windows")
foreach(entry IN LISTS WYNPIX_RENDER_TEST_BACKENDS)
    string(REPLACE "=" ";" entry_parts "${entry}")
    list(GET entry_parts 0 backend)
    list(GET entry_parts 1 platform)
    add_test(NAME wynpix_render_tests_${backend} COMMAND wynpix_render_tests)
    set_tests_properties(wynpix_render_tests_${backend} PROPERTIES
        ENVIRONMENT_MODIFICATION
            "PATH=path_list_prepend:${WYNPIX_TEST_QT_BIN_DIR};QT_QPA_PLATFORM=set:${platform};QSG_RHI_BACKEND=set:${backend};${WYNPIX_TEST_LOG_ENVIRONMENT}"
        SKIP_REGULAR_EXPRESSION "${WYNPIX_RENDER_TEST_SKIP_PATTERN}"
    )
endforeach()
