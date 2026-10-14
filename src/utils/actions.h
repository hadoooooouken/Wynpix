#pragma once

#include <QSet>
#include <QString>
#include <QStringList>

class Actions
{
public:
    Actions();
    static Actions *getInstance();
    bool contains(const QString &action) const;
    QStringList getList() const;

private:
    void init();
    QSet<QString> mActions;

};

extern Actions *appActions;
