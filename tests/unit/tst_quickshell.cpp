#include <QRegularExpression>
#include <QSignalSpy>
#include <QTest>
#include <QUrl>

#include <limits>

#include "gui/quick/ui/mainwindowshell.h"
#include "testsuites.h"

namespace {
using namespace Qt::StringLiterals;

constexpr qreal halfOpacity = 0.5;
constexpr qreal opaque = 1.0;
constexpr qreal transparent = 0.0;
constexpr qreal aboveOpaque = 1.5;
constexpr qreal belowTransparent = -0.5;
} // namespace

// MainWindowShell (the Quick main window's state and drops).
class QuickShellTests : public QObject {
  Q_OBJECT

private slots:
  void dropsAreForwardedWithTheirSource() {
    MainWindowShell shell;
    QObject source;
    QSignalSpy dropSpy(&shell, &MainWindowShell::urlsDropped);
    const QList<QUrl> urls{QUrl::fromLocalFile(u"C:/images/a.png"_s),
                           QUrl::fromLocalFile(u"C:/images/b.png"_s)};

    shell.dropUrls(urls, &source);
    QCOMPARE(dropSpy.count(), 1);
    QCOMPARE(dropSpy.first().at(0).value<QList<QUrl>>(), urls);
    QCOMPARE(dropSpy.first().at(1).value<QObject *>(), &source);
  }

  void dropsWithoutUrlsAreIgnored() {
    MainWindowShell shell;
    QSignalSpy dropSpy(&shell, &MainWindowShell::urlsDropped);
    QTest::ignoreMessage(QtWarningMsg, "MainWindowShell: a drop without URLs was ignored");
    shell.dropUrls({}, nullptr);
    QCOMPARE(dropSpy.count(), 0);
  }

  void stateChangesAreNotifiedOnce() {
    MainWindowShell shell;
    QSignalSpy folderSpy(&shell, &MainWindowShell::folderViewActiveChanged);
    QSignalSpy fullscreenSpy(&shell, &MainWindowShell::fullscreenChanged);
    shell.setFolderViewActive(true);
    shell.setFolderViewActive(true);
    shell.setFullscreen(true);
    shell.setFullscreen(true);
    QVERIFY(shell.folderViewActive());
    QVERIFY(shell.fullscreen());
    QCOMPARE(folderSpy.count(), 1);
    QCOMPARE(fullscreenSpy.count(), 1);
  }

  void backgroundIsOpaqueUntilAnOpacityIsSet() {
    MainWindowShell shell;
    QCOMPARE(shell.backgroundOpacity(), opaque);
  }

  void windowedOpacityAppliesOnlyInWindowedDocumentMode() {
    MainWindowShell shell;
    QSignalSpy opacitySpy(&shell, &MainWindowShell::backgroundOpacityChanged);
    shell.setWindowedBackgroundOpacity(halfOpacity);
    QCOMPARE(shell.backgroundOpacity(), halfOpacity);
    QCOMPARE(opacitySpy.count(), 1);

    shell.setFullscreen(true);
    QCOMPARE(shell.backgroundOpacity(), opaque);
    QCOMPARE(opacitySpy.count(), 2);
    shell.setFullscreen(false);
    QCOMPARE(shell.backgroundOpacity(), halfOpacity);
    QCOMPARE(opacitySpy.count(), 3);

    shell.setFolderViewActive(true);
    QCOMPARE(shell.backgroundOpacity(), opaque);
    QCOMPARE(opacitySpy.count(), 4);
  }

  void opacityChangesHiddenByFullscreenAreNotNotified() {
    MainWindowShell shell;
    shell.setFullscreen(true);
    QSignalSpy opacitySpy(&shell, &MainWindowShell::backgroundOpacityChanged);
    shell.setWindowedBackgroundOpacity(halfOpacity);
    QCOMPARE(opacitySpy.count(), 0);
    shell.setFullscreen(false);
    QCOMPARE(shell.backgroundOpacity(), halfOpacity);
    QCOMPARE(opacitySpy.count(), 1);
  }

  void translucentWindowChangesAreNotifiedOnce() {
    MainWindowShell shell;
    QVERIFY(!shell.translucentWindow());
    QSignalSpy translucentSpy(&shell, &MainWindowShell::translucentWindowChanged);
    shell.setTranslucentWindow(true);
    shell.setTranslucentWindow(true);
    QVERIFY(shell.translucentWindow());
    QCOMPARE(translucentSpy.count(), 1);
  }

  void windowedOpacityIsClampedAndValidated() {
    MainWindowShell shell;
    shell.setWindowedBackgroundOpacity(belowTransparent);
    QCOMPARE(shell.backgroundOpacity(), transparent);
    shell.setWindowedBackgroundOpacity(aboveOpaque);
    QCOMPARE(shell.backgroundOpacity(), opaque);

    shell.setWindowedBackgroundOpacity(halfOpacity);
    QTest::ignoreMessage(QtWarningMsg,
                         QRegularExpression(u"^MainWindowShell: the window opacity .* is "
                                            u"not finite; the previous value is kept$"_s));
    shell.setWindowedBackgroundOpacity(std::numeric_limits<qreal>::quiet_NaN());
    QCOMPARE(shell.backgroundOpacity(), halfOpacity);
  }
};

int runQuickShellTests(int argc, char **argv) {
  QuickShellTests tests;
  return QTest::qExec(&tests, argc, argv);
}

#include "tst_quickshell.moc"
