pragma ComponentBehavior: Bound

import QtQuick
import wynpix.bridges
import wynpix.render

// The image viewer of the Qt Quick UI: the GPU image view, its pointer input
// and the viewer chrome (zoom indicator, click zones). Presentation only;
// ImageViewportController decides what input does and drives the view.
// Input the viewer does not consume goes on to the action shortcuts.
FocusScope {
    id: root

    required property ImageViewportController controller
    // What the image is drawn over; may be translucent.
    property color backgroundColor: Theme.colors.background
    property int sidePanelLeftInset: 0
    property int sidePanelRightInset: 0
    property bool floatingPanelVisible: false

    Binding {
        target: root.controller
        property: "sidePanelLeftInset"
        value: root.sidePanelLeftInset
    }

    Binding {
        target: root.controller
        property: "sidePanelRightInset"
        value: root.sidePanelRightInset
    }

    // Click zone pills (ClickZoneOverlay).
    readonly property int zonePillWidth: 100
    readonly property int zonePillEdgeMargin: 16
    readonly property int zonePillHeightDivisor: 3
    readonly property int zonePillMinHeight: 80
    readonly property int zonePillRadius: 8
    readonly property int zonePillSlide: 12
    readonly property int zoneArrowSize: 48
    readonly property int zoneFadeInDuration: 160
    readonly property int zoneFadeOutDuration: 120
    readonly property real zoneArrowLightThemeLightness: 0.5
    readonly property color zoneArrowOnLight: "#646464"
    readonly property color zoneArrowOnDark: "#a4a4a4"

    // Zoom indicator (ZoomIndicatorOverlay).
    readonly property int indicatorBottomMargin: 16
    readonly property int indicatorHorizontalPadding: 14
    readonly property int indicatorVerticalPadding: 12
    readonly property int indicatorRadius: 8
    readonly property int indicatorFadeDuration: 300

    focus: true
    Keys.onPressed: event => event.accepted = Actions.handleKeyEvent(event)

    ImageRenderItem {
        id: imageView

        anchors.fill: parent
        backgroundColor: root.backgroundColor
    }

    Binding {
        target: root.controller
        property: "view"
        value: imageView
    }

    MouseArea {
        id: pointerArea

        anchors.fill: parent
        acceptedButtons: Qt.AllButtons
        hoverEnabled: true
        cursorShape: root.controller.cursorShape

        onEntered: root.controller.pointerEntered(Qt.point(pointerArea.mouseX, pointerArea.mouseY))
        onExited: root.controller.pointerExited()
        onPressed: mouse => {
            if (!root.controller.pointerPressed(Qt.point(mouse.x, mouse.y), mouse.button, mouse.modifiers))
                Actions.handleMousePress(mouse);
        }
        onPositionChanged: mouse => root.controller.pointerMoved(Qt.point(mouse.x, mouse.y), mouse.buttons)
        onReleased: mouse => {
            if (!root.controller.pointerReleased(Qt.point(mouse.x, mouse.y)))
                Actions.handleMouseRelease(mouse);
        }
        // The grab was lost with a button held: no release follows.
        onCanceled: root.controller.pointerCanceled()
        onDoubleClicked: mouse => {
            if (!root.controller.pointerDoubleClicked(Qt.point(mouse.x, mouse.y), mouse.button, mouse.modifiers))
                Actions.handleMouseDoubleClick(mouse);
        }
        onWheel: wheel => {
            if (!root.controller.wheelTurned(Qt.point(wheel.x, wheel.y), wheel.angleDelta, wheel.pixelDelta, wheel.buttons, wheel.modifiers))
                Actions.handleWheelEvent(wheel);
        }
    }

    PinchHandler {
        id: pinch

        target: null

        onActiveChanged: {
            if (pinch.active)
                root.controller.pinchStarted(pinch.centroid.position);
            else
                root.controller.pinchFinished();
        }
        onActiveScaleChanged: root.controller.pinchUpdated(pinch.activeScale)
    }

    component ClickZonePill: Rectangle {
        id: pill

        required property bool rightSide
        required property bool highlighted
        readonly property int inset: rightSide ? root.sidePanelRightInset : root.sidePanelLeftInset
        readonly property int restX: rightSide
            ? root.width - inset - root.zonePillEdgeMargin - root.zonePillWidth
            : inset + root.zonePillEdgeMargin
        readonly property int slideX: rightSide ? root.zonePillSlide : -root.zonePillSlide

        x: highlighted ? restX : restX + slideX
        y: (root.height - height) / 2
        width: root.zonePillWidth
        height: Math.max(root.height / root.zonePillHeightDivisor, root.zonePillMinHeight)
        radius: root.zonePillRadius
        color: Theme.colors.thumbPanel
        opacity: highlighted ? 1 : 0
        visible: opacity > 0

        Behavior on opacity {
            enabled: !root.controller.reducedMotion
            NumberAnimation {
                duration: pill.highlighted ? root.zoneFadeInDuration : root.zoneFadeOutDuration
                easing.type: pill.highlighted ? Easing.OutCubic : Easing.InCubic
            }
        }
        Behavior on x {
            enabled: !root.controller.reducedMotion
            NumberAnimation {
                duration: pill.highlighted ? root.zoneFadeInDuration : root.zoneFadeOutDuration
                easing.type: pill.highlighted ? Easing.OutCubic : Easing.InCubic
            }
        }

        Text {
            anchors.centerIn: parent
            font.family: Theme.iconFontFamily
            font.pixelSize: root.zoneArrowSize
            // The pill slides by fractional pixels; native glyphs snap to the
            // pixel grid and would step behind it.
            renderType: Text.QtRendering
            color: Theme.colors.thumbPanel.hslLightness > root.zoneArrowLightThemeLightness
                   ? root.zoneArrowOnLight : root.zoneArrowOnDark
            text: Theme.glyph(pill.rightSide ? FluentIcons.ChevronRight48 : FluentIcons.ChevronLeft48)
        }
    }

    ClickZonePill {
        rightSide: false
        highlighted: root.controller.clickZonesEnabled && root.controller.clickZonesDrawn
                     && root.controller.highlightedClickZone === ImageViewportController.Left
    }

    ClickZonePill {
        rightSide: true
        highlighted: root.controller.clickZonesEnabled && root.controller.clickZonesDrawn
                     && root.controller.highlightedClickZone === ImageViewportController.Right
    }

    Rectangle {
        anchors.left: parent.left
        anchors.bottom: parent.bottom
        anchors.bottomMargin: root.indicatorBottomMargin
        topRightRadius: root.indicatorRadius
        bottomRightRadius: root.indicatorRadius
        width: zoomText.implicitWidth + root.indicatorHorizontalPadding
        height: zoomText.implicitHeight + root.indicatorVerticalPadding
        color: Theme.colors.overlay
        opacity: root.controller.zoomIndicatorVisible && !root.floatingPanelVisible ? 1 : 0
        visible: opacity > 0

        Behavior on opacity {
            enabled: !root.controller.reducedMotion
            NumberAnimation { duration: root.indicatorFadeDuration }
        }

        Text {
            id: zoomText

            anchors.centerIn: parent
            font: Theme.fonts.title
            color: Theme.colors.overlayText
            text: qsTr("%1%").arg(root.controller.zoomPercent)
        }
    }
}
