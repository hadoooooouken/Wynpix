#include "decodebudget.h"

#include <QImageReader>

#include <algorithm>
#include <utility>

namespace {
constexpr int BitsPerByte = 8;
// Readers that do not report their pixel format decode to 32-bit ARGB/RGB.
constexpr int FallbackBitsPerPixel = 32;
constexpr qint64 BytesPerUnit = 1024 * 1024;
// How often a waiting decode checks for cancellation.
constexpr int CancellationPollMs = 50;
} // namespace

std::optional<qint64> estimateDecodedBytes(QImageReader &reader) {
    const QSize size = reader.size();
    if (!size.isValid() || size.isEmpty())
        return std::nullopt;
    const QImage::Format format = reader.imageFormat();
    const int bitsPerPixel =
        format == QImage::Format_Invalid
            ? FallbackBitsPerPixel
            : static_cast<int>(QImage::toPixelFormat(format).bitsPerPixel());
    return static_cast<qint64>(size.width()) * size.height() * bitsPerPixel /
           BitsPerByte;
}

DecodeBudget::Reservation::Reservation(Reservation &&other) noexcept
    : units(other.units), count(std::exchange(other.count, 0)) {}

DecodeBudget::Reservation &
DecodeBudget::Reservation::operator=(Reservation &&other) noexcept {
    if (this != &other) {
        release();
        units = other.units;
        count = std::exchange(other.count, 0);
    }
    return *this;
}

DecodeBudget::Reservation::~Reservation() {
    release();
}

void DecodeBudget::Reservation::release() {
    if (count > 0)
        units->release(std::exchange(count, 0));
}

DecodeBudget::DecodeBudget(qint64 capacityBytes)
    : units(static_cast<int>(std::max<qint64>(1, capacityBytes / BytesPerUnit))),
      capacityUnits(units.available()) {}

std::optional<DecodeBudget::Reservation>
DecodeBudget::reserve(qint64 bytes, const DecodeContext &context) {
    const qint64 wanted = (std::max<qint64>(0, bytes) + BytesPerUnit - 1) / BytesPerUnit;
    const int count = static_cast<int>(std::min<qint64>(wanted, capacityUnits));
    while (!units.tryAcquire(count, CancellationPollMs)) {
        if (context.isCancellationRequested())
            return std::nullopt;
    }
    return Reservation(&units, count);
}
