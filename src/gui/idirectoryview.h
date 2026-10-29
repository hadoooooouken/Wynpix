#pragma once

#include <QtPlugin>
#include <QList>
#include <QSize>
#include <memory>

#include <QDropEvent>

class Thumbnail;
class QString;
class QMimeData;

class IDirectoryView {
public:
    virtual ~IDirectoryView() {}

    virtual void populate(int) = 0;
    virtual void setThumbnail(int pos, std::shared_ptr<Thumbnail> thumb) = 0;
    virtual void setThumbnailPending(int pos, bool pending) {
        Q_UNUSED(pos)
        Q_UNUSED(pending)
    }
    // targetSize is the target box of the failed request.
    virtual void setThumbnailUnavailable(int pos, QSize targetSize) = 0;
    virtual void select(QList<int>) = 0;
    virtual void select(int) = 0;
    virtual void focusOn(int) = 0;
    virtual void focusOnSelection() = 0;
    virtual QList<int> selection() = 0;
    virtual void setDirectoryPath(QString path) = 0;
    virtual void insertItem(int index) = 0;
    virtual void removeItem(int index) = 0;
    virtual void reloadItem(int index) = 0;
    virtual void setDragHover(int index) = 0;
    virtual void setDirCount(int count) { Q_UNUSED(count) }
    [[nodiscard]] virtual bool hasThumbnail(int pos) const { Q_UNUSED(pos); return false; }

//signals
    virtual void itemActivated(int) = 0;
    // Indices, target box in device pixels, crop, force.
    virtual void thumbnailsRequested(QList<int>, QSize, bool, bool) = 0;
    virtual void draggedOut() = 0;
    virtual void draggedOver(int) = 0;
    virtual void droppedInto(const QMimeData*, QObject*, int, Qt::DropAction) = 0;
    virtual void backRequested() = 0;
    virtual void forwardRequested() = 0;
    virtual void openSelectedRequested() {}
};

Q_DECLARE_INTERFACE(IDirectoryView, "IDirectoryView")
