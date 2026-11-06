#include <QCoreApplication>
#include <QTest>

#include "gui/controllers/coldstartwindowcontroller.h"
#include "gui/ports/uievents.h"
#include "gui/ports/viewerport.h"
#include "gui/ports/viewmodeport.h"
#include "gui/ports/windowport.h"
#include "testsuites.h"

namespace {
class FakeWindow final : public IWindowPort {
public:
    void showWindow() override { visible = true; }
    void hideWindow() override { visible = false; }
    [[nodiscard]] bool isWindowVisible() const override { return visible; }
    void setWindowConcealed(bool value) override { concealed = value; }
    void revealWindowAfterComposition() override { concealed = false; }
    [[nodiscard]] bool isWindowMinimized() const override { return false; }
    void raiseAndActivateWindow() override {}
    void saveWindowGeometry() override {}

    bool visible = false;
    bool concealed = false;
};

class FakeViewMode final : public IViewModePort {
public:
    [[nodiscard]] ViewMode currentViewMode() const override { return mode; }
    void enableDocumentView() override { mode = MODE_DOCUMENT; }
    void enableFolderView() override { mode = MODE_FOLDERVIEW; }

    ViewMode mode = MODE_DOCUMENT;
};

class FakeViewer final : public IViewerPort {
public:
    void showImage(std::shared_ptr<const QImage>, const QString &) override {}
    void showAnimation(const QString &, const QString &, QSize) override {}
    void closeImage() override {}
    void showPreview(const QImage &, QSize) override {}
    void setLoopPlayback(bool) override {}
    void showUpscaledCrop(const QImage &, const QRect &) override {}
    void hideUpscaledCrop() override {}
    void refreshScaling() override {}
    [[nodiscard]] QRect visibleOriginalImageRect() const override { return {}; }
    [[nodiscard]] float currentScale() const override { return 1.0F; }
    [[nodiscard]] float devicePixelRatio() const override { return 1.0F; }
    [[nodiscard]] bool isBusyInteracting() const override { return false; }
    [[nodiscard]] bool isRenderingSettled() const override { return settled; }
    [[nodiscard]] bool panoramaMode() const override { return false; }

    bool settled = false;
};
} // namespace

class ColdStartWindowTests : public QObject {
    Q_OBJECT

private slots:
    void documentIsRevealedOnceRenderingSettles() {
        FakeWindow window;
        FakeViewMode viewMode;
        FakeViewer viewer;
        UiEvents events;
        ColdStartWindowController controller(window, viewMode, viewer, events);

        controller.show();
        QVERIFY(window.visible);
        QVERIFY(window.concealed);

        viewer.settled = true;
        emit events.documentRenderingSettled();
        QCoreApplication::processEvents();

        QVERIFY(!window.concealed);
    }

    // The directory listing finishing between the settled document and its
    // scheduled reveal (a small image decodes before the folder is read)
    // must not leave the window concealed.
    void directoryLoadedBeforeTheDocumentRevealKeepsTheReveal() {
        FakeWindow window;
        FakeViewMode viewMode;
        FakeViewer viewer;
        UiEvents events;
        ColdStartWindowController controller(window, viewMode, viewer, events);

        controller.show();
        viewer.settled = true;
        emit events.documentRenderingSettled();
        controller.onDirectoryModelLoaded();
        QCoreApplication::processEvents();

        QVERIFY(!window.concealed);
    }

    // A folder view whose reveal was scheduled waits again for the
    // thumbnails of the reloaded directory, and is revealed once they are
    // ready.
    void directoryLoadedBeforeTheFolderRevealWaitsForThumbnails() {
        FakeWindow window;
        FakeViewMode viewMode;
        viewMode.mode = MODE_FOLDERVIEW;
        FakeViewer viewer;
        UiEvents events;
        ColdStartWindowController controller(window, viewMode, viewer, events);

        controller.show();
        controller.onDirectoryModelLoaded();
        emit events.filesystemViewReady();
        emit events.visibleThumbnailsReady();
        controller.onDirectoryModelLoaded();
        QCoreApplication::processEvents();
        QVERIFY(window.concealed);

        emit events.visibleThumbnailsReady();
        QCoreApplication::processEvents();
        QVERIFY(!window.concealed);
    }

    void failedInitialPathRevealsImmediatelyAfterTheFirstFrame() {
        FakeWindow window;
        FakeViewMode viewMode;
        viewMode.mode = MODE_FOLDERVIEW;
        FakeViewer viewer;
        UiEvents events;
        ColdStartWindowController controller(window, viewMode, viewer, events);

        controller.show();
        QVERIFY(window.visible);
        QVERIFY(window.concealed);

        emit events.firstFramePresented();
        controller.onInitialPathFailed();
        QCoreApplication::processEvents();

        QVERIFY(!window.concealed);
    }

    void failedInitialPathWaitsForFirstFrameBeforeRevealing() {
        FakeWindow window;
        FakeViewMode viewMode;
        viewMode.mode = MODE_FOLDERVIEW;
        FakeViewer viewer;
        UiEvents events;
        ColdStartWindowController controller(window, viewMode, viewer, events);

        controller.show();
        QVERIFY(window.visible);
        QVERIFY(window.concealed);

        controller.onInitialPathFailed();
        QCoreApplication::processEvents();

        // Must still be concealed because the first frame has not been presented
        QVERIFY(window.concealed);

        emit events.firstFramePresented();
        QCoreApplication::processEvents();

        // Now revealed once the first frame has been presented
        QVERIFY(!window.concealed);
    }

    void filesystemViewReadyBeforeWaitForFolderViewRevealsOnceThumbnailsReady() {
        FakeWindow window;
        FakeViewMode viewMode;
        viewMode.mode = MODE_FOLDERVIEW;
        FakeViewer viewer;
        UiEvents events;
        ColdStartWindowController controller(window, viewMode, viewer, events);

        emit events.filesystemViewReady();

        controller.show();
        controller.onDirectoryModelLoaded();
        QVERIFY(window.concealed);

        emit events.visibleThumbnailsReady();
        QCoreApplication::processEvents();

        QVERIFY(!window.concealed);
    }
};

int runColdStartWindowTests(int argc, char **argv) {
    ColdStartWindowTests tests;
    return QTest::qExec(&tests, argc, argv);
}

#include "tst_coldstartwindow.moc"
