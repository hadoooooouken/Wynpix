#include <QSet>
#include <QSignalSpy>
#include <QTest>
#include <QUrl>

#include "components/scalingfilter/scalingfilterselection.h"
#include "components/settingseditor/settingseditormodel.h"
#include "components/settingseditor/settingsoptions.h"
#include "components/settingseditor/settingsscales.h"
#include "fakesettingsstores.h"
#include "gui/quick/ui/settings/settingsdialogcontroller.h"
#include "settings_types.h"
#include "testsuites.h"
#include "utils/actions.h"

namespace {
using namespace Qt::StringLiterals;

const QString kDefaultModel = u"4xLSDIRCompactC3"_s;
const QString kOtherModel = u"realesrgan-x4plus"_s;
const QString kDefaultZoomLevels = u"0.5,1,2"_s;

const ShortcutEntry kNextImage{.action = u"nextImage"_s, .shortcut = u"Right"_s};
const ShortcutEntry kPrevImage{.action = u"prevImage"_s, .shortcut = u"Left"_s};
const ShortcutEntry kZoomIn{.action = u"zoomIn"_s, .shortcut = u"+"_s};
const ShortcutEntry kNextImageWheel{.action = u"nextImage"_s, .shortcut = u"WheelDown"_s};

// The values of role in the rows of model, top to bottom.
QStringList column(const QAbstractItemModel &model, int role) {
    QStringList values;
    for (int row = 0; row < model.rowCount(); ++row)
        values.append(model.index(row, 0).data(role).toString());
    return values;
}

// Values that every normalization keeps.
SettingsValues validValues() {
    SettingsValues v;
    v.general.language = u"de_DE"_s;
    v.general.zoomIndicatorMode = INDICATOR_AUTO;
    v.general.autoResizeLimitStep = 18;
    v.general.panelEnabled = true;
    v.general.panelHideDelayMs = 600;
    v.general.thumbPanelStyle = TH_PANEL_EXTENDED;
    v.general.panelSizeStep = 20;
    v.general.panelPosition = PANEL_BOTTOM;
    v.general.folderEndAction = FOLDER_END_LOOP;
    v.general.sortingMode = SORT_TIME_DESC;
    v.general.slideshowIntervalMs = 3000;
    v.view.imageFitMode = FIT_WIDTH;
    v.view.focusPoint = FOCUS_CENTER;
    v.view.expandLimit = 2;
    v.view.zoomStepPercent = 25;
    v.view.zoomLevels = kDefaultZoomLevels;
    v.view.scalingFilter = QI_FILTER_CAS;
    v.view.casSharpeningPercent = 40;
    v.view.casContrastPercent = 10;
    v.view.monitorProfileType = u"DisplayP3"_s;
    v.view.hdrOperator = 2;
    v.view.hdrTargetWhiteLevel = 100;
    v.theme.themeMode = THEME_DARK;
    v.theme.accentColor = QColor(u"#3daee9"_s);
    v.theme.backgroundOpacityPercent = 100;
    v.theme.thumbnailOpacityPercent = 60;
    v.controls.imageScrolling = SCROLL_BY_TRACKPAD;
    v.controls.mouseScrollingSpeedStep = 2;
    v.advanced.thumbnailerThreads = 4;
    v.advanced.thumbnailResolution = 256;
    v.advanced.thumbnailCacheMaxSizeMB = 512;
    v.advanced.jpegQuality = 95;
    v.advanced.modernQuality = 90;
    v.advanced.pngCompression = 3;
    v.advanced.memoryLimitMB = 1024;
    v.upscale.useUpscayl = true;
    v.upscale.model = kOtherModel;
    v.upscale.limitEnabled = true;
    v.upscale.limitPercent = 200;
    return v;
}

struct EditorFixture {
    EditorFixture() {
        values.stored = validValues();
        values.environmentValues = {.upscaylModels = {kDefaultModel, kOtherModel},
                                    .defaultUpscaylModel = kDefaultModel,
                                    .defaultZoomLevels = kDefaultZoomLevels};
        shortcuts.actions = {u"nextImage"_s, u"prevImage"_s, u"zoomIn"_s};
        shortcuts.live = {kNextImage, kPrevImage};
        shortcuts.defaults = {kZoomIn};
    }

    FakeSettingsValueStore values;
    FakeShortcutScriptStore shortcuts;
};
} // namespace

class SettingsEditorTests : public QObject {
    Q_OBJECT

private slots:
    //--- scales -------------------------------------------------------------
    void scalesConvertLikeTheWidgetDialog() {
        using namespace SettingsScales;
        QCOMPARE(zoomStep(15), 15 / 100.f);
        QCOMPARE(zoomStepPercent(0.29f), 29);
        QCOMPARE(mouseScrollingSpeed(2), 1.0f);
        QCOMPARE(mouseScrollingSpeedStep(1.0f), 2);
        QCOMPARE(autoResizeLimitPercent(18), 90);
        QCOMPARE(autoResizeLimitStep(90), 18);
        QCOMPARE(panelSizePixels(20), 160);
        QCOMPARE(panelSizeStep(160), 20);
        QCOMPARE(opacity(60), 0.6);
        QCOMPARE(opacityPercent(0.6), 60);
        QCOMPARE(casValue(40), 40 / 100.f);
        QCOMPARE(casPercent(0.29f), 29);
    }

    void scalesSnapToTheNearestStep() {
        using namespace SettingsScales;
        QCOMPARE(snapped(250, 16), 256);
        QCOMPARE(snapped(247, 16), 240);
        QCOMPARE(snapped(149, 100), 100);
        QCOMPARE(snapped(150, 100), 200);
        QCOMPARE(snapped(7, 0), 7);
    }

    void scalesFormatTheValueTexts() {
        using namespace SettingsScales;
        QCOMPARE(zoomStepText(25), u"0.25x"_s);
        QCOMPARE(expandLimitText(0), u"-"_s);
        QCOMPARE(expandLimitText(2), u"2x"_s);
        QCOMPARE(percentText(95), u"95%"_s);
        QCOMPARE(mouseScrollingSpeedText(0), u"0.50x"_s);
        QCOMPARE(autoResizeLimitText(18), u"90%"_s);
        QCOMPARE(thumbnailResolutionText(256), u"256 px"_s);
        QCOMPARE(casValueText(5), u"0.05"_s);
        QCOMPARE(panelHideDelayText(600), u"600 ms"_s);
        QCOMPARE(pngCompressionText(0), u"Level 0 (None (Uncompressed))"_s);
        QCOMPARE(pngCompressionText(3), u"Level 3 (Fast)"_s);
        QCOMPARE(pngCompressionText(6), u"Level 6 (Balanced)"_s);
        QCOMPARE(pngCompressionText(9), u"Level 9 (Maximum)"_s);
    }

    //--- load / apply -------------------------------------------------------
    void loadThenApplyStoresTheValuesUnchanged() {
        EditorFixture f;
        SettingsEditorModel editor(f.values, f.shortcuts);
        editor.load();
        QCOMPARE(editor.values(), validValues());
        editor.apply();
        QCOMPARE(f.values.applyCount, 1);
        QCOMPARE(f.values.stored, validValues());
        QCOMPARE(f.values.appliedShortcuts, (ShortcutList{kNextImage, kPrevImage}));
    }

    void applyStoresTheEditedDraft() {
        EditorFixture f;
        SettingsEditorModel editor(f.values, f.shortcuts);
        editor.load();
        QSignalSpy generalChanged(&editor, &SettingsEditorModel::generalChanged);
        GeneralSettings general = editor.general();
        general.loopSlideshow = !general.loopSlideshow;
        editor.setGeneral(general);
        editor.setGeneral(general);
        QCOMPARE(generalChanged.count(), 1);
        QCOMPARE(f.values.applyCount, 0);
        editor.apply();
        QCOMPARE(f.values.stored.general.loopSlideshow, general.loopSlideshow);
    }

    void loadFallsBackToOfferedChoices() {
        EditorFixture f;
        f.values.stored.general.language = u"xx_XX"_s;
        f.values.stored.view.scalingFilter = 99;
        f.values.stored.view.monitorProfileType = u"Bogus"_s;
        f.values.stored.view.hdrOperator = 9;
        f.values.stored.view.hdrTargetWhiteLevel = 42;
        f.values.stored.upscale.model = u"missing"_s;
        SettingsEditorModel editor(f.values, f.shortcuts);
        editor.load();
        QCOMPARE(editor.general().language, u"en_US"_s);
        QCOMPARE(editor.view().scalingFilter, static_cast<int>(QI_FILTER_BILINEAR));
        QCOMPARE(editor.view().monitorProfileType, u"System"_s);
        QCOMPARE(editor.view().hdrOperator, 0);
        QCOMPARE(editor.view().hdrTargetWhiteLevel, 203);
        QCOMPARE(editor.upscale().model, kDefaultModel);

        f.values.environmentValues.defaultUpscaylModel = u"absent"_s;
        editor.load();
        QCOMPARE(editor.upscale().model, kDefaultModel);
    }

    void theViewerOffersTheGpuScalingFiltersOnly() {
        const SettingsOptionList options = SettingsOptions::scalingFilters();
        const std::span<const ScalingFilter> viewing = ScalingFilterSelection::viewingFilters();
        QCOMPARE(options.size(), static_cast<qsizetype>(viewing.size()));
        for (qsizetype i = 0; i < options.size(); ++i)
            QCOMPARE(options[i].value.toInt(), static_cast<int>(viewing[static_cast<std::size_t>(i)]));
    }

    void loadLimitsValuesToTheirRanges() {
        EditorFixture f;
        f.values.stored.advanced.thumbnailResolution = 250;
        f.values.stored.general.panelHideDelayMs = 2600;
        f.values.stored.upscale.limitPercent = 402;
        f.values.stored.view.zoomStepPercent = 0;
        f.values.stored.advanced.memoryLimitMB = 100;
        SettingsEditorModel editor(f.values, f.shortcuts);
        editor.load();
        QCOMPARE(editor.advanced().thumbnailResolution, 256);
        QCOMPARE(editor.general().panelHideDelayMs, SettingsScales::kMaxPanelHideDelayMs);
        QCOMPARE(editor.upscale().limitPercent, SettingsScales::kMaxUpscaylLimitPercent);
        QCOMPARE(editor.view().zoomStepPercent, SettingsScales::kMinZoomStepPercent);
        QCOMPARE(editor.advanced().memoryLimitMB, SettingsScales::kMinMemoryLimitMB);

        f.values.stored.advanced.thumbnailResolution = 3000;
        editor.load();
        QCOMPARE(editor.advanced().thumbnailResolution, SettingsScales::kMaxThumbnailResolution);

        f.values.stored.advanced.thumbnailResolution = 50;
        editor.load();
        QCOMPARE(editor.advanced().thumbnailResolution, SettingsScales::kMinThumbnailResolution);
    }

    void withoutUpscaylModelsEveryUpscaylOptionIsOff() {
        EditorFixture f;
        f.values.environmentValues.upscaylModels.clear();
        SettingsEditorModel editor(f.values, f.shortcuts);
        editor.load();
        QVERIFY(!editor.isUpscaylAvailable());
        QVERIFY(!editor.upscale().useUpscayl);
        QVERIFY(!editor.upscale().preloadUpscayl);
        QVERIFY(!editor.upscale().limitEnabled);
        QCOMPARE(editor.upscale().model, kOtherModel);
        QVERIFY(!editor.upscaylOptionsEnabled());
    }

    void derivedStateFollowsTheDraft() {
        EditorFixture f;
        SettingsEditorModel editor(f.values, f.shortcuts);
        editor.load();
        QVERIFY(editor.upscaylOptionsEnabled());
        QVERIFY(editor.upscaylLimitSliderEnabled());
        QVERIFY(editor.casOptionsVisible());
        QVERIFY(!editor.customProfileVisible());

        UpscaleSettings upscale = editor.upscale();
        upscale.limitEnabled = false;
        editor.setUpscale(upscale);
        QVERIFY(!editor.upscaylLimitSliderEnabled());

        ViewSettings view = editor.view();
        view.scalingFilter = QI_FILTER_MKS2021_GPU;
        view.colorManagementEnabled = true;
        view.monitorProfileType = u"Custom"_s;
        editor.setView(view);
        QVERIFY(!editor.casOptionsVisible());
        QVERIFY(editor.customProfileVisible());
        view.colorManagementEnabled = false;
        editor.setView(view);
        QVERIFY(!editor.customProfileVisible());
    }

    void resetZoomLevelsLoadsTheDefaults() {
        EditorFixture f;
        f.values.stored.view.zoomLevels = u"1,2"_s;
        SettingsEditorModel editor(f.values, f.shortcuts);
        editor.load();
        editor.resetZoomLevels();
        QCOMPARE(editor.view().zoomLevels, kDefaultZoomLevels);
    }

    void aPickedProfileFileSetsThePath() {
        EditorFixture f;
        SettingsEditorModel editor(f.values, f.shortcuts);
        editor.load();
        editor.setMonitorProfileFile(QUrl::fromLocalFile(u"C:/profiles/monitor.icc"_s));
        QCOMPARE(editor.view().monitorProfilePath, u"C:/profiles/monitor.icc"_s);
    }

    //--- theme preview ------------------------------------------------------
    void themeValuesArePreviewedAtOnce() {
        EditorFixture f;
        SettingsEditorModel editor(f.values, f.shortcuts);
        editor.load();

        editor.setThemeMode(THEME_LIGHT);
        QCOMPARE(f.values.previewedThemeMode, static_cast<int>(THEME_LIGHT));
        QCOMPARE(editor.theme().themeMode, static_cast<int>(THEME_LIGHT));
        QCOMPARE(editor.theme().accentColor, FakeSettingsValueStore::themeAccent(THEME_LIGHT));

        editor.setUseBlackBackground(true);
        QVERIFY(f.values.previewedBlackBackground);
        QVERIFY(editor.theme().useBlackBackground);

        editor.setThumbnailOpacityPercent(40, false);
        QCOMPARE(f.values.opacityPreviewCount, 0);
        QCOMPARE(editor.theme().thumbnailOpacityPercent, 40);
        editor.setThumbnailOpacityPercent(50, true);
        QCOMPARE(f.values.opacityPreviewCount, 1);
        QCOMPARE(f.values.previewedThumbnailOpacity, 0.5);

        const QColor accent(u"#ff8800"_s);
        editor.setCustomAccent(true);
        editor.setAccentColor(accent);
        QCOMPARE(f.values.customAccent, std::optional<QColor>(accent));
        QCOMPARE(editor.theme().accentColor, accent);
        editor.setCustomAccent(false);
        QVERIFY(!f.values.customAccent);
        QCOMPARE(editor.theme().accentColor, FakeSettingsValueStore::themeAccent(THEME_LIGHT));
        // Turned back on, the custom accent is the last one, not the theme's.
        editor.setCustomAccent(true);
        QCOMPARE(f.values.customAccent, std::optional<QColor>(accent));
        QCOMPARE(editor.theme().accentColor, accent);
        QCOMPARE(f.values.applyCount, 0);
    }

    void anUnknownThemeModeIsIgnored() {
        EditorFixture f;
        SettingsEditorModel editor(f.values, f.shortcuts);
        editor.load();
        QTest::ignoreMessage(QtWarningMsg, "SettingsEditorModel: unknown theme mode 7");
        editor.setThemeMode(7);
        QCOMPARE(editor.theme().themeMode, static_cast<int>(THEME_DARK));
    }

    void clearingTheCacheRefreshesItsSize() {
        EditorFixture f;
        f.values.cacheBytes = 4096;
        SettingsEditorModel editor(f.values, f.shortcuts);
        editor.load();
        const QString before = editor.thumbnailCacheSizeText();
        QSignalSpy sizeChanged(&editor, &SettingsEditorModel::thumbnailCacheSizeChanged);
        editor.clearThumbnailCache();
        QCOMPARE(f.values.cacheClearCount, 1);
        QCOMPARE(sizeChanged.count(), 1);
        QVERIFY(editor.thumbnailCacheSizeText() != before);
    }

    //--- shortcuts ----------------------------------------------------------
    void everyActionHasARowListingItsShortcuts() {
        ActionShortcutModel model;
        model.setActions({{u"nextImage"_s, {}}, {u"prevImage"_s, {}}, {u"zoomIn"_s, {}}}, {});
        model.setEntries({kZoomIn, kNextImageWheel, kPrevImage, kNextImage});
        QCOMPARE(model.entries(),
                 (ShortcutList{kNextImageWheel, kNextImage, kPrevImage, kZoomIn}));
        QCOMPARE(model.rowCount(), 3);
        QCOMPARE(model.data(model.index(0), ActionShortcutModel::ShortcutsRole).toStringList(),
                 (QStringList{kNextImageWheel.shortcut, kNextImage.shortcut}));
    }

    void puttingABoundShortcutMovesIt() {
        ActionShortcutModel model;
        model.setActions({{u"nextImage"_s, {}}, {u"prevImage"_s, {}}, {u"zoomIn"_s, {}}}, {});
        model.setEntries({kNextImage, kPrevImage});
        const ShortcutEntry rebound{.action = u"zoomIn"_s, .shortcut = u"Right"_s};
        QCOMPARE(model.put(rebound), 2);
        QCOMPARE(model.entries(), (ShortcutList{kPrevImage, rebound}));
        // A new shortcut of an action goes after its others.
        QCOMPARE(model.put(kNextImageWheel), 0);
        QCOMPARE(model.put({.action = u"prevImage"_s, .shortcut = u"Up"_s}), 1);
        QCOMPARE(model.put({.action = u""_s, .shortcut = u"X"_s}), -1);
        QCOMPARE(model.put({.action = u"zoomIn"_s, .shortcut = u""_s}), -1);
    }

    void editingAShortcutReplacesItInPlace() {
        ActionShortcutModel model;
        model.setActions({{u"nextImage"_s, {}}, {u"prevImage"_s, {}}, {u"zoomIn"_s, {}}}, {});
        model.setEntries({kNextImage, kNextImageWheel, kZoomIn});
        const ShortcutEntry edited{.action = u"nextImage"_s, .shortcut = u"PgDown"_s};
        QCOMPARE(model.put(edited, kNextImage), 0);
        QCOMPARE(model.entries(), (ShortcutList{edited, kNextImageWheel, kZoomIn}));
        QVERIFY(model.remove(edited));
        QVERIFY(!model.remove(edited));
        QCOMPARE(model.entries(), (ShortcutList{kNextImageWheel, kZoomIn}));
    }

    void everyActionHasACategory() {
        const Actions actions;
        QCOMPARE(actions.category(u"goUp"_s), ActionCategory::FolderView);
        QCOMPARE(actions.category(u"nextImage"_s), ActionCategory::Viewer);
        QCOMPARE(actions.category(u"zoomIn"_s), ActionCategory::Viewer);
        QCOMPARE(actions.category(u"save"_s), ActionCategory::Common);
        QCOMPARE(actions.category(u"contextMenu"_s), ActionCategory::Common);
        QCOMPARE(actions.category(u"noSuchAction"_s), ActionCategory::Common);

        QSet<QString> titles;
        for (const ActionCategory category :
             {ActionCategory::FolderView, ActionCategory::Viewer, ActionCategory::Common})
            titles.insert(Actions::categoryTitle(category));
        QCOMPARE(titles.size(), 3);
        QVERIFY(!titles.contains(QString()));
    }

    void rowsFollowTheCategoriesWithScriptsAndUnknownActionsLast() {
        ActionShortcutModel model;
        model.setActions({{u"zoomIn"_s, {.title = u"Zoom in"_s, .category = u"View"_s, .categoryOrder = 1}},
                          {u"fitWindow"_s, {.title = u"Fit to window"_s, .category = u"View"_s, .categoryOrder = 1}},
                          {u"nextImage"_s, {.title = u"Next image"_s, .category = u"Navigation"_s, .categoryOrder = 0}}},
                         {u"gimp"_s});
        const ShortcutEntry unknown{.action = u"gone"_s, .shortcut = u"F9"_s};
        model.setEntries({unknown, kZoomIn});
        QCOMPARE(column(model, ActionShortcutModel::ActionRole),
                 (QStringList{u"nextImage"_s, u"fitWindow"_s, u"zoomIn"_s, u"s:gimp"_s, u"gone"_s}));
        QCOMPARE(column(model, ActionShortcutModel::CategoryRole),
                 (QStringList{u"Navigation"_s, u"View"_s, u"View"_s, u"Scripts"_s, u"Other"_s}));
        // The category names the script; the dialog and warnings do too.
        QCOMPARE(column(model, ActionShortcutModel::TitleRole).at(3), u"gimp"_s);
        QCOMPARE(shortcutActionInfo(u"s:gimp"_s, {}).title, u"Script: gimp"_s);
        QCOMPARE(model.entries(), (ShortcutList{kZoomIn, unknown}));
    }

    void theFilterMatchesTitlesCategoriesAndShortcuts() {
        ActionShortcutModel model;
        model.setActions({{u"nextImage"_s, {.title = u"Next image"_s, .category = u"Navigation"_s, .categoryOrder = 0}},
                          {u"zoomIn"_s, {.title = u"Zoom in"_s, .category = u"View"_s, .categoryOrder = 1}}},
                         {});
        model.setEntries({kNextImage, kZoomIn});
        ShortcutFilterModel filter;
        filter.setSourceModel(&model);
        const QStringList everything{u"nextImage"_s, u"zoomIn"_s};
        QCOMPARE(column(filter, ActionShortcutModel::ActionRole), everything);

        filter.setFilterText(u"  ZOOM "_s);
        QCOMPARE(filter.filterText(), u"ZOOM"_s);
        QCOMPARE(column(filter, ActionShortcutModel::ActionRole), QStringList{u"zoomIn"_s});
        filter.setFilterText(u"navig"_s);
        QCOMPARE(column(filter, ActionShortcutModel::ActionRole), QStringList{u"nextImage"_s});
        filter.setFilterText(u"right"_s);
        QCOMPARE(column(filter, ActionShortcutModel::ActionRole), QStringList{u"nextImage"_s});

        // A row is filtered again when its shortcuts change.
        filter.setFilterText(u"PgDown"_s);
        QCOMPARE(filter.rowCount(), 0);
        model.put({.action = u"zoomIn"_s, .shortcut = u"PgDown"_s});
        QCOMPARE(column(filter, ActionShortcutModel::ActionRole), QStringList{u"zoomIn"_s});

        filter.setFilterText({});
        QCOMPARE(column(filter, ActionShortcutModel::ActionRole), everything);
    }

    void mouseInputsShowTheirNames() {
        QCOMPARE(shortcutDisplayText(u"XButton1"_s), u"Mouse Back"_s);
        QCOMPARE(shortcutDisplayText(u"XButton2"_s), u"Mouse Forward"_s);
        QCOMPARE(shortcutDisplayText(u"LMB_DoubleClick"_s), u"Double click"_s);
        QCOMPARE(shortcutDisplayText(u"Ctrl+Shift+WheelUp"_s), u"Ctrl+Shift+Wheel up"_s);
        // Keys keep their names, the "+" key too.
        QCOMPARE(shortcutDisplayText(u"Ctrl+Right"_s), u"Ctrl+Right"_s);
        QCOMPARE(shortcutDisplayText(u"Ctrl++"_s), u"Ctrl++"_s);
        QCOMPARE(shortcutDisplayText(u"+"_s), u"+"_s);
    }

    void rowsShowMouseInputsByNameAndTheFilterFindsBothNames() {
        ActionShortcutModel model;
        model.setActions({{u"nextImage"_s, {}}, {u"zoomIn"_s, {}}}, {});
        model.setEntries({kNextImage, kNextImageWheel, kZoomIn});
        QCOMPARE(model.data(model.index(0), ActionShortcutModel::ShortcutTextsRole).toStringList(),
                 (QStringList{u"Right"_s, u"Wheel down"_s}));
        // The stored shortcuts stay as they are.
        QCOMPARE(model.data(model.index(0), ActionShortcutModel::ShortcutsRole).toStringList(),
                 (QStringList{kNextImage.shortcut, kNextImageWheel.shortcut}));

        ShortcutFilterModel filter;
        filter.setSourceModel(&model);
        filter.setFilterText(u"wheel down"_s);
        QCOMPARE(column(filter, ActionShortcutModel::ActionRole), QStringList{u"nextImage"_s});
        filter.setFilterText(u"WheelDown"_s);
        QCOMPARE(column(filter, ActionShortcutModel::ActionRole), QStringList{u"nextImage"_s});
    }

    void theCreatorShowsTheMouseInputByName() {
        EditorFixture f;
        SettingsEditorModel editor(f.values, f.shortcuts);
        editor.load();
        ShortcutEditorModel &creator = *editor.shortcutEditor();
        editor.addShortcut(u"zoomIn"_s);
        creator.setShortcut(u"XButton2"_s);
        QCOMPARE(creator.shortcut(), u"XButton2"_s);
        QCOMPARE(creator.shortcutText(), u"Mouse Forward"_s);
    }

    void theEditorFiltersTheShortcutsAndLoadClearsTheSearch() {
        EditorFixture f;
        SettingsEditorModel editor(f.values, f.shortcuts);
        editor.load();
        ShortcutEditorModel &creator = *editor.shortcutEditor();
        QSignalSpy filterChanged(&editor, &SettingsEditorModel::shortcutFilterChanged);
        QSignalSpy put(&editor, &SettingsEditorModel::shortcutPut);

        editor.setShortcutFilter(u"zoom"_s);
        QCOMPARE(filterChanged.count(), 1);
        QCOMPARE(column(*editor.shortcutView(), ActionShortcutModel::ActionRole), QStringList{u"zoomIn"_s});
        // Spaces around the text search the same.
        editor.setShortcutFilter(u" zoom "_s);
        QCOMPARE(filterChanged.count(), 1);

        // The put shortcut is reported by its row among the shown rows.
        editor.addShortcut(u"zoomIn"_s);
        creator.setShortcut(u"Ctrl+K"_s);
        creator.accept();
        QCOMPARE(put.count(), 1);
        QCOMPARE(put.constFirst().constFirst().toInt(), 0);

        editor.load();
        QVERIFY(editor.shortcutFilter().isEmpty());
        QCOMPARE(filterChanged.count(), 2);
        QCOMPARE(editor.shortcutView()->rowCount(), 3);
    }

    void onlyAnActionWithARowCanGetAShortcut() {
        EditorFixture f;
        SettingsEditorModel editor(f.values, f.shortcuts);
        editor.load();
        QTest::ignoreMessage(QtWarningMsg, "SettingsEditorModel: no action \"noSuchAction\"");
        editor.addShortcut(u"noSuchAction"_s);
        QVERIFY(!editor.shortcutEditor()->isOpen());
        QTest::ignoreMessage(QtWarningMsg, "SettingsEditorModel: no shortcut \"F1\" of \"zoomIn\"");
        editor.editShortcut(u"zoomIn"_s, u"F1"_s);
        QVERIFY(!editor.shortcutEditor()->isOpen());
        QTest::ignoreMessage(QtWarningMsg, "SettingsEditorModel: no shortcut \"F1\" of \"zoomIn\"");
        editor.removeShortcut(u"zoomIn"_s, u"F1"_s);
    }

    void aNewScriptGetsARowAtOnce() {
        EditorFixture f;
        SettingsEditorModel editor(f.values, f.shortcuts);
        editor.load();
        ScriptEditorModel &scriptEditor = *editor.scriptEditor();
        editor.addScript();
        scriptEditor.setName(u"krita"_s);
        scriptEditor.setExecutablePath(u"C:/Apps/krita.exe"_s);
        scriptEditor.accept();
        QCOMPARE(editor.actionShortcuts()->rowOf(u"s:krita"_s), 3);
        editor.addShortcut(u"s:krita"_s);
        QCOMPARE(editor.shortcutEditor()->actionTitle(), u"Script: krita"_s);
        QCOMPARE(editor.shortcutEditor()->categoryTitle(), u"Scripts"_s);
    }

    void resetRestoresTheDefaultShortcuts() {
        EditorFixture f;
        SettingsEditorModel editor(f.values, f.shortcuts);
        editor.load();
        editor.resetShortcuts();
        QCOMPARE(editor.actionShortcuts()->entries(), (ShortcutList{kZoomIn}));
        // Like every shortcut edit, the reset waits for apply(): Cancel keeps
        // the shortcuts the application has.
        QCOMPARE(f.shortcuts.live, (ShortcutList{kNextImage, kPrevImage}));
        editor.apply();
        QCOMPARE(f.values.appliedShortcuts, (ShortcutList{kZoomIn}));
    }

    //--- shortcut creator ---------------------------------------------------
    void theCreatorAcceptsOnlyWithAShortcut() {
        EditorFixture f;
        SettingsEditorModel editor(f.values, f.shortcuts);
        editor.load();
        ShortcutEditorModel &creator = *editor.shortcutEditor();
        editor.addShortcut(u"zoomIn"_s);
        QVERIFY(creator.isOpen());
        QVERIFY(creator.isCreated());
        QVERIFY(!creator.canAccept());
        QCOMPARE(creator.actionTitle(), u"zoomIn"_s);
        QCOMPARE(creator.shortcutText(), u"[Enter shortcut]"_s);
        creator.accept();
        QVERIFY(creator.isOpen());

        creator.setShortcut(u""_s);
        QVERIFY(!creator.canAccept());
        creator.setShortcut(u"Ctrl+K"_s);
        QVERIFY(creator.canAccept());
        QVERIFY(creator.warning().isEmpty());
    }

    void theCreatorWarnsAboutABoundShortcutAndReplacesIt() {
        EditorFixture f;
        SettingsEditorModel editor(f.values, f.shortcuts);
        editor.load();
        ShortcutEditorModel &creator = *editor.shortcutEditor();
        QSignalSpy put(&editor, &SettingsEditorModel::shortcutPut);
        editor.addShortcut(u"zoomIn"_s);
        creator.setShortcut(u"Right"_s);
        QCOMPARE(creator.warning(), u"This shortcut is used for action: nextImage. Replace?"_s);
        creator.accept();
        QVERIFY(!creator.isOpen());
        QCOMPARE(put.count(), 1);
        const ShortcutEntry bound{.action = u"zoomIn"_s, .shortcut = u"Right"_s};
        QCOMPARE(editor.actionShortcuts()->entries(), (ShortcutList{kPrevImage, bound}));
        QCOMPARE(put.constFirst().constFirst().toInt(), 2);
    }

    void theCreatorChecksTheShortcutsBeingEdited() {
        EditorFixture f;
        SettingsEditorModel editor(f.values, f.shortcuts);
        editor.load();
        ShortcutEditorModel &creator = *editor.shortcutEditor();

        // Removed, not applied yet: free.
        editor.removeShortcut(kNextImage.action, kNextImage.shortcut);
        editor.addShortcut(u"zoomIn"_s);
        creator.setShortcut(u"Right"_s);
        QVERIFY(creator.warning().isEmpty());
        creator.accept();

        // Added, not applied yet: bound.
        editor.addShortcut(u"prevImage"_s);
        creator.setShortcut(u"Right"_s);
        QCOMPARE(creator.warning(), u"This shortcut is used for action: zoomIn. Replace?"_s);
        creator.reject();
        QVERIFY(f.shortcuts.live.contains(kNextImage));
    }

    void theEditedShortcutIsNoConflict() {
        EditorFixture f;
        SettingsEditorModel editor(f.values, f.shortcuts);
        editor.load();
        ShortcutEditorModel &creator = *editor.shortcutEditor();
        editor.editShortcut(kNextImage.action, kNextImage.shortcut);
        QCOMPARE(creator.shortcut(), kNextImage.shortcut);
        creator.setShortcut(kNextImage.shortcut);
        QVERIFY(creator.warning().isEmpty());
        // The shortcut of another binding still is one.
        creator.setShortcut(kPrevImage.shortcut);
        QCOMPARE(creator.warning(), u"This shortcut is used for action: prevImage. Replace?"_s);
    }

    void aScriptBindingIsEditedInItsRow() {
        EditorFixture f;
        f.shortcuts.scripts = {{u"a"_s, {}}, {u"b"_s, {}}};
        const ShortcutEntry scriptEntry{.action = u"s:b"_s, .shortcut = u"F5"_s};
        f.shortcuts.live = {kNextImage, scriptEntry};
        SettingsEditorModel editor(f.values, f.shortcuts);
        editor.load();
        ShortcutEditorModel &creator = *editor.shortcutEditor();
        QCOMPARE(editor.actionShortcuts()->entries(), (ShortcutList{kNextImage, scriptEntry}));
        editor.editShortcut(scriptEntry.action, scriptEntry.shortcut);
        QCOMPARE(creator.actionTitle(), u"Script: b"_s);
        QCOMPARE(creator.shortcut(), u"F5"_s);
        QVERIFY(creator.warning().isEmpty());
        QCOMPARE(creator.edited(), std::optional<ShortcutEntry>(scriptEntry));
        creator.setShortcut(u"F6"_s);
        creator.accept();
        QCOMPARE(editor.actionShortcuts()->entries(),
                 (ShortcutList{kNextImage, {.action = u"s:b"_s, .shortcut = u"F6"_s}}));
    }

    void rejectingTheCreatorChangesNothing() {
        EditorFixture f;
        SettingsEditorModel editor(f.values, f.shortcuts);
        editor.load();
        ShortcutEditorModel &creator = *editor.shortcutEditor();
        QSignalSpy rejected(&creator, &EditorSession::rejected);
        editor.addShortcut(u"zoomIn"_s);
        creator.setShortcut(u"Ctrl+K"_s);
        creator.reject();
        QCOMPARE(rejected.count(), 1);
        QCOMPARE(editor.actionShortcuts()->entries(), (ShortcutList{kNextImage, kPrevImage}));
    }

    //--- scripts ------------------------------------------------------------
    void theScriptEditorValidatesTheName() {
        EditorFixture f;
        f.shortcuts.scripts = {{u"gimp"_s, {.command = u"gimp %file%"_s, .blocking = true}}};
        SettingsEditorModel editor(f.values, f.shortcuts);
        editor.load();
        ScriptEditorModel &scriptEditor = *editor.scriptEditor();

        editor.addScript();
        QVERIFY(scriptEditor.isOpen());
        QCOMPARE(scriptEditor.title(), u"New application/script"_s);
        QVERIFY(!scriptEditor.canAccept());
        QCOMPARE(scriptEditor.message(), u"Enter script name"_s);
        scriptEditor.setName(u"gimp"_s);
        QCOMPARE(scriptEditor.message(), u"A script with this same name exists"_s);
        QCOMPARE(scriptEditor.acceptText(), u"Replace"_s);
        scriptEditor.setName(u"krita"_s);
        QVERIFY(scriptEditor.message().isEmpty());
        QCOMPARE(scriptEditor.acceptText(), u"Create"_s);
        scriptEditor.setExecutablePath(u"C:/Apps/krita.exe"_s);
        QCOMPARE(scriptEditor.command(), u"\"C:/Apps/krita.exe\" %file%"_s);
        scriptEditor.accept();
        QVERIFY(f.shortcuts.scripts.contains(u"krita"_s));
        QCOMPARE(editor.scriptList()->stringList(), (QStringList{u"gimp"_s, u"krita"_s}));
    }

    void editingAScriptKeepsOrReplacesItsName() {
        EditorFixture f;
        f.shortcuts.scripts = {{u"gimp"_s, {.command = u"gimp %file%"_s, .blocking = true}},
                               {u"krita"_s, {}}};
        SettingsEditorModel editor(f.values, f.shortcuts);
        editor.load();
        ScriptEditorModel &scriptEditor = *editor.scriptEditor();

        editor.editScript(0);
        QCOMPARE(scriptEditor.title(), u"Edit"_s);
        QCOMPARE(scriptEditor.name(), u"gimp"_s);
        QCOMPARE(scriptEditor.command(), u"gimp %file%"_s);
        QVERIFY(scriptEditor.isBlocking());
        QCOMPARE(scriptEditor.acceptText(), u"Save"_s);
        scriptEditor.setName(u"krita"_s);
        QCOMPARE(scriptEditor.acceptText(), u"Replace"_s);
        scriptEditor.reject();
        QCOMPARE(f.shortcuts.scripts.size(), 2);
    }

    void removingAScriptRemovesItsShortcuts() {
        EditorFixture f;
        f.shortcuts.scripts = {{u"gimp"_s, {}}};
        const ShortcutEntry scriptEntry{.action = u"s:gimp"_s, .shortcut = u"F5"_s};
        f.shortcuts.live = {kNextImage, scriptEntry};
        SettingsEditorModel editor(f.values, f.shortcuts);
        editor.load();
        editor.actionShortcuts()->put(kZoomIn);
        editor.removeScript(0);
        QVERIFY(f.shortcuts.scripts.isEmpty());
        QVERIFY(editor.scriptList()->stringList().isEmpty());
        // The unsaved table edits are kept.
        QCOMPARE(editor.actionShortcuts()->entries(), (ShortcutList{kNextImage, kZoomIn}));
    }

    //--- Quick settings window ----------------------------------------------
    void theWindowLoadsOnOpenAndAppliesOnAccept() {
        EditorFixture f;
        SettingsEditorModel editor(f.values, f.shortcuts);
        SettingsDialogController controller(editor);
        QVERIFY(!controller.isCreated());
        controller.show(SettingsEditorModel::Page::Scripts);
        QVERIFY(controller.isOpen());
        QVERIFY(controller.isCreated());
        QCOMPARE(controller.page(), static_cast<int>(SettingsEditorModel::Page::Scripts));
        QCOMPARE(editor.values(), validValues());

        GeneralSettings general = editor.general();
        general.sortFolders = !general.sortFolders;
        editor.setGeneral(general);
        controller.applyChanges();
        QVERIFY(controller.isOpen());
        QCOMPARE(f.values.applyCount, 1);
        controller.accept();
        QVERIFY(!controller.isOpen());
        QCOMPARE(f.values.applyCount, 2);
        QCOMPARE(f.values.stored.general.sortFolders, general.sortFolders);
    }

    void dismissingTheWindowAppliesNothing() {
        EditorFixture f;
        SettingsEditorModel editor(f.values, f.shortcuts);
        SettingsDialogController controller(editor);
        controller.show(SettingsEditorModel::Page::General);
        controller.dismiss();
        QVERIFY(!controller.isOpen());
        QVERIFY(controller.isCreated());
        QCOMPARE(f.values.applyCount, 0);
    }

    void showingAnOpenWindowOnlySwitchesThePage() {
        EditorFixture f;
        SettingsEditorModel editor(f.values, f.shortcuts);
        SettingsDialogController controller(editor);
        controller.show(SettingsEditorModel::Page::General);
        GeneralSettings general = editor.general();
        general.sortFolders = !general.sortFolders;
        editor.setGeneral(general);
        controller.show(SettingsEditorModel::Page::About);
        QCOMPARE(controller.page(), static_cast<int>(SettingsEditorModel::Page::About));
        QCOMPARE(editor.general().sortFolders, general.sortFolders);
        QTest::ignoreMessage(QtWarningMsg, "SettingsDialogController: no settings page 8");
        controller.setPage(SettingsDialogController::pageCount());
        QCOMPARE(controller.page(), static_cast<int>(SettingsEditorModel::Page::About));
    }

    void aboutTextIsFormattedAsStyledText() {
        const QString text = SettingsEditorModel::aboutText();
        QVERIFY(text.contains(u"<a href=\""_s));
        QVERIFY(text.contains(u"<b>"_s));
        QVERIFY(text.contains(u"<br>"_s));
        QVERIFY(!text.contains(u"**"_s));
        QVERIFY(!text.contains(u"]("_s));
        QVERIFY(!text.contains(u'\n'));
    }
};

int runSettingsEditorTests(int argc, char **argv) {
    SettingsEditorTests tests;
    return QTest::qExec(&tests, argc, argv);
}

#include "tst_settingseditor.moc"
