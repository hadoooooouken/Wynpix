import QtQuick
import QtQuick.Dialogs
import QtQuick.Layouts
import wynpix.bridges
import wynpix.style

// Settings page "Viewer": fit and focus, zoom, navigation, overlays,
// thumbnail panel, the scaling filter, colour management and HDR tone mapping.
SettingsPage {
    id: root

    required property SettingsEditorModel editor

    readonly property int zoomLevelsWidth: 380
    readonly property int sliderLabelWidth: 200

    title: qsTranslate("SettingsDialog", "Viewer")

    SettingsSection {
        title: qsTranslate("SettingsDialog", "Display options")

        SettingsRadioRow {
            label: qsTranslate("SettingsDialog", "Image fit:")
            options: root.editor.fitModes
            value: root.editor.view.imageFitMode
            onPicked: value => root.editor.view.imageFitMode = value
        }
        CheckBox {
            text: qsTranslate("SettingsDialog", "Keep fit mode selected via hotkey when switching images")
            checked: root.editor.view.keepFitMode
            onToggled: root.editor.view.keepFitMode = checked
        }
        SettingsRadioRow {
            label: qsTranslate("SettingsDialog", "Focus in 1:1 mode:")
            options: root.editor.focusPoints
            value: root.editor.view.focusPoint
            onPicked: value => root.editor.view.focusPoint = value
        }
        SettingsNote {
            text: qsTranslate("SettingsDialog", "Part of image that's focused after switching to 1:1")
        }
        CheckBox {
            text: qsTranslate("SettingsDialog", "Grid background on images with transparency")
            checked: root.editor.view.transparencyGrid
            onToggled: root.editor.view.transparencyGrid = checked
        }
        RowLayout {
            spacing: StyleConstants.dialogSpacing

            CheckBox {
                text: qsTranslate("SettingsDialog", "Expand images, up to:")
                checked: root.editor.view.expandImage
                onToggled: root.editor.view.expandImage = checked
            }
            SettingsSliderRow {
                enabled: root.editor.view.expandImage
                range: root.editor.ranges.expandLimit
                value: root.editor.view.expandLimit
                valueText: root.editor.expandLimitText(value)
                onMoved: value => root.editor.view.expandLimit = value
            }
        }
        SettingsNote {
            text: qsTranslate("SettingsDialog", "Images smaller than window will be zoomed in")
        }
    }

    SettingsSection {
        title: qsTranslate("SettingsDialog", "Zoom options")

        CheckBox {
            text: qsTranslate("SettingsDialog", "Smooth zooming")
            checked: root.editor.view.smoothZoom
            onToggled: root.editor.view.smoothZoom = checked
        }
        SettingsRadioRow {
            label: qsTranslate("SettingsDialog", "Zoom indicator:")
            options: root.editor.zoomIndicatorModes
            value: root.editor.view.zoomIndicatorMode
            onPicked: value => root.editor.view.zoomIndicatorMode = value
        }
        CheckBox {
            text: qsTranslate("SettingsDialog", "Unlock minimum zoom")
            checked: root.editor.view.unlockMinZoom
            onToggled: root.editor.view.unlockMinZoom = checked
        }
        SettingsNote {
            text: qsTranslate("SettingsDialog", "Always allow zooming below 100%")
        }
        SettingsSliderRow {
            label: qsTranslate("SettingsDialog", "Zoom step:")
            range: root.editor.ranges.zoomStep
            value: root.editor.view.zoomStepPercent
            valueText: root.editor.zoomStepText(value)
            onMoved: value => root.editor.view.zoomStepPercent = value
        }
        CheckBox {
            text: qsTranslate("SettingsDialog", "Use fixed zoom levels:")
            checked: root.editor.view.useFixedZoomLevels
            onToggled: root.editor.view.useFixedZoomLevels = checked
        }
        RowLayout {
            spacing: StyleConstants.dialogSpacing
            enabled: root.editor.view.useFixedZoomLevels

            TextField {
                id: zoomLevelsField

                Layout.preferredWidth: root.zoomLevelsWidth
                text: root.editor.view.zoomLevels
                onTextEdited: root.editor.view.zoomLevels = zoomLevelsField.text
            }
            Button {
                text: qsTranslate("SettingsDialog", "Load defaults")
                onClicked: {
                    root.editor.resetZoomLevels();
                    zoomLevelsField.text = root.editor.view.zoomLevels;
                }
            }
        }
    }

    SettingsSection {
        title: qsTranslate("SettingsDialog", "Navigation")

        SettingsRadioRow {
            label: qsTranslate("SettingsDialog", "After reaching the end:")
            vertical: true
            options: root.editor.folderEndActions
            value: root.editor.view.folderEndAction
            onPicked: value => root.editor.view.folderEndAction = value
        }
    }

    SettingsSection {
        title: qsTranslate("SettingsDialog", "Overlays")

        CheckBox {
            text: qsTranslate("SettingsDialog", "Extended image info in window title")
            checked: root.editor.view.windowTitleExtendedInfo
            onToggled: root.editor.view.windowTitleExtendedInfo = checked
        }
        CheckBox {
            text: qsTranslate("SettingsDialog", "Fullscreen info bar")
            checked: root.editor.view.infoBarFullscreen
            onToggled: root.editor.view.infoBarFullscreen = checked
        }
        CheckBox {
            text: qsTranslate("SettingsDialog", "Animation playback controls")
            checked: root.editor.view.showAnimationControls
            onToggled: root.editor.view.showAnimationControls = checked
        }
        CheckBox {
            text: qsTranslate("SettingsDialog", "Auto-hide cursor")
            checked: root.editor.view.cursorAutohide
            onToggled: root.editor.view.cursorAutohide = checked
        }
    }

    SettingsSection {
        CheckBox {
            text: qsTranslate("SettingsDialog", "Thumbnail panel")
            font: Theme.fonts.bodyStrong
            checked: root.editor.view.panelEnabled
            onToggled: root.editor.view.panelEnabled = checked
        }

        ColumnLayout {
            Layout.fillWidth: true
            spacing: StyleConstants.dialogSpacing
            enabled: root.editor.view.panelEnabled

            GridLayout {
                columns: 2
                columnSpacing: StyleConstants.dialogSpacing

                CheckBox {
                    text: qsTranslate("SettingsDialog", "Pinned")
                    checked: root.editor.view.panelPinned
                    onToggled: root.editor.view.panelPinned = checked
                }
                CheckBox {
                    text: qsTranslate("SettingsDialog", "Disable in windowed mode")
                    checked: root.editor.view.panelFullscreenOnly
                    onToggled: root.editor.view.panelFullscreenOnly = checked
                }
                CheckBox {
                    text: qsTranslate("SettingsDialog", "Center selected image")
                    checked: root.editor.view.panelCenterSelection
                    onToggled: root.editor.view.panelCenterSelection = checked
                }
                CheckBox {
                    text: qsTranslate("SettingsDialog", "Show subfolders")
                    checked: root.editor.view.showSubfoldersInPanel
                    onToggled: root.editor.view.showSubfoldersInPanel = checked
                }
            }
            SettingsSliderRow {
                label: qsTranslate("SettingsDialog", "Hide delay:")
                labelWidth: root.sliderLabelWidth
                range: root.editor.ranges.panelHideDelay
                value: root.editor.view.panelHideDelayMs
                valueText: root.editor.panelHideDelayText(value)
                onMoved: value => root.editor.view.panelHideDelayMs = value
            }
            SettingsRadioRow {
                label: qsTranslate("SettingsDialog", "Display style:")
                vertical: true
                options: root.editor.thumbPanelStyles
                value: root.editor.view.thumbPanelStyle
                onPicked: value => root.editor.view.thumbPanelStyle = value
            }
            SettingsSliderRow {
                label: qsTranslate("SettingsDialog", "Preview size:")
                labelWidth: root.sliderLabelWidth
                range: root.editor.ranges.panelSize
                value: root.editor.view.panelSizeStep
                onMoved: value => root.editor.view.panelSizeStep = value
            }
            RowLayout {
                spacing: StyleConstants.dialogSpacing

                Label {
                    Layout.preferredWidth: root.sliderLabelWidth
                    text: qsTranslate("SettingsDialog", "Position:")
                }
                SettingsComboBox {
                    options: root.editor.panelPositions
                    value: root.editor.view.panelPosition
                    onPicked: value => root.editor.view.panelPosition = value
                }
            }
        }
    }

    SettingsSection {
        title: qsTranslate("SettingsDialog", "Scaling quality")

        RowLayout {
            spacing: StyleConstants.dialogSpacing

            Label {
                text: qsTranslate("SettingsDialog", "Scaling filter:")
            }
            SettingsComboBox {
                options: root.editor.scalingFilters
                value: root.editor.view.scalingFilter
                onPicked: value => root.editor.view.scalingFilter = value
            }
        }
        ColumnLayout {
            Layout.leftMargin: StyleConstants.dialogPadding
            visible: root.editor.casOptionsVisible
            spacing: StyleConstants.dialogSpacing

            SettingsSliderRow {
                label: qsTranslate("SettingsDialog", "Sharpness:")
                range: root.editor.ranges.casSharpening
                value: root.editor.view.casSharpeningPercent
                valueText: root.editor.casValueText(value)
                onMoved: value => root.editor.view.casSharpeningPercent = value
            }
            SettingsSliderRow {
                label: qsTranslate("SettingsDialog", "Contrast:")
                range: root.editor.ranges.casContrast
                value: root.editor.view.casContrastPercent
                valueText: root.editor.casValueText(value)
                onMoved: value => root.editor.view.casContrastPercent = value
            }
        }
    }

    SettingsSection {
        title: qsTranslate("SettingsDialog", "Color Management")

        CheckBox {
            text: qsTranslate("SettingsDialog", "Enable color management")
            checked: root.editor.view.colorManagementEnabled
            onToggled: root.editor.view.colorManagementEnabled = checked
        }
        RowLayout {
            spacing: StyleConstants.dialogSpacing
            enabled: root.editor.view.colorManagementEnabled

            Label {
                text: qsTranslate("SettingsDialog", "Monitor profile:")
            }
            SettingsComboBox {
                options: root.editor.monitorProfiles
                value: root.editor.view.monitorProfileType
                onPicked: value => root.editor.view.monitorProfileType = value
            }
        }
        RowLayout {
            Layout.fillWidth: true
            spacing: StyleConstants.dialogSpacing
            visible: root.editor.customProfileVisible

            Label {
                text: qsTranslate("SettingsDialog", "Profile file:")
            }
            TextField {
                Layout.fillWidth: true
                readOnly: true
                text: root.editor.view.monitorProfilePath
            }
            Button {
                text: qsTranslate("SettingsDialog", "Browse...")
                onClicked: profileDialog.open()
            }
        }
    }

    SettingsSection {
        title: qsTranslate("SettingsDialog", "HDR Tone-Mapping")

        CheckBox {
            text: qsTranslate("SettingsDialog", "Enable HDR Tone-Mapping")
            checked: root.editor.view.hdrToneMappingEnabled
            onToggled: root.editor.view.hdrToneMappingEnabled = checked
        }
        GridLayout {
            columns: 2
            columnSpacing: StyleConstants.dialogSpacing
            enabled: root.editor.view.hdrToneMappingEnabled

            Label {
                text: qsTranslate("SettingsDialog", "Tone-mapping operator:")
            }
            SettingsComboBox {
                options: root.editor.hdrOperators
                value: root.editor.view.hdrOperator
                onPicked: value => root.editor.view.hdrOperator = value
            }
            Label {
                text: qsTranslate("SettingsDialog", "Target white level:")
            }
            SettingsComboBox {
                options: root.editor.hdrTargetWhiteLevels
                value: root.editor.view.hdrTargetWhiteLevel
                onPicked: value => root.editor.view.hdrTargetWhiteLevel = value
            }
        }
    }

    FileDialog {
        id: profileDialog

        title: root.editor.colorProfileDialogTitle
        nameFilters: root.editor.colorProfileFilters
        fileMode: FileDialog.OpenFile
        onAccepted: root.editor.setMonitorProfileFile(profileDialog.selectedFile)
    }
}
