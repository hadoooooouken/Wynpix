pragma ComponentBehavior: Bound

import QtQuick
import wynpix.bridges
import wynpix.style

// The thumbnail strip of the panel (ThumbnailStrip in the widget UI): a
// ListView of ThumbnailWidget cells that reuses its delegates, with the
// scroll bar and its selection marker. The model decides what to load,
// select and where to scroll; the strip reports its viewport and the
// pointer, and runs the scroll animations the model asks for. It never takes
// the keyboard focus.
Item {
    id: strip

    required property ThumbnailPanelController controller

    readonly property ThumbnailListModel thumbnails: controller.model
    readonly property thumbnailStripLayout stripLayout: controller.layout
    readonly property bool horizontal: stripLayout.horizontal
    // Item under the pointer, -1 for none.
    property int hoveredIndex: -1

    // The scroll bar of one orientation; only the bar of the strip's current
    // orientation is in use, the other one stays off.
    component StripScrollBar: ScrollBar {
        id: bar

        required property ThumbnailListModel thumbnails
        required property bool inUse

        // Selection marker in the scroll bar (ThumbnailStrip::
        // updateScrollbarIndicator()).
        readonly property int markerSize: 2
        readonly property int markerInset: 2
        readonly property real markerOpacity: 0.3
        readonly property real markerCenter: thumbnails.count > 0
            ? (thumbnails.currentIndex + 0.5) / thumbnails.count : 0

        policy: inUse ? ScrollBar.AsNeeded : ScrollBar.AlwaysOff

        // The strip's scroll bar track carries the panel colour once more,
        // as the thumbnail cells do (ThumbnailStrip QScrollBar in the widget
        // stylesheet), so a translucent panel has an even band under them.
        background: Rectangle {
            visible: bar.contentItem.visible
            color: Theme.colors.thumbPanel
        }

        Rectangle {
            visible: bar.size < 1.0 && bar.thumbnails.currentIndex >= 0
            color: "gray"
            opacity: bar.markerOpacity
            x: bar.horizontal ? bar.width * bar.markerCenter - bar.markerSize : bar.markerInset
            y: bar.horizontal ? bar.markerInset : bar.height * bar.markerCenter - bar.markerSize
            width: bar.horizontal ? bar.markerSize : bar.width - 2 * bar.markerInset
            height: bar.horizontal ? bar.height - 2 * bar.markerInset : bar.markerSize
        }
    }

    function reportViewport() {
        thumbnails.setViewport(list.viewOffset, list.viewExtent);
    }

    function updateHoveredIndex() {
        hoveredIndex = pointerArea.containsMouse
            ? list.indexAt(pointerArea.mouseX + list.contentX, pointerArea.mouseY + list.contentY)
            : -1;
    }

    function scrollTo(offset: real, durationMs: int) {
        scrollAnimation.stop();
        if (durationMs <= 0) {
            if (horizontal)
                list.contentX = offset;
            else
                list.contentY = offset;
            return;
        }
        scrollAnimation.property = horizontal ? "contentX" : "contentY";
        scrollAnimation.to = offset;
        scrollAnimation.duration = durationMs;
        scrollAnimation.start();
    }

    onVisibleChanged: thumbnails.setActive(visible)
    Component.onCompleted: {
        reportViewport();
        thumbnails.setActive(visible);
    }
    Component.onDestruction: thumbnails.setActive(false)

    Connections {
        target: strip.thumbnails

        function onScrollRequested(offset: real, durationMs: int) {
            strip.scrollTo(offset, durationMs);
        }
    }

    NumberAnimation {
        id: scrollAnimation

        target: list
        easing.type: Easing.OutSine
        onFinished: strip.thumbnails.scrollAnimationFinished()
    }

    ListView {
        id: list

        readonly property real viewOffset: strip.horizontal ? contentX : contentY
        readonly property real viewExtent: strip.horizontal ? width : height

        anchors.fill: parent
        orientation: strip.horizontal ? ListView.Horizontal : ListView.Vertical
        model: strip.thumbnails
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

        onViewOffsetChanged: {
            strip.reportViewport();
            strip.updateHoveredIndex();
        }
        onViewExtentChanged: strip.reportViewport()

        delegate: ThumbnailWidget {
            cellLayout: strip.stripLayout
            hoveredIndex: strip.hoveredIndex
            surfaceColor: Theme.colors.thumbPanel
            hoverColor: Theme.colors.thumbPanelHc
            labelOnSurface: strip.controller.labelTextColor(Theme.colors.thumbPanel)
            labelOnHover: strip.controller.labelTextColor(Theme.colors.thumbPanelHc)
            selectedLabelColor: Theme.colors.thumbPanelText
        }
    }

    MouseArea {
        id: pointerArea

        function itemAt(mouse: MouseEvent): int {
            return list.indexAt(mouse.x + list.contentX, mouse.y + list.contentY);
        }

        anchors.fill: list
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

    // Attaching to the list restacks a bar directly above it, which would
    // put the pointer area over the bar and take its presses. The attached
    // slot sets each bar's orientation.
    StripScrollBar {
        id: horizontalBar

        objectName: "horizontalScrollBar"
        thumbnails: strip.thumbnails
        inUse: strip.horizontal
        z: pointerArea.z + 1
        anchors.left: strip.left
        anchors.right: strip.right
        anchors.bottom: strip.bottom
    }

    StripScrollBar {
        id: verticalBar

        objectName: "verticalScrollBar"
        thumbnails: strip.thumbnails
        inUse: !strip.horizontal
        z: pointerArea.z + 1
        anchors.top: strip.top
        anchors.right: strip.right
        anchors.bottom: strip.bottom
    }
}
