#include <QSignalSpy>
#include <QTest>

#include "components/actionmanager/actionmanager.h"
#include "components/viewmode/viewmodecontroller.h"
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

  const bool changed = ActionManager::mergeMissingDefaults(defaults, userShortcuts);
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

  const bool changed = ActionManager::mergeMissingDefaults(defaults, userShortcuts);
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

  const bool changed = ActionManager::mergeMissingDefaults(defaults, userShortcuts);
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

  const bool changed = ActionManager::mergeMissingDefaults(defaults, userShortcuts);
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

  const bool changed = ActionManager::mergeMissingDefaults(defaults, userShortcuts);
  QVERIFY(changed);
  QCOMPARE(userShortcuts.values("Right").size(), 2);
  QVERIFY(userShortcuts.contains("Right", "goUp"));
  QVERIFY(!userShortcuts.contains("Left", "save"));
}

void WynpixTests::actionManager_actionFor_picksTheActionOfTheMode() {
  const ShortcutMap shortcuts = {
      {"Right", "nextImage"},
      {"Right", "goUp"},
      {"F", "toggleFullscreen"}
  };
  QCOMPARE(ActionManager::actionFor(shortcuts, "Right", false), QString("nextImage"));
  QCOMPARE(ActionManager::actionFor(shortcuts, "Right", true), QString("goUp"));
  QCOMPARE(ActionManager::actionFor(shortcuts, "F", false), QString("toggleFullscreen"));
  QCOMPARE(ActionManager::actionFor(shortcuts, "F", true), QString("toggleFullscreen"));
  QVERIFY(ActionManager::actionFor(shortcuts, "Left", false).isEmpty());
}

int runWynpixTests(int argc, char **argv) {
  WynpixTests tests;
  return QTest::qExec(&tests, argc, argv);
}

#include "tst_wynpix.moc"
