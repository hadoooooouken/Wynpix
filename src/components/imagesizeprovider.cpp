#include "imagesizeprovider.h"

#include <QDebug>
#include <QImageIOHandler>
#include <QImageReader>
#include <QThread>

#include <algorithm>
#include <utility>

#include "sourcecontainers/documentinfo.h"

namespace {
// Formats the image reader cannot size from the header alone; the thumbnailer
// has its own loaders for them.
bool isHeaderSizeUnavailable(const QString &format) {
    return format == QLatin1String("pdf") || format == QLatin1String("ico");
}

// The orientations (QImageIOHandler::Transformation, as DocumentInfo stores
// them) that turn the image by a quarter, which the thumbnailer applies to its
// thumbnails (ImageLib::exifRotated()): they swap width and height.
bool swapsWidthAndHeight(int orientation) {
    switch (static_cast<QImageIOHandler::Transformation>(orientation)) {
    case QImageIOHandler::TransformationRotate90:
    case QImageIOHandler::TransformationMirrorAndRotate90:
    case QImageIOHandler::TransformationFlipAndRotate90:
    case QImageIOHandler::TransformationRotate270:
        return true;
    default:
        return false;
    }
}

class ImageSizeRunnable final : public QRunnable {
public:
    ImageSizeRunnable(QStringList paths, quint64 generation,
                      std::shared_ptr<std::atomic_bool> cancelled, ImageSizeNotifier &notifier)
        : mPaths(std::move(paths)),
          mGeneration(generation),
          mCancelled(std::move(cancelled)),
          mNotifier(notifier) {}

    void run() override {
        ImageSizeBatch batch;
        batch.generation = mGeneration;
        batch.results.reserve(mPaths.size());
        for (const QString &path : std::as_const(mPaths)) {
            if (mCancelled->load())
                return;
            batch.results.append(ImageSizeProvider::readSize(path));
        }
        mNotifier.reportBatch(std::move(batch));
    }

private:
    QStringList mPaths;
    quint64 mGeneration;
    std::shared_ptr<std::atomic_bool> mCancelled;
    ImageSizeNotifier &mNotifier;
};
} // namespace

void ImageSizeNotifier::reportBatch(ImageSizeBatch batch) {
    emit batchReady(std::move(batch));
}

ImageSizeProvider::ImageSizeProvider(QObject *parent)
    : QObject(parent), mCancelled(std::make_shared<std::atomic_bool>(false)) {
    qRegisterMetaType<ImageSizeBatch>();
    qRegisterMetaType<QList<ImageSizeResult>>();
    mPool.setMaxThreadCount(
        qBound(1, QThread::idealThreadCount() / 2, kMaximumThreads));
    connect(&mNotifier, &ImageSizeNotifier::batchReady, this, &ImageSizeProvider::onBatchReady,
            Qt::QueuedConnection);
}

ImageSizeProvider::~ImageSizeProvider() {
    mCancelled->store(true);
    mPool.clear();
    // The tasks use the notifier, which is destroyed after this body.
    mPool.waitForDone();
}

void ImageSizeProvider::reset() {
    mCancelled->store(true);
    mPool.clear();
    mCancelled = std::make_shared<std::atomic_bool>(false);
    ++mGeneration;
    mRequested = 0;
    mAnswered = 0;
    mFailed = 0;
}

void ImageSizeProvider::request(const QStringList &paths) {
    for (qsizetype first = 0; first < paths.size(); first += kPathsPerTask) {
        mPool.start(new ImageSizeRunnable(paths.mid(first, kPathsPerTask), mGeneration, mCancelled,
                                          mNotifier));
    }
    mRequested += static_cast<int>(paths.size());
}

ImageSizeResult ImageSizeProvider::readSize(const QString &path) {
    ImageSizeResult result;
    result.path = path;
    const DocumentInfo info(path);
    if (info.type() == DocumentType::NONE) {
        result.error = QStringLiteral("not a supported image");
        return result;
    }
    const QString format = info.format();
    if (isHeaderSizeUnavailable(format)) {
        result.error = QStringLiteral("the size of the %1 format needs the thumbnail").arg(format);
        return result;
    }
    QImageReader reader(info.filePath(), format.toLatin1());
    const QSize size = reader.size();
    if (!size.isValid()) {
        result.error = reader.errorString();
        if (result.error.isEmpty())
            result.error = QStringLiteral("the reader gave no size");
        return result;
    }
    result.size = swapsWidthAndHeight(info.exifOrientation()) ? size.transposed() : size;
    return result;
}

void ImageSizeProvider::onBatchReady(ImageSizeBatch batch) {
    // A batch of an earlier request, queued before reset().
    if (batch.generation != mGeneration)
        return;
    mAnswered += static_cast<int>(batch.results.size());
    mFailed += static_cast<int>(std::count_if(batch.results.cbegin(), batch.results.cend(),
                                              [](const ImageSizeResult &result) {
                                                  return !result.size.isValid();
                                              }));
    emit sizesReady(batch.results);
    if (mAnswered >= mRequested)
        emit finished(mRequested, mFailed);
}
