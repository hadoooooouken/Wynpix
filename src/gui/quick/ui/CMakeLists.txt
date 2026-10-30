## QML MODULE wynpix.ui
# Application shell of the Qt Quick UI. QML files here are presentation only;
# decisions stay in C++ components (see docs/ARCHITECTURE.md).

qt_add_library(wynpix_ui STATIC)

qt_add_qml_module(wynpix_ui
    URI wynpix.ui
    IMPORTS wynpix.bridges wynpix.render wynpix.style
    SOURCES
        crop/cropcontroller.cpp
        crop/cropcontroller.h
        crop/cropselection.cpp
        crop/cropselection.h
        dialogs/batchconversionservice.cpp
        dialogs/batchconversionservice.h
        dialogs/batchconverterdialogmodel.cpp
        dialogs/batchconverterdialogmodel.h
        dialogs/batchqueuemodel.cpp
        dialogs/batchqueuemodel.h
        dialogs/confirmationdialogmodel.cpp
        dialogs/confirmationdialogmodel.h
        dialogs/dialogcoordinator.cpp
        dialogs/dialogcoordinator.h
        dialogs/dialogsession.cpp
        dialogs/dialogsession.h
        dialogs/filereplacedialogmodel.cpp
        dialogs/filereplacedialogmodel.h
        dialogs/printdialogmodel.cpp
        dialogs/printdialogmodel.h
        dialogs/resizedialogmodel.cpp
        dialogs/resizedialogmodel.h
        dialogs/savepathdialogmodel.cpp
        dialogs/savepathdialogmodel.h
        dialogs/textinputdialogmodel.cpp
        dialogs/textinputdialogmodel.h
        folderview/bookmarksmodel.cpp
        folderview/bookmarksmodel.h
        folderview/foldergridcontroller.cpp
        folderview/foldergridcontroller.h
        folderview/foldergridlayout.cpp
        folderview/foldergridlayout.h
        folderview/folderviewcontroller.cpp
        folderview/folderviewcontroller.h
        folderview/formatfiltermodel.cpp
        folderview/formatfiltermodel.h
        framepresentationtracker.cpp
        framepresentationtracker.h
        graphicsdevicelossmonitor.cpp
        graphicsdevicelossmonitor.h
        imageviewportcontroller.cpp
        imageviewportcontroller.h
        mainwindowshell.cpp
        mainwindowshell.h
        menus/contextmenumodel.cpp
        menus/contextmenumodel.h
        overlays/adjustmentslidermodel.cpp
        overlays/adjustmentslidermodel.h
        overlays/animationplaybackcontroller.cpp
        overlays/animationplaybackcontroller.h
        overlays/cassettingseditor.cpp
        overlays/cassettingseditor.h
        overlays/coloradjustmentseditor.cpp
        overlays/coloradjustmentseditor.h
        overlays/copytargetsmodel.cpp
        overlays/copytargetsmodel.h
        overlays/fullscreenchromecontroller.cpp
        overlays/fullscreenchromecontroller.h
        overlays/imageinfomodel.cpp
        overlays/imageinfomodel.h
        overlays/notificationoverlaymodel.cpp
        overlays/notificationoverlaymodel.h
        overlays/overlaycoordinator.cpp
        overlays/overlaycoordinator.h
        overlays/overlaystate.cpp
        overlays/overlaystate.h
        overlays/pagenavigationcontroller.cpp
        overlays/pagenavigationcontroller.h
        overlays/renamepromptcontroller.cpp
        overlays/renamepromptcontroller.h
        settings/settingsdialogcontroller.cpp
        settings/settingsdialogcontroller.h
        settings/settingseditortypes.h
        thumbnails/thumbnaillistmodel.cpp
        thumbnails/thumbnaillistmodel.h
        thumbnails/thumbnailpanelcontroller.cpp
        thumbnails/thumbnailpanelcontroller.h
        thumbnails/thumbnailstriplayout.cpp
        thumbnails/thumbnailstriplayout.h
    QML_FILES
        AccentColorDialog.qml
        AdjustmentSliders.qml
        AnimationPlaybackOverlay.qml
        BatchConverterDialog.qml
        BatchItemWidget.qml
        CasSettingsOverlay.qml
        ColorAdjustmentTracks.qml
        ColorAdjustmentsOverlay.qml
        ConfirmationDialog.qml
        ControlsOverlay.qml
        CopyOverlay.qml
        CropOverlay.qml
        CropPanel.qml
        DialogLayer.qml
        DialogWindow.qml
        FileReplaceDialog.qml
        FloatingMessage.qml
        FolderGridView.qml
        FolderView.qml
        FormatFilterComboBox.qml
        FullscreenInfoOverlay.qml
        ImageInfoOverlay.qml
        ImageViewport.qml
        LinkedSliderSpin.qml
        Main.qml
        MainPanel.qml
        OverlayHeaderButton.qml
        OverlayLayer.qml
        OverlayPanel.qml
        PageNavigationOverlay.qml
        PlacesPanel.qml
        PrintDialog.qml
        RenameOverlay.qml
        ResizeDialog.qml
        SaveConfirmOverlay.qml
        SaveFileDialog.qml
        ScriptEditorDialog.qml
        SettingsAboutPage.qml
        SettingsAdvancedPage.qml
        SettingsComboBox.qml
        SettingsControlsPage.qml
        SettingsDialog.qml
        SettingsGeneralPage.qml
        SettingsNote.qml
        SettingsPage.qml
        SettingsRadioRow.qml
        SettingsScriptsPage.qml
        SettingsSection.qml
        SettingsSliderRow.qml
        SettingsThemePage.qml
        SettingsUpscalePage.qml
        SettingsViewPage.qml
        ShortcutCaptureField.qml
        ShortcutCreatorDialog.qml
        SidePanel.qml
        TextInputDialog.qml
        ThumbnailStrip.qml
        ThumbnailWidget.qml
        ViewerContextMenu.qml
)

# The generated QML type registrations include the headers by file name.
target_include_directories(wynpix_ui PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/crop"
    "${CMAKE_CURRENT_SOURCE_DIR}/dialogs"
    "${CMAKE_CURRENT_SOURCE_DIR}/folderview"
    "${CMAKE_CURRENT_SOURCE_DIR}/menus"
    "${CMAKE_CURRENT_SOURCE_DIR}/overlays"
    "${CMAKE_CURRENT_SOURCE_DIR}/settings"
    "${CMAKE_CURRENT_SOURCE_DIR}/thumbnails"
)
target_compile_features(wynpix_ui PUBLIC cxx_std_23)
if(MSVC)
    # Same C++26 working-draft switch as wynpix.
    target_compile_options(wynpix_ui PRIVATE "$<$<COMPILE_LANGUAGE:CXX>:/std:c++latest>")
endif()

# qmlcachegen compiles the bindings on the bridge singletons and the render
# item to C++ that calls them directly, so the module links their libraries.
# ImageViewportController (public: the Quick UI host and the tests use it)
# builds on the view components and drives the render item.
target_link_libraries(wynpix_ui
    PUBLIC
        Qt6::Quick
        wynpix_bridges
        wynpix_render
        wynpix_style
        wynpix_viewcomponents
    PRIVATE
        Qt6::QuickControls2
        # FolderDialog of the copy / move overlay and the batch converter,
        # FileDialog of "Save as" and the PDF export, the settings window's
        # FileDialogs.
        Qt6::QuickDialogs2
)
