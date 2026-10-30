#pragma once

#include <QString>
#include <QStringList>

#include <optional>

#include "editorsession.h"
#include "settingsvalues.h"

class IShortcutScriptStore;

// The shortcut creator of the settings dialog (ShortcutCreatorDialog): binds
// a shortcut to one action or script ("s:<name>"), the one whose row it was
// opened from. The shortcut comes from the key, mouse button or wheel input
// the dialog captures; a shortcut another binding of the table has shows a
// warning (that binding is replaced when the dialog is accepted). The table
// is the one being edited, with the changes not applied yet. It can be
// accepted once a shortcut was entered.
//
// GUI thread only.
class ShortcutEditorModel final : public EditorSession {
    Q_OBJECT
    Q_PROPERTY(QString title READ title NOTIFY requestChanged FINAL)
    // The action the shortcut is for, and its category, as the settings
    // show them.
    Q_PROPERTY(QString actionTitle READ actionTitle NOTIFY requestChanged FINAL)
    Q_PROPERTY(QString categoryTitle READ categoryTitle NOTIFY requestChanged FINAL)
    Q_PROPERTY(QString shortcut READ shortcut NOTIFY shortcutChanged FINAL)
    // The shortcut as the settings show it (shortcutDisplayText()), or a
    // prompt while none was entered.
    Q_PROPERTY(QString shortcutText READ shortcutText NOTIFY shortcutChanged FINAL)
    Q_PROPERTY(QString warning READ warning NOTIFY shortcutChanged FINAL)
    Q_PROPERTY(bool canAccept READ canAccept NOTIFY shortcutChanged FINAL)

public:
    // store must outlive the model.
    explicit ShortcutEditorModel(IShortcutScriptStore &store, QObject *parent = nullptr);

    [[nodiscard]] QString title() const;
    [[nodiscard]] QString actionTitle() const;
    [[nodiscard]] QString categoryTitle() const;
    [[nodiscard]] QString shortcut() const;
    [[nodiscard]] QString shortcutText() const;
    [[nodiscard]] QString warning() const;
    [[nodiscard]] bool canAccept() const;

    // Opens the creator for a new shortcut of action; table is the shortcut
    // table.
    void startAdd(const QString &action, const ShortcutList &table);
    // Opens the editor for entry, a binding of table.
    void startEdit(const ShortcutEntry &entry, const ShortcutList &table);

    // A captured shortcut; empty ones (input without a shortcut) are ignored.
    Q_INVOKABLE void setShortcut(const QString &shortcut);

    // The binding the dialog was accepted with.
    [[nodiscard]] ShortcutEntry result() const;
    // The edited binding, std::nullopt for a new shortcut.
    [[nodiscard]] std::optional<ShortcutEntry> edited() const;

signals:
    void requestChanged();
    void shortcutChanged();

protected:
    [[nodiscard]] bool acceptable() const override;

private:
    void startRequest(const QString &title, const QString &action, const ShortcutList &table);
    // Action of the table binding that has shortcut, other than the edited
    // one; empty for none.
    [[nodiscard]] QString boundAction(const QString &shortcut) const;

    IShortcutScriptStore &mStore;
    ShortcutList mTable;
    QString mTitle;
    ActionInfoMap mInfos;
    QString mAction;
    QString mShortcut;
    QString mWarning;
    std::optional<ShortcutEntry> mEdited;
};
