import QtQuick
import QtTest
import wynpix.bridges
import wynpix.ui
import wynpix.tests

// The thumbnail panel over the Fixture's ThumbnailPanelController and
// ThumbnailListModel: the content waits for the creation permission, a
// pinned panel takes its place at each side, a large directory keeps the
// delegates and the requests to the visible range, delivered thumbnails are
// drawn, pointer input reaches the model, and a floating panel slides in.
// The panel is a sibling of the TestCase, which is not visible and would not
// get input.
Item {
    id: root

    width: 1200
    height: 800

    MainPanel {
        id: panel

        controller: Fixture.thumbnailPanel
    }

    TestCase {
        id: testCase

        readonly property int createTimeout: 3000
        readonly property int largeDirectory: 10000
        // More than a screen of cells plus the reuse pool, far fewer than
        // the directory.
        readonly property int delegateLimit: 60
        // The preloaded range of kStripPreloadDistance on both sides, in cells
        // of the test layout (cell width 80 + 2 * (9 + 2) = 102 pixels).
        readonly property int requestLimit: 80
        readonly property int visiblePanelButtonCount: 6
        readonly property int previewSizeMin: 104
        readonly property int previewSizeMax: 512
        readonly property int previewSizeStep: 24
        readonly property int previewSizeGridStep: 8

        name: "ThumbnailStrip"
        when: windowShown

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

        function findText(item, text) {
            if (item.text === text && item.visible)
                return item;
            for (let i = 0; i < item.children.length; ++i) {
                const found = findText(item.children[i], text);
                if (found)
                    return found;
            }
            return null;
        }

        function findNamed(item, objectName) {
            if (item.objectName === objectName)
                return item;
            for (let i = 0; i < item.children.length; ++i) {
                const found = findNamed(item.children[i], objectName);
                if (found)
                    return found;
            }
            return null;
        }

        function collectButtons(item) {
            const result = [];
            const visit = current => {
                if (current.glyph !== undefined && current.active !== undefined && current.visible)
                    result.push(current);
                for (let i = 0; i < current.children.length; ++i)
                    visit(current.children[i]);
            };
            visit(item);
            return result;
        }

        function findButtonsByGlyph(item, glyph) {
            const matches = [];
            const buttons = collectButtons(item);
            for (let i = 0; i < buttons.length; ++i) {
                if (buttons[i].glyph === glyph)
                    matches.push(buttons[i]);
            }
            return matches;
        }

        function findButtonByGlyph(item, glyph) {
            const matches = findButtonsByGlyph(item, glyph);
            return matches.length > 0 ? matches[0] : null;
        }

        // The strip's ListView once the asynchronous content exists.
        function list() {
            tryVerify(() => findType(panel, "QQuickListView") !== null, createTimeout,
                      "the strip was not created");
            return findType(panel, "QQuickListView");
        }

        function cells(view) {
            const result = [];
            const children = view.contentItem.children;
            for (let i = 0; i < children.length; ++i) {
                if (children[i].toString().startsWith("ThumbnailWidget") && children[i].visible)
                    result.push(children[i]);
            }
            return result;
        }

        function cellOf(view, index) {
            const all = cells(view);
            for (let i = 0; i < all.length; ++i) {
                if (all[i].index === index)
                    return all[i];
            }
            return null;
        }

        function pinnedAt(position) {
            Fixture.configureThumbnailPanel(true, position, false);
            Fixture.setPanelWindowSize(root.width, root.height);
        }

        function initTestCase() {
            Fixture.setShortcut("folderView", "");
            Fixture.setShortcut("openSettings", "");
        }

        function cleanup() {
            Fixture.setPanelMasonry(false);
            Fixture.configureThumbnailPanel(true, SettingsEnums.PanelPosition.Bottom, false);
        }

        // Runs first (alphabetical order): creation was not allowed yet.
        function test_aaContentWaitsForTheFirstDocument() {
            pinnedAt(SettingsEnums.PanelPosition.Bottom);
            Fixture.populateThumbnails(20);
            verify(panel.visible);
            verify(panel.controller.docked);
            compare(panel.dockedBottom, panel.controller.layout.panelExtent);
            compare(findType(panel, "QQuickListView"), null);
            compare(Fixture.thumbnailRequestCount, 0);

            Fixture.allowThumbnailPanelCreation();
            list();
            tryVerify(() => Fixture.thumbnailRequestCount > 0, createTimeout);
            compare(Fixture.requestedThumbnailCount, 20);
        }

        function test_pinnedPanelTakesItsSide_data() {
            return [
                { tag: "top", position: SettingsEnums.PanelPosition.Top, horizontal: true },
                { tag: "bottom", position: SettingsEnums.PanelPosition.Bottom, horizontal: true },
                { tag: "left", position: SettingsEnums.PanelPosition.Left, horizontal: false },
                { tag: "right", position: SettingsEnums.PanelPosition.Right, horizontal: false }
            ];
        }

        function test_pinnedPanelTakesItsSide(data) {
            pinnedAt(data.position);
            const extent = panel.controller.layout.panelExtent;
            const view = list();
            compare(view.orientation, data.horizontal ? ListView.Horizontal : ListView.Vertical);
            if (data.horizontal) {
                compare(panel.width, root.width);
                compare(panel.height, extent);
                compare(panel.y, data.position === SettingsEnums.PanelPosition.Top ? 0
                                                                                 : root.height - extent);
            } else {
                compare(panel.height, root.height);
                compare(panel.width, extent);
                compare(panel.x, data.position === SettingsEnums.PanelPosition.Left ? 0
                                                                                  : root.width - extent);
            }
            const docked = panel.dockedTop + panel.dockedBottom + panel.dockedLeft + panel.dockedRight;
            compare(docked, extent);
        }

        // The strip outlives a change of side, so the scroll bar of the new
        // orientation must still be shown with its marker afterwards.
        function test_scrollBarSurvivesAnOrientationChange() {
            pinnedAt(SettingsEnums.PanelPosition.Bottom);
            const view = list();
            Fixture.populateThumbnails(largeDirectory);
            Fixture.selectThumbnail(0);
            const sides = [
                { position: SettingsEnums.PanelPosition.Left, horizontal: false },
                { position: SettingsEnums.PanelPosition.Bottom, horizontal: true },
                { position: SettingsEnums.PanelPosition.Right, horizontal: false },
                { position: SettingsEnums.PanelPosition.Top, horizontal: true }
            ];
            for (const side of sides) {
                pinnedAt(side.position);
                compare(list(), view, "the strip was recreated");
                compare(view.orientation, side.horizontal ? ListView.Horizontal : ListView.Vertical);
                const bar = findNamed(view.parent, side.horizontal ? "horizontalScrollBar"
                                                                   : "verticalScrollBar");
                const idle = findNamed(view.parent, side.horizontal ? "verticalScrollBar"
                                                                    : "horizontalScrollBar");
                verify(bar !== null && idle !== null);
                compare(bar.parent, view.parent);
                verify(bar.visible, "the scroll bar is hidden");
                verify(bar.size < 1.0);
                verify(bar.contentItem.visible, "the scroll bar handle is hidden");
                verify(!idle.visible, "the other orientation's scroll bar is shown");
            }
        }

        // The masonry mode draws its tiles in a view of their own and leaves
        // the list without delegates; the mode button switches back.
        function test_masonryModeDrawsTilesInPlaceOfTheList() {
            pinnedAt(SettingsEnums.PanelPosition.Bottom);
            const view = list();
            Fixture.populateThumbnails(20);
            verify(cells(view).length > 0);
            const tiles = findNamed(panel, "masonryTiles");
            verify(tiles !== null);
            verify(!tiles.visible);

            panel.controller.toggleMasonry();
            verify(panel.stripLayout.masonry);
            verify(tiles.visible);
            verify(!view.visible);
            compare(cells(view).length, 0);
            tryVerify(() => cells(tiles).length > 0, createTimeout, "no tiles were drawn");
            const bar = findNamed(panel, "masonryHorizontalScrollBar");
            const idle = findNamed(panel, "horizontalScrollBar");
            verify(bar !== null && idle !== null);
            verify(bar.visible, "the scroll bar of the tiles is hidden");
            verify(bar.size < 1.0);
            verify(!idle.visible, "the scroll bar of the list is shown");

            panel.controller.toggleMasonry();
            verify(!panel.stripLayout.masonry);
            verify(view.visible);
            verify(!tiles.visible);
            tryVerify(() => cells(view).length > 0, createTimeout, "the list has no cells");
        }

        // A Flickable keeps its position across the strip, so the tiles of a
        // strip that turned would have stayed out of view.
        function test_masonryTilesStayInViewWhenTheStripTurns() {
            pinnedAt(SettingsEnums.PanelPosition.Bottom);
            list();
            Fixture.setPanelMasonry(true);
            Fixture.populateThumbnails(largeDirectory / 10);
            const tiles = findNamed(panel, "masonryTiles");
            verify(tiles !== null);
            Fixture.selectThumbnail(500);
            tryVerify(() => tiles.contentX > 0, createTimeout, "the selection was not brought into view");
            const sides = [
                { position: SettingsEnums.PanelPosition.Left, horizontal: false },
                { position: SettingsEnums.PanelPosition.Bottom, horizontal: true },
                { position: SettingsEnums.PanelPosition.Right, horizontal: false }
            ];
            for (const side of sides) {
                pinnedAt(side.position);
                verify(panel.stripLayout.masonry);
                tryVerify(() => cells(tiles).length > 0, createTimeout, "no tiles in view");
                // Nothing is scrolled across the strip.
                compare(side.horizontal ? tiles.contentY : tiles.contentX, 0);
                for (const cell of cells(tiles)) {
                    const along = side.horizontal ? cell.x - tiles.contentX : cell.y - tiles.contentY;
                    verify(along > -cell.width - cell.height && along < tiles.width + tiles.height,
                           "a tile is far out of view");
                }
            }
        }

        // The selection stays in view across the switch, in both directions:
        // the list is given its model again at the same moment as it is
        // scrolled to the selection.
        function test_masonryToggleKeepsTheSelectionInView() {
            const selected = 150;
            pinnedAt(SettingsEnums.PanelPosition.Bottom);
            const view = list();
            Fixture.populateThumbnails(200);
            Fixture.selectThumbnail(selected);
            tryVerify(() => cellOf(view, selected) !== null, createTimeout,
                      "the selection was not brought into view");
            const tiles = findNamed(panel, "masonryTiles");

            panel.controller.toggleMasonry();
            verify(tiles.visible);
            tryVerify(() => tiles.contentX > 0, createTimeout, "the tiles stayed at the start");
            tryVerify(() => cells(tiles).some(cell => cell.itemIndex === selected), createTimeout,
                      "the selection is not among the tiles");

            panel.controller.toggleMasonry();
            verify(view.visible);
            tryVerify(() => view.contentX > 0, createTimeout, "the list stayed at the start");
            tryVerify(() => cellOf(view, selected) !== null, createTimeout,
                      "the selection is not among the cells");
        }

        function test_largeDirectoryKeepsToTheVisibleRange() {
            pinnedAt(SettingsEnums.PanelPosition.Bottom);
            const view = list();
            Fixture.populateThumbnails(largeDirectory);
            const atStart = Fixture.requestedThumbnailCount;
            verify(atStart > 0 && atStart <= requestLimit, "requested " + atStart);
            Fixture.selectThumbnail(largeDirectory / 2);
            const atSelection = Fixture.requestedThumbnailCount - atStart;
            verify(atSelection > 0 && atSelection <= requestLimit, "requested " + atSelection);
            verify(cells(view).length <= delegateLimit, "delegates " + cells(view).length);
            verify(view.contentX > 0, "the selection was not brought into view");
            Fixture.deliverRequestedThumbnails();

            // Scrolling with the wheel moves by items and requests only what
            // came into range.
            const before = Fixture.requestedThumbnailCount;
            const start = view.contentX;
            for (let i = 0; i < 20; ++i)
                mouseWheel(view, view.width / 2, view.height / 2, 0, -120);
            verify(view.contentX > start);
            verify(Fixture.requestedThumbnailCount - before <= requestLimit);
            verify(cells(view).length <= delegateLimit, "delegates " + cells(view).length);
        }

        function test_deliveredThumbnailsAreDrawn() {
            pinnedAt(SettingsEnums.PanelPosition.Bottom);
            const view = list();
            Fixture.populateThumbnails(30);
            Fixture.selectThumbnail(0);
            tryVerify(() => Fixture.thumbnailRequestCount > 0, createTimeout);
            // The selection requests its thumbnails synchronously, before the
            // ListView has polished and created the delegate.
            tryVerify(() => cellOf(view, 0) !== null, createTimeout);
            verify(Fixture.deliverRequestedThumbnails() > 0);
            const cell = cellOf(view, 0);
            verify(cell.loaded);
            verify(cell.selected);
            const image = findType(cell, "ThumbnailItem");
            verify(image.visible);
            verify(image.paintedRect.width > 0 && image.paintedRect.height > 0);
            // The 4:3 thumbnail fills the 4:3 image area.
            compare(image.paintedRect.width, panel.controller.layout.imageArea.width);
        }

        function test_extendedStyleShowsTheLabels() {
            Fixture.configureThumbnailPanel(true, SettingsEnums.PanelPosition.Left, true);
            Fixture.setPanelWindowSize(root.width, root.height);
            list();
            Fixture.populateThumbnails(5);
            Fixture.selectThumbnail(1);
            tryVerify(() => Fixture.thumbnailRequestCount > 0, createTimeout);
            Fixture.deliverRequestedThumbnails();
            tryVerify(() => findText(panel, "item 1") !== null, createTimeout);
            verify(findText(panel, "64 x 48") !== null);
        }

        function test_pressActivatesTheItemUnderThePointer() {
            pinnedAt(SettingsEnums.PanelPosition.Bottom);
            const view = list();
            Fixture.populateThumbnails(10);
            Fixture.selectThumbnail(0);
            tryVerify(() => cellOf(view, 2) !== null, createTimeout);
            const cell = cellOf(view, 2);
            mousePress(cell, cell.width / 2, cell.height / 2);
            mouseRelease(cell, cell.width / 2, cell.height / 2);
            compare(Fixture.lastActivatedThumbnail, 2);
        }

        function test_pinButtonPublishesTheChange() {
            pinnedAt(SettingsEnums.PanelPosition.Bottom);
            list();
            // The panel buttons have a glyph and the checked look (active).
            const buttons = collectButtons(panel);
            compare(buttons.length, visiblePanelButtonCount);
            let pinButton = null;
            for (let i = 0; i < buttons.length; ++i) {
                if (buttons[i].active)
                    pinButton = buttons[i];
            }
            verify(pinButton !== null, "the pin button does not show the pinned state");
            mouseClick(pinButton);
            compare(Fixture.lastPinRequest, 0);
            verify(!panel.controller.pinned);
            verify(!pinButton.active);
        }

        function test_previewSizeButtonsAdjustTheStrip() {
            pinnedAt(SettingsEnums.PanelPosition.Bottom);
            list();
            const zoomIn = findButtonByGlyph(panel, FluentIcons.ZoomIn20);
            const zoomOut = findButtonByGlyph(panel, FluentIcons.ZoomOut20);
            verify(zoomIn !== null && zoomOut !== null, "preview size buttons are missing");

            // In a horizontal panel the +/- buttons sit at the bottom-left.
            const inPos = zoomIn.mapToItem(panel, 0, 0);
            const outPos = zoomOut.mapToItem(panel, 0, 0);
            verify(inPos.x < panel.width / 2 && outPos.x < panel.width / 2);
            verify(outPos.y > inPos.y);
            verify(zoomIn.enabled);
            verify(!zoomOut.enabled);

            // Stepping up crosses into the settings slider range and enables "-".
            mouseClick(zoomIn);
            compare(panel.controller.previewSize, previewSizeMin);
            compare(Fixture.lastPreviewSizeRequest, previewSizeMin);
            verify(!zoomOut.enabled);

            const previousExtent = panel.extent;
            mouseClick(zoomIn);
            compare(panel.controller.previewSize, previewSizeMin + previewSizeStep);
            compare(Fixture.lastPreviewSizeRequest, previewSizeMin + previewSizeStep);
            verify(zoomOut.enabled);
            verify(panel.extent > previousExtent);

            mouseClick(zoomOut);
            compare(panel.controller.previewSize, previewSizeMin);
            compare(Fixture.lastPreviewSizeRequest, previewSizeMin);
            verify(!zoomOut.enabled);

            // Increasing immediately after disabling "-" at minimum must not bounce back.
            mouseClick(zoomIn);
            compare(panel.controller.previewSize, previewSizeMin + previewSizeStep);
            compare(Fixture.lastPreviewSizeRequest, previewSizeMin + previewSizeStep);
            verify(zoomOut.enabled);
            wait(panel.pageRepeatDelay + panel.pageRepeatInterval);
            compare(panel.controller.previewSize, previewSizeMin + previewSizeStep);

            // Holding "-" down to minimum and then clicking "+" also stays increased.
            mousePress(zoomOut);
            tryVerify(() => panel.controller.previewSize === previewSizeMin,
                      createTimeout, "holding - did not reach minimum");
            mouseRelease(zoomOut);
            verify(!zoomOut.enabled);
            mouseClick(zoomIn);
            compare(panel.controller.previewSize, previewSizeMin + previewSizeStep);
            wait(panel.pageRepeatDelay + panel.pageRepeatInterval);
            compare(panel.controller.previewSize, previewSizeMin + previewSizeStep);

            mouseClick(zoomOut);
            compare(panel.controller.previewSize, previewSizeMin);
            verify(!zoomOut.enabled);

            // Holding "+" triggers auto-repeat steps.
            mousePress(zoomIn);
            tryVerify(() => panel.controller.previewSize >= previewSizeMin + 2 * previewSizeStep,
                      createTimeout, "holding + did not auto-repeat");
            mouseRelease(zoomIn);

            // Clamps at the upper bound and disables "+".
            Fixture.setPanelPreviewsSize(previewSizeMax - previewSizeGridStep);
            verify(zoomIn.enabled);
            mouseClick(zoomIn);
            compare(panel.controller.previewSize, previewSizeMax);
            compare(Fixture.lastPreviewSizeRequest, previewSizeMax);
            verify(!zoomIn.enabled);
            verify(zoomOut.enabled);

            // In a vertical panel the +/- buttons sit at the bottom of the panel.
            pinnedAt(SettingsEnums.PanelPosition.Left);
            const vInPos = zoomIn.mapToItem(panel, 0, 0);
            const vOutPos = zoomOut.mapToItem(panel, 0, 0);
            verify(vInPos.y > panel.height / 2 && vOutPos.y > panel.height / 2);
            verify(vOutPos.x > vInPos.x);
            const previousWidth = panel.width;
            mouseClick(zoomIn);
            compare(Fixture.lastPreviewSizeRequest, previewSizeMin);
            verify(panel.width > previousWidth);
        }

        function test_pageButtonsLiveAtTopOfSecondaryBar() {
            pinnedAt(SettingsEnums.PanelPosition.Bottom);
            Fixture.setPanelPreviewsSize(previewSizeMin);
            const view = list();
            Fixture.populateThumbnails(50);

            const pageButtons = findButtonsByGlyph(panel, FluentIcons.ChevronDown20);
            compare(pageButtons.length, 2);
            const backward = pageButtons[0];
            const forward = pageButtons[1];
            const bPos = backward.mapToItem(panel, 0, 0);
            const fPos = forward.mapToItem(panel, 0, 0);
            const inPos = findButtonByGlyph(panel, FluentIcons.ZoomIn20).mapToItem(panel, 0, 0);
            // Left column (secondary bar), above the preview size buttons.
            verify(bPos.x < panel.width / 2 && fPos.x < panel.width / 2);
            verify(bPos.y < fPos.y && fPos.y < inPos.y);
            verify(!backward.enabled);
            verify(forward.enabled);

            mouseClick(forward);
            tryVerify(() => view.contentX > 0, createTimeout, "forward page button did not scroll");
            verify(backward.enabled);

            // In a vertical panel the page buttons sit at the bottom-right end.
            pinnedAt(SettingsEnums.PanelPosition.Left);
            const vbPos = backward.mapToItem(panel, 0, 0);
            const vfPos = forward.mapToItem(panel, 0, 0);
            const vOutPos = findButtonByGlyph(panel, FluentIcons.ZoomOut20).mapToItem(panel, 0, 0);
            verify(vbPos.y > panel.height / 2 && vfPos.y > panel.height / 2);
            verify(vbPos.x > vOutPos.x && vfPos.x > vbPos.x);
        }

        function test_panelButtonsKeepTheirGeometryWhenThePanelTurns() {
            pinnedAt(SettingsEnums.PanelPosition.Bottom);
            Fixture.setPanelPreviewsSize(previewSizeMin);
            list();

            const sides = [
                { position: SettingsEnums.PanelPosition.Right, horizontal: false },
                { position: SettingsEnums.PanelPosition.Bottom, horizontal: true },
                { position: SettingsEnums.PanelPosition.Left, horizontal: false },
                { position: SettingsEnums.PanelPosition.Top, horizontal: true },
                { position: SettingsEnums.PanelPosition.Bottom, horizontal: true }
            ];
            for (const side of sides) {
                pinnedAt(side.position);
                compare(panel.horizontal, side.horizontal);

                const masonry = findButtonByGlyph(panel, FluentIcons.Masonry20);
                const settings = findButtonByGlyph(panel, FluentIcons.Settings20);
                const pageButtons = findButtonsByGlyph(panel, FluentIcons.ChevronDown20);
                verify(masonry !== null && settings !== null);
                compare(pageButtons.length, 2);

                for (const button of [masonry, settings, pageButtons[0], pageButtons[1]]) {
                    compare(button.width, panel.buttonSize);
                    compare(button.height, panel.buttonSize);
                    const pos = button.mapToItem(panel, 0, 0);
                    verify(pos.x >= 0 && pos.x + button.width <= panel.width);
                    verify(pos.y >= 0 && pos.y + button.height <= panel.height);
                }

                const mPos = masonry.mapToItem(panel, 0, 0);
                const sPos = settings.mapToItem(panel, 0, 0);
                const bPos = pageButtons[0].mapToItem(panel, 0, 0);
                const fPos = pageButtons[1].mapToItem(panel, 0, 0);
                if (side.horizontal) {
                    compare(mPos.x, sPos.x);
                    compare(mPos.y + masonry.height, sPos.y);
                    compare(bPos.x, fPos.x);
                    compare(bPos.y + pageButtons[0].height, fPos.y);
                } else {
                    compare(sPos.y, mPos.y);
                    compare(sPos.x + settings.width, mPos.x);
                    compare(bPos.y, fPos.y);
                    compare(bPos.x + pageButtons[0].width, fPos.x);
                }
            }
        }

        function test_floatingPanelSlidesIn() {
            Fixture.configureThumbnailPanel(false, SettingsEnums.PanelPosition.Bottom, false);
            Fixture.setPanelWindowSize(root.width, root.height);
            // An unpinned panel stays until the pointer is away from it.
            Fixture.panelPointerMoved(Qt.point(root.width / 2, root.height / 2), Qt.NoButton);
            tryCompare(panel, "visible", false, createTimeout);
            compare(panel.dockedBottom, 0);
            Fixture.panelPointerMoved(Qt.point(root.width / 2, root.height - 2), Qt.NoButton);
            verify(panel.controller.shown);
            verify(panel.visible);
            tryCompare(panel, "slide", 0, createTimeout);
            compare(panel.dockedBottom, 0);
            // Away from the panel it slides out after the hide delay.
            Fixture.panelPointerMoved(Qt.point(root.width / 2, root.height / 2), Qt.NoButton);
            tryCompare(panel, "visible", false, createTimeout);
        }
    }
}
