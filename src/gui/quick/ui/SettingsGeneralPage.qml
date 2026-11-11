import QtQuick
import QtQuick.Layouts
import wynpix.bridges
import wynpix.style

// Settings page "General": language, startup, user interface, thumbnail
// panel, folder navigation and slideshow.
SettingsPage {
    id: root

    required property SettingsEditorModel editor

    readonly property int sliderLabelWidth: 200

    title: qsTranslate("SettingsDialog", "General")

    SettingsSection {
        RowLayout {
            spacing: StyleConstants.dialogSpacing

            Label {
                text: qsTranslate("SettingsDialog", "Language:")
            }
            SettingsComboBox {
                options: root.editor.languages
                value: root.editor.general.language
                onPicked: value => root.editor.general.language = value
            }
            SettingsNote {
                text: qsTranslate("SettingsDialog", "Requires application restart")
            }
        }

        CheckBox {
            text: qsTranslate("SettingsDialog", "Open in fullscreen")
            checked: root.editor.general.fullscreenMode
            onToggled: root.editor.general.fullscreenMode = checked
        }
        CheckBox {
            text: qsTranslate("SettingsDialog", "Start in folder view by default")
            checked: root.editor.general.startInFolderView
            onToggled: root.editor.general.startInFolderView = checked
        }
        CheckBox {
            id: standbyBox

            text: qsTranslate("SettingsDialog", "Enable standby mode on close")
            checked: root.editor.general.standbyMode
            onToggled: root.editor.general.standbyMode = checked

            ToolTip {
                text: qsTranslate("SettingsDialog", "Keeps the application running in the background when closed. Subsequent launches will be instant.")
                visible: standbyBox.hovered
            }
        }
        CheckBox {
            text: qsTranslate("SettingsDialog", "Remember last opened folder")
            checked: root.editor.general.rememberLastFolder
            onToggled: root.editor.general.rememberLastFolder = checked
        }
    }

    SettingsSection {
        title: qsTranslate("SettingsDialog", "User interface")

        CheckBox {
            id: smoothScrollBox

            text: qsTranslate("SettingsDialog", "Smooth thumbnail scrolling")
            checked: root.editor.general.smoothScroll
            onToggled: root.editor.general.smoothScroll = checked

            ToolTip {
                text: qsTranslate("SettingsDialog", "Turn this off if you are using a touchpad with libinput driver.")
                visible: smoothScrollBox.hovered
            }
        }
        RowLayout {
            spacing: StyleConstants.dialogSpacing

            CheckBox {
                text: qsTranslate("SettingsDialog", "Automatic window resize")
                checked: root.editor.general.autoResizeWindow
                onToggled: root.editor.general.autoResizeWindow = checked
            }
            SettingsNote {
                text: qsTranslate("SettingsDialog", "Match displayed content")
            }
        }
        SettingsSliderRow {
            label: qsTranslate("SettingsDialog", "Screen area limit for auto resize:")
            range: root.editor.ranges.autoResizeLimit
            value: root.editor.general.autoResizeLimitStep
            valueText: root.editor.autoResizeLimitText(value)
            onMoved: value => root.editor.general.autoResizeLimitStep = value
        }
    }

    SettingsSection {
        title: qsTranslate("SettingsDialog", "Files & folders")

        RowLayout {
            spacing: StyleConstants.dialogSpacing

            Label {
                text: qsTranslate("SettingsDialog", "Default sorting mode:")
            }
            SettingsComboBox {
                options: root.editor.sortingModes
                value: root.editor.general.sortingMode
                onPicked: value => root.editor.general.sortingMode = value
            }
        }
        CheckBox {
            text: qsTranslate("SettingsDialog", "Apply sorting to folders")
            checked: root.editor.general.sortFolders
            onToggled: root.editor.general.sortFolders = checked
        }
        CheckBox {
            text: qsTranslate("SettingsDialog", "Show hidden files")
            checked: root.editor.general.showHiddenFiles
            onToggled: root.editor.general.showHiddenFiles = checked
        }
    }

    SettingsSection {
        title: qsTranslate("SettingsDialog", "Slideshow")

        RowLayout {
            spacing: StyleConstants.dialogSpacing

            Label {
                text: qsTranslate("SettingsDialog", "Switch interval:")
            }
            SpinBox {
                id: intervalBox

                from: root.editor.ranges.slideshowInterval.from
                to: root.editor.ranges.slideshowInterval.to
                value: root.editor.general.slideshowIntervalMs
                editable: true
                textFromValue: value => value + qsTranslate("SettingsDialog", "ms")
                valueFromText: text => parseInt(text, 10)
                onValueModified: root.editor.general.slideshowIntervalMs = intervalBox.value
            }
            CheckBox {
                text: qsTranslate("SettingsDialog", "Loop slideshow")
                checked: root.editor.general.loopSlideshow
                onToggled: root.editor.general.loopSlideshow = checked
            }
        }
    }
}
