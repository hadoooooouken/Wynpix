import QtQuick
import QtQuick.Controls.impl
import QtQuick.Templates as T
import wynpix.bridges

// Push button (QPushButton in the widget stylesheet). A flat button shows its
// surface only while hovered, pressed or checked. On a settings card in the
// dark scheme the button surface is the card's border tint instead: the
// regular one is nearly the card's own colour and would melt into it.
T.Button {
    id: control

    // Whether an ancestor is a card (an item with cardSurface set).
    readonly property bool onCard: {
        for (let item = control.parent; item; item = item.parent) {
            if (item.cardSurface === true)
                return true;
        }
        return false;
    }
    readonly property bool raisedSurface: Theme.dark && control.onCard
    readonly property color surfaceColor: control.raisedSurface ? Theme.surfaces.tintedLc
                                                                : Theme.colors.button
    readonly property color surfaceHoverColor: control.raisedSurface
                                               ? Qt.lighter(control.surfaceColor, 1.12)
                                               : Theme.colors.buttonHover
    readonly property color surfacePressedColor: control.raisedSurface
                                                 ? Qt.darker(control.surfaceColor, 1.12)
                                                 : Theme.colors.buttonPressed

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
    icon.color: Theme.colors.icons

    contentItem: IconLabel {
        spacing: control.spacing
        mirrored: control.mirrored
        display: control.display
        opacity: control.enabled ? 1.0 : StyleConstants.disabledOpacity

        icon: control.icon
        text: control.text
        font: control.font
        color: control.down || control.checked ? Theme.colors.text
             : control.hovered ? Theme.colors.textHc2
             : Theme.colors.textHc
    }

    background: Rectangle {
        implicitHeight: Theme.metrics.buttonHeight
        radius: StyleConstants.controlRadius
        visible: !control.flat || control.down || control.checked || control.hovered
        color: control.down || control.checked ? control.surfacePressedColor
             : control.hovered || control.highlighted ? control.surfaceHoverColor
             : control.surfaceColor
        border.width: control.visualFocus ? StyleConstants.separatorThickness : 0
        border.color: Theme.colors.accent
    }
}
