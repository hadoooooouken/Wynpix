#include "overlaystate.h"

OverlayState::OverlayState(QObject *parent) : QObject(parent) {}

bool OverlayState::isOpen() const {
    return mOpen;
}

bool OverlayState::isCreated() const {
    return mCreated;
}

QPointF OverlayState::anchor() const {
    return mAnchor;
}

void OverlayState::setOpen(bool open) {
    if (open && !mCreated) {
        mCreated = true;
        emit createdChanged();
    }
    if (mOpen == open)
        return;
    mOpen = open;
    emit openChanged();
}

void OverlayState::setAnchor(QPointF anchor) {
    if (mAnchor == anchor)
        return;
    mAnchor = anchor;
    emit anchorChanged();
}

void OverlayState::toggle() {
    setOpen(!mOpen);
}

void OverlayState::close() {
    setOpen(false);
}
