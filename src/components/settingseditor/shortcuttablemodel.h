#pragma once

#include <QAbstractTableModel>

#include <optional>

#include "settingsvalues.h"

// The title the settings show for action: its title in titles, "Script:
// <name>" for a script, or action itself when it has no title.
[[nodiscard]] QString shortcutActionTitle(const QString &action, const ActionTitleMap &titles);

// The shortcut table of the settings dialog ("Controls"): one row per
// shortcut, sorted by action title (rows of the same action keep their
// order, new ones go last), columns Action (its title) and Shortcut. A
// shortcut is bound once: putting a shortcut removes the row that had it
// before. Edits stay in the table until the dialog applies them.
//
// QML reads the roles `action`, `title` and `shortcut`. GUI thread only.
class ShortcutTableModel final : public QAbstractTableModel {
    Q_OBJECT

public:
    enum Column { ActionColumn, ShortcutColumn, ColumnCount };
    enum Role { ActionRole = Qt::UserRole + 1, TitleRole, ShortcutRole };

    explicit ShortcutTableModel(QObject *parent = nullptr);

    [[nodiscard]] int rowCount(const QModelIndex &parent = {}) const override;
    [[nodiscard]] int columnCount(const QModelIndex &parent = {}) const override;
    [[nodiscard]] QVariant data(const QModelIndex &index, int role) const override;
    [[nodiscard]] QVariant headerData(int section, Qt::Orientation orientation,
                                      int role) const override;
    [[nodiscard]] QHash<int, QByteArray> roleNames() const override;

    // The titles the Action column shows; sorts the table again.
    void setActionTitles(const ActionTitleMap &titles);

    [[nodiscard]] const ShortcutList &entries() const;
    // Replaces the table; entries are sorted by action title.
    void setEntries(const ShortcutList &entries);
    // The row at row, or std::nullopt when there is none.
    [[nodiscard]] std::optional<ShortcutEntry> entryAt(int row) const;

    // Binds shortcut to action: removes the row replacedRow (an edited row;
    // -1 for none) and the row that had shortcut, then inserts the entry.
    // Returns the row of the new entry, or -1 (nothing changed) when action
    // or shortcut is empty.
    int put(const ShortcutEntry &entry, int replacedRow = -1);
    // Removes the row at row; false when there is none.
    bool removeAt(int row);

private:
    [[nodiscard]] QString title(const QString &action) const;
    [[nodiscard]] bool titleLess(const ShortcutEntry &a, const ShortcutEntry &b) const;
    void sortEntries();
    [[nodiscard]] int insertionRow(const ShortcutEntry &entry) const;

    ShortcutList mEntries;
    ActionTitleMap mTitles;
};
