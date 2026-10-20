#pragma once

#include <QPointer>
#include <QThreadPool>

#include <atomic>
#include <memory>

#include "gui/quick/ui/dialogs/batchconversionservice.h"

class BatchConverter;
class Thumbnailer;

// The batch converter dialog's service in the application: BatchConverter
// converts, Thumbnailer makes the queue's thumbnails. Both are created with
// the service, one per dialog request. On destruction the service stops a
// running batch and the thumbnail tasks and leaves the converter and the
// thumbnailer to delete themselves once their workers finished, as the
// widget BatchConverterDialog did. Which files are animated (DocumentInfo's
// verdict, as in the viewer) is found on a worker thread of the service,
// which the destructor waits for.
//
// GUI thread only.
class AppBatchConversionService final : public BatchConversionService {
    Q_OBJECT

public:
    explicit AppBatchConversionService(QObject *parent = nullptr);
    ~AppBatchConversionService() override;

    void start(const QStringList &paths, const QList<int> &selectedIndices,
               const BatchJob &job) override;
    void cancel() override;
    void requestThumbnails(const QStringList &paths, int extent) override;
    void requestAnimationCheck(const QStringList &paths) override;

private:
    // Children of the service until it is destroyed; then they delete
    // themselves (enableSelfDestruct()).
    QPointer<BatchConverter> mConverter;
    QPointer<Thumbnailer> mThumbnailer;
    // Set when the service goes away; the animation check stops and posts
    // nothing more.
    std::shared_ptr<std::atomic<bool>> mChecksStopped = std::make_shared<std::atomic<bool>>(false);
    // Declared last: destroyed first, after the destructor waited for it.
    QThreadPool mCheckPool;
};
