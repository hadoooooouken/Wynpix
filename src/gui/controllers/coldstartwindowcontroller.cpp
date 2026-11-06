#include "coldstartwindowcontroller.h"

#include "gui/ports/uievents.h"
#include "gui/ports/viewerport.h"
#include "gui/ports/viewmodeport.h"
#include "gui/ports/windowport.h"
#include "utils/startuptiming.h"

#include <QDebug>

ColdStartWindowController::ColdStartWindowController(
    IWindowPort &window, IViewModePort &viewMode, IViewerPort &viewer,
    UiEvents &events)
    : window(window),
      viewMode(viewMode),
      viewer(viewer) {
    maximumWaitTimer.setSingleShot(true);
    maximumWaitTimer.setInterval(kFolderViewReadinessTimeoutMs);
    documentReadyFallbackTimer.setSingleShot(true);
    documentReadyFallbackTimer.setInterval(kDocumentReadyFallbackMs);

    connect(&events, &UiEvents::visibleThumbnailsReady,
            this, &ColdStartWindowController::onVisibleThumbnailsReady);
    connect(&events, &UiEvents::filesystemViewReady,
            this, &ColdStartWindowController::onFilesystemViewReady);
    connect(&events, &UiEvents::firstFramePresented,
            this, &ColdStartWindowController::onFirstFramePresented);
    connect(&events, &UiEvents::documentRenderingSettled,
            this, &ColdStartWindowController::onDocumentRenderingSettled);
    connect(&documentReadyFallbackTimer, &QTimer::timeout, this, [this]() {
        if(state != State::WaitingForDocumentLayout)
            return;
        qWarning() << "Cold-start document rendering did not settle within"
                   << kDocumentReadyFallbackMs << "ms; revealing the window";
        revealWindow();
    });
    connect(&maximumWaitTimer, &QTimer::timeout, this, [this]() {
        if(state != State::WaitingForFolderView)
            return;
        qWarning() << "Cold-start folder view did not become ready within"
                   << kFolderViewReadinessTimeoutMs << "ms";
        revealWindow();
    });
}

void ColdStartWindowController::show() {
    if(state == State::WaitingForFolderView) {
        if(viewMode.currentViewMode() != MODE_FOLDERVIEW)
            waitForDocumentLayout();
        return;
    }

    if(state == State::WaitingForDocumentLayout) {
        if(viewMode.currentViewMode() == MODE_FOLDERVIEW)
            waitForFolderView();
        else if(viewer.isRenderingSettled())
            onDocumentRenderingSettled();
        else
            documentReadyFallbackTimer.start();
        return;
    }

    if(state == State::RevealScheduled) {
        if(viewMode.currentViewMode() != MODE_FOLDERVIEW &&
           !viewer.isRenderingSettled())
            waitForDocumentLayout();
        return;
    }

    if(state != State::Initial || window.isWindowVisible()) {
        state = State::Shown;
        window.showWindow();
        return;
    }

    if(viewMode.currentViewMode() != MODE_FOLDERVIEW) {
        waitForDocumentLayout();
        return;
    }

    waitForFolderView();
}

void ColdStartWindowController::onDirectoryModelLoaded() {
    if(state == State::Shown)
        return;

    directoryModelLoaded = true;
    visibleThumbnailsReady = false;
    // Only a folder view waits for the thumbnails of the reloaded directory;
    // a scheduled document reveal must still run, or the window stays
    // concealed with nothing left to reveal it.
    if(state == State::RevealScheduled &&
       viewMode.currentViewMode() == MODE_FOLDERVIEW) {
        state = State::WaitingForFolderView;
        if(!maximumWaitTimer.isActive())
            maximumWaitTimer.start();
    }
}

void ColdStartWindowController::onInitialPathFailed() {
    if(state != State::WaitingForFolderView &&
       state != State::WaitingForDocumentLayout &&
       state != State::RevealScheduled)
        return;

    // No readiness signal can follow a path that was not loaded. The window is
    // cloaked to keep its opening animation and early frames off the screen,
    // so it is revealed once the first frame has been presented.
    initialPathFailed = true;
    if(firstFramePresented)
        revealWindow();
}

void ColdStartWindowController::onFirstFramePresented() {
    firstFramePresented = true;
    if(initialPathFailed)
        revealWindow();
}

void ColdStartWindowController::waitForFolderView() {
    documentReadyFallbackTimer.stop();
    state = State::WaitingForFolderView;
    visibleThumbnailsReady = false;
    window.setWindowConcealed(true);
    maximumWaitTimer.start();
    window.showWindow();
}

void ColdStartWindowController::waitForDocumentLayout() {
    maximumWaitTimer.stop();
    state = State::WaitingForDocumentLayout;
    window.setWindowConcealed(true);
    window.showWindow();
    if(state == State::WaitingForDocumentLayout &&
       viewer.isRenderingSettled()) {
        onDocumentRenderingSettled();
    } else if(state == State::WaitingForDocumentLayout) {
        documentReadyFallbackTimer.start();
    }
}

void ColdStartWindowController::onVisibleThumbnailsReady() {
    if(state != State::WaitingForFolderView || !directoryModelLoaded)
        return;

    visibleThumbnailsReady = true;
    tryRevealFolderView();
}

void ColdStartWindowController::onFilesystemViewReady() {
    // A drive root reports the folder tree as ready while its path loads,
    // before show() starts waiting for the folder view, and the report is not
    // repeated: keep it for that wait.
    if(state == State::Initial) {
        filesystemViewReady = true;
        return;
    }
    if(state != State::WaitingForFolderView)
        return;

    filesystemViewReady = true;
    tryRevealFolderView();
}

void ColdStartWindowController::tryRevealFolderView() {
    if(state != State::WaitingForFolderView ||
       !visibleThumbnailsReady || !filesystemViewReady)
        return;

    state = State::RevealScheduled;
    QTimer::singleShot(kLayoutSettleDelayMs, this, [this]() {
        if(state == State::RevealScheduled)
            revealWindow();
    });
}

void ColdStartWindowController::onDocumentRenderingSettled() {
    if(state != State::WaitingForDocumentLayout)
        return;

    documentReadyFallbackTimer.stop();
    state = State::RevealScheduled;
    QTimer::singleShot(kLayoutSettleDelayMs, this, [this]() {
        if(state != State::RevealScheduled)
            return;
        if(viewMode.currentViewMode() == MODE_FOLDERVIEW) {
            waitForFolderView();
            return;
        }
        if(!viewer.isRenderingSettled()) {
            waitForDocumentLayout();
            return;
        }
        revealWindow();
    });
}

void ColdStartWindowController::revealWindow() {
    if(state != State::WaitingForFolderView &&
       state != State::WaitingForDocumentLayout &&
       state != State::RevealScheduled)
        return;

    maximumWaitTimer.stop();
    documentReadyFallbackTimer.stop();
    state = State::Shown;
    window.setWindowConcealed(false);
    logStartupMilestone(u"cold-start window revealed");
}
