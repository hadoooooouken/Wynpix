#pragma once

#include <memory>

#include <QImage>
#include <QObject>
#include <QSize>
#include <QString>

#include "components/thumbnailer/thumbnailer.h"

// The thumbnail of the file being stepped to while its image is not loaded
// yet (NavigationThrottle defers the load), for the viewer to show in its
// place. It asks the Thumbnailer for the uncropped thumbnail of the cache's
// resolution (the size the thumbnail cache keeps, so a cached one comes
// without scaling); one that is not cached is decoded on the thumbnail pool.
//
// One request is in flight at a time: while the person keeps stepping, the
// files passed before the request finished are skipped and the file current
// by then is requested next, so a held key does not flood the thumbnail pool.
//
// GUI thread only.
class NavigationPreview final : public QObject {
    Q_OBJECT
public:
    explicit NavigationPreview(QObject *parent = nullptr);

    void setThumbnailer(std::shared_ptr<Thumbnailer> thumbnailer);
    // Side, in pixels, of the square the thumbnail cache keeps thumbnails in.
    void setThumbnailResolution(int resolution);

    // The preview of path is wanted; replaces any earlier wish.
    void request(const QString &path);
    // No preview is wanted any more (the image itself is shown or loading
    // on its own); a result still in flight is dropped.
    void cancel();

signals:
    // sourceSize: of the image the thumbnail was made from.
    void previewReady(const QImage &image, QSize sourceSize, const QString &path);

private:
    void startRequest(const QString &path);
    void finishRequest(const QString &path);
    void onThumbnailReady(std::shared_ptr<Thumbnail> thumbnail, QString filePath);
    void onThumbnailFailed(QString filePath, QSize failedSize);
    [[nodiscard]] QSize targetSize() const;

    std::shared_ptr<Thumbnailer> thumbnailer;
    int thumbnailResolution = 0;
    QString wantedPath;
    QString inFlightPath;
};
