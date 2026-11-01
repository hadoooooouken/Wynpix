/*
    PNG image format plugin for Wynpix, decoding via libspng.

    Read-only: capabilities() only ever reports CanRead. Saving PNG files
    (e.g. from the batch converter) continues to go through the built-in
    Qt QPngHandler.

    Read as "apng", an animated PNG plays its frames: libspng 0.7.4 does not
    know APNG, so each frame is handed to it as a PNG of its own and composed
    here.
*/

#include "png_p.h"
#include "util_p.h"

#include <QColorSpace>
#include <QIODevice>
#include <QImage>
#include <QList>
#include <QLoggingCategory>
#include <QPainter>

#include <spng.h>
#include <zlib.h>

#include <algorithm>
#include <climits>
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

quint32 readBigEndian32(const uchar *p)
{
    return (quint32(p[0]) << 24) | (quint32(p[1]) << 16) | (quint32(p[2]) << 8) | p[3];
}

void writeBigEndian32(char *p, quint32 value)
{
    p[0] = char(value >> 24);
    p[1] = char(value >> 16);
    p[2] = char(value >> 8);
    p[3] = char(value);
}

bool readAt(QIODevice *device, qint64 pos, void *dest, qint64 size)
{
    return device->seek(pos) && device->read(reinterpret_cast<char *>(dest), size) == size;
}

constexpr qint64 kMaxChunkLength = 0x7fffffff;

// How the area of an APNG frame is cleared before the next frame is drawn
// (fcTL dispose_op).
enum class ApngDispose : uchar {
    None = 0,
    Background = 1,
    Previous = 2,
};

// How an APNG frame is drawn onto the canvas (fcTL blend_op).
enum class ApngBlend : uchar {
    Source = 0,
    Over = 1,
};

// The compressed image data of a frame in one chunk: an IDAT payload, or an
// fdAT payload after its sequence number.
struct ApngDataRange {
    qint64 pos = 0;
    qint64 size = 0;
};

struct ApngFrame {
    QRect rect;
    int delayMs = 0;
    ApngDispose dispose = ApngDispose::None;
    ApngBlend blend = ApngBlend::Source;
    QList<ApngDataRange> data;
};

// The frames of an APNG, located by walking its chunk headers. The image
// data stays in the file until a frame is decoded.
struct ApngIndex {
    QByteArray ihdr;
    QByteArray plte;
    QByteArray trns;
    QSize size;
    // 0 plays forever.
    quint32 numPlays = 0;
    QList<ApngFrame> frames;
};

// Frame delay in milliseconds. As in browsers, a delay of up to 10 ms plays
// as 100 ms, so that frames written with no delay stay visible.
int apngDelayMs(quint16 numerator, quint16 denominator)
{
    const qint64 den = denominator ? denominator : 100;
    const int ms = int((qint64(numerator) * 1000 + den / 2) / den);
    return ms <= 10 ? 100 : ms;
}

// Locates the frames of an APNG. A damaged file plays up to its last intact
// frame: indexing stops at a truncated chunk, an out-of-order sequence number
// or an invalid frame control chunk. Returns nothing for a PNG without
// animation control (or frames), which decodes as a still image. Needs a
// random-access device; the read position is restored.
std::optional<ApngIndex> indexApng(QIODevice *device)
{
    if (!device || device->isSequential()) {
        return std::nullopt;
    }

    const qint64 start = device->pos();
    const qint64 end = device->size();
    ApngIndex index;
    bool animated = false;
    quint32 numFrames = 0;
    quint32 nextSequence = 0;
    // An fdAT chunk was seen since the last fcTL.
    bool frameHasFdat = false;
    qint64 chunkPos = start + kPngSignatureSize;
    for (;;) {
        uchar header[8];
        if (!readAt(device, chunkPos, header, 8)) {
            break;
        }
        const qint64 length = readBigEndian32(header);
        const qint64 dataPos = chunkPos + 8;
        if (length > kMaxChunkLength || dataPos + length + 4 > end) {
            break;
        }
        const auto is = [&header](const char *type) {
            return std::memcmp(header + 4, type, 4) == 0;
        };

        if (is("IEND")) {
            break;
        } else if (is("IHDR")) {
            index.ihdr.resize(13);
            if (length != 13 || !readAt(device, dataPos, index.ihdr.data(), 13)) {
                break;
            }
            const auto ihdr = reinterpret_cast<const uchar *>(index.ihdr.constData());
            const quint32 width = readBigEndian32(ihdr);
            const quint32 height = readBigEndian32(ihdr + 4);
            if (width == 0 || height == 0 || width > PNG_MAX_IMAGE_WIDTH || height > PNG_MAX_IMAGE_HEIGHT) {
                break;
            }
            index.size = QSize(int(width), int(height));
        } else if (is("PLTE") || is("tRNS")) {
            QByteArray &data = is("PLTE") ? index.plte : index.trns;
            data.resize(length);
            if (!readAt(device, dataPos, data.data(), length)) {
                break;
            }
        } else if (is("acTL")) {
            uchar actl[8];
            if (length != 8 || !readAt(device, dataPos, actl, 8)) {
                break;
            }
            numFrames = readBigEndian32(actl);
            index.numPlays = readBigEndian32(actl + 4);
            animated = true;
        } else if (is("fcTL") && animated) {
            uchar fctl[26];
            if (length != 26 || !readAt(device, dataPos, fctl, 26) || readBigEndian32(fctl) != nextSequence++) {
                break;
            }
            // A frame without image data ends the animation.
            if ((!index.frames.isEmpty() && index.frames.last().data.isEmpty()) || quint32(index.frames.size()) == numFrames) {
                break;
            }
            const qint64 width = readBigEndian32(fctl + 4);
            const qint64 height = readBigEndian32(fctl + 8);
            const qint64 x = readBigEndian32(fctl + 12);
            const qint64 y = readBigEndian32(fctl + 16);
            const uchar dispose = fctl[24];
            const uchar blend = fctl[25];
            if (width == 0 || height == 0 || x + width > index.size.width() || y + height > index.size.height() || dispose > 2 || blend > 1) {
                break;
            }
            ApngFrame frame;
            frame.rect = QRect(int(x), int(y), int(width), int(height));
            frame.delayMs = apngDelayMs(quint16((fctl[20] << 8) | fctl[21]), quint16((fctl[22] << 8) | fctl[23]));
            frame.dispose = ApngDispose(dispose);
            frame.blend = ApngBlend(blend);
            index.frames.append(frame);
            frameHasFdat = false;
        } else if (is("IDAT")) {
            // The image data of the first frame when its fcTL comes first;
            // otherwise the default image, which is not part of the
            // animation.
            if (index.frames.size() == 1 && !frameHasFdat) {
                index.frames.last().data.append({dataPos, length});
            }
        } else if (is("fdAT") && animated) {
            uchar sequence[4];
            if (length <= 4 || !readAt(device, dataPos, sequence, 4) || readBigEndian32(sequence) != nextSequence++) {
                break;
            }
            // The first frame takes its data from either IDAT or fdAT.
            if (index.frames.isEmpty() || (index.frames.size() == 1 && !index.frames.last().data.isEmpty() && !frameHasFdat)) {
                break;
            }
            index.frames.last().data.append({dataPos + 4, length - 4});
            frameHasFdat = true;
        }
        chunkPos = dataPos + length + 4;
    }
    device->seek(start);

    if (!index.frames.isEmpty() && index.frames.last().data.isEmpty()) {
        index.frames.removeLast();
    }
    if (!animated || index.frames.isEmpty() || index.size.isEmpty()) {
        return std::nullopt;
    }
    return index;
}

void appendChunk(QByteArray &png, const char *type, QByteArrayView data)
{
    char header[8];
    writeBigEndian32(header, quint32(data.size()));
    std::memcpy(header + 4, type, 4);
    png.append(header, 8);
    png.append(data);
    uLong crc = crc32(0, reinterpret_cast<const Bytef *>(type), 4);
    crc = crc32(crc, reinterpret_cast<const Bytef *>(data.data()), uInt(data.size()));
    char crcBytes[4];
    writeBigEndian32(crcBytes, quint32(crc));
    png.append(crcBytes, 4);
}

// Decodes an APNG frame to RGBA8 by handing libspng a PNG of its own: the
// file's IHDR with the frame's size, its PLTE and tRNS, and the frame's
// image data as one IDAT chunk.
QImage decodeApngFrame(QIODevice *device, const ApngIndex &apng, const ApngFrame &frame)
{
    qint64 dataSize = 0;
    for (const ApngDataRange &range : frame.data) {
        dataSize += range.size;
    }
    if (dataSize > kMaxChunkLength) {
        return {};
    }

    QByteArray data(dataSize, Qt::Uninitialized);
    char *dest = data.data();
    for (const ApngDataRange &range : frame.data) {
        if (!readAt(device, range.pos, dest, range.size)) {
            qCWarning(LOG_PNGPLUGIN) << "reading APNG frame data failed";
            return {};
        }
        dest += range.size;
    }

    QByteArray ihdr = apng.ihdr;
    writeBigEndian32(ihdr.data(), quint32(frame.rect.width()));
    writeBigEndian32(ihdr.data() + 4, quint32(frame.rect.height()));
    QByteArray png;
    png.reserve(kPngSignatureSize + 3 * 12 + ihdr.size() + apng.plte.size() + apng.trns.size() + 2 * 12 + dataSize);
    png.append(reinterpret_cast<const char *>(kPngSignature), kPngSignatureSize);
    appendChunk(png, "IHDR", ihdr);
    if (!apng.plte.isEmpty()) {
        appendChunk(png, "PLTE", apng.plte);
    }
    if (!apng.trns.isEmpty()) {
        appendChunk(png, "tRNS", apng.trns);
    }
    appendChunk(png, "IDAT", data);
    appendChunk(png, "IEND", {});

    SpngCtxPtr ctx(spng_ctx_new(0));
    if (!ctx) {
        qCWarning(LOG_PNGPLUGIN) << "spng_ctx_new failed";
        return {};
    }
    if (int ret = spng_set_image_limits(ctx.get(), PNG_MAX_IMAGE_WIDTH, PNG_MAX_IMAGE_HEIGHT); ret != 0) {
        qCWarning(LOG_PNGPLUGIN) << "spng_set_image_limits failed:" << spng_strerror(ret);
        return {};
    }
    if (int ret = spng_set_png_buffer(ctx.get(), png.constData(), size_t(png.size())); ret != 0) {
        qCWarning(LOG_PNGPLUGIN) << "spng_set_png_buffer failed:" << spng_strerror(ret);
        return {};
    }
    size_t outSize = 0;
    if (int ret = spng_decoded_image_size(ctx.get(), SPNG_FMT_RGBA8, &outSize); ret != 0) {
        qCWarning(LOG_PNGPLUGIN) << "spng_decoded_image_size failed:" << spng_strerror(ret);
        return {};
    }
    QImage img = imageAlloc(frame.rect.size(), QImage::Format_RGBA8888);
    if (img.isNull() || size_t(img.sizeInBytes()) < outSize) {
        qCWarning(LOG_PNGPLUGIN) << "image allocation failed or too small for decoded size" << outSize;
        return {};
    }
    if (int ret = spng_decode_image(ctx.get(), img.bits(), outSize, SPNG_FMT_RGBA8, SPNG_DECODE_TRNS); ret != 0) {
        qCWarning(LOG_PNGPLUGIN) << "spng_decode_image failed for an APNG frame:" << spng_strerror(ret);
        return {};
    }
    return img;
}

} // namespace

class SpngHandlerPrivate
{
public:
    spng_ihdr m_ihdr{};
    bool m_ihdrValid = false;
    QImage::Format m_format = targetImageFormat();

    // APNG playback (SpngHandler::isAnimation()).
    bool m_apngIndexed = false;
    std::optional<ApngIndex> m_apng;
    QImage m_canvas;
    // The frame composeNextFrame() draws next.
    int m_nextFrame = 0;
    int m_lastReadFrame = -1;
    int m_lastDelayMs = 0;
    // How the area of the frame drawn last is cleared before the next one,
    // and what was under it for ApngDispose::Previous.
    QRect m_disposeRect;
    ApngDispose m_dispose = ApngDispose::None;
    QImage m_previousArea;
};

SpngHandler::SpngHandler()
    : QImageIOHandler()
    , d(new SpngHandlerPrivate)
{
}

bool SpngHandler::canRead() const
{
    if (isAnimation()) {
        return d->m_nextFrame < d->m_apng->frames.size();
    }
    if (canRead(device())) {
        setFormat("png");
        return true;
    }
    return false;
}

bool SpngHandler::isAnimation() const
{
    if (format().compare("apng", Qt::CaseInsensitive) != 0) {
        return false;
    }
    if (!d->m_apngIndexed) {
        d->m_apng = indexApng(device());
        d->m_apngIndexed = true;
    }
    return d->m_apng.has_value();
}

// Disposes of the previous frame's area as it asks, then draws the next
// frame onto the canvas. The first frame starts from a transparent canvas.
bool SpngHandler::composeNextFrame()
{
    const ApngIndex &apng = *d->m_apng;
    if (d->m_nextFrame >= apng.frames.size()) {
        return false;
    }
    const ApngFrame &frame = apng.frames.at(d->m_nextFrame);

    if (d->m_nextFrame == 0) {
        d->m_canvas = imageAlloc(apng.size, QImage::Format_RGBA8888_Premultiplied, ImageInitToZero::All);
        if (d->m_canvas.isNull()) {
            qCWarning(LOG_PNGPLUGIN) << "APNG canvas allocation failed";
            return false;
        }
        d->m_canvas.setColorSpace(QColorSpace(QColorSpace::SRgb));
    } else if (d->m_dispose != ApngDispose::None) {
        QPainter painter(&d->m_canvas);
        painter.setCompositionMode(QPainter::CompositionMode_Source);
        if (d->m_dispose == ApngDispose::Background) {
            painter.fillRect(d->m_disposeRect, Qt::transparent);
        } else {
            painter.drawImage(d->m_disposeRect.topLeft(), d->m_previousArea);
        }
    }

    // On the first frame this keeps the transparent canvas, which is what
    // the specification asks for there.
    d->m_previousArea = frame.dispose == ApngDispose::Previous ? d->m_canvas.copy(frame.rect) : QImage();
    d->m_dispose = frame.dispose;
    d->m_disposeRect = frame.rect;

    const QImage pixels = decodeApngFrame(device(), apng, frame);
    if (pixels.isNull()) {
        return false;
    }
    QPainter painter(&d->m_canvas);
    painter.setCompositionMode(frame.blend == ApngBlend::Over ? QPainter::CompositionMode_SourceOver : QPainter::CompositionMode_Source);
    painter.drawImage(frame.rect.topLeft(), pixels);
    painter.end();

    d->m_lastDelayMs = frame.delayMs;
    ++d->m_nextFrame;
    return true;
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

    if (isAnimation()) {
        if (!composeNextFrame()) {
            return false;
        }
        d->m_lastReadFrame = d->m_nextFrame - 1;
        // Shared with the caller; the next frame is drawn on a copy.
        *image = d->m_canvas;
        return true;
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
    if (option == QImageIOHandler::Animation) {
        return isAnimation();
    }
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
        v = QVariant::fromValue(isAnimation() ? QImage::Format_RGBA8888_Premultiplied : d->m_format);
    }

    return v;
}

int SpngHandler::imageCount() const
{
    return isAnimation() ? int(d->m_apng->frames.size()) : QImageIOHandler::imageCount();
}

int SpngHandler::loopCount() const
{
    if (!isAnimation()) {
        return 0;
    }
    const quint32 plays = d->m_apng->numPlays;
    return plays == 0 ? -1 : int(std::min<quint32>(plays - 1, INT_MAX));
}

int SpngHandler::nextImageDelay() const
{
    return isAnimation() ? d->m_lastDelayMs : 0;
}

int SpngHandler::currentImageNumber() const
{
    return isAnimation() ? d->m_lastReadFrame : 0;
}

bool SpngHandler::jumpToNextImage()
{
    return isAnimation() && composeNextFrame();
}

// Frames build on the canvas left by the ones before them, so going back
// starts over from the first frame.
bool SpngHandler::jumpToImage(int imageNumber)
{
    if (!isAnimation() || imageNumber < 0 || imageNumber >= d->m_apng->frames.size()) {
        return false;
    }
    if (imageNumber < d->m_nextFrame) {
        d->m_nextFrame = 0;
    }
    while (d->m_nextFrame < imageNumber) {
        if (!composeNextFrame()) {
            return false;
        }
    }
    return true;
}

QImageIOPlugin::Capabilities SpngPlugin::capabilities(QIODevice *device, const QByteArray &format) const
{
    if (format == "png" || format == "PNG" || format == "apng" || format == "APNG") {
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
