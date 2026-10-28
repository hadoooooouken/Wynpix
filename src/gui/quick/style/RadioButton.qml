import QtQuick
import QtQuick.Templates as T
import wynpix.bridges

// Radio button with the Fluent glyph indicators ProxyStyle draws for
// QRadioButton.
T.RadioButton {
    id: control

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             implicitContentHeight + topPadding + bottomPadding,
                             implicitIndicatorHeight + topPadding + bottomPadding)

    padding: StyleConstants.indicatorPadding
    spacing: StyleConstants.indicatorSpacing

    indicator: IconGlyph {
        x: control.text ? (control.mirrored ? control.width - width - control.rightPadding : control.leftPadding)
                        : control.leftPadding + (control.availableWidth - width) / 2
        y: control.topPadding + (control.availableHeight - height) / 2
        icon: control.checked ? FluentIcons.Record16 : FluentIcons.RadioButton16
        size: StyleConstants.indicatorSize
        color: control.checked ? Theme.tokens.accent : Theme.tokens.iconDefault
        opacity: control.enabled ? 1.0 : StyleConstants.disabledOpacity
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
