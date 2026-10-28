import QtQuick
import QtQuick.Templates as T
import wynpix.bridges

// Thin line between groups of menu items, in the menu border colour.
T.MenuSeparator {
    id: control

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             implicitContentHeight + topPadding + bottomPadding)

    horizontalPadding: StyleConstants.menuItemHorizontalPadding
    verticalPadding: StyleConstants.menuSeparatorVerticalPadding

    contentItem: Rectangle {
        implicitHeight: StyleConstants.separatorThickness
        color: Theme.tokens.strokeDefault
    }
}
