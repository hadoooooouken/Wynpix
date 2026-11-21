pragma ComponentBehavior: Bound

import QtQuick
import wynpix.bridges
import wynpix.style

// The thumbnail strip of the panel (ThumbnailStrip in the widget UI): a
// ListView of ThumbnailWidget cells that reuses its delegates, with the
// scroll bar and its selection marker. In the masonry mode the cells are
// tiles of the proportions of their images, which StripMasonryController
// places and its tiles model provides, in a Flickable of their own. The
// model decides what to load, select and where to scroll; the strip reports
// its viewport and the pointer, and runs the scroll animations the model asks
// for. It never takes the keyboard focus.
Item {
    id: strip

    required property ThumbnailPanelController controller

    readonly property ThumbnailListModel thumbnails: controller.model
    readonly property StripMasonryController masonry: controller.masonry
    readonly property thumbnailStripLayout stripLayout: controller.layout
    readonly property bool horizontal: stripLayout.horizontal
    readonly property bool masonryActive: stripLayout.masonry
    // The view that scrolls: the list, or the tiles of the masonry mode.
    readonly property Flickable scroller: masonryActive ? tiles : list
    readonly property real viewOffset: horizontal ? scroller.contentX : scroller.contentY
    readonly property real viewExtent: horizontal ? scroller.width : scroller.height
    // Fade of the masonry tiles in once the first sizes are known.
    readonly property int readyFadeDuration: 60
    // Item under the pointer, -1 for none.
    property int hoveredIndex: -1

    // The scroll bar of one orientation; only the bar of the strip's current
    // orientation and mode is in use, the others stay off.
    component StripScrollBar: ScrollBar {
        id: bar

        required property ThumbnailListModel thumbnails
        required property bool inUse
        // Position of the selection in the bar, 0 - 1.
        required property real markerCenter

        // Selection marker in the scroll bar (ThumbnailStrip::
        // updateScrollbarIndicator()).
        readonly property int markerSize: 2
        readonly property int markerInset: 2

        policy: inUse ? ScrollBar.AsNeeded : ScrollBar.AlwaysOff

        // The strip's scroll bar track carries the panel colour once more,
        // as the thumbnail cells do (ThumbnailStrip QScrollBar in the widget
        // stylesheet), so a translucent panel has an even band under them.
        background: Rectangle {
            visible: bar.contentItem.visible
            color: Theme.tokens.surfaceChrome
        }

        Rectangle {
            visible: bar.size < 1.0 && bar.thumbnails.currentIndex >= 0
            color: Theme.tokens.scrollbarMarker
            x: bar.horizontal ? bar.width * bar.markerCenter - bar.markerSize : bar.markerInset
            y: bar.horizontal ? bar.markerInset : bar.height * bar.markerCenter - bar.markerSize
            width: bar.horizontal ? bar.markerSize : bar.width - 2 * bar.markerInset
            height: bar.horizontal ? bar.height - 2 * bar.markerInset : bar.markerSize
        }
    }

    function reportViewport() {
        thumbnails.setViewport(viewOffset, viewExtent);
        masonry.tiles.setViewport(viewOffset, viewExtent);
    }

    // The item at a point of the view, -1 for none.
    function itemAt(x: real, y: real): int {
        if (masonryActive)
            return masonry.itemAt(x + tiles.contentX, y + tiles.contentY);
        return list.indexAt(x + list.contentX, y + list.contentY);
    }

    function updateHoveredIndex() {
        hoveredIndex = pointerArea.containsMouse
            ? itemAt(pointerArea.mouseX, pointerArea.mouseY)
            : -1;
    }

    function scrollTo(offset: real, durationMs: int) {
        scrollAnimation.stop();
        if (durationMs <= 0) {
            if (horizontal)
                scroller.contentX = offset;
            else
                scroller.contentY = offset;
            return;
        }
        scrollAnimation.property = horizontal ? "contentX" : "contentY";
        scrollAnimation.to = offset;
        scrollAnimation.duration = durationMs;
        scrollAnimation.start();
    }

    // The tiles view keeps its position when it is not the one that scrolls,
    // and a Flickable keeps its position across the strip when the strip turns
    // (a horizontal strip scrolled far along would leave the tiles of a
    // vertical one out of view). Whenever the strip turns or the mode
    // changes, the tiles start at the beginning and the model's focus on the
    // selection scrolls them where they belong.
    function resetTiles() {
        scrollAnimation.stop();
        tiles.contentX = 0;
        tiles.contentY = 0;
        // The cells under the pointer are other ones now, and the offset may
        // stay the same, which would not report it.
        updateHoveredIndex();
    }

    onHorizontalChanged: resetTiles()
    onMasonryActiveChanged: resetTiles()
    onViewOffsetChanged: {
        reportViewport();
        updateHoveredIndex();
    }
    onViewExtentChanged: reportViewport()
    onVisibleChanged: thumbnails.setActive(visible)
    Component.onCompleted: {
        reportViewport();
        thumbnails.setActive(visible);
    }
    Component.onDestruction: thumbnails.setActive(false)

    // The tiles moved under a pointer at rest (a size arrived, the mode
    // changed): the hovered item is the one under it now.
    Connections {
        target: strip.masonry

        function onContentExtentChanged() {
            strip.updateHoveredIndex();
        }
    }

    Connections {
        target: strip.thumbnails

        function onScrollRequested(offset: real, durationMs: int) {
            strip.scrollTo(offset, durationMs);
        }
    }

    NumberAnimation {
        id: scrollAnimation

        target: strip.scroller
        easing.type: Easing.OutCubic
        onFinished: strip.thumbnails.scrollAnimationFinished()
    }

    ListView {
        id: list

        anchors.fill: parent
        visible: !strip.masonryActive
        orientation: strip.horizontal ? ListView.Horizontal : ListView.Vertical
        // The masonry mode draws its tiles in the Flickable below.
        model: strip.masonryActive ? null : strip.thumbnails
        interactive: false
        boundsBehavior: Flickable.StopAtBounds
        reuseItems: true
        clip: true

        // Each bar keeps its slot for the strip's lifetime: emptying a slot
        // makes Qt hide and unparent the bar it held (hideOldItem()), so a
        // single bar moved between the slots on an orientation change was
        // lost until restart.
        ScrollBar.horizontal: horizontalBar
        ScrollBar.vertical: verticalBar

        delegate: ThumbnailWidget {
            cellLayout: strip.stripLayout
            hoveredIndex: strip.hoveredIndex
            surfaceColor: Theme.tokens.surfaceChrome
            hoverColor: Theme.tokens.contrastOnChrome
            labelOnSurface: strip.controller.labelTextColor(Theme.tokens.surfaceChrome)
            labelOnHover: strip.controller.labelTextColor(Theme.tokens.contrastOnChrome)
            selectedLabelColor: Theme.tokens.textOnChrome
        }
    }

    // The masonry mode: the tiles near the viewport with the position and the
    // cell of each, the proportions of their images. Scrolling is the
    // model's: the view is not interactive.
    Flickable {
        id: tiles

        objectName: "masonryTiles"
        anchors.fill: parent
        visible: strip.masonryActive
        contentWidth: strip.horizontal ? strip.masonry.contentExtent : width
        contentHeight: strip.horizontal ? height : strip.masonry.contentExtent
        interactive: false
        boundsBehavior: Flickable.StopAtBounds
        clip: true

        ScrollBar.horizontal: masonryHorizontalBar
        ScrollBar.vertical: masonryVerticalBar

        // Hidden at once while the first sizes are awaited (the tiles are
        // about to change), faded in when they are known.
        states: State {
            name: "hidden"
            when: !strip.masonry.ready
            PropertyChanges {
                target: tiles
                opacity: 0.0
            }
        }

        transitions: Transition {
            from: "hidden"
            to: ""
            NumberAnimation {
                property: "opacity"
                duration: strip.readyFadeDuration
                easing.type: Easing.OutQuad
            }
        }

        Repeater {
            model: strip.masonryActive ? strip.masonry.tiles : null

            delegate: ThumbnailWidget {
                // The row of the tiles model changes when rows leave in
                // front, the item does not; the cell compares its row with
                // the hovered index.
                required property real tileX
                required property real tileY
                required property int itemIndex

                x: tileX
                y: tileY
                hoveredIndex: strip.hoveredIndex === itemIndex ? index : -1
                surfaceColor: Theme.tokens.surfaceChrome
                hoverColor: Theme.tokens.contrastOnChrome
                labelOnSurface: strip.controller.labelTextColor(Theme.tokens.surfaceChrome)
                labelOnHover: strip.controller.labelTextColor(Theme.tokens.contrastOnChrome)
                selectedLabelColor: Theme.tokens.textOnChrome
            }
        }
    }

    MouseArea {
        id: pointerArea

        function itemAt(mouse: MouseEvent): int {
            return strip.itemAt(mouse.x, mouse.y);
        }

        anchors.fill: parent
        hoverEnabled: true
        acceptedButtons: Qt.LeftButton | Qt.RightButton | Qt.BackButton | Qt.ForwardButton

        onContainsMouseChanged: strip.updateHoveredIndex()
        onPressed: mouse => strip.thumbnails.press(itemAt(mouse), mouse.button, mouse.modifiers,
                                                   Qt.point(mouse.x, mouse.y))
        onPositionChanged: mouse => {
            strip.updateHoveredIndex();
            strip.thumbnails.move(Qt.point(mouse.x, mouse.y), mouse.buttons);
        }
        onReleased: mouse => strip.thumbnails.release(itemAt(mouse), mouse.button,
                                                      Qt.point(mouse.x, mouse.y))
        onDoubleClicked: mouse => strip.thumbnails.doubleClick(itemAt(mouse), mouse.button)
        onWheel: wheel => {
            strip.thumbnails.wheelScrolled(wheel.angleDelta, wheel.pixelDelta);
            wheel.accepted = true;
        }
    }

    // Attaching to a view restacks a bar directly above it, which would put
    // the pointer area over the bar and take its presses. The attached slot
    // sets each bar's orientation. The list and the masonry tiles have a pair
    // of bars each, so that no bar changes its view.
    StripScrollBar {
        id: horizontalBar

        objectName: "horizontalScrollBar"
        thumbnails: strip.thumbnails
        inUse: strip.horizontal && !strip.masonryActive
        markerCenter: strip.thumbnails.count > 0
            ? (strip.thumbnails.currentIndex + 0.5) / strip.thumbnails.count : 0
        z: pointerArea.z + 1
        anchors.left: strip.left
        anchors.right: strip.right
        anchors.bottom: strip.bottom
    }

    StripScrollBar {
        id: verticalBar

        objectName: "verticalScrollBar"
        thumbnails: strip.thumbnails
        inUse: !strip.horizontal && !strip.masonryActive
        markerCenter: strip.thumbnails.count > 0
            ? (strip.thumbnails.currentIndex + 0.5) / strip.thumbnails.count : 0
        z: pointerArea.z + 1
        anchors.top: strip.top
        anchors.right: strip.right
        anchors.bottom: strip.bottom
    }

    StripScrollBar {
        id: masonryHorizontalBar

        objectName: "masonryHorizontalScrollBar"
        thumbnails: strip.thumbnails
        inUse: strip.horizontal && strip.masonryActive
        markerCenter: strip.masonry.markerPosition
        z: pointerArea.z + 1
        anchors.left: strip.left
        anchors.right: strip.right
        anchors.bottom: strip.bottom
    }

    StripScrollBar {
        id: masonryVerticalBar

        objectName: "masonryVerticalScrollBar"
        thumbnails: strip.thumbnails
        inUse: !strip.horizontal && strip.masonryActive
        markerCenter: strip.masonry.markerPosition
        z: pointerArea.z + 1
        anchors.top: strip.top
        anchors.right: strip.right
        anchors.bottom: strip.bottom
    }
}
