#include "appbatchconversionservice.h"

#include <QDebug>

#include <memory>

#include "components/batchconverter/batchconverter.h"
#include "components/thumbnailer/thumbnailer.h"
#include "sourcecontainers/documentinfo.h"
#include "sourcecontainers/thumbnail.h"

namespace {
// The queue shows square thumbnails, as the widget dialog did.
constexpr bool kCropThumbnails = true;
constexpr bool kForceThumbnails = false;
// The files are examined one after the other, so the disk access stays
// sequential.
constexpr int kCheckThreads = 1;
} // namespace

AppBatchConversionService::AppBatchConversionService(QObject *parent)
    : BatchConversionService(parent), mConverter(new BatchConverter(this)) {
    mCheckPool.setMaxThreadCount(kCheckThreads);
    connect(mConverter, &BatchConverter::progressUpdated, this,
            &BatchConversionService::progressUpdated);
    connect(mConverter, &BatchConverter::finished, this, &BatchConversionService::finished);
    connect(mConverter, &BatchConverter::cancelled, this, &BatchConversionService::cancelled);
    connect(mConverter, &BatchConverter::startFailed, this,
            &BatchConversionService::startFailed);
}

AppBatchConversionService::~AppBatchConversionService() {
    // The worker posts to this object: let it finish before anything goes.
    mChecksStopped->store(true);
    mCheckPool.waitForDone();
    if (mConverter) {
        disconnect(mConverter, nullptr, this, nullptr);
        mConverter->cancel();
        mConverter->setParent(nullptr);
        mConverter->enableSelfDestruct();
    }
    if (mThumbnailer) {
        disconnect(mThumbnailer, nullptr, this, nullptr);
        mThumbnailer->clearTasks();
        mThumbnailer->setParent(nullptr);
        mThumbnailer->enableSelfDestruct();
    }
}

void AppBatchConversionService::start(const QStringList &paths, const QList<int> &selectedIndices,
                                      const BatchJob &job) {
    mConverter->start(paths, selectedIndices, job);
}

void AppBatchConversionService::cancel() {
    mConverter->cancel();
}

void AppBatchConversionService::requestThumbnails(const QStringList &paths, int extent) {
    if (!mThumbnailer) {
        mThumbnailer = new Thumbnailer();
        mThumbnailer->setParent(this);
        connect(mThumbnailer, &Thumbnailer::thumbnailReady, this,
                [this](std::shared_ptr<Thumbnail> thumbnail, QString filePath) {
                    if (!thumbnail) {
                        qWarning() << "Batch converter: empty thumbnail for" << filePath;
                        return;
                    }
                    emit thumbnailReady(filePath, thumbnail->image());
                });
    }
    for (const QString &path : paths)
        mThumbnailer->getThumbnailAsync(path, extent, kCropThumbnails, kForceThumbnails);
}

void AppBatchConversionService::requestAnimationCheck(const QStringList &paths) {
    const std::shared_ptr<std::atomic<bool>> stopped = mChecksStopped;
    mCheckPool.start([this, paths, stopped]() {
        for (const QString &path : paths) {
            if (stopped->load())
                return;
            if (DocumentInfo(path).type() != DocumentType::ANIMATED)
                continue;
            QMetaObject::invokeMethod(
                this,
                [this, path, stopped]() {
                    if (!stopped->load())
                        emit animationFound(path);
                },
                Qt::QueuedConnection);
        }
    });
}
