/*
 * This is sort-of a main controller of application.
 * It creates and initializes all components, then sets up gui and actions.
 * Most of communication between components go through here.
 *
 */

#include "core.h"
#include "settings.h"
#include <QClipboard>
#include <QDebug>
#include <QDesktopServices>
#include <QDir>
#include <QDrag>
#include <QGuiApplication>
#include <QMimeDatabase>
#include <QProcess>
#include <algorithm>
#include <cstdlib>
#include <QRegularExpression>
#include <QCoreApplication>
#include <QThreadPool>
#include <QTimer>
#include "components/actionmanager/actionmanager.h"
#include "components/fileoptask/fileopcontroller.h"
#include "components/upscaler/upscaledecision.h"
#include "components/upscaler/upscaler.h"
#include "components/upscaler/upscaylresizerunnable.h"
#include "gui/controllers/coldstartwindowcontroller.h"
#include "components/wallpaper/wallpapercontroller.h"
#include "components/mimepayload/mimepayloadmanager.h"
#include "components/imagepreparer/imagepreparer.h"
#include <QColorSpace>
#include <QCursor>
#include <windows.h>
#include <dwmapi.h>
#include <psapi.h>
#include "utils/colormanager.h"
#include "utils/hdrtonemapper.h"
#include "utils/imagelib.h"
#include <QThread>
#include "sourcecontainers/imagestatic.h"

namespace {
constexpr int PreloadDebounceDelayMs = 200;
// A file opened from another folder is shown before that folder is scanned.
constexpr int ModelDelayLoadDelayMs = 40;
constexpr int FolderViewRevealDelayMs = 100;
constexpr int DocumentReadyFallbackMs = 1000;
constexpr int SettledDocumentRevealDelayMs = 0;
// Explorer starts one instance per selected file; their requests arrived up
// to 90 ms apart. The raised window is revealed once requests stop, so it
// shows the last file only.
constexpr qint64 RaiseRequestQuietMs = 150;
constexpr Qt::DropActions SupportedFileDragActions =
    Qt::CopyAction | Qt::MoveAction;

QString wallpaperApplyFailureMessage(const WallpaperApplyResult &result) {
  const QString nativeError = QString::number(result.nativeError);
  switch (result.error) {
  case WallpaperApplyError::StorageDirectoryCreationFailed:
    return QCoreApplication::translate(
        "Core", "Set wallpaper: failed to create application data directory");
  case WallpaperApplyError::RegistryOpenFailed:
    return QCoreApplication::translate(
               "Core",
               "Set wallpaper: failed to open desktop settings (Windows error %1)")
        .arg(nativeError);
  case WallpaperApplyError::WallpaperStyleWriteFailed:
    return QCoreApplication::translate(
               "Core",
               "Set wallpaper: failed to write wallpaper style (Windows error %1)")
        .arg(nativeError);
  case WallpaperApplyError::TileWallpaperWriteFailed:
    return QCoreApplication::translate(
               "Core",
               "Set wallpaper: failed to write wallpaper tiling (Windows error %1)")
        .arg(nativeError);
  case WallpaperApplyError::RegistryCloseFailed:
    return QCoreApplication::translate(
               "Core",
               "Set wallpaper: failed to close desktop settings (Windows error %1)")
        .arg(nativeError);
  case WallpaperApplyError::SystemParametersInfoFailed:
    return QCoreApplication::translate(
               "Core",
               "Set wallpaper: Windows wallpaper update failed (Windows error %1)")
        .arg(nativeError);
  case WallpaperApplyError::None:
    return {};
  }

  return QCoreApplication::translate("Core", "Set wallpaper: unknown error");
}

QString imageSaveFailureMessage(const ImageSaveResult &result) {
  if (!result.retainedBackupPath.isEmpty() &&
      !result.retainedTemporaryPath.isEmpty()) {
    return QCoreApplication::translate(
               "Core",
               "Could not save file. Recovery files were retained:\n"
               "Original backup:\n%1\n\nStaged copy:\n%2")
        .arg(QDir::toNativeSeparators(result.retainedBackupPath),
             QDir::toNativeSeparators(result.retainedTemporaryPath));
  }

  if (!result.retainedTemporaryPath.isEmpty()) {
    return QCoreApplication::translate(
               "Core",
               "Could not save file. A staged recovery copy was retained at:\n%1")
        .arg(QDir::toNativeSeparators(result.retainedTemporaryPath));
  }

  if (!result.retainedBackupPath.isEmpty()) {
    return QCoreApplication::translate(
               "Core",
               "Could not save file. A recovery backup was retained at:\n%1")
        .arg(QDir::toNativeSeparators(result.retainedBackupPath));
  }

  switch (result.error) {
  case ImageSaveError::InvalidSourceImage:
    return QCoreApplication::translate("Core",
                                       "Could not save file: the image is invalid");
  case ImageSaveError::InvalidDestinationPath:
    return QCoreApplication::translate(
        "Core", "Could not save file: the destination path is invalid");
  case ImageSaveError::SourceUnavailable:
    return QCoreApplication::translate(
        "Core", "Could not save file: the source is unavailable");
  case ImageSaveError::TemporaryFileCreationFailed:
    return QCoreApplication::translate(
        "Core", "Could not save file: a temporary file could not be created");
  case ImageSaveError::ImageEncodingFailed:
    return QCoreApplication::translate(
        "Core", "Could not save file: the image could not be encoded");
  case ImageSaveError::TemporaryFileFlushFailed:
    return QCoreApplication::translate(
        "Core", "Could not save file: the temporary file could not be written");
  case ImageSaveError::FileCopyFailed:
    return QCoreApplication::translate("Core",
                                       "Could not save file: the file copy failed");
  case ImageSaveError::CommitFailed:
    return QCoreApplication::translate(
               "Core",
               "Could not save file: the destination could not be replaced (Windows error %1)")
        .arg(result.nativeError);
  case ImageSaveError::RecoveryFailed:
    return QCoreApplication::translate(
               "Core",
               "Could not save file: automatic recovery failed (Windows error %1)")
        .arg(result.nativeError);
  case ImageSaveError::None:
    return {};
  }

  return QCoreApplication::translate("Core", "Could not save file");
}

QString retainedBackupWarningMessage(const ImageSaveResult &result) {
  return QCoreApplication::translate(
             "Core", "File saved, but a backup could not be removed:\n%1")
      .arg(QDir::toNativeSeparators(result.retainedBackupPath));
}

QString mimePayloadFailureMessage(MimePayloadError error) {
  switch (error) {
  case MimePayloadError::ImageUnavailable:
    return QCoreApplication::translate(
        "Core", "Could not prepare image transfer: the image is unavailable");
  case MimePayloadError::SourcePathUnavailable:
    return QCoreApplication::translate(
        "Core",
        "Could not prepare image transfer: the source path is unavailable");
  case MimePayloadError::StorageRootUnavailable:
    return QCoreApplication::translate(
        "Core",
        "Could not prepare image transfer: temporary storage is unavailable");
  case MimePayloadError::PayloadDirectoryCreationFailed:
    return QCoreApplication::translate(
        "Core",
        "Could not prepare image transfer: the temporary directory could not "
        "be created");
  case MimePayloadError::PayloadFileCreationFailed:
    return QCoreApplication::translate(
        "Core",
        "Could not prepare image transfer: the temporary file could not be "
        "created");
  case MimePayloadError::ImageEncodingFailed:
    return QCoreApplication::translate(
        "Core", "Could not prepare image transfer: the image could not be encoded");
  case MimePayloadError::PayloadCommitFailed:
    return QCoreApplication::translate(
        "Core",
        "Could not prepare image transfer: the temporary file could not be "
        "written");
  case MimePayloadError::None:
    return {};
  }

  return QCoreApplication::translate(
      "Core", "Could not prepare image transfer");
}
}

Core::Core(const UiPorts &ports)
    : ui(ports) {
  initComponents();
  connectComponents();
  connectUiEvents();
  initActions();
  readSettings();
  lastCMEnabled = settings->colorManagementEnabled();
  lastCMType = settings->monitorColorProfileType();
  lastCMPath = settings->monitorColorProfilePath();
  lastHdrEnabled = settings->hdrToneMappingEnabled();
  lastHdrOp = settings->hdrToneMappingOperator();
  lastHdrWhite = settings->hdrTargetWhiteLevel();
  lastThumbnailResolution = settings->thumbnailResolution();
  lastShowSubfoldersInPanel = settings->showSubfoldersInPanel();
  lastShowHiddenFiles = settings->showHiddenFiles();
  lastPanelPreviewsSize = settings->panelPreviewsSize();
  lastSortFolders = settings->sortFolders();
  lastFolderIconSortingMode = settings->folderIconSortingMode();
  lastThumbPanelStyle = settings->thumbPanelStyle();
  slideshowTimer.setSingleShot(true);
  preloadTimer.setSingleShot(true);
  m_raiseWindowRevealTimer.setSingleShot(true);
  connect(&m_raiseWindowRevealTimer, &QTimer::timeout, this, [this]() {
    const qint64 sinceRequest = m_lastRaiseRequestTimer.elapsed();
    if (sinceRequest < RaiseRequestQuietMs) {
      m_raiseWindowRevealTimer.start(static_cast<int>(RaiseRequestQuietMs - sinceRequest));
      return;
    }
    if (m_raiseWindowDocumentRenderingSettled &&
        !ui.viewer.isRenderingSettled()) {
      m_raiseWindowDocumentRenderingSettled = false;
      m_raiseWindowAwaitingDocumentRendering = true;
      m_raiseWindowRevealTimer.start(DocumentReadyFallbackMs);
      return;
    }
    if (m_raiseWindowAwaitingDocumentRendering) {
      qWarning() << "Raised document rendering did not settle within"
                 << DocumentReadyFallbackMs << "ms; revealing the window";
    }
    m_raiseWindowAwaitingDocumentRendering = false;
    m_raiseWindowDocumentRenderingSettled = false;
    m_raiseWindowConcealed = false;
    // A presented frame reaches the screen with the next DWM composition;
    // uncloaking before it can show the window's previous frame once.
    if (FAILED(DwmFlush()))
      qWarning() << "DwmFlush failed; revealing the raised window anyway";
    ui.window.setWindowConcealed(false);
  });
  connect(&ui.events, &UiEvents::documentRenderingSettled, this, [this]() {
    if (!m_raiseWindowConcealed ||
        !m_raiseWindowAwaitingDocumentRendering) {
      return;
    }

    m_raiseWindowDocumentRenderingSettled = true;
    m_raiseWindowAwaitingDocumentRendering = false;
    m_raiseWindowRevealTimer.start(SettledDocumentRevealDelayMs);
  });

  upscaler = std::make_unique<Upscaler>(this);
  // Initial load: no active image/UI state yet to refresh, so the
  // modelChanged result is not actionable here (unlike in Core::readSettings()).
  (void)upscaler->readSettings();
  wallpaperController = std::make_unique<WallpaperController>(this);
  connect(wallpaperController.get(),
          &WallpaperController::notificationRequested,
          this,
          [this](const NotificationRequest &request) {
            ui.notifications.showNotification(request);
          });
  mimePayloadManager =
      std::make_unique<MimePayloadManager>(settings->tmpDir(), this);
  connect(mimePayloadManager.get(),
          &MimePayloadManager::payloadCleanupFailed,
          this,
          [this](const QString &path) {
            ui.notifications.showError(
                tr("Failed to clean up temporary image-transfer data: %1")
                    .arg(QDir::toNativeSeparators(path)));
          });
  imagePreparer = std::make_unique<ImagePreparer>(this);
  connect(imagePreparer.get(), &ImagePreparer::busyChanged, this,
          [](bool busy) {
            if (busy)
              QGuiApplication::setOverrideCursor(Qt::BusyCursor);
            else
              QGuiApplication::restoreOverrideCursor();
          });
  connect(wallpaperController.get(),
          &WallpaperController::wallpaperApplyFinished,
          this,
          [this](const WallpaperApplyResult &result) {
            if (result.succeeded()) {
              ui.notifications.showSuccess(tr("Wallpaper set"));
              return;
            }

            ui.notifications.showError(wallpaperApplyFailureMessage(result));
          },
          Qt::QueuedConnection);
  connect(wallpaperController.get(),
          &WallpaperController::wallpaperFileCleanupFailed,
          this,
          [this](const QString &path) {
            ui.notifications.showError(
                tr("Set wallpaper: failed to clean up wallpaper file: %1")
                    .arg(QDir::toNativeSeparators(path)));
          },
          Qt::QueuedConnection);
  connect(upscaler.get(), &Upscaler::upscaleStarted, this, [this]() {
      ui.notifications.showAiUpscale(tr("AI Upscaling..."), 3600000);
  });
  connect(upscaler.get(), &Upscaler::upscaleFinished, this,
      [this](const UpscaleResult &result) {
          ui.notifications.hideNotifications();
          if (ui.viewer.panoramaMode()) {
              ui.viewer.hideUpscaledCrop();
              return;
          }
          if (!state.hasActiveImage || result.path != state.currentFilePath)
              return;
          // The path alone accepts a crop cut before an edit of this file;
          // the image is looked up as onUpscaleRequested() does.
          const std::shared_ptr<Image> image = model->getImage(result.path);
          if (image && result.isCutFrom(image->getDecodedImage()))
              ui.viewer.showUpscaledCrop(result.crop, result.sourceRect);
      });
  connect(upscaler.get(), &Upscaler::upscaleAborted, this,
          [this]() { ui.notifications.hideNotifications(); });
  connect(upscaler.get(), &Upscaler::upscaleFailed, this, [this](const QString &error) {
      ui.notifications.hideNotifications();
      ui.notifications.showError(error);
  });
  connect(upscaler.get(), &Upscaler::previewInvalidated, this,
          [this]() { ui.viewer.hideUpscaledCrop(); });
  connect(&ui.events, &UiEvents::upscaleWithdrawn, upscaler.get(),
          &Upscaler::cancelPendingPreview);

  connect(upscaler.get(), &Upscaler::requestUpscaleParams, this, [this](const QString &path, bool *ok, QRect *visibleRect, double *currentScale, double *dpr) {
      if (!state.hasActiveImage || path != state.currentFilePath || ui.viewer.panoramaMode() || ui.viewer.isBusyInteracting()) {
          *ok = false;
          return;
      }
      *ok = true;
      *visibleRect = ui.viewer.visibleOriginalImageRect();
      *currentScale = ui.viewer.currentScale();
      *dpr = ui.viewer.devicePixelRatio();
  });

  // Off-GUI-thread copy/move (see components/fileoptask/). model is a
  // stable DirectoryModel instance for Core's whole lifetime (attachModel()
  // is only ever called once, from initComponents()), so handing its raw
  // pointer to FileOpController here is safe.
  fileOpController =
      std::make_unique<FileOpController>(model.get(), ui.dialogs, this);
  connect(fileOpController.get(), &FileOpController::progress, this,
          &Core::onFileOpProgress);
  connect(fileOpController.get(), &FileOpController::finished, this,
          &Core::onFileOpFinished);
  connect(fileOpController.get(), &FileOpController::operationFailed, this,
          [this](const QString &message) { ui.notifications.showError(message); });

  QVersionNumber lastVersion = settings->lastVersion();
  if (settings->firstRun())
    onFirstRun();
  else if (appVersion > lastVersion)
    onUpdate();
}

Core::~Core() = default;

void Core::onSettingsChanged() {
  bool cmEnabled = settings->colorManagementEnabled();
  QString cmType = settings->monitorColorProfileType();
  QString cmPath = settings->monitorColorProfilePath();
  bool hdrEnabled = settings->hdrToneMappingEnabled();
  int hdrOp = settings->hdrToneMappingOperator();
  int hdrWhite = settings->hdrTargetWhiteLevel();

  bool cmChanged = (cmEnabled != lastCMEnabled) ||
                   (cmType != lastCMType) ||
                   (cmPath != lastCMPath);
  bool hdrChanged = (hdrEnabled != lastHdrEnabled) ||
                    (hdrOp != lastHdrOp) ||
                    (hdrWhite != lastHdrWhite);

  if (cmChanged || hdrChanged) {
      lastCMEnabled = cmEnabled;
      lastCMType = cmType;
      lastCMPath = cmPath;
      lastHdrEnabled = hdrEnabled;
      lastHdrOp = hdrOp;
      lastHdrWhite = hdrWhite;

      ColorManager::invalidateCache();
      // No reload on a tone mapping change: the viewer tone maps the
      // decoded pixels with the current settings and DecodedPixels
      // rebuilds the SDR copy for new params, while a reload would drop
      // unsaved edits. Edits stay as baked; Discard shows the new mapping.
      if (state.hasActiveImage && state.currentImg) {
          guiSetImage(state.currentImg);
          updateInfoString();
      }
  }

  int newThumbnailResolution = settings->thumbnailResolution();
  bool newShowSubfoldersInPanel = settings->showSubfoldersInPanel();
  bool newShowHiddenFiles = settings->showHiddenFiles();
  int newPanelPreviewsSize = settings->panelPreviewsSize();
  bool newSortFolders = settings->sortFolders();
  SortingMode newFolderIconSortingMode = settings->folderIconSortingMode();
  ThumbPanelStyle newThumbPanelStyle = settings->thumbPanelStyle();

  const bool thumbnailResolutionChanged =
      newThumbnailResolution != lastThumbnailResolution;
  bool layoutChanged = thumbnailResolutionChanged ||
                       (newShowSubfoldersInPanel != lastShowSubfoldersInPanel) ||
                       (newShowHiddenFiles != lastShowHiddenFiles) ||
                       (newPanelPreviewsSize != lastPanelPreviewsSize) ||
                       (newSortFolders != lastSortFolders) ||
                       (newFolderIconSortingMode != lastFolderIconSortingMode) ||
                       (newThumbPanelStyle != lastThumbPanelStyle);

  if (thumbnailResolutionChanged && !thumbnailer->clearCache()) {
      ui.notifications.showError(tr("Failed to clear thumbnail cache"));
  }

  if (layoutChanged) {
      bool folderSortingChanged = (newFolderIconSortingMode != lastFolderIconSortingMode);

      lastThumbnailResolution = newThumbnailResolution;
      lastShowSubfoldersInPanel = newShowSubfoldersInPanel;
      lastShowHiddenFiles = newShowHiddenFiles;
      lastPanelPreviewsSize = newPanelPreviewsSize;
      lastSortFolders = newSortFolders;
      lastFolderIconSortingMode = newFolderIconSortingMode;
      lastThumbPanelStyle = newThumbPanelStyle;

      if (folderSortingChanged) {
          ui.shell.notifyFolderSortingChanged(newFolderIconSortingMode);
      }
  }

  if (hdrChanged || layoutChanged) {
      // Re-request thumbnails for whatever is currently on screen so
      // already-rendered folder view / thumbnail panel icons pick up
      // the new tone-map settings without requiring the folder to be
      // re-entered. This does not touch the on-disk cache itself -
      // ThumbnailerRunnable's own staleness check (see
      // isToneMapCacheStale) decides per item, on this re-request,
      // whether a fresh decode is actually needed.
      // setShowDirs() in readSettings() repopulates the panel itself.
      if (thumbPanelPresenter.showDirs() == newShowSubfoldersInPanel)
          thumbPanelPresenter.reloadModel();
      folderViewPresenter.reloadModel();
  }

  readSettings();
}

void Core::readSettings() {
  if (upscaler) {
      const bool upscaylModelChanged = upscaler->readSettings();
      if (!settings->useUpscayl()) {
          upscaler->reset();
          ui.viewer.hideUpscaledCrop();
      } else if (upscaylModelChanged) {
          ui.viewer.refreshScaling();
      }
  }
  loopSlideshow = settings->loopSlideshow();
  folderEndAction = settings->folderEndAction();
  slideshowTimer.setInterval(settings->slideshowInterval());

  if (thumbPanelPresenter.showDirs() != settings->showSubfoldersInPanel())
    thumbPanelPresenter.setShowDirs(settings->showSubfoldersInPanel());

  if (shuffle)
    syncRandomizer();
}

void Core::showGui() {
    if (coldStartWindowController) {
      coldStartWindowController->show();
      return;
    }

    qWarning() << "Cold-start window controller is unavailable; showing the"
                  " window immediately";
    ui.window.showWindow();
}

void Core::raiseWindow(const QString &pathReceived) {
  m_lastRaiseRequestTimer.start();

  // The window may currently be hidden (standby) or minimized. Bringing
  // it back involves several native Win32 calls further down (SW_RESTORE,
  // a TOPMOST/NOTOPMOST toggle to steal focus, SetForegroundWindow...).
  // On Windows, SetWindowPos(..., SWP_SHOWWINDOW, ...) right after a
  // hide/minimize can make the OS present the window's raw backing
  // surface for a frame before Qt/DWM has painted real content over it,
  // which is what shows up as a brief white flash. Keeping the window
  // fully transparent for the duration of that dance and only revealing
  // it once everything has settled hides that frame regardless of its
  // cause (same technique ColdStartWindowController uses on first launch).
  // Qt counts a minimized window as visible, but SW_RESTORE shows its last
  // frame (the previous image) before the new one is presented.
  HWND hwnd = reinterpret_cast<HWND>(ui.window.nativeWindowHandle());
  const bool needsDelayedReveal =
      !ui.window.isWindowVisible() || IsIconic(hwnd) || m_raiseWindowConcealed;
  if (needsDelayedReveal) {
    m_raiseWindowRevealTimer.stop();
    m_raiseWindowAwaitingDocumentRendering = false;
    m_raiseWindowDocumentRenderingSettled = false;
    m_raiseWindowConcealed = true;
    ui.window.setWindowConcealed(true);
  }

  processRaiseWindowRequest(pathReceived);

  showGui();

  if (IsIconic(hwnd)) {
    ShowWindow(hwnd, SW_RESTORE);
  }

  SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
  SetWindowPos(hwnd, HWND_NOTOPMOST, 0, 0, 0, 0,
               SWP_SHOWWINDOW | SWP_NOMOVE | SWP_NOSIZE);

  SetForegroundWindow(hwnd);
  SetActiveWindow(hwnd);
  ui.window.raiseAndActivateWindow();

  if (needsDelayedReveal) {
    const bool waitsForDocumentRendering =
        ui.viewMode.currentViewMode() == MODE_DOCUMENT;
    m_raiseWindowAwaitingDocumentRendering = waitsForDocumentRendering;
    if (waitsForDocumentRendering && ui.viewer.isRenderingSettled())
      m_raiseWindowDocumentRenderingSettled = true;
    if (!waitsForDocumentRendering) {
      m_raiseWindowDocumentRenderingSettled = false;
      m_raiseWindowRevealTimer.start(FolderViewRevealDelayMs);
    } else if (m_raiseWindowDocumentRenderingSettled) {
      m_raiseWindowAwaitingDocumentRendering = false;
      m_raiseWindowRevealTimer.start(SettledDocumentRevealDelayMs);
    } else {
      m_raiseWindowRevealTimer.start(DocumentReadyFallbackMs);
    }
  }
}

void Core::processRaiseWindowRequest(const QString &pathReceived) {

  if (m_resumeFromStandby) {
      m_resumeFromStandby = false;
      if (!pathReceived.isEmpty()) {
          loadPath(pathReceived);
      } else if (!hasActiveState()) {
          loadDefaultPath();
      } else {
          if (m_lastViewMode == MODE_FOLDERVIEW) {
              ui.viewMode.enableFolderView();
          } else {
              ui.viewMode.enableDocumentView();
          }
          if (m_lastViewMode == MODE_DOCUMENT && !m_lastFilePath.isEmpty()) {
              loadPath(m_lastFilePath);
          }
      }
  } else {
      if (!pathReceived.isEmpty()) {
          loadPath(pathReceived);
      } else if (!hasActiveState()) {
          loadDefaultPath();
      }
  }
}

void Core::attachModel(DirectoryModel *_model) {
  model.reset(_model);
  thumbPanelPresenter.setModel(model);
  folderViewPresenter.setModel(model);
  bool showDirs = true;
  folderViewPresenter.setShowDirs(showDirs);
  if (shuffle)
    syncRandomizer();
}

void Core::initComponents() {
  // One Thumbnailer shared by both presenters so opening a folder queues
  // each thumbnail request once instead of racing two separate decodes -
  // must happen before attachModel() triggers the first populateView().
  thumbnailer = std::make_shared<Thumbnailer>();
  thumbPanelPresenter.setThumbnailer(thumbnailer);
  folderViewPresenter.setThumbnailer(thumbnailer);
  attachModel(new DirectoryModel());
  coldStartWindowController = std::make_unique<ColdStartWindowController>(
      ui.window, ui.viewMode, ui.viewer, ui.events);
}

void Core::connectComponents() {
  thumbPanelPresenter.setView(ui.thumbnailPanelView);
  connect(&thumbPanelPresenter, &DirectoryPresenter::fileActivated, this,
          &Core::onDirectoryViewFileActivated);
  connect(&thumbPanelPresenter, &DirectoryPresenter::filesActivated, this,
          &Core::onDirectoryViewFilesActivated);
  connect(&thumbPanelPresenter, &DirectoryPresenter::dirActivated, this,
          &Core::onThumbPanelDirActivated);
  connect(&thumbPanelPresenter, &DirectoryPresenter::backRequested, this,
          &Core::historyBack);
  connect(&thumbPanelPresenter, &DirectoryPresenter::forwardRequested, this,
          &Core::historyForward);
  connect(&thumbPanelPresenter, &DirectoryPresenter::selectionExpansionFailed,
          this, [this](const QString &error) { ui.notifications.showError(error); });

  folderViewPresenter.setView(ui.folderView);
  connect(&folderViewPresenter, &DirectoryPresenter::fileActivated, this,
          &Core::onDirectoryViewFileActivated);
  connect(&folderViewPresenter, &DirectoryPresenter::filesActivated, this,
          &Core::onDirectoryViewFilesActivated);
  connect(&folderViewPresenter, &DirectoryPresenter::dirActivated, this,
          &Core::loadPath);
  connect(&folderViewPresenter, &DirectoryPresenter::backRequested, this,
          &Core::historyBack);
  connect(&folderViewPresenter, &DirectoryPresenter::forwardRequested, this,
          &Core::historyForward);

  connect(&folderViewPresenter, &DirectoryPresenter::draggedOut, this,
          qOverload<QList<QString>>(&Core::onDraggedOut));

  connect(&folderViewPresenter, &DirectoryPresenter::droppedInto, this,
          &Core::onDirectoryPresenterDroppedInto);

  connect(&folderViewPresenter, &DirectoryPresenter::expandedSelectedPathsReady, this,
          &Core::onBatchConverterPathsReady);
  connect(&folderViewPresenter, &DirectoryPresenter::selectionExpansionFailed,
          this, [this](const QString &error) { ui.notifications.showError(error); });

  connect(scriptManager, &ScriptManager::error, this,
          [this](const QString &error) { ui.notifications.showError(error); });

  connect(settings, &Settings::settingsChanged, this, &Core::onSettingsChanged);

  connect(model.get(), &DirectoryModel::fileAdded, this, &Core::onFileAdded);
  connect(model.get(), &DirectoryModel::fileRemoved, this,
          &Core::onFileRemoved);
  connect(model.get(), &DirectoryModel::fileRenamed, this,
          &Core::onFileRenamed);
  connect(model.get(), &DirectoryModel::loaded, this, &Core::onModelLoaded);
  connect(model.get(), &DirectoryModel::imageReady, this,
          &Core::onModelItemReady);
  connect(model.get(), &DirectoryModel::imageUpdated, this,
          &Core::onModelItemUpdated);
  connect(model.get(), &DirectoryModel::sortingChanged, this,
          &Core::onModelSortingChanged);
  connect(model.get(), &DirectoryModel::loadFailed, this, &Core::onLoadFailed);

  connect(&slideshowTimer, &QTimer::timeout, this, &Core::nextImageSlideshow);
  connect(&preloadTimer, &QTimer::timeout, this, &Core::preloadNeighbors);
}

void Core::connectUiEvents() {
  UiEvents *events = &ui.events;
  connect(events, &UiEvents::pathOpened, this, &Core::loadPath);
  connect(events, &UiEvents::droppedIn, this, &Core::onDropIn);
  connect(events, &UiEvents::copyRequested, this, &Core::copyCurrentFile);
  connect(events, &UiEvents::moveRequested, this, &Core::moveCurrentFile);
  connect(events, &UiEvents::copyUrlsRequested, this,
          qOverload<QList<QString>, QString>(&Core::copyPathsTo));
  connect(events, &UiEvents::moveUrlsRequested, this, &Core::movePathsTo);
  connect(events, &UiEvents::cropRequested, this, &Core::crop);
  connect(events, &UiEvents::cropAndSaveRequested, this, &Core::cropAndSave);
  connect(events, &UiEvents::colorAdjustmentsApplyRequested, this,
          &Core::applyColorAdjustments);
  connect(events, &UiEvents::saveAsRequested, this, &Core::requestSavePath);
  connect(events, &UiEvents::saveRequested, this, &Core::saveCurrentFile);
  connect(events, &UiEvents::clearThumbnailCacheRequested, this,
          &Core::onClearThumbnailCacheRequested);
  connect(events, &UiEvents::batchConversionRequested, this,
          &Core::showBatchConverter);
  connect(events, &UiEvents::batchAddRequested, this,
          &Core::addToBatchConverter);
  connect(events, &UiEvents::renameRequested, this,
          &Core::renameCurrentSelection);
  connect(events, &UiEvents::sortingSelected, this, &Core::sortBy);
  connect(events, &UiEvents::folderSortingSelected, this,
          &Core::onFolderSortingSelected);
  connect(events, &UiEvents::formatFilterSelected, this,
          &Core::onFormatFilterSelected);
  connect(events, &UiEvents::nameFilterSelected, this,
          &Core::onNameFilterSelected);
  connect(events, &UiEvents::discardEditsRequested, this, &Core::discardEdits);
  connect(events, &UiEvents::draggedOut, this,
          qOverload<>(&Core::onDraggedOut));
  connect(events, &UiEvents::nextImageRequested, this, &Core::nextImage);
  connect(events, &UiEvents::prevImageRequested, this, &Core::prevImage);
  connect(events, &UiEvents::pageRequested, this, &Core::goToPage);
  connect(events, &UiEvents::playbackFinished, this,
          &Core::onPlaybackFinished);
  connect(events, &UiEvents::playbackFailed, this, &Core::onPlaybackFailed);
  connect(events, &UiEvents::upscaleRequested, this, &Core::onUpscaleRequested);
  connect(events, &UiEvents::suspendRequested, this, &Core::suspendToStandby);
}

void Core::initActions() {
  connect(actionManager, &ActionManager::nextImage, this, &Core::nextImage);
  connect(actionManager, &ActionManager::prevImage, this, &Core::prevImage);
  connect(actionManager, &ActionManager::resize, this, &Core::showResizeDialog);
  connect(actionManager, &ActionManager::flipH, this, &Core::flipH);
  connect(actionManager, &ActionManager::flipV, this, &Core::flipV);
  connect(actionManager, &ActionManager::rotateLeft, this, &Core::rotateLeft);
  connect(actionManager, &ActionManager::rotateRight, this, &Core::rotateRight);
  connect(actionManager, &ActionManager::nextPage, this, &Core::nextPage);
  connect(actionManager, &ActionManager::prevPage, this, &Core::prevPage);
  connect(actionManager, &ActionManager::firstPage, this, &Core::firstPage);
  connect(actionManager, &ActionManager::lastPage, this, &Core::lastPage);
  connect(actionManager, &ActionManager::crop, this, &Core::toggleCropPanel);
  connect(actionManager, &ActionManager::setWallpaper, this,
          &Core::setWallpaper);
  connect(actionManager, &ActionManager::save, this, &Core::saveCurrentFile);
  connect(actionManager, &ActionManager::saveAs, this, &Core::requestSavePath);
  connect(actionManager, &ActionManager::exit, this, &Core::forceExit);
  connect(actionManager, &ActionManager::removeFile, this,
          &Core::removePermanent);
  connect(actionManager, &ActionManager::moveToTrash, this, &Core::moveToTrash);
  connect(actionManager, &ActionManager::folderBack, this, &Core::historyBack);
  connect(actionManager, &ActionManager::folderForward, this, &Core::historyForward);
  connect(actionManager, &ActionManager::jumpToFirst, this, &Core::jumpToFirst);
  connect(actionManager, &ActionManager::jumpToLast, this, &Core::jumpToLast);
  connect(actionManager, &ActionManager::runScript, this, &Core::runScript);
  connect(actionManager, &ActionManager::folderView, this,
          &Core::enableFolderView);
  connect(actionManager, &ActionManager::documentView, this,
          &Core::enableDocumentView);
  connect(actionManager, &ActionManager::toggleFolderView, this,
          &Core::toggleFolderView);
  connect(actionManager, &ActionManager::reloadImage, this,
          qOverload<>(&Core::reloadImage));
  connect(actionManager, &ActionManager::copyFileClipboard, this,
          &Core::copyFileClipboard);
  connect(actionManager, &ActionManager::copyPathClipboard, this,
          &Core::copyPathClipboard);
  connect(actionManager, &ActionManager::renameFile, this,
          &Core::showRenameDialog);
  connect(actionManager, &ActionManager::sortByName, this, &Core::sortByName);
  connect(actionManager, &ActionManager::sortByTime, this, &Core::sortByTime);
  connect(actionManager, &ActionManager::sortBySize, this, &Core::sortBySize);
  connect(actionManager, &ActionManager::toggleShuffle, this,
          &Core::toggleShuffle);
  connect(actionManager, &ActionManager::showInDirectory, this,
          &Core::showInDirectory);
  connect(actionManager, &ActionManager::createDirectory, this,
          &Core::createDirectory);
  connect(actionManager, &ActionManager::toggleSlideshow, this,
          &Core::toggleSlideshow);
  connect(actionManager, &ActionManager::goUp, this, &Core::loadParentDir);
  connect(actionManager, &ActionManager::discardEdits, this,
          &Core::discardEdits);
  connect(actionManager, &ActionManager::nextDirectory, this,
          &Core::nextDirectory);
  connect(actionManager, &ActionManager::prevDirectory, this,
          qOverload<>(&Core::prevDirectory));
  connect(actionManager, &ActionManager::print, this, &Core::print);
  connect(actionManager, &ActionManager::toggleFullscreenInfoBar, this,
          &Core::toggleFullscreenInfoBar);
  connect(actionManager, &ActionManager::pasteFile, this,
          &Core::openFromClipboard);
}

void Core::onUpdate() {
  qDebug() << "Updated: " << settings->lastVersion().toString() << ">"
           << appVersion.toString();
  ui.notifications.showMessage(tr("Updated: ") + settings->lastVersion().toString() + " > " +
                      appVersion.toString(),
                  4000);
  settings->setLastVersion(appVersion);
}

void Core::onFirstRun() {
  ui.notifications.showMessage(tr("Welcome to ") + QCoreApplication::applicationName() +
                      tr(" version ") + appVersion.toString() + "!",
                  4000);

  settings->setScalingFilter(QI_FILTER_MKS2021_GPU);
  settings->setImageFitMode(FIT_WINDOW);
  settings->setBackgroundOpacity(0.8);
  settings->setThumbnailOpacity(0.6);

  // Set default thumbnailer threads on first run
  int defaultCount = std::clamp(QThread::idealThreadCount() / 2, Settings::MinThumbnailerThreads, Settings::MaxThumbnailerThreads);
  settings->setThumbnailerThreadCount(defaultCount);

  settings->setFirstRun(false);
  settings->setLastVersion(appVersion);
}

void Core::toggleShuffle() {
  if (shuffle) {
    ui.notifications.showMessage(tr("Shuffle mode: OFF"));
  } else {
    syncRandomizer();
    ui.notifications.showMessage(tr("Shuffle mode: ON"));
  }
  shuffle = !shuffle;
  updateInfoString();
}

void Core::toggleSlideshow() {
  if (slideshow) {
    stopSlideshow();
    ui.notifications.showMessage(tr("Slideshow: OFF"));

  } else {
    startSlideshow();
    ui.notifications.showMessage(tr("Slideshow: ON"));
  }
}

void Core::startSlideshow() {
  if (!slideshow) {
    slideshow = true;
    ui.viewer.setLoopPlayback(false);
    enableDocumentView();
    startSlideshowTimer();
    updateInfoString();
  }
}

void Core::stopSlideshow() {
  if (slideshow) {
    slideshow = false;
    ui.viewer.setLoopPlayback(true);
    slideshowTimer.stop();
    updateInfoString();
  }
}

void Core::onPlaybackFinished() {
  if (slideshow) {
    nextImageSlideshow();
  }
}

void Core::onPlaybackFailed() {
  // A broken animation never finishes: it stays for the slideshow interval
  // like a static image. The timer also keeps the advance out of the
  // synchronous guiSetImage() call that reported an open failure.
  if (slideshow) {
    slideshowTimer.start();
  }
}

void Core::syncRandomizer() {
  if (model) {
    randomizer.setCount(model->fileCount());
    randomizer.shuffle();
    randomizer.setCurrent(model->indexOfFile(state.currentFilePath));
  }
}

void Core::onModelLoaded() {
  if (coldStartWindowController)
    coldStartWindowController->onDirectoryModelLoaded();
  thumbPanelPresenter.reloadModel();
  folderViewPresenter.reloadModel();
  thumbPanelPresenter.selectAndFocus(state.currentFilePath);
  folderViewPresenter.selectAndFocus(state.currentFilePath);
  if (!pendingFolderViewSelectPath.isEmpty()) {
    // Now that the directory has actually finished loading, restore
    // selection/scroll to the folder we navigated up/back from instead of
    // leaving the view on the first item.
    folderViewPresenter.selectAndFocus(pendingFolderViewSelectPath);
    pendingFolderViewSelectPath.clear();
  }
  // A subfolder or adjacent folder was activated in document mode: the async
  // directory scan has finished, so we can now load the first or last image
  // (if any) while staying in document mode. If the folder contains no images
  // the strip will show its subfolders (when the setting is on) and the main
  // view simply keeps whatever was displayed before.
  // Guard with a view-mode check: if the user manually switched to folder
  // view during the scan, honour that choice instead of forcing an image.
  if (m_pendingDocumentLoad != PendingDocumentLoad::None) {
    auto target = m_pendingDocumentLoad;
    m_pendingDocumentLoad = PendingDocumentLoad::None;
    if (ui.viewMode.currentViewMode() == MODE_DOCUMENT && model->fileCount() > 0) {
      int targetIndex = (target == PendingDocumentLoad::LastImage)
                            ? (model->fileCount() - 1)
                            : 0;
      loadFileIndex(targetIndex, false, settings->usePreloader());
    }
  }
  if (pendingModelImageSync) {
    model->updateImage(state.currentFilePath, state.currentImg);
    pendingModelImageSync = false;
  }
  if (shuffle)
    syncRandomizer();
  updateInfoString();
}

void Core::onThumbPanelDirActivated(QString dirPath) {
  if (dirPath.isEmpty())
    return;
  stopSlideshow();
  QString absolutePath = QDir(dirPath).absolutePath();
  if (!setDirectory(absolutePath))
    return;
  // Stay in document mode; the thumbnail strip will refresh with the new
  // directory's contents once the async scan finishes (onModelLoaded).
  // We cannot call loadFileIndex() yet because fileCount() is still 0;
  // the deferred load is handled via m_pendingDocumentLoad.
  m_pendingDocumentLoad = PendingDocumentLoad::FirstImage;
  if (settings->rememberLastFolder())
    settings->setLastFolder(absolutePath);
}

void Core::onDirectoryViewFileActivated(QString filePath) {
  // we aren`t using async load so it won't flicker with empty view
  ui.viewMode.enableDocumentView();
  loadPath(filePath);
}

void Core::onDirectoryViewFilesActivated(QList<QString> filePaths, QString activePath) {
  ui.viewMode.enableDocumentView();
  loadFileList(filePaths, activePath);
}

bool Core::loadFileList(const QList<QString> &filePaths, QString activePath) {
  if (filePaths.isEmpty())
      return false;

  stopSlideshow();
  
  if (!blockHistory && !model->directoryPath().isEmpty()) {
      backHistory.append(model->directoryPath());
      if (backHistory.count() > 100)
          backHistory.removeFirst();
      forwardHistory.clear();
  }
  
  if (!model->setFileList(filePaths))
      return false;
      
  state.hasActiveImage = false;
  QString toLoad = activePath;
  if (toLoad.isEmpty() || !model->containsFile(toLoad)) {
      toLoad = filePaths.first();
  }
  
  return loadPath(toLoad);
}

void Core::rotateLeft() { rotateByDegrees(-90); }

void Core::rotateRight() { rotateByDegrees(90); }

void Core::nextPage() {
  if (!state.hasActiveImage || !state.currentImg)
    return;
  const QString path = state.currentFilePath;
  const int cur = ImageStatic::pageOverrideForPath(path);
  goToPage(cur + 1);
}

void Core::prevPage() {
  if (!state.hasActiveImage || !state.currentImg)
    return;
  const QString path = state.currentFilePath;
  const int cur = ImageStatic::pageOverrideForPath(path);
  goToPage(cur - 1);
}

void Core::firstPage() {
  goToPage(0);
}

void Core::lastPage() {
  if (!state.hasActiveImage || !state.currentImg)
    return;
  goToPage(state.currentImg->frameCount() - 1);
}

void Core::goToPage(int pageIndex) {
  if (!state.hasActiveImage || !state.currentImg)
    return;
  if (state.currentImg->type() != STATIC || state.currentImg->frameCount() <= 1)
    return;
  const QString path = state.currentFilePath;
  const int count = state.currentImg->frameCount();
  pageIndex = std::clamp(pageIndex, 0, count - 1);
  const int cur = ImageStatic::pageOverrideForPath(path);
  if (pageIndex == cur)
    return;
  ImageStatic::setPageOverrideForPath(path, pageIndex);
  if (!model->reload(path)) {
    ImageStatic::setPageOverrideForPath(path, cur);
  }
}

void Core::removePermanent() { removeSelection(false); }

void Core::moveToTrash() { removeSelection(true); }

void Core::removeSelection(bool trash) {
  auto paths = currentSelection();
  if (!paths.count())
    return;

  int dirCount = 0;
  int fileCount = 0;
  for (const auto &path : std::as_const(paths)) {
    if (QFileInfo(path).isDir()) {
      dirCount++;
    } else {
      fileCount++;
    }
  }

  if (trash ? settings->confirmTrash() : settings->confirmDelete()) {
    QString msg;
    if (paths.count() > 1) {
      const QString count = QString::number(paths.count());
      const QString verb = trash ? tr("Move ") : tr("Delete ");
      if (dirCount > 0 && fileCount == 0) {
        msg = verb + count +
              (trash ? tr(" folders to trash?") : tr(" folders permanently?"));
      } else if (fileCount > 0 && dirCount == 0) {
        msg = verb + count +
              (trash ? tr(" files to trash?") : tr(" files permanently?"));
      } else {
        msg = verb + count +
              (trash ? tr(" items to trash?") : tr(" items permanently?"));
      }
    } else {
      if (dirCount > 0)
        msg = trash ? tr("Move folder to trash?")
                    : tr("Delete folder permanently?");
      else
        msg = trash ? tr("Move file to trash?") : tr("Delete file permanently?");
    }
    const QString title = trash ? tr("Move to trash") : tr("Delete permanently");
    if (!ui.dialogs.confirm({title, msg}).accepted)
      return;
  }
  FileOpResult result;
  int successCount = 0;
  for (const auto &path : std::as_const(paths)) {
    QFileInfo fi(path);
    if (fi.isDir())
      model->removeDir(path, trash, true, result);
    else
      result = removeFile(path, trash);
    if (result == FileOpResult::SUCCESS)
      successCount++;
  }
  if (paths.count() == 1) {
    if (result == FileOpResult::SUCCESS) {
      if (dirCount > 0) {
        ui.shell.refreshFolderTree(QFileInfo(paths.first()).absolutePath());
        ui.notifications.showSuccess(trash ? tr("Folder moved to trash")
                                           : tr("Folder removed"));
      } else
        ui.notifications.showSuccess(trash ? tr("Moved to trash")
                                           : tr("File removed"));
    } else {
      outputError(result);
    }
  } else if (paths.count() > 1) {
    const QString done = (trash ? tr("Moved to trash: ") : tr("Removed: ")) +
                         QString::number(successCount);
    if (dirCount > 0 && fileCount == 0) {
      ui.notifications.showSuccess(done + tr(" folders"));
    } else if (fileCount > 0 && dirCount == 0) {
      ui.notifications.showSuccess(done + tr(" files"));
    } else {
      ui.notifications.showSuccess(done + tr(" items"));
    }
  }
}

void Core::reloadImage() { reloadImage(selectedPath()); }

void Core::reloadImage(QString filePath) {
  if (model->isEmpty())
    return;
  if (model->reload(filePath)) {
    model->refreshFileEntry(filePath);
  }
}

void Core::enableFolderView() {
  if (ui.viewMode.currentViewMode() == MODE_FOLDERVIEW)
    return;
  stopSlideshow();
  
  if (model && model->source() == SOURCE_LIST) {
      QString dirToLoad;
      if (!backHistory.isEmpty()) {
          dirToLoad = backHistory.takeLast();
      } else if (model->fileCount() > 0) {
          dirToLoad = QFileInfo(model->filePathAt(0)).absolutePath();
      }

      if (!dirToLoad.isEmpty()) {
          blockHistory = true;
          setDirectory(dirToLoad);
          blockHistory = false;
      }
  }
  
  ui.viewMode.enableFolderView();
}

void Core::enableDocumentView() {
  if (ui.viewMode.currentViewMode() == MODE_DOCUMENT)
    return;
  const auto selectedPaths = folderViewPresenter.selectedPaths();
  ui.viewMode.enableDocumentView();
  if (!model || !model->fileCount())
    return;

  // Prefer the last selected file. A selected directory does not replace the
  // current image; if no image has been opened yet, fall back to the first one.
  const QString selected = selectedPaths.isEmpty() ? QString()
                                                    : selectedPaths.constLast();
  if (model->containsFile(selected))
    loadPath(selected);
  else if (state.currentFilePath.isEmpty())
    loadPath(model->firstFile());
}

void Core::toggleFolderView() {
  if (ui.viewMode.currentViewMode() == MODE_FOLDERVIEW)
    enableDocumentView();
  else
    enableFolderView();
}

void Core::copyFileClipboard() {
  if (model->isEmpty())
    return;
  withPreparedImages({selectedPath()},
                     [this](const QString &, const std::shared_ptr<Image> &image) {
                       copyImageToClipboard(image);
                     });
}

void Core::copyImageToClipboard(const std::shared_ptr<Image> &image) {
  MimePayloadResult payloadResult = mimePayloadManager->createPayload(
      {image, MimePayloadTarget::Clipboard});
  if (!payloadResult.succeeded()) {
    ui.notifications.showError(mimePayloadFailureMessage(payloadResult.error));
    return;
  }

  QMimeData *mimeData = payloadResult.mimeData.release();

  // mimeData->text() should already contain an url
  QByteArray gnomeFormat =
      QByteArray("copy\n").append(QUrl(mimeData->text()).toEncoded());
  mimeData->setData("x-special/gnome-copied-files", gnomeFormat);

  QGuiApplication::clipboard()->setMimeData(mimeData);
  ui.notifications.showMessage(tr("File copied"));
}

void Core::copyPathClipboard() {
  if (model->isEmpty())
    return;
  QGuiApplication::clipboard()->setText(selectedPath());
  ui.notifications.showMessage(tr("Path copied"));
}

// open from clipboard
void Core::openFromClipboard() {
  auto cb = QGuiApplication::clipboard();
  auto mimeData = cb->mimeData();
  if (!mimeData)
    return;

  // try opening url
  if (mimeData->hasUrls()) {
    auto url = mimeData->urls().constFirst();
    QString path = url.toLocalFile();
    if (path.isEmpty()) {
      qDebug() << "Could not load url:" << url;
      qDebug() << "Currently only local files are supported.";
    } else if (loadPath(path)) {
      return;
    }
  }
  // try to save buffer image then open
  if (mimeData->hasImage()) {
    auto image = cb->image();
    if (image.isNull())
      return;
    QString destPath;
    if (!model->isEmpty())
      destPath = model->directoryPath() + "/";
    else
      destPath = QDir::homePath() + "/";
    destPath.append("clipboard.png");
    destPath = ui.dialogs.requestSavePath({destPath}).path;
    if (destPath.isEmpty())
      return;

    QFileInfo fi(destPath);
    QString ext = fi.suffix();
    int quality = 95;
    if (ext.compare("png", Qt::CaseInsensitive) == 0)
      quality = settings->pngSaveQuality() * 10;
    else if (ext.compare("jpg", Qt::CaseInsensitive) == 0 ||
             ext.compare("jpeg", Qt::CaseInsensitive) == 0)
      quality = settings->JPEGSaveQuality();
    else if (ext.compare("jxl", Qt::CaseInsensitive) == 0 ||
             ext.compare("webp", Qt::CaseInsensitive) == 0 ||
             ext.compare("avif", Qt::CaseInsensitive) == 0)
      quality = settings->modernSaveQuality();

    const ImageSaveResult saveResult =
        FileOperations::saveImage(image, destPath, quality);
    if (saveResult.succeeded()) {
      loadPath(destPath);
      if (!saveResult.retainedBackupPath.isEmpty())
        ui.notifications.showWarning(retainedBackupWarningMessage(saveResult));
    } else {
      ui.notifications.showError(imageSaveFailureMessage(saveResult));
    }
  }
}

void Core::onDropIn(const QMimeData *mimeData, QObject *source) {
  // ignore self
  if (source == this)
    return;
  // check for our needed mime type, here a file or a list of files
  if (mimeData->hasUrls()) {
    QStringList pathList;
    QList<QUrl> urlList = mimeData->urls();
    // extract the local paths of the files
    for (int i = 0; i < urlList.size(); ++i)
      pathList.append(urlList.at(i).toLocalFile());
    // try to open first file in the list
    loadPath(pathList.constFirst());
  }
}

// drag'n'drop
// drag image out of the program
void Core::onDraggedOut() { onDraggedOut(currentSelection()); }

void Core::onDraggedOut(QList<QString> paths) {
  if (paths.isEmpty())
    return;
  QMimeData *mimeData;
  // single selection, image. An image that is not held has no edits, so its
  // file is dragged as it is, without decoding it.
  const std::shared_ptr<Image> image =
      paths.count() == 1 ? heldImage(paths.constFirst()) : nullptr;
  if (image && model->containsFile(paths.constFirst())) {
    MimePayloadResult payloadResult = mimePayloadManager->createPayload(
        {image, MimePayloadTarget::Drop});
    if (!payloadResult.succeeded()) {
      ui.notifications.showError(mimePayloadFailureMessage(payloadResult.error));
      return;
    }
    mimeData = payloadResult.mimeData.release();
  } else { // multi-selection, or single directory. drag urls
    mimeData = new QMimeData();
    QList<QUrl> urlList;
    for (const auto &path : std::as_const(paths))
      urlList << QUrl::fromLocalFile(path);
    mimeData->setUrls(urlList);
  }
  auto *drag = new QDrag(this);
  drag->setMimeData(mimeData);
  drag->exec(SupportedFileDragActions, Qt::CopyAction);
  drag->deleteLater();
}

void Core::sortBy(SortingMode mode) { model->setSortingMode(mode); }

void Core::renameCurrentSelection(QString newName) {
  if (newName.isEmpty() || selectedPath().isEmpty())
    return;
  // The overwrite prompt runs a nested event loop that can change the
  // selection, so the retry must rename the file the user confirmed.
  const QString sourcePath = selectedPath();
  FileOpResult result;
  model->renameEntry(sourcePath, newName, false, result);
  if (result == FileOpResult::DESTINATION_DIR_EXISTS) {
    ui.shell.toggleRenamePrompt(newName);
  } else if (result == FileOpResult::DESTINATION_FILE_EXISTS) {
    if (ui.dialogs.confirm({tr("File exists"), tr("Overwrite file?")}).accepted) {
      model->renameEntry(sourcePath, newName, true, result);
    } else {
      // show rename dialog again
      ui.shell.toggleRenamePrompt(newName);
    }
  }
  outputError(result);
}

FileOpResult Core::removeFile(QString filePath, bool trash) {
  if (model->isEmpty())
    return FileOpResult::NOTHING_TO_DO;

  bool reopen = false;
  std::shared_ptr<Image> img;
  if (state.currentFilePath == filePath) {
    img = model->getImage(filePath);
    if (img->type() == ANIMATED) {
      ui.viewer.closeImage();
      reopen = true;
    }
  }
  FileOpResult result;
  model->removeFile(filePath, trash, result);
  if (result != FileOpResult::SUCCESS && reopen)
    guiSetImage(img);
  return result;
}

void Core::onFileRemoved(QString filePath, int index) {
  // no files left
  if (model->isEmpty()) {
    ui.viewer.closeImage();
    state.hasActiveImage = false;
    state.currentFilePath = "";
  }
  // image mode && removed current file
  if (state.currentFilePath == filePath) {
    if (ui.viewMode.currentViewMode() == MODE_DOCUMENT) {
      if (!loadFileIndex(index, true, settings->usePreloader()))
        loadFileIndex(--index, true, settings->usePreloader());
    } else {
      state.hasActiveImage = false;
      state.currentFilePath = "";
    }
  }
  updateInfoString();
}

void Core::onFileRenamed(QString fromPath, int /*indexFrom*/,
                         QString /*toPath*/, int indexTo) {
  if (state.currentFilePath == fromPath) {
    loadFileIndex(indexTo, true, settings->usePreloader());
  }
}

void Core::onFileAdded(QString filePath) {
  Q_UNUSED(filePath)
  // update file count
  updateInfoString();
  if (model->fileCount() == 1 && state.currentFilePath == "")
    loadFileIndex(0, false, settings->usePreloader());
}

void Core::outputError(const FileOpResult &error) const {
  if (error == FileOpResult::SUCCESS || error == FileOpResult::NOTHING_TO_DO)
    return;
  ui.notifications.showError(FileOperations::decodeResult(error));
  qDebug() << FileOperations::decodeResult(error);
}

void Core::showInDirectory() {
  if (!model)
    return;
  if (selectedPath().isEmpty()) {
    QDesktopServices::openUrl(QUrl::fromLocalFile(model->directoryPath()));
    return;
  }
  QStringList args;
  args << "/select," << QDir::toNativeSeparators(selectedPath());
  QProcess::startDetached("explorer", args);
}

void Core::createDirectory() {
  if (!model)
    return;
  QString currentDirPath = model->directoryPath();
  if (currentDirPath.isEmpty())
    return;

  const TextInputResult input =
      ui.dialogs.requestText({tr("Add folder"), tr("Folder name:"), QString()});
  const QString newFolderName = input.text;
  if (!input.accepted || newFolderName.trimmed().isEmpty())
    return;

  QDir currentDir(currentDirPath);
  if (currentDir.exists(newFolderName)) {
    ui.notifications.showError(tr("Folder already exists"));
    return;
  }

  if (currentDir.mkdir(newFolderName)) {
    QString newDirPath = currentDir.absoluteFilePath(newFolderName);
    model->insertDir(newDirPath);
    ui.shell.refreshFolderTree(currentDirPath);
  } else {
    ui.notifications.showError(tr("Failed to create folder"));
  }
}

void Core::interactiveCopy(QList<QString> paths, QString destDirectory) {
  // Recursion + I/O now happen off the GUI thread; see
  // components/fileoptask/. FileOpController::progress()/finished() report
  // back via onFileOpProgress()/onFileOpFinished().
  if (!fileOpController)
    return;
  fileOpController->startCopy(std::move(paths), std::move(destDirectory));
}

void Core::interactiveMove(QList<QString> paths, QString destDirectory) {
  if (!fileOpController)
    return;
  fileOpController->startMove(std::move(paths), std::move(destDirectory));
}
// -----------------------------------------------------------------------------------

void Core::onFileOpProgress(FileOpProgress progress) {
  const QString fileName = QFileInfo(progress.currentPath).fileName();
  ui.notifications.showMessage(progress.isMove
                       ? tr("Moving: %1 (%2)").arg(fileName).arg(progress.filesDone)
                       : tr("Copying: %1 (%2)").arg(fileName).arg(progress.filesDone));
}

void Core::onFileOpFinished(FileOpSummary summary) {
  if (summary.cancelled || summary.filesProcessed == 0) {
    ui.notifications.hideNotifications();
    return;
  }
  ui.notifications.showSuccess(summary.isMove
                              ? tr("Moved %n file(s)", "", summary.filesProcessed)
                              : tr("Copied %n file(s)", "", summary.filesProcessed));
}

// -----------------------------------------------------------------------------------

void Core::onClearThumbnailCacheRequested() {
  if (!thumbnailer->clearCache()) {
      ui.notifications.showError(tr("Failed to clear thumbnail cache"));
  }
}

// -----------------------------------------------------------------------------------

void Core::copyPathsTo(QList<QString> paths, QString destDirectory) {
  interactiveCopy(paths, destDirectory);
}

void Core::movePathsTo(QList<QString> paths, QString destDirectory) {
  interactiveMove(paths, destDirectory);
}

void Core::onDirectoryPresenterDroppedInto(QList<QString> paths, QString destDirectory, Qt::DropAction action) {
  if (action == Qt::CopyAction) {
    copyPathsTo(paths, destDirectory);
  } else if (action == Qt::MoveAction) {
    movePathsTo(paths, destDirectory);
  }
}

void Core::moveCurrentFile(QString destDirectory) {
  if (model->isEmpty())
    return;
  // pause updates to avoid flicker
  ui.window.setWindowUpdatesSuspended(true);
  // move fails during file playback, so we close it temporarily
  ui.viewer.closeImage();
  // The overwrite prompt runs a nested event loop that can change the
  // selection, so the retry must move the file the user confirmed.
  const QString sourcePath = selectedPath();
  FileOpResult result;
  model->moveFileTo(sourcePath, destDirectory, false, result);
  if (result == FileOpResult::SUCCESS) {
    ui.notifications.showSuccess(tr("File moved."));
  } else if (result == FileOpResult::DESTINATION_FILE_EXISTS) {
    if (ui.dialogs.confirm({tr("File exists"),
                            tr("Destination file exists. Overwrite?")}).accepted)
      model->moveFileTo(sourcePath, destDirectory, true, result);
  }
  if (result != FileOpResult::SUCCESS) {
    guiSetImage(model->getImage(sourcePath));
    updateInfoString();
    if (result != FileOpResult::DESTINATION_FILE_EXISTS)
      outputError(result);
  }
  ui.window.setWindowUpdatesSuspended(false);
}

void Core::copyCurrentFile(QString destDirectory) {
  if (model->isEmpty())
    return;
  // The overwrite prompt runs a nested event loop that can change the
  // selection, so the retry must copy the file the user confirmed.
  const QString sourcePath = selectedPath();
  FileOpResult result;
  model->copyFileTo(sourcePath, destDirectory, false, result);
  if (result == FileOpResult::SUCCESS) {
    ui.notifications.showSuccess(tr("File copied."));
  } else if (result == FileOpResult::DESTINATION_FILE_EXISTS) {
    if (ui.dialogs.confirm({tr("File exists"),
                            tr("Destination file exists. Overwrite?")}).accepted)
      model->copyFileTo(sourcePath, destDirectory, true, result);
  }
  if (result != FileOpResult::SUCCESS &&
      result != FileOpResult::DESTINATION_FILE_EXISTS)
    outputError(result);
}

void Core::toggleCropPanel() {
  if (model->isEmpty()) {
    qWarning() << "Core::toggleCropPanel: ignored, the directory model is empty";
    return;
  }
  if (!ui.shell.isCropPanelActive() && !state.hasActiveImage) {
    qWarning() << "Core::toggleCropPanel: ignored, no image is active yet";
    return;
  }
  ui.shell.toggleCropPanel();
}

void Core::toggleFullscreenInfoBar() { ui.shell.toggleFullscreenInfoBar(); }

void Core::requestSavePath() {
  if (model->isEmpty())
    return;
  const SavePathResult result = ui.dialogs.requestSavePath({selectedPath()});
  if (result.accepted())
    saveCurrentFileAs(result.path);
}

void Core::showResizeDialog() {
  if (model->isEmpty()) {
    qWarning() << "Core::showResizeDialog: ignored, the directory model is empty";
    return;
  }
  withPreparedImages(
      {selectedPath()},
      [this](const QString &path, const std::shared_ptr<Image> &image) {
        if (!image) {
          qWarning() << "Core::showResizeDialog: ignored, no image for the selected path"
                     << path;
          return;
        }
        const std::optional<ResizeRequest> request =
            ui.dialogs.requestResize(image->size());
        if (request)
          resize(*request);
      });
}

void Core::showBatchConverter() {
  // The Batch convert button while files wait in the converter (hidden by
  // its "Add more" button): reopen it with exactly those files; the
  // selection is added only by addToBatchConverter().
  if (!batchStaging.isEmpty()) {
    openBatchConverter();
    return;
  }
  addToBatchConverter();
}

void Core::addToBatchConverter() {
  // expandedSelectedPaths() used to run its recursive QDirIterator walk
  // synchronously right here, freezing the UI thread for as long as the
  // selected directories took to walk - the same freeze
  // DirectoryPresenter::onItemActivated()/onOpenSelectedRequested() had
  // before being moved onto DirectoryExpandWorker. Ask the presenter to
  // do the same background scan here instead of blocking; onBatchConverterPathsReady()
  // adds the files and opens the dialog once expandedSelectedPathsReady() arrives.
  folderViewPresenter.requestExpandedSelectedPathsAsync();
}

void Core::onBatchConverterPathsReady(QList<QString> filePaths, QString defaultOutputDir) {
  // Mirrors the old synchronous showBatchConverter()'s guard: do nothing
  // if the selection expanded to no matching files.
  if (filePaths.isEmpty())
    return;

  batchStaging.append(filePaths, defaultOutputDir);
  openBatchConverter();
}

void Core::openBatchConverter() {
  const QString currentDirPath = model->directoryPath();
  const BatchConversionResult result = ui.dialogs.runBatchConverter(
      {.files = batchStaging.entries(),
       .defaultOutputDirectory = batchStaging.defaultOutputDirectory()});
  // Only "Add more" keeps the list; every other way of closing the dialog
  // ends it.
  if (result.listKept)
    batchStaging.replace(result.files);
  else
    batchStaging.clear();
  if (result.conversionStarted && !currentDirPath.isEmpty()) {
    model->setDirectory(currentDirPath);
  }
}

// ---------------------------------------------------------------- image
// operations

std::shared_ptr<Image> Core::heldImage(const QString &path) {
  if (model->isLoaded(path))
    return model->getImage(path);
  if (state.currentImg && path == state.currentFilePath)
    return state.currentImg;
  return nullptr;
}

void Core::withPreparedImages(QList<QString> paths, ImageAction onImage,
                              std::function<void()> onFinished) {
  auto run = std::make_shared<ImageActionRun>(
      ImageActionRun{.paths = std::move(paths),
                     .onImage = std::move(onImage),
                     .onFinished = std::move(onFinished)});
  continueImageAction(run, nullptr);
}

void Core::continueImageAction(const std::shared_ptr<ImageActionRun> &run,
                               std::shared_ptr<Image> loaded) {
  while (run->next < run->paths.size()) {
    const QString path = run->paths.at(run->next);
    // The held image is the one the viewer shows and edits go to; it can
    // have been cached while ImagePreparer decoded loaded.
    std::shared_ptr<Image> image = heldImage(path);
    if (!image)
      image = std::move(loaded);
    loaded.reset();
    if (!ImagePreparer::isReady(image)) {
      imagePreparer->prepare(
          path, std::move(image),
          [this, run, path](std::shared_ptr<Image> prepared) {
            if (!model->containsFile(path) && path != state.currentFilePath) {
              qWarning() << "Core: an image action skipped" << path
                         << "- it left the folder while it was prepared";
              ++run->next;
              continueImageAction(run, nullptr);
              return;
            }
            if (!prepared) {
              run->onImage(path, nullptr);
              ++run->next;
              continueImageAction(run, nullptr);
              return;
            }
            // Checked again: the image can have been cached, or the tone
            // mapping settings changed, meanwhile.
            continueImageAction(run, std::move(prepared));
          });
      return;
    }
    run->onImage(path, image);
    ++run->next;
  }
  if (run->onFinished)
    run->onFinished();
}

void Core::editImages(const QList<QString> &paths,
                      const ImageEditRequest &request) {
  if (model->isEmpty())
    return;
  if (request.saving == EditSaving::AfterConfirmation &&
      !ui.dialogs.confirm({
          request.action,
          tr("Perform action \"") + request.action + "\"? \n\n" +
              tr("Changes will be saved immediately.")}).accepted)
    return;
  auto allEdited = std::make_shared<bool>(true);
  withPreparedImages(
      paths,
      [this, request, allEdited](const QString &path,
                                 const std::shared_ptr<Image> &image) {
        if (!applyEdit(path, image, request))
          *allEdited = false;
      },
      [this, action = request.action, allEdited]() {
        if (!*allEdited)
          ui.notifications.showError(
              tr("%1 failed: the image could not be edited.").arg(action));
        updateInfoString();
      });
}

// An image whose pixels are unavailable (it could not be decoded, or its
// HDR-to-SDR conversion failed) or whose edit produced no image is left
// unchanged and not saved. Images that are not static are not edited.
bool Core::applyEdit(const QString &path, const std::shared_ptr<Image> &image,
                     const ImageEditRequest &request) {
  if (!image)
    return false;
  const auto img = std::dynamic_pointer_cast<ImageStatic>(image);
  if (!img)
    return true;
  const std::shared_ptr<const QImage> source = img->getImage();
  if (!source || source->isNull() ||
      !img->setEditedImage(std::make_unique<const QImage>(request.edit(source)),
                           request.kind))
    return false;
  model->updateImage(path, image);
  if (request.saving != EditSaving::None) {
    (void)saveFile(path);
    if (state.currentFilePath != path)
      model->unload(path);
  }
  return true;
}

// The folder view saves rotations and flips at once.
ImageEditRequest Core::orientationEdit(
    QString action, std::function<QImage(std::shared_ptr<const QImage>)> edit) {
  const EditSaving saving = ui.viewMode.currentViewMode() == MODE_FOLDERVIEW
                                ? EditSaving::AfterConfirmation
                                : EditSaving::None;
  return {.kind = ImageEditKind::Orientation,
          .action = std::move(action),
          .edit = std::move(edit),
          .saving = saving};
}

ImageEditRequest Core::resizeEdit(const ResizeRequest &request) const {
  return {.kind = ImageEditKind::Content,
          .action = tr("Resize"),
          .edit = [size = request.size, filter = request.filter](
                      std::shared_ptr<const QImage> source) {
            return ImageLib::scaled(std::move(source), size, filter);
          }};
}

void Core::flipH() {
  editImages(currentSelection(),
             orientationEdit(tr("Flip horizontal"), ImageLib::flippedH));
}

void Core::flipV() {
  editImages(currentSelection(),
             orientationEdit(tr("Flip vertical"), ImageLib::flippedV));
}

void Core::rotateByDegrees(int degrees) {
  editImages(currentSelection(),
             orientationEdit(tr("Rotate"),
                             [degrees](std::shared_ptr<const QImage> source) {
                               return ImageLib::rotated(std::move(source), degrees);
                             }));
}

void Core::resize(const ResizeRequest &resizeRequest) {
  if (!resizeRequest.useUpscayl) {
    editImages(currentSelection(), resizeEdit(resizeRequest));
    return;
  }
  if (model->isEmpty())
    return;

  if (activeAiResizeOperation.has_value()) {
    ui.notifications.showAiUpscale(tr("AI resize is already running."));
    return;
  }

  const auto selection = currentSelection();
  if (selection.isEmpty())
    return;

  if (selection.size() > 1) {
    ui.notifications.showWarning(tr("AI resize supports one image at a time."));
    return;
  }

  withPreparedImages(
      {selection.constFirst()},
      [this, resizeRequest](const QString &path,
                            const std::shared_ptr<Image> &image) {
        startAiResize(path, image, resizeRequest);
      });
}

void Core::startAiResize(const QString &path, const std::shared_ptr<Image> &image,
                         const ResizeRequest &resizeRequest) {
  // Another AI resize can have started while the image was prepared.
  if (activeAiResizeOperation.has_value()) {
    ui.notifications.showAiUpscale(tr("AI resize is already running."));
    return;
  }

  const auto img = std::dynamic_pointer_cast<ImageStatic>(image);
  if (!img) {
    ui.notifications.showError(tr("Could not resize image."));
    return;
  }

  std::shared_ptr<const QImage> source = img->getImage();
  if (!source || source->isNull()) {
    ui.notifications.showError(tr("Could not resize image."));
    return;
  }

  // Final safety net: AI upscaling only makes sense when the target is
  // actually larger than the source in some dimension. A caller might
  // still pass useUpscayl=true here from a stale/persisted checkbox state
  // for a same-size or downscale request - don't pay for a full AI model
  // pass in that case, just do the regular (much cheaper) CPU resize.
  const QSize size = resizeRequest.size;
  if (size.width() <= source->width() && size.height() <= source->height()) {
    const ImageEditRequest edit = resizeEdit(resizeRequest);
    if (!applyEdit(path, image, edit))
      ui.notifications.showError(
          tr("%1 failed: the image could not be edited.").arg(edit.action));
    updateInfoString();
    return;
  }

  UpscaylResizeRequest request;
  request.path = path;
  request.targetSize = size;
  request.filter = resizeRequest.filter;
  request.modelName = resizeRequest.upscaylModel;
  request.sourceImage = source;
  request.generation = ++aiResizeGeneration;

  // The result is applied to the cached image, so an image of the folder view
  // selection that is not cached yet is cached now.
  if (!heldImage(path))
    model->updateImage(path, image);
  activeAiResizeOperation = AiResizeOperation{
      request.generation, path, img->contentRevision(), img};
  QGuiApplication::setOverrideCursor(Qt::WaitCursor);
  aiResizeBusyUiActive = true;
  ui.notifications.showAiUpscale(tr("AI resizing..."), 3600000);

  auto task = new UpscaylResizeRunnable(request);
  task->setAutoDelete(false);
  connect(task, &UpscaylResizeRunnable::finished, task, &QObject::deleteLater, Qt::QueuedConnection);
  connect(task, &UpscaylResizeRunnable::finished, this, &Core::onAiResizeFinished, Qt::QueuedConnection);
  QThreadPool::globalInstance()->start(task);
}

void Core::clearAiResizeBusyUi() {
  if (!aiResizeBusyUiActive)
    return;

  aiResizeBusyUiActive = false;
  QGuiApplication::restoreOverrideCursor();
  ui.notifications.hideNotifications();
}

void Core::onAiResizeFinished(int generation, QString path, QImage image, bool success, QString error) {
  if (!activeAiResizeOperation.has_value() ||
      generation != activeAiResizeOperation->generation)
    return;

  const AiResizeOperation operation = *activeAiResizeOperation;
  activeAiResizeOperation.reset();
  clearAiResizeBusyUi();

  if (generation != aiResizeGeneration || path != operation.path)
    return;

  if (!success || image.isNull()) {
    ui.notifications.showError(error.isEmpty() ? tr("AI resize failed.") : error);
    return;
  }

  if (!model->containsFile(path)) {
    ui.notifications.showWarning(tr("AI resize finished, but the image is no longer in the list."));
    return;
  }

  // An image no longer held was unloaded or reloaded meanwhile.
  const std::shared_ptr<ImageStatic> img = operation.sourceImage.lock();
  if (!img || heldImage(path) != img ||
      img->contentRevision() != operation.sourceRevision) {
    ui.notifications.showWarning(tr("AI resize finished, but the image has changed."));
    return;
  }

  img->setEditedImage(std::make_unique<const QImage>(std::move(image)),
                      ImageEditKind::Content);
  model->updateImage(path, std::static_pointer_cast<Image>(img));

  if (state.hasActiveImage && path == state.currentFilePath) {
    updateInfoString();
    ui.notifications.showSuccess(tr("AI resize finished."));
  } else {
    ui.notifications.showSuccess(tr("AI resize finished for %1.").arg(QFileInfo(path).fileName()));
  }
}

void Core::crop(QRect rect) {
  if (ui.viewMode.currentViewMode() == MODE_FOLDERVIEW)
    return;
  editImages(currentSelection(),
             {.kind = ImageEditKind::Content,
              .action = tr("Crop"),
              .edit = [rect](std::shared_ptr<const QImage> source) {
                return ImageLib::cropped(std::move(source), rect);
              }});
}

void Core::cropAndSave(QRect rect) {
  if (ui.viewMode.currentViewMode() == MODE_FOLDERVIEW)
    return;
  editImages(currentSelection(),
             {.kind = ImageEditKind::Content,
              .action = tr("Crop"),
              .edit = [rect](std::shared_ptr<const QImage> source) {
                return ImageLib::cropped(std::move(source), rect);
              },
              .saving = EditSaving::Immediately});
}

void Core::applyColorAdjustments(const ColorAdjustments &adjustments) {
  if (model->isEmpty() || !adjustments.hasAdjustments())
    return;
  withPreparedImages({state.currentFilePath},
                     [this, adjustments](const QString &path,
                                         const std::shared_ptr<Image> &image) {
                       applyColorAdjustmentsTo(path, image, adjustments);
                     });
}

void Core::applyColorAdjustmentsTo(const QString &path,
                                   const std::shared_ptr<Image> &image,
                                   const ColorAdjustments &adjustments) {
  const auto img = std::dynamic_pointer_cast<ImageStatic>(image);
  if (!img)
    return;

  QImage adjusted = ImageLib::applyColorAdjustments(img->getImage(), adjustments);
  if (adjusted.isNull()) {
    ui.notifications.showError(tr("Could not adjust the image colors."));
    return;
  }

  img->setEditedImage(std::make_unique<const QImage>(adjusted),
                      ImageEditKind::Content);
  model->updateImage(path, image);
  updateInfoString();
}

// ---------------------------------------------------------------- image
// operations ^

ImageSaveResult Core::saveFile(const QString &filePath) {
  return saveFile(filePath, filePath);
}

ImageSaveResult Core::saveFile(const QString &filePath, const QString &newPath) {
  const ImageSaveResult saveResult = model->saveFile(filePath, newPath);
  if (!saveResult.succeeded()) {
    ui.notifications.showError(imageSaveFailureMessage(saveResult));
    return saveResult;
  }
  if (!saveResult.retainedBackupPath.isEmpty())
    ui.notifications.showWarning(retainedBackupWarningMessage(saveResult));
  ui.shell.setSaveOverlayVisible(false);
  // switch to the new file
  if (model->containsFile(newPath) && state.currentFilePath != newPath) {
    discardEditsOf(filePath);
    if (ui.viewMode.currentViewMode() == MODE_DOCUMENT)
      loadPath(newPath);
  }
  return saveResult;
}

void Core::saveCurrentFile() { saveCurrentFileAs(selectedPath()); }

void Core::saveCurrentFileAs(QString destPath) {
  if (model->isEmpty())
    return;
  withPreparedImages({selectedPath()},
                     [this, destPath](const QString &path,
                                      const std::shared_ptr<Image> &image) {
                       saveImageAs(path, image, destPath);
                     });
}

void Core::saveImageAs(const QString &path, const std::shared_ptr<Image> &image,
                       const QString &destPath) {
  if (!image) {
    ui.notifications.showError(
        imageSaveFailureMessage({ImageSaveError::SourceUnavailable}));
    return;
  }
  // DirectoryModel saves cached images; an image of the folder view
  // selection that is not cached is cached for the save only.
  const bool cachedForSave = !heldImage(path);
  if (cachedForSave)
    model->updateImage(path, image);
  const ImageSaveResult saveResult = saveFile(path, destPath);
  if (cachedForSave && state.currentFilePath != path)
    model->unload(path);
  if (saveResult.succeeded()) {
    if (saveResult.retainedBackupPath.isEmpty())
      ui.notifications.showSuccess(tr("File saved"));
    updateInfoString();
  }
}

void Core::discardEdits() {
  if (model->isEmpty())
    return;
  discardEditsOf(selectedPath());
}

// Only a held image can have edits.
void Core::discardEditsOf(const QString &path) {
  std::shared_ptr<Image> img = heldImage(path);
  if (img && img->type() == STATIC) {
    auto imgStatic = dynamic_cast<ImageStatic *>(img.get());
    imgStatic->discardEditedImage();
    model->updateImage(path, img);
  }
  ui.shell.setSaveOverlayVisible(false);
}

QString Core::selectedPath() {
  if (!model)
    return "";
  else if (ui.viewMode.currentViewMode() == MODE_FOLDERVIEW) {
    auto paths = folderViewPresenter.selectedPaths();
    return paths.isEmpty() ? QString() : paths.constLast();
  }
  else
    return state.currentFilePath;
}

QList<QString> Core::currentSelection() {
  if (!model)
    return QList<QString>();
  else if (ui.viewMode.currentViewMode() == MODE_FOLDERVIEW)
    return folderViewPresenter.selectedPaths();
  else
    return QList<QString>() << state.currentFilePath;
}

//------------------------

void Core::sortByName() {
  auto mode = SortingMode::SORT_NAME;
  if (model->sortingMode() == mode)
    mode = SortingMode::SORT_NAME_DESC;
  model->setSortingMode(mode);
}

void Core::sortByTime() {
  auto mode = SortingMode::SORT_TIME;
  if (model->sortingMode() == mode)
    mode = SortingMode::SORT_TIME_DESC;
  model->setSortingMode(mode);
}

void Core::sortBySize() {
  auto mode = SortingMode::SORT_SIZE;
  if (model->sortingMode() == mode)
    mode = SortingMode::SORT_SIZE_DESC;
  model->setSortingMode(mode);
}

void Core::showRenameDialog() {
  if (selectedPath().isEmpty())
    return;
  QFileInfo fi(selectedPath());
  ui.shell.toggleRenamePrompt(fi.fileName());
}

void Core::runScript(const QString &scriptName) {
  if (model->isEmpty())
    return;
  scriptManager->runScript(scriptName, selectedPath());
}

void Core::setWallpaper() {
  if (model->isEmpty() || selectedPath().isEmpty())
    return;
  withPreparedImages({selectedPath()},
                     [this](const QString &, const std::shared_ptr<Image> &image) {
                       setImageAsWallpaper(image);
                     });
}

void Core::setImageAsWallpaper(const std::shared_ptr<Image> &img) {
  if (!img || img->type() != DocumentType::STATIC) {
    ui.notifications.showMessage(tr("Set wallpaper: file not supported"));
    return;
  }

  auto imgStatic = std::dynamic_pointer_cast<ImageStatic>(img);
  if (!imgStatic) {
    ui.notifications.showMessage(tr("Set wallpaper: file not supported"));
    return;
  }

  auto sourceImage = imgStatic->getImage();
  if (!sourceImage || sourceImage->isNull()) {
    ui.notifications.showMessage(tr("Set wallpaper: failed to get image"));
    return;
  }

  wallpaperController->setWallpaper(sourceImage);
}

void Core::print() {
  if (model->isEmpty())
    return;
  withPreparedImages({selectedPath()},
                     [this](const QString &, const std::shared_ptr<Image> &image) {
                       printImage(image);
                     });
}

void Core::printImage(const std::shared_ptr<Image> &img) {
  if (!img) {
    ui.notifications.showError(tr("Could not open image"));
    return;
  }
  if (img->type() != DocumentType::STATIC) {
    ui.notifications.showError(tr("Can only print static images"));
    return;
  }
  const std::shared_ptr<const QImage> pixels = img->getImage();
  if (!pixels || pixels->isNull()) {
    ui.notifications.showError(tr("Could not open image"));
    return;
  }
  QString pdfPath = model->directoryPath() + "/" + img->baseName() + ".pdf";
  ui.dialogs.print({pixels, pdfPath});
}

// The viewer scales on the GPU and asks for the upscale directly, without a
// CPU-scaled copy.
void Core::onUpscaleRequested(QSize size) {
  if (!ui.window.isWindowVisible() || !state.hasActiveImage)
    return;
  std::shared_ptr<Image> image = model->getImage(state.currentFilePath);
  if (image)
    updateUpscale(image, size, state.currentFilePath);
}

// size: the size the image is displayed at, device pixels.
void Core::updateUpscale(const std::shared_ptr<Image> &image, QSize size,
                         const QString &path) {
  constexpr float kPercent = 100.0f;
  const UpscaleInputs inputs{
      .panoramaMode = ui.viewer.panoramaMode(),
      .useUpscayl = settings->useUpscayl(),
      .staticImage = image && image->type() == DocumentType::STATIC,
      .limitEnabled = settings->upscaylLimitEnabled(),
      .limitPercent = settings->upscaylLimitValue(),
      .zoomPercent = ui.viewer.currentScale() * kPercent,
      .displayedWidth = size.width(),
      .imageWidth = image ? image->width() : 0,
  };
  switch (decideUpscale(inputs)) {
  case UpscaleAction::HideCropAndReset:
    ui.viewer.hideUpscaledCrop();
    upscaler->reset();
    break;
  case UpscaleAction::HideCrop:
    ui.viewer.hideUpscaledCrop();
    break;
  case UpscaleAction::Request:
    upscaler->requestUpscale(image, size, path);
    break;
  case UpscaleAction::InvalidatePreview:
    upscaler->invalidatePreview();
    break;
  case UpscaleAction::None:
    break;
  }
}


// reset state; clear cache; etc
void Core::reset() {
  upscaler->reset();
  aiResizeGeneration++;
  clearAiResizeBusyUi();
  state.hasActiveImage = false;
  state.currentFilePath = "";
  state.directoryPath = "";
  state.currentImg.reset();
  m_pendingDocumentLoad = PendingDocumentLoad::None;
  model->setDirectory("");
}

bool Core::loadPath(QString path) {
  upscaler->reset();
  if (path.isEmpty())
    return false;
  if (path.startsWith("file://", Qt::CaseInsensitive))
    path.remove(0, 7);

  stopSlideshow();
  state.delayModel = false;
  QFileInfo fileInfo(path);
  if (fileInfo.isDir()) {
    state.directoryPath = QDir(path).absolutePath();
  } else if (fileInfo.isFile()) {
    state.directoryPath = fileInfo.absolutePath();
    if (model->source() == SOURCE_LIST && model->containsFile(path)) {
      // already in the list, keep SOURCE_LIST
    } else if (model->directoryPath() != state.directoryPath) {
      state.delayModel = true;
    }
  } else {
    ui.notifications.showError(tr("Could not open path: ") + path);
    qDebug() << "Could not open path: " << path;
    return false;
  }
  
  bool skipSetDir = (model->source() == SOURCE_LIST && model->containsFile(path));
  if (!skipSetDir && !state.delayModel && !setDirectory(state.directoryPath))
    return false;

  // load file / folderview
  bool success = false;
  if (fileInfo.isFile()) {
    int index = model->indexOfFile(fileInfo.absoluteFilePath());
    // DirectoryManager only checks file extensions via regex (performance
    // reasons) But in this case we force check mimetype.
    // If the file index is not found (e.g. delayed loading), check against
    // supported regex. Falls back to QMimeDatabase query if regex check is not
    // matched.
    if (index == -1) {
      bool isSupported = false;
      QRegularExpression re(settings->supportedFormatsRegex(),
                            QRegularExpression::CaseInsensitiveOption);
      if (re.match(fileInfo.fileName()).hasMatch()) {
        isSupported = true;
      } else {
        QStringList types = settings->supportedMimeTypes();
        static QMimeDatabase db;
        QMimeType type = db.mimeTypeForFile(fileInfo.absoluteFilePath());
        if (types.contains(type.name())) {
          isSupported = true;
        }
      }

      if (isSupported) {
        if (model->forceInsert(fileInfo.absoluteFilePath())) {
          index = model->indexOfFile(fileInfo.absoluteFilePath());
        }
      }
    }
    ui.viewMode.enableDocumentView();
    success = loadFileIndex(index, false, settings->usePreloader());
  } else {
    if (ui.viewMode.currentViewMode() == MODE_DOCUMENT && model->fileCount() > 0) {
      success = loadFileIndex(0, false, settings->usePreloader());
    } else {
      ui.viewMode.enableFolderView();
      success = true;
    }
  }
  if (success && settings->rememberLastFolder()) {
    settings->setLastFolder(state.directoryPath);
  }
  return success;
}

bool Core::setDirectory(QString path) {
  if (model->directoryPath() != path) {
    if (!blockHistory && !model->directoryPath().isEmpty()) {
      backHistory.append(model->directoryPath());
      if (backHistory.count() > 100)
        backHistory.removeFirst();
      forwardHistory.clear();
    }
    this->reset();
    if (!model->setDirectory(path)) {
      ui.notifications.showError(tr("Could not load folder: ") + path);
      return false;
    }
    ui.shell.setDirectoryPath(path);
    state.directoryPath = path;
  }
  return true;
}

bool Core::loadFileIndex(int index, bool async, bool preload) {
  if (!model)
    return false;
  auto entry = model->fileEntryAt(index);
  if (entry.path.isEmpty())
    return false;
  state.currentFilePath = entry.path;
  preloadTimer.stop();
  model->unloadExcept(entry.path, preload);
  model->load(entry.path, async);
  thumbPanelPresenter.selectAndFocus(entry.path);
  folderViewPresenter.selectAndFocus(entry.path);
  updateInfoString();
  return true;
}

void Core::loadParentDir() {
  if (ui.viewMode.currentViewMode() != MODE_FOLDERVIEW)
    return;
  if (model->directoryPath().isEmpty()) {
      if (model->source() == SOURCE_LIST) {
          if (!backHistory.isEmpty()) {
              historyBack();
          } else if (model->fileCount() > 0) {
              QString path = model->filePathAt(0);
              loadPath(QFileInfo(path).absolutePath());
          }
      }
      return;
  }
  stopSlideshow();
  QFileInfo currentDir(model->directoryPath());
  QFileInfo parentDir(currentDir.absolutePath());
  if (parentDir.exists() && parentDir.isReadable()) {
    // Directory scanning is asynchronous, so the parent directory isn't
    // loaded yet at this point - selecting it now would no-op. Defer to
    // onModelLoaded() instead (see pendingFolderViewSelectPath).
    pendingFolderViewSelectPath = currentDir.absoluteFilePath();
    loadPath(parentDir.absoluteFilePath());
  }
}

void Core::nextDirectory() {
  if (model->directoryPath().isEmpty() ||
      ui.viewMode.currentViewMode() != MODE_DOCUMENT ||
      model->loaderBusy())
    return;
  stopSlideshow();
  auto nextResult = model->nextSiblingDir(model->directoryPath());
  if (!nextResult) {
    ui.notifications.showError(tr("Could not enumerate sibling folders: ") +
                  QString::fromStdString(nextResult.error().message()));
    return;
  }
  QString next = *nextResult;
  if (!next.isEmpty()) {
    if (!setDirectory(next))
      return;
    m_pendingDocumentLoad = PendingDocumentLoad::FirstImage;
    QFileInfo fi(next);
    ui.notifications.showDirectory(fi.baseName());
    if (settings->rememberLastFolder())
      settings->setLastFolder(next);
  } else {
    ui.notifications.showDirectoryEnd();
  }
}

void Core::prevDirectory(bool selectLast) {
  if (model->directoryPath().isEmpty() ||
      ui.viewMode.currentViewMode() != MODE_DOCUMENT ||
      model->loaderBusy())
    return;
  stopSlideshow();
  auto prevResult = model->prevSiblingDir(model->directoryPath());
  if (!prevResult) {
    ui.notifications.showError(tr("Could not enumerate sibling folders: ") +
                  QString::fromStdString(prevResult.error().message()));
    return;
  }
  QString prev = *prevResult;
  if (!prev.isEmpty()) {
    if (!setDirectory(prev))
      return;
    m_pendingDocumentLoad = selectLast ? PendingDocumentLoad::LastImage : PendingDocumentLoad::FirstImage;
    QFileInfo fi(prev);
    ui.notifications.showDirectory(fi.baseName());
    if (settings->rememberLastFolder())
      settings->setLastFolder(prev);
  } else {
    ui.notifications.showDirectoryStart();
  }
}

void Core::prevDirectory() { prevDirectory(false); }

void Core::historyBack() {
  if (!backHistory.isEmpty()) {
    QString childPath = model->directoryPath();
    QString path = backHistory.takeLast();
    forwardHistory.append(childPath);
    blockHistory = true;
    // Restore selection/scroll position to the folder we just left once the
    // parent directory has actually finished loading (see
    // pendingFolderViewSelectPath) - otherwise the view jumps back to the
    // top instead of where we were.
    if (!childPath.isEmpty())
      pendingFolderViewSelectPath = childPath;
    // When in document mode (thumbnail panel with subfolders), stay in
    // document mode instead of switching to folder view.  loadPath() would
    // see fileCount()==0 (async scan) and flip to folder view.
    if (ui.viewMode.currentViewMode() == MODE_DOCUMENT && QFileInfo(path).isDir()) {
      stopSlideshow();
      setDirectory(path);
      m_pendingDocumentLoad = PendingDocumentLoad::FirstImage;
      if (settings->rememberLastFolder())
        settings->setLastFolder(path);
    } else {
      loadPath(path);
    }
    blockHistory = false;
  } else {
    loadParentDir();
  }
}

void Core::historyForward() {
  if (!forwardHistory.isEmpty()) {
    QString path = forwardHistory.takeLast();
    backHistory.append(model->directoryPath());
    blockHistory = true;
    if (ui.viewMode.currentViewMode() == MODE_DOCUMENT && QFileInfo(path).isDir()) {
      stopSlideshow();
      setDirectory(path);
      m_pendingDocumentLoad = PendingDocumentLoad::FirstImage;
      if (settings->rememberLastFolder())
        settings->setLastFolder(path);
    } else {
      loadPath(path);
    }
    blockHistory = false;
  }
}

void Core::nextImage() {
  if ((model->isEmpty() && folderEndAction != FOLDER_END_GOTO_ADJACENT))
    return;
  stopSlideshow();
  if (shuffle) {
    loadFileIndex(randomizer.next(), true, false);
    return;
  }
  int newIndex = model->indexOfFile(state.currentFilePath) + 1;
  if (newIndex >= model->fileCount()) {
    if (folderEndAction == FOLDER_END_LOOP) {
      newIndex = 0;
    } else if (folderEndAction == FOLDER_END_GOTO_ADJACENT) {
      nextDirectory();
      return;
    } else {
      if (!model->loaderBusy())
        ui.notifications.showDirectoryEnd();
      return;
    }
  }
  loadFileIndex(newIndex, true, settings->usePreloader());
}

void Core::prevImage() {
  if ((model->isEmpty() && folderEndAction != FOLDER_END_GOTO_ADJACENT))
    return;
  stopSlideshow();
  if (shuffle) {
    loadFileIndex(randomizer.prev(), true, false);
    return;
  }

  int newIndex = model->indexOfFile(state.currentFilePath) - 1;
  if (newIndex < 0) {
    if (folderEndAction == FOLDER_END_LOOP) {
      newIndex = model->fileCount() - 1;
    } else if (folderEndAction == FOLDER_END_GOTO_ADJACENT) {
      prevDirectory(true);
      return;
    } else {
      if (!model->loaderBusy())
        ui.notifications.showDirectoryStart();
      return;
    }
  }
  loadFileIndex(newIndex, true, settings->usePreloader());
}

void Core::nextImageSlideshow() {
  if (model->isEmpty() || ui.viewMode.currentViewMode() == MODE_FOLDERVIEW)
    return;
  if (shuffle) {
    loadFileIndex(randomizer.next(), false, false);
  } else {
    int newIndex = model->indexOfFile(state.currentFilePath) + 1;
    if (newIndex >= model->fileCount()) {
      if (loopSlideshow) {
        newIndex = 0;
      } else {
        stopSlideshow();
        ui.notifications.showMessage(tr("End of directory."));
        return;
      }
    }
    loadFileIndex(newIndex, false, true);
  }
  startSlideshowTimer();
}

void Core::startSlideshowTimer() {
  // start timer only for static images or single frame gifs
  auto img = model->getImage(state.currentFilePath);
  if (img && (img->type() == STATIC || img->frameCount() <= 1)) {
    slideshowTimer.start();
  }
}

void Core::jumpToFirst() {
  if (model->isEmpty())
    return;
  stopSlideshow();
  loadFileIndex(0, true, settings->usePreloader());
  ui.notifications.showDirectoryStart();
}

void Core::jumpToLast() {
  if (model->isEmpty())
    return;
  stopSlideshow();
  loadFileIndex(model->fileCount() - 1, true, settings->usePreloader());
  ui.notifications.showDirectoryEnd();
}

void Core::onLoadFailed(const QString &path) {
  ui.notifications.showMessage(tr("Load failed: ") + path);
  if (path == state.currentFilePath)
    ui.viewer.closeImage();
}

void Core::onModelItemReady(std::shared_ptr<Image> img, const QString &path) {
  if (path == state.currentFilePath) {
    state.currentImg = img;
    guiSetImage(img);
    updateInfoString();
    if (state.delayModel) {
      this->showGui();
      state.delayModel = false;
      QTimer::singleShot(ModelDelayLoadDelayMs, this, &Core::modelDelayLoad);
    }
    model->unloadExcept(state.currentFilePath, settings->usePreloader());
    if (settings->usePreloader()) {
      preloadTimer.start(PreloadDebounceDelayMs);
    }
  }
}

void Core::modelDelayLoad() {
  model->setDirectory(state.directoryPath);
  ui.shell.setDirectoryPath(state.directoryPath);
  pendingModelImageSync = true;
}

void Core::preloadNeighbors() {
  if (state.hasActiveImage && !state.currentFilePath.isEmpty()) {
    model->preload({model->nextOf(state.currentFilePath),
                    model->prevOf(state.currentFilePath)});
  }
}

void Core::onModelItemUpdated(QString filePath) {
  if (filePath == state.currentFilePath) {
    guiSetImage(model->getImage(filePath));
    updateInfoString();
  }
}

void Core::onModelSortingChanged(SortingMode mode) {
  settings->setSortingMode(mode);
  ui.shell.notifySortingChanged(mode);
  thumbPanelPresenter.reloadModel();
  thumbPanelPresenter.selectAndFocus(state.currentFilePath);
  folderViewPresenter.reloadModel();
  folderViewPresenter.selectAndFocus(state.currentFilePath);
}

void Core::onFolderSortingSelected(SortingMode mode) {
  settings->setFolderIconSortingMode(mode);
  ui.shell.notifyFolderSortingChanged(mode);
  folderViewPresenter.reloadModel();
}

void Core::onFormatFilterSelected(QStringList extensions) {
  settings->setFormatFilter(extensions);
  model->setFormatFilter(extensions);
}

void Core::onNameFilterSelected(QString nameFilter) {
  model->setNameFilter(std::move(nameFilter));
}

bool Core::needsSaveOverlay(const std::shared_ptr<Image> &img) const {
  if (!img || !img->isEdited())
    return false;
  if (!settings->skipSaveOverlayForOrientationEdits())
    return true;
  const auto *imgStatic = dynamic_cast<const ImageStatic *>(img.get());
  return !imgStatic || !imgStatic->hasOnlyOrientationEdits();
}

void Core::guiSetImage(std::shared_ptr<Image> img) {
  state.hasActiveImage = true;
  if (!img) {
    ui.notifications.showMessage(tr("Error: could not load image."));
    return;
  }
  DocumentType type = img->type();
  if (type == STATIC) {
    auto displayImage = img->getDisplayImage();
    if (!displayImage) {
      ui.notifications.showMessage(tr("Error: could not load image."));
      return;
    }
    ui.viewer.showImage(displayImage, img->filePath());
  } else if (type == ANIMATED) {
    ui.viewer.showAnimation(img->filePath(), img->format(), img->size());
  }
  ui.shell.setSaveOverlayVisible(needsSaveOverlay(img));

  // EXIF tags don't have a meaningful display order (alphabetical is fine),
  // but generation info does — QList<QPair<>> is used there instead of
  // QMap specifically to keep that order intact through to the UI.
  QList<QPair<QString, QString>> info;
  auto qimg = img->getDecodedImage();
  if (qimg) {
    QString hdrProfile = HdrToneMapper::detectHdrProfile(*qimg);
    if (!hdrProfile.isEmpty()) {
      info.append({ tr("HDR Profile"), hdrProfile });
      QString maxCll = qimg->text(QStringLiteral("HDR_MaxCLL"));
      QString maxPall = qimg->text(QStringLiteral("HDR_MaxPALL"));
      if (!maxCll.isEmpty() || !maxPall.isEmpty()) {
        QString clli = QStringLiteral("%1 / %2 nits").arg(maxCll.isEmpty() ? QStringLiteral("-") : maxCll,
                                                           maxPall.isEmpty() ? QStringLiteral("-") : maxPall);
        info.append({ tr("MaxCLL / MaxFALL"), clli });
      }
    }
  }
  const QMap<QString, QString> exifTags = img->getExifTags();
  for (auto it = exifTags.constBegin(); it != exifTags.constEnd(); ++it)
    info.append({ it.key(), it.value() });
  info.append(img->getGenerationInfo());
  ui.shell.setMetadata(info);
}

void Core::updateInfoString() {
  QSize imageSize(0, 0);
  qint64 fileSize = 0;
  bool edited = false;
  QString format;
  QString colorProfile;

  int pageIndex = 0;
  int pageCount = 1;

  if (model->isLoaded(state.currentFilePath)) {
    auto img = model->getImage(state.currentFilePath);
    if (img) {
      imageSize = img->size();
      fileSize = img->fileSize();
      edited = img->isEdited();
      format = img->format();
      if (img->type() == STATIC && img->frameCount() > 1) {
        pageCount = img->frameCount();
        pageIndex = ImageStatic::pageOverrideForPath(state.currentFilePath);
      }
      auto qimg = img->getDecodedImage();
      if (qimg) {
        QString hdrProfile = HdrToneMapper::detectHdrProfile(*qimg);
        if (!hdrProfile.isEmpty()) {
          colorProfile = hdrProfile;
        } else {
          QColorSpace cs = qimg->colorSpace();
          if (cs.isValid()) {
            QString desc = cs.description();
            if (desc.isEmpty()) {
              if (cs == QColorSpace(QColorSpace::SRgb)) {
                desc = "sRGB";
              } else if (cs == QColorSpace(QColorSpace::SRgbLinear)) {
                desc = "Linear sRGB";
              } else if (cs == QColorSpace(QColorSpace::AdobeRgb)) {
                desc = "Adobe RGB";
              } else if (cs == QColorSpace(QColorSpace::DisplayP3)) {
                desc = "Display P3";
              } else if (cs == QColorSpace(QColorSpace::ProPhotoRgb)) {
                desc = "ProPhoto RGB";
              } else if (cs == QColorSpace(QColorSpace::Bt2020)) {
                desc = "BT.2020";
              } else {
                desc = "Custom";
              }
            }
            colorProfile = desc;
          }
        }
      }
    }
  }
  int index = model->indexOfFile(state.currentFilePath);
  ShellFileInfo info;
  info.index = index;
  info.fileCount = model->fileCount();
  info.filePath = model->filePathAt(index);
  info.fileName = model->fileNameAt(index);
  info.imageSize = imageSize;
  info.fileSize = fileSize;
  info.format = format;
  info.colorProfile = colorProfile;
  info.slideshow = slideshow;
  info.shuffle = shuffle;
  info.edited = edited;
  info.pageIndex = pageIndex;
  info.pageCount = pageCount;
  ui.shell.setCurrentInfo(info);
}

void Core::suspendToStandby() {
    m_resumeFromStandby = true;
    m_lastViewMode = ui.viewMode.currentViewMode();
    stopSlideshow();
    preloadTimer.stop();
    ui.viewer.closeImage();
    this->reset();
    ui.shell.setDirectoryPath("");

    ui.window.saveWindowGeometry();
    ui.window.hideWindow();
    EmptyWorkingSet(GetCurrentProcess());
}

bool Core::hasActiveState() const {
    return state.hasActiveImage || !state.directoryPath.isEmpty();
}

void Core::loadDefaultPath() {
    if (settings->rememberLastFolder() && !settings->lastFolder().isEmpty() && QFileInfo(settings->lastFolder()).exists()) {
        loadPath(settings->lastFolder());
    } else if (settings->defaultViewMode() == MODE_FOLDERVIEW) {
        QStringList bookmarks = settings->bookmarks();
        if (!bookmarks.isEmpty() && QFileInfo(bookmarks.first()).exists()) {
            loadPath(bookmarks.first());
        } else {
            loadPath(QDir::homePath());
        }
    }
}

// exit() rather than quit(): QGuiApplication handles quit() by closing every
// top-level window first and gives up if one refuses, and the UI windows
// refuse by design (the main window turns closing into exit or standby, the
// dialogs, which persist once created, into dismiss). exit() still emits
// aboutToQuit and ends every running event loop, nested modal ones included.
void Core::forceExit() {
    ui.window.hideWindow();
    QCoreApplication::exit(EXIT_SUCCESS);
}
