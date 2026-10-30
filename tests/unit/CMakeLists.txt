## wynpix_tests (Qt Test)

set(WYNPIX_APP_SOURCE_DIR "${PROJECT_SOURCE_DIR}/src")

qt_add_executable(wynpix_tests
    main.cpp
    tst_animationplayer.cpp
    tst_colortransform.cpp
    tst_wynpix.cpp
    testanimations.h
    tst_imagefilter.cpp
    tst_imageviewportcontroller.cpp
    tst_tilegrid.cpp
    tst_viewportinteraction.cpp
    tst_viewtransform.cpp
    tst_decodedpixels.cpp
    tst_decodebudget.cpp
    tst_quickshell.cpp
    tst_singleinstance.cpp
    tst_windowstate.cpp
    tst_upscaledecision.cpp
    tst_windowtitle.cpp
    tst_uimetrics.cpp
    tst_overlays.cpp
    tst_thumbnailstrip.cpp
    tst_thumbnailgeometry.cpp
    tst_folderview.cpp
    tst_contextmenu.cpp
    tst_crop.cpp
    tst_svgrasterizer.cpp
    tst_dialogs.cpp
    tst_settingseditor.cpp
    tst_batchprint.cpp
    tst_viewertoggles.cpp
    tst_rendererrorchannel.cpp
    tst_hdrsource.cpp
    tst_settingsvaluemapping.cpp
    tst_imagelib.cpp
    ../fakes/fakebatchconversionservice.h
    ../fakes/fakesettingsstores.h
    "${WYNPIX_APP_SOURCE_DIR}/components/viewmode/viewmodecontroller.cpp"
    "${WYNPIX_APP_SOURCE_DIR}/components/shellinfo/windowtitle.cpp"
    "${WYNPIX_APP_SOURCE_DIR}/components/singleinstance/singleinstancechannel.cpp"
    "${WYNPIX_APP_SOURCE_DIR}/components/upscaler/upscaledecision.cpp"
    "${WYNPIX_APP_SOURCE_DIR}/components/windowstate/windowstatecontroller.cpp"
    "${WYNPIX_APP_SOURCE_DIR}/sourcecontainers/decodedpixels.cpp"
    "${WYNPIX_APP_SOURCE_DIR}/utils/decodebudget.cpp"
    # HdrToneMapper::toSdr, the SDR conversion DecodedPixels is given.
    "${WYNPIX_APP_SOURCE_DIR}/utils/hdrtonemapper.cpp"
    # ImageLib scalers and colour adjustments (tst_imagelib.cpp).
    "${WYNPIX_APP_SOURCE_DIR}/utils/imagelib.cpp"
    # The auto white balance of ImageLib and ImageRenderItem.
    "${WYNPIX_APP_SOURCE_DIR}/utils/autocoloradjustments.cpp"
    "${WYNPIX_APP_SOURCE_DIR}/sourcecontainers/thumbnail.cpp"
    "${WYNPIX_APP_SOURCE_DIR}/components/thumbnailer/thumbnailgeometry.cpp"
    "${WYNPIX_APP_SOURCE_DIR}/gui/quick/adapters/directoryviewadapter.cpp"
    "${WYNPIX_APP_SOURCE_DIR}/gui/quick/adapters/themesnapshotbuilder.cpp"
    "${WYNPIX_APP_SOURCE_DIR}/themestore.cpp"
    "${WYNPIX_APP_SOURCE_DIR}/utils/actions.cpp"
    # The shortcut table rules (tst_wynpix.cpp).
    "${WYNPIX_APP_SOURCE_DIR}/components/actionmanager/shortcutmap.cpp"
    "${WYNPIX_APP_SOURCE_DIR}/utils/inputmap.cpp"
    # The application's Settings under SettingsValueMapping (golden-ini test).
    "${WYNPIX_APP_SOURCE_DIR}/gui/settingseditor/settingsvaluemapping.cpp"
    "${WYNPIX_APP_SOURCE_DIR}/settings.cpp"
    "${WYNPIX_APP_SOURCE_DIR}/utils/formatregistry.cpp"
    "${WYNPIX_APP_SOURCE_DIR}/utils/script.cpp"
)

# Components under test are compiled from the application sources and use
# the application's include root.
target_include_directories(wynpix_tests PRIVATE
    "${WYNPIX_APP_SOURCE_DIR}"
    # In-memory stores of the settings editor (also used by wynpix_qml_tests).
    "${CMAKE_CURRENT_SOURCE_DIR}/../fakes"
)

target_link_libraries(wynpix_tests PRIVATE
    Qt6::Core
    Qt6::Gui
    # SingleInstanceChannel (QLocalServer / QLocalSocket).
    Qt6::Network
    Qt6::Test
    Qt6::Quick
    # TileGrid, ImageFilterMode (moc data of RenderEnums), colour transform
    # plans and ColorLutBuilder.
    wynpix_render
    # ImageViewportController and FramePresentationTracker; the view
    # components (ViewTransform, ViewportInteraction, AnimationPlayer, ...)
    # come with it.
    wynpix_ui
    # DwmSetWindowAttribute (WindowStateController suppresses DWM transitions
    # while switching to and from fullscreen).
    dwmapi
)

wynpix_configure_test(wynpix_tests)
