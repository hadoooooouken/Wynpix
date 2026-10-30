import QtQuick
import QtTest
import wynpix.tests

// The settings window (SettingsDialog) over an editor with in-memory
// stores: lazily created pages, OK / Apply / Cancel, the previewed theme
// values, the shortcut creator and the script editor.
TestCase {
    id: testCase

    name: "Settings"
    when: windowShown

    // SettingsEditorModel.Page values.
    readonly property int generalPage: 0
    readonly property int themePage: 2
    readonly property int controlsPage: 3
    readonly property int scriptsPage: 4
    readonly property int advancedPage: 5
    readonly property int pageCount: 8
    // ThemeMode value of the light theme.
    readonly property int lightTheme: 2
    readonly property int activationTimeoutMs: 5000
    // How long the shortcut list may take to lay its rows out.
    readonly property int layoutTimeoutMs: 1000
    // How long a view takes to scroll by a wheel turn.
    readonly property int wheelScrollMs: 300

    property var settingsWindow: null

    function initTestCase() {
        const component = Qt.createComponent("wynpix.ui", "SettingsDialog");
        compare(component.status, Component.Ready, component.errorString());
        settingsWindow = component.createObject(testCase, {controller: Fixture.settingsDialog});
        verify(settingsWindow !== null, "wynpix.ui/SettingsDialog could not be instantiated");
    }

    function cleanupTestCase() {
        settingsWindow.destroy();
    }

    function init() {
        Fixture.resetSettingsStores();
    }

    // A failed test must not leave a modal window over the next one.
    function cleanup() {
        Fixture.closeSettings();
    }

    function findAll(item, typeName, found) {
        if (item.toString().startsWith(typeName))
            found.push(item);
        for (let i = 0; i < item.children.length; ++i)
            findAll(item.children[i], typeName, found);
        return found;
    }

    function findText(item, typeName, text) {
        const found = findAll(item, typeName, []).filter(candidate => candidate.text === text);
        return found.length > 0 ? found[0] : null;
    }

    function pageLoaders() {
        return findAll(settingsWindow.contentItem, "QQuickLoader", [])
            .filter(loader => loader.index !== undefined);
    }

    function currentPage() {
        const loaders = pageLoaders();
        for (let i = 0; i < loaders.length; ++i) {
            if (loaders[i].visible && loaders[i].item)
                return loaders[i].item;
        }
        return null;
    }

    function openOn(page) {
        Fixture.openSettings(page);
        tryVerify(() => settingsWindow.visible);
        tryVerify(() => settingsWindow.active, activationTimeoutMs, "the settings window was not activated");
        tryVerify(() => currentPage() !== null);
        return currentPage();
    }

    function button(item, text) {
        const found = findText(item, "Button", text);
        verify(found !== null, "no button " + text);
        return found;
    }

    // The row of action on the Controls page.
    function rowOf(page, action) {
        // By the view, once it has laid the rows out: rows of a reset model
        // may wait for deletion.
        const list = findAll(page, "QQuickListView", [])[0];
        let found = null;
        tryVerify(() => {
            for (let i = 0; i < list.count && !found; ++i) {
                const row = list.itemAtIndex(i);
                if (row && row.action === action)
                    found = row;
            }
            return found !== null;
        }, layoutTimeoutMs, "no row of " + action);
        return found;
    }

    // The "+" button of the row of action on the Controls page.
    function addButtonOf(page, action) {
        return findAll(rowOf(page, action), "ToolButton", [])[0];
    }

    // The chip of shortcut in the row of action on the Controls page.
    function chipOf(page, action, shortcut) {
        const chips = findAll(rowOf(page, action), "", [])
            .filter(chip => chip.objectName === "shortcutChip" && chip.shortcut === shortcut);
        verify(chips.length === 1, "no chip " + shortcut + " of " + action);
        return chips[0];
    }

    // The number of rows the Controls page shows.
    function shortcutRowCount(page) {
        return findAll(page, "QQuickListView", [])[0].count;
    }

    // The sub-dialog window of a Loader in the settings window, once open
    // and active.
    function subDialog(typeName) {
        let window = null;
        tryVerify(() => {
            const loaders = findAll(settingsWindow.contentItem, "QQuickLoader", [])
                .filter(loader => loader.item && loader.item.toString().startsWith(typeName));
            window = loaders.length > 0 ? loaders[0].item : null;
            return window !== null && window.visible;
        });
        tryVerify(() => window.active, activationTimeoutMs,
                  typeName + " was not activated (settings window active: " + settingsWindow.active + ")");
        tryVerify(() => window.activeFocusItem !== null, activationTimeoutMs,
                  "nothing in " + typeName + " has the focus");
        return window;
    }

    function test_onlyTheShownPageIsCreated() {
        openOn(advancedPage);
        const created = pageLoaders().filter(loader => loader.item !== null);
        compare(created.length, 1);
        compare(created[0].index, advancedPage);
        compare(pageLoaders().length, pageCount);
    }

    function test_okAppliesTheEditedValuesAndCloses() {
        const page = openOn(generalPage);
        const fullscreen = findText(page, "CheckBox", "Open in fullscreen");
        verify(fullscreen !== null);
        verify(!fullscreen.checked);
        mouseClick(fullscreen);
        compare(Fixture.settingsApplyCount, 0);
        mouseClick(button(settingsWindow.contentItem, "OK"));
        tryVerify(() => !settingsWindow.visible);
        compare(Fixture.settingsApplyCount, 1);
        compare(Fixture.storedSetting("general", "fullscreenMode"), true);
    }

    function test_applyKeepsTheWindowOpen() {
        openOn(generalPage);
        mouseClick(button(settingsWindow.contentItem, "Apply"));
        compare(Fixture.settingsApplyCount, 1);
        verify(settingsWindow.visible);
    }

    function test_cancelAndEscapeCloseWithoutApplying() {
        let page = openOn(generalPage);
        mouseClick(findText(page, "CheckBox", "Open in fullscreen"));
        mouseClick(button(settingsWindow.contentItem, "Cancel"));
        tryVerify(() => !settingsWindow.visible);

        // Reopened, the page shows the stored values again.
        page = openOn(generalPage);
        verify(!findText(page, "CheckBox", "Open in fullscreen").checked);
        Fixture.sendKey(settingsWindow, Qt.Key_Escape);
        tryVerify(() => !settingsWindow.visible);
        compare(Fixture.settingsApplyCount, 0);
    }

    function test_theThemeModeIsPreviewedAtOnce() {
        const page = openOn(themePage);
        const combos = findAll(page, "SettingsComboBox", []);
        verify(combos.length > 0);
        combos[0].activated(lightTheme);
        compare(Fixture.previewedThemeMode, lightTheme);
        compare(Fixture.settingsApplyCount, 0);
    }

    function test_aCapturedKeyBecomesANewShortcut() {
        const page = openOn(controlsPage);
        addButtonOf(page, "nextImage").clicked();
        const creator = subDialog("ShortcutCreatorDialog");
        const ok = findText(creator.contentItem, "Button", "OK");
        verify(!ok.enabled);
        Fixture.sendKey(creator, Qt.Key_K);
        tryCompare(Fixture.settingsDialog.editor.shortcutEditor, "shortcut", "K");
        verify(ok.enabled);
        mouseClick(ok);
        tryVerify(() => !creator.visible);
        mouseClick(button(settingsWindow.contentItem, "Apply"));
        // A new shortcut goes after the other shortcuts of its action.
        compare(Fixture.appliedShortcuts, "nextImage=Right;nextImage=K;prevImage=Left");
    }

    function test_aMouseButtonCanBeCaptured() {
        const page = openOn(controlsPage);
        addButtonOf(page, "nextImage").clicked();
        const creator = subDialog("ShortcutCreatorDialog");
        const field = findAll(creator.contentItem, "ShortcutCaptureField", [])[0];
        verify(field !== undefined);
        mouseClick(field);
        compare(Fixture.settingsDialog.editor.shortcutEditor.shortcut, "LMB");
        mouseDoubleClickSequence(field, field.width / 2, field.height / 2, Qt.LeftButton);
        compare(Fixture.settingsDialog.editor.shortcutEditor.shortcut, "LMB_DoubleClick");
        mouseWheel(field, field.width / 2, field.height / 2, 0, 120);
        compare(Fixture.settingsDialog.editor.shortcutEditor.shortcut, "WheelUp");
        mouseClick(findText(creator.contentItem, "Button", "Cancel"));
        tryVerify(() => !creator.visible);
    }

    function test_aClickOnARowAddsAShortcutToItsAction() {
        const page = openOn(controlsPage);
        const row = rowOf(page, "prevImage");
        mouseClick(row, row.width / 4, row.height / 2);
        const creator = subDialog("ShortcutCreatorDialog");
        compare(Fixture.settingsDialog.editor.shortcutEditor.title, "Add shortcut");
        compare(Fixture.settingsDialog.editor.shortcutEditor.actionTitle, "prevImage");
        mouseClick(findText(creator.contentItem, "Button", "Cancel"));
        tryVerify(() => !creator.visible);
    }

    function test_aChipEditsItsShortcutAndItsCrossRemovesIt() {
        const page = openOn(controlsPage);
        let chip = chipOf(page, "nextImage", "Right");
        // On its text, clear of the cross.
        mouseClick(chip, chip.height / 2, chip.height / 2);
        const creator = subDialog("ShortcutCreatorDialog");
        compare(Fixture.settingsDialog.editor.shortcutEditor.title, "Edit shortcut");
        compare(Fixture.settingsDialog.editor.shortcutEditor.shortcut, "Right");
        mouseClick(findText(creator.contentItem, "Button", "Cancel"));
        tryVerify(() => !creator.visible);

        chip = chipOf(page, "nextImage", "Right");
        mouseClick(chip, chip.width - page.chipRemovePadding - page.chipRemoveSize / 2, chip.height / 2);
        mouseClick(button(settingsWindow.contentItem, "Apply"));
        compare(Fixture.appliedShortcuts, "prevImage=Left");
    }

    function test_theSearchFiltersTheRowsUntilItIsCleared() {
        const page = openOn(controlsPage);
        const allRows = shortcutRowCount(page);
        const field = findAll(page, "SearchField", [])[0];
        verify(field !== undefined);

        field.forceActiveFocus();
        Fixture.sendText(settingsWindow, "prev");
        tryCompare(Fixture.settingsDialog.editor, "shortcutFilter", "prev");
        compare(shortcutRowCount(page), 1);
        // Escape clears the search and keeps the window open.
        Fixture.sendKey(settingsWindow, Qt.Key_Escape);
        tryCompare(Fixture.settingsDialog.editor, "shortcutFilter", "");
        compare(shortcutRowCount(page), allRows);
        verify(settingsWindow.visible);

        Fixture.sendText(settingsWindow, "zoom");
        tryCompare(Fixture.settingsDialog.editor, "shortcutFilter", "zoom");
        compare(shortcutRowCount(page), 1);
        // So does the clear button.
        mouseClick(field, field.width - field.rightPadding / 2, field.height / 2);
        tryCompare(Fixture.settingsDialog.editor, "shortcutFilter", "");
        compare(shortcutRowCount(page), allRows);
    }

    function test_theWheelOverTheShortcutListLeavesThePage() {
        const page = openOn(controlsPage);
        const list = findAll(page, "QQuickListView", [])[0];
        verify(page.contentItem.contentHeight > page.height, "the page does not scroll");
        compare(page.contentItem.contentY, 0);
        mouseWheel(list, list.width / 2, list.height / 2, 0, -120);
        wait(wheelScrollMs);
        compare(page.contentItem.contentY, 0);
    }

    function test_aNewScriptIsAddedAtOnce() {
        const page = openOn(scriptsPage);
        button(page, "Add").clicked();
        const scriptEditor = subDialog("ScriptEditorDialog");
        const create = findText(scriptEditor.contentItem, "Button", "Create");
        verify(!create.enabled);
        Fixture.sendText(scriptEditor, "krita");
        tryVerify(() => create.enabled);
        mouseClick(create);
        tryVerify(() => !scriptEditor.visible);
        compare(Fixture.storedScripts, ["gimp", "krita"]);
        compare(Fixture.settingsApplyCount, 0);
    }

    function test_clearingTheCacheRequestsItAtOnce() {
        const page = openOn(advancedPage);
        button(page, "Clear").clicked();
        compare(Fixture.cacheClearCount, 1);
    }
}
