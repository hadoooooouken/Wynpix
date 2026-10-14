#include <QSignalSpy>
#include <QTest>

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

int runWynpixTests(int argc, char **argv) {
  WynpixTests tests;
  return QTest::qExec(&tests, argc, argv);
}

#include "tst_wynpix.moc"
