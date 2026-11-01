#include "navigationpreview.h"

#include <utility>

#include <QDebug>

namespace {
// The thumbnail cache keeps uncropped thumbnails; a cached one is served
// as it is.
constexpr bool kPreviewCrop = false;
constexpr bool kPreviewForce = false;
} // namespace

NavigationPreview::NavigationPreview(QObject *parent)
    : QObject(parent) {}

void NavigationPreview::setThumbnailer(std::shared_ptr<Thumbnailer> value) {
    if (thumbnailer)
        disconnect(thumbnailer.get(), nullptr, this, nullptr);
    thumbnailer = std::move(value);
    inFlightPath.clear();
    if (!thumbnailer) {
        qWarning() << "NavigationPreview: no thumbnailer; previews are off";
        return;
    }
    connect(thumbnailer.get(), &Thumbnailer::thumbnailReady, this,
            &NavigationPreview::onThumbnailReady);
    connect(thumbnailer.get(), &Thumbnailer::thumbnailFailed, this,
            &NavigationPreview::onThumbnailFailed);
}

void NavigationPreview::setThumbnailResolution(int resolution) {
    thumbnailResolution = resolution;
    // A new resolution clears the thumbnailer's tasks (Thumbnailer::
    // clearCache()) without a result, the one in flight among them.
    inFlightPath.clear();
}

void NavigationPreview::request(const QString &path) {
    wantedPath = path;
    if (inFlightPath.isEmpty())
        startRequest(path);
}

void NavigationPreview::cancel() {
    wantedPath.clear();
}

void NavigationPreview::startRequest(const QString &path) {
    if (!thumbnailer || thumbnailResolution <= 0) {
        qWarning() << "NavigationPreview: cannot request a preview of" << path
                   << "without a thumbnailer and a thumbnail resolution";
        return;
    }
    inFlightPath = path;
    thumbnailer->getThumbnailAsync(path, targetSize(), kPreviewCrop,
                                   kPreviewForce,
                                   kNavigationPreviewThumbnailPriority);
}

// Requests the file current by now, unless it is the one just answered.
void NavigationPreview::finishRequest(const QString &path) {
    inFlightPath.clear();
    if (!wantedPath.isEmpty() && wantedPath != path)
        startRequest(wantedPath);
}

void NavigationPreview::onThumbnailReady(std::shared_ptr<Thumbnail> thumbnail,
                                         QString filePath) {
    // The thumbnailer publishes every result to every consumer.
    if (filePath != inFlightPath || !thumbnail ||
        thumbnail->targetSize() != targetSize() ||
        thumbnail->cropped().value_or(kPreviewCrop) != kPreviewCrop)
        return;

    const QImage image = thumbnail->image();
    if (filePath == wantedPath && !image.isNull())
        emit previewReady(image, thumbnail->sourceSize(), filePath);
    finishRequest(filePath);
}

void NavigationPreview::onThumbnailFailed(QString filePath, QSize failedSize) {
    if (filePath != inFlightPath || failedSize != targetSize())
        return;
    finishRequest(filePath);
}

QSize NavigationPreview::targetSize() const {
    return QSize(thumbnailResolution, thumbnailResolution);
}
