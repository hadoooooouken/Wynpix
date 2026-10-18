#pragma once

#include <QObject>

class ActionManager;
class ContextMenuModel;
class ImageViewportController;
class ScriptManager;

// Connects the Qt Quick context menu (ContextMenuModel) to the application:
// the menu action of ActionManager shows it at the pointer, after the scripts
// (those with a command, as the widget menu lists them), the displayed-image
// state and the CAS filter state were brought up to date. The script
// settings request ("Configure menu") opens the settings window, which the
// Quick UI host connects. The menu closes when a pointer interaction with the
// image starts (a drag, a gesture zoom or a wheel turn).
//
// All referenced objects must outlive this object. GUI thread only.
class QuickContextMenuActions final : public QObject {
    Q_OBJECT
public:
    QuickContextMenuActions(ActionManager &actions, ScriptManager &scripts,
                            ContextMenuModel &menu, ImageViewportController &viewport,
                            QObject *parent = nullptr);

private:
    void showMenu();

    ScriptManager &scripts;
    ContextMenuModel &menu;
    ImageViewportController &viewport;
};
