import QtQuick
import QtQuick.Controls.impl
import QtQuick.Templates as T
import wynpix.bridges

// Push button (QPushButton in the widget stylesheet). A flat button shows its
// surface only while hovered, pressed or checked. The fills are translucent,
// so the button stays visible on a panel, a card or a dialog alike. A
// highlighted (default) button is the primary one: solid accent with the
// accent text colour.
T.Button {
    id: control

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             implicitContentHeight + topPadding + bottomPadding)

    topPadding: StyleConstants.buttonVerticalPadding
    bottomPadding: StyleConstants.buttonVerticalPadding
    leftPadding: StyleConstants.buttonHorizontalPadding
    rightPadding: StyleConstants.buttonHorizontalPadding
    spacing: StyleConstants.indicatorSpacing

    icon.width: Theme.compactIconSize
    icon.height: Theme.compactIconSize
    icon.color: control.highlighted ? Theme.colors.accentText : Theme.colors.icons

    contentItem: IconLabel {
        spacing: control.spacing
        mirrored: control.mirrored
        display: control.display
        opacity: control.enabled ? 1.0 : StyleConstants.disabledOpacity

        icon: control.icon
        text: control.text
        font: control.font
        color: control.highlighted ? Theme.colors.accentText : Theme.colors.textHc
    }

    background: Rectangle {
        implicitHeight: Theme.metrics.buttonHeight
        radius: StyleConstants.controlRadius
        visible: !control.flat || control.down || control.checked || control.hovered
        color: control.highlighted ? (control.down ? Theme.colors.accentPressed
                                      : control.hovered ? Theme.colors.accentHover
                                      : Theme.colors.accent)
             : control.down ? Theme.colors.fillPressed
             : control.checked ? (control.hovered ? Theme.colors.accentTintHover : Theme.colors.accentTint)
             : control.hovered ? Theme.colors.fillHover
             : Theme.colors.fillRest
        border.width: control.visualFocus ? StyleConstants.fieldBorderWidth : 0
        // The ring of the shared focus; on the solid accent it takes the text colour.
        border.color: control.highlighted ? Theme.colors.accentText : Theme.colors.accent
    }
}
