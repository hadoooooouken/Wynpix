#pragma once

#include <QRect>
#include <QSize>
#include <QtCore/qnamespace.h>

// Geometry of a thumbnail made for a rectangular target box in device pixels.
// Pure functions shared by the thumbnail worker and its tests.
namespace ThumbnailGeometry {

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

} // namespace ThumbnailGeometry
