#include <QCoreApplication>
#include <QSettings>
#include <QTest>
#include <QVariant>

#include <cmath>
#include <limits>

#include "gui/settingseditor/settingsvaluemapping.h"
#include "settings.h"
#include "testsuites.h"

// Golden-ini test of SettingsValueMapping: storing the settings dialog's
// values writes the 3.6.4 keys with the values the 3.6.4 widget dialog
// (gui/dialogs/settingsdialog.cpp, saveSettings() at bb8df119) wrote for the
// same choices, so an upgraded configuration keeps its meaning.
//
// The application's Settings writes next to the test executable (conf/), as
// it does next to Wynpix. Two complete value sets that differ in every
// field are stored one after the other: a value that is not stored keeps the
// other set's value or is missing, whatever an earlier run left in the file.

namespace {
using namespace Qt::StringLiterals;

// Floats are written with the shortest round-trip text of the float.
constexpr double kFloatTolerance = 1e-6;

// One ini value of a value set: the key and the text 3.6.4 wrote, or a
// number compared within kFloatTolerance.
struct IniValue {
  QString key;
  QVariant expected;
};

// Field values of one set. Each field of SettingsValues differs between set
// A and set B, and so does each expected ini value.
struct ValueSet {
  SettingsValues values;
  QList<IniValue> ini;
  // Theme.ini accent, empty when the set has no custom accent (key removed).
  QString accent;
};

ValueSet valueSetA() {
  ValueSet set;
  SettingsValues &v = set.values;
  v.general = {.language = u"de_DE"_s,
               .fullscreenMode = true,
               .startInFolderView = true,
               .standbyMode = true,
               .rememberLastFolder = true,
               .windowTitleExtendedInfo = true,
               .infoBarFullscreen = true,
               .showAnimationControls = true,
               .cursorAutohide = true,
               .smoothScroll = true,
               .smoothZoom = true,
               .zoomIndicatorMode = INDICATOR_ENABLED,
               .autoResizeWindow = true,
               .autoResizeLimitStep = 14,
               .panelEnabled = true,
               .panelPinned = true,
               .panelFullscreenOnly = true,
               .panelCenterSelection = true,
               .showSubfoldersInPanel = true,
               .panelHideDelayMs = 300,
               .thumbPanelStyle = TH_PANEL_EXTENDED,
               .panelSizeStep = 15,
               .panelPosition = PANEL_LEFT,
               .folderEndAction = FOLDER_END_LOOP,
               .sortingMode = SORT_SIZE_DESC,
               .sortFolders = true,
               .showHiddenFiles = true,
               .slideshowIntervalMs = 4500,
               .loopSlideshow = true};
  v.view = {.imageFitMode = FIT_WIDTH,
            .keepFitMode = true,
            .focusPoint = FOCUS_CENTER,
            .transparencyGrid = true,
            .expandImage = true,
            .expandLimit = 2,
            .unlockMinZoom = true,
            .zoomStepPercent = 15,
            .useFixedZoomLevels = true,
            .zoomLevels = u"0.5,1,2"_s,
            .scalingFilter = QI_FILTER_SMART_GPU,
            .casSharpeningPercent = 35,
            .casContrastPercent = 10,
            .colorManagementEnabled = true,
            .monitorProfileType = u"Custom"_s,
            .monitorProfilePath = u"C:/profiles/a.icc"_s,
            .hdrToneMappingEnabled = true,
            .hdrOperator = 1,
            .hdrTargetWhiteLevel = 100};
  v.theme = {.themeMode = THEME_DARK,
             .customAccent = true,
             .accentColor = QColor(u"#3366cc"_s),
             .backgroundOpacityPercent = 85,
             .thumbnailOpacityPercent = 70,
             .useBlackBackground = true};
  v.controls = {.clickableEdges = true,
                .clickableEdgesVisible = true,
                .imageScrolling = SCROLL_BY_TRACKPAD,
                .mouseScrollingSpeedStep = 2,
                .trackpadDetection = true};
  v.advanced = {.usePreloader = true,
                .thumbnailerThreads = 3,
                .useThumbnailCache = true,
                .thumbnailResolution = 192,
                .thumbnailCacheMaxSizeMB = 256,
                .excludedCachePaths = u"C:/a;C:/b"_s,
                .unloadThumbs = true,
                .showSaveOverlay = true,
                .jpegQuality = 80,
                .modernQuality = 70,
                .pngCompression = 5,
                .confirmTrash = true,
                .confirmDelete = true,
                .multiInstance = true,
                .memoryLimitMB = 1024};
  v.upscale = {.useUpscayl = true,
               .model = u"anime"_s,
               .preloadUpscayl = true,
               .limitEnabled = true,
               .limitPercent = 150};

  set.ini = {
      {u"language"_s, u"de_DE"_s},
      {u"openInFullscreen"_s, true},
      {u"defaultViewMode"_s, int(MODE_FOLDERVIEW)},
      {u"standbyMode"_s, true},
      {u"rememberLastFolder"_s, true},
      {u"windowTitleExtendedInfo"_s, true},
      {u"infoBarFullscreen"_s, true},
      {u"showAnimationControls"_s, true},
      {u"cursorAutohiding"_s, true},
      {u"enableSmoothScroll"_s, true},
      {u"enableSmoothZoom"_s, true},
      {u"zoomIndicatorMode"_s, int(INDICATOR_ENABLED)},
      {u"autoResizeWindow"_s, true},
      // 3.6.4: slider value * 5.
      {u"autoResizeLimit"_s, 70},
      {u"panelEnabled"_s, true},
      {u"panelPinned"_s, true},
      {u"panelFullscreenOnly"_s, true},
      {u"panelCenterSelection"_s, true},
      {u"showSubfoldersInPanel"_s, true},
      {u"panelHideDelayMs"_s, 300},
      {u"thumbPanelStyle"_s, int(TH_PANEL_EXTENDED)},
      // 3.6.4: slider value * 8.
      {u"panelPreviewsSize"_s, 120},
      // 3.6.4 stores the side by name.
      {u"panelPosition"_s, u"left"_s},
      {u"folderEndAction"_s, int(FOLDER_END_LOOP)},
      {u"sortingMode"_s, int(SORT_SIZE_DESC)},
      {u"sortFolders"_s, true},
      {u"showHiddenFiles"_s, true},
      {u"slideshowInterval"_s, 4500},
      {u"loopSlideshow"_s, true},
      {u"defaultFitMode"_s, int(FIT_WIDTH)},
      {u"keepFitMode"_s, true},
      {u"focusPointIn1to1Mode"_s, int(FOCUS_CENTER)},
      {u"drawTransparencyGrid"_s, true},
      {u"expandImage"_s, true},
      {u"expandLimit"_s, 2},
      {u"unlockMinZoom"_s, true},
      // 3.6.4: slider value / 100.
      {u"zoomStep"_s, 0.15},
      {u"useFixedZoomLevels"_s, true},
      {u"fixedZoomLevels"_s, u"0.5,1,2"_s},
      {u"scalingFilter"_s, int(QI_FILTER_SMART_GPU)},
      {u"casSharpening"_s, 0.35},
      {u"casContrast"_s, 0.1},
      {u"colorManagementEnabled"_s, true},
      {u"monitorColorProfileType"_s, u"Custom"_s},
      {u"monitorColorProfilePath"_s, u"C:/profiles/a.icc"_s},
      {u"hdrToneMappingEnabled"_s, true},
      {u"hdrToneMappingOperator"_s, 1},
      {u"hdrTargetWhiteLevel"_s, 100},
      {u"themeMode"_s, int(THEME_DARK)},
      {u"backgroundOpacity"_s, 0.85},
      {u"thumbnailOpacity"_s, 0.7},
      {u"useBlackBackground"_s, true},
      {u"clickableEdges"_s, true},
      {u"clickableEdgesVisible"_s, true},
      {u"imageScrolling"_s, int(SCROLL_BY_TRACKPAD)},
      // 3.6.4: 0.5 + slider value * 0.25.
      {u"mouseScrollingSpeed"_s, 1.0},
      {u"trackpadDetection"_s, true},
      {u"usePreloader"_s, true},
      {u"thumbnailerThreads"_s, 3},
      {u"thumbnailCache"_s, true},
      {u"thumbnailResolution"_s, 192},
      {u"thumbnailCacheMaxSizeMB"_s, 256},
      {u"excludedCachePaths"_s, u"C:/a;C:/b"_s},
      {u"unloadThumbs"_s, true},
      {u"showSaveOverlay"_s, true},
      {u"JPEGSaveQuality"_s, 80},
      {u"modernSaveQuality"_s, 70},
      {u"pngSaveQuality"_s, 5},
      {u"confirmTrash"_s, true},
      {u"confirmDelete"_s, true},
      {u"multiInstance"_s, true},
      {u"memoryAllocationLimit"_s, 1024},
      {u"useUpscayl"_s, true},
      {u"upscaylModel"_s, u"anime"_s},
      {u"preloadUpscayl"_s, true},
      {u"upscaylLimitEnabled"_s, true},
      {u"upscaylLimitValue"_s, 150},
  };
  set.accent = u"#3366cc"_s;
  return set;
}

ValueSet valueSetB() {
  ValueSet set;
  SettingsValues &v = set.values;
  v.general = {.language = u"en_US"_s,
               .fullscreenMode = false,
               .startInFolderView = false,
               .standbyMode = false,
               .rememberLastFolder = false,
               .windowTitleExtendedInfo = false,
               .infoBarFullscreen = false,
               .showAnimationControls = false,
               .cursorAutohide = false,
               .smoothScroll = false,
               .smoothZoom = false,
               .zoomIndicatorMode = INDICATOR_AUTO,
               .autoResizeWindow = false,
               .autoResizeLimitStep = 18,
               .panelEnabled = false,
               .panelPinned = false,
               .panelFullscreenOnly = false,
               .panelCenterSelection = false,
               .showSubfoldersInPanel = false,
               .panelHideDelayMs = 1200,
               .thumbPanelStyle = TH_PANEL_SIMPLE,
               .panelSizeStep = 24,
               .panelPosition = PANEL_RIGHT,
               .folderEndAction = FOLDER_END_GOTO_ADJACENT,
               .sortingMode = SORT_TIME,
               .sortFolders = false,
               .showHiddenFiles = false,
               .slideshowIntervalMs = 12000,
               .loopSlideshow = false};
  v.view = {.imageFitMode = FIT_HEIGHT,
            .keepFitMode = false,
            .focusPoint = FOCUS_CURSOR,
            .transparencyGrid = false,
            .expandImage = false,
            .expandLimit = 4,
            .unlockMinZoom = false,
            .zoomStepPercent = 40,
            .useFixedZoomLevels = false,
            .zoomLevels = u"0.25,1,4"_s,
            .scalingFilter = QI_FILTER_MKS2021_GPU,
            .casSharpeningPercent = 80,
            .casContrastPercent = 55,
            .colorManagementEnabled = false,
            .monitorProfileType = u"System"_s,
            .monitorProfilePath = u"C:/profiles/b.icc"_s,
            .hdrToneMappingEnabled = false,
            .hdrOperator = 2,
            .hdrTargetWhiteLevel = 300};
  v.theme = {.themeMode = THEME_LIGHT,
             .customAccent = false,
             .accentColor = QColor(u"#cc6633"_s),
             .backgroundOpacityPercent = 40,
             .thumbnailOpacityPercent = 30,
             .useBlackBackground = false};
  v.controls = {.clickableEdges = false,
                .clickableEdgesVisible = false,
                .imageScrolling = SCROLL_BY_TRACKPAD_AND_WHEEL,
                .mouseScrollingSpeedStep = 5,
                .trackpadDetection = false};
  v.advanced = {.usePreloader = false,
                .thumbnailerThreads = 12,
                .useThumbnailCache = false,
                .thumbnailResolution = 384,
                .thumbnailCacheMaxSizeMB = 1024,
                .excludedCachePaths = u"D:/c"_s,
                .unloadThumbs = false,
                .showSaveOverlay = false,
                .jpegQuality = 92,
                .modernQuality = 85,
                .pngCompression = 8,
                .confirmTrash = false,
                .confirmDelete = false,
                .multiInstance = false,
                .memoryLimitMB = 4096};
  v.upscale = {.useUpscayl = false,
               .model = u"x4plus"_s,
               .preloadUpscayl = false,
               .limitEnabled = false,
               .limitPercent = 300};

  set.ini = {
      {u"language"_s, u"en_US"_s},
      {u"openInFullscreen"_s, false},
      {u"defaultViewMode"_s, int(MODE_DOCUMENT)},
      {u"standbyMode"_s, false},
      {u"rememberLastFolder"_s, false},
      {u"windowTitleExtendedInfo"_s, false},
      {u"infoBarFullscreen"_s, false},
      {u"showAnimationControls"_s, false},
      {u"cursorAutohiding"_s, false},
      {u"enableSmoothScroll"_s, false},
      {u"enableSmoothZoom"_s, false},
      {u"zoomIndicatorMode"_s, int(INDICATOR_AUTO)},
      {u"autoResizeWindow"_s, false},
      {u"autoResizeLimit"_s, 90},
      {u"panelEnabled"_s, false},
      {u"panelPinned"_s, false},
      {u"panelFullscreenOnly"_s, false},
      {u"panelCenterSelection"_s, false},
      {u"showSubfoldersInPanel"_s, false},
      {u"panelHideDelayMs"_s, 1200},
      {u"thumbPanelStyle"_s, int(TH_PANEL_SIMPLE)},
      {u"panelPreviewsSize"_s, 192},
      {u"panelPosition"_s, u"right"_s},
      {u"folderEndAction"_s, int(FOLDER_END_GOTO_ADJACENT)},
      {u"sortingMode"_s, int(SORT_TIME)},
      {u"sortFolders"_s, false},
      {u"showHiddenFiles"_s, false},
      {u"slideshowInterval"_s, 12000},
      {u"loopSlideshow"_s, false},
      {u"defaultFitMode"_s, int(FIT_HEIGHT)},
      {u"keepFitMode"_s, false},
      {u"focusPointIn1to1Mode"_s, int(FOCUS_CURSOR)},
      {u"drawTransparencyGrid"_s, false},
      {u"expandImage"_s, false},
      {u"expandLimit"_s, 4},
      {u"unlockMinZoom"_s, false},
      {u"zoomStep"_s, 0.4},
      {u"useFixedZoomLevels"_s, false},
      {u"fixedZoomLevels"_s, u"0.25,1,4"_s},
      {u"scalingFilter"_s, int(QI_FILTER_MKS2021_GPU)},
      {u"casSharpening"_s, 0.8},
      {u"casContrast"_s, 0.55},
      {u"colorManagementEnabled"_s, false},
      {u"monitorColorProfileType"_s, u"System"_s},
      {u"monitorColorProfilePath"_s, u"C:/profiles/b.icc"_s},
      {u"hdrToneMappingEnabled"_s, false},
      {u"hdrToneMappingOperator"_s, 2},
      {u"hdrTargetWhiteLevel"_s, 300},
      {u"themeMode"_s, int(THEME_LIGHT)},
      {u"backgroundOpacity"_s, 0.4},
      {u"thumbnailOpacity"_s, 0.3},
      {u"useBlackBackground"_s, false},
      {u"clickableEdges"_s, false},
      {u"clickableEdgesVisible"_s, false},
      {u"imageScrolling"_s, int(SCROLL_BY_TRACKPAD_AND_WHEEL)},
      {u"mouseScrollingSpeed"_s, 1.75},
      {u"trackpadDetection"_s, false},
      {u"usePreloader"_s, false},
      {u"thumbnailerThreads"_s, 12},
      {u"thumbnailCache"_s, false},
      {u"thumbnailResolution"_s, 384},
      {u"thumbnailCacheMaxSizeMB"_s, 1024},
      {u"excludedCachePaths"_s, u"D:/c"_s},
      {u"unloadThumbs"_s, false},
      {u"showSaveOverlay"_s, false},
      {u"JPEGSaveQuality"_s, 92},
      {u"modernSaveQuality"_s, 85},
      {u"pngSaveQuality"_s, 8},
      {u"confirmTrash"_s, false},
      {u"confirmDelete"_s, false},
      {u"multiInstance"_s, false},
      {u"memoryAllocationLimit"_s, 4096},
      {u"useUpscayl"_s, false},
      {u"upscaylModel"_s, u"x4plus"_s},
      {u"preloadUpscayl"_s, false},
      {u"upscaylLimitEnabled"_s, false},
      {u"upscaylLimitValue"_s, 300},
  };
  return set;
}

QString confPath(const QString &fileName) {
  return QCoreApplication::applicationDirPath() + u"/conf/"_s + fileName;
}
} // namespace

class SettingsValueMappingTests : public QObject {
  Q_OBJECT

private slots:
  void initTestCase() {
    mSettings = Settings::getInstance();
    // As main.cpp does: the ini can hold scripts of an earlier run.
    qRegisterMetaType<Script>("Script");
    QVERIFY(mSettings);
  }

  void bothValueSetsDifferInEveryKey() {
    const ValueSet a = valueSetA();
    const ValueSet b = valueSetB();
    QCOMPARE(a.ini.size(), b.ini.size());
    for (qsizetype i = 0; i < a.ini.size(); ++i) {
      QCOMPARE(a.ini[i].key, b.ini[i].key);
      QVERIFY2(a.ini[i].expected != b.ini[i].expected, qPrintable(a.ini[i].key));
    }
  }

  void storedValuesUseThe364Keys() {
    for (const ValueSet &set : {valueSetA(), valueSetB(), valueSetA()}) {
      SettingsValueMapping mapping(*mSettings);
      mapping.store(set.values);
      mSettings->sync();
      verifyIni(set);
      if (QTest::currentTestFailed())
        return;
    }
  }

  void storedValuesLoadBack() {
    SettingsValueMapping mapping(*mSettings);
    for (const ValueSet &set : {valueSetA(), valueSetB()}) {
      mapping.store(set.values);
      SettingsValues expected = set.values;
      const SettingsValues loaded = mapping.load();
      // Without a custom accent the scheme's own accent is shown.
      if (!expected.theme.customAccent)
        expected.theme.accentColor = loaded.theme.accentColor;
      // Settings keeps the last custom accent, the one of set A, also
      // while set B turns the custom accent off.
      expected.theme.lastCustomAccent = valueSetA().values.theme.accentColor;
      QCOMPARE(loaded.general, expected.general);
      QCOMPARE(loaded.view, expected.view);
      QCOMPARE(loaded.theme, expected.theme);
      QCOMPARE(loaded.controls, expected.controls);
      QCOMPARE(loaded.advanced, expected.advanced);
      QCOMPARE(loaded.upscale, expected.upscale);
    }
  }

  // A script goes through QDataStream into the ini, with the stream
  // operators every translation unit sees in utils/script.h.
  void scriptsSaveAndReadBack() {
    QMap<QString, Script> scripts;
    scripts.insert(u"blocking"_s, Script(u"cmd /c echo %file%"_s, true));
    scripts.insert(u"detached"_s, Script(u"notepad.exe %file%"_s, false));
    mSettings->saveScripts(scripts);
    mSettings->sync();

    QSettings ini(confPath(QCoreApplication::applicationName() + u".ini"_s),
                  QSettings::IniFormat);
    ini.beginGroup(u"Scripts"_s);
    QCOMPARE(ini.beginReadArray(u"script"_s), 2);
    for (int i = 0; i < 2; ++i) {
      ini.setArrayIndex(i);
      const QString name = ini.value(u"name"_s).toString();
      QVERIFY2(scripts.contains(name), qPrintable(name));
      const Script stored = ini.value(u"value"_s).value<Script>();
      QCOMPARE(stored.command, scripts.value(name).command);
      QCOMPARE(stored.blocking, scripts.value(name).blocking);
    }
    ini.endArray();
    ini.endGroup();

    mSettings->saveScripts({});
    mSettings->sync();
  }

  void defaultThumbnailResolutionDependsOnDpr() {
    // Default parameter uses DefaultDevicePixelRatio (1.0 -> 256)
    QCOMPARE(Settings::computeDefaultThumbnailResolution(), Settings::DefaultThumbnailResolution);
    QCOMPARE(Settings::computeDefaultThumbnailResolution(Settings::DefaultDevicePixelRatio),
             Settings::DefaultThumbnailResolution);

    // 100% -> 256
    QCOMPARE(Settings::computeDefaultThumbnailResolution(1.0), 256);
    // 125% -> 320
    QCOMPARE(Settings::computeDefaultThumbnailResolution(1.25), 320);
    // 150% -> 384
    QCOMPARE(Settings::computeDefaultThumbnailResolution(1.5), 384);
    // 175% -> 448
    QCOMPARE(Settings::computeDefaultThumbnailResolution(1.75), 448);
    // 200% -> 512
    QCOMPARE(Settings::computeDefaultThumbnailResolution(2.0), 512);

    // Floating-point noise around standard scale factors
    QCOMPARE(Settings::computeDefaultThumbnailResolution(1.2500000001), 320);
    QCOMPARE(Settings::computeDefaultThumbnailResolution(1.2499999999), 320);

    // Rounding up to step 16
    // 256 * 1.33 = 340.48 -> round 340 -> step up 352
    QCOMPARE(Settings::computeDefaultThumbnailResolution(1.33), 352);

    // Clamping to bounds [128, 2048]
    QCOMPARE(Settings::computeDefaultThumbnailResolution(0.1), Settings::MinThumbnailResolution);
    QCOMPARE(Settings::computeDefaultThumbnailResolution(10.0), Settings::MaxThumbnailResolution);

    // Infinity
    QCOMPARE(Settings::computeDefaultThumbnailResolution(std::numeric_limits<double>::infinity()),
             Settings::MaxThumbnailResolution);
    QCOMPARE(Settings::computeDefaultThumbnailResolution(-std::numeric_limits<double>::infinity()),
             Settings::DefaultThumbnailResolution);

    // Invalid / fallback values
    QCOMPARE(Settings::computeDefaultThumbnailResolution(0.0), Settings::DefaultThumbnailResolution);
    QCOMPARE(Settings::computeDefaultThumbnailResolution(-1.5), Settings::DefaultThumbnailResolution);
    QCOMPARE(Settings::computeDefaultThumbnailResolution(std::numeric_limits<double>::quiet_NaN()),
             Settings::DefaultThumbnailResolution);
  }

private:
  static void verifyIni(const ValueSet &set) {
    const QSettings ini(confPath(QCoreApplication::applicationName() + u".ini"_s),
                        QSettings::IniFormat);
    QCOMPARE(ini.status(), QSettings::NoError);
    for (const IniValue &entry : set.ini) {
      const QVariant stored = ini.value(entry.key);
      QVERIFY2(stored.isValid(), qPrintable(u"missing key "_s + entry.key));
      if (entry.expected.typeId() == QMetaType::Double) {
        bool ok = false;
        const double number = stored.toDouble(&ok);
        QVERIFY2(ok && std::abs(number - entry.expected.toDouble()) < kFloatTolerance,
                 qPrintable(u"%1 = %2, expected %3"_s.arg(entry.key, stored.toString(),
                                                          entry.expected.toString())));
      } else {
        QVERIFY2(stored.toString() == entry.expected.toString(),
                 qPrintable(u"%1 = %2, expected %3"_s.arg(entry.key, stored.toString(),
                                                          entry.expected.toString())));
      }
    }
    const QSettings theme(confPath(u"theme.ini"_s), QSettings::IniFormat);
    QCOMPARE(theme.value(u"Colors/accent"_s).toString(), set.accent);
  }

  Settings *mSettings = nullptr;
};

int runSettingsValueMappingTests(int argc, char **argv) {
  SettingsValueMappingTests tests;
  return QTest::qExec(&tests, argc, argv);
}

#include "tst_settingsvaluemapping.moc"
