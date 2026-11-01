/*
    PNG image format plugin for Wynpix, decoding via libspng.

    Read-only: capabilities() only ever reports CanRead. Saving PNG files
    (e.g. from the batch converter) continues to go through the built-in
    Qt QPngHandler.
*/

#include "png_p.h"
#include "util_p.h"

#include <QColorSpace>
#include <QIODevice>
#include <QImage>
#include <QLoggingCategory>

#include <spng.h>

#include <algorithm>
#include <cstring>
#include <memory>
#include <optional>

#ifdef QT_DEBUG
Q_LOGGING_CATEGORY(LOG_PNGPLUGIN, "kf.imageformats.plugins.png", QtDebugMsg)
#else
Q_LOGGING_CATEGORY(LOG_PNGPLUGIN, "kf.imageformats.plugins.png", QtWarningMsg)
#endif

/* *** PNG_MAX_IMAGE_WIDTH and PNG_MAX_IMAGE_HEIGHT ***
 * The maximum size in pixels allowed by the plugin, enforced via
 * spng_set_image_limits() so decompression-bomb PNGs are rejected before
 * any pixel data is decoded.
 */
#ifndef PNG_MAX_IMAGE_WIDTH
#define PNG_MAX_IMAGE_WIDTH KIF_LARGE_IMAGE_PIXEL_LIMIT
#endif
#ifndef PNG_MAX_IMAGE_HEIGHT
#define PNG_MAX_IMAGE_HEIGHT PNG_MAX_IMAGE_WIDTH
#endif

namespace // Private
{

constexpr uchar kPngSignature[8] = {0x89, 'P', 'N', 'G', '\r', '\n', 0x1a, '\n'};
constexpr qint64 kPngSignatureSize = qint64(sizeof(kPngSignature));

// Signature (8) + one complete IHDR chunk (4 length + 4 "IHDR" + 13 data +
// 4 CRC = 25) = 33 bytes. That is all libspng needs to populate spng_ihdr,
// so Size/ImageFormat queries can be answered from a non-destructive
// device->peek() without decoding any pixel data.
constexpr qint64 kPngIhdrPeekSize = kPngSignatureSize + 25;

struct SpngCtxDeleter {
    void operator()(spng_ctx *ctx) const noexcept
    {
        if (ctx) {
            spng_ctx_free(ctx);
        }
    }
};
using SpngCtxPtr = std::unique_ptr<spng_ctx, SpngCtxDeleter>;

bool matchesSignature(const QByteArray &head)
{
    return head.size() >= kPngSignatureSize && std::memcmp(head.constData(), kPngSignature, size_t(kPngSignatureSize)) == 0;
}

// spng read callback bridging into a QIODevice for streaming decode.
//
// QIODevice::read() is not guaranteed to fill the buffer in a single call
// even when there is no real end-of-stream (this is especially likely for
// large chunk reads, e.g. big IDAT chunks). Treating any short read as EOF
// truncates decoding of otherwise-valid files, so we loop until either the
// full length has been read or we hit a genuine end/error.
int qiodeviceReadFn(spng_ctx *, void *user, void *dest, size_t length)
{
    auto device = reinterpret_cast<QIODevice *>(user);
    auto *buf = reinterpret_cast<char *>(dest);
    size_t total = 0;

    while (total < length) {
        qint64 n = device->read(buf + total, qint64(length - total));
        if (n < 0) {
            return SPNG_IO_ERROR;
        }
        if (n == 0) {
            if (device->atEnd()) {
                return SPNG_IO_EOF;
            }
            if (!device->waitForReadyRead(-1)) {
                return SPNG_IO_EOF;
            }
            continue;
        }
        total += size_t(n);
    }
    return 0;
}

// Reads just the IHDR chunk (via peek, without moving the device's read
// cursor) to answer Size/ImageFormat queries cheaply.
bool peekIhdr(QIODevice *device, spng_ihdr &ihdr)
{
    if (!device) {
        return false;
    }

    auto head = device->peek(kPngIhdrPeekSize);
    if (head.size() < kPngIhdrPeekSize || !matchesSignature(head)) {
        return false;
    }

    SpngCtxPtr ctx(spng_ctx_new(0));
    if (!ctx) {
        return false;
    }

    if (spng_set_png_buffer(ctx.get(), head.constData(), size_t(head.size())) != 0) {
        return false;
    }

    return spng_get_ihdr(ctx.get(), &ihdr) == 0;
}

QImage::Format targetImageFormat()
{
    // Decision: always decode to 8-bit RGBA for v1, matching the QOI plugin.
    return QImage::Format_RGBA8888;
}

// Colour space of a PNG whose cICP chunk (PNG 3rd edition) describes PQ or
// HLG samples with RGB coding, together with its video range flag. Other
// cICP code points are left to the sRGB default.
struct HdrCicp {
    QColorSpace colorSpace;
    bool fullRange = true;
};

std::optional<HdrCicp> hdrCicpFromCodePoints(const uchar *cp)
{
    QColorSpace::TransferFunction transfer;
    switch (cp[1]) {
    case 16:
        transfer = QColorSpace::TransferFunction::St2084;
        break;
    case 18:
        transfer = QColorSpace::TransferFunction::Hlg;
        break;
    default:
        return std::nullopt;
    }

    QColorSpace::Primaries primaries;
    switch (cp[0]) {
    case 1:
        primaries = QColorSpace::Primaries::SRgb;
        break;
    case 9:
        primaries = QColorSpace::Primaries::Bt2020;
        break;
    case 12:
        primaries = QColorSpace::Primaries::DciP3D65;
        break;
    default:
        return std::nullopt;
    }

    if (cp[2] != 0) {
        return std::nullopt;
    }
    return HdrCicp{QColorSpace(primaries, transfer), cp[3] != 0};
}

// Finds an HDR cICP chunk by walking the chunk headers in front of the image
// data. libspng 0.7.4 does not know cICP, and its spng_get_unknown_chunks()
// copies out only the first unknown chunk. Needs a random-access device; the
// read position is restored.
std::optional<HdrCicp> findHdrCicp(QIODevice *device)
{
    if (!device || device->isSequential()) {
        return std::nullopt;
    }

    const qint64 start = device->pos();
    std::optional<HdrCicp> result;
    qint64 chunkPos = start + kPngSignatureSize;
    // cICP comes before PLTE and IDAT, after at most a handful of chunks.
    for (int i = 0; i < 64 && device->seek(chunkPos); ++i) {
        uchar header[8];
        if (device->read(reinterpret_cast<char *>(header), 8) != 8) {
            break;
        }
        const quint32 length = (quint32(header[0]) << 24) | (quint32(header[1]) << 16) | (quint32(header[2]) << 8) | header[3];
        const auto type = reinterpret_cast<const char *>(header + 4);
        if (std::memcmp(type, "cICP", 4) == 0) {
            uchar cp[4];
            if (length == 4 && device->read(reinterpret_cast<char *>(cp), 4) == 4) {
                result = hdrCicpFromCodePoints(cp);
            }
            break;
        }
        if (std::memcmp(type, "PLTE", 4) == 0 || std::memcmp(type, "IDAT", 4) == 0 || std::memcmp(type, "IEND", 4) == 0 || length > 0x7fffffffu) {
            break;
        }
        chunkPos += 12 + qint64(length);
    }
    device->seek(start);
    return result;
}

// Expands narrow-range (video) samples to full range in place, alpha aside.
// Narrow range spans 16..235 scaled to the PNG's bit depth; libspng widens
// 8-bit samples to 16 bits by multiplying by 257.
void expandNarrowRange(QImage &img, int bitDepth)
{
    const int black = bitDepth == 16 ? 16 << 8 : 16 * 257;
    const int white = bitDepth == 16 ? 235 << 8 : 235 * 257;
    const float scale = 65535.0f / float(white - black);
    for (int y = 0; y < img.height(); ++y) {
        auto px = reinterpret_cast<quint16 *>(img.scanLine(y));
        for (int x = 0; x < img.width() * 4; x += 4) {
            for (int c = 0; c < 3; ++c) {
                const float v = (float(px[x + c]) - float(black)) * scale;
                px[x + c] = quint16(std::clamp(v, 0.0f, 65535.0f) + 0.5f);
            }
        }
    }
}

} // namespace

class SpngHandlerPrivate
{
public:
    spng_ihdr m_ihdr{};
    bool m_ihdrValid = false;
    QImage::Format m_format = targetImageFormat();
};

SpngHandler::SpngHandler()
    : QImageIOHandler()
    , d(new SpngHandlerPrivate)
{
}

bool SpngHandler::canRead() const
{
    if (canRead(device())) {
        setFormat("png");
        return true;
    }
    return false;
}

bool SpngHandler::canRead(QIODevice *device)
{
    if (!device) {
        qCWarning(LOG_PNGPLUGIN) << "SpngHandler::canRead() called with no device";
        return false;
    }

    return matchesSignature(device->peek(kPngSignatureSize));
}

bool SpngHandler::read(QImage *image)
{
    auto dev = device();
    if (!dev) {
        return false;
    }

    // PQ / HLG samples keep 16 bits and their colour space, so that the
    // viewer tone maps them.
    const std::optional<HdrCicp> hdr = findHdrCicp(dev);
    const spng_format spngFormat = hdr ? SPNG_FMT_RGBA16 : SPNG_FMT_RGBA8;
    const QImage::Format imageFormat = hdr ? QImage::Format_RGBA64 : targetImageFormat();

    SpngCtxPtr ctx(spng_ctx_new(0));
    if (!ctx) {
        qCWarning(LOG_PNGPLUGIN) << "spng_ctx_new failed";
        return false;
    }

    if (int ret = spng_set_image_limits(ctx.get(), PNG_MAX_IMAGE_WIDTH, PNG_MAX_IMAGE_HEIGHT); ret != 0) {
        qCWarning(LOG_PNGPLUGIN) << "spng_set_image_limits failed:" << spng_strerror(ret);
        return false;
    }
    if (int ret = spng_set_png_stream(ctx.get(), qiodeviceReadFn, dev); ret != 0) {
        qCWarning(LOG_PNGPLUGIN) << "spng_set_png_stream failed:" << spng_strerror(ret);
        return false;
    }

    spng_ihdr ihdr{};
    if (int ret = spng_get_ihdr(ctx.get(), &ihdr); ret != 0) {
        qCWarning(LOG_PNGPLUGIN) << "spng_get_ihdr failed:" << spng_strerror(ret);
        return false;
    }

    // Cache for any option() call that might follow (e.g. a re-query of
    // ImageFormat after read()).
    d->m_ihdr = ihdr;
    d->m_ihdrValid = true;
    d->m_format = imageFormat;

    size_t outSize = 0;
    if (int ret = spng_decoded_image_size(ctx.get(), spngFormat, &outSize); ret != 0) {
        qCWarning(LOG_PNGPLUGIN) << "spng_decoded_image_size failed:" << spng_strerror(ret);
        return false;
    }

    QImage img = imageAlloc(ihdr.width, ihdr.height, imageFormat);
    if (img.isNull() || size_t(img.sizeInBytes()) < outSize) {
        qCWarning(LOG_PNGPLUGIN) << "image allocation failed or too small for decoded size" << outSize;
        return false;
    }

    // SPNG_DECODE_TRNS is required whenever the source color type has no
    // native alpha channel (grayscale/indexed, e.g. 1-bit optimized icons)
    // but conveys transparency via a tRNS chunk instead. Without this flag
    // libspng leaves such pixels fully opaque when converting to RGBA8.
    if (int ret = spng_decode_image(ctx.get(), img.bits(), outSize, spngFormat, SPNG_DECODE_TRNS); ret != 0) {
        qCWarning(LOG_PNGPLUGIN) << "spng_decode_image failed:" << spng_strerror(ret);
        return false;
    }

    if (hdr) {
        if (!hdr->fullRange) {
            expandNarrowRange(img, ihdr.bit_depth);
        }
        img.setColorSpace(hdr->colorSpace);
    } else {
        img.setColorSpace(QColorSpace(QColorSpace::SRgb));
    }

    *image = img;
    return true;
}

bool SpngHandler::supportsOption(ImageOption option) const
{
    return option == QImageIOHandler::Size || option == QImageIOHandler::ImageFormat;
}

QVariant SpngHandler::option(ImageOption option) const
{
    QVariant v;

    if (option != QImageIOHandler::Size && option != QImageIOHandler::ImageFormat) {
        return v;
    }

    bool haveIhdr = d->m_ihdrValid;
    spng_ihdr ihdr = d->m_ihdr;

    if (!haveIhdr && peekIhdr(device(), ihdr)) {
        d->m_ihdr = ihdr;
        d->m_ihdrValid = true;
        // read() decodes PQ / HLG samples to 16 bits.
        if (findHdrCicp(device())) {
            d->m_format = QImage::Format_RGBA64;
        }
        haveIhdr = true;
    }

    if (!haveIhdr) {
        return v;
    }

    if (option == QImageIOHandler::Size) {
        v = QVariant::fromValue(QSize(int(ihdr.width), int(ihdr.height)));
    } else if (option == QImageIOHandler::ImageFormat) {
        v = QVariant::fromValue(d->m_format);
    }

    return v;
}

QImageIOPlugin::Capabilities SpngPlugin::capabilities(QIODevice *device, const QByteArray &format) const
{
    if (format == "png" || format == "PNG") {
        return Capabilities(CanRead);
    }
    if (!format.isEmpty()) {
        return {};
    }
    if (!device || !device->isOpen() || !device->isReadable()) {
        return {};
    }

    Capabilities cap;
    if (SpngHandler::canRead(device)) {
        cap |= CanRead;
    }
    return cap;
}

QImageIOHandler *SpngPlugin::create(QIODevice *device, const QByteArray &format) const
{
    QImageIOHandler *handler = new SpngHandler;
    handler->setDevice(device);
    handler->setFormat(format);
    return handler;
}

#include "moc_png_p.cpp"
