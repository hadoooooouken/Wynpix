pragma ComponentBehavior: Bound

import QtQuick
import QtQml.Models
import QtQuick.Templates as T
import wynpix.bridges
import wynpix.style

// The viewer's context menu (ContextMenu in the widget UI): a row of zoom
// buttons and a row of transform buttons, then the action rows with their
// shortcuts, "More" expanding the rarely used rows in place and the
// "Open with..." submenu of scripts. ContextMenuModel holds the rows and
// decides when the menu opens; the menu is its own window, so it can extend
// past the main window's edges, and opens at the pointer. Named apart from
// the ContextMenu attached type of Qt Quick Controls; its strings keep the
// widget menu's translation context.
Menu {
    id: root

    required property ContextMenuModel menuModel

    // Items declared before the instantiated rows: the button rows and the
    // separator below them.
    readonly property int fixedItemCount: 2
    readonly property int buttonRowSpacing: 7
    readonly property int buttonRowInset: 4

    readonly property real widestRow: {
        let width = Math.max(zoomRow.minimumWidth, transformRow.minimumWidth);
        for (let i = 0; i < rows.count; ++i) {
            const row = rows.objectAt(i) as Item;
            if (row)
                width = Math.max(width, row.implicitWidth);
        }
        return width;
    }

    popupType: T.Popup.Window
    // All rows laid out at once: the style's ListView only lays out the rows
    // in view and estimates the rest, which leaves the window too short
    // once "More" expands. Hidden rows take no space in a Column.
    contentItem: Column {
        Repeater {
            model: root.contentModel
        }
    }
    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            widestRow + leftPadding + rightPadding)

    // The scripts submenu's own row in this menu.
    delegate: MenuItem {
        id: submenuRow

        readonly property ScriptsMenu scripts: submenuRow.subMenu as ScriptsMenu

        hasGlyph: scripts !== null
        glyph: scripts ? scripts.glyph : FluentIcons.OpenWith20
        enabled: scripts ? scripts.available : true
        visible: scripts ? scripts.shown : true
        height: visible ? implicitHeight : 0
    }

    // No row is current until the pointer or a key picks one: popup() clears
    // the index, but a row can become current while the window opens.
    onOpened: root.currentIndex = -1
    onClosed: root.menuModel.close()
    // Created by the first opening, after the model opened.
    Component.onCompleted: {
        if (root.menuModel.open)
            root.popup();
    }

    Connections {
        target: root.menuModel

        function onOpenChanged() {
            if (root.menuModel.open)
                root.popup();
            else
                root.dismiss();
        }
    }

    // Zoom and transform buttons; they act on press and keep the menu open,
    // so the image can be zoomed or turned several times. A button that
    // opens a modal UI (resize) closes it: the modal window would leave the
    // menu open and unable to take input.
    Column {
        width: root.availableWidth
        spacing: root.buttonRowSpacing
        bottomPadding: root.buttonRowSpacing

        ButtonRow {
            id: zoomRow

            buttons: root.menuModel.zoomButtons
        }

        ButtonRow {
            id: transformRow

            buttons: root.menuModel.transformButtons
        }
    }

    MenuSeparator {}

    Instantiator {
        id: rows

        model: root.menuModel.items
        delegate: DelegateChooser {
            role: "entryKind"

            DelegateChoice {
                roleValue: ContextMenuModel.Action
                ActionRow {}
            }
            DelegateChoice {
                roleValue: ContextMenuModel.Separator
                SeparatorRow {}
            }
            DelegateChoice {
                roleValue: ContextMenuModel.Expander
                ExpanderRow {}
            }
            DelegateChoice {
                roleValue: ContextMenuModel.Submenu
                ScriptsMenu {}
            }
        }

        onObjectAdded: (index, object) => {
            if (object instanceof ScriptsMenu)
                root.insertMenu(root.fixedItemCount + index, object as ScriptsMenu);
            else
                root.insertItem(root.fixedItemCount + index, object as Item);
        }
        onObjectRemoved: (index, object) => {
            if (object instanceof ScriptsMenu)
                root.removeMenu(object as ScriptsMenu);
            else
                root.removeItem(object as Item);
        }
    }

    // A row of buttons sharing the menu's width equally, as the widget menu's
    // layout stretched them; the menu is at least wide enough for square
    // buttons.
    component ButtonRow: Row {
        id: buttonRow

        required property var buttons
        readonly property real minimumWidth: rowButtons.count * Theme.metrics.contextMenuItemHeight
                                             + leftPadding + rightPadding
        readonly property real buttonWidth: rowButtons.count > 0
                                            ? (width - leftPadding - rightPadding) / rowButtons.count
                                            : 0

        width: root.availableWidth
        leftPadding: root.buttonRowInset
        rightPadding: root.buttonRowInset

        Repeater {
            id: rowButtons

            model: buttonRow.buttons
            delegate: MenuButton {
                width: buttonRow.buttonWidth
            }
        }
    }

    // The widget menu's button: the height of a menu row, no inset, the
    // glyph centred.
    component MenuButton: ToolButton {
        id: button

        required property string entryAction
        required property string entryText
        required property int entryIcon
        required property bool entryEnabled
        required property bool entryClosesMenu

        implicitHeight: Theme.metrics.contextMenuItemHeight
        // The style sets each side, which overrides padding.
        topPadding: 0
        bottomPadding: 0
        leftPadding: 0
        rightPadding: 0
        topInset: 0
        bottomInset: 0
        leftInset: 0
        rightInset: 0
        enabled: entryEnabled
        focusPolicy: Qt.NoFocus
        hoverEnabled: true
        contentItem: IconGlyph {
            icon: button.entryIcon
            opacity: button.enabled ? 1.0 : StyleConstants.disabledOpacity
        }
        // The accent highlight of the menu rows, not the panel button tint.
        background: Rectangle {
            radius: StyleConstants.controlRadius
            color: button.hovered
                   ? Color.transparent(Theme.colors.accent, StyleConstants.accentHighlightOpacity)
                   : "transparent"
        }

        ToolTip {
            visible: button.hovered
            text: button.entryText
        }
        Accessible.name: entryText

        // Not a menu item, so the menu would keep the last hovered row
        // highlighted.
        onHoveredChanged: {
            if (hovered)
                root.currentIndex = -1;
        }
        onPressed: {
            if (entryClosesMenu)
                root.menuModel.trigger(entryAction);
            else
                root.menuModel.invoke(entryAction);
        }
    }

    component ActionRow: MenuItem {
        id: row

        required property string entryAction
        required property string entryText
        required property int entryIcon
        required property int entryTone
        required property string entryShortcut
        required property bool entryEnabled
        required property bool entryShown

        readonly property color toneColor: entryTone === ContextMenuModel.Trash ? Theme.colors.trash
                                         : entryTone === ContextMenuModel.Danger ? Theme.colors.danger
                                         : "transparent"
        readonly property bool toned: entryTone !== ContextMenuModel.Normal

        text: entryText
        hasGlyph: true
        glyph: entryIcon
        glyphColor: toned ? toneColor : Theme.colors.icons
        labelColor: toned ? toneColor : Theme.colors.textHc
        shortcutText: entryShortcut
        enabled: entryEnabled
        visible: entryShown
        height: visible ? implicitHeight : 0

        // The highlight follows the pointer, as in the widget menu: a row the
        // pointer leaves stays current in a Menu otherwise.
        onHoveredChanged: {
            if (!hovered && highlighted && menu)
                menu.currentIndex = -1;
        }
        onTriggered: root.menuModel.trigger(entryAction)
    }

    component SeparatorRow: MenuSeparator {
        required property bool entryShown

        visible: entryShown
        height: visible ? implicitHeight : 0
    }

    // "More": expands the rows below in place and keeps the menu open, so it
    // is not a MenuItem (a triggered MenuItem closes the menu).
    component ExpanderRow: Item {
        id: expander

        required property string entryText
        required property int entryIcon

        implicitWidth: expanderContent.implicitWidth + 2 * StyleConstants.menuItemHorizontalPadding
        implicitHeight: Theme.metrics.contextMenuItemHeight
        width: root.availableWidth

        Rectangle {
            anchors.fill: parent
            visible: expanderArea.containsMouse
            radius: StyleConstants.itemRadius
            color: Color.transparent(Theme.colors.accent, StyleConstants.accentHighlightOpacity)
        }

        Row {
            id: expanderContent

            anchors.left: parent.left
            anchors.leftMargin: StyleConstants.menuItemHorizontalPadding
            anchors.verticalCenter: parent.verticalCenter
            spacing: StyleConstants.menuItemSpacing

            IconGlyph {
                anchors.verticalCenter: parent.verticalCenter
                icon: expander.entryIcon
            }

            Label {
                anchors.verticalCenter: parent.verticalCenter
                text: expander.entryText
                color: Theme.colors.textHc
            }
        }

        MouseArea {
            id: expanderArea

            anchors.fill: parent
            hoverEnabled: true
            // Not a menu item, so the menu would keep the last hovered row
            // highlighted.
            onEntered: root.currentIndex = -1
            onPressed: root.menuModel.toggleMore()
        }

        Accessible.role: Accessible.Button
        Accessible.name: entryText
    }

    // "Open with...": the scripts with a command, then "Configure menu".
    component ScriptsMenu: Menu {
        id: scriptsMenu

        required property string entryText
        required property int entryIcon
        required property bool entryEnabled
        required property bool entryShown

        readonly property int glyph: entryIcon
        readonly property bool available: entryEnabled
        readonly property bool shown: entryShown

        title: entryText

        Instantiator {
            model: root.menuModel.scripts
            delegate: ActionRow {}
            onObjectAdded: (index, object) => scriptsMenu.insertItem(index, object as Item)
            onObjectRemoved: (index, object) => scriptsMenu.removeItem(object as Item)
        }

        MenuSeparator {}

        MenuItem {
            text: qsTranslate("ContextMenu", "Configure menu")
            hasGlyph: true
            glyph: FluentIcons.Settings20
            onTriggered: root.menuModel.configureScripts()
        }
    }
}
