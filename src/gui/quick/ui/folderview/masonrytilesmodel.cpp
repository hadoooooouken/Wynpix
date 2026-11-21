#include "masonrytilesmodel.h"

#include <QDebug>
#include <QtGlobal>

#include <utility>

MasonryTilesModel::MasonryTilesModel(QObject *parent) : QAbstractListModel(parent) {}

void MasonryTilesModel::setSource(ThumbnailListModel *source) {
    if (mSource) {
        qWarning() << "MasonryTilesModel already has a source";
        return;
    }
    if (!source) {
        qWarning() << "MasonryTilesModel needs a source";
        return;
    }
    mSource = source;
    connect(source, &QAbstractItemModel::dataChanged, this, &MasonryTilesModel::onSourceDataChanged);
    connect(source, &QAbstractItemModel::modelReset, this,
            &MasonryTilesModel::onSourceStructureChanged);
    connect(source, &QAbstractItemModel::rowsInserted, this,
            &MasonryTilesModel::onSourceStructureChanged);
    connect(source, &QAbstractItemModel::rowsRemoved, this,
            &MasonryTilesModel::onSourceStructureChanged);
}

void MasonryTilesModel::setLayout(std::shared_ptr<const TileRows> layout,
                                  const ThumbnailStripLayout &folderCell) {
    mLayout = std::move(layout);
    mFolderCell = folderCell;
    updateWindow();
    if (mCount > 0)
        emit dataChanged(index(0), index(mCount - 1), {TileXRole, TileYRole, CellLayoutRole});
}

void MasonryTilesModel::setViewport(qreal offset, qreal extent) {
    if (offset == mOffset && extent == mExtent)
        return;
    mOffset = offset;
    mExtent = extent;
    updateWindow();
}

int MasonryTilesModel::firstItem() const {
    return mFirst;
}

int MasonryTilesModel::rowCount(const QModelIndex &parent) const {
    return parent.isValid() ? 0 : mCount;
}

QVariant MasonryTilesModel::data(const QModelIndex &index, int role) const {
    if (!mSource || !index.isValid() || index.row() < 0 || index.row() >= mCount)
        return {};
    const int item = mFirst + index.row();
    switch (role) {
    case TileXRole:
        return mLayout ? mLayout->itemRect(item).x() : 0.0;
    case TileYRole:
        return mLayout ? mLayout->itemRect(item).y() : 0.0;
    case CellLayoutRole:
        return QVariant::fromValue(cellLayoutFor(item));
    case ItemIndexRole:
        return item;
    default:
        return mSource->data(mSource->index(item, 0), role);
    }
}

QHash<int, QByteArray> MasonryTilesModel::roleNames() const {
    QHash<int, QByteArray> names;
    if (mSource)
        names = mSource->roleNames();
    names.insert(TileXRole, "tileX");
    names.insert(TileYRole, "tileY");
    names.insert(CellLayoutRole, "cellLayout");
    names.insert(ItemIndexRole, "itemIndex");
    return names;
}

// A tile has no padding, margins or labels: the thumbnail fills it, and the
// selection is drawn over it.
ThumbnailStripLayout MasonryTilesModel::cellLayoutFor(int item) const {
    if (!mSource || item < mSource->dirCount())
        return mFolderCell;
    const QSize size = mLayout ? mLayout->itemRect(item).size().toSize() : QSize();
    return thumbnailTileCellFor(size);
}

MasonryTilesModel::Window MasonryTilesModel::windowForViewport() const {
    if (!mSource || !mLayout || mLayout->rowCount() == 0 || mSource->itemCount() == 0 ||
        mExtent <= 0.0)
        return {};
    const int firstRow = mLayout->rowAt(mOffset - kWindowMargin);
    const int lastRow = mLayout->rowAt(mOffset + mExtent + kWindowMargin);
    const int first = mLayout->firstItem(firstRow);
    // The layout may be older than the source.
    const int last = qMin(mLayout->lastItem(lastRow), mSource->itemCount() - 1);
    if (first > last)
        return {};
    return {first, last - first + 1};
}

void MasonryTilesModel::updateWindow() {
    applyWindow(windowForViewport());
}

// Rows leave and enter at the ends, so that the delegates of the items that
// stay in the window are kept.
void MasonryTilesModel::applyWindow(const Window &window) {
    if (window.first == mFirst && window.count == mCount)
        return;
    const int previousFirst = mFirst;
    const bool overlaps = mCount > 0 && window.count > 0 && window.first < mFirst + mCount &&
                          window.first + window.count > mFirst;
    if (!overlaps) {
        beginResetModel();
        mFirst = window.first;
        mCount = window.count;
        endResetModel();
    } else {
        const int newLast = window.first + window.count - 1;
        if (window.first > mFirst) {
            const int leaving = window.first - mFirst;
            beginRemoveRows({}, 0, leaving - 1);
            mFirst += leaving;
            mCount -= leaving;
            endRemoveRows();
        }
        const int currentLast = mFirst + mCount - 1;
        if (newLast < currentLast) {
            const int leaving = currentLast - newLast;
            beginRemoveRows({}, mCount - leaving, mCount - 1);
            mCount -= leaving;
            endRemoveRows();
        }
        if (window.first < mFirst) {
            const int entering = mFirst - window.first;
            beginInsertRows({}, 0, entering - 1);
            mFirst = window.first;
            mCount += entering;
            endInsertRows();
        }
        const int lastNow = mFirst + mCount - 1;
        if (newLast > lastNow) {
            const int entering = newLast - lastNow;
            beginInsertRows({}, mCount, mCount + entering - 1);
            mCount += entering;
            endInsertRows();
        }
    }
    if (mFirst != previousFirst)
        emit windowChanged();
}

void MasonryTilesModel::onSourceDataChanged(const QModelIndex &topLeft,
                                            const QModelIndex &bottomRight,
                                            const QList<int> &roles) {
    if (mCount == 0)
        return;
    const int top = qMax(topLeft.row(), mFirst);
    const int bottom = qMin(bottomRight.row(), mFirst + mCount - 1);
    if (top > bottom)
        return;
    QList<int> changed = roles;
    // An empty list means every role, the cell included.
    if (!roles.isEmpty() && roles.contains(ThumbnailListModel::DirectoryRole))
        changed.append(CellLayoutRole);
    emit dataChanged(index(top - mFirst), index(bottom - mFirst), changed);
}

// The rows of the source moved or went: the window starts over, with the rows
// the layout (that may be older than the source) still gives.
void MasonryTilesModel::onSourceStructureChanged() {
    const Window window = windowForViewport();
    const int previousFirst = mFirst;
    beginResetModel();
    mFirst = window.first;
    mCount = window.count;
    endResetModel();
    if (mFirst != previousFirst)
        emit windowChanged();
}
