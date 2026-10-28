#pragma once

#include <QColor>
#include <QFont>
#include <QObject>
#include <QString>
#include <QtQml/qqmlregistration.h>

// Theme snapshot published by ThemeBridge. The application side fills it from
// Settings::colorScheme() and the application font
// (gui/quick/adapters/themesnapshotbuilder.h).

// The colours of ColorScheme (themestore.h) that QML reads, under camelCase
// names: ColorScheme::folderview_topbar is folderViewTopBar. The exceptions
// are derived from the accent (AccentContrast): accentText, the colour of
// text and glyphs on a solid accent fill, and the fills of the accent hovered
// (accentHover) and pressed (accentPressed) under that text, and the
// translucent accent of selected items (accentTint, accentTintHover,
// accentFrame; SelectionFills).
// The fills and strokes of neutral controls (fillRest, fillHover, fillPressed,
// the HUD ones and stroke*, NeutralFills) are translucent and follow; the
// dialog window and its cards (surfaceWindow, surfaceCard) come last.
struct ThemeColors {
  Q_GADGET
  QML_VALUE_TYPE(themeColors)
  QML_UNCREATABLE("Provided by Theme.colors")
  Q_PROPERTY(QColor background MEMBER background)
  Q_PROPERTY(QColor backgroundFullscreen MEMBER backgroundFullscreen)
  Q_PROPERTY(QColor text MEMBER text)
  Q_PROPERTY(QColor icons MEMBER icons)
  Q_PROPERTY(QColor folderIcons MEMBER folderIcons)
  Q_PROPERTY(QColor widget MEMBER widget)
  Q_PROPERTY(QColor accent MEMBER accent)
  Q_PROPERTY(QColor accentText MEMBER accentText)
  Q_PROPERTY(QColor accentHover MEMBER accentHover)
  Q_PROPERTY(QColor accentPressed MEMBER accentPressed)
  Q_PROPERTY(QColor accentTint MEMBER accentTint)
  Q_PROPERTY(QColor accentTintHover MEMBER accentTintHover)
  Q_PROPERTY(QColor accentFrame MEMBER accentFrame)
  Q_PROPERTY(QColor folderView MEMBER folderView)
  Q_PROPERTY(QColor folderViewTopBar MEMBER folderViewTopBar)
  Q_PROPERTY(QColor thumbPanel MEMBER thumbPanel)
  Q_PROPERTY(QColor scrollbar MEMBER scrollbar)
  Q_PROPERTY(QColor scrollbarHover MEMBER scrollbarHover)
  Q_PROPERTY(QColor overlayText MEMBER overlayText)
  Q_PROPERTY(QColor overlay MEMBER overlay)
  Q_PROPERTY(QColor statusPending MEMBER statusPending)
  Q_PROPERTY(QColor statusError MEMBER statusError)
  Q_PROPERTY(QColor statusProcessing MEMBER statusProcessing)
  Q_PROPERTY(QColor statusSuccess MEMBER statusSuccess)
  Q_PROPERTY(QColor danger MEMBER danger)
  Q_PROPERTY(QColor trash MEMBER trash)
  Q_PROPERTY(QColor textHc2 MEMBER textHc2)
  Q_PROPERTY(QColor textHc MEMBER textHc)
  Q_PROPERTY(QColor textLc MEMBER textLc)
  Q_PROPERTY(QColor folderViewHc MEMBER folderViewHc)
  Q_PROPERTY(QColor folderViewHc2 MEMBER folderViewHc2)
  Q_PROPERTY(QColor thumbPanelHc MEMBER thumbPanelHc)
  Q_PROPERTY(QColor thumbPanelHc2 MEMBER thumbPanelHc2)
  Q_PROPERTY(QColor thumbPanelText MEMBER thumbPanelText)
  Q_PROPERTY(QColor comboField MEMBER comboField)
  Q_PROPERTY(QColor comboFieldHover MEMBER comboFieldHover)
  Q_PROPERTY(QColor comboFieldPressed MEMBER comboFieldPressed)
  Q_PROPERTY(QColor comboFieldBorder MEMBER comboFieldBorder)
  Q_PROPERTY(QColor fillRest MEMBER fillRest)
  Q_PROPERTY(QColor fillHover MEMBER fillHover)
  Q_PROPERTY(QColor fillPressed MEMBER fillPressed)
  Q_PROPERTY(QColor hudHover MEMBER hudHover)
  Q_PROPERTY(QColor hudPressed MEMBER hudPressed)
  Q_PROPERTY(QColor strokeSubtle MEMBER strokeSubtle)
  Q_PROPERTY(QColor strokeDefault MEMBER strokeDefault)
  Q_PROPERTY(QColor strokeStrong MEMBER strokeStrong)
  Q_PROPERTY(QColor surfaceWindow MEMBER surfaceWindow)
  Q_PROPERTY(QColor surfaceCard MEMBER surfaceCard)

public:
  QColor background;
  QColor backgroundFullscreen;
  QColor text;
  QColor icons;
  QColor folderIcons;
  QColor widget;
  QColor accent;
  QColor accentText;
  QColor accentHover;
  QColor accentPressed;
  QColor accentTint;
  QColor accentTintHover;
  QColor accentFrame;
  QColor folderView;
  QColor folderViewTopBar;
  QColor thumbPanel;
  QColor scrollbar;
  QColor scrollbarHover;
  QColor overlayText;
  QColor overlay;
  QColor statusPending;
  QColor statusError;
  QColor statusProcessing;
  QColor statusSuccess;
  QColor danger;
  QColor trash;
  QColor textHc2;
  QColor textHc;
  QColor textLc;
  QColor folderViewHc;
  QColor folderViewHc2;
  QColor thumbPanelHc;
  QColor thumbPanelHc2;
  QColor thumbPanelText;
  QColor comboField;
  QColor comboFieldHover;
  QColor comboFieldPressed;
  QColor comboFieldBorder;
  QColor fillRest;
  QColor fillHover;
  QColor fillPressed;
  QColor hudHover;
  QColor hudPressed;
  QColor strokeSubtle;
  QColor strokeDefault;
  QColor strokeStrong;
  QColor surfaceWindow;
  QColor surfaceCard;

  friend bool operator==(const ThemeColors &, const ThemeColors &) = default;
};

// Text styles of the UI (UiMetrics::Typography), named after the Windows 11
// type ramp styles; body is the application font.
struct ThemeFonts {
  Q_GADGET
  QML_VALUE_TYPE(themeFonts)
  QML_UNCREATABLE("Provided by Theme.fonts")
  Q_PROPERTY(QFont caption MEMBER caption)
  Q_PROPERTY(QFont body MEMBER body)
  Q_PROPERTY(QFont bodyStrong MEMBER bodyStrong)
  Q_PROPERTY(QFont title MEMBER title)

public:
  QFont caption;
  QFont body;
  QFont bodyStrong;
  QFont title;

  friend bool operator==(const ThemeFonts &, const ThemeFonts &) = default;
};

// Control sizes in pixels, derived from the application font
// (UiMetrics::ControlMetrics).
struct ThemeMetrics {
  Q_GADGET
  QML_VALUE_TYPE(themeMetrics)
  QML_UNCREATABLE("Provided by Theme.metrics")
  Q_PROPERTY(int buttonHeight MEMBER buttonHeight)
  Q_PROPERTY(int topPanelHeight MEMBER topPanelHeight)
  Q_PROPERTY(int overlayHeaderSize MEMBER overlayHeaderSize)
  Q_PROPERTY(int contextMenuWidth MEMBER contextMenuWidth)
  Q_PROPERTY(int contextMenuItemHeight MEMBER contextMenuItemHeight)
  Q_PROPERTY(int renameOverlayWidth MEMBER renameOverlayWidth)
  Q_PROPERTY(int tooltipBorderWidth MEMBER tooltipBorderWidth)
  Q_PROPERTY(int tooltipBorderRadius MEMBER tooltipBorderRadius)
  Q_PROPERTY(int contextMenuBorderRadius MEMBER contextMenuBorderRadius)

public:
  int buttonHeight = 0;
  int topPanelHeight = 0;
  int overlayHeaderSize = 0;
  int contextMenuWidth = 0;
  int contextMenuItemHeight = 0;
  int renameOverlayWidth = 0;
  int tooltipBorderWidth = 0;
  int tooltipBorderRadius = 0;
  int contextMenuBorderRadius = 0;

  friend bool operator==(const ThemeMetrics &, const ThemeMetrics &) = default;
};

// Complete set published by ThemeBridge in one step.
struct ThemeSnapshot {
  ThemeColors colors;
  ThemeFonts fonts;
  ThemeMetrics metrics;
  bool dark = false;
  QString iconFontFamily;

  friend bool operator==(const ThemeSnapshot &, const ThemeSnapshot &) = default;
};
