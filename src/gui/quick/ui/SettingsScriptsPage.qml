pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Layouts
import wynpix.bridges
import wynpix.style

// Settings page "Scripts": the "Open with" entries (add, edit, remove; a
// double click edits one). Changes take effect at once.
SettingsPage {
    id: root

    required property SettingsEditorModel editor

    readonly property int listHeight: 300
    readonly property int listPadding: 4
    readonly property int rowHeight: 30
    // Keeps the fills of a hovered and a selected row apart.
    readonly property int rowSpacing: 2

    title: qsTranslate("SettingsDialog", "Scripts")

    SettingsSection {
        SettingsNote {
            text: qsTranslate("SettingsDialog", "Note: these will appear in \"Open with\" menu.")
        }
        SettingsNote {
            text: qsTranslate("SettingsDialog", "Also, you can assign shortcuts to scripts (in \"Controls\" section).")
        }
        RowLayout {
            spacing: StyleConstants.dialogSpacing

            Button {
                text: qsTranslate("SettingsDialog", "Add")
                onClicked: root.editor.addScript()
            }
            Button {
                text: qsTranslate("SettingsDialog", "Edit")
                enabled: scriptList.currentIndex >= 0
                onClicked: root.editor.editScript(scriptList.currentIndex)
            }
            Button {
                text: qsTranslate("SettingsDialog", "Remove")
                enabled: scriptList.currentIndex >= 0
                onClicked: root.editor.removeScript(scriptList.currentIndex)
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: root.listHeight
            radius: StyleConstants.controlRadius
            color: Theme.tokens.surfaceView
            border.color: Theme.tokens.strokeDefault
            border.width: StyleConstants.separatorThickness

            Label {
                anchors.centerIn: parent
                text: qsTranslate("SettingsDialog", "No scripts yet")
                color: Theme.tokens.textTertiary
                visible: scriptList.count === 0
            }

            ListView {
                id: scriptList

                // Clear of the scroll bar, which lies over the list's right edge,
                // by the list padding: the handle gets the same space on both
                // sides.
                readonly property int scrollBarGap: visibleArea.heightRatio < 1.0
                                                    ? StyleConstants.scrollBarThickness + root.listPadding : 0

                anchors.fill: parent
                anchors.margins: root.listPadding
                spacing: root.rowSpacing
                clip: true
                model: root.editor.scripts
                currentIndex: -1
                boundsBehavior: Flickable.StopAtBounds
                ScrollBar.vertical: ScrollBar {}

                DialogWheelScroller {
                    flickable: scriptList
                }

                delegate: RowBackground {
                    id: scriptRow

                    required property int index
                    required property string display

                    width: ListView.view.width - scriptList.scrollBarGap
                    height: root.rowHeight
                    selected: ListView.isCurrentItem
                    hovered: rowHover.hovered

                    HoverHandler {
                        id: rowHover
                    }

                    Label {
                        anchors.fill: parent
                        anchors.leftMargin: StyleConstants.indicatorPadding
                        verticalAlignment: Text.AlignVCenter
                        elide: Text.ElideRight
                        textFormat: Text.PlainText
                        text: scriptRow.display
                    }

                    MouseArea {
                        anchors.fill: parent
                        onClicked: scriptList.currentIndex = scriptRow.index
                        onDoubleClicked: {
                            scriptList.currentIndex = scriptRow.index;
                            root.editor.editScript(scriptRow.index);
                        }
                    }
                }
            }
        }
    }
}
