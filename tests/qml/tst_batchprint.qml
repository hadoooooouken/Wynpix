import QtQuick
import QtQuick.Controls
import QtTest
import wynpix.tests

// The batch converter and print windows of the dialog port (DialogLayer):
// created on their first request, driven through their view-models, closed
// with them. The batch converter converts through the fixture's fake
// service; printing is never started on a real printer.
TestCase {
    id: testCase

    name: "BatchPrint"
    when: windowShown

    // Positions of the dialogs in DialogLayer.
    readonly property int batchIndex: 4
    readonly property int printIndex: 5
    readonly property int activationTimeoutMs: 5000
    readonly property int colorSliderCount: 9
    // BatchJobRules::ColorSlider::Exposure: -3 .. 3 in steps of 0.01.
    readonly property int exposureSlider: 1

    // BatchQueueModel.ItemState values.
    readonly property int processingState: 1
    readonly property int doneState: 2

    property var dialogLayer: null

    function initTestCase() {
        const component = Qt.createComponent("wynpix.ui", "DialogLayer");
        compare(component.status, Component.Ready, component.errorString());
        dialogLayer = component.createObject(testCase, {coordinator: Fixture.dialogs});
        verify(dialogLayer !== null, "wynpix.ui/DialogLayer could not be instantiated");
    }

    function cleanupTestCase() {
        dialogLayer.destroy();
    }

    function init() {
        Fixture.clearDialogAnswer();
    }

    // A failed test must not leave a modal window over the next one.
    function cleanup() {
        Fixture.abandonDialogs();
    }

    function findNamed(item, objectName) {
        if (item.objectName === objectName)
            return item;
        const children = item.children ?? [];
        for (let i = 0; i < children.length; ++i) {
            const found = findNamed(children[i], objectName);
            if (found)
                return found;
        }
        return null;
    }

    function findAll(item, typeName, found) {
        if (item.toString().startsWith(typeName) && item.visible)
            found.push(item);
        for (let i = 0; i < item.children.length; ++i)
            findAll(item.children[i], typeName, found);
        return found;
    }

    // A window declared inside another window is in its `data`.
    function childWindow(window, objectName) {
        for (let i = 0; i < window.data.length; ++i) {
            if (window.data[i].objectName === objectName)
                return window.data[i];
        }
        return null;
    }

    function openedWindow(index) {
        const loader = dialogLayer.children[index];
        tryVerify(() => loader.item !== null && loader.item.visible);
        const window = loader.item;
        tryVerify(() => window.active, activationTimeoutMs, "the dialog window was not activated");
        tryVerify(() => window.activeFocusItem !== null, activationTimeoutMs, "nothing in the dialog has the focus");
        return window;
    }

    // Sources next to the test executable, so the initial output folder
    // exists.
    function sources() {
        return [Fixture.artifactPath("a.png"), Fixture.artifactPath("b.png"),
                Fixture.artifactPath("c.png")];
    }

    function test_batchWindowShowsTheQueue() {
        verify(Fixture.requestBatchConversion(sources()));
        const window = openedWindow(batchIndex);
        const settingsView = findNamed(window.contentItem, "batchSettingsView");
        const settingsColumn = findNamed(window.contentItem, "batchSettingsColumn");
        verify(settingsView !== null);
        verify(settingsColumn !== null);
        verify(window.width >= window.minimumWidth);
        verify(settingsView.width >= settingsColumn.implicitWidth + 2 * window.settingsMargin);
        const queue = findNamed(window.contentItem, "batchQueue");
        verify(queue !== null);
        compare(queue.count, 3);
        tryVerify(() => queue.itemAtIndex(1) !== null);
        compare(queue.itemAtIndex(1).name, "b.png");
        compare(queue.itemAtIndex(1).stateText, "Pending");
        compare(findNamed(window.contentItem, "batchStatus").text, "Ready to convert.");
        // The colour sliders are collapsed until the section is enabled.
        compare(findAll(window.contentItem, "LinkedSliderSpin", []).length, 0);
        Fixture.dialogs.batchConverter.setColorEnabled(true);
        tryCompare(findAll(window.contentItem, "LinkedSliderSpin", []), "length", colorSliderCount);
        Fixture.sendKey(window, Qt.Key_Escape);
        tryCompare(window, "visible", false);
        compare(Fixture.lastDialogAnswer, "batch:0");
    }

    function test_batchResizeModesEnableOnlyTheirControls() {
        verify(Fixture.requestBatchConversion(sources()));
        const window = openedWindow(batchIndex);
        const dialog = Fixture.dialogs.batchConverter;
        dialog.setResizeEnabled(true);

        const percentMode = findNamed(window.contentItem, "batchPercentMode");
        const percentControls = findNamed(window.contentItem, "batchPercentControls");
        const absoluteMode = findNamed(window.contentItem, "batchAbsoluteMode");
        const absoluteWidthControls = findNamed(window.contentItem, "batchAbsoluteWidthControls");
        const commonSizeAndHeight = findNamed(
            window.contentItem, "batchCommonSizeAndHeightControls");
        const megapixelMode = findNamed(window.contentItem, "batchMegapixelMode");
        const megapixels = findNamed(window.contentItem, "batchMegapixels");
        const binaryMegapixel = findNamed(window.contentItem, "batchBinaryMegapixel");
        const keepAspectRatio = findNamed(window.contentItem, "batchKeepAspectRatio");
        verify(percentMode !== null);
        verify(percentControls !== null);
        verify(absoluteMode !== null);
        verify(absoluteWidthControls !== null);
        verify(commonSizeAndHeight !== null);
        verify(megapixelMode !== null);
        verify(megapixels !== null);
        verify(binaryMegapixel !== null);
        verify(keepAspectRatio !== null);
        const fitModes = [
            findNamed(window.contentItem, "batchAspectFit_0"),
            findNamed(window.contentItem, "batchAspectFit_1"),
            findNamed(window.contentItem, "batchAspectFit_2")
        ];
        verify(fitModes[0] !== null);
        verify(fitModes[1] !== null);
        verify(fitModes[2] !== null);

        compare(absoluteMode.checked, true);
        compare(percentControls.enabled, false);
        compare(absoluteWidthControls.enabled, true);
        compare(commonSizeAndHeight.enabled, true);
        compare(megapixels.enabled, false);
        compare(binaryMegapixel.enabled, false);
        compare(keepAspectRatio.enabled, true);
        compare(fitModes[0].enabled, true);
        compare(fitModes[1].enabled, true);
        compare(fitModes[2].enabled, true);

        dialog.setByPercent(true);
        compare(percentMode.checked, true);
        compare(percentControls.enabled, true);
        compare(absoluteMode.checked, false);
        compare(absoluteWidthControls.enabled, false);
        compare(commonSizeAndHeight.enabled, false);
        compare(keepAspectRatio.enabled, false);
        compare(fitModes[0].enabled, false);
        compare(fitModes[1].enabled, false);
        compare(fitModes[2].enabled, false);

        dialog.setByMegapixels(true);
        compare(megapixelMode.checked, true);
        compare(percentMode.checked, false);
        compare(absoluteMode.checked, false);
        compare(percentControls.enabled, false);
        compare(absoluteWidthControls.enabled, false);
        compare(commonSizeAndHeight.enabled, false);
        compare(megapixels.enabled, true);
        compare(binaryMegapixel.enabled, true);
        compare(keepAspectRatio.enabled, false);
        compare(fitModes[0].enabled, false);
        compare(fitModes[1].enabled, false);
        compare(fitModes[2].enabled, false);

        dialog.setUseBinaryMegapixel(true);
        compare(binaryMegapixel.checked, true);
        dialog.setResizeEnabled(false);
        compare(megapixels.enabled, false);
        compare(binaryMegapixel.enabled, false);

        Fixture.sendKey(window, Qt.Key_Escape);
        tryCompare(window, "visible", false);
    }

    function test_batchColorSliderDoubleClickResets() {
        verify(Fixture.requestBatchConversion(sources()));
        const window = openedWindow(batchIndex);
        const dialog = Fixture.dialogs.batchConverter;
        dialog.setColorEnabled(true);

        const slider = findNamed(window.contentItem, "batchColorSlider_" + exposureSlider);
        const label = findNamed(window.contentItem, "batchColorLabel_" + exposureSlider);
        const firstValue = findNamed(window.contentItem, "batchColorValue_" + exposureSlider);
        const secondValue = findNamed(window.contentItem, "batchColorValue_" + (exposureSlider + 1));
        verify(slider !== null);
        verify(label !== null);
        verify(firstValue !== null);
        verify(secondValue !== null);

        const firstRight = firstValue.mapToItem(
            window.contentItem, firstValue.width, 0).x;
        const secondRight = secondValue.mapToItem(
            window.contentItem, secondValue.width, 0).x;
        verify(Math.abs(firstRight - secondRight) < 1,
               "color value texts do not share a right edge");

        dialog.setColorValue(exposureSlider, 1.5);
        compare(dialog.colorValues[exposureSlider], 1.5);
        mouseDoubleClickSequence(slider, slider.width / 2, slider.height / 2);
        tryVerify(() => dialog.colorValues[exposureSlider] === 0);

        dialog.setColorValue(exposureSlider, 1.5);
        mouseDoubleClickSequence(label, label.width / 2, label.height / 2);
        tryVerify(() => dialog.colorValues[exposureSlider] === 0);

        dialog.setColorValue(exposureSlider, 1.5);
        mouseDoubleClickSequence(firstValue, firstValue.width / 2, firstValue.height / 2);
        tryVerify(() => dialog.colorValues[exposureSlider] === 0);
    }

    function test_batchColorSliderKeepsDraggingOutsideItsBounds() {
        verify(Fixture.requestBatchConversion(sources()));
        const window = openedWindow(batchIndex);
        const dialog = Fixture.dialogs.batchConverter;
        dialog.setColorEnabled(true);
        const slider = findNamed(window.contentItem, "batchColorSlider_" + exposureSlider);
        verify(slider !== null);

        mousePress(slider, slider.width / 2, slider.height / 2);
        mouseMove(slider, slider.width + 100, slider.height / 2);
        tryVerify(() => dialog.colorValues[exposureSlider] > 1.5);
        mouseMove(slider, -100, slider.height / 2);
        tryVerify(() => dialog.colorValues[exposureSlider] < -1.5);
        mouseRelease(slider, -100, slider.height / 2);
    }

    function test_batchColorSliderWheelOverRowChangesItsValue() {
        verify(Fixture.requestBatchConversion(sources()));
        const window = openedWindow(batchIndex);
        const dialog = Fixture.dialogs.batchConverter;
        dialog.setColorEnabled(true);
        const label = findNamed(window.contentItem, "batchColorLabel_" + exposureSlider);
        const slider = findNamed(window.contentItem, "batchColorSlider_" + exposureSlider);
        const valueText = findNamed(window.contentItem, "batchColorValue_" + exposureSlider);
        verify(label !== null);
        verify(slider !== null);
        verify(valueText !== null);

        mouseWheel(label, label.width / 2, label.height / 2, 0, 120);
        tryVerify(() => dialog.colorValues[exposureSlider] === 0.01);

        mouseWheel(slider, slider.width / 2, slider.height / 2, 0, -120);
        tryVerify(() => dialog.colorValues[exposureSlider] === 0);

        mouseWheel(valueText, valueText.width / 2, valueText.height / 2, 0, -120);
        tryVerify(() => dialog.colorValues[exposureSlider] === -0.01);
    }

    function test_batchWarnsWithoutSelectedFiles() {
        const startsBefore = Fixture.batchStartCount();
        verify(Fixture.requestBatchConversion(sources()));
        const window = openedWindow(batchIndex);
        Fixture.dialogs.batchConverter.queue.setAllChecked(false);
        findNamed(window.contentItem, "convertButton").clicked();
        const message = childWindow(window, "batchMessage");
        verify(message !== null);
        tryCompare(message, "visible", true);
        compare(message.title, "No files");
        compare(Fixture.batchStartCount(), startsBefore);
        tryVerify(() => message.active, activationTimeoutMs);
        tryVerify(() => message.activeFocusItem !== null, activationTimeoutMs);
        Fixture.sendKey(message, Qt.Key_Return);
        tryCompare(message, "visible", false);
        verify(window.visible);
    }

    function test_batchRunsStopsAndFinishes() {
        const startsBefore = Fixture.batchStartCount();
        const cancelsBefore = Fixture.batchCancelCount();
        verify(Fixture.requestBatchConversion(sources()));
        const window = openedWindow(batchIndex);
        const convertButton = findNamed(window.contentItem, "convertButton");
        const cancelButton = findNamed(window.contentItem, "cancelButton");
        const status = findNamed(window.contentItem, "batchStatus");
        compare(cancelButton.text, "Cancel");

        convertButton.clicked();
        compare(Fixture.batchStartCount(), startsBefore + 1);
        compare(status.text, "Processing...");
        compare(cancelButton.text, "Stop");
        verify(!convertButton.enabled);

        Fixture.reportBatchProgress(0, processingState, "");
        Fixture.reportBatchProgress(0, doneState, "JPG • 40x20");
        const queue = findNamed(window.contentItem, "batchQueue");
        compare(queue.itemAtIndex(0).stateText, "Done");
        compare(queue.itemAtIndex(0).details, "JPG • 40x20");
        compare(status.text, "Processed 1 / 3 files.");

        cancelButton.clicked();
        compare(Fixture.batchCancelCount(), cancelsBefore + 1);
        compare(cancelButton.text, "Stopping...");
        verify(!cancelButton.enabled);
        Fixture.cancelBatch(1, 0, 3);
        compare(cancelButton.text, "Cancel");
        compare(status.text, "Stopped by user. Success: 1, Failed: 0");
        verify(convertButton.enabled);

        convertButton.clicked();
        Fixture.finishBatch(3, 0, 3);
        const message = childWindow(window, "batchMessage");
        tryCompare(message, "visible", true);
        compare(message.title, "Batch Conversion Complete");
        Fixture.dialogs.batchConverter.dismissMessage();
        tryCompare(message, "visible", false);

        cancelButton.clicked();
        tryCompare(window, "visible", false);
        compare(Fixture.lastDialogAnswer, "batch:1");
    }

    function test_printWithoutPrintersOffersOnlyTheExport() {
        verify(Fixture.requestPrint([]));
        const window = openedWindow(printIndex);
        compare(window.title, "Print image");
        verify(!findNamed(window.contentItem, "printButton").enabled);
        compare(window.activeFocusItem, findNamed(window.contentItem, "exportPdfButton"));
        const preview = findNamed(window.contentItem, "printPreview");
        tryVerify(() => preview.paintedRect.height > 0);
        // A4 portrait fills the preview's height.
        compare(preview.paintedRect.height, Fixture.dialogs.print.previewExtent);
        verify(preview.paintedRect.width < preview.paintedRect.height);
        Fixture.dialogs.print.setLandscape(true);
        tryVerify(() => preview.paintedRect.width > preview.paintedRect.height);
        Fixture.sendKey(window, Qt.Key_Escape);
        tryCompare(window, "visible", false);
        compare(Fixture.lastDialogAnswer, "print:0");
    }

    function test_printWithPrintersOffersPrinting() {
        verify(Fixture.requestPrint(["First", "Second"]));
        const window = openedWindow(printIndex);
        const printButton = findNamed(window.contentItem, "printButton");
        verify(printButton.enabled);
        compare(window.activeFocusItem, printButton);
        compare(Fixture.dialogs.print.printerIndex, 0);
        window.close();
        tryCompare(window, "visible", false);
        compare(Fixture.lastDialogAnswer, "print:0");
    }
}
