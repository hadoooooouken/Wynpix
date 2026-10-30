#include <QSignalSpy>
#include <QTest>

#include "components/actionmanager/actionmanager.h"
#include "components/viewmode/viewmodecontroller.h"
#include "shortcutbuilder.h"
#include "testsuites.h"

// Qt Test suite for UI-independent wynpix components. Test cases are added
// together with the components they cover.
class WynpixTests : public QObject {
  Q_OBJECT

private slots:
  void viewModeController_keepsInitialMode_data();
  void viewModeController_keepsInitialMode();
  void viewModeController_switchesAndNotifies();
  void viewModeController_notifiesRepeatedRequests();
  void actionManager_mergeMissingDefaults_addsNewActions();
  void actionManager_mergeMissingDefaults_preservesCustomActionMapping();
  void actionManager_mergeMissingDefaults_doesNotOverwriteOccupiedKey();
  void actionManager_mergeMissingDefaults_addsMultipleDefaultsForSameAction();
  void actionManager_mergeMissingDefaults_addsAKeyTakenInTheOtherMode();
  void actionManager_actionFor_picksTheActionOfTheMode();
  void actionManager_actionFor_extendsASelectionMoveWithShift();
  void actionManager_removeRetiredDefaults_dropsEnterFullscreen();
  void actionManager_actionFor_fallsBackFromANumpadKey();
  void actionManager_mergeMissingDefaults_keepsEnterFullscreenInTheViewer();
  void inputMap_mainKeyShortcut_data();
  void inputMap_mainKeyShortcut();
  void shortcutBuilder_namesANumpadKeyByNumLock_data();
  void shortcutBuilder_namesANumpadKeyByNumLock();
  void inputMap_keysKeepTheMainNamesOfTheNumpad();
};

void WynpixTests::viewModeController_keepsInitialMode_data() {
  QTest::addColumn<ViewMode>("initialMode");
  QTest::newRow("document") << MODE_DOCUMENT;
  QTest::newRow("folder view") << MODE_FOLDERVIEW;
}

void WynpixTests::viewModeController_keepsInitialMode() {
  QFETCH(ViewMode, initialMode);
  ViewModeController controller(initialMode);
  QCOMPARE(controller.currentViewMode(), initialMode);
}

void WynpixTests::viewModeController_switchesAndNotifies() {
  ViewModeController controller(MODE_DOCUMENT);
  QSignalSpy applied(&controller, &ViewModeController::viewModeApplied);

  ViewMode modeSeenByListener = MODE_DOCUMENT;
  connect(&controller, &ViewModeController::viewModeApplied, this,
          [&controller, &modeSeenByListener](ViewMode) {
            modeSeenByListener = controller.currentViewMode();
          });

  controller.enableFolderView();
  QCOMPARE(controller.currentViewMode(), MODE_FOLDERVIEW);
  QCOMPARE(applied.count(), 1);
  QCOMPARE(applied.at(0).at(0).value<ViewMode>(), MODE_FOLDERVIEW);
  // The state is updated before listeners run.
  QCOMPARE(modeSeenByListener, MODE_FOLDERVIEW);

  controller.enableDocumentView();
  QCOMPARE(controller.currentViewMode(), MODE_DOCUMENT);
  QCOMPARE(applied.count(), 2);
  QCOMPARE(applied.at(1).at(0).value<ViewMode>(), MODE_DOCUMENT);
}

void WynpixTests::viewModeController_notifiesRepeatedRequests() {
  ViewModeController controller(MODE_FOLDERVIEW);
  QSignalSpy applied(&controller, &ViewModeController::viewModeApplied);

  controller.enableFolderView();
  controller.enableFolderView();
  QCOMPARE(controller.currentViewMode(), MODE_FOLDERVIEW);
  QCOMPARE(applied.count(), 2);
}

void WynpixTests::actionManager_mergeMissingDefaults_addsNewActions() {
  const ShortcutMap defaults = {
      {"Ctrl+,", "firstPage"},
      {"Ctrl+.", "lastPage"},
      {"Right", "nextImage"}
  };
  ShortcutMap userShortcuts = {
      {"Right", "nextImage"}
  };

  const bool changed = ShortcutRules::mergeMissingDefaults(defaults, userShortcuts);
  QVERIFY(changed);
  QCOMPARE(userShortcuts.value("Ctrl+,"), QString("firstPage"));
  QCOMPARE(userShortcuts.value("Ctrl+."), QString("lastPage"));
  QCOMPARE(userShortcuts.value("Right"), QString("nextImage"));
  QCOMPARE(userShortcuts.size(), 3);
}

void WynpixTests::actionManager_mergeMissingDefaults_preservesCustomActionMapping() {
  const ShortcutMap defaults = {
      {"Ctrl+,", "firstPage"},
      {"Right", "nextImage"}
  };
  // User mapped firstPage to Alt+, instead of default Ctrl+,
  ShortcutMap userShortcuts = {
      {"Alt+,", "firstPage"},
      {"Right", "nextImage"}
  };

  const bool changed = ShortcutRules::mergeMissingDefaults(defaults, userShortcuts);
  QVERIFY(!changed);
  QVERIFY(!userShortcuts.contains("Ctrl+,"));
  QCOMPARE(userShortcuts.value("Alt+,"), QString("firstPage"));
}

void WynpixTests::actionManager_mergeMissingDefaults_doesNotOverwriteOccupiedKey() {
  const ShortcutMap defaults = {
      {"Ctrl+,", "firstPage"},
      {"Right", "nextImage"}
  };
  // User remapped Ctrl+, to do jumpToFirst
  ShortcutMap userShortcuts = {
      {"Ctrl+,", "jumpToFirst"},
      {"Right", "nextImage"}
  };

  const bool changed = ShortcutRules::mergeMissingDefaults(defaults, userShortcuts);
  QVERIFY(!changed);
  QCOMPARE(userShortcuts.value("Ctrl+,"), QString("jumpToFirst"));
  QVERIFY(userShortcuts.key("firstPage").isEmpty());
}

void WynpixTests::actionManager_mergeMissingDefaults_addsMultipleDefaultsForSameAction() {
  const ShortcutMap defaults = {
      {"F", "toggleFullscreen"},
      {"Enter", "toggleFullscreen"},
      {"Right", "nextImage"}
  };
  // User config lacks toggleFullscreen completely
  ShortcutMap userShortcuts = {
      {"Right", "nextImage"}
  };

  const bool changed = ShortcutRules::mergeMissingDefaults(defaults, userShortcuts);
  QVERIFY(changed);
  QCOMPARE(userShortcuts.value("F"), QString("toggleFullscreen"));
  QCOMPARE(userShortcuts.value("Enter"), QString("toggleFullscreen"));
  QCOMPARE(userShortcuts.size(), 3);
}

void WynpixTests::actionManager_mergeMissingDefaults_addsAKeyTakenInTheOtherMode() {
  // goUp works in the folder view, nextImage in the viewer, save in both.
  const ShortcutMap defaults = {
      {"Right", "goUp"},
      {"Left", "save"}
  };
  ShortcutMap userShortcuts = {
      {"Right", "nextImage"},
      {"Left", "prevImage"}
  };

  const bool changed = ShortcutRules::mergeMissingDefaults(defaults, userShortcuts);
  QVERIFY(changed);
  QCOMPARE(userShortcuts.values("Right").size(), 2);
  QVERIFY(userShortcuts.contains("Right", "goUp"));
  QVERIFY(!userShortcuts.contains("Left", "save"));
}

void WynpixTests::actionManager_actionFor_picksTheActionOfTheMode() {
  const ShortcutMap shortcuts = {
      {"Right", "nextImage"},
      {"Right", "goUp"},
      {"R", "resize"}
  };
  QCOMPARE(ShortcutRules::actionFor(shortcuts, "Right", false), QString("nextImage"));
  QCOMPARE(ShortcutRules::actionFor(shortcuts, "Right", true), QString("goUp"));
  QCOMPARE(ShortcutRules::actionFor(shortcuts, "R", false), QString("resize"));
  QCOMPARE(ShortcutRules::actionFor(shortcuts, "R", true), QString("resize"));
  QVERIFY(ShortcutRules::actionFor(shortcuts, "Left", false).isEmpty());
}

void WynpixTests::actionManager_actionFor_extendsASelectionMoveWithShift() {
  const ShortcutMap shortcuts = {
      {"Left", "folderMoveLeft"},
      {"Shift+Left", "prevDirectory"},
      {"B", "save"}
  };
  // In the folder view the viewer's Shift+Left does not run.
  QCOMPARE(ShortcutRules::actionFor(shortcuts, "Shift+Left", true), QString("folderMoveLeft"));
  QCOMPARE(ShortcutRules::actionFor(shortcuts, "Shift+Left", false), QString("prevDirectory"));
  QVERIFY(ShortcutRules::actionFor(shortcuts, "Shift+B", true).isEmpty());
}

void WynpixTests::actionManager_removeRetiredDefaults_dropsEnterFullscreen() {
  ShortcutMap shortcuts = {
      {"Enter", "toggleFullscreen"},
      {"F", "toggleFullscreen"}
  };
  QVERIFY(ShortcutRules::removeRetiredDefaults(2, shortcuts));
  QVERIFY(!shortcuts.contains("Enter"));
  QVERIFY(shortcuts.contains("F", "toggleFullscreen"));

  ShortcutMap current = {{"Enter", "toggleFullscreen"}};
  QVERIFY(!ShortcutRules::removeRetiredDefaults(ActionManager::kCurrentShortcutsVersion, current));
  QVERIFY(current.contains("Enter", "toggleFullscreen"));
}

void WynpixTests::actionManager_actionFor_fallsBackFromANumpadKey() {
  const ShortcutMap shortcuts = {
      {"1", "fitWindow"},
      {"Num1", "fitWidth"},
      {"Home", "jumpToFirst"},
      {"Ctrl+Home", "firstPage"},
      {"Down", "folderMoveDown"}
  };
  // A numpad key of its own shortcut runs its own action.
  QCOMPARE(ShortcutRules::actionFor(shortcuts, "Num1", false), QString("fitWidth"));
  QCOMPARE(ShortcutRules::actionFor(shortcuts, "1", false), QString("fitWindow"));
  QCOMPARE(ShortcutRules::actionFor(shortcuts, "NumHome", false), QString("jumpToFirst"));
  QCOMPARE(ShortcutRules::actionFor(shortcuts, "Ctrl+NumHome", false), QString("firstPage"));
  QVERIFY(ShortcutRules::actionFor(shortcuts, "Num2", false).isEmpty());
  // Shift extends the selection move of the main keyboard key.
  QCOMPARE(ShortcutRules::actionFor(shortcuts, "Shift+NumDown", true), QString("folderMoveDown"));
}

// Fullscreen is an action of each mode, so Enter can toggle it in the viewer
// and open the selection in the folder view.
void WynpixTests::actionManager_mergeMissingDefaults_keepsEnterFullscreenInTheViewer() {
  const ShortcutMap defaults = {
      {"F", "toggleFullscreen"},
      {"F", "folderToggleFullscreen"},
      {"Enter", "folderOpenSelected"}
  };
  ShortcutMap userShortcuts = {
      {"Enter", "toggleFullscreen"},
      {"Enter", "folderOpenSelected"}
  };

  QVERIFY(ShortcutRules::mergeMissingDefaults(defaults, userShortcuts));
  QVERIFY(userShortcuts.contains("F", "folderToggleFullscreen"));
  QCOMPARE(ShortcutRules::actionFor(userShortcuts, "Enter", false), QString("toggleFullscreen"));
  QCOMPARE(ShortcutRules::actionFor(userShortcuts, "Enter", true), QString("folderOpenSelected"));
  QCOMPARE(ShortcutRules::actionFor(userShortcuts, "F", true), QString("folderToggleFullscreen"));
}

void WynpixTests::inputMap_mainKeyShortcut_data() {
  QTest::addColumn<QString>("shortcut");
  QTest::addColumn<QString>("mainKey");
  QTest::newRow("digit") << "Num7" << "7";
  QTest::newRow("NumLock off") << "NumHome" << "Home";
  QTest::newRow("modifiers") << "Alt+Ctrl+Shift+NumPgDown" << "Alt+Ctrl+Shift+PgDown";
  QTest::newRow("plus") << "Ctrl+Num+" << "Ctrl++";
  QTest::newRow("enter") << "NumEnter" << "Enter";
  QTest::newRow("Num5 with NumLock off") << "NumClear" << "";
  QTest::newRow("NumLock") << "NumLock" << "";
  QTest::newRow("main keyboard") << "Ctrl+7" << "";
}

void WynpixTests::inputMap_mainKeyShortcut() {
  QFETCH(QString, shortcut);
  QFETCH(QString, mainKey);
  QCOMPARE(InputMap::mainKeyShortcut(shortcut), mainKey);
}

void WynpixTests::shortcutBuilder_namesANumpadKeyByNumLock_data() {
  QTest::addColumn<int>("key");
  QTest::addColumn<int>("modifiers");
  QTest::addColumn<quint32>("scanCode");
  QTest::addColumn<QString>("text");
  QTest::addColumn<QString>("shortcut");
  const int keypad = Qt::KeypadModifier;
  QTest::newRow("Num7") << int(Qt::Key_7) << keypad << quint32(71) << "7" << "Num7";
  QTest::newRow("Num7 with NumLock off") << int(Qt::Key_Home) << keypad << quint32(71) << ""
                                         << "NumHome";
  QTest::newRow("decimal comma") << int(Qt::Key_Comma) << keypad << quint32(83) << "," << "Num.";
  QTest::newRow("Num. with NumLock off") << int(Qt::Key_Delete) << keypad << quint32(83) << ""
                                         << "NumDel";
  QTest::newRow("Ctrl+Num1") << int(Qt::Key_1) << (keypad | Qt::ControlModifier) << quint32(79)
                             << "" << "Ctrl+Num1";
  QTest::newRow("Num+") << int(Qt::Key_Plus) << keypad << quint32(78) << "+" << "Num+";
  QTest::newRow("Shift+= types +") << int(Qt::Key_Plus) << int(Qt::ShiftModifier) << quint32(13)
                                   << "+" << "+";
  QTest::newRow("NumEnter") << int(Qt::Key_Enter) << keypad << quint32(284) << "\r" << "NumEnter";
  QTest::newRow("Enter") << int(Qt::Key_Return) << 0 << quint32(28) << "\r" << "Enter";
  QTest::newRow("Home") << int(Qt::Key_Home) << 0 << quint32(327) << "" << "Home";
}

void WynpixTests::shortcutBuilder_namesANumpadKeyByNumLock() {
  QFETCH(int, key);
  QFETCH(int, modifiers);
  QFETCH(quint32, scanCode);
  QFETCH(QString, text);
  QFETCH(QString, shortcut);
  QKeyEvent event(QEvent::KeyPress, key, Qt::KeyboardModifiers::fromInt(modifiers), scanCode, 0,
                  0, text);
  QCOMPARE(ShortcutBuilder::fromEvent(&event), shortcut);
}

// The copy and move overlay picks its folders by these names.
void WynpixTests::inputMap_keysKeepTheMainNamesOfTheNumpad() {
  const auto &keys = InputMap::getInstance()->keys();
  QCOMPARE(keys.value(79), QString("1"));
  QCOMPARE(keys.value(284), QString("Enter"));
}

int runWynpixTests(int argc, char **argv) {
  WynpixTests tests;
  return QTest::qExec(&tests, argc, argv);
}

#include "tst_wynpix.moc"
