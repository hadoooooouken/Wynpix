#pragma once

#include <QObject>
#include <QElapsedTimer>
#include <QImage>
#include <QTimer>
#include <functional>
#include <memory>
#include <optional>
#include "appversion.h"
#include "settings_types.h"
#include "components/batchconverter/batchstaging.h"
#include "components/directorymodel.h"
#include "components/directorypresenter.h"
#include "components/fileoptask/fileopcontroller.h"
#include "components/thumbnailer/thumbnailer.h"
#include "components/scriptmanager/scriptmanager.h"
#include "gui/ports/uiports.h"
#include "utils/coloradjustments.h"
#include "utils/randomizer.h"

class ColdStartWindowController;
class ImageStatic;
enum class ImageEditKind;

// How the images an edit changed are saved.
enum class EditSaving {
    None,
    // Each one at once, after the person confirmed the action.
    AfterConfirmation,
    // Each one at once.
    Immediately
};

// An edit of the source pixels of each image (Core::editImages()).
struct ImageEditRequest {
    ImageEditKind kind;
    // The action as messages name it.
    QString action;
    std::function<QImage(std::shared_ptr<const QImage>)> edit;
    EditSaving saving = EditSaving::None;
};

// Called with each image of an action and the path it is at; image is null
// when it could not be decoded.
using ImageAction =
    std::function<void(const QString &path, const std::shared_ptr<Image> &image)>;

struct State {
    bool hasActiveImage = false;
    bool delayModel = false;
    QString currentFilePath = "";
    QString directoryPath = "";
    std::shared_ptr<Image> currentImg;
};

class Core : public QObject {
    Q_OBJECT
public:
    // The user interface behind `ports` must outlive this Core.
    explicit Core(const UiPorts &ports);
    ~Core();
    void showGui();
    bool hasActiveState() const;
    void loadDefaultPath();

public slots:
    void updateInfoString();
    bool loadPath(QString);
    void raiseWindow(const QString &pathReceived = QString());
    void suspendToStandby();
    void forceExit();


private:
    void initComponents();
    void connectComponents();
    void connectUiEvents();
    void initActions();
    void onUpdate();
    void onFirstRun();
    void processRaiseWindowRequest(const QString &pathReceived);

    // ui stuff
    UiPorts ui;
    std::unique_ptr<ColdStartWindowController> coldStartWindowController;

    State state;
    bool loopSlideshow = false;
    bool slideshow = false;
    bool shuffle = false;
    FolderEndAction folderEndAction = FOLDER_END_NO_ACTION;

    // components
    std::shared_ptr<DirectoryModel> model;

    // Shared between thumbPanelPresenter and folderViewPresenter (they
    // always mirror the same DirectoryModel/folder) so a thumbnail
    // requested by both panels at once is decoded once, not twice. See
    // DirectoryPresenter::setThumbnailer().
    std::shared_ptr<Thumbnailer> thumbnailer;

    DirectoryPresenter thumbPanelPresenter, folderViewPresenter;

    // Files collected for the batch converter while its dialog is hidden by
    // "Add more"; empty otherwise.
    BatchStaging batchStaging;

    void rotateByDegrees(int degrees);
    void reset();
    bool setDirectory(QString path);

    Randomizer randomizer;
    void syncRandomizer();

    void attachModel(DirectoryModel *_model);
    QString selectedPath();
    void guiSetImage(std::shared_ptr<Image> img);
    QTimer slideshowTimer;
    QTimer preloadTimer;

    void startSlideshowTimer();
    void startSlideshow();
    void stopSlideshow();

    [[nodiscard]] ImageSaveResult saveFile(const QString &filePath,
                                           const QString &newPath);
    [[nodiscard]] ImageSaveResult saveFile(const QString &filePath);

    QList<QString> currentSelection();
    // Deletes the selected paths, or moves them to the trash, after the
    // confirmation the settings ask for.
    void removeSelection(bool trash);
    // The decoded image at path that Core already holds: the cached one, or
    // the shown one before the folder scan has registered it
    // (modelDelayLoad()).
    [[nodiscard]] std::shared_ptr<Image> heldImage(const QString &path);

    // An action over images (withPreparedImages()) and how far it got.
    struct ImageActionRun {
        QList<QString> paths;
        ImageAction onImage;
        std::function<void()> onFinished;
        qsizetype next = 0;
    };
    // Calls onImage for each path in turn with its image decoded and its SDR
    // copy built, then onFinished. A held image that is ready is passed at
    // once; any other is readied by ImagePreparer first, so an action over a
    // selection holds one image that is not cached at a time. The selection
    // can change meanwhile, so callers pass the paths.
    void withPreparedImages(QList<QString> paths, ImageAction onImage,
                            std::function<void()> onFinished = {});
    // loaded: the image ImagePreparer readied for the next path, if any.
    void continueImageAction(const std::shared_ptr<ImageActionRun> &run,
                             std::shared_ptr<Image> loaded);
    void editImages(const QList<QString> &paths, const ImageEditRequest &request);
    // Returns false when the image could not be edited.
    [[nodiscard]] bool applyEdit(const QString &path,
                                 const std::shared_ptr<Image> &image,
                                 const ImageEditRequest &request);
    [[nodiscard]] ImageEditRequest orientationEdit(
        QString action, std::function<QImage(std::shared_ptr<const QImage>)> edit);
    [[nodiscard]] ImageEditRequest resizeEdit(const ResizeRequest &request) const;
    void startAiResize(const QString &path, const std::shared_ptr<Image> &image,
                       const ResizeRequest &request);
    void applyColorAdjustmentsTo(const QString &path,
                                 const std::shared_ptr<Image> &image,
                                 const ColorAdjustments &adjustments);
    void saveImageAs(const QString &path, const std::shared_ptr<Image> &image,
                     const QString &destPath);
    void discardEditsOf(const QString &path);
    void copyImageToClipboard(const std::shared_ptr<Image> &image);
    void setImageAsWallpaper(const std::shared_ptr<Image> &image);
    void printImage(const std::shared_ptr<Image> &image);

    // True when the save overlay should be offered for the image.
    [[nodiscard]] bool needsSaveOverlay(const std::shared_ptr<Image> &img) const;

private slots:
    void onSettingsChanged();
    void readSettings();
    void nextImage();
    void prevImage();
    void nextImageSlideshow();
    void jumpToFirst();
    void jumpToLast();
    void onModelItemReady(std::shared_ptr<Image>, const QString&);
    void onModelItemUpdated(QString fileName);
    void onModelSortingChanged(SortingMode mode);
    void onFolderSortingSelected(SortingMode mode);
    void onFormatFilterSelected(QStringList extensions);
    void onNameFilterSelected(QString nameFilter);
    void onLoadFailed(const QString &path);
    void rotateLeft();
    void rotateRight();
    void nextPage();
    void prevPage();
    void firstPage();
    void lastPage();
    void goToPage(int pageIndex);
    void onUpscaleRequested(QSize size);
    void updateUpscale(const std::shared_ptr<Image> &image, QSize size,
                       const QString &path);
    void copyCurrentFile(QString destDirectory);
    void moveCurrentFile(QString destDirectory);
    void copyPathsTo(QList<QString> paths, QString destDirectory);
    void interactiveCopy(QList<QString> paths, QString destDirectory);
    void interactiveMove(QList<QString> paths, QString destDirectory);
    void movePathsTo(QList<QString> paths, QString destDirectory);
    void onDirectoryPresenterDroppedInto(QList<QString> paths, QString destDirectory, Qt::DropAction action);
    FileOpResult removeFile(QString fileName, bool trash);
    void onFileRemoved(QString filePath, int index);
    void onFileRenamed(QString fromPath, int indexFrom, QString toPath, int indexTo);
    void onFileAdded(QString filePath);
    void showResizeDialog();
    void showBatchConverter();
    void addToBatchConverter();
    void resize(const ResizeRequest &request);
    void flipH();
    void flipV();
    void crop(QRect rect);
    void cropAndSave(QRect rect);
    void discardEdits();
    void applyColorAdjustments(const ColorAdjustments &adjustments);
    void toggleCropPanel();
    void toggleFullscreenInfoBar();
    void requestSavePath();
    void saveCurrentFile();
    void saveCurrentFileAs(QString);
    void runScript(const QString&);
    void setWallpaper();
    void removePermanent();
    void moveToTrash();
    void reloadImage();
    void reloadImage(QString fileName);
    void copyFileClipboard();
    void copyPathClipboard();
    void openFromClipboard();
    void renameCurrentSelection(QString newName);
    void sortBy(SortingMode mode);
    void sortByName();
    void sortByTime();
    void sortBySize();
    void showRenameDialog();
    void onDraggedOut();
    void onDraggedOut(QList<QString> paths);
    void onDropIn(const QMimeData *mimeData, QObject* source);
    void toggleShuffle();
    void onModelLoaded();
    void outputError(const FileOpResult &error) const;
    void showInDirectory();
    void createDirectory();
    void onDirectoryViewFileActivated(QString filePath);
    void onThumbPanelDirActivated(QString dirPath);
    void onDirectoryViewFilesActivated(QList<QString> filePaths, QString activePath);
    // Opens the batch converter dialog once folderViewPresenter's background
    // scan (kicked off by showBatchConverter()) has expanded the current
    // selection into a concrete file list; see DirectoryPresenter::
    // requestExpandedSelectedPathsAsync().
    void onBatchConverterPathsReady(QList<QString> filePaths, QString defaultOutputDir);
    // Runs the dialog over batchStaging's files and keeps or drops them by
    // how the dialog closed.
    void openBatchConverter();
    bool loadFileList(const QList<QString> &filePaths, QString activePath = "");
    bool loadFileIndex(int index, bool async, bool preload);
    void enableDocumentView();
    void enableFolderView();
    void toggleFolderView();
    void toggleSlideshow();
    void onPlaybackFinished();
    void onPlaybackFailed();
    void loadParentDir();
    void nextDirectory();
    void prevDirectory(bool selectLast);
    void prevDirectory();
    void print();
    void historyBack();
    void historyForward();
    void modelDelayLoad();
    void preloadNeighbors();
    void onFileOpProgress(FileOpProgress progress);
    void onFileOpFinished(FileOpSummary summary);
    void onClearThumbnailCacheRequested();

private:
    struct AiResizeOperation {
        int generation = 0;
        QString path;
        quint64 sourceRevision = 0;
        std::weak_ptr<ImageStatic> sourceImage;
    };

    std::unique_ptr<class Upscaler> upscaler;
    std::unique_ptr<class WallpaperController> wallpaperController;
    std::unique_ptr<class MimePayloadManager> mimePayloadManager;
    std::unique_ptr<class FileOpController> fileOpController;
    std::unique_ptr<class ImagePreparer> imagePreparer;
    int aiResizeGeneration = 0;
    std::optional<AiResizeOperation> activeAiResizeOperation;
    bool aiResizeBusyUiActive = false;
    void clearAiResizeBusyUi();

private slots:
    void onAiResizeFinished(int generation, QString path, QImage image, bool success, QString error);

private:
    bool m_raiseWindowConcealed = false;
    bool m_raiseWindowAwaitingDocumentRendering = false;
    bool m_raiseWindowDocumentRenderingSettled = false;
    QTimer m_raiseWindowRevealTimer;
    QElapsedTimer m_lastRaiseRequestTimer;

    QStringList backHistory, forwardHistory;
    bool blockHistory = false;
    // Directory scanning (DirectoryManager/DirectoryScanner) runs on a
    // background thread, so the folder view's model is not ready yet when
    // loadPath() returns - selecting/scrolling to a path synchronously right
    // after it would silently no-op. Instead, the path we want highlighted
    // once the new directory finishes loading is stashed here and applied
    // in onModelLoaded() (mirrors the dirTreeView's m_pendingScrollPath
    // approach in FolderView).
    QString pendingFolderViewSelectPath;
    // When a directory is activated from the thumbnail panel ("Show
    // subfolders" mode) or adjacent folder navigation, we navigate into it
    // while staying in document mode. Because directory scanning is async,
    // the model is empty when setDirectory() returns. This enum tells
    // onModelLoaded() to auto-load the target image once the scan finishes.
    enum class PendingDocumentLoad {
        None,
        FirstImage,
        LastImage
    };
    PendingDocumentLoad m_pendingDocumentLoad = PendingDocumentLoad::None;
    // modelDelayLoad() displays the already-decoded image before the
    // directory scan has run, so DirectoryModel::updateImage() can't
    // register it in the model's cache yet - containsFile() stays false
    // until the async scan populates fileLookupMap. Stash the need to do
    // so and flush it once the scan actually finishes, in onModelLoaded()
    // (same pattern as pendingFolderViewSelectPath above).
    bool pendingModelImageSync = false;
    ViewMode m_lastViewMode = MODE_DOCUMENT;
    QString m_lastFilePath;
    bool m_resumeFromStandby = false;
    bool lastCMEnabled = false;
    QString lastCMType = "";
    QString lastCMPath = "";
    bool lastHdrEnabled = true;
    int lastHdrOp = 0;
    int lastHdrWhite = 203;
    int lastThumbnailResolution = 0;
    bool lastShowSubfoldersInPanel = false;
    bool lastShowHiddenFiles = false;
    int lastPanelPreviewsSize = 0;
    bool lastSortFolders = false;
    SortingMode lastFolderIconSortingMode = SORT_NAME;
    ThumbPanelStyle lastThumbPanelStyle = TH_PANEL_SIMPLE;
};
