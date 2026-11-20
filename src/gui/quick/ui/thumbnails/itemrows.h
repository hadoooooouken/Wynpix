#pragma once

#include <QtGlobal>

// Geometry of the items of a thumbnail view, grouped in rows along the scroll
// axis: the strip and the uniform folder grid (UniformItemRows) and the
// masonry layout (MasonryLayout) describe themselves with it, so that
// ThumbnailListModel scrolls, pages and loads by rows without knowing how
// the rows were laid out. Positions are in content coordinates of the model:
// the first row starts at its own rowStart(0).
//
// Queries about rows or items out of range are clamped to the nearest valid
// one, because the model may ask about a count that changed since the rows
// were built; they return -1 or 0 only when there are no rows at all.
class ItemRows {
public:
    ItemRows() = default;
    virtual ~ItemRows() = default;

    [[nodiscard]] virtual int rowCount() const = 0;
    // The row of item, -1 without rows.
    [[nodiscard]] virtual int rowOf(int item) const = 0;
    // The first and the last item of row.
    [[nodiscard]] virtual int firstItem(int row) const = 0;
    [[nodiscard]] virtual int lastItem(int row) const = 0;
    // Start and size of row along the scroll axis. rowStart(rowCount())
    // is the end of the content.
    [[nodiscard]] virtual qreal rowStart(int row) const = 0;
    [[nodiscard]] virtual qreal rowExtent(int row) const = 0;
    // The row that contains position: the last row that starts at or before
    // it, the first row for a position before it. -1 without rows.
    [[nodiscard]] virtual int rowAt(qreal position) const = 0;
    // Size of the content along the scroll axis.
    [[nodiscard]] virtual qreal contentExtent() const = 0;

protected:
    // Copyable only through a concrete class, never sliced through the base.
    ItemRows(const ItemRows &) = default;
    ItemRows &operator=(const ItemRows &) = default;
};

// Rows of the same size with the same number of items (the last one may hold
// fewer), after a leading space: the thumbnail strip and the folder grid.
class UniformItemRows final : public ItemRows {
public:
    UniformItemRows() = default;
    UniformItemRows(int itemCount, int columns, int itemExtent, int leadingSpace);

    [[nodiscard]] int rowCount() const override;
    [[nodiscard]] int rowOf(int item) const override;
    [[nodiscard]] int firstItem(int row) const override;
    [[nodiscard]] int lastItem(int row) const override;
    [[nodiscard]] qreal rowStart(int row) const override;
    [[nodiscard]] qreal rowExtent(int row) const override;
    [[nodiscard]] int rowAt(qreal position) const override;
    [[nodiscard]] qreal contentExtent() const override;

private:
    int mItemCount = 0;
    int mColumns = 1;
    int mItemExtent = 0;
    int mLeadingSpace = 0;
};
