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

ActionGroup Actions::group(const QString &action) const {
  return mEntries.value(action).group;
}

QString Actions::sectionTitle(ActionCategory category, ActionGroup group) {
  const char *source = nullptr;
  switch (group) {
  case ActionGroup::Navigation:
    source = QT_TRANSLATE_NOOP("Actions", "Navigation");
    break;
  case ActionGroup::Selection:
    source = QT_TRANSLATE_NOOP("Actions", "Selection");
    break;
  case ActionGroup::View:
    source = QT_TRANSLATE_NOOP("Actions", "View");
    break;
  case ActionGroup::File:
    source = QT_TRANSLATE_NOOP("Actions", "File");
    break;
  case ActionGroup::Editing:
    source = QT_TRANSLATE_NOOP("Actions", "Editing");
    break;
  case ActionGroup::FilesAndClipboard:
    source = QT_TRANSLATE_NOOP("Actions", "Files and clipboard");
    break;
  case ActionGroup::Upscale:
    source = QT_TRANSLATE_NOOP("Actions", "AI upscale");
    break;
  case ActionGroup::Clipboard:
    source = QT_TRANSLATE_NOOP("Actions", "Clipboard");
    break;
  case ActionGroup::Sorting:
    source = QT_TRANSLATE_NOOP("Actions", "Sorting");
    break;
  case ActionGroup::Application:
    source = QT_TRANSLATE_NOOP("Actions", "Application");
    break;
  }
  // One string, so that a translation can order the two parts.
  return QCoreApplication::translate(kTitleContext, QT_TRANSLATE_NOOP("Actions", "%1 › %2"))
      .arg(categoryTitle(category), QCoreApplication::translate(kTitleContext, source));
}

int Actions::sectionOrder(ActionCategory category, ActionGroup group) {
  return static_cast<int>(category) * kActionGroupCount + static_cast<int>(group);
}

bool Actions::categoriesOverlap(ActionCategory one, ActionCategory other) {
  return one == other || one == ActionCategory::Common || other == ActionCategory::Common;
}

bool Actions::worksIn(ActionCategory category, bool folderView) {
  return categoriesOverlap(category, folderView ? ActionCategory::FolderView
                                                : ActionCategory::Viewer);
}

bool Actions::extendsWithShift(const QString &action) const {
  return mEntries.value(action).extendsWithShift;
}

void Actions::add(const QString &action, const char *title, ActionCategory category,
                  ActionGroup group) {
  mEntries.insert(action, {.title = title, .category = category, .group = group});
}

void Actions::addSelectionMove(const QString &action, const char *title) {
  mEntries.insert(action, {.title = title,
                           .category = ActionCategory::FolderView,
                           .group = ActionGroup::Selection,
                           .extendsWithShift = true});
}

void Actions::init() {
  add(u"nextImage"_s, QT_TRANSLATE_NOOP("Actions", "Next image"), ActionCategory::Viewer, ActionGroup::Navigation);
  add(u"prevImage"_s, QT_TRANSLATE_NOOP("Actions", "Previous image"), ActionCategory::Viewer, ActionGroup::Navigation);
  add(u"toggleFullscreen"_s, QT_TRANSLATE_NOOP("Actions", "Toggle fullscreen"), ActionCategory::Common, ActionGroup::Application);
  add(u"fitWindow"_s, QT_TRANSLATE_NOOP("Actions", "Fit to window"), ActionCategory::Viewer, ActionGroup::View);
  add(u"fitWidth"_s, QT_TRANSLATE_NOOP("Actions", "Fit to width"), ActionCategory::Viewer, ActionGroup::View);
  add(u"fitNormal"_s, QT_TRANSLATE_NOOP("Actions", "Original size"), ActionCategory::Viewer, ActionGroup::View);
  add(u"fitHeight"_s, QT_TRANSLATE_NOOP("Actions", "Fit to height"), ActionCategory::Viewer, ActionGroup::View);
  add(u"toggleFitMode"_s, QT_TRANSLATE_NOOP("Actions", "Switch fit mode"), ActionCategory::Viewer, ActionGroup::View);
  add(u"resize"_s, QT_TRANSLATE_NOOP("Actions", "Resize"), ActionCategory::Common, ActionGroup::Editing);
  add(u"rotateRight"_s, QT_TRANSLATE_NOOP("Actions", "Rotate right"), ActionCategory::Common, ActionGroup::Editing);
  add(u"rotateLeft"_s, QT_TRANSLATE_NOOP("Actions", "Rotate left"), ActionCategory::Common, ActionGroup::Editing);
  add(u"scrollUp"_s, QT_TRANSLATE_NOOP("Actions", "Scroll image up"), ActionCategory::Viewer, ActionGroup::View);
  add(u"scrollDown"_s, QT_TRANSLATE_NOOP("Actions", "Scroll image down"), ActionCategory::Viewer, ActionGroup::View);
  add(u"scrollLeft"_s, QT_TRANSLATE_NOOP("Actions", "Scroll image left"), ActionCategory::Viewer, ActionGroup::View);
  add(u"scrollRight"_s, QT_TRANSLATE_NOOP("Actions", "Scroll image right"), ActionCategory::Viewer, ActionGroup::View);
  add(u"zoomIn"_s, QT_TRANSLATE_NOOP("Actions", "Zoom in"), ActionCategory::Viewer, ActionGroup::View);
  add(u"zoomOut"_s, QT_TRANSLATE_NOOP("Actions", "Zoom out"), ActionCategory::Viewer, ActionGroup::View);
  add(u"zoomInCursor"_s, QT_TRANSLATE_NOOP("Actions", "Zoom in at cursor"), ActionCategory::Viewer, ActionGroup::View);
  add(u"zoomOutCursor"_s, QT_TRANSLATE_NOOP("Actions", "Zoom out at cursor"), ActionCategory::Viewer, ActionGroup::View);
  add(u"save"_s, QT_TRANSLATE_NOOP("Actions", "Save"), ActionCategory::Common, ActionGroup::File);
  add(u"saveAs"_s, QT_TRANSLATE_NOOP("Actions", "Save as"), ActionCategory::Common, ActionGroup::File);
  add(u"setWallpaper"_s, QT_TRANSLATE_NOOP("Actions", "Set as wallpaper"), ActionCategory::Viewer, ActionGroup::FilesAndClipboard);
  add(u"crop"_s, QT_TRANSLATE_NOOP("Actions", "Crop"), ActionCategory::Viewer, ActionGroup::Editing);
  add(u"removeFile"_s, QT_TRANSLATE_NOOP("Actions", "Delete permanently"), ActionCategory::Common, ActionGroup::File);
  add(u"copyFile"_s, QT_TRANSLATE_NOOP("Actions", "Quick copy"), ActionCategory::Viewer, ActionGroup::FilesAndClipboard);
  add(u"moveFile"_s, QT_TRANSLATE_NOOP("Actions", "Quick move"), ActionCategory::Viewer, ActionGroup::FilesAndClipboard);
  add(u"jumpToFirst"_s, QT_TRANSLATE_NOOP("Actions", "First image"), ActionCategory::Viewer, ActionGroup::Navigation);
  add(u"jumpToLast"_s, QT_TRANSLATE_NOOP("Actions", "Last image"), ActionCategory::Viewer, ActionGroup::Navigation);
  add(u"openSettings"_s, QT_TRANSLATE_NOOP("Actions", "Settings"), ActionCategory::Common, ActionGroup::Application);
  add(u"closeFullScreenOrExit"_s, QT_TRANSLATE_NOOP("Actions", "Exit fullscreen or quit"), ActionCategory::Common, ActionGroup::Application);
  add(u"exit"_s, QT_TRANSLATE_NOOP("Actions", "Quit"), ActionCategory::Common, ActionGroup::Application);
  add(u"flipH"_s, QT_TRANSLATE_NOOP("Actions", "Flip horizontal"), ActionCategory::Common, ActionGroup::Editing);
  add(u"flipV"_s, QT_TRANSLATE_NOOP("Actions", "Flip vertical"), ActionCategory::Common, ActionGroup::Editing);
  add(u"folderView"_s, QT_TRANSLATE_NOOP("Actions", "Go to folder view"), ActionCategory::Viewer, ActionGroup::Navigation);
  add(u"documentView"_s, QT_TRANSLATE_NOOP("Actions", "Go to image view"), ActionCategory::FolderView, ActionGroup::Navigation);
  add(u"toggleFolderView"_s, QT_TRANSLATE_NOOP("Actions", "Toggle folder/image view"), ActionCategory::Common, ActionGroup::Application);
  add(u"moveToTrash"_s, QT_TRANSLATE_NOOP("Actions", "Move to trash"), ActionCategory::Common, ActionGroup::File);
  add(u"reloadImage"_s, QT_TRANSLATE_NOOP("Actions", "Reload image"), ActionCategory::Common, ActionGroup::File);
  add(u"copyFileClipboard"_s, QT_TRANSLATE_NOOP("Actions", "Copy image to clipboard"), ActionCategory::Common, ActionGroup::Clipboard);
  add(u"copyPathClipboard"_s, QT_TRANSLATE_NOOP("Actions", "Copy file path"), ActionCategory::Common, ActionGroup::Clipboard);
  add(u"copyViewportClipboard"_s, QT_TRANSLATE_NOOP("Actions", "Copy visible area to clipboard"), ActionCategory::Viewer, ActionGroup::FilesAndClipboard);
  add(u"renameFile"_s, QT_TRANSLATE_NOOP("Actions", "Rename"), ActionCategory::Common, ActionGroup::File);
  add(u"contextMenu"_s, QT_TRANSLATE_NOOP("Actions", "Context menu"), ActionCategory::Common, ActionGroup::Application);
  add(u"toggleTransparencyGrid"_s, QT_TRANSLATE_NOOP("Actions", "Toggle transparency grid"), ActionCategory::Viewer, ActionGroup::View);
  add(u"sortByName"_s, QT_TRANSLATE_NOOP("Actions", "Sort by name"), ActionCategory::Common, ActionGroup::Sorting);
  add(u"sortByTime"_s, QT_TRANSLATE_NOOP("Actions", "Sort by date"), ActionCategory::Common, ActionGroup::Sorting);
  add(u"sortBySize"_s, QT_TRANSLATE_NOOP("Actions", "Sort by size"), ActionCategory::Common, ActionGroup::Sorting);
  add(u"toggleImageInfo"_s, QT_TRANSLATE_NOOP("Actions", "Toggle image info"), ActionCategory::Viewer, ActionGroup::View);
  add(u"toggleShuffle"_s, QT_TRANSLATE_NOOP("Actions", "Toggle shuffle"), ActionCategory::Viewer, ActionGroup::Navigation);
  add(u"toggleScalingFilter"_s, QT_TRANSLATE_NOOP("Actions", "Toggle scaling filter"), ActionCategory::Viewer, ActionGroup::View);
  add(u"cycleScalingFilter"_s, QT_TRANSLATE_NOOP("Actions", "Next scaling filter"), ActionCategory::Viewer, ActionGroup::View);
  add(u"toggleSlideshow"_s, QT_TRANSLATE_NOOP("Actions", "Toggle slideshow"), ActionCategory::Viewer, ActionGroup::Navigation);
  add(u"showInDirectory"_s, QT_TRANSLATE_NOOP("Actions", "Show in folder"), ActionCategory::Common, ActionGroup::File);
  add(u"goUp"_s, QT_TRANSLATE_NOOP("Actions", "Parent folder"), ActionCategory::FolderView, ActionGroup::Navigation);
  add(u"discardEdits"_s, QT_TRANSLATE_NOOP("Actions", "Discard edits"), ActionCategory::Common, ActionGroup::Editing);
  add(u"nextDirectory"_s, QT_TRANSLATE_NOOP("Actions", "Next folder"), ActionCategory::Viewer, ActionGroup::Navigation);
  add(u"prevDirectory"_s, QT_TRANSLATE_NOOP("Actions", "Previous folder"), ActionCategory::Viewer, ActionGroup::Navigation);
  add(u"lockZoom"_s, QT_TRANSLATE_NOOP("Actions", "Lock zoom"), ActionCategory::Viewer, ActionGroup::View);
  add(u"lockView"_s, QT_TRANSLATE_NOOP("Actions", "Lock view"), ActionCategory::Viewer, ActionGroup::View);
  add(u"print"_s, QT_TRANSLATE_NOOP("Actions", "Print"), ActionCategory::Common, ActionGroup::File);
  add(u"toggleFullscreenInfoBar"_s, QT_TRANSLATE_NOOP("Actions", "Toggle fullscreen info bar"), ActionCategory::Viewer, ActionGroup::View);
  add(u"pasteFile"_s, QT_TRANSLATE_NOOP("Actions", "Open from clipboard"), ActionCategory::Common, ActionGroup::Clipboard);
  add(u"togglePanorama"_s, QT_TRANSLATE_NOOP("Actions", "Panorama mode"), ActionCategory::Viewer, ActionGroup::View);
  add(u"colorAdjustments"_s, QT_TRANSLATE_NOOP("Actions", "Color adjustments"), ActionCategory::Viewer, ActionGroup::Editing);
  add(u"casSettings"_s, QT_TRANSLATE_NOOP("Actions", "CAS settings"), ActionCategory::Viewer, ActionGroup::View);
  add(u"createDirectory"_s, QT_TRANSLATE_NOOP("Actions", "New folder"), ActionCategory::Common, ActionGroup::File);
  add(u"nextPage"_s, QT_TRANSLATE_NOOP("Actions", "Next page"), ActionCategory::Viewer, ActionGroup::Navigation);
  add(u"prevPage"_s, QT_TRANSLATE_NOOP("Actions", "Previous page"), ActionCategory::Viewer, ActionGroup::Navigation);
  add(u"firstPage"_s, QT_TRANSLATE_NOOP("Actions", "First page"), ActionCategory::Viewer, ActionGroup::Navigation);
  add(u"lastPage"_s, QT_TRANSLATE_NOOP("Actions", "Last page"), ActionCategory::Viewer, ActionGroup::Navigation);
  add(u"toggleUpscayl"_s, QT_TRANSLATE_NOOP("Actions", "Toggle AI upscale"), ActionCategory::Viewer, ActionGroup::Upscale);
  add(u"cycleUpscaylModel"_s, QT_TRANSLATE_NOOP("Actions", "Next AI upscale model"), ActionCategory::Viewer, ActionGroup::Upscale);
  add(u"toggleHdrToneMapping"_s, QT_TRANSLATE_NOOP("Actions", "Toggle HDR tone mapping"), ActionCategory::Viewer, ActionGroup::View);
  add(u"focusNameFilter"_s, QT_TRANSLATE_NOOP("Actions", "Name filter"), ActionCategory::FolderView, ActionGroup::Navigation);
  add(u"folderOpenSelected"_s, QT_TRANSLATE_NOOP("Actions", "Open selected"), ActionCategory::FolderView, ActionGroup::Selection);
  add(u"folderSelectAll"_s, QT_TRANSLATE_NOOP("Actions", "Select all"), ActionCategory::FolderView, ActionGroup::Selection);
  add(u"folderBack"_s, QT_TRANSLATE_NOOP("Actions", "Back"), ActionCategory::FolderView, ActionGroup::Navigation);
  add(u"folderForward"_s, QT_TRANSLATE_NOOP("Actions", "Forward"), ActionCategory::FolderView, ActionGroup::Navigation);
  addSelectionMove(u"folderMoveLeft"_s, QT_TRANSLATE_NOOP("Actions", "Move selection left"));
  addSelectionMove(u"folderMoveRight"_s, QT_TRANSLATE_NOOP("Actions", "Move selection right"));
  addSelectionMove(u"folderMoveUp"_s, QT_TRANSLATE_NOOP("Actions", "Move selection up"));
  addSelectionMove(u"folderMoveDown"_s, QT_TRANSLATE_NOOP("Actions", "Move selection down"));
  addSelectionMove(u"folderPageUp"_s, QT_TRANSLATE_NOOP("Actions", "Move selection a page up"));
  addSelectionMove(u"folderPageDown"_s, QT_TRANSLATE_NOOP("Actions", "Move selection a page down"));
  addSelectionMove(u"folderFirstItem"_s, QT_TRANSLATE_NOOP("Actions", "First item"));
  addSelectionMove(u"folderLastItem"_s, QT_TRANSLATE_NOOP("Actions", "Last item"));
}
