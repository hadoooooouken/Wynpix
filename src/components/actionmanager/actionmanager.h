#pragma once

#include <QObject>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QMap>
#include <QSet>
#include <QDebug>
#include <QStringList>
#include "utils/actions.h"
enum ActionType {
    ACTION_INVALID,
    ACTION_NORMAL,
    ACTION_SCRIPT
};

class ActionManager : public QObject {
    Q_OBJECT
public:
    static constexpr int kCurrentShortcutsVersion = 2;
    static bool mergeMissingDefaults(const QMap<QString, QString> &defaults,
                                     QMap<QString, QString> &shortcuts) {
        bool changed = false;
        QSet<QString> boundActions;
        boundActions.reserve(shortcuts.size());
        for (auto it = shortcuts.cbegin(); it != shortcuts.cend(); ++it) {
            boundActions.insert(it.value());
        }

        for (auto it = defaults.cbegin(); it != defaults.cend(); ++it) {
            const QString &defaultKey = it.key();
            const QString &defaultAction = it.value();
            if (!shortcuts.contains(defaultKey) && !boundActions.contains(defaultAction)) {
                shortcuts.insert(defaultKey, defaultAction);
                changed = true;
            }
        }
        return changed;
    }

    static ActionManager* getInstance();
    ~ActionManager();
    bool processEvent(QInputEvent*);
    void addShortcut(const QString &keys, const QString &action);
    void resetDefaults();
    const QString shortcutForAction(QString action);
    const QList<QString> shortcutsForAction(QString action);
    QStringList actionList();
    // The title the settings show for action, translated.
    QString actionTitle(const QString &action);
    // The group the settings show action in.
    ActionCategory actionCategory(const QString &action);
    const QMap<QString,QString>& allShortcuts();
    const QMap<QString,QString>& defaultShortcuts();
    void removeShortcut(const QString &keys);
    void removeAllShortcuts();
    void removeAllShortcuts(QString actionName);
    QString keyForNativeScancode(quint32 scanCode);
    void saveShortcuts();
public slots:
    bool invokeAction(const QString &actionName);
private:
    explicit ActionManager(QObject *parent = nullptr);
    QMap<QString, QString> defaults, shortcuts; // <shortcut, action>

    static void initDefaults();
    static void initActions();
    static void initShortcuts();
    QString modifierKeys(QEvent *event);
    bool invokeActionForShortcut(const QString &action);
    void validateShortcuts();
    void readShortcuts();
    ActionType validateAction(const QString &actionName);

signals:
    void save();
    void saveAs();
    void openSettings();
    void crop();
    void setWallpaper();
    void nextImage();
    void prevImage();
    void fitWindow();
    void fitWidth();
    void fitNormal();
    void fitHeight();
    void flipH();
    void flipV();
    void toggleFitMode();
    void toggleFullscreen();
    void scrollUp();
    void scrollDown();
    void scrollLeft();
    void scrollRight();
    void zoomIn();
    void zoomOut();
    void zoomInCursor();
    void zoomOutCursor();
    void resize();
    void rotateLeft();
    void rotateRight();
    void nextPage();
    void prevPage();
    void firstPage();
    void lastPage();
    void exit();
    void removeFile();
    void copyFile();
    void moveFile();
    void closeFullScreenOrExit();
    void jumpToFirst();
    void jumpToLast();
    void folderView();
    void documentView();
    void runScript(const QString&);
    void toggleFolderView();
    void moveToTrash();
    void reloadImage();
    void copyFileClipboard();
    void copyPathClipboard();
    void copyViewportClipboard();
    void renameFile();
    void contextMenu();
    void toggleTransparencyGrid();
    void sortByName();
    void sortByTime();
    void sortBySize();
    void toggleImageInfo();
    void toggleShuffle();
    void toggleScalingFilter();
    void cycleScalingFilter();
    void showInDirectory();
    void createDirectory();
    void toggleSlideshow();
    void discardEdits();
    void goUp();
    void nextDirectory();
    void prevDirectory();
    void lockZoom();
    void lockView();
    void print();
    void toggleFullscreenInfoBar();
    void pasteFile();
    void togglePanorama();
    void colorAdjustments();
    void casSettings();
    void toggleUpscayl();
    void cycleUpscaylModel();
    void toggleHdrToneMapping();
    void focusNameFilter();
};

extern ActionManager *actionManager;
