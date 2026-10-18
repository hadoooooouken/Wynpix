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
// decides when the menu opens, and opens at the pointer. Named apart from
// the ContextMenu attached type of Qt Quick Controls; its strings keep the
// widget menu's translation context.
Menu {
    id: root

    required property ContextMenuModel menuModel

    // Set while an open menu is closed only to be shown again at the
    // pointer, so the closing does not reach the model.
    property bool reopening: false

    // Items declared before the instantiated rows: the zoom row, the
    // transform row and the separator below them.
    readonly property int fixedItemCount: 3
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

    popupType: T.Popup.Item
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
        // A hidden row stays disabled: the menu's arrow keys skip disabled
        // rows but not invisible ones, and would stop on it unseen.
        enabled: scripts ? scripts.available && scripts.shown : true
        visible: scripts ? scripts.shown : true
        height: visible ? implicitHeight : 0
    }

    // No row is current until the pointer or a key picks one: popup() clears
    // the index, but a row can become current while the window opens.
    onOpened: root.currentIndex = -1
    onClosed: {
        if (!root.reopening)
            root.menuModel.close();
    }
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

        // A right click while the menu is open: dismiss() also closes an
        // open submenu, which popup() alone would leave at the old place.
        function onRepositionRequested() {
            root.reopening = true;
            root.dismiss();
            root.reopening = false;
            root.popup();
        }
    }

    // Zoom and transform buttons keep the menu open, so the image can be
    // zoomed or turned several times. A button that opens a modal UI (resize)
    // closes it: the modal window would leave the menu open and unable to
    // take input.
    ButtonRow {
        id: zoomRow

        buttons: root.menuModel.zoomButtons
    }

    ButtonRow {
        id: transformRow

        buttons: root.menuModel.transformButtons
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
    // buttons. The row is a menu item so that the arrow keys reach it: Up and
    // Down arrive at it like at any row, Left and Right pick a button, Return
    // and Space press it. The row never triggers as a menu item, which would
    // close the menu; the buttons act on their own.
    component ButtonRow: MenuItem {
        id: buttonRow

        required property var buttons
        // The button the keys act on and the pointer last touched.
        property int currentButton: 0
        readonly property real minimumWidth: rowButtons.count * Theme.metrics.contextMenuItemHeight
                                             + leftPadding + rightPadding
        readonly property real buttonWidth: rowButtons.count > 0
                                            ? (width - leftPadding - rightPadding) / rowButtons.count
                                            : 0
        // Read from the delegates' own flag: the Item's enabled follows this
        // row, so a disabled row could never become enabled again.
        readonly property bool anyButtonEnabled: {
            for (let i = 0; i < rowButtons.count; ++i) {
                const candidate = rowButtons.itemAt(i) as MenuButton;
                if (candidate && candidate.entryEnabled)
                    return true;
            }
            return false;
        }

        topPadding: 0
        bottomPadding: root.buttonRowSpacing
        leftPadding: root.buttonRowInset
        rightPadding: root.buttonRowInset
        // The buttons follow the pointer themselves; a hovered row would
        // become current with a stale button.
        hoverEnabled: false
        // Skipped by the arrow keys while no button can act, as the buttons
        // are drawn disabled then.
        enabled: anyButtonEnabled
        background: null

        contentItem: Row {
            Repeater {
                id: rowButtons

                model: buttonRow.buttons
                delegate: MenuButton {
                    width: buttonRow.buttonWidth
                    keyboardCurrent: buttonRow.highlighted && index === buttonRow.currentButton
                    onHoveredChanged: buttonRow.buttonHovered(index, hovered)
                }
            }
        }

        // Presses in the gaps around the buttons would reach the row, and a
        // triggered menu item closes the menu.
        MouseArea {
            z: -1
            anchors.fill: parent
            acceptedButtons: Qt.AllButtons
        }

        Keys.onLeftPressed: event => {
            buttonRow.stepButton(-1);
            event.accepted = true;
        }
        Keys.onRightPressed: event => {
            buttonRow.stepButton(1);
            event.accepted = true;
        }
        Keys.onReturnPressed: event => {
            buttonRow.pressCurrentButton();
            event.accepted = true;
        }
        Keys.onEnterPressed: event => {
            buttonRow.pressCurrentButton();
            event.accepted = true;
        }
        Keys.onSpacePressed: event => {
            buttonRow.pressCurrentButton();
            event.accepted = true;
        }

        // A row that is entered by key starts at a button that can act.
        onHighlightedChanged: {
            if (highlighted)
                buttonRow.settleOnEnabledButton();
        }

        // The row's own index in the menu.
        function menuIndex(): int {
            for (let i = 0; i < root.fixedItemCount; ++i) {
                if (root.itemAt(i) === buttonRow)
                    return i;
            }
            return -1;
        }

        // The pointer enters a button: the row becomes current with that
        // button. The pointer leaves it: nothing stays highlighted.
        function buttonHovered(index: int, entered: bool): void {
            if (entered) {
                buttonRow.currentButton = index;
                root.currentIndex = buttonRow.menuIndex();
            } else if (buttonRow.highlighted && buttonRow.currentButton === index) {
                root.currentIndex = -1;
            }
        }

        // Moves to the nearest button in direction (-1 or 1) that can act;
        // stays where it is at either end.
        function stepButton(direction: int): void {
            for (let i = buttonRow.currentButton + direction; i >= 0 && i < rowButtons.count; i += direction) {
                const candidate = rowButtons.itemAt(i) as MenuButton;
                if (candidate && candidate.entryEnabled) {
                    buttonRow.currentButton = i;
                    return;
                }
            }
        }

        function settleOnEnabledButton(): void {
            const current = rowButtons.itemAt(buttonRow.currentButton) as MenuButton;
            if (current && current.entryEnabled)
                return;
            for (let i = 0; i < rowButtons.count; ++i) {
                const candidate = rowButtons.itemAt(i) as MenuButton;
                if (candidate && candidate.entryEnabled) {
                    buttonRow.currentButton = i;
                    return;
                }
            }
        }

        function pressCurrentButton(): void {
            const current = rowButtons.itemAt(buttonRow.currentButton) as MenuButton;
            if (current && current.entryEnabled)
                current.activate();
        }
    }

    // The widget menu's button: the height of a menu row, no inset, the
    // glyph centred.
    component MenuButton: ToolButton {
        id: button

        required property int index
        required property string entryAction
        required property string entryText
        required property int entryIcon
        required property bool entryEnabled
        required property bool entryClosesMenu

        // The row's keys act on this button.
        property bool keyboardCurrent: false

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
            color: button.hovered || button.keyboardCurrent
                   ? Color.transparent(Theme.colors.accent, StyleConstants.accentHighlightOpacity)
                   : "transparent"
        }

        ToolTip {
            visible: button.hovered || button.keyboardCurrent
            text: button.entryText
        }
        Accessible.name: entryText

        onClicked: button.activate()

        // Runs the button's action; a click and the row's keys both end here.
        function activate(): void {
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
        // A hidden row stays disabled: the menu's arrow keys skip disabled
        // rows but not invisible ones, and would stop on it unseen.
        enabled: entryEnabled && entryShown
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

        popupType: T.Popup.Item

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
