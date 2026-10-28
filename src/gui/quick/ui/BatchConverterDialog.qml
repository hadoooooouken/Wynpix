pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Dialogs as Dialogs
import QtQuick.Layouts
import QtQuick.Templates as T
import wynpix.bridges
import wynpix.style

// The batch converter (BatchConverterDialog in the widget UI): an
// application-modal, resizable window over the main window. Left: the file
// queue with select / deselect all and the selection size; right: output
// format and quality, resize, rotation and flips, colour adjustments, output
// folder and file name pattern; below: the progress, the status and Convert
// / Cancel (Stop while a batch runs). Every decision is the view-model's.
// Enter converts unless a button has the focus; Escape, Cancel and the close
// button close the window, stopping a running batch. Closing keeps the queue
// for the next time the window opens (a batch that ran ends it); Add more
// closes the window the same way, and only Clear empties the queue.
Window {
    id: root

    required property BatchConverterDialogModel dialog
    readonly property bool editable: !root.dialog.running && !root.dialog.cancelling

    readonly property int initialWidth: 1048
    readonly property int initialHeight: 816
    readonly property int minimumContentWidth: Math.max(
        920,
        root.queueMinimumWidth + root.settingsPreferredWidth + StyleConstants.dialogSpacing
        + 2 * StyleConstants.dialogPadding)
    readonly property int minimumContentHeight: 600
    readonly property int queueMinimumWidth: 400
    readonly property int settingsWidth: 500
    readonly property int settingsMargin: 14
    readonly property int settingsPreferredWidth: Math.max(
        root.settingsWidth,
        settingsColumn.implicitWidth + 2 * root.settingsMargin + StyleConstants.scrollBarThickness)
    readonly property int sectionSpacing: 12
    readonly property int indent: 20
    readonly property int fieldWidth: 120
    readonly property int queueFrameWidth: 1
    readonly property int qualityStep: 1

    title: qsTr("Batch Converter")
    flags: Qt.Dialog | Qt.CustomizeWindowHint | Qt.WindowTitleHint | Qt.WindowCloseButtonHint
    modality: Qt.ApplicationModal
    color: Theme.surfaces.window
    width: Math.max(root.initialWidth, root.minimumContentWidth)
    height: root.initialHeight
    minimumWidth: root.minimumContentWidth
    minimumHeight: root.minimumContentHeight
    visible: root.dialog.open

    onMinimumContentWidthChanged: {
        if (root.visible && root.width < root.minimumContentWidth)
            root.fitSettingsWidth();
    }

    onVisibleChanged: {
        if (!root.visible)
            return;
        root.fitSettingsWidth();
        root.fitSettingsHeight();
        const owner = root.transientParent;
        if (owner) {
            root.x = owner.x + (owner.width - root.width) / 2;
            root.y = owner.y + (owner.height - root.height) / 2;
        }
        keyScope.forceActiveFocus();
        root.requestActivate();
    }

    // Opens wide enough to fit the queue and the settings, within the screen's
    // available width.
    function fitSettingsWidth() {
        const neededWidth = Math.max(root.initialWidth, root.minimumContentWidth);
        const availableWidth = root.screen ? root.screen.desktopAvailableWidth : neededWidth;
        root.width = Math.max(root.width, Math.min(neededWidth, availableWidth));
        if (root.screen && root.x + root.width > root.screen.desktopAvailableWidth)
            root.x = Math.max(0, root.screen.desktopAvailableWidth - root.width);
    }

    // Opens tall enough to show every setting without the scroll bar, within
    // the screen's available height.
    function fitSettingsHeight() {
        const chromeHeight = 2 * StyleConstants.dialogPadding + 2 * StyleConstants.dialogSpacing
                             + progressBar.implicitHeight + buttonRow.implicitHeight
                             + settingsView.topPadding + settingsView.bottomPadding;
        const neededHeight = settingsColumn.implicitHeight + chromeHeight;
        const availableHeight = root.screen ? root.screen.desktopAvailableHeight : neededHeight;
        root.height = Math.max(root.height, Math.min(neededHeight, availableHeight));
    }

    onActiveChanged: {
        if (root.active && root.activeFocusItem === null)
            keyScope.forceActiveFocus();
    }

    onClosing: close => {
        close.accepted = false;
        root.dialog.reject();
    }

    FocusScope {
        id: keyScope

        anchors.fill: parent
        focus: true

        Keys.onPressed: event => {
            if (event.key === Qt.Key_Escape) {
                event.accepted = true;
                root.dialog.reject();
            } else if (event.key === Qt.Key_Return || event.key === Qt.Key_Enter) {
                event.accepted = true;
                const focusedButton = root.activeFocusItem as T.Button;
                if (focusedButton)
                    focusedButton.click();
                else
                    root.dialog.convert();
            }
        }

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: StyleConstants.dialogPadding
            spacing: StyleConstants.dialogSpacing

            RowLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                spacing: StyleConstants.dialogSpacing

                // The queue.
                ColumnLayout {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    Layout.minimumWidth: root.queueMinimumWidth
                    spacing: StyleConstants.dialogSpacing

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: StyleConstants.dialogButtonSpacing

                        Button {
                            objectName: "addMoreButton"
                            text: qsTr("Add more")
                            enabled: root.editable
                            onClicked: root.dialog.addMore()
                        }
                        Button {
                            objectName: "clearButton"
                            text: qsTr("Clear")
                            enabled: root.editable && root.dialog.queue.count > 0
                            onClicked: root.dialog.clearList()
                        }

                        Item {
                            Layout.fillWidth: true
                        }

                        Button {
                            text: qsTr("Select all")
                            enabled: root.editable
                            onClicked: root.dialog.queue.setAllChecked(true)
                        }
                        Button {
                            text: qsTr("Deselect all")
                            enabled: root.editable
                            onClicked: root.dialog.queue.setAllChecked(false)
                        }
                    }

                    Label {
                        Layout.alignment: Qt.AlignRight
                        text: root.dialog.queue.selectionText
                        font: Theme.fonts.bodyStrong
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        color: Theme.colors.folderView
                        border.width: root.queueFrameWidth
                        border.color: Theme.colors.strokeDefault

                        ListView {
                            objectName: "batchQueue"
                            anchors.fill: parent
                            anchors.margins: root.queueFrameWidth
                            clip: true
                            model: root.dialog.queue
                            boundsBehavior: Flickable.StopAtBounds
                            ScrollBar.vertical: ScrollBar {
                                id: queueScrollBar
                            }

                            // The scroll bar overlays the view: keep the rows
                            // (and their right-aligned state) clear of it.
                            delegate: BatchItemWidget {
                                width: ListView.view.width
                                       - (queueScrollBar.size < 1.0 ? queueScrollBar.width : 0)
                                queue: root.dialog.queue
                                thumbnailSize: root.dialog.thumbnailSize
                            }
                        }
                    }
                }

                // The settings.
                ScrollView {
                    id: settingsView

                    objectName: "batchSettingsView"
                    Layout.preferredWidth: root.settingsPreferredWidth
                    Layout.fillHeight: true
                    enabled: root.editable
                    contentWidth: availableWidth

                    ColumnLayout {
                        id: settingsColumn

                        objectName: "batchSettingsColumn"
                        x: root.settingsMargin
                        width: settingsView.availableWidth - 2 * root.settingsMargin
                        spacing: StyleConstants.dialogSpacing

                        // Format and quality.
                        RowLayout {
                            spacing: StyleConstants.dialogSpacing

                            Label {
                                text: qsTr("Save as type:")
                            }
                            ComboBox {
                                Layout.fillWidth: true
                                model: root.dialog.formats
                                currentIndex: root.dialog.formatIndex
                                onActivated: index => root.dialog.setFormatIndex(index)
                            }
                        }

                        RowLayout {
                            spacing: StyleConstants.dialogSpacing
                            enabled: root.dialog.qualityAvailable

                            Slider {
                                id: qualitySlider

                                Layout.fillWidth: true
                                from: root.dialog.qualityMinimum
                                to: root.dialog.qualityMaximum
                                stepSize: root.qualityStep
                                snapMode: Slider.SnapAlways
                                value: root.dialog.quality
                                onMoved: root.dialog.setQuality(Math.round(qualitySlider.value))

                                ToolTip {
                                    text: root.dialog.qualityToolTip
                                    visible: qualitySlider.hovered && text.length > 0
                                }
                            }
                            SpinBox {
                                id: qualityBox

                                Layout.preferredWidth: root.fieldWidth
                                from: root.dialog.qualityMinimum
                                to: root.dialog.qualityMaximum
                                value: root.dialog.quality
                                editable: true
                                onValueModified: root.dialog.setQuality(qualityBox.value)
                            }
                        }

                        // Resize.
                        CheckBox {
                            Layout.topMargin: root.sectionSpacing
                            text: qsTr("Resize")
                            checked: root.dialog.resizeEnabled
                            onToggled: root.dialog.setResizeEnabled(checked)
                        }

                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: StyleConstants.dialogSpacing
                            enabled: root.dialog.resizeEnabled

                            RowLayout {
                                Layout.fillWidth: true
                                spacing: StyleConstants.dialogSpacing

                                RadioButton {
                                    objectName: "batchPercentMode"
                                    text: qsTr("By Percent:")
                                    checkable: false
                                    checked: root.dialog.byPercent
                                    onClicked: root.dialog.setByPercent(true)
                                }

                                RowLayout {
                                    objectName: "batchPercentControls"
                                    Layout.fillWidth: true
                                    Layout.leftMargin: root.indent
                                    spacing: StyleConstants.dialogSpacing
                                    enabled: root.dialog.byPercent

                                    Label {
                                        Layout.fillWidth: true
                                        horizontalAlignment: Text.AlignRight
                                        text: qsTr("Percent:")
                                    }
                                    DoubleSpinBox {
                                        id: percentBox

                                        Layout.preferredWidth: root.fieldWidth
                                        from: root.dialog.minimumPercent
                                        to: root.dialog.maximumPercent
                                        decimals: root.dialog.percentDecimals
                                        value: root.dialog.percent
                                        editable: true
                                        onValueModified: root.dialog.setPercent(percentBox.value)
                                    }
                                }
                            }

                            ColumnLayout {
                                Layout.fillWidth: true
                                spacing: StyleConstants.dialogSpacing

                                RowLayout {
                                    Layout.fillWidth: true
                                    spacing: StyleConstants.dialogSpacing

                                    RadioButton {
                                        objectName: "batchAbsoluteMode"
                                        text: qsTr("By Absolute Size:")
                                        checkable: false
                                        checked: root.dialog.byAbsoluteSize
                                        onClicked: root.dialog.setByPercent(false)
                                    }

                                    RowLayout {
                                        objectName: "batchAbsoluteWidthControls"
                                        Layout.fillWidth: true
                                        Layout.leftMargin: root.indent
                                        spacing: StyleConstants.dialogSpacing
                                        enabled: root.dialog.byAbsoluteSize

                                        Label {
                                            Layout.fillWidth: true
                                            horizontalAlignment: Text.AlignRight
                                            text: qsTr("Max Width:")
                                        }
                                        SpinBox {
                                            id: widthBox

                                            objectName: "batchWidth"
                                            Layout.preferredWidth: root.fieldWidth
                                            from: root.dialog.minimumSide
                                            to: root.dialog.maximumSide
                                            value: root.dialog.targetWidth
                                            editable: true
                                            onValueModified: root.dialog.setTargetWidth(widthBox.value)
                                        }
                                    }
                                }

                                RowLayout {
                                    objectName: "batchCommonSizeAndHeightControls"
                                    Layout.fillWidth: true
                                    Layout.leftMargin: StyleConstants.indicatorPadding
                                                        + StyleConstants.indicatorSize
                                                        + StyleConstants.indicatorSpacing
                                    spacing: StyleConstants.dialogSpacing
                                    enabled: root.dialog.byAbsoluteSize

                                    Label {
                                        text: qsTr("Common sizes:")
                                    }
                                    ComboBox {
                                        Layout.preferredWidth: root.fieldWidth
                                        model: root.dialog.commonSizes
                                        currentIndex: root.dialog.commonSizeIndex
                                        onActivated: index => root.dialog.selectCommonSize(index)
                                    }

                                    Label {
                                        Layout.fillWidth: true
                                        horizontalAlignment: Text.AlignRight
                                        text: qsTr("Max Height:")
                                    }
                                    SpinBox {
                                        id: heightBox

                                        objectName: "batchHeight"
                                        Layout.preferredWidth: root.fieldWidth
                                        from: root.dialog.minimumSide
                                        to: root.dialog.maximumSide
                                        value: root.dialog.targetHeight
                                        editable: true
                                        onValueModified: root.dialog.setTargetHeight(heightBox.value)
                                    }
                                }

                                RowLayout {
                                    Layout.fillWidth: true
                                    spacing: StyleConstants.dialogSpacing

                                    RadioButton {
                                        objectName: "batchMegapixelMode"
                                        text: qsTr("By Megapixels:")
                                        checkable: false
                                        checked: root.dialog.byMegapixels
                                        onClicked: root.dialog.setByMegapixels(true)
                                    }

                                    RowLayout {
                                        Layout.fillWidth: true
                                        Layout.leftMargin: root.indent
                                        spacing: StyleConstants.dialogSpacing
                                        enabled: root.dialog.byMegapixels

                                        Label {
                                            Layout.fillWidth: true
                                            horizontalAlignment: Text.AlignRight
                                            text: qsTr("Megapixels:")
                                        }
                                        DoubleSpinBox {
                                            id: megapixelBox

                                            objectName: "batchMegapixels"
                                            Layout.preferredWidth: root.fieldWidth
                                            from: root.dialog.minimumMegapixels
                                            to: root.dialog.maximumMegapixels
                                            decimals: root.dialog.megapixelDecimals
                                            value: root.dialog.targetMegapixels
                                            editable: true
                                            onValueModified: root.dialog.setTargetMegapixels(megapixelBox.value)
                                        }
                                    }
                                }

                                CheckBox {
                                    objectName: "batchBinaryMegapixel"
                                    Layout.leftMargin: StyleConstants.indicatorPadding
                                                        + StyleConstants.indicatorSize
                                                        + StyleConstants.indicatorSpacing
                                    text: qsTr("Binary (1M = 1024x1024)")
                                    checked: root.dialog.useBinaryMegapixel
                                    enabled: root.dialog.byMegapixels
                                    onToggled: root.dialog.setUseBinaryMegapixel(checked)
                                }
                            }

                            Button {
                                Layout.fillWidth: true
                                text: qsTr("Reset: %1 x %2").arg(root.dialog.originalWidth)
                                                             .arg(root.dialog.originalHeight)
                                onClicked: root.dialog.resetSize()
                            }

                            RowLayout {
                                spacing: StyleConstants.dialogSpacing

                                CheckBox {
                                    objectName: "batchKeepAspectRatio"
                                    text: qsTr("Keep aspect ratio")
                                    enabled: root.dialog.byAbsoluteSize
                                    checked: root.dialog.keepAspectRatio
                                    onToggled: root.dialog.setKeepAspectRatio(checked)
                                }

                                Repeater {
                                    // In AspectFitMode order.
                                    model: [qsTr("Auto"), qsTr("Width"), qsTr("Height")]

                                    delegate: RadioButton {
                                        required property int index
                                        required property string modelData

                                        objectName: "batchAspectFit_" + index
                                        text: modelData
                                        enabled: root.dialog.keepAspectRatio && root.dialog.byAbsoluteSize
                                        checkable: false
                                        checked: root.dialog.aspectFitMode === index
                                        onClicked: root.dialog.setAspectFitMode(index)
                                    }
                                }

                                Item {
                                    Layout.fillWidth: true
                                }

                                // A disabled check box gets no hover, so the
                                // wrapper shows why Upscayl is unavailable.
                                Item {
                                    implicitWidth: upscaylBox.implicitWidth
                                    implicitHeight: upscaylBox.implicitHeight

                                    CheckBox {
                                        id: upscaylBox

                                        text: qsTr("Upscayl")
                                        enabled: root.dialog.upscaylAvailable
                                        checked: root.dialog.useUpscayl
                                        onToggled: root.dialog.setUseUpscayl(checked)
                                    }

                                    HoverHandler {
                                        id: upscaylHover
                                    }

                                    ToolTip {
                                        text: root.dialog.upscaylAvailable
                                              ? "" : qsTr("No AI models found in models/ directory.")
                                        visible: upscaylHover.hovered && text.length > 0
                                    }
                                }
                            }

                            RowLayout {
                                spacing: StyleConstants.dialogSpacing

                                ColumnLayout {
                                    Layout.fillWidth: true

                                    Label {
                                        text: qsTr("Filter:")
                                    }
                                    ComboBox {
                                        Layout.fillWidth: true
                                        model: root.dialog.filters
                                        currentIndex: root.dialog.filterIndex
                                        onActivated: index => root.dialog.setFilterIndex(index)
                                    }
                                }

                                ColumnLayout {
                                    Layout.fillWidth: true
                                    enabled: root.dialog.upscaylAvailable && root.dialog.useUpscayl

                                    Label {
                                        text: qsTr("Model:")
                                    }
                                    ComboBox {
                                        Layout.fillWidth: true
                                        model: root.dialog.upscaylModels
                                        currentIndex: root.dialog.upscaylModelIndex
                                        onActivated: index => root.dialog.setUpscaylModelIndex(index)
                                    }
                                }
                            }
                        }

                        // Rotation and flips.
                        RowLayout {
                            Layout.topMargin: root.sectionSpacing
                            spacing: StyleConstants.dialogSpacing

                            Repeater {
                                model: root.dialog.rotations

                                delegate: RadioButton {
                                    required property int index
                                    required property string modelData

                                    Layout.fillWidth: true
                                    text: modelData
                                    checkable: false
                                    checked: root.dialog.rotationIndex === index
                                    onClicked: root.dialog.setRotationIndex(index)
                                }
                            }
                        }

                        RowLayout {
                            spacing: StyleConstants.dialogSpacing

                            CheckBox {
                                Layout.fillWidth: true
                                text: qsTr("Flip horizontal")
                                checked: root.dialog.flipHorizontal
                                onToggled: root.dialog.setFlipHorizontal(checked)
                            }
                            CheckBox {
                                Layout.fillWidth: true
                                text: qsTr("Flip vertical")
                                checked: root.dialog.flipVertical
                                onToggled: root.dialog.setFlipVertical(checked)
                            }
                        }

                        // Colour adjustments, collapsed while disabled.
                        RowLayout {
                            Layout.fillWidth: true
                            Layout.topMargin: root.sectionSpacing

                            CheckBox {
                                text: qsTr("Color adjustments")
                                checked: root.dialog.colorEnabled
                                onToggled: root.dialog.setColorEnabled(checked)
                            }

                            Item {
                                Layout.fillWidth: true
                            }

                            Button {
                                text: qsTr("Paste")
                                visible: root.dialog.canPasteColorAdjustments
                                onClicked: root.dialog.pasteColorAdjustments()
                            }
                        }

                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: StyleConstants.dialogSpacing
                            visible: root.dialog.colorEnabled

                            Repeater {
                                model: root.dialog.colorSliders

                                delegate: LinkedSliderSpin {
                                    required property int index
                                    required property var modelData

                                    rowIndex: index
                                    Layout.fillWidth: true
                                    spec: modelData
                                    trackSpec: colorTracks.tracks[index]
                                    value: root.dialog.colorValues[index]
                                    onEdited: value => root.dialog.setColorValue(index, value)
                                }
                            }

                            Button {
                                Layout.fillWidth: true
                                text: qsTr("Reset Color Adjustments")
                                onClicked: root.dialog.resetColorValues()
                            }
                        }

                        // Output.
                        Label {
                            Layout.topMargin: root.sectionSpacing
                            text: qsTr("Output folder:")
                        }

                        RowLayout {
                            spacing: StyleConstants.dialogSpacing

                            TextField {
                                Layout.fillWidth: true
                                text: root.dialog.outputDirectory
                                onTextEdited: root.dialog.setOutputDirectory(text)
                            }
                            Button {
                                text: qsTr("...")
                                onClicked: folderDialog.open()
                            }
                        }

                        CheckBox {
                            text: qsTr("Create subfolder for batch")
                            checked: root.dialog.createSubfolder
                            onToggled: root.dialog.setCreateSubfolder(checked)
                        }

                        Label {
                            text: qsTr("Filename pattern:")
                        }

                        TextField {
                            Layout.fillWidth: true
                            text: root.dialog.pattern
                            onTextEdited: root.dialog.setPattern(text)
                        }

                        Label {
                            text: root.dialog.patternHelp
                            font: Theme.fonts.caption
                        }

                        CheckBox {
                            text: qsTr("Overwrite existing files")
                            checked: root.dialog.overwrite
                            onToggled: root.dialog.setOverwrite(checked)
                        }
                    }
                }
            }

            ProgressBar {
                id: progressBar

                Layout.fillWidth: true
                from: 0
                to: Math.max(root.dialog.progressMaximum, 1)
                value: root.dialog.progressValue
            }

            RowLayout {
                id: buttonRow

                Layout.fillWidth: true
                spacing: StyleConstants.dialogButtonSpacing

                Label {
                    objectName: "batchStatus"
                    Layout.fillWidth: true
                    text: root.dialog.statusText
                    elide: Text.ElideRight
                }
                Button {
                    objectName: "convertButton"
                    text: qsTr("Convert")
                    enabled: root.editable
                    onClicked: root.dialog.convert()
                }
                Button {
                    objectName: "cancelButton"
                    text: root.dialog.cancelling ? qsTr("Stopping...")
                                                 : root.dialog.running ? qsTr("Stop") : qsTr("Cancel")
                    enabled: !root.dialog.cancelling
                    onClicked: root.dialog.stopOrClose()
                }
            }
        }
    }

    Dialogs.FolderDialog {
        id: folderDialog

        title: qsTr("Select Output Directory")
        parentWindow: root
        currentFolder: root.dialog.outputDirectoryUrl
        onAccepted: root.dialog.setOutputDirectoryUrl(folderDialog.selectedFolder)
    }

    ColorAdjustmentTracks {
        id: colorTracks
    }

    // Warnings and the completion message (the QMessageBoxes of the widget
    // dialog), modal over the batch window.
    DialogWindow {
        id: messageWindow

        objectName: "batchMessage"
        readonly property int iconSize: 32
        readonly property int maximumTextWidth: 480

        transientParent: root
        title: root.dialog.messageTitle
        visible: root.dialog.open && root.dialog.messageOpen

        onAcceptRequested: root.dialog.dismissMessage()
        onDismissRequested: root.dialog.dismissMessage()

        RowLayout {
            Layout.fillWidth: true
            spacing: StyleConstants.popupPadding

            IconGlyph {
                Layout.alignment: Qt.AlignTop
                icon: root.dialog.messageIsWarning ? FluentIcons.Warning20 : FluentIcons.Info20
                size: messageWindow.iconSize
            }

            Label {
                Layout.fillWidth: true
                Layout.maximumWidth: messageWindow.maximumTextWidth
                text: root.dialog.messageText
                wrapMode: Text.Wrap
                textFormat: Text.PlainText
            }
        }

        DialogButtonBox {
            Layout.fillWidth: true
            onAccepted: root.dialog.dismissMessage()

            // Own button, not standardButtons: no Qt base translation is
            // loaded.
            Button {
                text: qsTr("OK")
                DialogButtonBox.buttonRole: DialogButtonBox.AcceptRole
            }
        }
    }
}
