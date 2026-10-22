#include "autocoloradjustments.h"

#include <algorithm>
#include <cmath>
#include <cstdint>

namespace {
// Percentage of pixels to ignore at top and bottom of histogram (0.05% threshold).
constexpr double kPercentIgnore = 0.0005;
constexpr int kHistogramBins = 256;
constexpr int kMaxBinIndex = 255;
constexpr int kMinDynamicRangeBins = 16;
constexpr int kMaxDimensionSamples = 256;
constexpr float kMaxGain = 2.5f;
constexpr float kMinRelativeRangeRatio = 0.20f;

void computeChannelFactors(int minVal, int maxVal, int minRequiredRange, float &scale, float &bias) {
    const int range = maxVal - minVal;
    if (range < minRequiredRange) {
        // Channel dynamic range is too narrow (e.g. flat image, monochrome, noise, or subordinate channel).
        // Keep neutral coefficients to prevent turning flat images black or blowing up noise.
        scale = 1.0f;
        bias = 0.0f;
        return;
    }

    const float minNorm = minVal / 255.0f;
    const float maxNorm = maxVal / 255.0f;
    const float diff = maxNorm - minNorm;

    scale = std::min(1.0f / diff, kMaxGain);
    bias = -minNorm * scale;
}
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

    // Ensure threshold is at least 1 when totalSampled is non-zero.
    const auto threshold = std::max<std::uint64_t>(1, static_cast<std::uint64_t>(totalSampled * kPercentIgnore));

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
        for (int i = kMaxBinIndex; i >= 0; --i) {
            if (tally + hist[i] >= threshold) {
                return i;
            }
            tally += hist[i];
        }
        return kMaxBinIndex;
    };

    const int rMin = std::clamp(findMin(rHist), 0, kMaxBinIndex);
    const int gMin = std::clamp(findMin(gHist), 0, kMaxBinIndex);
    const int bMin = std::clamp(findMin(bHist), 0, kMaxBinIndex);

    const int rMax = std::clamp(findMax(rHist), 0, kMaxBinIndex);
    const int gMax = std::clamp(findMax(gHist), 0, kMaxBinIndex);
    const int bMax = std::clamp(findMax(bHist), 0, kMaxBinIndex);

    const int rRange = rMax - rMin;
    const int gRange = gMax - gMin;
    const int bRange = bMax - bMin;
    const int maxRange = std::max({rRange, gRange, bRange});
    const int minRequiredRange = std::max(
        kMinDynamicRangeBins,
        static_cast<int>(std::round(maxRange * kMinRelativeRangeRatio)));

    computeChannelFactors(rMin, rMax, minRequiredRange, factors.scale[0], factors.bias[0]);
    computeChannelFactors(gMin, gMax, minRequiredRange, factors.scale[1], factors.bias[1]);
    computeChannelFactors(bMin, bMax, minRequiredRange, factors.scale[2], factors.bias[2]);

    return factors;
}
