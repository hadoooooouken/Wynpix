#include "fileoptaskrunnable.h"

#include <QCoreApplication>
#include <QDebug>
#include <QMetaObject>
#include <QSemaphore>
#include <optional>
#include <type_traits>
#include <utility>

#include "components/directorymodel.h"
#include "components/fileoptask/fileopcontroller.h"

namespace {

// How often a worker waiting on a GUI-thread call re-checks cancellation.
// Bounds how long ~FileOpController() waits for an in-flight task once the
// GUI event loop has ended.
constexpr int kGuiCallCancelPollMs = 50;

template <typename Result>
struct GuiCallState {
    QSemaphore done;
    std::optional<Result> result;
};

// Owned by the queued call; wakes the waiting worker when the call is
// destroyed, whether it ran, skipped its work after cancellation, or was
// discarded together with its context object.
template <typename Result>
class GuiCallCompletion {
public:
    explicit GuiCallCompletion(std::shared_ptr<GuiCallState<Result>> state)
        : state(std::move(state)) {}
    ~GuiCallCompletion() { state->done.release(); }
    GuiCallCompletion(const GuiCallCompletion &) = delete;
    GuiCallCompletion &operator=(const GuiCallCompletion &) = delete;

    void setResult(Result result) { state->result = std::move(result); }

private:
    std::shared_ptr<GuiCallState<Result>> state;
};

// Runs fn on context's thread and waits for its result. Replaces
// Qt::BlockingQueuedConnection, which cannot be abandoned: once the GUI event
// loop has ended, ~FileOpController() would wait for the worker while the
// worker waited for a call the GUI thread never runs. Here the worker gives up
// as soon as cancelled is set, and a call still queued at that point skips fn,
// so no file operation or prompt starts after cancellation. The result lives
// in shared state rather than on the worker's stack, so an abandoned call
// never writes into a finished frame. Returns std::nullopt when cancelled or
// when the call was discarded without running.
template <typename Fn>
std::optional<std::invoke_result_t<Fn &>> invokeOnGuiThread(
    QObject *context, const std::shared_ptr<std::atomic<bool>> &cancelled, Fn fn) {
    using Result = std::invoke_result_t<Fn &>;
    if (!context || !cancelled) {
        qWarning() << "File operation GUI call has no context or cancellation flag";
        return std::nullopt;
    }
    auto state = std::make_shared<GuiCallState<Result>>();
    auto completion = std::make_shared<GuiCallCompletion<Result>>(state);
    const bool queued = QMetaObject::invokeMethod(
        context,
        [completion = std::move(completion), cancelled, fn = std::move(fn)]() mutable {
            if (!cancelled->load())
                completion->setResult(fn());
        },
        Qt::QueuedConnection);
    if (!queued) {
        qWarning() << "File operation GUI call could not be queued";
        return std::nullopt;
    }
    while (!state->done.tryAcquire(1, kGuiCallCancelPollMs)) {
        if (cancelled->load())
            return std::nullopt;
    }
    return std::move(state->result);
}

bool isDestinationInsideSource(const QString &srcPath, const QString &destDirectory) {
    QFileInfo srcFi(srcPath);
    if (!srcFi.isDir()) {
        return false;
    }

    QString cleanSrc = srcFi.canonicalFilePath();
    if (cleanSrc.isEmpty()) {
        cleanSrc = QDir::cleanPath(srcFi.absoluteFilePath());
    }

    QFileInfo dstFi(destDirectory);
    QString cleanDst = dstFi.canonicalFilePath();
    if (cleanDst.isEmpty()) {
        cleanDst = QDir::cleanPath(dstFi.absoluteFilePath());
    }

    cleanSrc = QDir::cleanPath(cleanSrc);
    cleanDst = QDir::cleanPath(cleanDst);

    if (cleanSrc.isEmpty() || cleanDst.isEmpty()) {
        return false;
    }

    if (QString::compare(cleanSrc, cleanDst, Qt::CaseInsensitive) == 0) {
        return true;
    }

    if (!cleanSrc.endsWith(u'/')) {
        cleanSrc.append(u'/');
    }

    return cleanDst.startsWith(cleanSrc, Qt::CaseInsensitive);
}

} // namespace

void FileOpTaskNotifier::reportProgress(FileOpProgress progress) {
    emit progressReported(std::move(progress));
}

void FileOpTaskNotifier::reportFinished(FileOpSummary summary) {
    emit operationFinished(std::move(summary));
}

void FileOpTaskNotifier::reportError(QString message) {
    emit errorReported(std::move(message));
}

FileOperationTask::FileOperationTask(FileOpRequest request,
                                     QPointer<DirectoryModel> model,
                                     QPointer<FileOpController> controller,
                                     FileOpTaskNotifier &notifier,
                                     std::shared_ptr<std::atomic<bool>> cancelled)
    : request(std::move(request)),
      model(model),
      controller(controller),
      notifier(notifier),
      cancelled(std::move(cancelled)) {
    setAutoDelete(true);
}

bool FileOperationTask::isCancelled() const {
    return cancelled && cancelled->load();
}

FileReplaceDecision FileOperationTask::requestReplaceDecision(
    const FileReplaceRequest &replaceRequest) {
    if (!controller) {
        qWarning() << "File operation controller is gone; skipping"
                   << replaceRequest.targetPath;
        return {};
    }
    const std::optional<FileReplaceDecision> decision = invokeOnGuiThread(
        controller.data(), cancelled, [controllerPtr = controller, replaceRequest]() {
            if (!controllerPtr)
                return FileReplaceDecision{};
            return controllerPtr->resolveFileReplace(replaceRequest);
        });
    if (!decision && isCancelled())
        return {.yes = false, .all = false, .cancel = true};
    return decision.value_or(FileReplaceDecision{});
}

FileOpResult FileOperationTask::moveFileOnGuiThread(const QString &path,
                                                    const QString &destDirectory,
                                                    bool force) {
    if (!model)
        return FileOpResult::OTHER_ERROR;
    return invokeOnGuiThread(model.data(), cancelled,
                             [modelPtr = model, path, destDirectory, force]() {
                                 FileOpResult result = FileOpResult::OTHER_ERROR;
                                 if (modelPtr)
                                     modelPtr->moveFileTo(path, destDirectory, force, result);
                                 return result;
                             })
        .value_or(FileOpResult::OTHER_ERROR);
}

FileOpResult FileOperationTask::removeDirOnGuiThread(const QString &dirPath) {
    if (!model)
        return FileOpResult::OTHER_ERROR;
    return invokeOnGuiThread(model.data(), cancelled,
                             [modelPtr = model, dirPath]() {
                                 FileOpResult result = FileOpResult::OTHER_ERROR;
                                 if (modelPtr)
                                     modelPtr->removeDir(dirPath, false, false, result);
                                 return result;
                             })
        .value_or(FileOpResult::OTHER_ERROR);
}

void FileOperationTask::run() {
    FileReplaceDecision overwriteFiles;
    for (const auto &path : std::as_const(request.paths)) {
        if (isCancelled())
            break;
        if (request.isMove)
            processMove(path, request.destDirectory, overwriteFiles);
        else
            processCopy(path, request.destDirectory, overwriteFiles);
        if (overwriteFiles.cancel || isCancelled())
            break;
    }

    FileOpSummary summary;
    summary.cancelled = overwriteFiles.cancel || isCancelled();
    summary.filesProcessed = filesProcessed;
    summary.isMove = request.isMove;
    notifier.reportFinished(std::move(summary));
}

// SINGLE FILE / DIR COPY, recursive.
// Direct port of the former Core::doInteractiveCopy(); the file-replace
// prompt runs on the GUI thread through FileOpController, and errors are
// reported through FileOpTaskNotifier instead of from this worker thread.
void FileOperationTask::processCopy(const QString &path, const QString &destDirectory,
                                    FileReplaceDecision &overwriteFiles) {
    if (isCancelled())
        return;

    QFileInfo srcFi(path);
    if (srcFi.isDir() && isDestinationInsideSource(path, destDirectory)) {
        notifier.reportError(QCoreApplication::translate(
            "FileOperationTask",
            "Cannot copy a directory into itself or a subdirectory of itself."));
        return;
    }
    // SINGLE FILE COPY
    // ===========================================================================
    if (!srcFi.isDir()) {
        FileOpResult result;
        FileOperations::copyFileTo(path, destDirectory, overwriteFiles, result);
        if (result == FileOpResult::DESTINATION_FILE_EXISTS) {
            if (overwriteFiles.all) // skipping all
                return;
            FileReplaceDecision dialogResult;
            QString srcPath = srcFi.absoluteFilePath();
            QString dstPath = destDirectory + "/" + srcFi.fileName();
            dialogResult = requestReplaceDecision({srcPath, dstPath, FILE_TO_FILE, true});
            overwriteFiles = dialogResult;
            if (!overwriteFiles || overwriteFiles.cancel)
                return;
            FileOperations::copyFileTo(path, destDirectory, true, result);
        }
        if (result != FileOpResult::SUCCESS &&
            !(result == FileOpResult::DESTINATION_FILE_EXISTS && !overwriteFiles)) {
            QString errorText = FileOperations::decodeResult(result);
            notifier.reportError(errorText);
            qDebug() << errorText;
        }
        if (result == FileOpResult::SUCCESS) {
            ++filesProcessed;
            notifier.reportProgress({path, filesProcessed, /*isMove=*/false});
        }
        if (!overwriteFiles.all) // copy attempt done; reset temporary flag
            overwriteFiles.yes = false;
        return;
    }
    // DIR COPY (RECURSIVE)
    // =======================================================================
    QDir srcDir(srcFi.absoluteFilePath());
    QFileInfo dstFi(destDirectory + "/" + srcFi.fileName());
    QDir dstDir(dstFi.absoluteFilePath());
    if (dstFi.exists() && !dstFi.isDir()) { // overwriting file with a folder
        if (!overwriteFiles && !overwriteFiles.all) {
            FileReplaceDecision dialogResult;
            QString srcPath = srcFi.absoluteFilePath();
            QString dstPath = dstFi.absoluteFilePath();
            dialogResult = requestReplaceDecision({srcPath, dstPath, DIR_TO_FILE, true});
            overwriteFiles = dialogResult;
            if (!overwriteFiles || overwriteFiles.cancel)
                return;
            if (!overwriteFiles.all) // reset temp flag right away
                overwriteFiles.yes = false;
        }
        // remove dst file; give up if not writable
        FileOpResult result;
        FileOperations::removeFile(dstFi.absoluteFilePath(), result);
        if (result != FileOpResult::SUCCESS) {
            QString errorText = FileOperations::decodeResult(result);
            notifier.reportError(errorText);
            qDebug() << errorText;
            return;
        }
    } else if (!dstDir.mkpath(".")) {
        QString dirPath = dstDir.absolutePath();
        notifier.reportError(QCoreApplication::translate(
            "FileOperationTask", "Could not create directory ") + dirPath);
        qDebug() << "Could not create directory " << dirPath;
        return;
    }
    // copy all contents
    QStringList entryList =
        srcDir.entryList(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot |
                         QDir::Hidden | QDir::System);
    for (const auto &entry : std::as_const(entryList)) {
        if (isCancelled())
            return;
        processCopy(srcDir.absolutePath() + "/" + entry, dstDir.absolutePath(), overwriteFiles);
        if (overwriteFiles.cancel)
            return;
    }
}

// SINGLE FILE / DIR MOVE, recursive.
// Direct port of the former Core::doInteractiveMove(). In addition to the
// file-replace prompt, model->moveFileTo() and model->removeDir()
// both also mutate DirectoryManager bookkeeping (see DirectoryModel::
// moveFileTo()'s "chew through watcher events" comment) and so are wrapped
// the same way as the dialog calls, not left to run on this worker thread.
void FileOperationTask::processMove(const QString &path, const QString &destDirectory,
                                    FileReplaceDecision &overwriteFiles) {
    if (isCancelled())
        return;

    QFileInfo srcFi(path);
    if (srcFi.isDir() && isDestinationInsideSource(path, destDirectory)) {
        notifier.reportError(QCoreApplication::translate(
            "FileOperationTask",
            "Cannot move a directory into itself or a subdirectory of itself."));
        return;
    }
    // SINGLE FILE MOVE
    // ===========================================================================
    if (!srcFi.isDir()) {
        FileOpResult result = moveFileOnGuiThread(path, destDirectory, overwriteFiles);
        if (isCancelled())
            return;
        if (result == FileOpResult::DESTINATION_FILE_EXISTS) {
            if (overwriteFiles.all) // skipping all
                return;
            FileReplaceDecision dialogResult;
            QString srcPath = srcFi.absoluteFilePath();
            QString dstPath = destDirectory + "/" + srcFi.fileName();
            dialogResult = requestReplaceDecision({srcPath, dstPath, FILE_TO_FILE, true});
            overwriteFiles = dialogResult;
            if (!overwriteFiles || overwriteFiles.cancel)
                return;
            result = moveFileOnGuiThread(path, destDirectory, true);
            if (isCancelled())
                return;
        }
        if (result != FileOpResult::SUCCESS &&
            !(result == FileOpResult::DESTINATION_FILE_EXISTS && !overwriteFiles)) {
            QString errorText = FileOperations::decodeResult(result);
            notifier.reportError(errorText);
            qDebug() << errorText;
        }
        if (result == FileOpResult::SUCCESS) {
            ++filesProcessed;
            notifier.reportProgress({path, filesProcessed, /*isMove=*/true});
        }
        if (!overwriteFiles.all) // move attempt done; reset temporary flag
            overwriteFiles.yes = false;
        return;
    }
    // DIR MOVE (RECURSIVE)
    // =======================================================================
    QDir srcDir(srcFi.absoluteFilePath());
    QFileInfo dstFi(destDirectory + "/" + srcFi.fileName());
    QDir dstDir(dstFi.absoluteFilePath());
    if (dstFi.exists() && !dstFi.isDir()) { // overwriting file with a folder
        if (!overwriteFiles && !overwriteFiles.all) {
            FileReplaceDecision dialogResult;
            QString srcPath = srcFi.absoluteFilePath();
            QString dstPath = dstFi.absoluteFilePath();
            dialogResult = requestReplaceDecision({srcPath, dstPath, DIR_TO_FILE, true});
            overwriteFiles = dialogResult;
            if (!overwriteFiles || overwriteFiles.cancel)
                return;
            if (!overwriteFiles.all) // reset temp flag right away
                overwriteFiles.yes = false;
        }
        // remove dst file; give up if not writable
        FileOpResult result;
        FileOperations::removeFile(dstFi.absoluteFilePath(), result);
        if (result != FileOpResult::SUCCESS) {
            QString errorText = FileOperations::decodeResult(result);
            notifier.reportError(errorText);
            qDebug() << errorText;
            return;
        }
    } else if (!dstDir.mkpath(".")) {
        QString dirPath = dstDir.absolutePath();
        notifier.reportError(QCoreApplication::translate(
            "FileOperationTask", "Could not create directory ") + dirPath);
        qDebug() << "Could not create directory " << dirPath;
        return;
    }
    // move all contents
    QStringList entryList =
        srcDir.entryList(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot |
                         QDir::Hidden | QDir::System);
    for (const auto &entry : std::as_const(entryList)) {
        if (isCancelled())
            return;
        processMove(srcDir.absolutePath() + "/" + entry, dstDir.absolutePath(), overwriteFiles);
        if (overwriteFiles.cancel)
            return;
    }
    // Clean up the now-empty source directory. FileOperations::removeDir()
    // only ever returns SUCCESS, SOURCE_DOES_NOT_EXIST, DIRECTORY_NOT_EMPTY,
    // or OTHER_ERROR - never a "nothing to do" style soft-success - so any
    // non-SUCCESS result here is a genuine failure worth surfacing, same as
    // every other FileOpResult check in this file. The original
    // Core::doInteractiveMove() discarded this result silently; that was a
    // pre-existing gap, not something Phase 1 needed to preserve.
    const FileOpResult dirRmRes = removeDirOnGuiThread(srcDir.absolutePath());
    if (isCancelled())
        return;
    if (dirRmRes != FileOpResult::SUCCESS) {
        QString errorText = FileOperations::decodeResult(dirRmRes);
        notifier.reportError(errorText);
        qDebug() << errorText;
    }
}
