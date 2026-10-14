#pragma once

#include <memory>
#include <optional>

#include <QMetaType>
#include <QObject>
#include <QRunnable>
#include "utils/imagefactory.h"

struct ImageLoadRequest {
    QString path;
    quint64 taskId = 0;
    DecodeContext decodeContext;
    // Set for preloads: an image whose decoded pixels would exceed it is not
    // kept.
    std::optional<qint64> decodedBytesLimit;
};

enum class ImageLoadCompletionStatus {
    RemovedBeforeRun,
    Finished,
    // The image exceeds the request's decodedBytesLimit; not an error.
    OverMemoryBudget,
};

struct ImageLoadCompletion {
    quint64 taskId = 0;
    ImageLoadCompletionStatus status =
        ImageLoadCompletionStatus::RemovedBeforeRun;
    std::shared_ptr<Image> image;
};

Q_DECLARE_METATYPE(ImageLoadCompletion)

class LoaderTaskNotifier final : public QObject {
    Q_OBJECT
public:
    using QObject::QObject;
    // May be called by pool threads; receivers must use queued connections.
    void reportCompletion(ImageLoadCompletion completion);

signals:
    void taskCompleted(ImageLoadCompletion completion);
};

class LoaderRunnable final : public QRunnable {
public:
    LoaderRunnable(ImageLoadRequest request, LoaderTaskNotifier &notifier);
    ~LoaderRunnable() override;
    void run() override;

private:
    ImageLoadRequest request;
    LoaderTaskNotifier &notifier;
    ImageLoadCompletion completion;
};
