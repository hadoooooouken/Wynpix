#pragma once

#include <QRect>
#include <QSize>
#include <QtCore/qnamespace.h>

// Geometry of a thumbnail made for a rectangular target box in device pixels.
// Pure functions shared by the thumbnail worker and its tests.
namespace ThumbnailGeometry {

inline constexpr int kCacheUpgradeStepPx = 64;
inline constexpr int kMaxAdaptiveCacheExtent = 2048;
inline constexpr int kCacheRoundingTolerancePx = 1;

struct CacheDecodePlan {
  // Exact 1:1 output size in display orientation (bounded by native source size).
  QSize desiredOutput;
  // True when the cached image already covers desiredOutput within rounding
  // tolerance (or the capped cache limit).
  bool cacheSufficient = false;
  // Decode/cache bounding box in display orientation when a fresh decode is needed.
  QSize decodeBox;
};

// The box a thumbnail is scaled into: the target box, or the square cut out
// of it when the thumbnail is cropped.
[[nodiscard]] QSize outputBox(QSize targetBox, bool crop);

// The square a cropped thumbnail is cut to: its side is the shorter side of
// the target box, so the cropped thumbnail is drawn without a resize.
[[nodiscard]] QSize cropBox(QSize targetBox);

// Same as QSize::scaled(box, mode), but never enlarges an image that is
// already smaller than the box - avoids blurry upscaled thumbnails for tiny
// source images (icons, small screenshots, etc). With
// Qt::KeepAspectRatioByExpanding the image is kept as it is only when it is
// smaller than the box in both dimensions, as it always was.
[[nodiscard]] QSize noUpscaleScaledSize(QSize originalSize, QSize box,
                                        Qt::AspectRatioMode mode);

// The part of an image of scaledSize that is cut out to box, centred.
[[nodiscard]] QRect cropRect(QSize scaledSize, QSize box);

// True when 0-based DocumentInfo::exifOrientation() rotates by +/-90 degrees
// and swaps width and height (ImageLib::exifRotated cases 4..7).
[[nodiscard]] bool isExifOrientationTransposed(int exifOrientation);

// Returns size transposed when transpose is true.
[[nodiscard]] QSize orientSize(QSize size, bool transpose);

// Returns sourceSize transposed if its orientation (landscape vs portrait)
// differs from thumbnailSize.
[[nodiscard]] QSize sizeInThumbnailOrientation(QSize thumbnailSize,
                                               QSize sourceSize);

// Evaluates whether cachedSize is sufficient for targetBox/crop and computes
// the quantized display-oriented decodeBox when decoding from source is needed.
[[nodiscard]] CacheDecodePlan planCacheDecode(QSize cachedSize,
                                              QSize orientedOriginalSize,
                                              QSize targetBox, bool crop,
                                              int baseResolution);

// Rounds dimensions down to the nearest even integer (keeping 0 or 1 untouched).
[[nodiscard]] QSize evenSize(QSize size) noexcept;

} // namespace ThumbnailGeometry
