#pragma once

#include <QImage>
#include <QMutex>
#include <QSize>

#include <functional>
#include <memory>
#include <optional>

#include "utils/hdrtonemapper.h"

// Decoded pixels of a static image and the SDR copy the CPU works with
// (editing, saving, copying, upscaling). An HDR image is converted to SDR by
// the conversion it is given on the first sdr() call; the viewer displays the
// decoded HDR pixels itself (decoded()). The SDR copy is kept for the tone
// mapping parameters it was built with and rebuilt when sdr() is asked for
// other ones, so a cached image follows a tone mapping change like the viewer.
// An SDR image is its own SDR copy.
//
// Thread safe: images are shared between the GUI thread and the loader and
// upscaler pools.
class DecodedPixels {
public:
    // HDR -> SDR; returns a null image when the conversion fails.
    using SdrConversion =
        std::function<QImage(const QImage &hdr, const HdrToneMapParams &params)>;

    DecodedPixels() = default;
    DecodedPixels(const DecodedPixels &) = delete;
    DecodedPixels &operator=(const DecodedPixels &) = delete;

    // Takes decoded (null clears). hdr: decoded needs toSdr.
    void assign(std::shared_ptr<const QImage> decoded, bool hdr,
                SdrConversion toSdr);

    // The SDR pixels; converts an HDR image with params on first use and
    // again whenever params differ from those of the kept copy. nullptr
    // without pixels or when the conversion with params failed (reported once
    // per params).
    [[nodiscard]] std::shared_ptr<const QImage> sdr(const HdrToneMapParams &params);
    // The pixels as decoded: the HDR source while it is kept, otherwise the
    // SDR pixels. Never converts.
    [[nodiscard]] std::shared_ptr<const QImage> decoded() const;
    // True while the HDR source is kept.
    [[nodiscard]] bool hasHdrSource() const;
    [[nodiscard]] QSize size() const;
    // Bytes of all pixel copies kept: the HDR source plus a built SDR copy.
    [[nodiscard]] qint64 sizeInBytes() const;

    // Replaces the pixels with SDR ones (committed edits); a kept HDR source
    // is dropped.
    void replace(std::shared_ptr<const QImage> sdrPixels);

private:
    // Requires mMutex.
    void convertLocked(const HdrToneMapParams &params);

    mutable QMutex mMutex;
    std::shared_ptr<const QImage> mHdr;
    std::shared_ptr<const QImage> mSdr;
    SdrConversion mToSdr;
    // The params of the last conversion of mHdr: mSdr was built with them,
    // or the conversion failed with them (mSdr null).
    std::optional<HdrToneMapParams> mSdrParams;
};
