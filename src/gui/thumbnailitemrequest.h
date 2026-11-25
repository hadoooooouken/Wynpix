#pragma once

#include <QMetaType>
#include <QSize>

// One thumbnail request item: model/view index and target box in device pixels.
struct ThumbnailItemRequest {
    int index = -1;
    QSize targetSize;

    friend bool operator==(const ThumbnailItemRequest &, const ThumbnailItemRequest &) = default;
};
Q_DECLARE_METATYPE(ThumbnailItemRequest)
