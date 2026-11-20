#pragma once

#include <QAbstractListModel>
#include <QList>
#include <QPointer>
#include <QtQml/qqmlregistration.h>

#include <memory>

#include "gui/quick/ui/folderview/masonrylayout.h"
#include "gui/quick/ui/thumbnails/thumbnaillistmodel.h"
#include "gui/quick/ui/thumbnails/thumbnailstriplayout.h"

// The items of the folder grid that a masonry view has to draw: those near
// the viewport, with the position and the cell of each. It reads a
// ThumbnailListModel (the items) and a MasonryLayout (where they are), and
// exposes the roles of the first plus
//   tileX, tileY    position of the tile in content coordinates,
//   cellLayout      the cell of the tile as the strip's cell item draws it:
//                   the folder cell for a folder, a tile of the size of the
//                   image without padding and labels for an image,
//                   itemIndex       the item of the source; unlike the row it
//                   does not change when rows leave in front of it.
// Row r of this model is item firstItem + r of the source, so a view that
// binds delegates to it creates and destroys delegates only as items enter
// and leave the window, and not for every item of a large directory.
// GUI thread only.
class MasonryTilesModel : public QAbstractListModel {
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("Provided by FolderGridController.tiles")
    // The item of the first row.
    Q_PROPERTY(int firstItem READ firstItem NOTIFY windowChanged FINAL)

public:
    enum TileRole {
        TileXRole = ThumbnailListModel::DragHoverRole + 1,
        TileYRole,
        CellLayoutRole,
        ItemIndexRole,
    };
    Q_ENUM(TileRole)

    // Distance beyond the viewport in which items have a row, so that
    // scrolling does not show empty space before the delegates exist.
    static constexpr int kWindowMargin = 300;

    explicit MasonryTilesModel(QObject *parent = nullptr);

    // source must outlive the model.
    void setSource(ThumbnailListModel *source);
    // Positions of the items, and the cell of the folders.
    void setLayout(std::shared_ptr<const MasonryLayout> layout,
                   const ThumbnailStripLayout &folderCell);
    // The part of the content in view (ThumbnailListModel::setViewport()).
    Q_INVOKABLE void setViewport(qreal offset, qreal extent);

    [[nodiscard]] int firstItem() const;

    [[nodiscard]] int rowCount(const QModelIndex &parent = {}) const override;
    [[nodiscard]] QVariant data(const QModelIndex &index, int role) const override;
    [[nodiscard]] QHash<int, QByteArray> roleNames() const override;

signals:
    void windowChanged();

private:
    struct Window {
        int first = 0;
        int count = 0;
    };

    [[nodiscard]] Window windowForViewport() const;
    [[nodiscard]] ThumbnailStripLayout cellLayoutFor(int item) const;
    void updateWindow();
    void applyWindow(const Window &window);
    void onSourceDataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight,
                             const QList<int> &roles);
    void onSourceStructureChanged();

    QPointer<ThumbnailListModel> mSource;
    std::shared_ptr<const MasonryLayout> mLayout;
    ThumbnailStripLayout mFolderCell;
    qreal mOffset = 0.0;
    qreal mExtent = 0.0;
    int mFirst = 0;
    int mCount = 0;
};
