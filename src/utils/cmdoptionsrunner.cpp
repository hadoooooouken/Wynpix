#include "cmdoptionsrunner.h"
#include "components/directorymanager/directorymanager.h"
#include "components/thumbnailer/thumbnailer.h"

#include <QEventLoop>

void CmdOptionsRunner::generateThumbs(QString dirPath, int size) {
    if(size <= 50 || size > 400) {
        qWarning() << "Error: Invalid thumbnail size.";
        qInfo() << "Please specify a value between [50, 400].";
        qInfo() << "Example:  wynpix --gen-thumbs=/home/user/Pictures/ --gen-thumbs-size=120";
        QCoreApplication::exit(1);
        return;
    }

    Thumbnailer th;
    DirectoryManager dm;
    // The scan runs on the thread pool and delivers the list with loaded().
    QEventLoop scanLoop;
    connect(&dm, &DirectoryManager::loaded, &scanLoop, &QEventLoop::quit);
    if(!dm.setDirectoryRecursive(dirPath)) {
        qWarning() << "Error: Invalid path.";
        QCoreApplication::exit(1);
        return;
    }
    scanLoop.exec();

    auto list = dm.fileList();

    qInfo() << "\nDirectory:" << dirPath;
    qInfo() << "File count:" << list.size();
    qInfo() << "Size limit:" << size << "x" << size << "px";
    qInfo() << "Generating thumbnails...";

    // Finished thumbnails reach the cache writer through queued completions
    // on this thread, so the event loop must run until every file reports.
    qsizetype pending = list.size();
    QEventLoop thumbLoop;
    auto finishOne = [&pending, &thumbLoop] {
        if(--pending == 0)
            thumbLoop.quit();
    };
    connect(&th, &Thumbnailer::thumbnailReady, &thumbLoop, finishOne);
    connect(&th, &Thumbnailer::thumbnailFailed, &thumbLoop, finishOne);

    for(const auto &path : std::as_const(list))
        th.getThumbnailAsync(path, size, false, false);
    if(pending > 0)
        thumbLoop.exec();

    th.waitForDone();
    qInfo() << "\nDone.";
    QCoreApplication::quit();
}

void CmdOptionsRunner::showBuildOptions() {
    QStringList features;
#ifdef USE_EXIV2
    features << "USE_EXIV2";
#endif
    qInfo() << "\nEnabled build options:";
    if(!features.count())
        qInfo() << "   --";
    for(int i = 0; i < features.count(); i++)
        qInfo() << "   " << features.at(i);
    QCoreApplication::quit();
}

