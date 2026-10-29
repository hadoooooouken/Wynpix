import QtQuick
import QtQuick.Templates as T
import wynpix.bridges

// Slider (global QSlider rules of the widget stylesheet): a thin groove in
// the dialog tint, the filled part in the accent colour and a round accent
// handle ringed with the widget colour.
T.Slider {
    id: control

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            implicitHandleWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             implicitHandleHeight + topPadding + bottomPadding)

    padding: 0

    // Optional custom track gradient. When set, renders the full-width
    // gradient without the filled highlight segment.
    property var trackGradient: null
    // Base color beneath trackGradient, used to blend the gradient with
    // Theme.tokens.accent (e.g. for exposure, contrast and brightness).
    property color trackBaseColor: "transparent"
    // Ring around the handle: the colour of the surface under the slider,
    // so the ring reads as a gap between the handle and the groove.
    property color handleRingColor: Theme.tokens.surfacePopup

    handle: Rectangle {
        x: control.leftPadding + (control.horizontal
                                  ? control.visualPosition * (control.availableWidth - width)
                                  : (control.availableWidth - width) / 2)
        y: control.topPadding + (control.horizontal
                                 ? (control.availableHeight - height) / 2
                                 : control.visualPosition * (control.availableHeight - height))
        implicitWidth: StyleConstants.sliderHandleSize
        implicitHeight: StyleConstants.sliderHandleSize
        radius: width / 2
        // Disabled: the text colour at the disabled opacity over the widget,
        // not the brightest colour of the control.
        color: !control.enabled ? Qt.tint(Theme.tokens.surfacePopup,
                                          Color.transparent(Theme.tokens.textSecondary, StyleConstants.disabledOpacity))
             : control.hovered || control.pressed ? Color.lighter(Theme.tokens.accent, StyleConstants.accentLightFactor)
             : Theme.tokens.accent
        border.width: StyleConstants.sliderHandleBorderWidth
        border.color: control.handleRingColor

        // The shared focus ring, outside the ring of the widget colour.
        Rectangle {
            anchors.fill: parent
            anchors.margins: -(StyleConstants.sliderHandleBorderWidth + StyleConstants.fieldBorderWidth)
            radius: width / 2
            color: "transparent"
            border.width: StyleConstants.fieldBorderWidth
            border.color: Theme.tokens.accent
            visible: control.visualFocus
        }
    }

    background: Rectangle {
        id: groove

        x: control.leftPadding + (control.horizontal ? 0 : (control.availableWidth - width) / 2)
        y: control.topPadding + (control.horizontal ? (control.availableHeight - height) / 2 : 0)
        implicitWidth: control.horizontal ? StyleConstants.sliderLength : StyleConstants.sliderThickness
        implicitHeight: control.horizontal ? StyleConstants.sliderThickness : StyleConstants.sliderLength
        width: control.horizontal ? control.availableWidth : StyleConstants.sliderGrooveThickness
        height: control.horizontal ? StyleConstants.sliderGrooveThickness : control.availableHeight
        radius: StyleConstants.sliderGrooveThickness / 2
        color: control.trackGradient !== null
             ? control.trackBaseColor
             : Theme.tokens.strokeStrong
        scale: control.horizontal && control.mirrored ? -1 : 1

        Rectangle {
            anchors.fill: parent
            radius: parent.radius
            visible: control.trackGradient !== null
            opacity: control.enabled ? 1.0 : StyleConstants.disabledOpacity
            gradient: control.trackGradient
        }

        // Filled part, from the start of the range to the handle.
        Rectangle {
            visible: control.trackGradient === null && control.enabled
            y: control.horizontal ? 0 : control.visualPosition * parent.height
            width: control.horizontal ? control.position * parent.width : parent.width
            height: control.horizontal ? parent.height : control.position * parent.height
            radius: groove.radius
            color: Theme.tokens.accent
        }
    }
}
