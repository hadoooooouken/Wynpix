#include "itemrows.h"

#include <QtGlobal>

#include <cmath>

UniformItemRows::UniformItemRows(int itemCount, int columns, int itemExtent, int leadingSpace)
    : mItemCount(qMax(0, itemCount)),
      mColumns(qMax(1, columns)),
      mItemExtent(itemExtent),
      mLeadingSpace(leadingSpace) {}

// A view without a row size has no rows (nothing to load or scroll to).
int UniformItemRows::rowCount() const {
    if (mItemExtent <= 0)
        return 0;
    return (mItemCount + mColumns - 1) / mColumns;
}

int UniformItemRows::rowOf(int item) const {
    const int rows = rowCount();
    if (rows == 0)
        return -1;
    return qBound(0, item / mColumns, rows - 1);
}

int UniformItemRows::firstItem(int row) const {
    const int rows = rowCount();
    if (rows == 0)
        return -1;
    return qBound(0, row, rows - 1) * mColumns;
}

int UniformItemRows::lastItem(int row) const {
    const int rows = rowCount();
    if (rows == 0)
        return -1;
    return qMin(mItemCount - 1, (qBound(0, row, rows - 1) + 1) * mColumns - 1);
}

// Not clamped: the row after the last one starts where the content ends.
qreal UniformItemRows::rowStart(int row) const {
    return mLeadingSpace + static_cast<qreal>(row) * mItemExtent;
}

qreal UniformItemRows::rowExtent(int) const {
    return mItemExtent;
}

int UniformItemRows::rowAt(qreal position) const {
    const int rows = rowCount();
    if (rows == 0)
        return -1;
    const qreal row = std::floor((position - mLeadingSpace) / mItemExtent);
    return static_cast<int>(qBound(0.0, row, static_cast<qreal>(rows - 1)));
}

qreal UniformItemRows::contentExtent() const {
    return mLeadingSpace + static_cast<qreal>(rowCount()) * mItemExtent;
}
