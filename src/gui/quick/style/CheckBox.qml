import QtQuick
import QtQuick.Templates as T
import wynpix.bridges

// Check box with a Windows 11 indicator: an outlined box when unchecked, a
// box filled with the accent and a check mark or dash on it otherwise.
T.CheckBox {
    id: control

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             implicitContentHeight + topPadding + bottomPadding,
                             implicitIndicatorHeight + topPadding + bottomPadding)

    padding: StyleConstants.indicatorPadding
    spacing: StyleConstants.indicatorSpacing

    indicator: Rectangle {
        readonly property bool filled: control.checkState !== Qt.Unchecked

        x: control.text ? (control.mirrored ? control.width - width - control.rightPadding : control.leftPadding)
                        : control.leftPadding + (control.availableWidth - width) / 2
        y: control.topPadding + (control.availableHeight - height) / 2
        implicitWidth: StyleConstants.indicatorSize
        implicitHeight: StyleConstants.indicatorSize
        radius: StyleConstants.indicatorRadius
        color: filled ? Theme.tokens.accent : "transparent"
        border.width: filled ? 0 : StyleConstants.indicatorBorderWidth
        border.color: Theme.tokens.iconDefault
        opacity: control.enabled ? 1.0 : StyleConstants.disabledOpacity

        IconGlyph {
            anchors.centerIn: parent
            visible: control.checkState === Qt.Checked
            icon: FluentIcons.Checkmark16
            size: StyleConstants.indicatorSize
            color: Theme.tokens.accentText
        }

        Rectangle {
            anchors.centerIn: parent
            visible: control.checkState === Qt.PartiallyChecked
            width: StyleConstants.indicatorDashLength
            height: StyleConstants.indicatorDashThickness
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
