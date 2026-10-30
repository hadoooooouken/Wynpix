import QtQuick
import QtTest
import wynpix.bridges
import wynpix.ui
import wynpix.tests

// The folder view over the Fixture's FolderViewController, FolderGridController
// and their ThumbnailListModel: a large directory keeps the delegates and the
// requests to the visible rows, the pointer selects, activates, draws the
// rubber band and asks for the context menu, keys move and letters do not,
// drops keep their target, the name filter is published and focused by its
// action, the top bar compacts, and the places panel shows the bookmarks. The view is a sibling of the
// TestCase, which is not visible and would not get input.
Item {
    id: root

    width: 1200
    height: 800

    FolderView {
        id: view

        anchors.fill: parent
        controller: Fixture.folderView
        focus: true
    }

    TestCase {
        id: testCase

        readonly property int createTimeout: 3000
        readonly property int largeDirectory: 10000
        readonly property int dirCount: 3
        // More than a screen of cells plus the reuse pool, far fewer than the
        // directory.
        readonly property int delegateLimit: 200
        readonly property int compactWidth: 500
        readonly property int nameFilterTimeout: 1000
        readonly property url droppedFile: "file:///C:/images/dropped.png"
        readonly property int copyAction: 1

        name: "FolderView"
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
            if (item.text === text)
                return item;
            for (let i = 0; i < item.children.length; ++i) {
                const found = findText(item.children[i], text);
                if (found)
                    return found;
            }
            return null;
        }

        function grid() {
            return findType(view, "QQuickGridView");
        }

        function cellOf(index) {
            const children = grid().contentItem.children;
            for (let i = 0; i < children.length; ++i) {
                const child = children[i];
                if (child.toString().startsWith("ThumbnailWidget") && child.visible
                        && child.index === index)
                    return child;
            }
            return null;
        }

        function centreOf(index) {
            const cell = cellOf(index);
            verify(cell !== null, "no cell for item " + index);
            return cell.mapToItem(root, cell.width / 2, cell.height / 2);
        }

        function initTestCase() {
            Fixture.setShortcut("exit", "");
            Fixture.setFolderViewActive(true);
        }

        function init() {
            root.width = 1200;
            Fixture.populateFolder(40, dirCount, "C:/");
            Fixture.deliverFolderThumbnails();
            tryVerify(() => cellOf(0) !== null, createTimeout, "the grid shows no cells");
        }

        function test_compactTopBarHidesTheGridSize() {
            const label = findText(view, qsTranslate("FolderView", "Grid size"));
            verify(label !== null);
            verify(label.visible);
            root.width = compactWidth;
            tryVerify(() => !label.visible);
        }

        function test_doubleClickActivates() {
            const point = centreOf(5);
            mouseClick(root, point.x, point.y);
            compare(Fixture.folderView.grid.model.currentIndex, 5);
            compare(Fixture.lastFolderRequest, "");
            mouseDoubleClickSequence(root, point.x, point.y);
            compare(Fixture.lastFolderRequest, "activated:5");
        }

        function test_dropKeepsItsTarget() {
            const point = centreOf(4);
            verify(Fixture.dropExternalFile(root.Window.window, point, droppedFile));
            compare(Fixture.lastFolderRequest, "drop:4:" + copyAction);
        }

        // The grid's keys are action shortcuts: the grid does not move on
        // its own.
        function test_keysGoToTheActions() {
            const point = centreOf(2);
            mouseClick(root, point.x, point.y);
            Fixture.clearInputLog();
            keyClick(Qt.Key_Right);
            compare(Fixture.lastKey, Qt.Key_Right);
            compare(Fixture.folderView.grid.model.currentIndex, 2);
            keyClick(Qt.Key_X);
            compare(Fixture.lastKey, Qt.Key_X);
            compare(Fixture.lastFolderRequest, "");
        }

        // The mouse back and forward buttons are action shortcuts over the
        // grid ("folderBack", "folderForward") and leave the selection alone.
        function test_backAndForwardButtonsGoToTheActions() {
            const point = centreOf(2);
            mouseClick(root, point.x, point.y);
            Fixture.clearInputLog();
            mousePress(root, point.x, point.y, Qt.BackButton);
            mouseRelease(root, point.x, point.y, Qt.BackButton);
            mousePress(root, point.x, point.y, Qt.ForwardButton);
            mouseRelease(root, point.x, point.y, Qt.ForwardButton);
            compare(Fixture.mouseButtons, [Qt.BackButton, Qt.ForwardButton]);
            compare(Fixture.folderView.grid.model.currentIndex, 2);
        }

        function test_largeDirectoryKeepsTheDelegatesBounded() {
            Fixture.populateFolder(largeDirectory, dirCount, "C:/");
            tryVerify(() => Fixture.folderThumbnailRequestCount > 0, createTimeout);
            const delivered = Fixture.deliverFolderThumbnails();
            verify(delivered > 0 && delivered < largeDirectory / 10,
                   "requested " + delivered + " thumbnails");
            verify(grid().contentItem.children.length < delegateLimit);
            tryVerify(() => cellOf(0) !== null && cellOf(0).loaded, createTimeout);
            // Scrolling to the end requests the last rows.
            const point = centreOf(0);
            mouseClick(root, point.x, point.y);
            Fixture.selectLastFolderItem();
            tryVerify(() => cellOf(largeDirectory - 1) !== null, createTimeout);
            verify(grid().contentItem.children.length < delegateLimit);
        }

        function test_nameFilterIsPublished() {
            const field = findType(view, "SearchField");
            verify(field !== null);
            mouseClick(field);
            keyClick(Qt.Key_A);
            tryCompare(Fixture, "lastFolderRequest", "nameFilter:a", nameFilterTimeout);
            // The field follows the controller.
            Fixture.folderView.editNameFilter("");
            compare(field.text, "");
            tryCompare(Fixture, "lastFolderRequest", "nameFilter:", nameFilterTimeout);

            keyClick(Qt.Key_A);
            tryCompare(Fixture, "lastFolderRequest", "nameFilter:a", nameFilterTimeout);
            // So does the clear button.
            mouseClick(field, field.width - field.rightPadding / 2, field.height / 2);
            compare(Fixture.folderView.nameFilter, "");
            tryCompare(Fixture, "lastFolderRequest", "nameFilter:", nameFilterTimeout);
            // The typing goes on in the field.
            keyClick(Qt.Key_B);
            tryCompare(Fixture, "lastFolderRequest", "nameFilter:b", nameFilterTimeout);
            mouseClick(field, field.width - field.rightPadding / 2, field.height / 2);
            tryCompare(Fixture, "lastFolderRequest", "nameFilter:", nameFilterTimeout);
        }

        // The action focuses the field with its text selected; Enter and Down
        // hand the focus back to the grid and keep the filter.
        function test_nameFilterActionFocusesTheField() {
            const field = findType(view, "SearchField");
            verify(field !== null);
            Fixture.folderView.editNameFilter("ab");
            const keys = [Qt.Key_Return, Qt.Key_Enter, Qt.Key_Down];
            for (const key of keys) {
                const point = centreOf(2);
                mouseClick(root, point.x, point.y);
                Fixture.focusNameFilter();
                verify(field.contentItem.activeFocus, "no focus for key " + key);
                compare(field.contentItem.selectedText, "ab");
                keyClick(key);
                verify(!field.activeFocus, "the field kept the focus on key " + key);
                compare(field.text, "ab");
                // The next key goes to the actions, not to the field.
                Fixture.clearInputLog();
                keyClick(Qt.Key_Right);
                compare(Fixture.lastKey, Qt.Key_Right);
            }
            Fixture.folderView.editNameFilter("");
        }

        // Escape clears the filter first, then hands the focus to the grid.
        function test_escapeClearsTheNameFilterThenLeaves() {
            const field = findType(view, "SearchField");
            verify(field !== null);
            const point = centreOf(2);
            mouseClick(root, point.x, point.y);
            Fixture.folderView.editNameFilter("ab");
            Fixture.focusNameFilter();
            keyClick(Qt.Key_Escape);
            compare(Fixture.folderView.nameFilter, "");
            compare(field.text, "");
            verify(field.contentItem.activeFocus);
            keyClick(Qt.Key_Escape);
            verify(!field.activeFocus);
            Fixture.clearInputLog();
            keyClick(Qt.Key_Right);
            compare(Fixture.lastKey, Qt.Key_Right);
            // The cleared filter is published late; it stays out of the next
            // test.
            tryCompare(Fixture, "lastFolderRequest", "nameFilter:", nameFilterTimeout);
            Fixture.clearFolderRequest();
        }

        function test_placesPanelShowsTheBookmarks() {
            const places = findType(view, "PlacesPanel");
            verify(places !== null);
            verify(!places.visible);
            Fixture.folderView.setPlacesPanelEnabled(true);
            tryVerify(() => places.visible);
            const list = findType(places, "QQuickListView");
            verify(list !== null);
            compare(list.count, Fixture.folderView.bookmarks.items.rowCount());
            verify(list.count > 0);
            // The grid keeps the remaining width.
            verify(grid().x > places.width);
            Fixture.folderView.setPlacesPanelEnabled(false);
            tryVerify(() => !places.visible);
        }

        function test_rightClickAsksForTheMenu() {
            const point = centreOf(6);
            mouseClick(root, point.x, point.y, Qt.RightButton);
            compare(Fixture.lastFolderRequest, "contextMenu");
            compare(Fixture.folderView.grid.model.currentIndex, 6);
            const gridView = findType(view, "FolderGridView");
            verify(gridView !== null);
            tryVerify(() => gridView.contextMenuCreated);
        }

        function test_rubberBandSelectsTheTouchedCells() {
            const first = cellOf(0);
            const topLeft = first.mapToItem(root, 0, 0);
            const second = centreOf(1);
            // From the left margin, beside the first cell, into the second one.
            mousePress(root, topLeft.x - 4, topLeft.y + 4);
            mouseMove(root, second.x, second.y, -1, Qt.LeftButton);
            const band = Fixture.folderView.grid.rubberBand;
            verify(band.width > 0 && band.height > 0);
            mouseRelease(root, second.x, second.y);
            compare(Fixture.folderView.grid.model.currentIndex, 1);
            verify(Fixture.folderView.grid.rubberBand.width === 0);
        }
    }
}
