#pragma once

#include <QObject>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QMap>
#include <QMultiMap>
#include <QSet>
#include <QDebug>
#include <QStringList>
#include "utils/actions.h"
enum ActionType {
    ACTION_INVALID,
    ACTION_NORMAL,
    ACTION_SCRIPT
};

// <shortcut, action>. A shortcut has at most one action per mode (see
// Actions::categoriesOverlap()): one of the folder view and one of the viewer,
// or one common to both.
using ShortcutMap = QMultiMap<QString, QString>;

class ActionManager : public QObject {
    Q_OBJECT
public:
    static constexpr int kCurrentShortcutsVersion = 3;
    // Adds to shortcuts the defaults of the actions without a shortcut whose
    // shortcut has no action of a common mode yet; true when one was added.
    static bool mergeMissingDefaults(const ShortcutMap &defaults, ShortcutMap &shortcuts);
    // Removes from shortcuts stored before version the defaults that later
    // versions dropped; true when one was removed.
    static bool removeRetiredDefaults(int version, ShortcutMap &shortcuts);
    // The action that shortcut runs in the folder view (folderView) or in the
    // viewer; empty for none. In the folder view a shortcut with Shift runs
    // the selection move of the shortcut without it (Actions::extendsWithShift()).
    static QString actionFor(const ShortcutMap &shortcuts, const QString &shortcut,
                             bool folderView);

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
    const ShortcutMap& allShortcuts();
    const ShortcutMap& defaultShortcuts();
    // The mode the shortcuts run the actions of: the folder view or the
    // viewer.
    void setFolderViewActive(bool active);
    void removeShortcut(const QString &keys);
    void removeAllShortcuts();
    void removeAllShortcuts(QString actionName);
    QString keyForNativeScancode(quint32 scanCode);
    void saveShortcuts();
public slots:
    bool invokeAction(const QString &actionName);
private:
    explicit ActionManager(QObject *parent = nullptr);
    ShortcutMap defaults, shortcuts;
    bool mFolderViewActive = false;

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
    void folderOpenSelected();
    void folderSelectAll();
    void folderMoveLeft();
    void folderMoveRight();
    void folderMoveUp();
    void folderMoveDown();
    void folderPageUp();
    void folderPageDown();
    void folderFirstItem();
    void folderLastItem();
};

extern ActionManager *actionManager;
