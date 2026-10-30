#pragma once

#include <QColor>
#include <QFont>
#include <QObject>
#include <QString>
#include <QtQml/qqmlregistration.h>

// Theme snapshot published by ThemeBridge. The application side fills it from
// Settings::colorScheme() and the application font
// (gui/quick/adapters/themesnapshotbuilder.h).

// Colours of the UI by role (SemanticTokens::Colors): surfaces, strokes,
// fills, the accent and its states, text, icons and states.
struct ThemeTokens {
  Q_GADGET
  QML_VALUE_TYPE(themeTokens)
  QML_UNCREATABLE("Provided by Theme.tokens")
  Q_PROPERTY(QColor surfaceCanvas MEMBER surfaceCanvas)
  Q_PROPERTY(QColor surfaceCanvasFullscreen MEMBER surfaceCanvasFullscreen)
  Q_PROPERTY(QColor surfaceChrome MEMBER surfaceChrome)
  Q_PROPERTY(QColor surfaceChromeRaised MEMBER surfaceChromeRaised)
  Q_PROPERTY(QColor surfaceView MEMBER surfaceView)
  Q_PROPERTY(QColor surfaceViewRaised MEMBER surfaceViewRaised)
  Q_PROPERTY(QColor surfaceViewRaisedStrong MEMBER surfaceViewRaisedStrong)
  Q_PROPERTY(QColor surfaceBar MEMBER surfaceBar)
  Q_PROPERTY(QColor surfacePopup MEMBER surfacePopup)
  Q_PROPERTY(QColor surfaceWindow MEMBER surfaceWindow)
  Q_PROPERTY(QColor surfaceCard MEMBER surfaceCard)
  Q_PROPERTY(QColor surfaceHud MEMBER surfaceHud)
  Q_PROPERTY(QColor strokeSubtle MEMBER strokeSubtle)
  Q_PROPERTY(QColor strokeDefault MEMBER strokeDefault)
  Q_PROPERTY(QColor strokeStrong MEMBER strokeStrong)
  Q_PROPERTY(QColor strokeField MEMBER strokeField)
  Q_PROPERTY(QColor fillRest MEMBER fillRest)
  Q_PROPERTY(QColor fillHover MEMBER fillHover)
  Q_PROPERTY(QColor fillPressed MEMBER fillPressed)
  Q_PROPERTY(QColor fieldRest MEMBER fieldRest)
  Q_PROPERTY(QColor fieldHover MEMBER fieldHover)
  Q_PROPERTY(QColor fieldPressed MEMBER fieldPressed)
  Q_PROPERTY(QColor hudHover MEMBER hudHover)
  Q_PROPERTY(QColor hudPressed MEMBER hudPressed)
  Q_PROPERTY(QColor rowHover MEMBER rowHover)
  Q_PROPERTY(QColor rowSelected MEMBER rowSelected)
  Q_PROPERTY(QColor rowSelectedHover MEMBER rowSelectedHover)
  Q_PROPERTY(QColor rowPressed MEMBER rowPressed)
  Q_PROPERTY(QColor accent MEMBER accent)
  Q_PROPERTY(QColor accentHover MEMBER accentHover)
  Q_PROPERTY(QColor accentPressed MEMBER accentPressed)
  Q_PROPERTY(QColor accentText MEMBER accentText)
  Q_PROPERTY(QColor accentTint MEMBER accentTint)
  Q_PROPERTY(QColor accentTintHover MEMBER accentTintHover)
  Q_PROPERTY(QColor accentFrame MEMBER accentFrame)
  Q_PROPERTY(QColor textPrimary MEMBER textPrimary)
  Q_PROPERTY(QColor textSecondary MEMBER textSecondary)
  Q_PROPERTY(QColor textTertiary MEMBER textTertiary)
  Q_PROPERTY(QColor textOnView MEMBER textOnView)
  Q_PROPERTY(QColor textOnChrome MEMBER textOnChrome)
  Q_PROPERTY(QColor textOnHud MEMBER textOnHud)
  Q_PROPERTY(QColor iconDefault MEMBER iconDefault)
  Q_PROPERTY(QColor iconFolder MEMBER iconFolder)
  Q_PROPERTY(QColor iconOnChrome MEMBER iconOnChrome)
  Q_PROPERTY(QColor contrastOnChrome MEMBER contrastOnChrome)
  Q_PROPERTY(QColor statusPending MEMBER statusPending)
  Q_PROPERTY(QColor statusError MEMBER statusError)
  Q_PROPERTY(QColor statusProcessing MEMBER statusProcessing)
  Q_PROPERTY(QColor statusSuccess MEMBER statusSuccess)
  Q_PROPERTY(QColor danger MEMBER danger)
  Q_PROPERTY(QColor trash MEMBER trash)
  Q_PROPERTY(QColor dropTarget MEMBER dropTarget)
  Q_PROPERTY(QColor scrollbar MEMBER scrollbar)
  Q_PROPERTY(QColor scrollbarHover MEMBER scrollbarHover)
  Q_PROPERTY(QColor scrollbarMarker MEMBER scrollbarMarker)

public:
  QColor surfaceCanvas;
  QColor surfaceCanvasFullscreen;
  QColor surfaceChrome;
  QColor surfaceChromeRaised;
  QColor surfaceView;
  QColor surfaceViewRaised;
  QColor surfaceViewRaisedStrong;
  QColor surfaceBar;
  QColor surfacePopup;
  QColor surfaceWindow;
  QColor surfaceCard;
  QColor surfaceHud;
  QColor strokeSubtle;
  QColor strokeDefault;
  QColor strokeStrong;
  QColor strokeField;
  QColor fillRest;
  QColor fillHover;
  QColor fillPressed;
  QColor fieldRest;
  QColor fieldHover;
  QColor fieldPressed;
  QColor hudHover;
  QColor hudPressed;
  QColor rowHover;
  QColor rowSelected;
  QColor rowSelectedHover;
  QColor rowPressed;
  QColor accent;
  QColor accentHover;
  QColor accentPressed;
  QColor accentText;
  QColor accentTint;
  QColor accentTintHover;
  QColor accentFrame;
  QColor textPrimary;
  QColor textSecondary;
  QColor textTertiary;
  QColor textOnView;
  QColor textOnChrome;
  QColor textOnHud;
  QColor iconDefault;
  QColor iconFolder;
  QColor iconOnChrome;
  QColor contrastOnChrome;
  QColor statusPending;
  QColor statusError;
  QColor statusProcessing;
  QColor statusSuccess;
  QColor danger;
  QColor trash;
  QColor dropTarget;
  QColor scrollbar;
  QColor scrollbarHover;
  QColor scrollbarMarker;

  friend bool operator==(const ThemeTokens &, const ThemeTokens &) = default;
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
  ThemeTokens tokens;
  ThemeFonts fonts;
  ThemeMetrics metrics;
  QString iconFontFamily;

  friend bool operator==(const ThemeSnapshot &, const ThemeSnapshot &) = default;
};
