#pragma once

#include <QHash>
#include <QString>
#include <QStringList>

// The mode an action works in, which is the group the settings show it in, in
// the order of the groups: only the folder view, only the viewer, or both
// alike.
enum class ActionCategory { FolderView, Viewer, Common };

// The subgroup the settings show an action in within its category, in the
// order of the subgroups.
enum class ActionGroup {
    Navigation,
    Selection,
    View,
    File,
    Editing,
    FilesAndClipboard,
    Upscale,
    Clipboard,
    Sorting,
    Application,
};
inline constexpr int kActionGroupCount = static_cast<int>(ActionGroup::Application) + 1;

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
    // The subgroup of action; Application when it is not a known action.
    ActionGroup group(const QString &action) const;
    // The title of category, translated.
    static QString categoryTitle(ActionCategory category);
    // The title of the settings section of group in category, translated
    // ("Viewer › Navigation").
    static QString sectionTitle(ActionCategory category, ActionGroup group);
    // The position of that section among all sections.
    static int sectionOrder(ActionCategory category, ActionGroup group);
    // Whether actions of the two categories work in a common mode; one
    // shortcut then runs only one of them.
    static bool categoriesOverlap(ActionCategory one, ActionCategory other);
    // Whether an action of category works in the folder view (folderView) or
    // in the viewer.
    static bool worksIn(ActionCategory category, bool folderView);
    // Whether action moves the folder view selection, so that its shortcut
    // with Shift added extends the selection instead.
    bool extendsWithShift(const QString &action) const;

private:
    struct Entry {
        // Untranslated title (context "Actions").
        const char *title = nullptr;
        ActionCategory category = ActionCategory::Common;
        ActionGroup group = ActionGroup::Application;
        bool extendsWithShift = false;
    };

    void init();
    void add(const QString &action, const char *title, ActionCategory category,
             ActionGroup group);
    // A folder view action that moves the selection.
    void addSelectionMove(const QString &action, const char *title);
    // Every action with its title and category.
    QHash<QString, Entry> mEntries;

};

extern Actions *appActions;
