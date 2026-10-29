#pragma once

#include <QHash>
#include <QString>
#include <QStringList>

// The group the settings show an action in, in the order of the groups.
enum class ActionCategory { Navigation, View, Editing, File, Clipboard, Upscale, Application };

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
    // The category of action; Application when it is not a known action.
    ActionCategory category(const QString &action) const;
    // The title of category, translated.
    static QString categoryTitle(ActionCategory category);

private:
    struct Entry {
        // Untranslated title (context "Actions").
        const char *title = nullptr;
        ActionCategory category = ActionCategory::Application;
    };

    void init();
    void add(const QString &action, const char *title, ActionCategory category);
    // Every action with its title and category.
    QHash<QString, Entry> mEntries;

};

extern Actions *appActions;
