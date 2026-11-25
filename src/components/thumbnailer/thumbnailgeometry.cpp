#include "thumbnailgeometry.h"

#include <QtGlobal>

namespace ThumbnailGeometry {
namespace {
constexpr int kMinExifTransposedOrientation = 4;
constexpr int kMaxExifTransposedOrientation = 7;

[[nodiscard]] int quantizeCacheExtent(int value, int minExtent) {
  const int boundedMin = qBound(1, minExtent, kMaxAdaptiveCacheExtent);
  const int clamped = qBound(boundedMin, value, kMaxAdaptiveCacheExtent);
  const int steps = (clamped + kCacheUpgradeStepPx - 1) / kCacheUpgradeStepPx;
  return qMin(steps * kCacheUpgradeStepPx, kMaxAdaptiveCacheExtent);
}

[[nodiscard]] QSize minBounds(QSize left, QSize right) {
  return QSize(qMin(left.width(), right.width()),
               qMin(left.height(), right.height()));
}
} // namespace

QSize outputBox(QSize targetBox, bool crop) {
  return crop ? cropBox(targetBox) : targetBox;
}

QSize cropBox(QSize targetBox) {
  const int edge = qMin(targetBox.width(), targetBox.height());
  return QSize(edge, edge);
}

QSize noUpscaleScaledSize(QSize originalSize, QSize box,
                          Qt::AspectRatioMode mode) {
  if (!originalSize.isValid())
    return originalSize;

  if (mode == Qt::KeepAspectRatioByExpanding) {
    // "Expanding" mode always wants to cover the box completely, which
    // forces an upscale once the source is smaller than that box in both
    // dimensions. In that case just keep native resolution instead.
    if (originalSize.width() < box.width() &&
        originalSize.height() < box.height())
      return originalSize;
  } else {
    // KeepAspectRatio / IgnoreAspectRatio: no scaling is needed at all (and
    // no upscaling happens) once the source already fits within the box.
    if (originalSize.width() <= box.width() &&
        originalSize.height() <= box.height())
      return originalSize;
  }
  return originalSize.scaled(box, mode);
}

QRect cropRect(QSize scaledSize, QSize box) {
  QRect clip(QPoint(0, 0), box);
  clip.moveCenter(QRect(QPoint(0, 0), scaledSize).center());
  return clip;
}

bool isExifOrientationTransposed(int exifOrientation) {
  return exifOrientation >= kMinExifTransposedOrientation &&
         exifOrientation <= kMaxExifTransposedOrientation;
}

QSize orientSize(QSize size, bool transpose) {
  return transpose ? size.transposed() : size;
}

QSize sizeInThumbnailOrientation(QSize thumbnailSize, QSize sourceSize) {
  if (!sourceSize.isValid() || sourceSize.isEmpty() ||
      !thumbnailSize.isValid() || thumbnailSize.isEmpty())
    return sourceSize;
  const bool sourceIsLandscape = sourceSize.width() > sourceSize.height();
  const bool thumbnailIsLandscape =
      thumbnailSize.width() > thumbnailSize.height();
  if (sourceIsLandscape != thumbnailIsLandscape)
    sourceSize.transpose();
  return sourceSize;
}

CacheDecodePlan planCacheDecode(QSize cachedSize, QSize orientedOriginalSize,
                                QSize targetBox, bool crop,
                                int baseResolution) {
  const QSize outBox = outputBox(targetBox, crop);
  const Qt::AspectRatioMode mode =
      crop ? Qt::KeepAspectRatioByExpanding : Qt::KeepAspectRatio;
  const bool hasOriginal =
      orientedOriginalSize.isValid() && !orientedOriginalSize.isEmpty();
  const QSize desiredOutput =
      hasOriginal ? noUpscaleScaledSize(orientedOriginalSize, outBox, mode)
                  : outBox;

  const QSize maxCacheBox(kMaxAdaptiveCacheExtent, kMaxAdaptiveCacheExtent);
  const QSize maxCacheable =
      hasOriginal ? noUpscaleScaledSize(orientedOriginalSize, maxCacheBox,
                                        Qt::KeepAspectRatio)
                  : maxCacheBox;
  const QSize requiredCached = minBounds(desiredOutput, maxCacheable);

  const bool hasCached = cachedSize.isValid() && !cachedSize.isEmpty();
  const bool coversRequired =
      hasCached &&
      cachedSize.width() + kCacheRoundingTolerancePx >= requiredCached.width() &&
      cachedSize.height() + kCacheRoundingTolerancePx >= requiredCached.height();
  const bool alreadyNative =
      hasCached && hasOriginal &&
      cachedSize.width() + kCacheRoundingTolerancePx >=
          orientedOriginalSize.width() &&
      cachedSize.height() + kCacheRoundingTolerancePx >=
          orientedOriginalSize.height();

  const int baseFloor = qMax(1, baseResolution);
  const QSize decodeBox(
      quantizeCacheExtent(qMax(baseFloor, desiredOutput.width()), baseFloor),
      quantizeCacheExtent(qMax(baseFloor, desiredOutput.height()), baseFloor));

  return {
      .desiredOutput = desiredOutput,
      .cacheSufficient = coversRequired || alreadyNative,
      .decodeBox = decodeBox,
  };
}

QSize evenSize(QSize size) noexcept {
  return QSize(size.width() > 1 ? (size.width() & ~1) : size.width(),
               size.height() > 1 ? (size.height() & ~1) : size.height());
}

} // namespace ThumbnailGeometry
