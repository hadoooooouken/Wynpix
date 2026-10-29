pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Layouts
import wynpix.bridges
import wynpix.style

// Settings page "Controls": the shortcuts, one row per action under its
// category, with a search over titles, categories and shortcuts. "+" on a
// row (or a click on it) adds a shortcut to its action; a shortcut
// chip edits that shortcut and its cross removes it. Below: clickable edges,
// image scrolling and mouse scrolling speed.
SettingsPage {
    id: root

    required property SettingsEditorModel editor

    readonly property int listHeight: 320
    readonly property int listPadding: 4
    readonly property int rowHeight: 30
    readonly property int sectionHeight: 30
    readonly property int categoryGap: 10
    readonly property int chipHeight: 22
    readonly property int chipSpacing: 4
    readonly property int chipRemoveSize: 16
    readonly property int chipRemoveGap: 2
    readonly property int chipRemovePadding: 3
    readonly property int titleChipGap: 12

    title: qsTranslate("SettingsDialog", "Controls")

    Connections {
        target: root.editor

        function onShortcutPut(row) {
            shortcutList.positionViewAtIndex(row, ListView.Contain);
        }

        // A new search shows its results from the first one.
        function onShortcutFilterChanged() {
            shortcutList.positionViewAtBeginning();
        }
    }

    // One shortcut of a row: a click edits it, the cross removes it.
    component ShortcutChip: Rectangle {
        id: chip

        required property string action
        required property string shortcut

        objectName: "shortcutChip"
        implicitWidth: StyleConstants.indicatorPadding + chipLabel.implicitWidth + root.chipRemoveGap
                       + root.chipRemoveSize + root.chipRemovePadding
        implicitHeight: root.chipHeight
        radius: StyleConstants.controlRadius
        color: chipArea.pressed ? Theme.tokens.fillPressed
             : chipArea.containsMouse ? Theme.tokens.fillHover
             : Theme.tokens.fillRest
        border.width: StyleConstants.separatorThickness
        border.color: Theme.tokens.strokeDefault

        MouseArea {
            id: chipArea

            anchors.fill: parent
            hoverEnabled: true
            onClicked: root.editor.editShortcut(chip.action, chip.shortcut)
        }

        Label {
            id: chipLabel

            x: StyleConstants.indicatorPadding
            anchors.verticalCenter: parent.verticalCenter
            text: chip.shortcut
            textFormat: Text.PlainText
        }

        Rectangle {
            x: chipLabel.x + chipLabel.width + root.chipRemoveGap
            anchors.verticalCenter: parent.verticalCenter
            width: root.chipRemoveSize
            height: root.chipRemoveSize
            radius: StyleConstants.controlRadius
            color: removeArea.pressed ? Theme.tokens.fillPressed
                 : removeArea.containsMouse ? Theme.tokens.fillHover
                 : "transparent"

            IconGlyph {
                anchors.centerIn: parent
                icon: FluentIcons.Dismiss12
                size: StyleConstants.chevronSize
                color: removeArea.containsMouse ? Theme.tokens.textPrimary : Theme.tokens.textSecondary
            }

            MouseArea {
                id: removeArea

                anchors.fill: parent
                hoverEnabled: true
                onClicked: root.editor.removeShortcut(chip.action, chip.shortcut)
            }

            ToolTip {
                visible: removeArea.containsMouse
                text: qsTranslate("SettingsDialog", "Remove")
            }
        }
    }

    SettingsSection {
        RowLayout {
            spacing: StyleConstants.dialogSpacing

            SearchField {
                Layout.fillWidth: true
                placeholderText: qsTranslate("SettingsDialog", "Search actions or shortcuts")
                text: root.editor.shortcutFilter
                // Not textEdited: the clear button and Escape clear the text
                // without it.
                onTextChanged: root.editor.shortcutFilter = text
            }
            Button {
                text: qsTranslate("SettingsDialog", "Reset to defaults")
                onClicked: root.editor.resetShortcuts()
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: root.listHeight
            radius: StyleConstants.controlRadius
            color: Theme.tokens.surfaceView
            border.color: Theme.tokens.strokeDefault
            border.width: StyleConstants.separatorThickness

            ListView {
                id: shortcutList

                // Clear of the scroll bar, which lies over the list's right edge,
                // by the list padding: the handle gets the same space on both
                // sides.
                readonly property int scrollBarGap: visibleArea.heightRatio < 1.0
                                                    ? StyleConstants.scrollBarThickness + root.listPadding : 0

                anchors.fill: parent
                anchors.margins: root.listPadding
                clip: true
                model: root.editor.shortcuts
                boundsBehavior: Flickable.StopAtBounds
                ScrollBar.vertical: ScrollBar {}

                section.property: "category"
                section.criteria: ViewSection.FullString
                section.labelPositioning: ViewSection.InlineLabels | ViewSection.CurrentLabelAtStart
                // The header of the category at the top stays there; opaque, as
                // the rows scroll under it. A fixed band at the top of the list
                // is tinted with the accent: each header tints only its part
                // inside the band, so while a header pushes the pinned one out
                // the two parts make one band and the titles slide through it.
                section.delegate: Rectangle {
                    id: sectionHeader

                    required property string section

                    // Distance of the header from the band, up (< 0) or down.
                    readonly property real bandOffset: y - shortcutList.contentY

                    width: ListView.view.width - shortcutList.scrollBarGap
                    height: root.sectionHeight
                    color: Theme.tokens.surfaceView

                    Rectangle {
                        y: Math.max(0, -sectionHeader.bandOffset)
                        width: parent.width
                        height: Math.max(0, sectionHeader.height - Math.abs(sectionHeader.bandOffset))
                        visible: height > 0
                        color: Theme.tokens.accentTint
                        // Rounded only where the part ends with the band.
                        topLeftRadius: sectionHeader.bandOffset <= 0 ? StyleConstants.rowRadius : 0
                        topRightRadius: topLeftRadius
                        bottomLeftRadius: sectionHeader.bandOffset >= 0 ? StyleConstants.rowRadius : 0
                        bottomRightRadius: bottomLeftRadius
                    }

                    Label {
                        anchors.left: parent.left
                        anchors.leftMargin: StyleConstants.indicatorPadding
                        anchors.verticalCenter: parent.verticalCenter
                        text: sectionHeader.section
                        font: Theme.fonts.bodyStrong
                    }
                }

                // The last row of a category keeps a gap to the next category.
                delegate: Item {
                    id: actionRow

                    required property string action
                    required property string title
                    required property list<string> shortcuts

                    readonly property bool lastOfCategory: ListView.nextSection !== ListView.section

                    objectName: "shortcutRow"
                    width: ListView.view.width - shortcutList.scrollBarGap
                    height: root.rowHeight + (lastOfCategory ? root.categoryGap : 0)

                    RowBackground {
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.top: parent.top
                        height: root.rowHeight
                        hovered: rowHover.hovered

                        HoverHandler {
                            id: rowHover
                        }

                        // Between the rows of a category.
                        Rectangle {
                            anchors.left: parent.left
                            anchors.right: parent.right
                            anchors.leftMargin: StyleConstants.indicatorPadding
                            anchors.rightMargin: StyleConstants.indicatorPadding
                            anchors.bottom: parent.bottom
                            height: StyleConstants.separatorThickness
                            color: Theme.tokens.rowHover
                            visible: !actionRow.lastOfCategory && !rowHover.hovered
                        }

                        // Under the chips and the button, which take their own clicks.
                        MouseArea {
                            anchors.fill: parent
                            onClicked: root.editor.addShortcut(actionRow.action)
                        }

                        Label {
                            anchors.left: parent.left
                            anchors.leftMargin: StyleConstants.indicatorPadding
                            anchors.right: chips.left
                            anchors.rightMargin: root.titleChipGap
                            anchors.verticalCenter: parent.verticalCenter
                            elide: Text.ElideRight
                            textFormat: Text.PlainText
                            text: actionRow.title
                        }

                        Row {
                            id: chips

                            anchors.right: addButton.left
                            anchors.rightMargin: root.chipSpacing
                            anchors.verticalCenter: parent.verticalCenter
                            spacing: root.chipSpacing

                            Repeater {
                                model: actionRow.shortcuts

                                ShortcutChip {
                                    required property string modelData

                                    action: actionRow.action
                                    shortcut: modelData
                                }
                            }
                        }

                        ToolButton {
                            id: addButton

                            anchors.right: parent.right
                            anchors.rightMargin: StyleConstants.indicatorPadding
                            anchors.verticalCenter: parent.verticalCenter
                            implicitWidth: root.chipHeight
                            implicitHeight: root.chipHeight
                            topInset: 0
                            bottomInset: 0
                            leftInset: 0
                            rightInset: 0
                            padding: 0
                            opacity: rowHover.hovered || addButton.visualFocus ? 1.0 : 0.0
                            contentItem: IconGlyph {
                                icon: FluentIcons.Add16
                                size: Theme.compactIconSize
                            }
                            onClicked: root.editor.addShortcut(actionRow.action)

                            ToolTip {
                                visible: addButton.hovered
                                text: qsTranslate("ShortcutCreatorDialog", "Add shortcut")
                            }
                        }
                    }
                }
            }

            Label {
                anchors.centerIn: parent
                visible: shortcutList.count === 0
                text: qsTranslate("SettingsDialog", "No matching actions")
                color: Theme.tokens.textSecondary
            }
        }
    }

    SettingsSection {
        RowLayout {
            spacing: StyleConstants.dialogSpacing

            CheckBox {
                text: qsTranslate("SettingsDialog", "Switch image by clicking window edges")
                checked: root.editor.controls.clickableEdges
                onToggled: root.editor.controls.clickableEdges = checked
            }
            CheckBox {
                text: qsTranslate("SettingsDialog", "Visible edges")
                enabled: root.editor.controls.clickableEdges
                checked: root.editor.controls.clickableEdgesVisible
                onToggled: root.editor.controls.clickableEdgesVisible = checked
            }
        }
        RowLayout {
            spacing: StyleConstants.dialogSpacing

            Label {
                text: qsTranslate("SettingsDialog", "Scroll image with:")
            }
            SettingsComboBox {
                options: root.editor.imageScrollingModes
                value: root.editor.controls.imageScrolling
                onPicked: value => root.editor.controls.imageScrolling = value
            }
        }
        SettingsNote {
            text: qsTranslate("SettingsDialog", "Note: you can also zoom by holding RMB and moving the mouse")
        }
        SettingsSliderRow {
            label: qsTranslate("SettingsDialog", "Mouse scrolling speed:")
            range: root.editor.ranges.mouseScrollingSpeed
            value: root.editor.controls.mouseScrollingSpeedStep
            valueText: root.editor.mouseScrollingSpeedText(value)
            onMoved: value => root.editor.controls.mouseScrollingSpeedStep = value
        }
        CheckBox {
            text: qsTranslate("SettingsDialog", "Trackpad detection")
            checked: root.editor.controls.trackpadDetection
            onToggled: root.editor.controls.trackpadDetection = checked
        }
        SettingsNote {
            text: qsTranslate("SettingsDialog", "Disable if you have issues with mouse scrolling")
        }
    }
}
