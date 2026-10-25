#include "actions.h"

Actions *appActions = nullptr;

Actions::Actions() { init(); }

Actions *Actions::getInstance() {
  if (!appActions)
    appActions = new Actions();
  return appActions;
}

bool Actions::contains(const QString &action) const {
  return mActions.contains(action);
}

QStringList Actions::getList() const {
  QStringList list = mActions.values();
  list.sort();
  return list;
}

void Actions::init() {
  mActions.insert("nextImage");
  mActions.insert("prevImage");
  mActions.insert("toggleFullscreen");
  mActions.insert("fitWindow");
  mActions.insert("fitWidth");
  mActions.insert("fitNormal");
  mActions.insert("fitHeight");
  mActions.insert("toggleFitMode");
  mActions.insert("resize");
  mActions.insert("rotateRight");
  mActions.insert("rotateLeft");
  mActions.insert("scrollUp");
  mActions.insert("scrollDown");
  mActions.insert("scrollLeft");
  mActions.insert("scrollRight");
  mActions.insert("zoomIn");
  mActions.insert("zoomOut");
  mActions.insert("zoomInCursor");
  mActions.insert("zoomOutCursor");
  mActions.insert("save");
  mActions.insert("saveAs");
  mActions.insert("setWallpaper");
  mActions.insert("crop");
  mActions.insert("removeFile");
  mActions.insert("copyFile");
  mActions.insert("moveFile");
  mActions.insert("jumpToFirst");
  mActions.insert("jumpToLast");
  mActions.insert("openSettings");
  mActions.insert("closeFullScreenOrExit");
  mActions.insert("exit");
  mActions.insert("flipH");
  mActions.insert("flipV");
  mActions.insert("folderView");
  mActions.insert("documentView");
  mActions.insert("toggleFolderView");
  mActions.insert("moveToTrash");
  mActions.insert("reloadImage");
  mActions.insert("copyFileClipboard");
  mActions.insert("copyPathClipboard");
  mActions.insert("copyViewportClipboard");
  mActions.insert("renameFile");
  mActions.insert("contextMenu");
  mActions.insert("toggleTransparencyGrid");
  mActions.insert("sortByName");
  mActions.insert("sortByTime");
  mActions.insert("sortBySize");
  mActions.insert("toggleImageInfo");
  mActions.insert("toggleShuffle");
  mActions.insert("toggleScalingFilter");
  mActions.insert("cycleScalingFilter");
  mActions.insert("toggleSlideshow");
  mActions.insert("showInDirectory");
  mActions.insert("goUp");
  mActions.insert("discardEdits");
  mActions.insert("nextDirectory");
  mActions.insert("prevDirectory");
  mActions.insert("lockZoom");
  mActions.insert("lockView");
  mActions.insert("print");
  mActions.insert("toggleFullscreenInfoBar");
  mActions.insert("pasteFile");
  mActions.insert("togglePanorama");
  mActions.insert("colorAdjustments");
  mActions.insert("casSettings");
  mActions.insert("createDirectory");
  mActions.insert("nextPage");
  mActions.insert("prevPage");
  mActions.insert("firstPage");
  mActions.insert("lastPage");
  mActions.insert("toggleUpscayl");
  mActions.insert("cycleUpscaylModel");
  mActions.insert("toggleHdrToneMapping");
}
