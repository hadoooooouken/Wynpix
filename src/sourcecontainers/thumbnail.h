#pragma once

#include <QString>
#include <QPixmap>
#include <QImage>
#include <QSize>
#include <memory>
#include <optional>

class Thumbnail {
public:
    Thumbnail(QString _name, QString _info, QSize _targetSize, std::shared_ptr<QPixmap> _pixmap);
    Thumbnail(QString _name, QString _info, QSize _targetSize, QImage _image);
    QString name();
    QString info();
    // The target box (device pixels) of the request this thumbnail answers.
    // It identifies the request, so a view takes the thumbnails of its own
    // box; it is set by whoever answers the request and is not derived from
    // the pixels (a folder icon and a cropped thumbnail have other pixel
    // sizes than the box they were made for).
    QSize targetSize() const;
    bool hasAlphaChannel();
    QSize sourceSize() const;
    std::shared_ptr<QPixmap> pixmap();
    // The thumbnail pixels without a conversion to QPixmap: the decoded
    // image, or the image of a thumbnail made from a pixmap (shared, not
    // copied). Null for a thumbnail without pixels.
    QImage image() const;
    // Whether the thumbnailer cropped the pixels to a square. Thumbnails of
    // one path and size are published to every view, so a view takes only
    // those of its own crop. std::nullopt: the thumbnail serves either crop
    // (a folder icon composed for one view).
    [[nodiscard]] std::optional<bool> cropped() const;
    void setCropped(bool cropped);
private:
    QString mName, mInfo;
    std::shared_ptr<QPixmap> mPixmap;
    QImage mImage;
    QSize mSourceSize;
    QSize mTargetSize;
    bool mHasAlphaChannel = false;
    std::optional<bool> mCropped;
};

