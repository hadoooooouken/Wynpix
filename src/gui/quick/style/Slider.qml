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
    // Theme.colors.accent (e.g. for exposure, contrast and brightness).
    property color trackBaseColor: "transparent"

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
        color: !control.enabled ? Theme.colors.text
             : control.hovered || control.pressed ? Color.lighter(Theme.colors.accent, StyleConstants.accentLightFactor)
             : Theme.colors.accent
        border.width: StyleConstants.sliderHandleBorderWidth
        border.color: Theme.colors.widget
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
             : Theme.surfaces.tintedHc2
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
            color: Theme.colors.accent
        }
    }
}
