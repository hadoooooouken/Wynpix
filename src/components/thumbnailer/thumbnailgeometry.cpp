#include "thumbnailgeometry.h"

#include <QtGlobal>

namespace ThumbnailGeometry {

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

} // namespace ThumbnailGeometry
