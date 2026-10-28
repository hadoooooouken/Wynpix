import QtQuick
import QtQuick.Templates as T
import QtTest
import wynpix.ui
import wynpix.tests

// The viewer's context menu over the Fixture's ContextMenuModel: it opens as
// an item popup in the window, shows the shortcuts of the action system, runs
// the rows' actions, expands "More" in place while it or a row under it is
// current and lists the scripts with "Configure menu".
Item {
    id: root

    // Room for the menu with "More" expanded: an item popup is confined to
    // the window.
    width: 600
    height: 900

    ViewerContextMenu {
        id: menu

        menuModel: Fixture.contextMenu
    }

    TestCase {
        id: testCase

        // Items before the instantiated rows: the zoom row, the transform row
        // and their separator (ViewerContextMenu.fixedItemCount).
        readonly property int fixedItems: 3
        readonly property int transformRowItem: 1
        // Rows of ContextMenuModel.items.
        readonly property int panoramaRow: 1
        readonly property int copyRow: 5
        readonly property int expanderRow: 12
        readonly property int submenuRow: 13
        readonly property int renameRow: 14
        readonly property int rowCount: 20
        readonly property int timeout: 3000

        name: "ContextMenu"
        when: windowShown

        function row(index) {
            return menu.itemAt(fixedItems + index);
        }

        function openMenu(scripts) {
            Fixture.showContextMenu(scripts, false);
            tryVerify(() => menu.opened, timeout, "the menu did not open");
            // The menu window is exposed with its first frame.
            waitForRendering(menu.contentItem);
        }

        function closeMenu() {
            Fixture.closeContextMenu();
            tryVerify(() => !menu.visible, timeout);
        }

        function init() {
            Fixture.showTestImage(64, 64);
            Fixture.clearInputLog();
        }

        function cleanup() {
            closeMenu();
        }

        // The button of action in the transform row.
        function transformButton(action) {
            const buttons = menu.itemAt(transformRowItem).contentItem.children;
            for (let i = 0; i < buttons.length; ++i) {
                if (buttons[i].entryAction === action)
                    return buttons[i];
            }
            return null;
        }

        // Makes row index of the model current, as the pointer or a key
        // does; the menu reports it to the model.
        function makeCurrent(index) {
            menu.currentIndex = fixedItems + index;
        }

        function test_opensAsAnItemPopupWithEveryRow() {
            openMenu([]);
            compare(menu.popupType, T.Popup.Item);
            compare(menu.count, fixedItems + rowCount);
        }

        function test_shownShortcutsAreTheActionShortcuts() {
            Fixture.setShortcut("copyFile", "C");
            Fixture.setShortcut("removeFile", "Shift+Del");
            openMenu([]);
            let checked = 0;
            for (let i = 0; i < rowCount; ++i) {
                const item = row(i);
                if (item.entryAction === undefined || item.entryAction === "")
                    continue;
                compare(item.shortcutText, Actions.shortcutFor(item.entryAction), item.entryAction);
                ++checked;
            }
            verify(checked > 10);
            compare(row(copyRow).shortcutText, "C");

            // Edits reach the open menu.
            Fixture.setShortcut("copyFile", "Ctrl+Shift+C");
            compare(row(copyRow).shortcutText, "Ctrl+Shift+C");
        }

        function test_rowRunsItsActionAndCloses() {
            // The fake dispatcher runs the actions it has a shortcut for.
            Fixture.setShortcut("copyFile", "C");
            openMenu([]);
            const copy = row(copyRow);
            compare(copy.text, "Quick copy");
            verify(copy.enabled);
            mouseClick(copy);
            compare(Fixture.lastInvoked, "copyFile");
            tryVerify(() => !menu.visible, timeout);
            verify(!Fixture.contextMenu.open);
        }

        function test_buttonsActOnClick() {
            Fixture.setShortcut("fitWindow", "1");
            openMenu([]);
            const rows = menu.itemAt(0);
            const zoomRow = rows.children[0];
            let fitWindow = null;
            for (let i = 0; i < zoomRow.children.length; ++i) {
                if (zoomRow.children[i].entryAction === "fitWindow")
                    fitWindow = zoomRow.children[i];
            }
            verify(fitWindow !== null, "no fitWindow button");
            tryVerify(() => fitWindow.Window.window !== null, timeout, "the button is in no window");
            mousePress(fitWindow);
            compare(Fixture.lastInvoked, "");
            mouseRelease(fitWindow);
            compare(Fixture.lastInvoked, "fitWindow");
            // The menu stays open for further zooming, as the widget menu did.
            verify(menu.opened, "a button closed the menu");
            verify(Fixture.contextMenu.open);
        }

        function test_cropButtonRunsOnSingleClick() {
            Fixture.setShortcut("crop", "X");
            openMenu([]);
            const crop = transformButton("crop");
            verify(crop !== null, "no crop button");
            mouseClick(crop);
            compare(Fixture.lastInvoked, "crop");
            verify(menu.opened, "crop closed the menu in the test dispatcher");
        }

        function test_resizeButtonRunsOnSingleClickAndClosesMenu() {
            Fixture.setShortcut("resize", "R");
            openMenu([]);
            const resize = transformButton("resize");
            verify(resize !== null, "no resize button");
            mouseClick(resize);
            compare(Fixture.lastInvoked, "resize");
            tryVerify(() => !menu.visible, timeout);
            verify(!Fixture.contextMenu.open);
        }

        function test_moreExpandsInPlace() {
            openMenu([]);
            const rename = row(renameRow);
            verify(!rename.visible);
            compare(rename.height, 0);
            const heightBefore = menu.height;
            makeCurrent(expanderRow);
            tryVerify(() => rename.visible && rename.height > 0, timeout);
            tryVerify(() => menu.height > heightBefore, timeout);
            compare(menu.width, menu.implicitWidth);
            // A press on "More" does not trigger it.
            mouseClick(row(expanderRow));
            verify(menu.opened, "a click on More closed the menu");

            // A row under it keeps it expanded, a row above collapses it.
            makeCurrent(renameRow);
            tryCompare(Fixture.contextMenu, "moreExpanded", true);
            makeCurrent(copyRow);
            tryVerify(() => !rename.visible, timeout);

            // Collapsed again when the menu opens next time.
            makeCurrent(expanderRow);
            tryVerify(() => rename.visible, timeout);
            closeMenu();
            openMenu([]);
            verify(!row(renameRow).visible);
        }

        function test_imageRowsAreDisabledWithoutAnImage() {
            Fixture.closeTestImage();
            openMenu([]);
            verify(!row(copyRow).enabled);
            verify(!row(panoramaRow).enabled);
        }

        function test_scriptsSubmenu() {
            openMenu(["Edit", "Upload"]);
            makeCurrent(submenuRow);
            const scriptsRow = row(submenuRow);
            // The menu's column places the expanded rows on its next polish.
            tryVerify(() => scriptsRow.visible && scriptsRow.y > row(expanderRow).y, timeout,
                      "the expanded rows were not laid out");
            // The expanded rows fit in the window.
            verify(scriptsRow.mapToItem(null, 0, scriptsRow.height).y <= root.height,
                   "the expanded rows lie outside the window");
            verify(scriptsRow.subMenu !== null);
            const scripts = scriptsRow.subMenu;
            compare(scripts.title, "Open with...");
            // Two scripts, the separator, "Configure menu".
            compare(scripts.count, 4);
            compare(scripts.itemAt(1).text, "Upload");
            // The Right key on its row opens it as a cascade. (A synthetic
            // click opens it on the press and closes it again on the release.)
            keyClick(Qt.Key_Right);
            tryVerify(() => scripts.opened, timeout, "the scripts submenu did not open");
            waitForRendering(scripts.contentItem);
            mouseClick(scripts.itemAt(3));
            compare(Fixture.scriptSettingsRequests, 1);
            tryVerify(() => !menu.visible, timeout);
        }

        // Saved next to the test executable for review.
        function test_rendersForReview() {
            Fixture.setShortcut("copyFile", "C");
            openMenu(["Edit"]);
            makeCurrent(expanderRow);
            tryVerify(() => row(renameRow).visible && row(renameRow).y > row(expanderRow).y, timeout);
            waitForRendering(menu.contentItem);
            grabImage(menu.contentItem).save(Fixture.artifactPath("contextmenu.png"));
        }

        function test_escapeAndTheFolderViewClose() {
            openMenu([]);
            keyClick(Qt.Key_Escape);
            tryVerify(() => !menu.visible, timeout);
            verify(!Fixture.contextMenu.open);

            openMenu([]);
            Fixture.setFolderViewActive(true);
            tryVerify(() => !menu.visible, timeout);
            Fixture.showContextMenu([], false);
            verify(!Fixture.contextMenu.open);
            Fixture.setFolderViewActive(false);
        }
    }
}
