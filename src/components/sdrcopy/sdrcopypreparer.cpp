#include "components/sdrcopy/sdrcopypreparer.h"

#include "sourcecontainers/imagestatic.h"

#include <QDebug>
#include <QMetaObject>

#include <algorithm>
#include <utility>

namespace {
constexpr int WorkerThreadCount = 1;

[[nodiscard]] ImageStatic *staticImage(const std::shared_ptr<Image> &image) {
    return dynamic_cast<ImageStatic *>(image.get());
}
} // namespace

SdrCopyPreparer::SdrCopyPreparer(QObject *parent) : QObject(parent) {
    mPool.setMaxThreadCount(WorkerThreadCount);
}

SdrCopyPreparer::~SdrCopyPreparer() {
    mPool.waitForDone();
}

bool SdrCopyPreparer::isReady(const QList<std::shared_ptr<Image>> &images) {
    return std::ranges::none_of(images, [](const std::shared_ptr<Image> &image) {
        ImageStatic *imageStatic = staticImage(image);
        return imageStatic && imageStatic->needsSdrConversion();
    });
}

void SdrCopyPreparer::prepare(QList<std::shared_ptr<Image>> images,
                              std::function<void()> onPrepared) {
    if (++mPending == 1)
        emit busyChanged(true);
    mPool.start([this, images = std::move(images),
                 onPrepared = std::move(onPrepared)]() {
        for (const std::shared_ptr<Image> &image : images) {
            if (ImageStatic *imageStatic = staticImage(image))
                imageStatic->prepareSdrCopy();
        }
        const bool queued = QMetaObject::invokeMethod(
            this, [this, onPrepared]() { finish(onPrepared); },
            Qt::QueuedConnection);
        if (!queued)
            qWarning() << "SdrCopyPreparer: could not queue the finished preparation";
    });
}

void SdrCopyPreparer::finish(const std::function<void()> &onPrepared) {
    if (--mPending == 0)
        emit busyChanged(false);
    if (onPrepared)
        onPrepared();
}
