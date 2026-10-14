# Qt Quick UI Typography: Analysis and Plan

Status: analysis only, nothing implemented beyond the text render type (see
"Already done"). Written 2026-10-14 for a follow-up session. Rulebook:
`AGENTS.md` (Analysis & Plan first, no magic numbers, atomic commits, CRLF).

## Problem

After the Qt Widgets -> Qt Quick migration the UI text looked smaller and
blurrier than before. That is resolved by the text render type change (see
"Already done"); text is crisp again and nothing in this plan is about
rendering quality.

What remains: font sizes and weights vary across the UI without a system.
Some text uses theme roles, some uses hard-coded pixel sizes copied from the
old stylesheets, some adjusts sizes ad hoc, and emphasis switches between
`bold` and `DemiBold`.

## Already done: native text rendering

Cause of the "smaller, blurrier" look: Qt Quick's default
`QQuickWindow::QtTextRendering` draws distance-field glyphs with no hinting and
grayscale antialiasing only. Segoe UI at 9 pt / 12 px depends on hinting, so
it looked thinner and smaller than in the widget UI even at the same size.

Fix (`QuickUiHost::applyTextRenderType()` in
`src/gui/quick/quickuihost.cpp`, called in `start()` right after
`applyGraphicsApi()`, before QML loads): `NativeTextRendering` globally, so
DirectWrite rasterizes glyphs with hinting and ClearType into the glyph cache
and the GPU only composites them. A user-set
`QT_QUICK_DEFAULT_TEXT_RENDER_TYPE` is kept (set it to `QtRendering` to compare
with the old look). `IconGlyph.qml` keeps `Text.QtRendering` explicitly because
its glyphs are rotated and animated (menu and tree chevrons). Verified by the
user on 2026-10-14: the blur is gone and the text looks much better; no
further rendering work is planned.

Proposed commit: `Render Qt Quick text natively through DirectWrite for hinting
and ClearType`.

Open check: in the translucent-window mode (`Main.qml`, the window colour is
transparent and the background is painted by an item) Qt is expected to fall
back to grayscale antialiasing because subpixel blending needs an opaque
target. Not verified yet; check visually and record the result here.

Rule for future QML: any text that is scaled, rotated or animated in size at
run time must set `renderType: Text.QtRendering`; everything else keeps the
native default.

## Current state

### Font source

- The base font is `QGuiApplication::font()` (Segoe UI 9 pt on a default
  Windows install). There is no font setting in `Settings`.
- `buildThemeSnapshot()` (`src/gui/quick/adapters/themesnapshotbuilder.cpp`)
  derives `ThemeFonts` from it through `UiMetrics::typographyFor()`
  (`src/gui/uimetrics.h`), exposed to QML as `Theme.fonts`:

  | Role | Derivation | 9 pt base |
  |---|---|---|
  | `compact` | `max(int(base * 0.9), 8)` pt | 8 pt |
  | `base` | application font | 9 pt |
  | `section` | `base + 1` pt | 10 pt |
  | `large` | `int(base * 1.8)` pt | 16 pt |

- `UiMetrics::controlMetricsFor()` derives control heights (button, top panel,
  overlay header, context menu) from the base font's line height, so any change
  to the base size resizes controls. Unit test: `tests/unit/tst_uimetrics.cpp`.
- Controls of the `wynpix.style` style (Basic-based) take their font from the
  Qt Quick theme, which is the application font; the style delegates bind
  `font: control.font`. Plain `Text` items without a font also get the
  application font.

### Inventory of deviations (paths under `src/gui/quick/`)

Re-checked against `dff790b3` on 2026-10-14; line numbers below are current.

Hard-coded sizes:

- `ui/BatchItemWidget.qml:29-30` `nameFontSize: 12`, `infoFontSize: 10`
  (pixels, copied from the old `batchconverterdialog.cpp` stylesheets), used at
  lines 86, 93, 109, 121. Pixel sizes ignore the system font and DPI text
  scaling.
- `ui/ImageViewport.qml:35-36,174-177` zoom indicator: `base.pointSize * 2.0`
  with an 18 pt fallback when the base font has no point size.
- `ui/FormatFilterComboBox.qml:35-37` `headerFont` built as
  `base.pointSize + 1`, which is the `section` role computed again.
- `ui/PlacesPanel.qml:259-261` copies `section.family` and `section.pointSize`
  field by field and adds `bold`, instead of binding a role.

Emphasis without a role (mix of `bold` and `DemiBold`):

- `font.bold: true`: `ui/BatchConverterDialog.qml:135`,
  `ui/BatchItemWidget.qml:85,108`, `ui/FileReplaceDialog.qml:61,78`,
  `ui/FloatingMessage.qml:65`, `ui/OverlayPanel.qml:67` (optional
  `boldTitle`), `ui/PlacesPanel.qml:261`, `ui/SettingsGeneralPage.qml:127`,
  `ui/SettingsSection.qml:41`.
- `Font.DemiBold`: `style/ComboBox.qml:34` and `style/SearchField.qml:39`
  (selected popup entry), `ui/SettingsControlsPage.qml:90,98` (table headers).
- Italic: `ui/BatchConverterDialog.qml:543` (pattern help).

Icon-font pixel sizes are not text and stay as they are:
`style/IconGlyph.qml:27` and the zone arrows in `ui/ImageViewport.qml:133-134`
(`Theme.iconFontFamily`).

Roles already used correctly: `ui/SettingsPage.qml:33` (`large`),
`ui/SettingsNote.qml:15` (`compact`), `base` in `ThumbnailWidget.qml`,
`FolderView.qml`, `ImageInfoOverlay.qml`, `FormatFilterComboBox.qml`,
`style/ProgressBar.qml`.

Implicit text: about 120 `Text`/`Label` items across `ui/*.qml` set no font
at all. Most are body text and are fine as is, but headers, captions, hints
and secondary labels among them must be classified and given a role (largest
counts: `BatchConverterDialog.qml` 13, `FileReplaceDialog.qml`,
`PrintDialog.qml`, `ResizeDialog.qml` 6 each, the `Settings*Page.qml` files
1-5 each).

## Design reference: a Windows 11 application

Set by the user on 2026-10-14: the UI should look and read like a native
Windows 11 application. The reference is the Windows 11 / Fluent type ramp and
its usage rules. Values verified on 2026-10-14 against Microsoft Learn,
"Typography in Windows" (learn.microsoft.com/windows/apps/design/
signature-experiences/typography) and "XAML theme resources", section "The
XAML type ramp":

| Fluent style | Size / line height (epx) | Segoe UI Variable instance |
|---|---|---|
| Caption | 12 / 16 | Small, Regular |
| Body | 14 / 20 | Text, Regular |
| Body Strong | 14 / 20 | Text, Semibold |
| Body Large | 18 / 24 | Text, Regular |
| Body Large Strong | 18 / 24 | Text, Semibold |
| Subtitle | 20 / 28 | Display, Semibold |
| Title | 28 / 36 | Display, Semibold |
| Title Large | 40 / 52 | Display, Semibold |
| Display | 68 / 92 | Display, Semibold |

Usage rules from the same pages: Regular for most text, Semibold for titles
and emphasis; Bold and Italic are not part of the ramp; minimum sizes are
12 px Regular and 14 px Semibold (smaller text is illegible in some
languages); sentence case for all UI text, including titles; left alignment by
default.

Today's text breaks the minimums: `compact` (8 pt, about 10.7 px) and
`BatchItemWidget`'s `infoFontSize: 10` px are below 12 px, and bold body text
at 9 pt (12 px) is below the 14 px Semibold minimum.

Consequences for this plan:

- Typeface: Segoe UI Variable, the Windows 11 system UI font (installed with
  Windows 11, nothing to bundle). The supported range starts at Windows 10
  (`AGENTS.md`), which lacks it, so the theme falls back to Segoe UI there.
  Segoe UI Variable has weight and optical-size axes. Qt 6.7+ selects them
  with `QFont::setVariableAxis("opsz", ...)` or a named instance through
  `QFont::setStyleName()`; per the Qt 6.12 docs, variable axes work with the
  default DirectWrite backend but not with the optional GDI backend. Still to
  check on the machine: which family names Qt's font database lists for it
  (the variable family or the "Segoe UI Variable Text/Small/Display" legacy
  families) and whether the axis also reaches Qt Quick text, which builds its
  font from the role's `QFont`.
- Emphasis is Semibold (`Font.DemiBold`), never Bold. No italic either:
  `BatchConverterDialog.qml:543` (pattern help) loses it in stage 3.
- Sizes are effective pixels (Qt device-independent pixels), not points.
  `QGuiApplication::font()` stays the input only to pick up the Windows
  "Text size" accessibility setting: the ramp scales by the ratio of the
  system message font to its default 9 pt, so a user who enlarged text still
  gets larger text. Verify how Qt reports that setting before implementing.
- Sentence case for titles and labels, as Windows 11 does (check existing
  strings while classifying text in stage 4; do not change translations
  wholesale in a typography commit).

## Goals and non-goals

Goals:

- The Windows 11 / Fluent type ramp as the one named ramp, owned by
  `UiMetrics::Typography` and exposed as `Theme.fonts`; QML refers to roles
  only.
- No font sizes, scale factors or weights as literals in QML.
- One emphasis weight across the UI.
- Every header, caption and hint uses the role that matches its meaning.

Non-goals:

- No bundled fonts and no user font setting; only the system UI fonts named
  above.
- No changes to Core; no layout or colour redesign beyond what a role change
  forces.

## Decisions

Settled: the target is the Fluent ramp (see "Design reference"), with
Semibold emphasis.

Settled by the user on 2026-10-14: roles first at today's sizes (stages 1-4:
body = application font, about 12 px), then the switch to Fluent sizes in one
separate commit (stage 5). Each step is small and the role cleanup can be
reviewed without every screen changing size at the same time. The roles carry
their Fluent weights from stage 1 on.

The Fluent body is 14 px against today's ~12 px, and
`controlMetricsFor()` derives control heights from the body line height, so
every control grows; the reference sizes in `uimetrics.h`
(`kReferenceTextHeightPx`, context menu and rename overlay sizes) must be
re-tuned against Windows 11 control sizes in the same commit.

## Proposed design

Owner: `UiMetrics` (pure derivation, unit-tested) -> `buildThemeSnapshot()`
(adapter) -> `ThemeFonts` / `Theme.fonts` (bridge) -> QML. No new component is
needed; this extends the existing theme path and adds nothing to Core.

Roles, named after the Fluent styles they become in stage 5:

| Role | Fluent style | Use | Replaces |
|---|---|---|---|
| `caption` | Caption | notes, hints, secondary info lines | `compact`, `infoFontSize` |
| `body` | Body | default text and controls | `base` |
| `bodyStrong` | Body Strong | inline emphasis, table headers, item names | `bold` / `DemiBold` on body text, `nameFontSize` |
| `subtitle` | Subtitle | section and group titles | `section` (+ bold), `headerFont` |
| `title` | Title | page titles | `large` |
| `display` | Title Large | zoom indicator | `indicatorFontScale`, `indicatorFallbackPointSize` |

Notes:

- Compute sizes with `pointSizeF()` and `setPointSizeF()`; the current integer
  truncation (`int(9 * 0.9)`) loses steps. Handle a base font that only has a
  pixel size (`pointSizeF() <= 0`) in `UiMetrics`, not in QML (removes the
  18 pt fallback in `ImageViewport.qml`).
- The zoom indicator is 18 pt (24 px) today. Title Large (40 px) is 1.7 times
  that, while Title (28 px) is the closest Fluent step. Decide in stage 5 from
  the visual pass; if Title wins, the indicator binds `title` and the
  `display` role is dropped.
- Fluent line heights (16/20/28/36/52 epx) are not applied in this plan; Qt's
  natural line spacing for Segoe UI is close to them. Revisit only if the
  stage 5 visual pass shows multi-line text that is visibly off.
- Scale factors and weights become named `constexpr` values in `uimetrics.h`.
- Renaming roles touches every `Theme.fonts.*` user; keep the old names only if
  the rename is split into its own commit.
- The selected-entry `DemiBold` in `ComboBox.qml` / `SearchField.qml` is a
  state, not a role; bind it to the emphasis weight constant exposed by the
  theme instead of a literal.

## Stages (one commit each)

1. Ramp in `UiMetrics::Typography` and `ThemeFonts`, `pointSizeF`-based, with
   the pixel-size fallback and named constants; extend `tst_uimetrics.cpp`.
   QML unchanged except renamed role bindings.
   Implemented 2026-10-14: `UiMetrics::typographyFor()` returns the six role
   fonts (`caption`, `body`, `bodyStrong`, `subtitle`, `title`, `display`),
   body is the application font unchanged, the other sizes are fractional
   points from `bodyPointSizeFor()` (a pixel-sized font converts at 72 pt per
   96 logical DPI; `QFontInfo::pointSizeF()` returned no size for it in the
   test run, so it is not used; 9 pt if the font has no size at all). Visible
   effects: caption 8.1 pt instead of 8 pt, and the settings page title
   (`title`, 16.2 pt) is Semibold instead of Regular. `subtitle` and
   `display` have no QML user until stage 2.
2. Replace hard-coded sizes and hand-built fonts: `BatchItemWidget.qml`,
   `ImageViewport.qml`, `FormatFilterComboBox.qml`, `PlacesPanel.qml`.
   Implemented 2026-10-14: batch item name and state bind `bodyStrong`, its
   info and details lines `caption`; the zoom indicator binds `display`; the
   format filter's category headers and the places panel headers bind
   `subtitle`. Visible effects: bold becomes Semibold in all of them, the
   zoom indicator turns Semibold at the same 18 pt, and the batch item text
   now follows the Windows text size and DPI (12 px name about the same,
   10 px info lines grow to 8.1 pt, about 10.8 px).
3. Unify emphasis: replace `font.bold` / `Font.DemiBold` literals with
   `bodyStrong` / `subtitle` or the theme's emphasis weight.
   Implemented 2026-10-14. `bodyStrong`: batch selection count, file replace
   "Source:" / "Destination:", floating message, settings section titles and
   the "Thumbnail panel" check box that heads its section (Windows 11
   Settings heads its groups with Body Strong, so no size change), the
   controls table headers, and the overlay panel title (`boldTitle` renamed
   `strongTitle`, used by the CAS and colour adjustment overlays). The
   pattern help in the batch converter loses its italic and binds `caption`
   as a hint. The selected entry in `ComboBox` / `SearchField` switches
   between `Theme.fonts.bodyStrong.weight` and `Theme.fonts.body.weight`, so
   no separate weight property was needed. Afterwards the only font literals
   left in QML are the icon-font pixel sizes listed in the inventory.
4. Classify the implicit `Text`/`Label` items screen by screen (dialogs,
   settings pages, overlays, folder view) and assign roles where the text is
   not plain body text.
   Implemented 2026-10-14. 83 `Text`/`Label` items in `ui/` and `style/` set
   no font after stage 3. Four got a role: the crop panel group titles
   (`bodyStrong`, like the settings section titles), the print dialog's
   "Preview" label under the preview (`caption`), the application and Qt
   versions in the settings sidebar (`caption`), and the script editor's
   name validation hint (`caption`). The rest is body text and stays
   implicit: form labels ("Width:", "Model:", ...), values, list rows, dialog
   messages, warnings and errors, the fullscreen info line, and overlay
   names. Left for the stage 5 visual pass: the shortcut column of
   `style/MenuItem.qml` and `ui/CopyOverlay.qml` stays body; check whether
   Windows 11 menus draw keyboard accelerators in Caption before changing
   it.
5. Switch the ramp to the Fluent sizes and Segoe UI Variable (Segoe UI on
   Windows 10), re-tune the `controlMetricsFor()` reference sizes, and do the
   visual pass over all screens against Windows 11's own apps (Settings,
   File Explorer, Photos).
   Implemented 2026-10-14 (the visual pass is the user's, on Windows 10):
   - `UiMetrics::typographyFor(systemFont)` builds each role from a
     `TextStyle` (Segoe UI Variable optical-size family, Fluent size in
     effective pixels, weight): caption 12 px Small Regular, body 14 px Text
     Regular, bodyStrong 14 px Text Semibold, title 28 px Display Semibold.
     The family list is `{"Segoe UI Variable <size>", <system family>}`, so
     Windows 10 gets Segoe UI (its Semibold face `seguisb.ttf` exists). The
     legacy family names "Segoe UI Variable Small/Text/Display" select the
     optical size without variable-axis API; not verified on Windows 11 yet.
   - Sizes scale by the system font against its default 9 pt (Windows text
     size setting), never below the Fluent sizes (`kMinTextScale`).
   - Roles reduced to four. At Fluent sizes `subtitle` (20 px) is too large
     for the only headers that used it (places panel, format filter
     categories); they bind `bodyStrong` like the settings section titles,
     as Windows 11 Settings does. `display` is dropped: the zoom indicator
     binds `title` (28 px, closest to its former 24 px).
   - The theme snapshot derives from `QFontDatabase::systemFont(GeneralFont)`
     instead of the application font; `QuickUiHost::applyApplicationFont()`
     sets the application font to body before QML loads (Basic-style
     controls without a `qtquickcontrols2.conf` font, `Text` items and dialog
     windows all start from the application font), and the thumbnail and
     folder grid label fonts come from the theme's body.
   - Control metrics derive from the body style. No reference size had to be
     re-tuned: buttons and fields size from their content (about 31-33 px
     with the 19 px body line, Windows 11 uses 32 px), and the body line
     stays under `kReferenceTextHeightPx`, so the context menu keeps 212 x
     32 px.
   Revised the same day after the user's visual check on Windows 10 at
   100 % scaling: the 14 px body read too large and thin (ClearType was on;
   Segoe UI Regular keeps 1 px stems at 14 px, so the larger glyphs look
   lighter, and Segoe UI has no weight between Regular and Semibold). The
   ramp keeps its roles, weights and Segoe UI Variable optical sizes at
   desktop sizes instead: caption 11 px, body and bodyStrong 12 px (the
   system font's 9 pt, as in Windows 10 and the Win32 surfaces of Windows
   11), title 24 px. Control metrics are back to the 16 px line, the sizes
   they had before stage 5. The Fluent minimums (12 px Regular, 14 px
   Semibold) are deliberately not met by caption and bodyStrong.
   Open checks for the visual pass: whether Windows 11
   draws menu accelerators in Caption; the translucent-window ClearType
   check above; a larger Windows text size; Segoe UI Variable on a Windows
   11 machine.

## Verification

- `wynpix_tests` (including `tst_uimetrics`), `wynpix_qml_tests` and
  `all_qmllint` must pass. Building requires the user's explicit request in the
  session unless it falls under the standing QML-migration authorization.
- Visual check, cold start without a file argument (folder view is the default
  start screen), then viewer, context menu, settings pages, batch converter,
  file replace, resize and print dialogs, in both opaque and translucent window
  modes.
- Check at 100 % and 150 % Windows display scaling and with a changed Windows
  text size, since pixel sizes are being removed for exactly that reason.
- A final grep must find no `font.pixelSize`/`font.pointSize` literals,
  `font.bold`, `font.italic` or `Font.DemiBold` outside the theme and the
  icon-font items listed in the inventory.
