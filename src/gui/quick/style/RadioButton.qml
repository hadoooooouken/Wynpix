import QtQuick
import QtQuick.Templates as T
import wynpix.bridges

// Radio button with a Windows 11 indicator: an outlined circle when
// unchecked, a circle filled with the accent and a dot on it when checked.
T.RadioButton {
    id: control

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             implicitContentHeight + topPadding + bottomPadding,
                             implicitIndicatorHeight + topPadding + bottomPadding)

    padding: StyleConstants.indicatorPadding
    spacing: StyleConstants.indicatorSpacing

    indicator: Rectangle {
        x: control.text ? (control.mirrored ? control.width - width - control.rightPadding : control.leftPadding)
                        : control.leftPadding + (control.availableWidth - width) / 2
        y: control.topPadding + (control.availableHeight - height) / 2
        implicitWidth: StyleConstants.indicatorSize
        implicitHeight: StyleConstants.indicatorSize
        radius: width / 2
        color: control.checked ? Theme.tokens.accent : "transparent"
        border.width: control.checked ? 0 : StyleConstants.indicatorBorderWidth
        border.color: Theme.tokens.iconDefault
        opacity: control.enabled ? 1.0 : StyleConstants.disabledOpacity

        Rectangle {
            anchors.centerIn: parent
            visible: control.checked
            width: StyleConstants.radioDotSize
            height: width
            radius: width / 2
            color: Theme.tokens.accentText
        }
    }

    // The shared focus ring (FieldBackground), shown for keyboard focus.
    background: Rectangle {
        radius: StyleConstants.controlRadius
        color: "transparent"
        border.width: control.visualFocus ? StyleConstants.fieldBorderWidth : 0
        border.color: Theme.tokens.accent
    }

    contentItem: Text {
        leftPadding: control.indicator && !control.mirrored ? control.indicator.width + control.spacing : 0
        rightPadding: control.indicator && control.mirrored ? control.indicator.width + control.spacing : 0

        text: control.text
        font: control.font
        color: Theme.tokens.textPrimary
        opacity: control.enabled ? 1.0 : StyleConstants.disabledOpacity
        elide: Text.ElideRight
        verticalAlignment: Text.AlignVCenter
    }
}
