#pragma once

#include <QHash>
#include <QString>
#include <QStringList>

// The mode an action works in, which is the group the settings show it in, in
// the order of the groups: only the folder view, only the viewer, or both
// alike.
enum class ActionCategory { FolderView, Viewer, Common };

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
    // The category of action; Common when it is not a known action.
    ActionCategory category(const QString &action) const;
    // The title of category, translated.
    static QString categoryTitle(ActionCategory category);
    // Whether actions of the two categories work in a common mode; one
    // shortcut then runs only one of them.
    static bool categoriesOverlap(ActionCategory one, ActionCategory other);
    // Whether an action of category works in the folder view (folderView) or
    // in the viewer.
    static bool worksIn(ActionCategory category, bool folderView);

private:
    struct Entry {
        // Untranslated title (context "Actions").
        const char *title = nullptr;
        ActionCategory category = ActionCategory::Common;
    };

    void init();
    void add(const QString &action, const char *title, ActionCategory category);
    // Every action with its title and category.
    QHash<QString, Entry> mEntries;

};

extern Actions *appActions;
