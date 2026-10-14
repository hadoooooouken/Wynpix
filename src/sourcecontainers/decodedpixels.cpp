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
    QMutexLocker locker(&mMutex);
    if (mHdr && mSdrParams != params)
        convertLocked(params);
    return mSdr;
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

void DecodedPixels::convertLocked(const HdrToneMapParams &params) {
    // A copy built with other params is dropped; readers keep their reference.
    mSdr.reset();
    mSdrParams = params;
    if (!mToSdr) {
        qWarning() << "DecodedPixels: an HDR image has no SDR conversion";
        return;
    }
    QImage converted = mToSdr(*mHdr, params);
    if (converted.isNull()) {
        qWarning() << "DecodedPixels: the HDR image could not be converted to SDR";
        return;
    }
    mSdr = std::make_shared<const QImage>(std::move(converted));
}
