#pragma once

#include <QString>

class WatcherEvent {
public:
    enum Type {
        None
    };

    WatcherEvent(const QString &name,int timerId, Type type = None);
    ~WatcherEvent();

    QString name() const;
    void setName(const QString& name);

    Type type() const;

private:
    QString mName;
    int mTimerId;
    Type mType;

};
