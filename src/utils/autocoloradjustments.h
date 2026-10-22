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
