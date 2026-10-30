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
  return mEntries.contains(action);
}

QStringList Actions::getList() const {
  QStringList list = mEntries.keys();
  list.sort();
  return list;
}

QString Actions::title(const QString &action) const {
  const char *source = mEntries.value(action).title;
  if (!source)
    return action;
  return QCoreApplication::translate(kTitleContext, source);
}

ActionCategory Actions::category(const QString &action) const {
  return mEntries.value(action).category;
}

QString Actions::categoryTitle(ActionCategory category) {
  const char *source = nullptr;
  switch (category) {
  case ActionCategory::FolderView:
    source = QT_TRANSLATE_NOOP("Actions", "Folder view");
    break;
  case ActionCategory::Viewer:
    source = QT_TRANSLATE_NOOP("Actions", "Viewer");
    break;
  case ActionCategory::Common:
    source = QT_TRANSLATE_NOOP("Actions", "Common");
    break;
  }
  return QCoreApplication::translate(kTitleContext, source);
}

bool Actions::categoriesOverlap(ActionCategory one, ActionCategory other) {
  return one == other || one == ActionCategory::Common || other == ActionCategory::Common;
}

bool Actions::worksIn(ActionCategory category, bool folderView) {
  return categoriesOverlap(category, folderView ? ActionCategory::FolderView
                                                : ActionCategory::Viewer);
}

void Actions::add(const QString &action, const char *title, ActionCategory category) {
  mEntries.insert(action, {.title = title, .category = category});
}

void Actions::init() {
  add(u"nextImage"_s, QT_TRANSLATE_NOOP("Actions", "Next image"), ActionCategory::Viewer);
  add(u"prevImage"_s, QT_TRANSLATE_NOOP("Actions", "Previous image"), ActionCategory::Viewer);
  add(u"toggleFullscreen"_s, QT_TRANSLATE_NOOP("Actions", "Toggle fullscreen"), ActionCategory::Common);
  add(u"fitWindow"_s, QT_TRANSLATE_NOOP("Actions", "Fit to window"), ActionCategory::Viewer);
  add(u"fitWidth"_s, QT_TRANSLATE_NOOP("Actions", "Fit to width"), ActionCategory::Viewer);
  add(u"fitNormal"_s, QT_TRANSLATE_NOOP("Actions", "Original size"), ActionCategory::Viewer);
  add(u"fitHeight"_s, QT_TRANSLATE_NOOP("Actions", "Fit to height"), ActionCategory::Viewer);
  add(u"toggleFitMode"_s, QT_TRANSLATE_NOOP("Actions", "Switch fit mode"), ActionCategory::Viewer);
  add(u"resize"_s, QT_TRANSLATE_NOOP("Actions", "Resize"), ActionCategory::Common);
  add(u"rotateRight"_s, QT_TRANSLATE_NOOP("Actions", "Rotate right"), ActionCategory::Common);
  add(u"rotateLeft"_s, QT_TRANSLATE_NOOP("Actions", "Rotate left"), ActionCategory::Common);
  add(u"scrollUp"_s, QT_TRANSLATE_NOOP("Actions", "Scroll up"), ActionCategory::Viewer);
  add(u"scrollDown"_s, QT_TRANSLATE_NOOP("Actions", "Scroll down"), ActionCategory::Viewer);
  add(u"scrollLeft"_s, QT_TRANSLATE_NOOP("Actions", "Scroll left"), ActionCategory::Viewer);
  add(u"scrollRight"_s, QT_TRANSLATE_NOOP("Actions", "Scroll right"), ActionCategory::Viewer);
  add(u"zoomIn"_s, QT_TRANSLATE_NOOP("Actions", "Zoom in"), ActionCategory::Viewer);
  add(u"zoomOut"_s, QT_TRANSLATE_NOOP("Actions", "Zoom out"), ActionCategory::Viewer);
  add(u"zoomInCursor"_s, QT_TRANSLATE_NOOP("Actions", "Zoom in at cursor"), ActionCategory::Viewer);
  add(u"zoomOutCursor"_s, QT_TRANSLATE_NOOP("Actions", "Zoom out at cursor"), ActionCategory::Viewer);
  add(u"save"_s, QT_TRANSLATE_NOOP("Actions", "Save"), ActionCategory::Common);
  add(u"saveAs"_s, QT_TRANSLATE_NOOP("Actions", "Save as"), ActionCategory::Common);
  add(u"setWallpaper"_s, QT_TRANSLATE_NOOP("Actions", "Set as wallpaper"), ActionCategory::Common);
  add(u"crop"_s, QT_TRANSLATE_NOOP("Actions", "Crop"), ActionCategory::Viewer);
  add(u"removeFile"_s, QT_TRANSLATE_NOOP("Actions", "Delete permanently"), ActionCategory::Common);
  add(u"copyFile"_s, QT_TRANSLATE_NOOP("Actions", "Quick copy"), ActionCategory::Viewer);
  add(u"moveFile"_s, QT_TRANSLATE_NOOP("Actions", "Quick move"), ActionCategory::Viewer);
  add(u"jumpToFirst"_s, QT_TRANSLATE_NOOP("Actions", "First image"), ActionCategory::Viewer);
  add(u"jumpToLast"_s, QT_TRANSLATE_NOOP("Actions", "Last image"), ActionCategory::Viewer);
  add(u"openSettings"_s, QT_TRANSLATE_NOOP("Actions", "Settings"), ActionCategory::Common);
  add(u"closeFullScreenOrExit"_s, QT_TRANSLATE_NOOP("Actions", "Exit fullscreen or quit"), ActionCategory::Common);
  add(u"exit"_s, QT_TRANSLATE_NOOP("Actions", "Quit"), ActionCategory::Common);
  add(u"flipH"_s, QT_TRANSLATE_NOOP("Actions", "Flip horizontal"), ActionCategory::Common);
  add(u"flipV"_s, QT_TRANSLATE_NOOP("Actions", "Flip vertical"), ActionCategory::Common);
  add(u"folderView"_s, QT_TRANSLATE_NOOP("Actions", "Folder view"), ActionCategory::Viewer);
  add(u"documentView"_s, QT_TRANSLATE_NOOP("Actions", "Image view"), ActionCategory::FolderView);
  add(u"toggleFolderView"_s, QT_TRANSLATE_NOOP("Actions", "Toggle folder view"), ActionCategory::Common);
  add(u"moveToTrash"_s, QT_TRANSLATE_NOOP("Actions", "Move to trash"), ActionCategory::Common);
  add(u"reloadImage"_s, QT_TRANSLATE_NOOP("Actions", "Reload image"), ActionCategory::Common);
  add(u"copyFileClipboard"_s, QT_TRANSLATE_NOOP("Actions", "Copy image to clipboard"), ActionCategory::Common);
  add(u"copyPathClipboard"_s, QT_TRANSLATE_NOOP("Actions", "Copy file path"), ActionCategory::Common);
  add(u"copyViewportClipboard"_s, QT_TRANSLATE_NOOP("Actions", "Copy visible area to clipboard"), ActionCategory::Viewer);
  add(u"renameFile"_s, QT_TRANSLATE_NOOP("Actions", "Rename"), ActionCategory::Common);
  add(u"contextMenu"_s, QT_TRANSLATE_NOOP("Actions", "Context menu"), ActionCategory::Common);
  add(u"toggleTransparencyGrid"_s, QT_TRANSLATE_NOOP("Actions", "Toggle transparency grid"), ActionCategory::Viewer);
  add(u"sortByName"_s, QT_TRANSLATE_NOOP("Actions", "Sort by name"), ActionCategory::Common);
  add(u"sortByTime"_s, QT_TRANSLATE_NOOP("Actions", "Sort by date"), ActionCategory::Common);
  add(u"sortBySize"_s, QT_TRANSLATE_NOOP("Actions", "Sort by size"), ActionCategory::Common);
  add(u"toggleImageInfo"_s, QT_TRANSLATE_NOOP("Actions", "Toggle image info"), ActionCategory::Viewer);
  add(u"toggleShuffle"_s, QT_TRANSLATE_NOOP("Actions", "Toggle shuffle"), ActionCategory::Viewer);
  add(u"toggleScalingFilter"_s, QT_TRANSLATE_NOOP("Actions", "Toggle scaling filter"), ActionCategory::Viewer);
  add(u"cycleScalingFilter"_s, QT_TRANSLATE_NOOP("Actions", "Next scaling filter"), ActionCategory::Viewer);
  add(u"toggleSlideshow"_s, QT_TRANSLATE_NOOP("Actions", "Toggle slideshow"), ActionCategory::Viewer);
  add(u"showInDirectory"_s, QT_TRANSLATE_NOOP("Actions", "Show in folder"), ActionCategory::Common);
  add(u"goUp"_s, QT_TRANSLATE_NOOP("Actions", "Parent folder"), ActionCategory::FolderView);
  add(u"discardEdits"_s, QT_TRANSLATE_NOOP("Actions", "Discard edits"), ActionCategory::Common);
  add(u"nextDirectory"_s, QT_TRANSLATE_NOOP("Actions", "Next folder"), ActionCategory::Viewer);
  add(u"prevDirectory"_s, QT_TRANSLATE_NOOP("Actions", "Previous folder"), ActionCategory::Viewer);
  add(u"lockZoom"_s, QT_TRANSLATE_NOOP("Actions", "Lock zoom"), ActionCategory::Viewer);
  add(u"lockView"_s, QT_TRANSLATE_NOOP("Actions", "Lock view"), ActionCategory::Viewer);
  add(u"print"_s, QT_TRANSLATE_NOOP("Actions", "Print"), ActionCategory::Common);
  add(u"toggleFullscreenInfoBar"_s, QT_TRANSLATE_NOOP("Actions", "Toggle fullscreen info bar"), ActionCategory::Viewer);
  add(u"pasteFile"_s, QT_TRANSLATE_NOOP("Actions", "Open from clipboard"), ActionCategory::Common);
  add(u"togglePanorama"_s, QT_TRANSLATE_NOOP("Actions", "Panorama mode"), ActionCategory::Viewer);
  add(u"colorAdjustments"_s, QT_TRANSLATE_NOOP("Actions", "Color adjustments"), ActionCategory::Viewer);
  add(u"casSettings"_s, QT_TRANSLATE_NOOP("Actions", "CAS settings"), ActionCategory::Viewer);
  add(u"createDirectory"_s, QT_TRANSLATE_NOOP("Actions", "New folder"), ActionCategory::Common);
  add(u"nextPage"_s, QT_TRANSLATE_NOOP("Actions", "Next page"), ActionCategory::Viewer);
  add(u"prevPage"_s, QT_TRANSLATE_NOOP("Actions", "Previous page"), ActionCategory::Viewer);
  add(u"firstPage"_s, QT_TRANSLATE_NOOP("Actions", "First page"), ActionCategory::Viewer);
  add(u"lastPage"_s, QT_TRANSLATE_NOOP("Actions", "Last page"), ActionCategory::Viewer);
  add(u"toggleUpscayl"_s, QT_TRANSLATE_NOOP("Actions", "Toggle AI upscale"), ActionCategory::Viewer);
  add(u"cycleUpscaylModel"_s, QT_TRANSLATE_NOOP("Actions", "Next AI upscale model"), ActionCategory::Viewer);
  add(u"toggleHdrToneMapping"_s, QT_TRANSLATE_NOOP("Actions", "Toggle HDR tone mapping"), ActionCategory::Viewer);
  add(u"focusNameFilter"_s, QT_TRANSLATE_NOOP("Actions", "Name filter"), ActionCategory::FolderView);
}
