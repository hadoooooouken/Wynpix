#pragma once

#include <QString>
#include <QPixmap>
#include <QImage>
#include <memory>
#include <optional>

class Thumbnail {
public:
    Thumbnail(QString _name, QString _info, int _size, std::shared_ptr<QPixmap> _pixmap);
    Thumbnail(QString _name, QString _info, int _size, QImage _image);
    QString name();
    QString info();
    int size();
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
    int mSize;
    bool mHasAlphaChannel = false;
    std::optional<bool> mCropped;
};

