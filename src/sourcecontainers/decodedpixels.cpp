#include "decodedpixels.h"

#include <QDebug>
#include <QMutexLocker>

#include <utility>

void DecodedPixels::assign(std::shared_ptr<const QImage> decoded, bool hdr,
                           SdrConversion toSdr) {
    QMutexLocker locker(&mMutex);
    mHdr.reset();
    mSdr.reset();
    mToSdr = std::move(toSdr);
    mSdrParams.reset();
    if (!decoded)
        return;
    if (!hdr) {
        mSdr = std::move(decoded);
        return;
    }
    mHdr = std::move(decoded);
}

std::shared_ptr<const QImage> DecodedPixels::sdr(const HdrToneMapParams &params) {
    {
        QMutexLocker locker(&mMutex);
        if (hasSdrLocked(params))
            return mSdr;
    }
    QMutexLocker conversionLocker(&mConversionMutex);
    std::shared_ptr<const QImage> hdr;
    SdrConversion toSdr;
    {
        QMutexLocker locker(&mMutex);
        if (hasSdrLocked(params))
            return mSdr;
        // A copy built with other params is dropped; readers keep their
        // reference.
        mSdr.reset();
        mSdrParams.reset();
        hdr = mHdr;
        toSdr = mToSdr;
    }

    std::shared_ptr<const QImage> converted;
    if (!toSdr) {
        qWarning() << "DecodedPixels: an HDR image has no SDR conversion";
    } else {
        QImage image = toSdr(*hdr, params);
        if (image.isNull())
            qWarning() << "DecodedPixels: the HDR image could not be converted to SDR";
        else
            converted = std::make_shared<const QImage>(std::move(image));
    }

    QMutexLocker locker(&mMutex);
    // Pixels assigned or replaced during the conversion keep their state; the
    // caller still gets the copy of the pixels it asked for.
    if (mHdr == hdr) {
        mSdr = converted;
        mSdrParams = params;
    }
    return converted;
}

bool DecodedPixels::hasSdr(const HdrToneMapParams &params) const {
    QMutexLocker locker(&mMutex);
    return hasSdrLocked(params);
}

bool DecodedPixels::hasSdrLocked(const HdrToneMapParams &params) const {
    return !mHdr || mSdrParams == params;
}

std::shared_ptr<const QImage> DecodedPixels::decoded() const {
    QMutexLocker locker(&mMutex);
    return mHdr ? mHdr : mSdr;
}

bool DecodedPixels::hasHdrSource() const {
    QMutexLocker locker(&mMutex);
    return mHdr != nullptr;
}

QSize DecodedPixels::size() const {
    QMutexLocker locker(&mMutex);
    if (mHdr)
        return mHdr->size();
    return mSdr ? mSdr->size() : QSize();
}

qint64 DecodedPixels::sizeInBytes() const {
    QMutexLocker locker(&mMutex);
    qint64 bytes = 0;
    if (mHdr)
        bytes += mHdr->sizeInBytes();
    if (mSdr)
        bytes += mSdr->sizeInBytes();
    return bytes;
}

void DecodedPixels::replace(std::shared_ptr<const QImage> sdrPixels) {
    QMutexLocker locker(&mMutex);
    mHdr.reset();
    mSdr = std::move(sdrPixels);
    mSdrParams.reset();
}
