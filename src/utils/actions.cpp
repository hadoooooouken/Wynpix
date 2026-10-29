#include "actions.h"

#include <QCoreApplication>

using namespace Qt::StringLiterals;

namespace {
constexpr char kTitleContext[] = "Actions";
} // namespace

Actions *appActions = nullptr;

Actions::Actions() { init(); }

Actions *Actions::getInstance() {
  if (!appActions)
    appActions = new Actions();
  return appActions;
}

bool Actions::contains(const QString &action) const {
  return mTitles.contains(action);
}

QStringList Actions::getList() const {
  QStringList list = mTitles.keys();
  list.sort();
  return list;
}

QString Actions::title(const QString &action) const {
  const char *source = mTitles.value(action);
  if (!source)
    return action;
  return QCoreApplication::translate(kTitleContext, source);
}

void Actions::add(const QString &action, const char *title) {
  mTitles.insert(action, title);
}

void Actions::init() {
  add(u"nextImage"_s, QT_TRANSLATE_NOOP("Actions", "Next image"));
  add(u"prevImage"_s, QT_TRANSLATE_NOOP("Actions", "Previous image"));
  add(u"toggleFullscreen"_s, QT_TRANSLATE_NOOP("Actions", "Toggle fullscreen"));
  add(u"fitWindow"_s, QT_TRANSLATE_NOOP("Actions", "Fit to window"));
  add(u"fitWidth"_s, QT_TRANSLATE_NOOP("Actions", "Fit to width"));
  add(u"fitNormal"_s, QT_TRANSLATE_NOOP("Actions", "Original size"));
  add(u"fitHeight"_s, QT_TRANSLATE_NOOP("Actions", "Fit to height"));
  add(u"toggleFitMode"_s, QT_TRANSLATE_NOOP("Actions", "Switch fit mode"));
  add(u"resize"_s, QT_TRANSLATE_NOOP("Actions", "Resize"));
  add(u"rotateRight"_s, QT_TRANSLATE_NOOP("Actions", "Rotate right"));
  add(u"rotateLeft"_s, QT_TRANSLATE_NOOP("Actions", "Rotate left"));
  add(u"scrollUp"_s, QT_TRANSLATE_NOOP("Actions", "Scroll up"));
  add(u"scrollDown"_s, QT_TRANSLATE_NOOP("Actions", "Scroll down"));
  add(u"scrollLeft"_s, QT_TRANSLATE_NOOP("Actions", "Scroll left"));
  add(u"scrollRight"_s, QT_TRANSLATE_NOOP("Actions", "Scroll right"));
  add(u"zoomIn"_s, QT_TRANSLATE_NOOP("Actions", "Zoom in"));
  add(u"zoomOut"_s, QT_TRANSLATE_NOOP("Actions", "Zoom out"));
  add(u"zoomInCursor"_s, QT_TRANSLATE_NOOP("Actions", "Zoom in at cursor"));
  add(u"zoomOutCursor"_s, QT_TRANSLATE_NOOP("Actions", "Zoom out at cursor"));
  add(u"save"_s, QT_TRANSLATE_NOOP("Actions", "Save"));
  add(u"saveAs"_s, QT_TRANSLATE_NOOP("Actions", "Save as"));
  add(u"setWallpaper"_s, QT_TRANSLATE_NOOP("Actions", "Set as wallpaper"));
  add(u"crop"_s, QT_TRANSLATE_NOOP("Actions", "Crop"));
  add(u"removeFile"_s, QT_TRANSLATE_NOOP("Actions", "Delete permanently"));
  add(u"copyFile"_s, QT_TRANSLATE_NOOP("Actions", "Quick copy"));
  add(u"moveFile"_s, QT_TRANSLATE_NOOP("Actions", "Quick move"));
  add(u"jumpToFirst"_s, QT_TRANSLATE_NOOP("Actions", "First image"));
  add(u"jumpToLast"_s, QT_TRANSLATE_NOOP("Actions", "Last image"));
  add(u"openSettings"_s, QT_TRANSLATE_NOOP("Actions", "Settings"));
  add(u"closeFullScreenOrExit"_s, QT_TRANSLATE_NOOP("Actions", "Exit fullscreen or quit"));
  add(u"exit"_s, QT_TRANSLATE_NOOP("Actions", "Quit"));
  add(u"flipH"_s, QT_TRANSLATE_NOOP("Actions", "Flip horizontal"));
  add(u"flipV"_s, QT_TRANSLATE_NOOP("Actions", "Flip vertical"));
  add(u"folderView"_s, QT_TRANSLATE_NOOP("Actions", "Folder view"));
  add(u"documentView"_s, QT_TRANSLATE_NOOP("Actions", "Image view"));
  add(u"toggleFolderView"_s, QT_TRANSLATE_NOOP("Actions", "Toggle folder view"));
  add(u"moveToTrash"_s, QT_TRANSLATE_NOOP("Actions", "Move to trash"));
  add(u"reloadImage"_s, QT_TRANSLATE_NOOP("Actions", "Reload image"));
  add(u"copyFileClipboard"_s, QT_TRANSLATE_NOOP("Actions", "Copy image to clipboard"));
  add(u"copyPathClipboard"_s, QT_TRANSLATE_NOOP("Actions", "Copy file path"));
  add(u"copyViewportClipboard"_s, QT_TRANSLATE_NOOP("Actions", "Copy visible area to clipboard"));
  add(u"renameFile"_s, QT_TRANSLATE_NOOP("Actions", "Rename"));
  add(u"contextMenu"_s, QT_TRANSLATE_NOOP("Actions", "Context menu"));
  add(u"toggleTransparencyGrid"_s, QT_TRANSLATE_NOOP("Actions", "Toggle transparency grid"));
  add(u"sortByName"_s, QT_TRANSLATE_NOOP("Actions", "Sort by name"));
  add(u"sortByTime"_s, QT_TRANSLATE_NOOP("Actions", "Sort by date"));
  add(u"sortBySize"_s, QT_TRANSLATE_NOOP("Actions", "Sort by size"));
  add(u"toggleImageInfo"_s, QT_TRANSLATE_NOOP("Actions", "Toggle image info"));
  add(u"toggleShuffle"_s, QT_TRANSLATE_NOOP("Actions", "Toggle shuffle"));
  add(u"toggleScalingFilter"_s, QT_TRANSLATE_NOOP("Actions", "Toggle scaling filter"));
  add(u"cycleScalingFilter"_s, QT_TRANSLATE_NOOP("Actions", "Next scaling filter"));
  add(u"toggleSlideshow"_s, QT_TRANSLATE_NOOP("Actions", "Toggle slideshow"));
  add(u"showInDirectory"_s, QT_TRANSLATE_NOOP("Actions", "Show in folder"));
  add(u"goUp"_s, QT_TRANSLATE_NOOP("Actions", "Parent folder"));
  add(u"discardEdits"_s, QT_TRANSLATE_NOOP("Actions", "Discard edits"));
  add(u"nextDirectory"_s, QT_TRANSLATE_NOOP("Actions", "Next folder"));
  add(u"prevDirectory"_s, QT_TRANSLATE_NOOP("Actions", "Previous folder"));
  add(u"lockZoom"_s, QT_TRANSLATE_NOOP("Actions", "Lock zoom"));
  add(u"lockView"_s, QT_TRANSLATE_NOOP("Actions", "Lock view"));
  add(u"print"_s, QT_TRANSLATE_NOOP("Actions", "Print"));
  add(u"toggleFullscreenInfoBar"_s, QT_TRANSLATE_NOOP("Actions", "Toggle fullscreen info bar"));
  add(u"pasteFile"_s, QT_TRANSLATE_NOOP("Actions", "Open from clipboard"));
  add(u"togglePanorama"_s, QT_TRANSLATE_NOOP("Actions", "Panorama mode"));
  add(u"colorAdjustments"_s, QT_TRANSLATE_NOOP("Actions", "Color adjustments"));
  add(u"casSettings"_s, QT_TRANSLATE_NOOP("Actions", "CAS settings"));
  add(u"createDirectory"_s, QT_TRANSLATE_NOOP("Actions", "New folder"));
  add(u"nextPage"_s, QT_TRANSLATE_NOOP("Actions", "Next page"));
  add(u"prevPage"_s, QT_TRANSLATE_NOOP("Actions", "Previous page"));
  add(u"firstPage"_s, QT_TRANSLATE_NOOP("Actions", "First page"));
  add(u"lastPage"_s, QT_TRANSLATE_NOOP("Actions", "Last page"));
  add(u"toggleUpscayl"_s, QT_TRANSLATE_NOOP("Actions", "Toggle AI upscale"));
  add(u"cycleUpscaylModel"_s, QT_TRANSLATE_NOOP("Actions", "Next AI upscale model"));
  add(u"toggleHdrToneMapping"_s, QT_TRANSLATE_NOOP("Actions", "Toggle HDR tone mapping"));
}
