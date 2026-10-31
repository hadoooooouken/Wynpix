#pragma once

#include <QImage>

// White balance per-channel stretch factors.
struct WhiteBalanceFactors {
    float scale[3]{1.0f, 1.0f, 1.0f};
    float bias[3]{0.0f, 0.0f, 0.0f};
};

// Calculates per-channel (R, G, B) min/max histogram thresholds ignoring top and bottom 0.05%
// of pixels, producing affine scale and bias factors to stretch each channel to [0.0, 1.0].
[[nodiscard]] WhiteBalanceFactors calculateWhiteBalanceFactors(const QImage *image);

// The pixels calculateWhiteBalanceFactors() samples from image, as a small image
// with its format, colour space and text, so that measuring it gives the same
// factors. Lets a per-pixel conversion (HDR tone mapping) run on the samples
// instead of the whole image. Formats below one byte per pixel are returned whole.
[[nodiscard]] QImage whiteBalanceSamples(const QImage &image);
