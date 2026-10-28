import QtQuick
import QtQuick.Controls.impl
import QtQuick.Templates as T
import wynpix.bridges

// Context menu row of the widget UI: label on the left, the action's
// shortcut on the right in the secondary text colour, a rounded neutral
// highlight under the pointer. The icon is either icon.source or a glyph of
// the application icon font (hasGlyph, glyph).
T.MenuItem {
    id: control

    // Text shown on the right; the Action's shortcut by default.
    property string shortcutText: control.action ? ShortcutText.of(control.action.shortcut) : ""
    // Draws glyph (a FluentIcons value) as the icon.
    property bool hasGlyph: false
    property int glyph
    property int glyphSize: Theme.standardIconSize
    property color glyphColor: Theme.tokens.iconDefault
    property color labelColor: Theme.tokens.textPrimary

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             implicitContentHeight + topPadding + bottomPadding,
                             implicitIndicatorHeight + topPadding + bottomPadding)

    verticalPadding: 0
    horizontalPadding: StyleConstants.menuItemHorizontalPadding
    spacing: StyleConstants.menuItemSpacing

    icon.width: Theme.compactIconSize
    icon.height: Theme.compactIconSize
    icon.color: Theme.tokens.iconDefault

    contentItem: Item {
        id: content

        readonly property real indicatorPadding: control.checkable && control.indicator
                                                 ? control.indicator.width + control.spacing : 0
        readonly property real arrowPadding: control.subMenu && control.arrow
                                             ? control.arrow.width + control.spacing : 0
        readonly property real glyphPadding: control.hasGlyph ? glyphIcon.width + control.spacing : 0
        readonly property string shortcutText: control.shortcutText
        readonly property color secondaryColor: Color.transparent(Theme.tokens.textSecondary,
                                                                  StyleConstants.secondaryTextOpacity)

        implicitWidth: indicatorPadding + glyphPadding + label.implicitWidth
                       + (shortcutText ? StyleConstants.menuShortcutGap + shortcut.implicitWidth : 0)
                       + arrowPadding
        implicitHeight: Math.max(label.implicitHeight, shortcut.implicitHeight)
        LayoutMirroring.enabled: control.mirrored
        LayoutMirroring.childrenInherit: true

        IconGlyph {
            id: glyphIcon

            anchors.left: parent.left
            anchors.leftMargin: content.indicatorPadding
            anchors.verticalCenter: parent.verticalCenter
            visible: control.hasGlyph
            icon: control.glyph
            size: control.glyphSize
            color: control.glyphColor
            opacity: control.enabled ? 1.0 : StyleConstants.disabledOpacity
        }

        IconLabel {
            id: label

            anchors.left: parent.left
            anchors.leftMargin: content.indicatorPadding + content.glyphPadding
            anchors.right: shortcut.visible ? shortcut.left : parent.right
            anchors.rightMargin: shortcut.visible ? StyleConstants.menuShortcutGap : content.arrowPadding
            anchors.verticalCenter: parent.verticalCenter

            spacing: control.spacing
            mirrored: control.mirrored
            display: control.display
            alignment: Qt.AlignLeft
            icon: control.icon
            text: control.text
            font: control.font
            color: control.enabled ? control.labelColor : content.secondaryColor
        }

        Text {
            id: shortcut

            anchors.right: parent.right
            anchors.rightMargin: content.arrowPadding
            anchors.verticalCenter: parent.verticalCenter
            visible: content.shortcutText.length > 0

            text: content.shortcutText
            font: control.font
            color: content.secondaryColor
        }
    }

    indicator: IconGlyph {
        x: control.mirrored ? control.width - width - control.rightPadding : control.leftPadding
        y: control.topPadding + (control.availableHeight - height) / 2
        visible: control.checked
        icon: FluentIcons.Checkmark16
        size: StyleConstants.indicatorSize
        opacity: control.enabled ? 1.0 : StyleConstants.disabledOpacity
    }

    arrow: IconGlyph {
        x: control.mirrored ? control.leftPadding : control.width - width - control.rightPadding
        y: control.topPadding + (control.availableHeight - height) / 2
        visible: control.subMenu
        // ChevronDown turned to point into the submenu.
        rotation: control.mirrored ? StyleConstants.quarterTurn : -StyleConstants.quarterTurn
        icon: FluentIcons.ChevronDown20
        size: StyleConstants.chevronSize
    }

    background: Rectangle {
        implicitHeight: Theme.metrics.contextMenuItemHeight
        radius: StyleConstants.itemRadius
        color: control.down ? Theme.tokens.fillPressed
             : control.highlighted ? Theme.tokens.fillHover
             : "transparent"
    }
}
