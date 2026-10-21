#include "thumbnail.h"
#include <QGuiApplication>

namespace {
const QString kOriginalWidthMetadataKey = QStringLiteral("originalWidth");
const QString kOriginalHeightMetadataKey = QStringLiteral("originalHeight");
}

Thumbnail::Thumbnail(QString _name, QString _info, QSize _targetSize, std::shared_ptr<QPixmap> _pixmap)
    : mName(_name),
      mInfo(_info),
      mPixmap(_pixmap),
      mTargetSize(_targetSize)
{
    if(_pixmap) {
        mHasAlphaChannel = _pixmap->hasAlphaChannel();
        mSourceSize = _pixmap->size();
    }
}

Thumbnail::Thumbnail(QString _name, QString _info, QSize _targetSize, QImage _image)
    : mName(_name),
      mInfo(_info),
      mImage(_image),
      mTargetSize(_targetSize),
      mHasAlphaChannel(_image.hasAlphaChannel())
{
    const QSize metadataSourceSize(
        _image.text(kOriginalWidthMetadataKey).toInt(),
        _image.text(kOriginalHeightMetadataKey).toInt());
    mSourceSize = _image.size();
    if (metadataSourceSize.isValid() && !metadataSourceSize.isEmpty())
        mSourceSize = metadataSourceSize;
}

QString Thumbnail::name() {
    return mName;
}

QString Thumbnail::info() {
    return mInfo;
}

QSize Thumbnail::targetSize() const {
    return mTargetSize;
}

bool Thumbnail::hasAlphaChannel() {
    return mHasAlphaChannel;
}

QSize Thumbnail::sourceSize() const {
    return mSourceSize;
}

QImage Thumbnail::image() const {
    if (!mImage.isNull())
        return mImage;
    return mPixmap ? mPixmap->toImage() : QImage();
}

std::optional<bool> Thumbnail::cropped() const {
    return mCropped;
}

void Thumbnail::setCropped(bool cropped) {
    mCropped = cropped;
}

std::shared_ptr<QPixmap> Thumbnail::pixmap() {
    if (!mPixmap && !mImage.isNull()) {
        mPixmap = std::make_shared<QPixmap>(QPixmap::fromImage(mImage));
        mPixmap->setDevicePixelRatio(qApp->devicePixelRatio());
    }
    return mPixmap;
}

