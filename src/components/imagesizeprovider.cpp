#include "imagesizeprovider.h"

#include <QDebug>
#include <QDir>
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

ImageSizeProvider::ImageSizeProvider(int maxCacheEntries, QObject *parent)
    : QObject(parent),
      mMaxCacheEntries(qMax(1, maxCacheEntries)),
      mCancelled(std::make_shared<std::atomic_bool>(false)) {
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
    mInvalidatedPaths.clear();
}

QString ImageSizeProvider::normalizePath(const QString &path) {
    if (path.isEmpty())
        return {};
    return QDir::cleanPath(QDir::fromNativeSeparators(path)).toCaseFolded();
}

void ImageSizeProvider::request(const QStringList &paths) {
    Q_ASSERT(QThread::currentThread() == thread());
    if (paths.isEmpty())
        return;

    QList<ImageSizeResult> cachedResults;
    QStringList uncachedPaths;

    for (const QString &path : paths) {
        const QString key = normalizePath(path);
        auto it = mCache.find(key);
        if (it != mCache.end()) {
            mRecency.splice(mRecency.begin(), mRecency, it->recencyPosition);
            cachedResults.append(ImageSizeResult{
                .path = path,
                .size = it->size,
                .error = QString(),
            });
        } else {
            uncachedPaths.append(path);
        }
    }

    const quint64 generation = mGeneration;
    mRequested += static_cast<int>(paths.size());

    // Schedule background workers before emitting any synchronous signals to
    // avoid state corruption if a connected slot invokes reset().
    for (qsizetype first = 0; first < uncachedPaths.size(); first += kPathsPerTask) {
        mPool.start(new ImageSizeRunnable(uncachedPaths.mid(first, kPathsPerTask), generation,
                                          mCancelled, mNotifier));
    }

    if (!cachedResults.isEmpty()) {
        mAnswered += static_cast<int>(cachedResults.size());
        emit sizesReady(cachedResults);
        if (mGeneration != generation)
            return;
    }

    if (mAnswered >= mRequested)
        emit finished(mRequested, mFailed);
}

QSize ImageSizeProvider::cachedSize(const QString &path) const {
    Q_ASSERT(QThread::currentThread() == thread());
    if (path.isEmpty())
        return {};
    const QString key = normalizePath(path);
    auto it = mCache.find(key);
    if (it == mCache.end())
        return {};
    auto &recency = const_cast<std::list<QString> &>(mRecency);
    recency.splice(recency.begin(), recency, it->recencyPosition);
    return it->size;
}

void ImageSizeProvider::insert(const QString &path, const QSize &size) {
    Q_ASSERT(QThread::currentThread() == thread());
    if (path.isEmpty() || !size.isValid() || size.isEmpty())
        return;
    const QString key = normalizePath(path);
    mInvalidatedPaths.remove(key);
    insertCacheEntry(key, size);
}

void ImageSizeProvider::invalidate(const QString &path) {
    Q_ASSERT(QThread::currentThread() == thread());
    if (path.isEmpty())
        return;
    const QString key = normalizePath(path);
    mInvalidatedPaths.insert(key);
    auto it = mCache.find(key);
    if (it != mCache.end()) {
        mRecency.erase(it->recencyPosition);
        mCache.erase(it);
    }
}

void ImageSizeProvider::clearCache() {
    Q_ASSERT(QThread::currentThread() == thread());
    mCache.clear();
    mRecency.clear();
    mInvalidatedPaths.clear();
}

void ImageSizeProvider::insertCacheEntry(const QString &key, const QSize &size) {
    auto it = mCache.find(key);
    if (it != mCache.end()) {
        it->size = size;
        mRecency.splice(mRecency.begin(), mRecency, it->recencyPosition);
        return;
    }
    if (static_cast<int>(mCache.size()) >= mMaxCacheEntries && !mRecency.empty()) {
        const QString oldest = mRecency.back();
        mRecency.pop_back();
        mCache.remove(oldest);
    }
    mRecency.push_front(key);
    mCache.insert(key, CacheEntry{size, mRecency.begin()});
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
    for (const ImageSizeResult &result : std::as_const(batch.results)) {
        if (!result.size.isValid() || result.size.isEmpty())
            continue;
        const QString key = normalizePath(result.path);
        if (mInvalidatedPaths.contains(key))
            continue;
        insertCacheEntry(key, result.size);
    }
    mAnswered += static_cast<int>(batch.results.size());
    mFailed += static_cast<int>(std::count_if(batch.results.cbegin(), batch.results.cend(),
                                              [](const ImageSizeResult &result) {
                                                  return !result.size.isValid();
                                              }));
    emit sizesReady(batch.results);
    if (mAnswered >= mRequested)
        emit finished(mRequested, mFailed);
}
