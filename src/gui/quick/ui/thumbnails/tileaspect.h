#pragma once

#include <QSize>
#include <QtGlobal>

// Proportions (width / height) of the tiles of the layouts that follow the
// proportions of their images (MasonryLayout, StripMasonryLayout).
namespace TileAspect {
// Proportions of an image of unknown size.
inline constexpr double kUnknown = 4.0 / 3.0;
// The limits the proportions of the other images are held to; a thumbnail
// more extreme than that is fitted into its tile with empty space around.
inline constexpr double kMinimum = 0.25;
inline constexpr double kMaximum = 4.0;

// Proportions of a tile for an image of the given size: kUnknown for an
// invalid size, otherwise the proportions bounded to kMinimum - kMaximum.
[[nodiscard]] inline double of(const QSize &size) {
    if (size.width() <= 0 || size.height() <= 0)
        return kUnknown;
    const double aspect = static_cast<double>(size.width()) / size.height();
    return qBound(kMinimum, aspect, kMaximum);
}
} // namespace TileAspect
