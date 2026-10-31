#pragma once

#include <QColorSpace>
#include <QString>
#include <QStringList>

#include "gui/quick/bridges/settingsenums.h"

// UI-relevant settings, grouped by area. They are plain snapshots: the
// application side fills them from Settings
// (gui/quick/adapters/bridgesnapshots.h) and passes them to the controllers.
// Members are value-initialized only so that a default-constructed snapshot
// is well-defined; published values always come from Settings.

// Image viewer: fit, zoom, scrolling, filtering and viewer chrome.
struct ViewerSettings {
  SettingsEnums::FitMode fitMode{};
  bool keepFitMode{};
  bool expandImage{};
  int expandLimit{};
  SettingsEnums::ScalingFilter scalingFilter{};
  double casSharpening{};
  double casContrast{};
  // AI upscaling of the visible area when zoomed in.
  bool useUpscayl{};
  bool smoothZoom{};
  bool smoothScroll{};
  double zoomStep{};
  bool useFixedZoomLevels{};
  QString zoomLevels;
  bool unlockMinZoom{};
  SettingsEnums::FocusPoint focusPointIn1to1Mode{};
  SettingsEnums::ImageScrolling imageScrolling{};
  double mouseScrollingSpeed{};
  bool trackpadDetection{};
  bool transparencyGrid{};
  double backgroundOpacity{};
  bool cursorAutohide{};
  bool clickableEdges{};
  bool clickableEdgesVisible{};

  friend bool operator==(const ViewerSettings &,
                         const ViewerSettings &) = default;
};

// Thumbnail strip / main panel.
struct PanelSettings {
  bool enabled{};
  SettingsEnums::PanelPosition position{};
  SettingsEnums::PanelStyle style{};
  bool fullscreenOnly{};
  bool pinned{};
  int hideDelayMs{};
  int previewsSize{};
  bool centerSelection{};
  // Drop thumbnails that scrolled far out of view (unloadThumbs).
  bool unloadThumbnails{};
  // Size of the cached thumbnails (thumbnailResolution).
  int thumbnailResolution{};

  friend bool operator==(const PanelSettings &,
                         const PanelSettings &) = default;
};

// Folder view: thumbnail grid, sorting and the places panel.
struct FolderViewSettings {
  int iconSize{};
  SettingsEnums::SortingMode sortingMode{};
  // Sorting of the folder icons (folderIconSortingMode).
  SettingsEnums::SortingMode folderSortingMode{};
  // Extensions the grid shows; empty for all formats.
  QStringList formatFilter;
  QStringList bookmarks;
  bool placesPanel{};
  int placesPanelWidth{};
  bool bookmarksExpanded{};
  bool treeExpanded{};

  friend bool operator==(const FolderViewSettings &,
                         const FolderViewSettings &) = default;
};

// Overlays drawn over the viewer: info bar, zoom indicator, save, animation
// playback and crop.
struct OverlaySettings {
  bool infoBarFullscreen{};
  SettingsEnums::ZoomIndicatorMode zoomIndicatorMode{};
  bool showSaveOverlay{};
  bool showAnimationControls = true;
  SettingsEnums::CropAction defaultCropAction{};

  friend bool operator==(const OverlaySettings &,
                         const OverlaySettings &) = default;
};

// Colour pipeline of the image renderer: HDR tone mapping and display colour
// management. C++ only (ImageViewportController); not exposed to QML.
struct DisplayColorSettings {
  // Settings::hdrTargetWhiteLevel() default (BT.2408 reference white).
  static constexpr int kDefaultHdrWhiteLevel = 203;

  bool toneMapping = true;
  // A ::ToneMapOperator value (Settings::hdrToneMappingOperator()).
  int toneMapOperator = 0;
  // Luminance in nits that is shown as SDR white.
  int hdrWhiteLevel = kDefaultHdrWhiteLevel;
  bool colorManagement = false;
  // ColorManager::getTargetColorSpace(); invalid while colour management
  // is off.
  QColorSpace target;

  friend bool operator==(const DisplayColorSettings &,
                         const DisplayColorSettings &) = default;
};

// Complete set, applied to the controllers in one step.
struct UiSettingsSnapshot {
  ViewerSettings viewer;
  PanelSettings panel;
  FolderViewSettings folderView;
  OverlaySettings overlays;
  DisplayColorSettings displayColor;

  friend bool operator==(const UiSettingsSnapshot &,
                         const UiSettingsSnapshot &) = default;
};
