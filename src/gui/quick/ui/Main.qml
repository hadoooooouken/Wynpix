pragma ComponentBehavior: Bound

import QtQuick
import wynpix.bridges
import wynpix.style

// Main window of the Qt Quick UI. The application style
// (wynpix.style, based on Basic) is imported directly instead of
// QtQuick.Controls: the style is selected at compile time, so qmlsc compiles
// the bindings ahead of time instead of resolving the style at runtime.
//
// Created hidden: the Quick UI host applies the graphics configuration first,
// and Core shows the window (geometry, fullscreen) through its window port.
ApplicationWindow {
    id: root

    // Provided by QuickUiHost (initial properties).
    required property ImageViewportController viewportController
    required property MainWindowShell windowShell
    required property OverlayCoordinator overlays
    required property ThumbnailPanelController thumbnailPanel
    required property ContextMenuModel contextMenu
    required property CropController crop
    required property FolderViewController folderView
    required property DialogCoordinator dialogs
    required property SettingsDialogController settingsDialog

    // The context menu exists from its first opening on.
    property bool contextMenuCreated: false

    readonly property int initialWidth: 1280
    readonly property int initialHeight: 800
    readonly property real opaqueOpacity: 1.0

    // The background behind the image. In windowed mode the window opacity
    // setting makes it translucent; the background item then carries the
    // alpha alone and the viewer clears to transparent, so it applies once.
    readonly property color viewerBackground: windowShell.fullscreen ? Theme.tokens.surfaceCanvasFullscreen
                                                                     : Theme.tokens.surfaceCanvas
    readonly property color windowBackground: Color.transparent(viewerBackground,
                                                                windowShell.backgroundOpacity)
    readonly property bool translucentBackground: windowShell.backgroundOpacity < opaqueOpacity

    width: initialWidth
    height: initialHeight
    visible: false
    // The title is kept current by the Quick UI host.
    title: Qt.application.name
    // QQuickWindow::setColor() rewrites the surface format's alpha buffer
    // whenever the colour's alpha changes, and the format is fixed once the
    // native window exists (the window starts hidden, possibly in folder view
    // where the background is opaque). A translucent window therefore keeps
    // a transparent colour for its whole life and the background is painted
    // by an item.
    color: windowShell.translucentWindow ? "transparent" : viewerBackground

    Rectangle {
        anchors.fill: parent
        visible: root.windowShell.translucentWindow
        color: root.windowBackground
    }

    // Document mode: the image viewer and the thumbnail panel, which takes
    // its space from the viewer while pinned and floats over it otherwise.
    // The side panel takes its space from both while the crop mode is
    // active; the crop overlay covers the viewer then.
    Item {
        anchors.fill: parent
        anchors.rightMargin: sidePanel.visible ? sidePanel.width : 0
        visible: !root.windowShell.folderViewActive

        ImageViewport {
            id: viewport

            anchors.fill: parent
            anchors.topMargin: mainPanel.dockedTop
            anchors.bottomMargin: mainPanel.dockedBottom
            anchors.leftMargin: mainPanel.dockedLeft
            anchors.rightMargin: mainPanel.dockedRight
            sidePanelLeftInset: !mainPanel.controller.docked && mainPanel.atLeft && mainPanel.controller.enabled ? mainPanel.extent : 0
            sidePanelRightInset: !mainPanel.controller.docked && mainPanel.atRight && mainPanel.controller.enabled ? mainPanel.extent : 0
            floatingPanelVisible: !mainPanel.controller.docked && mainPanel.visible
            controller: root.viewportController
            backgroundColor: root.translucentBackground ? "transparent" : root.viewerBackground
            focus: parent.visible && !root.overlays.keyboardOverlayOpen && !root.crop.active
        }

        CropOverlay {
            anchors.fill: viewport
            controller: root.crop
        }

        MainPanel {
            id: mainPanel

            controller: root.thumbnailPanel
        }
    }

    SidePanel {
        id: sidePanel

        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.right: parent.right
        crop: root.crop
        focus: root.crop.active && !root.overlays.keyboardOverlayOpen
    }

    // Folder mode. The folder view is created on its first activation, in
    // the background once the window is shown; until then the page keeps
    // the action shortcuts working.
    FocusScope {
        anchors.fill: parent
        visible: root.windowShell.folderViewActive
        focus: visible && !root.overlays.keyboardOverlayOpen
        Keys.onPressed: event => event.accepted = Actions.handleKeyEvent(event)

        Loader {
            anchors.fill: parent
            active: root.folderView.created
            // A view created while the window is still hidden (cold start in
            // folder mode) is created at once: incubated asynchronously, its
            // grid would incubate the visible cells too and fill them in
            // over the frames after the window was revealed.
            asynchronous: root.visible
            focus: true
            // The view anchors itself: an asynchronous Loader sizes its item
            // only when the incubation starts and once it is ready, so a
            // window maximized in between would leave the view and the
            // range its thumbnails are prepared for at the restored size.
            sourceComponent: FolderView {
                anchors.fill: parent
                controller: root.folderView
                focus: true
            }
        }
    }

    OverlayLayer {
        anchors.fill: parent
        coordinator: root.overlays
        focus: root.overlays.keyboardOverlayOpen
        bottomInset: !root.windowShell.folderViewActive ? mainPanel.dockedBottom : 0
        bottomFloatingPanelVisible: !root.windowShell.folderViewActive && !mainPanel.controller.docked && mainPanel.atBottom && mainPanel.visible
    }

    Connections {
        target: root.contextMenu

        function onOpenChanged() {
            if (root.contextMenu.open)
                root.contextMenuCreated = true;
        }
    }

    Loader {
        active: root.contextMenuCreated
        sourceComponent: ViewerContextMenu {
            menuModel: root.contextMenu
        }
    }

    // Modal dialog windows of the dialog port, created on first use.
    DialogLayer {
        coordinator: root.dialogs
    }

    // The settings window, created when it is first opened.
    Loader {
        active: root.settingsDialog.created
        sourceComponent: SettingsDialog {
            controller: root.settingsDialog
            transientParent: root
        }
    }

    // Files dropped anywhere on the window open like in the widget UI; the
    // drop areas of the folder view lie above it.
    DropArea {
        anchors.fill: parent
        z: -1
        onEntered: drag => drag.accepted = drag.hasUrls
        onDropped: drop => {
            root.windowShell.dropUrls(drop.urls, drop.source);
            drop.accept();
        }
    }
}
