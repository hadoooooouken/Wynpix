#include <QSignalSpy>
#include <QTest>

#include "components/actionmanager/actionmanager.h"
#include "components/viewmode/viewmodecontroller.h"
#include "testsuites.h"

// Qt Test suite for UI-independent wynpix components. Test cases are added
// together with the components they cover (docs/QML_MIGRATION_PLAN.md).
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
  const QMap<QString, QString> defaults = {
      {"Ctrl+Home", "firstPage"},
      {"Ctrl+End", "lastPage"},
      {"Right", "nextImage"}
  };
  QMap<QString, QString> userShortcuts = {
      {"Right", "nextImage"}
  };

  const bool changed = ActionManager::mergeMissingDefaults(defaults, userShortcuts);
  QVERIFY(changed);
  QCOMPARE(userShortcuts.value("Ctrl+Home"), QString("firstPage"));
  QCOMPARE(userShortcuts.value("Ctrl+End"), QString("lastPage"));
  QCOMPARE(userShortcuts.value("Right"), QString("nextImage"));
  QCOMPARE(userShortcuts.size(), 3);
}

void WynpixTests::actionManager_mergeMissingDefaults_preservesCustomActionMapping() {
  const QMap<QString, QString> defaults = {
      {"Ctrl+Home", "firstPage"},
      {"Right", "nextImage"}
  };
  // User mapped firstPage to Alt+Home instead of default Ctrl+Home
  QMap<QString, QString> userShortcuts = {
      {"Alt+Home", "firstPage"},
      {"Right", "nextImage"}
  };

  const bool changed = ActionManager::mergeMissingDefaults(defaults, userShortcuts);
  QVERIFY(!changed);
  QVERIFY(!userShortcuts.contains("Ctrl+Home"));
  QCOMPARE(userShortcuts.value("Alt+Home"), QString("firstPage"));
}

void WynpixTests::actionManager_mergeMissingDefaults_doesNotOverwriteOccupiedKey() {
  const QMap<QString, QString> defaults = {
      {"Ctrl+Home", "firstPage"},
      {"Right", "nextImage"}
  };
  // User remapped Ctrl+Home to do jumpToFirst
  QMap<QString, QString> userShortcuts = {
      {"Ctrl+Home", "jumpToFirst"},
      {"Right", "nextImage"}
  };

  const bool changed = ActionManager::mergeMissingDefaults(defaults, userShortcuts);
  QVERIFY(!changed);
  QCOMPARE(userShortcuts.value("Ctrl+Home"), QString("jumpToFirst"));
  QVERIFY(userShortcuts.key("firstPage").isEmpty());
}

void WynpixTests::actionManager_mergeMissingDefaults_addsMultipleDefaultsForSameAction() {
  const QMap<QString, QString> defaults = {
      {"F", "toggleFullscreen"},
      {"Enter", "toggleFullscreen"},
      {"Right", "nextImage"}
  };
  // User config lacks toggleFullscreen completely
  QMap<QString, QString> userShortcuts = {
      {"Right", "nextImage"}
  };

  const bool changed = ActionManager::mergeMissingDefaults(defaults, userShortcuts);
  QVERIFY(changed);
  QCOMPARE(userShortcuts.value("F"), QString("toggleFullscreen"));
  QCOMPARE(userShortcuts.value("Enter"), QString("toggleFullscreen"));
  QCOMPARE(userShortcuts.size(), 3);
}

int runWynpixTests(int argc, char **argv) {
  WynpixTests tests;
  return QTest::qExec(&tests, argc, argv);
}

#include "tst_wynpix.moc"
