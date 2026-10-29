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
  case ActionCategory::Navigation:
    source = QT_TRANSLATE_NOOP("Actions", "Navigation");
    break;
  case ActionCategory::View:
    source = QT_TRANSLATE_NOOP("Actions", "View");
    break;
  case ActionCategory::Editing:
    source = QT_TRANSLATE_NOOP("Actions", "Editing");
    break;
  case ActionCategory::File:
    source = QT_TRANSLATE_NOOP("Actions", "File");
    break;
  case ActionCategory::Clipboard:
    source = QT_TRANSLATE_NOOP("Actions", "Clipboard");
    break;
  case ActionCategory::Upscale:
    source = QT_TRANSLATE_NOOP("Actions", "AI upscale");
    break;
  case ActionCategory::Application:
    source = QT_TRANSLATE_NOOP("Actions", "Application");
    break;
  }
  return QCoreApplication::translate(kTitleContext, source);
}

void Actions::add(const QString &action, const char *title, ActionCategory category) {
  mEntries.insert(action, {.title = title, .category = category});
}

void Actions::init() {
  add(u"nextImage"_s, QT_TRANSLATE_NOOP("Actions", "Next image"), ActionCategory::Navigation);
  add(u"prevImage"_s, QT_TRANSLATE_NOOP("Actions", "Previous image"), ActionCategory::Navigation);
  add(u"toggleFullscreen"_s, QT_TRANSLATE_NOOP("Actions", "Toggle fullscreen"), ActionCategory::View);
  add(u"fitWindow"_s, QT_TRANSLATE_NOOP("Actions", "Fit to window"), ActionCategory::View);
  add(u"fitWidth"_s, QT_TRANSLATE_NOOP("Actions", "Fit to width"), ActionCategory::View);
  add(u"fitNormal"_s, QT_TRANSLATE_NOOP("Actions", "Original size"), ActionCategory::View);
  add(u"fitHeight"_s, QT_TRANSLATE_NOOP("Actions", "Fit to height"), ActionCategory::View);
  add(u"toggleFitMode"_s, QT_TRANSLATE_NOOP("Actions", "Switch fit mode"), ActionCategory::View);
  add(u"resize"_s, QT_TRANSLATE_NOOP("Actions", "Resize"), ActionCategory::Editing);
  add(u"rotateRight"_s, QT_TRANSLATE_NOOP("Actions", "Rotate right"), ActionCategory::Editing);
  add(u"rotateLeft"_s, QT_TRANSLATE_NOOP("Actions", "Rotate left"), ActionCategory::Editing);
  add(u"scrollUp"_s, QT_TRANSLATE_NOOP("Actions", "Scroll up"), ActionCategory::View);
  add(u"scrollDown"_s, QT_TRANSLATE_NOOP("Actions", "Scroll down"), ActionCategory::View);
  add(u"scrollLeft"_s, QT_TRANSLATE_NOOP("Actions", "Scroll left"), ActionCategory::View);
  add(u"scrollRight"_s, QT_TRANSLATE_NOOP("Actions", "Scroll right"), ActionCategory::View);
  add(u"zoomIn"_s, QT_TRANSLATE_NOOP("Actions", "Zoom in"), ActionCategory::View);
  add(u"zoomOut"_s, QT_TRANSLATE_NOOP("Actions", "Zoom out"), ActionCategory::View);
  add(u"zoomInCursor"_s, QT_TRANSLATE_NOOP("Actions", "Zoom in at cursor"), ActionCategory::View);
  add(u"zoomOutCursor"_s, QT_TRANSLATE_NOOP("Actions", "Zoom out at cursor"), ActionCategory::View);
  add(u"save"_s, QT_TRANSLATE_NOOP("Actions", "Save"), ActionCategory::File);
  add(u"saveAs"_s, QT_TRANSLATE_NOOP("Actions", "Save as"), ActionCategory::File);
  add(u"setWallpaper"_s, QT_TRANSLATE_NOOP("Actions", "Set as wallpaper"), ActionCategory::File);
  add(u"crop"_s, QT_TRANSLATE_NOOP("Actions", "Crop"), ActionCategory::Editing);
  add(u"removeFile"_s, QT_TRANSLATE_NOOP("Actions", "Delete permanently"), ActionCategory::File);
  add(u"copyFile"_s, QT_TRANSLATE_NOOP("Actions", "Quick copy"), ActionCategory::File);
  add(u"moveFile"_s, QT_TRANSLATE_NOOP("Actions", "Quick move"), ActionCategory::File);
  add(u"jumpToFirst"_s, QT_TRANSLATE_NOOP("Actions", "First image"), ActionCategory::Navigation);
  add(u"jumpToLast"_s, QT_TRANSLATE_NOOP("Actions", "Last image"), ActionCategory::Navigation);
  add(u"openSettings"_s, QT_TRANSLATE_NOOP("Actions", "Settings"), ActionCategory::Application);
  add(u"closeFullScreenOrExit"_s, QT_TRANSLATE_NOOP("Actions", "Exit fullscreen or quit"), ActionCategory::Application);
  add(u"exit"_s, QT_TRANSLATE_NOOP("Actions", "Quit"), ActionCategory::Application);
  add(u"flipH"_s, QT_TRANSLATE_NOOP("Actions", "Flip horizontal"), ActionCategory::Editing);
  add(u"flipV"_s, QT_TRANSLATE_NOOP("Actions", "Flip vertical"), ActionCategory::Editing);
  add(u"folderView"_s, QT_TRANSLATE_NOOP("Actions", "Folder view"), ActionCategory::View);
  add(u"documentView"_s, QT_TRANSLATE_NOOP("Actions", "Image view"), ActionCategory::View);
  add(u"toggleFolderView"_s, QT_TRANSLATE_NOOP("Actions", "Toggle folder view"), ActionCategory::View);
  add(u"moveToTrash"_s, QT_TRANSLATE_NOOP("Actions", "Move to trash"), ActionCategory::File);
  add(u"reloadImage"_s, QT_TRANSLATE_NOOP("Actions", "Reload image"), ActionCategory::File);
  add(u"copyFileClipboard"_s, QT_TRANSLATE_NOOP("Actions", "Copy image to clipboard"), ActionCategory::Clipboard);
  add(u"copyPathClipboard"_s, QT_TRANSLATE_NOOP("Actions", "Copy file path"), ActionCategory::Clipboard);
  add(u"copyViewportClipboard"_s, QT_TRANSLATE_NOOP("Actions", "Copy visible area to clipboard"), ActionCategory::Clipboard);
  add(u"renameFile"_s, QT_TRANSLATE_NOOP("Actions", "Rename"), ActionCategory::File);
  add(u"contextMenu"_s, QT_TRANSLATE_NOOP("Actions", "Context menu"), ActionCategory::Application);
  add(u"toggleTransparencyGrid"_s, QT_TRANSLATE_NOOP("Actions", "Toggle transparency grid"), ActionCategory::View);
  add(u"sortByName"_s, QT_TRANSLATE_NOOP("Actions", "Sort by name"), ActionCategory::Navigation);
  add(u"sortByTime"_s, QT_TRANSLATE_NOOP("Actions", "Sort by date"), ActionCategory::Navigation);
  add(u"sortBySize"_s, QT_TRANSLATE_NOOP("Actions", "Sort by size"), ActionCategory::Navigation);
  add(u"toggleImageInfo"_s, QT_TRANSLATE_NOOP("Actions", "Toggle image info"), ActionCategory::View);
  add(u"toggleShuffle"_s, QT_TRANSLATE_NOOP("Actions", "Toggle shuffle"), ActionCategory::Navigation);
  add(u"toggleScalingFilter"_s, QT_TRANSLATE_NOOP("Actions", "Toggle scaling filter"), ActionCategory::View);
  add(u"cycleScalingFilter"_s, QT_TRANSLATE_NOOP("Actions", "Next scaling filter"), ActionCategory::View);
  add(u"toggleSlideshow"_s, QT_TRANSLATE_NOOP("Actions", "Toggle slideshow"), ActionCategory::Navigation);
  add(u"showInDirectory"_s, QT_TRANSLATE_NOOP("Actions", "Show in folder"), ActionCategory::File);
  add(u"goUp"_s, QT_TRANSLATE_NOOP("Actions", "Parent folder"), ActionCategory::Navigation);
  add(u"discardEdits"_s, QT_TRANSLATE_NOOP("Actions", "Discard edits"), ActionCategory::Editing);
  add(u"nextDirectory"_s, QT_TRANSLATE_NOOP("Actions", "Next folder"), ActionCategory::Navigation);
  add(u"prevDirectory"_s, QT_TRANSLATE_NOOP("Actions", "Previous folder"), ActionCategory::Navigation);
  add(u"lockZoom"_s, QT_TRANSLATE_NOOP("Actions", "Lock zoom"), ActionCategory::View);
  add(u"lockView"_s, QT_TRANSLATE_NOOP("Actions", "Lock view"), ActionCategory::View);
  add(u"print"_s, QT_TRANSLATE_NOOP("Actions", "Print"), ActionCategory::File);
  add(u"toggleFullscreenInfoBar"_s, QT_TRANSLATE_NOOP("Actions", "Toggle fullscreen info bar"), ActionCategory::View);
  add(u"pasteFile"_s, QT_TRANSLATE_NOOP("Actions", "Open from clipboard"), ActionCategory::Clipboard);
  add(u"togglePanorama"_s, QT_TRANSLATE_NOOP("Actions", "Panorama mode"), ActionCategory::View);
  add(u"colorAdjustments"_s, QT_TRANSLATE_NOOP("Actions", "Color adjustments"), ActionCategory::Editing);
  add(u"casSettings"_s, QT_TRANSLATE_NOOP("Actions", "CAS settings"), ActionCategory::View);
  add(u"createDirectory"_s, QT_TRANSLATE_NOOP("Actions", "New folder"), ActionCategory::File);
  add(u"nextPage"_s, QT_TRANSLATE_NOOP("Actions", "Next page"), ActionCategory::Navigation);
  add(u"prevPage"_s, QT_TRANSLATE_NOOP("Actions", "Previous page"), ActionCategory::Navigation);
  add(u"firstPage"_s, QT_TRANSLATE_NOOP("Actions", "First page"), ActionCategory::Navigation);
  add(u"lastPage"_s, QT_TRANSLATE_NOOP("Actions", "Last page"), ActionCategory::Navigation);
  add(u"toggleUpscayl"_s, QT_TRANSLATE_NOOP("Actions", "Toggle AI upscale"), ActionCategory::Upscale);
  add(u"cycleUpscaylModel"_s, QT_TRANSLATE_NOOP("Actions", "Next AI upscale model"), ActionCategory::Upscale);
  add(u"toggleHdrToneMapping"_s, QT_TRANSLATE_NOOP("Actions", "Toggle HDR tone mapping"), ActionCategory::View);
  add(u"focusNameFilter"_s, QT_TRANSLATE_NOOP("Actions", "Name filter"), ActionCategory::Navigation);
}
