#include "components/imagepreparer/imagepreparer.h"

#include "sourcecontainers/imagestatic.h"
#include "utils/imagefactory.h"

#include <QDebug>
#include <QMetaObject>

#include <utility>

namespace {
constexpr int WorkerThreadCount = 1;
} // namespace

ImagePreparer::ImagePreparer(QObject *parent) : QObject(parent) {
    mPool.setMaxThreadCount(WorkerThreadCount);
}

ImagePreparer::~ImagePreparer() {
    mPool.waitForDone();
}

bool ImagePreparer::isReady(const std::shared_ptr<Image> &image) {
    if (!image)
        return false;
    auto *imageStatic = dynamic_cast<ImageStatic *>(image.get());
    return !imageStatic || !imageStatic->needsSdrConversion();
}

void ImagePreparer::prepare(QString path, std::shared_ptr<Image> image,
                            Prepared onPrepared) {
    if (++mPending == 1)
        emit busyChanged(true);
    mPool.start([this, path = std::move(path), image = std::move(image),
                 onPrepared = std::move(onPrepared)]() mutable {
        if (!image) {
            image = ImageFactory::createImage(path);
            if (image && !image->isLoaded())
                image.reset();
            if (!image)
                qWarning() << "ImagePreparer: could not decode" << path;
        }
        if (auto *imageStatic = dynamic_cast<ImageStatic *>(image.get()))
            imageStatic->prepareSdrCopy();
        const bool queued = QMetaObject::invokeMethod(
            this,
            [this, onPrepared, image = std::move(image)]() mutable {
                finish(onPrepared, std::move(image));
            },
            Qt::QueuedConnection);
        if (!queued)
            qWarning() << "ImagePreparer: could not queue the prepared image" << path;
    });
}

void ImagePreparer::finish(const Prepared &onPrepared,
                           std::shared_ptr<Image> image) {
    if (--mPending == 0)
        emit busyChanged(false);
    if (onPrepared)
        onPrepared(std::move(image));
}
