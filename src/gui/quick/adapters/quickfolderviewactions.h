#pragma once

#include <QObject>
#include <QString>

#include <memory>

class ActionManager;
class DirectoryViewAdapter;
class FileSystemModelCustom;
class FolderGridController;
class FolderViewController;
class Settings;
class UiEvents;

// Everything the folder view works with. All referenced objects must
// outlive QuickFolderViewActions.
struct QuickFolderViewContext {
    FolderViewController &folderView;
    FolderGridController &grid;
    // The grid's directory view, driven by Core's folder view presenter.
    DirectoryViewAdapter &gridView;
    UiEvents &events;
    Settings &settings;
    ActionManager &actions;
};

// Connects the Qt Quick folder view (FolderViewController with its
// FolderGridController) to the application, as FolderView / FolderViewProxy
// and MW did in 3.6.4:
// - creates the folder tree (FileSystemModelCustom, folders only) when the
//   view asks for it on its first activation, and re-reads it on request;
// - sends sorting, filters, folder selections, copy / move drops, batch
//   conversion (the button and the "Add to Batch Converter" row) and the
//   readiness of the grid and the tree to Core
//   (UiEvents);
// - turns the grid's requests into the directory view's signals (drag
//   hover, drops, opening the selection);
// - runs the folder view actions on the grid (open the selected item,
//   select all, the selection moves) and focuses the name filter on its
//   action ("focusNameFilter");
// - stores the icon size, the places panel layout and the bookmarks.
// GUI thread only.
class QuickFolderViewActions final : public QObject {
    Q_OBJECT
public:
    explicit QuickFolderViewActions(const QuickFolderViewContext &context,
                                    QObject *parent = nullptr);
    ~QuickFolderViewActions() override;

    // Re-reads directoryPath in the folder tree (empty: everything).
    void refreshFolderTree(const QString &directoryPath);

private:
    void createFolderTree();
    void forwardIntents();
    void forwardGridRequests();
    void storeLayout();

    FolderViewController &folderView;
    FolderGridController &grid;
    DirectoryViewAdapter &gridView;
    UiEvents &events;
    Settings &settings;
    ActionManager &actions;
    std::unique_ptr<FileSystemModelCustom> folderTree;
};
