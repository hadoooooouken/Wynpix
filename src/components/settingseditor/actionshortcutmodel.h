#pragma once

#include <QAbstractListModel>
#include <QStringList>

#include <optional>

#include "settingsvalues.h"

// How the settings show action: its entry in actions; for a script ("s:<name>")
// the title "Script: <name>" in the category "Scripts", after every action
// category; for an unknown action the action itself in the category "Other",
// last.
[[nodiscard]] ActionInfo shortcutActionInfo(const QString &action, const ActionInfoMap &actions);

// How the settings show shortcut: a mouse input by its name ("Mouse Back" for
// "XButton1"), after its modifiers ("Ctrl+Wheel up"); any other shortcut as it
// is stored.
[[nodiscard]] QString shortcutDisplayText(const QString &shortcut);

// The shortcuts of the settings dialog ("Controls"), one row per action:
// every action and every script, with or without shortcuts, sorted by
// category and then by title. A row lists the shortcuts of its action in the
// order they were bound. A shortcut is bound once per mode (ShortcutScope):
// putting a shortcut removes it from the action of a common mode that had it
// before. Edits stay in the model until the
// dialog applies them.
//
// QML reads the roles `action`, `title` (a script row shows the script name),
// `category`, `shortcuts` and `shortcutTexts` (the shortcuts as the settings
// show them, see shortcutDisplayText()). GUI thread only.
class ActionShortcutModel final : public QAbstractListModel {
    Q_OBJECT

public:
    enum Role { ActionRole = Qt::UserRole + 1, TitleRole, CategoryRole, ShortcutsRole, ShortcutTextsRole };

    explicit ActionShortcutModel(QObject *parent = nullptr);

    [[nodiscard]] int rowCount(const QModelIndex &parent = {}) const override;
    [[nodiscard]] QVariant data(const QModelIndex &index, int role) const override;
    [[nodiscard]] QHash<int, QByteArray> roleNames() const override;

    // The actions and the scripts (names) that have rows.
    void setActions(const ActionInfoMap &actions, const QStringList &scripts);

    // Every binding, row by row.
    [[nodiscard]] ShortcutList entries() const;
    // Replaces every binding.
    void setEntries(const ShortcutList &entries);

    // Binds shortcut to action: removes replaced (an edited binding) and the
    // binding that had shortcut in a mode of action, then adds the entry, in
    // place of replaced
    // when that is a binding of the same action. Returns the row of the
    // action, or -1 (nothing changed) when action or shortcut is empty.
    int put(const ShortcutEntry &entry, const std::optional<ShortcutEntry> &replaced = std::nullopt);
    // Removes the binding entry; false when there is none.
    bool remove(const ShortcutEntry &entry);
    // The row of action, or -1.
    [[nodiscard]] int rowOf(const QString &action) const;

private:
    struct Row {
        QString action;
        ActionInfo info;
        QStringList shortcuts;
    };

    // Rebuilds the rows from the actions, the scripts and entries; an
    // action of entries that is neither gets a row too.
    void rebuild(const ShortcutList &entries);
    // Removes shortcut from the row at row; returns its position there, or
    // -1 when the row does not have it.
    qsizetype takeShortcut(int row, const QString &shortcut);

    ActionInfoMap mActions;
    QStringList mScripts;
    QList<Row> mRows;
};
