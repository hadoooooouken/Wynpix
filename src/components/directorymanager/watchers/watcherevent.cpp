#include <QDebug>
#include "watcherevent.h"

WatcherEvent::WatcherEvent(const QString &name, int timerId, WatcherEvent::Type type) :
    mName(name),
    mTimerId(timerId),
    mType(type)
{
}

WatcherEvent::~WatcherEvent() {
}

QString WatcherEvent::name() const {
    return mName;
}

void WatcherEvent::setName(const QString &name) {
    mName = name;
}

WatcherEvent::Type WatcherEvent::type() const {
    return mType;
}
