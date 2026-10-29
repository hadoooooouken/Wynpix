#pragma once

#include <QHash>
#include <QString>
#include <QStringList>

class Actions
{
public:
    Actions();
    static Actions *getInstance();
    bool contains(const QString &action) const;
    QStringList getList() const;
    // The title the settings show for action, translated; action itself when
    // it is not a known action.
    QString title(const QString &action) const;

private:
    void init();
    void add(const QString &action, const char *title);
    // Every action with its untranslated title (context "Actions").
    QHash<QString, const char *> mTitles;

};

extern Actions *appActions;
