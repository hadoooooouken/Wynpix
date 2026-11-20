#pragma once

#include <QList>
#include <QMetaType>
#include <QObject>
#include <QRunnable>
#include <QSize>
#include <QString>
#include <QStringList>
#include <QThreadPool>

#include <atomic>
#include <memory>

// The size of one image, read from its header.
struct ImageSizeResult {
    QString path;
    // Invalid when the size could not be read.
    QSize size;
    // Why the size could not be read; empty when it was.
    QString error;
};

// Results of one task of the pool, for the request that started it.
struct ImageSizeBatch {
    quint64 generation = 0;
    QList<ImageSizeResult> results;
};

Q_DECLARE_METATYPE(ImageSizeBatch)

class ImageSizeNotifier final : public QObject {
    Q_OBJECT
public:
    using QObject::QObject;
    // May be called by pool threads; receivers must use queued connections.
    void reportBatch(ImageSizeBatch batch);

signals:
    void batchReady(ImageSizeBatch batch);
};

// Reads the sizes of images without decoding them, on the threads of a small
// pool, so that a view can lay images out by their proportions before their
// thumbnails exist. The size is that of the image as it is shown: the stored
// size with the EXIF orientation applied, as the thumbnailer rotates its
// thumbnails. Formats whose header cannot be read cheaply (PDF, ICO) and
// files the reader cannot open produce a result with an error and an invalid
// size; the thumbnail of such an image reports its size when it is made.
//
// GUI thread only. Results are delivered by the signals, in batches. The
// tasks of the pool run side by side, so batches may arrive in any order;
// every result names its path.
class ImageSizeProvider final : public QObject {
    Q_OBJECT
public:
    // Paths per task of the pool.
    static constexpr int kPathsPerTask = 64;
    // Threads of the pool at most: reading headers is bound by the storage.
    static constexpr int kMaximumThreads = 4;

    explicit ImageSizeProvider(QObject *parent = nullptr);
    ~ImageSizeProvider() override;

    // Drops the queued paths and the results of the tasks that already run.
    void reset();
    // Queues paths after the earlier ones.
    void request(const QStringList &paths);

    // The size of the image at path, read synchronously.
    [[nodiscard]] static ImageSizeResult readSize(const QString &path);

signals:
    void sizesReady(QList<ImageSizeResult> results);
    // All the paths requested since reset() were answered; failed of the
    // total paths gave no size.
    void finished(int total, int failed);

private slots:
    void onBatchReady(ImageSizeBatch batch);

private:
    ImageSizeNotifier mNotifier;
    QThreadPool mPool;
    // Set by reset() to stop the tasks queued or running for the previous
    // request; each task keeps its own token.
    std::shared_ptr<std::atomic_bool> mCancelled;
    quint64 mGeneration = 0;
    int mRequested = 0;
    int mAnswered = 0;
    int mFailed = 0;
};
