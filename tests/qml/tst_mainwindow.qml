import QtQuick
import QtTest
import wynpix.bridges
import wynpix.tests

TestCase {
    id: testCase

    name: "MainWindow"

    readonly property url droppedFile: "file:///C:/images/dropped.png"
    readonly property real halfOpacity: 0.5
    readonly property real opaque: 1.0
    readonly property real alphaTolerance: 0.01

    function createWindow() {
        const component = Qt.createComponent("wynpix.ui", "Main");
        compare(component.status, Component.Ready, component.errorString());
        const window = createTemporaryObject(component, testCase, {
            viewportController: Fixture.viewportController,
            windowShell: Fixture.windowShell,
            overlays: Fixture.overlays,
            thumbnailPanel: Fixture.thumbnailPanel,
            contextMenu: Fixture.contextMenu,
            crop: Fixture.crop,
            folderView: Fixture.folderView,
            dialogs: Fixture.dialogs,
            settingsDialog: Fixture.settingsDialog
        });
        verify(window !== null, "wynpix.ui/Main could not be instantiated");
        return window;
    }

    function findType(item, typeName) {
        if (item.toString().startsWith(typeName))
            return item;
        for (let i = 0; i < item.children.length; ++i) {
            const found = findType(item.children[i], typeName);
            if (found)
                return found;
        }
        return null;
    }

    function childOfType(window, typeName) {
        return findType(window.contentItem, typeName);
    }

    function cleanup() {
        Fixture.setFolderViewActive(false);
        Fixture.setFullscreen(false);
        Fixture.setWindowedBackgroundOpacity(testCase.opaque);
        Fixture.setTranslucentWindow(false);
    }

    function test_createsHiddenFromModule() {
        const window = createWindow();
        // Core shows the window through its port.
        verify(!window.visible);
        compare(window.width, window.initialWidth);
        compare(window.height, window.initialHeight);
        verify(Qt.colorEqual(window.color, Theme.colors.background));
    }

    function test_fullscreenUsesTheFullscreenBackground() {
        const window = createWindow();
        Fixture.setFullscreen(true);
        verify(Qt.colorEqual(window.color, Theme.colors.backgroundFullscreen));
        Fixture.setFullscreen(false);
        verify(Qt.colorEqual(window.color, Theme.colors.background));
    }

    function test_viewerClearsToTheFullscreenBackgroundInFullscreen() {
        const window = createWindow();
        const viewport = childOfType(window, "ImageViewport");
        verify(viewport !== null, "no ImageViewport in the main window");
        verify(Qt.colorEqual(viewport.backgroundColor, Theme.colors.background));
        Fixture.setFullscreen(true);
        verify(Qt.colorEqual(viewport.backgroundColor, Theme.colors.backgroundFullscreen));
    }

    function test_windowOpacityMakesOnlyTheWindowedBackgroundTranslucent() {
        Fixture.setTranslucentWindow(true);
        const window = createWindow();
        const viewport = childOfType(window, "ImageViewport");
        verify(viewport !== null, "no ImageViewport in the main window");

        Fixture.setWindowedBackgroundOpacity(testCase.halfOpacity);
        fuzzyCompare(window.windowBackground.a, testCase.halfOpacity, testCase.alphaTolerance);
        // The background item carries the alpha alone.
        compare(viewport.backgroundColor.a, 0);

        Fixture.setFullscreen(true);
        compare(window.windowBackground.a, testCase.opaque);
        verify(Qt.colorEqual(viewport.backgroundColor, Theme.colors.backgroundFullscreen));
        Fixture.setFullscreen(false);

        Fixture.setFolderViewActive(true);
        compare(window.windowBackground.a, testCase.opaque);
    }

    // QQuickWindow::setColor() drops the alpha buffer when the colour turns
    // opaque, which would make the window opaque for good.
    function test_translucentWindowColourNeverTurnsOpaque() {
        Fixture.setTranslucentWindow(true);
        Fixture.setFolderViewActive(true);
        const window = createWindow();
        compare(window.color.a, 0);
        Fixture.setFolderViewActive(false);
        Fixture.setWindowedBackgroundOpacity(testCase.halfOpacity);
        compare(window.color.a, 0);
        Fixture.setFullscreen(true);
        compare(window.color.a, 0);
    }

    function test_folderModeReplacesTheViewport() {
        const window = createWindow();
        const viewport = childOfType(window, "ImageViewport");
        verify(viewport !== null, "no ImageViewport in the main window");
        verify(viewport.visible);

        // The folder view is created on its first activation.
        verify(childOfType(window, "FolderView") === null);
        Fixture.setFolderViewActive(true);
        verify(!viewport.visible);
        tryVerify(() => childOfType(window, "FolderView") !== null);
        Fixture.setFolderViewActive(false);
        verify(viewport.visible);
    }

    function test_pinnedPanelTakesItsSpaceFromTheViewport() {
        Fixture.configureThumbnailPanel(true, SettingsEnums.PanelPosition.Bottom, false);
        const window = createWindow();
        Fixture.setPanelWindowSize(window.width, window.height);
        const viewport = childOfType(window, "ImageViewport");
        compare(viewport.height, window.height - Fixture.thumbnailPanel.layout.panelExtent);
        // A floating panel covers the viewport instead.
        Fixture.configureThumbnailPanel(false, SettingsEnums.PanelPosition.Bottom, false);
        compare(viewport.height, window.height);
    }

    function test_cropModeDocksTheSidePanelAndCoversTheViewport() {
        Fixture.configureThumbnailPanel(false, SettingsEnums.PanelPosition.Bottom, false);
        const window = createWindow();
        const viewport = childOfType(window, "ImageViewport");
        const overlay = childOfType(window, "CropOverlay");
        const sidePanel = childOfType(window, "SidePanel");
        verify(overlay !== null && sidePanel !== null, "no crop overlay or side panel");
        verify(!overlay.visible);
        verify(!sidePanel.visible);
        Fixture.showTestImage(64, 64);
        Fixture.toggleCrop(Qt.size(1920, 1080));
        verify(overlay.visible);
        verify(sidePanel.visible);
        compare(viewport.width, window.width - sidePanel.width);
        compare(overlay.width, viewport.width);
        compare(sidePanel.x, window.width - sidePanel.width);
        Fixture.crop.cancel();
        compare(viewport.width, window.width);
    }

    function test_externalFileDropsReachTheShell() {
        const window = createWindow();
        window.visible = true;
        verify(waitForRendering(window.contentItem));

        verify(Fixture.dropExternalFile(window, Qt.point(window.width / 2, window.height / 2),
                                        testCase.droppedFile));
        compare(Fixture.lastDroppedUrls.length, 1);
        compare(Fixture.lastDroppedUrls[0], testCase.droppedFile);
    }
}
