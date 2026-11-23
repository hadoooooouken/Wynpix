pragma ComponentBehavior: Bound

import QtQuick
import wynpix.bridges
import wynpix.style

// Metadata of the current image at the right edge (ImageInfoOverlay in the
// widget UI): name and value rows, long values below their name; values are
// selectable. The panel can be dragged by its header and returns to its
// place when it opens again.
Item {
    id: root

    required property OverlayState overlay
    required property ImageInfoModel entries

    // The value field that holds the current text selection, or null. The
    // fields never take keyboard focus (the viewer keeps it for its
    // shortcuts), so Ctrl+C reaches the Actions and copies the image; while
    // a selection exists the Shortcut below copies the selected text instead.
    property TextEdit activeSelectionEdit: null

    readonly property int panelWidth: 280
    readonly property int edgeMargin: 20
    readonly property int nameWidth: 120
    readonly property int valueWidth: 142
    readonly property int rowMinimumHeight: 30
    readonly property int rowPadding: 9
    readonly property int valuePadding: 3
    readonly property int listPadding: 2
    readonly property int stubHeight: 48
    readonly property int bottomPadding: 4

    visible: overlay.open

    // Clears the selection unless it belongs to the field called except.
    function releaseSelection(except: TextEdit): void {
        if (activeSelectionEdit && activeSelectionEdit !== except)
            activeSelectionEdit.deselect();
    }

    Shortcut {
        sequence: StandardKey.Copy
        enabled: root.visible && root.activeSelectionEdit !== null
        onActivated: {
            if (root.activeSelectionEdit)
                root.activeSelectionEdit.copy();
        }
    }

    // A press outside the selected field ends the selection, so Ctrl+C
    // copies the image again.
    PointHandler {
        acceptedButtons: Qt.LeftButton
        onActiveChanged: {
            const edit = root.activeSelectionEdit;
            if (active && edit && !edit.contains(edit.mapFromItem(null, point.scenePressPosition)))
                edit.deselect();
        }
    }

    Connections {
        target: root.overlay
        function onOpenChanged() {
            if (root.overlay.open)
                panel.resetPosition();
            else
                root.releaseSelection(null);
        }
    }

    OverlayPanel {
        id: panel

        icon: FluentIcons.Info20
        title: qsTr("Image info")
        draggable: true
        width: root.panelWidth
        bottomPadding: root.bottomPadding
        placementX: root.width - width - root.edgeMargin
        placementY: (root.height - height) / 2
        onCloseClicked: root.overlay.close()

        Item {
            width: parent.width
            height: root.listPadding
        }

        Label {
            width: parent.width
            height: root.stubHeight
            visible: root.entries.empty
            text: qsTr("<no metadata found>")
            textFormat: Text.PlainText
            color: Theme.tokens.textSecondary
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
        }

        Repeater {
            model: root.entries

            delegate: Item {
                id: entry

                required property string name
                required property string value
                required property bool stacked

                width: parent.width
                height: Math.max(root.rowMinimumHeight, entryLayout.height)

                Rectangle {
                    anchors.fill: parent
                    visible: hover.hovered
                    color: Theme.tokens.fillHover
                }

                HoverHandler {
                    id: hover
                }

                Flow {
                    id: entryLayout

                    x: root.rowPadding
                    width: parent.width - 2 * root.rowPadding

                    Label {
                        width: entry.stacked ? entryLayout.width : root.nameWidth
                        text: entry.name
                        color: Theme.tokens.textSecondary
                    }

                    TextEdit {
                        id: valueEdit

                        width: entry.stacked ? entryLayout.width : root.valueWidth
                        leftPadding: root.valuePadding
                        text: entry.value
                        textFormat: TextEdit.PlainText
                        wrapMode: TextEdit.Wrap
                        readOnly: true
                        selectByMouse: true
                        activeFocusOnPress: false
                        color: Theme.tokens.textSecondary
                        selectionColor: Theme.tokens.accent
                        selectedTextColor: Theme.tokens.accentText
                        font: Theme.fonts.body

                        onSelectedTextChanged: {
                            if (selectedText.length > 0) {
                                root.releaseSelection(valueEdit);
                                root.activeSelectionEdit = valueEdit;
                            } else if (root.activeSelectionEdit === valueEdit) {
                                root.activeSelectionEdit = null;
                            }
                        }

                        // A press in another field ends the selection of
                        // the previous one even when it selects nothing.
                        PointHandler {
                            acceptedButtons: Qt.LeftButton
                            onActiveChanged: {
                                if (active)
                                    root.releaseSelection(valueEdit);
                            }
                        }
                    }
                }
            }
        }
    }
}
