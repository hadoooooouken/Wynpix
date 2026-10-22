#include "autocoloradjustments.h"

#include <algorithm>
#include <cmath>
#include <cstdint>

namespace {
// Percentage of pixels to ignore at top and bottom of histogram (0.05% threshold).
constexpr double kPercentIgnore = 0.0005;
constexpr int kHistogramBins = 256;
constexpr float kMinChannelDiff = 0.001f;
constexpr int kMaxDimensionSamples = 256;
} // namespace

WhiteBalanceFactors calculateWhiteBalanceFactors(const QImage *image) {
    WhiteBalanceFactors factors;
    if (!image || image->isNull()) {
        return factors;
    }

    const int width = image->width();
    const int height = image->height();

    // Sample down large images for sub-millisecond histogram collection.
    const int stepX = std::max(1, width / kMaxDimensionSamples);
    const int stepY = std::max(1, height / kMaxDimensionSamples);

    std::uint32_t rHist[kHistogramBins]{};
    std::uint32_t gHist[kHistogramBins]{};
    std::uint32_t bHist[kHistogramBins]{};
    std::uint64_t totalSampled = 0;

    for (int y = 0; y < height; y += stepY) {
        for (int x = 0; x < width; x += stepX) {
            const QRgb pixel = image->pixel(x, y);
            rHist[qRed(pixel)]++;
            gHist[qGreen(pixel)]++;
            bHist[qBlue(pixel)]++;
            ++totalSampled;
        }
    }

    if (totalSampled == 0) {
        return factors;
    }

    const auto threshold = static_cast<std::uint64_t>(totalSampled * kPercentIgnore);

    // Find min bounds (accumulating from 0 upwards).
    const auto findMin = [threshold](const std::uint32_t hist[kHistogramBins]) -> int {
        std::uint64_t tally = 0;
        for (int i = 0; i < kHistogramBins; ++i) {
            if (tally + hist[i] >= threshold) {
                return i;
            }
            tally += hist[i];
        }
        return 0;
    };

    // Find max bounds (accumulating from 255 downwards).
    const auto findMax = [threshold](const std::uint32_t hist[kHistogramBins]) -> int {
        std::uint64_t tally = 0;
        for (int i = kHistogramBins - 1; i >= 0; --i) {
            if (tally + hist[i] >= threshold) {
                return i;
            }
            tally += hist[i];
        }
        return kHistogramBins - 1;
    };

    const int rMin = findMin(rHist);
    const int gMin = findMin(gHist);
    const int bMin = findMin(bHist);

    const int rMax = std::max(rMin + 1, findMax(rHist));
    const int gMax = std::max(gMin + 1, findMax(gHist));
    const int bMax = std::max(bMin + 1, findMax(bHist));

    const float rMinNorm = rMin / 255.0f;
    const float gMinNorm = gMin / 255.0f;
    const float bMinNorm = bMin / 255.0f;

    const float rMaxNorm = rMax / 255.0f;
    const float gMaxNorm = gMax / 255.0f;
    const float bMaxNorm = bMax / 255.0f;

    const float rDiff = std::max(kMinChannelDiff, rMaxNorm - rMinNorm);
    const float gDiff = std::max(kMinChannelDiff, gMaxNorm - gMinNorm);
    const float bDiff = std::max(kMinChannelDiff, bMaxNorm - bMinNorm);

    factors.scale[0] = 1.0f / rDiff;
    factors.bias[0]  = -rMinNorm / rDiff;

    factors.scale[1] = 1.0f / gDiff;
    factors.bias[1]  = -gMinNorm / gDiff;

    factors.scale[2] = 1.0f / bDiff;
    factors.bias[2]  = -bMinNorm / bDiff;

    return factors;
}
