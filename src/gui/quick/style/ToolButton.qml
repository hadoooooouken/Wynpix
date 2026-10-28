import QtQuick
import QtQuick.Controls.impl
import QtQuick.Templates as T
import wynpix.bridges

// Panel button (PanelButton / CheckableButton in the widget stylesheet):
// transparent on the panel, tinted while hovered, pressed or checked. Icon
// glyphs are set as the contentItem (IconGlyph); icon.source images and text
// are drawn by the default content.
T.ToolButton {
    id: control

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             implicitContentHeight + topPadding + bottomPadding)

    topInset: StyleConstants.toolButtonVerticalInset
    bottomInset: StyleConstants.toolButtonVerticalInset
    leftInset: StyleConstants.toolButtonHorizontalInset
    rightInset: StyleConstants.toolButtonHorizontalInset
    topPadding: StyleConstants.toolButtonVerticalInset + StyleConstants.toolButtonPadding
    bottomPadding: StyleConstants.toolButtonVerticalInset + StyleConstants.toolButtonPadding
    leftPadding: StyleConstants.toolButtonHorizontalInset + StyleConstants.toolButtonPadding
    rightPadding: StyleConstants.toolButtonHorizontalInset + StyleConstants.toolButtonPadding
    spacing: StyleConstants.indicatorSpacing

    icon.width: Theme.standardIconSize
    icon.height: Theme.standardIconSize
    icon.color: Theme.tokens.iconDefault

    contentItem: IconLabel {
        spacing: control.spacing
        mirrored: control.mirrored
        display: control.display
        opacity: control.enabled ? 1.0 : StyleConstants.disabledOpacity

        icon: control.icon
        text: control.text
        font: control.font
        color: Theme.tokens.textPrimary
    }

    background: Rectangle {
        radius: StyleConstants.controlRadius
        color: control.down ? Theme.tokens.fillPressed
             : control.checked ? (control.hovered ? Theme.tokens.accentTintHover : Theme.tokens.accentTint)
             : control.hovered ? Theme.tokens.fillHover
             : "transparent"
        border.width: control.visualFocus ? StyleConstants.fieldBorderWidth : 0
        border.color: Theme.tokens.accent
    }
}
