import QtQuick
import QtTest
import wynpix.style

// DialogWheelScroller over a list in a window of its own: in a dialog window
// (one with a transient parent) the wheel scrolls the list on the window's
// frames without the list's own wheel animation, stops at the ends and hands
// the turn on there; in a window without a transient parent the list keeps
// its own wheel scroll.
TestCase {
    id: testCase

    name: "WheelScroller"
    when: windowShown

    readonly property int rowHeight: 20
    readonly property int rowCount: 50
    readonly property int windowSize: 200
    readonly property int wheelNotch: 120
    // Pixels one notch scrolls, as Flickable.
    readonly property real notchHeight: Qt.styleHints.wheelScrollLines * 24
    // Longer than a scroll takes.
    readonly property int scrollTimeoutMs: 1000
    readonly property real positionTolerance: 0.5

    Component {
        id: windowComponent

        Window {
            id: window

            property alias list: list
            property int wheelsBehind: 0

            width: testCase.windowSize
            height: testCase.windowSize
            visible: true

            // Takes the turns the list hands on.
            MouseArea {
                anchors.fill: parent
                acceptedButtons: Qt.NoButton
                onWheel: wheel => {
                    ++window.wheelsBehind;
                    wheel.accepted = true;
                }
            }

            ListView {
                id: list

                anchors.fill: parent
                boundsBehavior: Flickable.StopAtBounds
                model: testCase.rowCount
                delegate: Rectangle {
                    required property int index

                    width: ListView.view.width
                    height: testCase.rowHeight
                    color: index % 2 ? "white" : "gray"
                }

                DialogWheelScroller {
                    flickable: list
                }
            }
        }
    }

    function createWindow(dialog) {
        const window = createTemporaryObject(windowComponent, testCase,
                                             {transientParent: dialog ? testCase.Window.window : null});
        verify(window !== null);
        compare(window.transientParent !== null, dialog);
        waitForRendering(window.list);
        return window;
    }

    function turn(window, delta) {
        mouseWheel(window.list, window.list.width / 2, window.list.height / 2, 0, delta);
    }

    function reaches(list, contentY) {
        tryVerify(() => Math.abs(list.contentY - contentY) < positionTolerance, scrollTimeoutMs,
                  "contentY " + list.contentY + " instead of " + contentY);
    }

    function lastContentY(list) {
        return list.contentHeight - list.height;
    }

    function test_aNotchScrollsWithoutTheListAnimation() {
        const window = createWindow(true);
        turn(window, -wheelNotch);
        verify(!window.list.flicking);
        verify(!window.list.moving);
        reaches(window.list, notchHeight);
        verify(!window.list.moving);
        compare(window.wheelsBehind, 0);
    }

    function test_notchesAddUp() {
        const window = createWindow(true);
        turn(window, -wheelNotch);
        turn(window, -wheelNotch);
        reaches(window.list, 2 * notchHeight);
    }

    function test_theScrollStopsAtTheEnd() {
        const window = createWindow(true);
        const notches = Math.ceil(lastContentY(window.list) / notchHeight) + 1;
        for (let i = 0; i < notches; ++i)
            turn(window, -wheelNotch);
        reaches(window.list, lastContentY(window.list));
        wait(scrollTimeoutMs / 2);
        compare(window.list.contentY, lastContentY(window.list));
    }

    function test_aTurnPastTheEndGoesOn() {
        const window = createWindow(true);
        turn(window, wheelNotch);
        compare(window.wheelsBehind, 1);
        compare(window.list.contentY, 0);

        window.list.positionViewAtEnd();
        turn(window, -wheelNotch);
        compare(window.wheelsBehind, 2);
    }

    function test_anotherMoveStopsTheScroll() {
        const window = createWindow(true);
        const moved = notchHeight / 2;
        turn(window, -wheelNotch);
        window.list.contentY = moved;
        wait(scrollTimeoutMs / 2);
        compare(window.list.contentY, moved);
    }

    // The list's own wheel scroll moves it at once.
    function test_withoutATransientParentTheListScrollsItself() {
        const window = createWindow(false);
        turn(window, -wheelNotch);
        verify(window.list.moving);
        reaches(window.list, notchHeight);
    }
}
