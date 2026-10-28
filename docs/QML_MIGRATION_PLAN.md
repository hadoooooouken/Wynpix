# wynpix: Qt Widgets to Qt Quick (QML) Migration Plan

Target: Qt 6.12 LTS, MSVC, Windows 10/11 x64. Branch: `qml`.

This document defines the approved, sequential migration stages. Each stage
is sized to be handed out as one task, keeps the application buildable and
runnable, and lists its acceptance criteria. Stage IDs (`S0.1`, `S1.3`, ...)
are the units referred to by the "Standing QML migration authorization" in
`AGENTS.md`.

---

## 1. Current Architecture (as of 3.6.4)

### 1.1 Layers

| Layer | Content | Size |
|---|---|---|
| Entry | `main.cpp`: `QApplication`, Fusion-based `ProxyStyle`, single-instance `QLocalServer`, global singletons (`settings`, `actionManager`, `scriptManager`, `inputMap`, `appActions`, `shrRes`) | ~260 lines |
| Mediator | `Core` (`core.h/.cpp`): navigation, slideshow, editing, file operations, history, cold start, AI resize | ~3000 lines |
| Components | `components/`: `DirectoryModel`, `DirectoryManager` + Windows watcher, `DirectoryPresenter` (MVP presenter over `IDirectoryView`), `Loader`, `Scaler`, `Thumbnailer`, caches, `Upscaler` (ncnn/Vulkan), `FileOpController`, `BatchConverter`, `WallpaperController`, `MimePayloadManager`, `ScriptManager`, `ActionManager` | UI-free, reusable |
| Sources | `sourcecontainers/`: `Image`, `ImageStatic`, `ImageAnimated`, `Thumbnail`, `DocumentInfo` (exiv2) | UI-free |
| Utils | `utils/`: `ImageLib`, `HdrToneMapper` (CPU), `ColorManager` (ICC via `GetICMProfileW` + `QColorSpace`), readers (DjVu, Blend), `PngWriter` | UI-free |
| GUI | `gui/`: `MW` (main window), `CentralWidget` (`QStackedWidget`), `DocumentWidget`, `ViewerWidget`, `ImageViewerV2` (`QGraphicsView`), `ThumbnailView` (`QGraphicsView` widget pool), folder view, 19 overlays, panels, 7 dialogs, 22 custom widgets | ~22k lines |

### 1.2 Rendering path today

- `ImageViewerV2` is a `QGraphicsView` whose viewport is a `QOpenGLWidget`
  (`AA_UseDesktopOpenGL`). On Windows this means desktop OpenGL composited
  into a widget window through an extra FBO copy.
- `FilterPixmapItem` / `PanoramaGraphicsItem` issue raw OpenGL calls from
  `QGraphicsItem::paint()`: texture upload with mipmaps, GLSL 1.x shaders
  (`res/shaders/filter.frag`: colour adjustments, CAS, smart sharpen;
  `boxreduce.*`: exact-ratio downsample; `panorama.frag`). Images larger than
  `GL_MAX_TEXTURE_SIZE` fall back to CPU painting.
- High-quality CPU filters (`QI_FILTER_SMART`, `QI_FILTER_MKS2021`) go through
  the `Scaler` component and a round trip back into the view.
- HDR tone mapping (`HdrToneMapper`, ~950 lines) and colour management
  (`QColorSpace::transform`) run on the CPU before upload.
- Everything else (thumbnail grid, strip, overlays, panels, fade animations
  via `QGraphicsOpacityEffect`, menus) is CPU-rasterized by the widget
  painter.

### 1.3 Coupling relevant to the migration

- `Core` holds a raw `MW *mw` and calls ~50 distinct `MW` methods (181 call
  sites): messages (`showMessage/Error/Warning/Success`: ~65), view mode
  queries (`currentViewMode`: 24), dialogs (`showConfirmation`,
  `fileReplaceDialog`, `getSaveFileName`, `showResizeDialog`), viewer
  commands (`showImage`, `showAnimation`, `onScalingFinished`,
  `visibleOriginalImageRect`, `currentScale`, ...). `core.h` includes
  `gui/mainwindow.h` and `gui/dialogs/printdialog.h`.
- `DirectoryPresenter` already talks to views only through `IDirectoryView`
  (MVP). This is the cleanest seam in the codebase and is reused as-is.
- 30 GUI files read the global `settings` directly (411 call sites).
  `Settings` has no `Q_PROPERTY` declarations, only getters/setters and a
  single `settingsChanged()` signal.
- `ActionManager::processEvent(QInputEvent*)` is the single keyboard/mouse
  shortcut dispatcher. It is UI-toolkit-agnostic and is kept.
- The "proxy" classes (`*OverlayProxy`, `FolderViewProxy`,
  `ThumbnailStripProxy`) exist to defer widget construction for startup
  speed. Qt Quick replaces them with `Loader { asynchronous: true }`.

### 1.4 Existing debt the migration must not propagate (flagged)

- Manual `delete` of singletons in `main.cpp::cleanupSingletons()` and raw
  owning pointers (`new ProxyStyle`, `new QLocalServer`, `MW *mw`) violate
  the ownership rule.
- `setColorAdjustments(float x7)` and `applyColorAdjustments(float x7)`
  violate the "group loose parameters into a struct" rule.
- `#ifdef _WIN32` / `#ifdef __GLIBC__` blocks in `main.cpp` / `core.h`.
- `ImageViewerV2` mixes view-transform math (fit modes, zoom levels, anchored
  zoom, edge snapping, view locks) with rendering and input handling.
- `SettingsDialog` (~4300 lines) mixes UI construction with settings
  load/save logic.

---

## 2. Target Architecture

```
main.cpp
 └─ QApplication (kept: QPrintDialog / QPrinter need QtWidgets)
     ├─ AppServices (owns singletons via std::unique_ptr, replaces cleanupSingletons)
     ├─ Core (mediator, unchanged responsibilities; talks to UI ports only)
     ├─ UI ports (pure C++ interfaces, implemented twice during transition):
     │    INotificationPort, IDialogPort, IViewerPort, IShellPort,
     │    IWindowPort, IViewModePort (outbound) + UiEvents (inbound signals)
     ├─ Widgets UI (legacy, until S4.2)        ─┐
     └─ Quick UI (QQmlApplicationEngine)        ─┴─ selected at startup
          ├─ QML module  wynpix.ui       (QML_ELEMENT C++ types + .qml files)
          ├─ QML module  wynpix.render   (ImageRenderItem: QQuickRhiItem, shaders via qt_add_shaders)
          └─ Bridges     (QObject adapters: SettingsBridge, ThemeBridge,
                          ActionBridge, DirectoryViewAdapter : IDirectoryView, ...)
```

Principles:

1. **Core and components do not change ownership.** The migration only adds
   adapters on the UI side and narrow interfaces on the `Core` side. No new
   responsibilities are added to `Core`.
2. **No business logic in QML.** QML files bind properties, lay out items,
   run visual animations and forward user intent to C++ (`invokable` methods
   or signals). Decisions (what to load, what to save, error handling) stay
   in C++ components.
3. **One GPU path.** All pixels on screen go through the Qt Quick scene graph
   on QRhi (Direct3D 11 by default). Image filtering, tone mapping, colour
   management and downsampling move into RHI shaders.
4. **Dual UI during transition.** The Quick UI was opt-in (`--ui=quick`)
   until parity (S4.1). Since S4.1 it is the default, and `--ui=widgets` or
   the hidden `userInterface` setting selects the widget UI until it is
   deleted (S4.2). Every stage leaves both UIs runnable.
5. **Render-thread discipline.** Qt Quick on Windows uses the threaded render
   loop. Renderer objects (`QQuickRhiItemRenderer`, `QSGNode`s) never touch
   `Core`, `Settings` or components. Data crosses only in
   `QQuickRhiItemRenderer::synchronize()` / `updatePaintNode()` while the GUI
   thread is blocked, as immutable value objects (`std::shared_ptr<const
   QImage>`, parameter structs).

---

## 3. Qt 6.12 (and recent 6.x) Feature Selection

### 3.1 Adopt

| Feature | Since | Where it is used |
|---|---|---|
| `QQuickRhiItem` (+ `TextureFormat::RGBA16F/RGBA32F`) | 6.7 | `ImageRenderItem`: portable GPU image renderer on D3D11/D3D12/Vulkan/GL, multi-pass (reduce, sharpen, tone map) |
| `qt_add_shaders()` (`.qsb`, GLSL 440 compiled for HLSL/SPIR-V/GLSL) | 6.0 | Port of `filter.frag`, `boxreduce.*`, `panorama.frag` |
| QRhi `generateMips`, 3D textures, float textures, `TextureSizeMax` query | 6.6 public | Mip chains, colour LUTs, HDR sources, tiling decision |
| `QQuickGraphicsConfiguration::setPipelineCacheSaveFile/LoadFile` | 6.5 | Persist pipeline cache to avoid shader compile cost at cold start |
| `qt_add_qml_module` + qmlcachegen/qmlsc AOT, `QML_ELEMENT`, `QML_SINGLETON` | 6.x | All QML is compiled; no runtime QML parsing of app code |
| `QQmlEngine::setExternalSingletonInstance()`; `QML_UNCREATABLE` combined with `QML_SINGLETON` | **6.12** | Expose existing C++ singletons (settings/theme/actions bridges) to QML without a factory or context properties |
| Missing `required property` is now an error | **6.12** | Use `required property` in every delegate; regressions fail loudly |
| `QRangeModel` (drag-and-drop, `sort()`, `match()`, `headerData()`, flags) | 6.10 / **6.12** | Small list models: bookmarks, format filters, shortcuts table, batch converter queue, scripts list |
| QML `SortFilterProxyModel` (+ `RangeFilter`, `RegExpFilter`, `AllOfFilter`, `AnyOfFilter`) | 6.10 / **6.12** | Filtering in settings (shortcut search), format filter list |
| `TreeView` (QML) over the existing `FileSystemModelCustom` | 6.3+ | Folder tree in folder view |
| `Popup.popupType: Popup.Window` / `Popup.Native` | 6.8 | Context menu and combo popups may exceed window bounds, like today's `QMenu` |
| `MenuItem` shows the shortcut of its `Action` | **6.12** | Context menu shortcuts, synced from `ActionManager` |
| `ToolTip.policy`, `ComboBox.highlightOnHover`, `Menu.separatorsCollapsible` | **6.12** | Panel buttons, sort/filter combos, context menu |
| `DoubleSpinBox`, `DialogButtonBox.defaultButton` | 6.11 | Colour adjustments, resize, CAS settings dialogs |
| `SearchField` (+ text selection APIs) | 6.10 / **6.12** | Name filter in folder view, shortcut search |
| `Flickable` position-to-child / `flickTo` APIs | 6.11 | Scroll-to-selection in grid and strip |
| `RectangularShadow` (per-corner radii), `MultiEffect` | 6.9 / 6.11 | GPU shadows and backdrop blur for overlays and floating panels |
| `Color` QML singleton | **6.12** | Derive hover/pressed/disabled variants in the style instead of hand-written extra colours |
| `QAccessibilityHints::motionPreference` | **6.12** | Disable fade/zoom/scroll animations when the OS requests reduced motion |
| Animated SVG in `QMovie` | **6.12** | Animated SVG documents flow through the existing `ImageAnimated` frame path at no extra cost |
| `VectorImage` `asynchronous` / `status` | **6.12** | Optional GPU (curve renderer) SVG display path, see S2.6 |
| Qt Canvas Painter (`QCanvasPainterItem`), now fully supported | **6.12** | GPU 2D painting for custom-drawn controls that are awkward as `Shape`s (map overlay, slider tracks), see S3.4 |
| `qmllint` new checks, `qmlls`, `<exe>_qmlpreview` target | **6.12** | CI-style lint target and fast QML iteration |
| `lupdate` preserves CRLF in `.ts` files | **6.12** | Translations keep repository line endings |
| `rcc` content deduplication | **6.12** | Automatic; no action needed |

### 3.2 Evaluate (spike only, not on the critical path)

| Feature | Reason |
|---|---|
| Windows screen colour space derived from ICC (`ColorProfileGetDisplayDefault`) in the 6.12 QPA plugin | No public `QScreen` accessor was found in the 6.12 docs; `ColorManager`'s `GetICMProfileW` path stays until a public API exists. |
| Direct3D 12 / Vulkan scene graph backends | Offered as a user setting; D3D11 remains the default for startup time and driver maturity. |

### 3.3 Do not adopt

| Feature | Reason |
|---|---|
| `QtQuick.Controls.Native` style | Cannot be themed; the app has its own colour schemes (`ThemeStore`). Use a custom style built on `Basic`. |
| Qt Labs StyleKit / `QStyleKitStyle` | Labs, technology preview, no compatibility promise. |
| `QtCanvas2D` QML API | Technology preview in 6.12. Use the supported C++ `QCanvasPainterItem` instead. |
| Qt TaskTree, `QRangeModelAdapter` | Technology preview; existing `QThreadPool`/`QRunnable` architecture is kept. |
| `QQuickWidget` / `createWindowContainer` hybrid embedding | Extra offscreen copy per frame and two compositors; contradicts the GPU goal. The Quick UI is a separate top-level window. |
| QML `Image` + `QQuickImageProvider` for the main image | No custom filtering/tone mapping, CPU decode on provider threads, duplicate caching. |
| Qt Quick 3D for the panorama view | Large module and startup cost for a single fragment shader. |
| `QtQuick.Pdf` | PDF pages are already rasterized into the image pipeline by `ImageStatic::loadPdf()`. |
| HDR swapchain for Qt Quick windows | Qt Quick has no public, documented API for an scRGB/HDR10 window swapchain in 6.12, and HDR displays are out of scope. Output stays SDR with GPU tone mapping. |
| `qmltc` (QML type compiler, `qt_target_compile_qml_to_cpp` / `ENABLE_TYPE_COMPILER`) | Technology preview; root objects must be constructed from C++ instead of `QQmlApplicationEngine::loadFromModule()` (`QuickUiHost`), which changes the window lifecycle. It speeds up object creation, which is not a bottleneck for the viewer. Binding compilation already comes from qmlcachegen (3.1); see section 7 for its coverage. |

---

## 4. Stages

Legend for each stage: **Goal**, **Owner** (component that owns the logic),
**Scope**, **Acceptance**. Stages within a phase are sequential unless marked
"parallelizable".

### Phase 0: Preparation (refactoring only, no visible change)

These stages are pure refactoring commits, kept separate from new
functionality as required by `AGENTS.md`.

#### S0.1 Build system and QML module skeleton
- **Goal:** the project builds with Qt Quick available and an empty Quick UI
  can be launched behind a flag.
- **Owner:** build system; `main.cpp` startup selection only.
- **Scope:**
  - Raise `find_package(Qt6 6.12 ...)` and add `Quick`, `Qml`,
    `QuickControls2`, `QuickDialogs2`, `ShaderTools`, `QuickTest`, `Test`.
  - `qt_policy(SET ...)` / `qt_standard_project_setup(REQUIRES 6.12)` where
    compatible with the existing MSVC profile.
  - Create QML modules `wynpix.ui` and `wynpix.render` with
    `qt_add_qml_module` (static backing libraries linked into
    `wynpix`), `NO_GENERATE_QTCONF` if the generated `qt.conf` conflicts
    with the custom plugin path logic in `main.cpp`.
  - Add `qt_add_lupdate` coverage for QML sources.
  - Add a `wynpix_tests` target (Qt Test) and a `wynpix_qml_tests` target
    (Qt Quick Test), registered with CTest. The repository currently has no
    tests; later stages add to these.
  - Add a `--ui=quick` command-line option that opens an empty
    `ApplicationWindow` (no behaviour) instead of `MW`.
- **Acceptance:** Release and Debug presets configure and build; the default
  startup path is unchanged; `--ui=quick` shows an empty window;
  `windeployqt` deploys the QML imports; CTest runs (empty suites pass).

#### S0.2 Ownership and startup cleanup
- **Goal:** remove manual memory management on the startup path before more
  objects are added to it.
- **Owner:** new `AppServices` value-owner in `main.cpp` scope.
- **Scope:** replace `initSingletons()/cleanupSingletons()` with an RAII
  owner (`std::unique_ptr` members, reverse destruction order preserved);
  give `ProxyStyle` and `QLocalServer` deterministic ownership; remove the
  `#ifdef _WIN32` / `__GLIBC__` blocks.
- **Acceptance:** zero manual `delete` in `main.cpp`; behaviour, single
  instance and command-line modes unchanged.

#### S0.3 UI ports: decouple `Core` from `MW`
- **Goal:** `Core` no longer includes or names any widget class.
- **Owner:** `Core` keeps its responsibilities; new interfaces live in
  `gui/ports/` (header-only, no Qt Widgets includes).
- **Split:** the stage is too large for one reviewable unit, so it is
  organised as sub-stages S0.3a–S0.3e. They are delivered together in one
  commit; each sub-stage below states its own scope.
- **Acceptance (whole stage):** `core.h` has no `gui/` include except the
  port headers; `ColdStartWindowController` depends on ports, not `MW`;
  no component under `components/` names `MW`; behaviour unchanged.

##### S0.3a `ColorAdjustments` value type
- `utils/coloradjustments.h` replaces the 7-float colour adjustment
  signatures across `Core`, `MW`, `ViewerWidget`, `ImageViewerV2`,
  `FilterPixmapItem`, `PanoramaGraphicsItem`, the colour adjustments
  overlay, `ImageLib::applyColorAdjustments()`/`getColorAdjustmentMatrix()`
  and `BatchConversionJob`. `hasAdjustments()` replaces the four
  duplicated epsilon checks.

##### S0.3b `INotificationPort`
- `showNotification(NotificationRequest)` (text, kind, optional duration)
  plus `hideNotifications()`; convenience helpers (`showMessage`,
  `showSuccess`, `showWarning`, `showError`, `showAiUpscale`,
  `showDirectory`, `showDirectoryStart/End`) are non-virtual.
- Components no longer reach the window from worker threads:
  `WallpaperController` emits `notificationRequested()` on the GUI
  thread; `FileOperationTask` reports errors through `FileOpTaskNotifier`
  and `FileOpController::operationFailed()`. `Core` forwards both to the
  port.

##### S0.3c `IDialogPort`
- Modal dialogs with value-object results: `confirm` →
  `ConfirmationResult`, `resolveFileReplace` → `FileReplaceDecision`
  (formerly `DialogResult`), `requestSavePath` → `SavePathResult`,
  `requestResize` → `std::optional<ResizeRequest>`, `requestText` →
  `TextInputResult` (create folder), `runBatchConverter` →
  `BatchConversionResult`, `print(PrintRequest)`.
- `FileOperationTask` asks `FileOpController::resolveFileReplace()`
  through a blocking queued call; the controller owns the `IDialogPort`
  reference.
- Settings stays a UI-internal action (the UI host routes
  `openSettings`); the rename prompt is non-modal and lives on
  `IShellPort`.

##### S0.3d `IViewerPort`, `IShellPort`, `IWindowPort`, `IViewModePort`
- `IViewerPort`: displayed image/animation, scaled image and upscaled
  crop delivery, view queries (`visibleOriginalImageRect`,
  `currentScale`, `devicePixelRatio`, `isBusyInteracting`,
  `isRenderingSettled`, `panoramaMode`).
- `IShellPort` (added; not in the original stage text): directory path,
  `ShellFileInfo` (replaces the 11-argument `setCurrentInfo`), metadata,
  sorting notifications, folder tree refresh, save overlay, crop panel,
  fullscreen info bar, rename prompt.
- `IWindowPort` (added): show/hide, visibility, conceal/reveal (opacity),
  raise/activate, native handle, geometry save, update suspension. Used by
  `Core::raiseWindow()`/standby and `ColdStartWindowController`.
- `IViewModePort` is implemented by the new `ViewModeController`
  (`components/viewmode/`), which owns the document/folder mode;
  `viewModeApplied()` drives the widget UI. Covered by `wynpix_tests`.
- `UiEvents` (`gui/ports/uievents.h`) is the inbound side: the user
  intents and readiness signals (`documentRenderingSettled`,
  `visibleThumbnailsReady`, `filesystemViewReady`) `Core` and
  `ColdStartWindowController` connect to.

##### S0.3e Widget UI composition root
- `WidgetUi` (`gui/widgetui/`) owns `MW` (`std::unique_ptr`), the
  `ViewModeController`, `UiEvents` and the port adapters
  (`widgetportadapters.*`), forwards `MW` signals into `UiEvents` and
  connects the window-only `ActionManager` actions (zoom, fit, scroll,
  fullscreen, overlays, settings).
- `Core(const UiPorts &)` receives the ports and the two `IDirectoryView`
  views grouped in `UiPorts`; `main.cpp` builds `AppTranslator` →
  `WidgetUi` → `Core`. Translation loading moved out of `Core` into
  `AppTranslator` so it still precedes widget construction.
- `MW` is now destroyed at exit. This exposed a latent teardown bug in
  `MainPanel` (member order let `~QWidget` delete a non-heap layout) and
  required an explicit release order of the shared widget pointers in
  `~MW`.

#### S0.4 Extract view-transform math from `ImageViewerV2`
- **Goal:** a UI-independent, unit-tested model of zoom/pan/fit that both
  the old and the new viewer use.
- **Owner:** new `ViewTransform` (plain C++ value type + `ViewTransformController`
  `QObject`) in `components/viewtransform/`.
- **Scope:** fit window/width/height/original, expand-small-images, zoom
  levels and fixed steps, anchored zoom (`zoomAnchor`), min/max scale,
  edge snapping, centring, lock zoom / lock view, saved viewport position,
  DPR handling, panorama yaw/pitch/FOV clamping. `ImageViewerV2` is switched
  to use it (refactor only).
- **Acceptance:** `ImageViewerV2` behaviour identical; `wynpix_tests` covers
  every fit mode, anchored zoom invariants, lock modes and DPR 1.0/1.5/2.0.
- **Delivered:**
  - `ViewTransform` (value type: scale, image position in viewport
    coordinates, fit/lock state, `ViewTransformConfig` settings struct),
    `PanoramaView` (yaw/pitch/FOV) and `ViewTransformController`
    (`QObject`; samples viewport size and pointer through `IViewSurface`
    at the start of every operation, then emits `transformChanged`,
    `scaleChanged`, `positionChanged`, `imageCentered`,
    `anchoredZoomApplied`, `panoramaChanged`).
  - `ImageViewerV2` no longer scrolls: its view scene rect is top-left
    aligned so scene coordinates equal viewport coordinates, and the image
    items are placed at `ViewTransform::imagePosition()`. This removes the
    10000/10000 item offset, the scroll-bar clamp of very large zoomed
    images and the first-`fitWindow` scroll-bar workaround.
  - Fit-mode re-detection after a manual zoom uses
    `ViewTransform::kScaleEpsilon` everywhere (previously exact `==` in
    three of four places).
  - Positions may differ from 3.6.4 by at most one logical pixel (rounding
    of the former integer scroll offsets).
  - `wynpix_tests` now runs several suites from `tests/unit/main.cpp`.

#### S0.5 QML bridges for global services
- **Goal:** QML can read settings, theme and actions through typed,
  notifying properties.
- **Owner:** bridge `QObject`s in `gui/quick/bridges/`; `Settings`,
  `ThemeStore`, `ActionManager` themselves stay untouched except for
  signals they already emit.
- **Scope:**
  - `SettingsBridge` (`QML_SINGLETON` + `QML_UNCREATABLE`, instance supplied
    with `QQmlEngine::setExternalSingletonInstance()`): `Q_PROPERTY` per
    UI-relevant setting, grouped by area (viewer, panel, folder view,
    overlays). Re-emits on `Settings::settingsChanged`.
  - `Theme` singleton: all `ColorScheme` colours, fonts, metrics from
    `UiMetrics`; icon glyph lookup from `IconFontManager`.
  - `ActionBridge`: `invoke(name)`, `shortcutFor(name)`,
    `handleKeyEvent()` that forwards `QKeyEvent`/`QWheelEvent` from QML items
    into `ActionManager::processEvent()`; exposes a `QRangeModel` of actions
    for menus.
- **Acceptance:** Qt Quick Test verifies property notifications for a
  settings change and a theme switch; `qmllint` passes on the module.
- **Delivered:**
  - New QML module `wynpix.bridges` (`gui/quick/bridges/`, static library
    imported like `wynpix.ui`). QML names: `AppSettings` (`SettingsBridge`),
    `Theme` (`ThemeBridge`), `Actions` (`ActionBridge`), plus the enum
    namespaces `SettingsEnums` and `FluentIcons`.
  - Push model: the bridges never read the global singletons. They hold
    snapshots (`UiSettingsSnapshot`, `ThemeSnapshot`) or talk to the
    `IActionDispatcher` seam, so `wynpix_qml_tests` links them without the
    application services. The app-side readers live in
    `gui/quick/adapters/` (`BridgeSnapshots`, `ActionManagerDispatcher`);
    `QuickUiHost` owns the bridges (declared before the engine so they
    outlive it), refreshes all three on `Settings::settingsChanged` (which
    also announces theme switches and shortcut edits) and publishes them with
    `setExternalSingletonInstance()`. `--ui=quick` now starts `AppServices`.
  - Settings are grouped into the QML value types `viewerSettings`,
    `panelSettings`, `folderViewSettings` and `overlaySettings`, one notify
    signal per group, emitted only when that group changed. Enum-typed
    settings use the scoped `SettingsEnums.*` mirrors, whose values are taken
    from `settings_types.h`.
  - `Theme`: `colors` (all 44 `ColorScheme` colours, camelCase), `fonts`
    (`base`, `compact`, `section`, `large`), `dark`, `iconFontFamily`,
    constant `compactIconSize`/`standardIconSize`, and
    `glyph(FluentIcons.X)`.
  - `Actions`: `invoke()`, `shortcutFor()`, `handleKeyEvent()` /
    `handleWheelEvent()` (rebuild `QKeyEvent`/`QWheelEvent` from the QML
    event's properties for `ActionManager::processEvent()`), and `actions`, a
    `QRangeModelAdapter` list with `name`/`shortcut` roles that is reset only
    when a shortcut actually changed.
  - Refactors needed by the bridges: `FluentIcon` and its codepoint table
    moved to `utils/fluenticon.{h,cpp}` (`Q_NAMESPACE FluentIcons`, a
    using-declaration keeps `FluentIcon::X`) in the static library
    `wynpix_fluenticons`, so its meta-object exists once; `IconFontManager`
    gained `family()`. The text-style point sizes moved from
    `Settings::loadStylesheet()` into `UiMetrics::typographyFor()`, shared by
    the stylesheet and `Theme.fonts`.
  - `Main.qml` binds its background to `Theme.colors.background`.
    `wynpix_qml_tests` is also the `wynpix.tests` module, whose `Fixture`
    singleton provides per-engine bridges over a fake dispatcher;
    `tst_bridges.qml` covers per-group notification, no notification on an
    unchanged snapshot, binding updates, enums, a theme switch, glyph lookup,
    invoke/shortcut lookup, the model, and key and wheel forwarding.
  - Flagged, not changed: `Settings::absoluteZoomStep()` /
    `setAbsoluteZoomStep()` are declared in `settings.h` but never defined, so
    the bridge does not expose them.

### Phase 1: GPU image renderer (the core of the GPU goal)

All Phase 1 work lives in the `wynpix.render` module and is exercised by a
test QML scene and by `--ui=quick`. The widget viewer is not modified.

#### S1.1 `ImageRenderItem`: base textured rendering
- **Goal:** display a `std::shared_ptr<const QImage>` through QRhi with
  correct premultiplied alpha, mipmaps and nearest/bilinear/trilinear
  sampling.
- **Owner:** `ImageRenderItem : QQuickRhiItem` (GUI thread state) and
  `ImageRenderer : QQuickRhiItemRenderer` (render thread).
- **Scope:**
  - Transform from `ViewTransform` (S0.4) supplied as a value object in
    `synchronize()`.
  - Upload path equivalent to `FilterPixmapItem::uploadPremultipliedTexture`
    (no straight-alpha conversion), `generateMips` on upload.
  - **Tiling** for images larger than `QRhi::TextureSizeMax`: a tile grid of
    textures with one-texel overlap; replaces today's CPU fallback.
  - Transparency checkerboard and background colour drawn in the same pass.
  - Device-loss / `QRhi` resource rebuild handling; errors reported through a
    `renderError(QString)` signal, never swallowed.
- **Acceptance:** pixel comparison against the current viewer at 100 %,
  50 %, 25 %, 300 % on opaque, alpha and 16-bit test images; a 32000 x 8000
  image renders without CPU fallback; no GUI-thread access from the
  renderer.
- **Delivered:**
  - `wynpix.render` (`gui/quick/render/`): `ImageRenderItem`
    (`QQuickRhiItem`, `QML_ELEMENT`) holds GUI-thread state only;
    `ImageRenderer` copies an immutable `RenderFrame` in `synchronize()`
    (`std::shared_ptr<const QImage>` + image generation, `ImagePlacement`,
    `RenderSettings`, DPR). C++ API: `setImage()`, `setPlacement()`,
    `setRenderSettings()`; QML properties `sampling`
    (`RenderEnums.TextureSampling.Nearest/Bilinear/Trilinear`),
    `backgroundColor`, `transparencyGrid`, `imagePosition`, `imageScale`
    (device px per source px, as in `ViewTransform`), `imageSize`.
    `ImagePlacement` does not include `viewtransform.h`; the
    `ViewTransform` -> `ImagePlacement` adapter belongs to `QuickViewerPort`
    (S1.6).
  - One pass, one pipeline (`res/shaders/rhi/image.vert/.frag`, GLSL 440 via
    `qt_add_shaders`): clear to the premultiplied background, then one quad
    per tile, clipped to the item on the CPU in double precision. The
    checkerboard (same 16 px / `#999999` / `#666666` pattern as
    `ImageViewerV2`) is composited under the image in the fragment shader.
    The image origin is snapped to whole device pixels so 1:1 is
    texel-exact at fractional DPR.
  - Upload: `RGB32` / `ARGB32_Premultiplied` as `BGRA8` and
    `RGBA8888_Premultiplied` / `RGBX8888` as `RGBA8` without conversion;
    other 8-bit formats are converted per tile to premultiplied RGBA8.
    **Deviation (approved):** formats above 8 bits per channel go to
    `RGBA16F` premultiplied when supported (better than the 8-bit parity
    baseline). Mips are generated on every upload; conversion runs on the
    render thread, never on the GUI thread.
  - **Deviation (approved):** tiles overlap by `TileGrid::kOverlap` = 64
    texels with texture origins aligned to 64, instead of one texel, so mip
    levels 0..6 match the whole-image pyramid and trilinear is seamless down
    to 1/64 scale; below that, slight seams are possible.
  - Errors (shader load, resource `create()`, untileable size, missing
    update batch) are reported once per distinct message through
    `RenderErrorChannel` -> `renderError(QString)` on the GUI thread
    (queued `invokeMethod`). `QQuickRhiItemRenderer::update()` does not
    re-run `synchronize()`, so errors cannot be handed over there. A new
    `QRhi` in `initialize()` (window change, device loss) drops all
    resources and re-uploads from the retained image.
  - `Main.qml` contains an `ImageRenderItem` bound to the theme background
    and the transparency-grid setting; it gets images from `Core` in S2.1.
  - Tests: `wynpix_tests` gained the `TileGrid` suite. New
    `wynpix_render_tests` renders headless through `QQuickRenderControl` on
    Direct3D 11 and compares against CPU references (box averages = exact
    mip levels, nearest / bilinear magnification) for opaque, alpha and
    16-bit images at 100 / 50 / 25 / 300 %, plus checkerboard, tiled vs
    untiled, a 32000 x 8000 image (two tiles on D3D11, no CPU path), error
    reporting and moving the item to another `QRhi`. The comparison is
    against these CPU references, not an automated capture of the widget
    viewer.
  - `Qt6::GuiPrivate` is now required (QRhi headers);
    `QQuickRhiItem` is still a technology preview in 6.12.

#### S1.2 Shader port: colour adjustments, sharpening, exact downsample
- **Goal:** feature parity with `filter.frag` and `boxreduce.*` on all RHI
  backends.
- **Owner:** `ImageRenderer`; shader sources in `res/shaders/rhi/*.frag/.vert`
  (GLSL 440), compiled by `qt_add_shaders`.
- **Scope:** port CAS, smart sharpen (CPU-equivalent path
  `QI_FILTER_SMART_GPU`), colour matrix/offset adjustments; settle-triggered
  exact-ratio box reduce into an intermediate render target
  (`QRhiTextureRenderTarget`), reused while the scale is unchanged; uniform
  buffer layout as a `std140` struct with named `constexpr` defaults.
- **Acceptance:** visual parity with the widget viewer for every
  `ScalingFilter` GPU mode; D3D11, D3D12 and Vulkan backends verified via
  `QSG_RHI_BACKEND`.
- **Delivered:**
  - `ColorMatrix` and the `ColorAdjustments` -> matrix derivation moved from
    `ImageLib::getColorAdjustmentMatrix()` to the header-only
    `colorAdjustmentMatrix()` in `utils/coloradjustments.h`, shared by the
    CPU path, the widget viewer and the renderer (the matrix math is
    unchanged).
  - `ImageFilter` value type in `RenderFrame` (`RenderEnums.Sharpening`
    `None/Cas/Smart`, CAS strength/contrast, `ColorAdjustments`) and a
    `settled` flag. `ImageRenderItem` gained `setImageFilter()`,
    `setColorAdjustments()` and the QML properties `sharpening`,
    `casSharpening`, `casContrast`, `settled`. `imageFilterModeFor()` maps
    every `ScalingFilter` to sampling + sharpening (Smart / MKS2021 show as
    trilinear without sharpening; S1.3 added the GPU MKS2021 mode); the viewer port applies it in
    S1.6. `Main.qml` binds the CAS settings.
  - `image.frag` ports CAS and smart sharpening (plain and downscale taps,
    luma-only weights, opacity gate, off at 1:1) and the colour matrix on
    straight colour, with the original constants named. An identity
    adjustment skips the colour math, so unfiltered rendering is unchanged
    from S1.1. The sharpened colour is computed in uniform control flow
    (implicit derivatives of the biased taps stay defined on every backend).
    The uniform block is mirrored by `std140` C++ structs with `static_assert`
    offsets and named defaults.
  - Exact-ratio downsample (`boxreduce.vert/.frag`): while settled below 1:1,
    every visible tile is reduced to its on-screen size by a chain of
    exact-area passes into `QRhiTextureRenderTarget`s (sides halved rounding
    up, ratios in [0.5, 1]) in the tile's own format, then drawn bilinearly
    with the plain sharpening taps. Cached per tile until the scale or the
    image changes, so panning reuses it; while not settled the mip chain is
    drawn. Intermediate targets are still used by the frame being
    recorded and are released with `QRhiResource::deleteLater()`. The caller
    (S1.6) must clear `settled` during zoom, pan, resize and animation
    playback, as `ImageViewerV2` does for `FilterPixmapItem`.
  - **Deviation (approved): exact downsample on tiled images** is built per
    tile from its overlapping texture, so seams match the untiled result; the
    sharpening taps near a seam clamp to the 64-texel overlap below about
    1/23 scale.
  - **Found and worked around: Qt 6.12 Direct3D 12 `generateMips()` is wrong
    from mip level 5 on** (the compute generator works in batches of four;
    the second batch produces wrong data). Pre-existing since S1.1, found by
    the new backend runs. On D3D12 the renderer builds the mip chain with the
    box-reduce pipeline (each level rendered and copied into the mip level);
    the other backends keep `generateMips()`. Revisit when Qt fixes it.
  - Tests: `wynpix_tests` gained `ImageFilterTests` (matrix derivation,
    filter mapping). `wynpix_render_tests` gained CPU references for the
    colour matrix, plain CAS / smart sharpening and the exact reduce chain,
    property tests for the downscale taps (flat image unchanged, grey stays
    grey, visible sharpening), the settled / pan / unsettled cycle, tiled vs
    untiled exact downsample and a per-level mip chain check. The harness
    selects the backend through `QSG_RHI_BACKEND`; CTest runs D3D11, D3D12
    and Vulkan (Vulkan on the `windows` QPA platform, because the offscreen
    platform cannot create Vulkan instances). All three pass on the
    reference machine (RTX 3060).

#### S1.3 GPU Magic Kernel Sharp 2021 display filter
- **Goal:** a GPU MKS2021 viewing filter, the default for viewing, without
  the `Scaler` CPU round trip.
- **Owner:** `ImageRenderer` (render thread) for display. The CPU filters
  (`ImageLib::scaled_Smart`, `scaled_MKS2021`) and `Scaler` stay unchanged:
  resize, batch conversion, wallpapers, the AI-upscale resize and the widget
  viewer use them.
- **Scope (revised 2026-10-09):** the original scope (porting CPU Smart and
  MKS2021 to shaders, with a CPU fallback setting) was replaced. The widget UI
  remains the fallback until S4.1, so no fallback setting is added. The CPU
  Smart filter is not ported; the GPU filters CAS and Smart (GPU) were already
  ported in S1.2.
- **Acceptance:** GPU output matches the CPU MKS2021 kernel within a
  documented tolerance on D3D11, D3D12 and Vulkan; the Quick viewer needs no
  `scalingRequested` traffic for the new mode; settle latency measured and
  not worse than the CPU path.
- **Delivered:**
  - New `ScalingFilter` value `QI_FILTER_MKS2021_GPU`, appended (value 6) so
    stored configurations keep their meaning; it is the default of
    `Settings::scalingFilter()` and of the first-run settings. Shown as
    "Magic Kernel Sharp 2021 (GPU)" in the settings dialog, the filter
    message and the filter cycle; `SettingsEnums::ScalingFilter::Mks2021Gpu`
    in the QML bridge. The CPU "Magic Kernel Sharp 2021" entry is kept.
    Resize and batch-converter dialogs are unchanged (file output stays on
    the CPU).
  - Widget viewer: the new mode requests the CPU MKS2021 from `Scaler`
    (identical kernel), so the default works in the widget UI until it is
    removed in S4.2.
  - `RenderEnums::Resampling {None, Mks2021}` in `ImageFilter`, QML property
    `resampling` on `ImageRenderItem`; `imageFilterModeFor()` maps
    `QI_FILTER_MKS2021_GPU` to trilinear sampling + `Resampling::Mks2021`.
    CPU Smart / MKS2021 still show as trilinear in the Quick viewer (their
    `Scaler` display is S1.6 work if it is ever needed).
  - `res/shaders/rhi/resample.vert/.frag`: one separable pass per axis
    (horizontal into an intermediate texture of the tile's format, then
    vertical), premultiplied, clamped to [0, 1] per pass like the CPU, the
    final pass clamps rgb to alpha. The per-output taps are built on the CPU
    exactly like `buildMksAxisTaps()` (`Mks2021Axis::weights()` in
    `gui/quick/render/resamplegrid.*`) and uploaded as an R32F weight table;
    evaluating the kernel per tap in the shader was ALU-bound and several
    times slower. Baked for GLSL ES 3.00 / GLSL 3.30 and up (texelFetch and
    dynamic loops; no GLSL ES 1.00 variant).
  - Output grid: the image at `round(size * scale)` device pixels, the CPU
    target size, drawn 1:1 at the snapped image origin. Only the visible
    outputs are computed, per tile (each output belongs to the tile whose
    core contains its source centre), so strong magnification of huge images
    has no size cap (the CPU display path stops at 12288 px / 100 MP). The
    result is cached per tile until the scale or the visible part changes;
    it is drawn only while `settled` and not at 1:1, otherwise the mip chain
    is drawn. Resampling replaces the exact box downsample for this mode.
    Failures fall back to the mip chain and are reported through
    `renderError`.
  - **Limits (documented):** tiled images clamp taps at the 64-texel tile
    overlap, so below about 1/14 scale the seams can differ slightly from an
    untiled result; below about 1/1800 scale the weight table would exceed
    the texture size limit and the mip chain is drawn.
  - Tests: `wynpix_tests` covers the filter mapping, the tap geometry and
    weights and the tile partition of the outputs. `wynpix_render_tests`
    compares against a double-precision copy of the CPU kernel (8-bit
    intermediate) for opaque and alpha images at 300 / 170 / 50 / 37 / 20 %
    (tolerance 2 levels), the visible part of a 4x magnified image, tiled vs
    untiled (1 level), the settled / pan / unsettled cycle and 1:1. All pass
    on D3D11, D3D12 and Vulkan (RTX 3060).
  - Settle latency (6000 x 4000 image, 1920 x 1280 frame, RTX 3060, idle
    GPU; `mks2021SettleLatency`, fastest of 9 rebuilds, extra time of the
    settled frame over an unsettled one including read-back):
    | Case | CPU `scaled_MKS2021` | GPU D3D11 | GPU D3D12 | GPU Vulkan |
    |---|---|---|---|---|
    | Fit (0.32) | 45 ms | 6.5-7 ms | 6.3-6.5 ms | 5.5-5.6 ms |
    | 300 % | not shown (over the 12288 px / 100 MP cap; 218 ms for the full image) | 1.5 ms | 1.9 ms | 1.6 ms |

    The CPU value is the unchanged CPU code (median of 5); the CPU path also
    adds the 80 ms settle delay, colour management and the upload of the
    result, so the GPU settle is about an order of magnitude faster.

#### S1.4 GPU HDR tone mapping and colour management
- **Goal:** move `HdrToneMapper` and display colour transforms to the GPU.
- **Owner:** `ImageRenderer` (shaders); `ColorManager` remains the single
  source of the target colour space and produces transform data.
- **Scope:**
  - Upload linear float images (`QImage::Format_RGBA16FPx4` /
    `RGBA32FPx4`) as `RGBA16F` / `RGBA32F` textures.
  - Port each `ToneMapOperator` and the "HDR white" parameter to shader
    code; CPU tone mapping is skipped when the GPU path is active.
  - Colour management: parametric spaces (sRGB, Display P3, Adobe RGB,
    custom primaries) as a 3x3 matrix + transfer function in the shader;
    ICC profiles that are not parametric as a 3D LUT texture built once per
    (source profile, target profile) pair on a worker thread with
    `QColorTransform`, cached by profile identity.
- **Acceptance:** EXR/HDR/AVIF/JXL HDR samples match the CPU output within
  tolerance; switching operator or white level re-renders without
  re-decoding; LUT generation never runs on the GUI or render thread.
- **Delivered:**
  - **Scope (agreed 2026-10-09):** renderer side only. The HDR detection
    rules moved unchanged from `hdrtonemapper.cpp` into the header-only
    `utils/hdrsource.h` (`isHdrImage()`, `isLinearFloatHdrFormat()`,
    `detectHdrSourceEncoding()`), shared by `HdrToneMapper` and the
    renderer; CPU behaviour is unchanged. **Deferred to S2.1:** keeping the
    HDR source in `ImageStatic` and skipping CPU tone mapping when the Quick
    viewer is active (nothing consumes it before `Core` feeds the Quick
    viewer).
  - `ImageRenderItem` resolves a `SourceConversion` on the GUI thread from
    the image (HDR detection, its colour space), `ToneMapping` (enabled,
    `RenderEnums::ToneMapOperator`, white level in nits; same values as
    `ToneMapOperator` / the settings) and `ColorManagement` (enabled, target
    `QColorSpace` supplied by the caller from
    `ColorManager::getTargetColorSpace()`; the renderer never calls
    `ColorManager`). C++ setters `setToneMapping()` /
    `setColorManagement()`, QML properties `toneMapping`,
    `toneMapOperator`, `hdrWhiteLevel`. The bridge and `Main.qml`
    bindings are S1.6 / S2.1 wiring.
  - **Conversion pass** (`res/shaders/rhi/convert.vert/.frag`): when the
    conversion is active, each tile is uploaded with straight alpha into a
    source texture and rendered once into the tile's displayed texture
    (premultiplied, display-encoded), whose mip chain is built afterwards.
    The mip chain, sharpening, exact downsample and MKS2021 then work
    unchanged on display-encoded texels, which is the CPU order (tone map ->
    colour management -> scale / filter). HDR samples are decoded and tone
    mapped exactly like `HdrToneMapper` (same constants: PQ / HLG / scRGB
    decode, white level, BT.2020 / P3 -> sRGB matrices, gamut compression,
    the four operators with highlight desaturation), then colour managed from
    sRGB like the CPU. Tone mapping off clamps and shows the samples as sRGB
    (the CPU fallback). SDR images already in the display space keep the
    S1.3 path with no extra pass or memory.
  - **Re-render without re-decode:** HDR source textures are kept, so a new
    operator, white level or display space only re-runs the conversion and
    mip passes (cost: one source texture per HDR image in VRAM). SDR sources
    are released after the conversion; a new display space re-uploads them
    from the retained `QImage`.
  - **Deviation (approved): source precision.** Float HDR images
    (`RGBA16FPx4`, `RGBA32FPx4`) are uploaded as `RGBA16F` (the CPU mapper
    quantizes 32-bit floats to the same half floats); 16-bit integer HDR
    images (PQ / HLG codes) as `RGBA32F`, because half floats shifted dark
    PQ codes by up to 13 8-bit levels after tone mapping. Displayed textures
    are `RGBA16F` for float sources, `RGBA8` otherwise.
  - **Colour transforms** (`gui/quick/render/colortransformplan.*`, pure
    functions): source and target both `ThreeComponentMatrix` with a named
    transfer function (every `ColorManager` preset and most monitor ICC
    profiles) -> parametric (transfer curve with Qt's parameters, 3x3 matrix,
    inverse curve). The matrix comes from the ICC colorant tags (`rXYZ`,
    `gXYZ`, `bXYZ`) of `QColorSpace::iccProfile()`, i.e. the D50 matrix Qt's
    own transforms use. Everything else (table curves, element-list
    profiles) -> a 33^3 `RGBA16F` 3D LUT built by `QColorTransform` in
    `ColorLutBuilder` on the thread pool. Results come back through a queued
    call guarded by a lifetime token and the task id. The builder caches 4
    (source, target) pairs by ICC identity and reports each failure once.
    Until the LUT is ready the image is shown without the colour transform.
    Grey and other non-RGB spaces and invalid targets are reported through
    `renderError` and shown unconverted.
  - **Found: Qt 6.12 `QColorSpace::primaryPoints()` returns its points in
    the wrong fields** (red in `whitePoint`, ...), and `PrimaryPoints::
    isValid()` fails for Display P3 and BT.2020. That is why the colorant
    tags are read instead.
  - **Found, out of scope (flagged):** `ColorManager`'s "ProPhoto" preset
    builds an invalid `QColorSpace` (Qt rejects the blue primary with
    y = 0), so the CPU path silently skips colour management for it. The
    GPU path receives the same invalid target and reports it.
  - Tests: `wynpix_tests` gained `ColorTransformTests` (curve round trips,
    plans for named and ICC spaces against `QColorTransform`, LUT lattice
    values, builder cache / single failure / destroyed builder).
    `wynpix_render_tests` links the unchanged `hdrtonemapper.cpp` and compares
    the GPU with it for PQ, PQ with alpha, HLG, untagged half-float and
    linear-sRGB float images x 4 operators, a 400 nit white level and tone
    mapping off (tolerance 2 levels). It compares colour management with
    `QImage::convertedToColorSpace()` for opaque / alpha / 16-bit images to
    P3, ProPhoto, linear sRGB (2 levels) and Adobe RGB / Rec2020 (4 levels:
    Qt's 8-bit tables flatten the steep start of inverse pure-gamma curves,
    and the GPU's analytic value is the exact one). LUT targets are tested
    within 4 levels (darkest shades of a curve that is infinitely steep at
    black) and HDR + colour management within 3. Also covered: operator /
    white level / enable switches (no `imageChanged`, same result as a
    fresh render), the display-space switch re-upload, identity colour
    management being bit-identical to the plain path, tiled = untiled, and
    exact downsample / MKS2021 on converted images. All pass on D3D11,
    D3D12 and Vulkan (RTX 3060).
  - Startup: the conversion resources (two shaders, a 1x1x1 3D placeholder)
    are created with the renderer's other device resources; no thread-pool
    work happens until an image needs a LUT.

#### S1.5 Animation, panorama and upscaled-crop layers
- **Goal:** remaining viewer content types on the GPU path.
- **Owner:** `ImageRenderItem` (layers), `ImageAnimated` (frame source,
  unchanged).
- **Scope:** animated frames pushed as texture updates (reuse textures,
  no reallocation per frame), playback/loop/finished state exposed as
  properties; panorama mode as a renderer mode using a ported
  `panorama.frag`; upscaled-crop overlay layer (`setUpscaledCrop`) as a
  second textured quad.
- **Acceptance:** GIF/WebP/APNG/animated SVG playback timing matches the
  widget viewer; panorama yaw/pitch/FOV parity; AI upscale crop displays at
  the correct position at every zoom.
- **Delivered:**
  - **Found:** `ImageAnimated` is not the frame source of the widget
    viewer; `ImageViewerV2` drives its own `QMovie` (timer, loop, stepping).
    `ImageAnimated` stays unchanged (metadata only).
  - **Playback** in the new UI-free component `AnimationPlayer`
    (`components/animationplayer/`, compiled into the app; wired to the
    viewer by `QuickViewerPort` in S1.6). It wraps `QMovie` with the widget
    viewer's schedule: each frame stays for `QMovie::nextFrameDelay()`
    (precise single-shot timer), after the last frame it loops or stops with
    `playbackFinished()`, `nextFrame()` / `prevFrame()` wrap without
    changing the playing state, enabling the loop resumes playback.
    Properties `playing`, `loop`, `finished`, `frameIndex`, `frameCount`;
    frames are published as `std::shared_ptr<const QImage>` through
    `frameReady()`; decode failures through `playbackError()`. The timing
    logic now exists twice (`ImageViewerV2` and `AnimationPlayer`); flagged,
    resolved by the widget removal in S4.2.
  - **Texture reuse:** `ImageRenderItem::setImage(image,
    ImageUpdate::AnimationFrame)` keeps the upscaled crop and the source
    traits; the renderer uploads a frame of the same size and texture
    formats into the existing textures (mips regenerated, filter caches
    dropped) and keeps converted sources of animation frames, so playback
    with colour management allocates nothing per frame. `RenderStatistics`
    (texture creations, uploads), readable through
    `ImageRenderItem::statistics()`, makes this testable.
  - **Refactoring (same commit, user decision):** `ImageRenderer` was split.
    `RenderDevice` holds the per-`QRhi` resources (shaders, quad, samplers,
    layout bindings, colour LUT, offscreen pass pipelines and their
    recording); `ImageLayer` holds one image's tiles, upload, conversion,
    exact downsample and MKS2021 caches; `ImageRenderer` only orchestrates
    the frame and owns the two pipelines of the item's render pass.
    `RenderErrorReporter` carries the once-per-message error reporting;
    the std140 uniform mirrors moved to `rhipassuniforms.h`. The existing
    render tests pass unchanged on all backends. Fixed on the way: a failed
    upload no longer submits an update batch that references the released
    textures.
  - **Panorama:** `RenderEnums::Projection {Flat, Equirectangular}` and
    `PanoramaCamera` (yaw / pitch / vertical FOV in degrees, as
    `PanoramaView`) in `RenderFrame`; QML properties `projection`,
    `panoramaYaw`, `panoramaPitch`, `panoramaFov`.
    `res/shaders/rhi/panorama.vert/.frag` port `panorama.frag` (same ray,
    rotation order and mapping; colour matrix on straight colour). The image
    keeps its conversion (tone mapping, colour management), which the widget
    panorama lacked. **Deviations (improvements):** images above the
    texture size limit render as panoramas (one full-item quad per tile,
    rays outside the tile core are discarded; a single-column image uses a
    horizontally repeating sampler, a multi-column one has a clamped
    one-texel seam at the back); sampling follows the item's `sampling`
    with explicit gradients whose longitude part is corrected at the back
    seam, so trilinear sampling no longer aliases on large panoramas.
    Built for GLSL ES 3.00 / GLSL 3.30 and up (`textureGrad`).
  - **Upscaled crop:** `setUpscaledCrop(std::shared_ptr<const QImage>,
    QRect sourceRect)` / `clearUpscaledCrop()`, property `hasUpscaledCrop`.
    The crop is a second `ImageLayer` drawn after the image over
    `sourceRect`: corner snapped to whole device pixels, scale
    `imageScale * sourceRect.width / crop.width` (the widget viewer's
    formula), same sampling, sharpening, exact downsample / MKS2021 and
    colour adjustments, and its own `SourceConversion` from its colour space
    (the `Upscaler` keeps the source's). A new image clears it, animation
    frames keep it, panorama mode does not draw it (as the widget viewer).
  - Tests: `wynpix_tests` gained `AnimationPlayerTests` (an embedded GIF:
    schedule equals `QMovie::nextFrameDelay()` per frame, loop, stop at the
    end, stepping, single frame, errors, close); the suite now runs under
    `QGuiApplication`. `wynpix_render_tests` gained animation texture reuse
    (plain and colour managed), panorama against a double-precision CPU
    port of `panorama.frag` (four cameras incl. the back seam and colour
    adjustments, 2-4 levels), seam detail with trilinear sampling (verified
    to fail without the gradient correction), tiled = untiled panorama, the
    crop at 50 / 100 / 200 / 400 % (panned) against references, the colour
    managed crop and the crop lifetime. All pass on D3D11, D3D12 and Vulkan
    (RTX 3060). Real GIF / WebP / APNG / animated SVG files through the
    Quick viewer are checked when S1.6 / S2.1 wire the player in.

#### S1.6 `ImageViewport` interaction component
- **Goal:** the complete viewer as a reusable QML component.
- **Owner:** `ImageViewport.qml` (visuals, input handlers) +
  `ViewTransformController` (all math and state).
- **Scope:** `WheelHandler` (wheel zoom, trackpad detection preserved),
  `DragHandler` (pan, wrapping pan, RMB zoom gesture, drag-out start),
  `PinchHandler`, `TapHandler` (double click, click zones); smooth zoom and
  scroll animations driven by the controller, disabled by
  `QAccessibilityHints::motionPreference`; key events forwarded to
  `ActionBridge`; `renderingSettled` derived from
  `QQuickWindow::afterFrameEnd` after the settle pass completes (replaces
  `QOpenGLWidget::frameSwapped`).
- **Acceptance:** every viewer action in `ActionManager` works in
  `--ui=quick`; zoom indicator and cursor auto-hide behave as before;
  `IViewerPort` is fully implemented by a `QuickViewerPort` adapter.
- **Delivered:**
  - **Scope decision (user):** verified by tests only. Core is not attached
    to `--ui=quick` before S2.1, so real files are checked by hand there;
    `QuickViewerPort` and the viewport's outbound signals
    (`scalingRequested`, `nextImageRequested` / `prevImageRequested`,
    `draggedOut`, `renderingSettled`) are wired to Core / `UiEvents` in
    S2.1.
  - **`wynpix_viewcomponents`** static library (`ViewTransform`,
    `ViewTransformController`, `AnimationPlayer`, the new
    `ViewportInteraction` and `ScalingFilterSelection`), linked by
    wynpix, `wynpix.ui` and the tests instead of compiling the sources
    into each.
  - **`ViewportInteraction`** (`components/viewtransform/`): the widget
    viewer's mouse state machine without widgets (left-button pan /
    drag-out, right-button zoom stroke and next/previous gesture, wheel zoom
    with the right button, DPR-scaled thresholds), `WheelClassifier`
    (wheel vs. trackpad heuristic and cooldown) and the wheel scroll
    helpers. `ScalingFilterSelection` (`components/scalingfilter/`) holds
    the toggle/cycle decision that lives in `MW` for the widget UI.
  - **`ImageViewportController`** (`wynpix.ui`, `QML_ELEMENT`, uncreatable):
    owns the `ViewTransformController`, the interaction, `AnimationPlayer`,
    the smooth zoom (smootherstep) and scroll (OutSine) animations as
    `QVariantAnimation`s (jumps when `motionPreference` is `ReducedMotion`),
    the settle pass, click zones, cursor auto-hide and the zoom indicator
    state; drives the `ImageRenderItem` set as its `view` (image, animation
    frames, placement, filter via `imageFilterModeFor`, CAS strength,
    transparency grid, colour adjustments, projection and panorama camera,
    upscaled crop, `settled`). Settings arrive as `UiSettingsSnapshot`
    through `applySettings()` from `QuickUiHost`; `ViewerSettings` gained
    `useUpscayl`. Passed to `Main.qml` as the required property
    `viewportController`.
  - **`ImageViewport.qml`**: `ImageRenderItem`, one `MouseArea` (all
    buttons, hover) and a `PinchHandler`, the zoom indicator and the click
    zone pills; keys, and pointer events the controller does not consume,
    go to `Actions` (`ActionBridge` gained `handleMousePress`,
    `handleMouseRelease`, `handleMouseDoubleClick`, so mouse shortcuts such
    as `LMB_DoubleClick` and the right-click menu work as in the widget UI).
    **Deviation:** a `MouseArea` instead of `WheelHandler` / `DragHandler` /
    `TapHandler`. One press-move-release sequence decides between zoom,
    gesture, drag-out and right click; split over several handlers that
    decision would be made by grab arbitration in QML instead of in the
    tested controller. "Wrapping pan" was dropped: the widget viewer has no
    such behaviour.
  - **Settling:** any view change unsettles the item; the settle pass runs
    80 ms after the view rests or when a zoom ends, and `renderingSettled`
    follows `FramePresentationTracker`: requests are numbered, the render
    thread records the newest one in `beforeSynchronizing` and reports it in
    `afterFrameEnd`, so a frame already in flight never completes a newer
    request. During animation playback the item stays unsettled.
  - **Scaling requests:** the GPU shows every filter, so a CPU-scaled copy
    is requested only as the AI upscaler's source (Upscayl on, scale above
    1:1, not panorama / animation); `QuickViewerPort::showScaledImage`
    ignores the result. **Flagged for S2.1:** Core only starts an upscale
    after a CPU scale finishes, which costs one unneeded CPU scale per
    upscale in the Quick UI.
  - **`QuickViewerActions`** (`gui/quick/adapters/`): fit, zoom, scroll,
    lock, transparency grid, filter toggle/cycle and panorama actions of
    `ActionManager` run on the controller; the confirmation messages (the
    widget UI's `MW` translations) go out as `notificationRequested` and are
    logged until S2.3. Not in this stage: context menu (S2.5), viewport
    copy to clipboard (needs a GPU readback, S2.1), Upscayl / HDR toggles
    and "Zoom temporarily disabled" (S2.3 / S2.5).
  - **Deviations (improvements):** session overrides (temporary filter,
    transparency grid) reset only when viewer settings change, not on
    every settings notification; one drag-out per press; a press in a
    click zone suppresses panning until release; a double click in a click
    zone does not navigate a third time.
  - **Flagged:** the interaction logic now exists twice (`ImageViewerV2` /
    `ViewerWidget` and the controller), resolved by S4.2; `MW` keeps its own
    filter toggle/cycle code.
  - Tests: `wynpix_tests` gained `ViewportInteractionTests` (every mode,
    thresholds at DPR 1.0 / 1.5 / 2.0, wheel classification, scroll
    helpers, filter selection, the tracker's numbering) and
    `ImageViewportControllerTests` (fit / zoom / smooth zoom, pan,
    drag-out, gestures, right click, wheel and trackpad, upscale requests,
    panorama, click zones, zoom indicator modes, session overrides, visible
    rect, rotation, animation); `wynpix_render_tests` gained
    `viewportSettlesAfterThePresentedFrame` on the real frame loop
    (mutation-checked); `wynpix_qml_tests` gained `tst_imageviewport.qml`
    (forwarding of keys, double clicks, right clicks and unused wheels to
    the shortcuts). All pass on D3D11, D3D12 and Vulkan.

### Phase 2: Quick application shell

#### S2.1 Main window, startup and window states
- **Goal:** a functional `--ui=quick` window showing images from `Core`.
- **Owner:** `QuickMainWindowController` (`QObject`) implementing the ports
  for the Quick UI; `Main.qml` for layout only.
- **Scope:** `QQmlApplicationEngine` created after services; geometry
  save/restore, fullscreen and pseudo-fullscreen, multi-display memory;
  `DropArea` forwarding to `Core::onDropIn` via `MimePayloadManager`;
  `ColdStartWindowController` adapted to reveal on the first rendered frame
  (no opacity trick, no delayed show; see `AGENTS.md` startup rules);
  pipeline cache load/save via `QQuickGraphicsConfiguration`; graphics API
  setting (D3D11 default, D3D12, Vulkan) applied before the first window.
- **Acceptance:** cold start time and first-frame time measured against the
  widget UI on the same machine (Release preset) and not worse; single
  instance raise works.
- **Delivered:**
  - **Startup (`main.cpp`):** both user interfaces run through one path:
    `SingleInstanceChannel` (`components/singleinstance/`, the former
    inline `QLocalServer` / `QLocalSocket` code) → `AppServices` →
    `AppTranslator` → `WidgetUi` or `QuickUiHost` → `runCore()` (Core,
    command-line or `Core::loadDefaultPath()`, `showGui()`). `--ui=quick`
    now takes part in the single-instance hand-off and runs Core.
    `main.cpp` defines `NOMINMAX`: `core.h` includes `<windows.h>` through
    the directory watcher headers, and its `max` macro broke `QRangeModel`
    from the bridges. **Flagged:** that `<windows.h>` leak into every Core
    includer is not fixed here.
  - **`QuickUiHost`** is the Quick composition root, like `WidgetUi`: it
    owns `UiEvents`, `ViewModeController`, the port adapters and the
    engine, and its `ports()` returns `std::nullopt` until `start()` has
    created the window. The viewport's `scalingRequested`,
    `renderingSettled` (→ `documentRenderingSettled`), `draggedOut` and
    next/previous signals go to `UiEvents`. Viewer messages and playback
    errors go to the notification port.
  - **`QuickMainWindowController`** (`gui/quick/adapters/`) implements
    `IWindowPort` and `IShellPort` on the `QQuickWindow`. It owns
    `WindowStateController` (`components/windowstate/`, works on any
    `QWindow`): saved placement (geometry, maximized, display) restored on
    the hidden window, reported once the window has rested (30 ms) and
    persisted in `Settings`; pseudo-fullscreen as a frameless window over
    the remembered display; a saved geometry without a display is centred
    on the primary display. The controller also handles the
    `toggleFullscreen` and `closeFullScreenOrExit` actions, and window
    close (standby or exit, as `MW::closeEvent`, an event filter on the
    window). The window title comes from `windowTitleFor()`
    (`components/shellinfo/`), the widget UI's title rules in the `MW`
    translation context. **Flagged:** `MW` keeps its own copies of the
    window-state and title logic until S4.2.
  - **`MainWindowShell`** (`wynpix.ui`, required property `windowShell`):
    `folderViewActive`, `fullscreen` (fullscreen background colour), and
    `dropUrls()` from the `Main.qml` `DropArea`. The host turns the URLs
    into a `QMimeData` and emits `UiEvents::droppedIn`. **Deviation:** not
    through `MimePayloadManager`, which builds outbound payloads only;
    `Core::onDropIn()` reads the URLs directly.
  - **Interim ports**, to be replaced by their stages:
    `LoggingNotificationPort` (S2.3); `DecliningDialogPort`, which declines
    every dialog, so nothing is deleted, overwritten or saved without
    confirmation (S3.2); `PlaceholderDirectoryView` for the thumbnail strip
    (S2.4) and the folder view (S3.1). The folder placeholder reports both
    readiness signals when populated, and `Main.qml` shows a "not available
    yet" page in folder mode that keeps the action shortcuts. Shell parts
    without a Quick UI (metadata, folder tree, sorting, overlays, crop panel,
    rename prompt) are logged once.
  - **Cold start:** `ColdStartWindowController` and `Core::raiseWindow()`
    are unchanged. The Quick window port does not conceal (no opacity
    trick): the window is shown at once with the theme background as its
    clear colour, and the image appears with the frame that renders it.
    **Superseded 2026-10-13:** the port cloaks the window again; see S4.2
    "Post-audit fix: cold-start reveal".
    `setWindowUpdatesSuspended()` does nothing either (the scene graph
    synchronizes only between event-loop iterations).
  - **Graphics:** hidden setting `quickGraphicsApi` (`d3d11` default,
    `d3d12`, `vulkan`) applied with `QQuickWindow::setGraphicsApi()` before
    the window is created. The pipeline cache is loaded from and saved to
    `cache/quick-pipeline-<api>.cache` through `QQuickGraphicsConfiguration`;
    `Main.qml` is created hidden so the configuration applies before the
    first exposure.
  - **Workaround (Qt 6.12):** the DXGI vertical blank thread of the Windows
    platform plugin stopped delivering update requests on the development
    machine (GeForce RTX 3060, 164 Hz, single display). After the first
    frame no further frame was rendered, so the viewport never settled and
    every QML animation and timer stalled; Qt's own `qml` tool hangs the
    same way. `QuickUiHost` sets `QT_D3D_NO_VBLANK_THREAD=1` for the
    Direct3D backends unless the user set it; frames are still paced by the
    swap chain's vertical sync.
  - **Measured** (Release, same machine, `image-2.jpg` 300×300, process
    start → first `documentRenderingSettled`, 6 runs each, logged with
    `QT_LOGGING_RULES="wynpix.startup.info=true"` by
    `utils/startuptiming`): widget UI median ≈ 510 ms (501–606), Quick UI
    median ≈ 322 ms (314–438); the Quick UI's first frame is presented 2–3
    ms before it settles. A second launch hands its path to a running Quick
    UI and exits in 0.04 s.
  - **Not in this stage:** `autoResizeWindow` (`MW::preShowResize`), the
    fullscreen info bar and the controls overlay (S2.3), and the settings
    dialog entry for the graphics API (S3.3).
  - Tests: `wynpix_tests` gained `WindowStateTests` (restore, maximized,
    off-screen geometry, pseudo-fullscreen and back, start in fullscreen,
    debounced and immediate reports, no reports in fullscreen),
    `SingleInstanceTests` (hand-off from a second thread, empty path,
    several forwards, no primary), `WindowTitleTests` and `QuickShellTests`
    (drops, shell state, placeholder views). `wynpix_qml_tests`:
    `tst_mainwindow.qml` checks that the window is created hidden, the
    fullscreen background, the folder-mode page, and an external file drop
    (real `QDragEnter` / `QDrop` events sent by the fixture) reaching the
    shell.

#### S2.1b Quick viewer carry-overs
- **Goal:** finish the viewer work earlier stages deferred to S2.1.
- **Owner:** `ImageStatic` / `utils/hdrsource.h` (HDR source), `Core`'s
  upscale trigger, `ImageViewportController` with the renderer (readback).
- **Scope:**
  - From S1.4: for the Quick UI, `ImageStatic` keeps the HDR source and
    skips CPU tone mapping; the GPU conversion pass tone maps it.
  - From S1.6: start the AI upscale without first requesting a CPU-scaled
    copy the Quick UI never displays.
  - From S1.6: `copyViewportToClipboard` through a GPU readback of the
    viewport.
- **Acceptance:** HDR images look the same as with CPU tone mapping (pixel
  test); an upscale in the Quick UI runs no CPU scale; the copied viewport
  matches the screen; the widget UI is unchanged.
- **Delivered:**
  - **Scope decision (user):** GPU-only display with a lazily built CPU
    copy, rather than keeping both copies at load.
  - **`DisplayPipeline`** (`utils/displaypipeline.h`): `Cpu` (widget
    viewer) or `Gpu` (Quick viewer), reported by the new
    `IViewerPort::displayPipeline()`. Core passes it once to
    `DirectoryModel::setDisplayPipeline()`; `Loader` puts it into every
    `DecodeContext`.
  - **`DecodedPixels`** (`sourcecontainers/`, thread safe): the unedited
    pixels of an `ImageStatic` and their SDR copy. With `Cpu`, an HDR image
    is converted at load as before. With `Gpu`, the HDR source is kept and
    the SDR copy is built on the first `getImage()` / `getSourceImage()`
    (save, edit, copy, print, wallpaper, scaler, upscaler), once even with
    concurrent callers; a failed conversion is reported once. Committed edits
    replace the pixels and drop the HDR source.
  - **`ImageStatic`:** with `Gpu`, no CPU tone mapping and no CPU colour
    management at load; `getDisplayImage()` returns the decoded image (HDR
    stays HDR). The new `Image::getDecodedImage()` gives Core's metadata
    (HDR profile, colour profile in the title and info) the decoded pixels
    without forcing the SDR copy, so the Quick UI now shows the real HDR or
    linear profile; the widget UI is unchanged (`getDecodedImage()` equals
    `getImage()` there). **Fixed in passing:** `commitEdits()` kept the
    colour-managed copy of the pre-edit image, so with colour management
    on, the CPU viewer could show stale pixels after a commit.
  - **Viewer:** `UiSettingsSnapshot` gained the C++-only
    `DisplayColorSettings` (tone mapping on/off, operator, white level,
    colour management and `ColorManager::getTargetColorSpace()`), applied by
    `ImageViewportController` to the render item. A change re-renders from
    the kept source and does not reset the viewer's session overrides.
    `ColorManager`'s cache is now keyed on the profile settings, so the
    Quick host (whose `settingsChanged` slot runs before Core's
    `invalidateCache()`) never reads the previous profile.
  - **Upscale without a CPU scale:** the upscale rules of
    `Core::onScalingFinished()` moved unchanged into the pure
    `decideUpscale()` (`components/upscaler/upscaledecision.*`), used by
    the widget path after a CPU scale and by the new
    `Core::onUpscaleRequested()` for `UiEvents::upscaleRequested`. The
    viewport emits `upscaleRequested(size)` instead of `scalingRequested`,
    and hides the crop itself when Upscayl is turned off.
  - **Copy viewport to clipboard:** `ImageViewportController::
    grabVisibleImage()` reads the render item back with
    `QQuickItem::grabToImage()` (asynchronous; a newer grab replaces a
    pending one) and crops it to every device pixel the image covers;
    `QuickViewerActions` puts it on the clipboard with the widget UI's
    messages. **Deviation:** not a hand-written `QRhi` readback: on D3D12
    and Vulkan that completes only frames later and needs extra frames
    scheduled, while `grabToImage()` is Qt's readback through the scene
    graph. The copy includes the colour adjustments, tone mapping and
    filtering exactly as shown.
  - **Measured** (Release, `ramp.hdr` 1200×800 linear float RGBE, process
    start → first `documentRenderingSettled`): widget UI 475 ms, Quick UI
    358 ms (no CPU tone mapping at load). The window titles show the source
    profile: "sRGB" (tone-mapped CPU copy) vs "Linear sRGB" (decoded source).
  - **Flagged, not changed:** Core still reloads the current image when an
    HDR setting changes. The Quick viewer would not need it (it re-renders
    from the source), but the reload also refreshes the lazily built SDR
    copy, so it is kept.
  - **Not verified live:** the upscale (needs Upscayl models and zoom
    input) and the clipboard copy (would need synthetic key input and
    overwrite the clipboard); both are covered by the tests below.
  - Tests: `wynpix_tests` gained `DecodedPixelsTests` (eager CPU conversion,
    lazy GPU conversion, SDR passthrough, one conversion for eight
    concurrent readers, one failure report, edits drop the HDR source) and
    `UpscaleDecisionTests` (every branch of the former inline rules,
    including the limit boundary). `ImageViewportControllerTests` now checks
    `upscaleRequested`, crop hiding when Upscayl is turned off, the display
    colour settings on the item (and that they keep the session filter), the
    unknown-operator fallback and a grab without a window.
    `wynpix_render_tests` gained `viewportGrabMatchesThePresentedImage` (real
    frame loop, within one 8-bit level; skipped on the Vulkan variant, which
    runs on the windows platform where showing the scene would open a
    window).

#### S2.2 Style and theme
- **Goal:** the Quick UI matches the current look in light and dark schemes.
- **Owner:** custom Qt Quick Controls style module `wynpix.style` (based on
  `Basic`), reading only the `Theme` singleton.
- **Scope:** Button, ToolButton, ComboBox (`highlightOnHover`), Slider,
  SpinBox/DoubleSpinBox, CheckBox, RadioButton, TextField/SearchField,
  ScrollBar, Menu/MenuItem (shortcut display), ToolTip (`policy`), Popup;
  icon font glyph component; derived state colours via the 6.12 `Color`
  singleton; overlay shadows via `RectangularShadow`.
- **Acceptance:** Qt Quick Test snapshot of a control gallery in both
  schemes; theme switch at runtime updates without restart.
- **Delivered:**
  - **Shared metrics and surfaces:** the sizes and dialog colours that
    `Settings::loadStylesheet()` computed inline moved into pure functions
    used by both UIs, like `UiMetrics::typographyFor()`:
    `UiMetrics::controlMetricsFor(font)` (button, top bar and overlay header
    heights, context menu width and row height, rename overlay width, tooltip
    and context menu frame sizes) and `DialogSurfaces::colorsFor(dark)`
    (`gui/dialogsurfaces.h`: dialog window and text colours and the five
    tints, `sys_window_tinted*` in the stylesheet). The stylesheet output is
    unchanged. Later the dialogs moved to the translucent `stroke*` tokens
    and `DialogSurfaces` was removed: the window and card colours are
    `Theme.colors.surfaceWindow` and `surfaceCard`.
  - **Theme bridge:** `Theme.metrics` and `Theme.surfaces` (since merged into
    `Theme.colors`), notified only when they change. The snapshot is built by the pure
    `buildThemeSnapshot(scheme, font, iconFontFamily)`
    (`gui/quick/adapters/themesnapshotbuilder.*`); `BridgeSnapshots::
    readTheme()` calls it with the Settings scheme.
  - **`wynpix.style`** (`gui/quick/style/`, static module, `IMPORTS
    QtQuick.Controls.Basic`): selected at compile time. `Main.qml` imports
    `wynpix.style` instead of `QtQuick.Controls.Basic`; controls not provided
    fall back to Basic. Every control is built on `QtQuick.Templates` and
    reads only `Theme` and the `StyleConstants` singleton (radii, paddings,
    colour factors: the literal values of the stylesheet and ProxyStyle).
    Button (QPushButton), ToolButton (PanelButton), ComboBox and
    ItemDelegate (ProxyStyle's flat panel, chevron glyph, list popup with
    `highlightOnHover`), Slider, SpinBox and DoubleSpinBox, CheckBox and
    RadioButton (ProxyStyle's Fluent glyph indicators), TextField and
    SearchField, ScrollBar and ScrollView, Menu (`separatorsCollapsible`),
    MenuItem, MenuSeparator, ToolTip (default `policy`), Popup, Label.
    Translucent and derived colours use the 6.12 `Color` singleton
    (`transparent`, `lighter`, `blend`). Popups, menus and tooltips share
    `PopupBackground` with a `RectangularShadow`. `IconGlyph` draws one
    `FluentIcons` glyph.
  - **Menu shortcuts:** Qt's `MenuItemIconLabel` draws the shortcut in the
    label colour; the widget menu dims it. `MenuItem` draws it itself in the
    secondary text colour, formatted by the `ShortcutText` singleton
    (`Action.shortcut` as native text, like QMenu).
  - **Deviations:** SpinBox and DoubleSpinBox keep chevron step buttons
    (the crop panel hides them, the settings dialog has them). SearchField
    has no search icon (the widget name filter has none; the icon font has
    no magnifier glyph). Buttons, panel buttons and combo boxes show a 1 px
    accent border on keyboard focus (ProxyStyle hides focus frames).
  - **Measured** (Release, `smoke.png` 300×300, process start → first
    `documentRenderingSettled`, 5 runs each, same session): Quick UI median
    ≈ 380 ms (319–401), widget UI median ≈ 738 ms (719–795). The machine
    was loaded during the measurement (widget UI ≈ 510 ms in S2.1); the
    Quick/widget ratio is better than in S2.1. No style control is
    instantiated before the first frame.
  - **Flagged, not changed:** the dark and light `QPalette` literals in
    `Settings::loadStylesheet()` stay inline; Auto theme mode does not
    follow an OS dark/light switch at runtime (nothing listens to
    `QStyleHints::colorSchemeChanged`, in either UI).
  - Tests: `wynpix_tests` gained `UiMetricsTests` (dark and light tints
    against the former stylesheet values, designed sizes for small fonts,
    scaling for large fonts, fixed frame sizes, the theme snapshot of both
    schemes). `wynpix_qml_tests` now builds the fixture theme from
    `ThemeStore`'s real schemes and loads Segoe UI and the icon font (the
    offscreen platform finds no fonts). `tst_style.qml` checks, in both
    schemes, the colours of every control's flat parts in its states
    (normal, hovered, pressed, checked, focused, disabled), glyph
    indicators, spin stepping, the clear button, the minimum scroll handle,
    the menu shortcut text and highlight, and a runtime theme switch of live
    controls. It writes `style-dark.png` and `style-light.png` (gallery with
    a menu, a tooltip and a popup open) next to the test executable for
    review; they are not compared with reference images (text rendering
    differs between machines).

#### S2.3 Overlays and floating messages (parallelizable after S2.2)
- **Goal:** port the 19 overlays as GPU-composited items.
- **Owner:** each overlay is a QML component; state comes from existing
  `Core`/`MW` signals through the ports; any logic currently inside overlay
  `.cpp` files (e.g. rename validation, copy/move target lists) moves to a
  small C++ model first.
- **Scope:** floating message, zoom indicator, image info (exif model),
  fullscreen info bar, controls overlay, click zones, copy/move, rename,
  save confirm, colour adjustments (`DoubleSpinBox`, live preview through
  `ImageRenderItem`), CAS settings, crop overlay (handles via `Shape`),
  draggable slider. Fade animations via `Behavior on opacity` and
  `Loader { asynchronous: true }` for rarely used overlays (replaces the
  `*Proxy` classes).
- **Acceptance:** every overlay reachable from its action; keyboard focus
  rules match (`acceptKeyboardFocus`); no overlay is instantiated at
  startup unless enabled by settings.

- **Scope decision (user, 2026-10-09):** the zoom indicator and the click
  zones were already ported in S1.6; the map overlay stays in S3.4; the crop
  overlay moves to S2.5 with the crop panel (it is reachable only through the
  panel, and its selection logic needs its own C++ model first).
- **Delivered:**
  - **Pure components** (`wynpix_viewcomponents`):
    `components/shellinfo/fileinfotext.*` (`filePositionText`,
    `imageResolutionText`, `fileSizeText`, `fullscreenInfoFor`; the window
    title uses the same helpers) and `components/copytargets/copytargetlist.*`
    (`copyTargetsFrom`: the default destinations of `CopyOverlay`, the
    visible writable home folders up to `kMaxCopyTargets`;
    `savableCopyTargets`: without empty and repeated entries).
  - **Overlay models** (`gui/quick/ui/overlays/`, module `wynpix.ui`), all
    Settings-free (values in, edit signals out):
    `OverlayState` (open, `created` latch for loading on first use,
    `takesKeyboardFocus`, anchor); `NotificationOverlayModel` (the Quick
    UI's `INotificationPort`, `notificationPresentationFor`: icon, default
    text and display time by kind); `ImageInfoModel` (name, value, stacked
    for values over 100 characters); `AdjustmentSliderModel` (slider rows,
    `sliderValueText`) with `ColorAdjustmentsEditor` (live preview, compare,
    apply-and-reset) and `CasSettingsEditor` (`CasParameters`);
    `CopyTargetsModel` (copy / move mode, digit shortcuts,
    `activateShortcut(keyText)`, folder replacement publishes the list to
    save); `RenamePromptController` (base-name selection, non-empty accept,
    pass-through shortcuts of exit and rename, backdrop in folder view);
    `FullscreenChromeController` (info bar while the setting is on, controls
    when the panel is off, at the bottom or left; `kHideTimeoutMs` auto-hide
    on pointer moves, controls kept while hovered; the info bar toggle
    publishes the setting).
  - **`OverlayCoordinator`** owns the states and models and holds `MW`'s
    rules across overlays: entering the folder view closes copy / move,
    rename, colour adjustments and CAS settings and hides the image info,
    which returns with the document view; copy / move needs a displayed
    image; the save confirmation follows `showSaveOverlay`; save
    confirmation and copy list move to the top when the panel would cover
    them; colour adjustments and CAS settings open at the last pointer
    position; `keyboardOverlayOpen` moves the focus between the viewer and
    the copy list or rename prompt. It keeps the current CAS parameters, so
    a settings snapshot with unchanged values does not undo an overlay edit.
  - **Application side:** `QuickOverlayActions` (adapter) runs the copy,
    move, image info, colour adjustments and CAS actions, fills the copy
    destinations on first use (never at startup) and stores their edits,
    stores CAS edits and applies them through the new narrow
    `ImageViewportController::setCasParameters()` (a settings notification
    would reset the session scaling filter), stores the info bar toggle and
    keeps the rename pass-through shortcuts current.
    `QuickMainWindowController` forwards the shell port's metadata, save
    overlay, info bar, rename and current-file calls, the window's pointer
    moves, the fullscreen state and the view mode to the coordinator.
    `QuickUiHost` replaces `LoggingNotificationPort` with the message model,
    forwards the overlays' requests to `UiEvents` and hands the coordinator
    to `Main.qml`. `ActionBridge` gained `shortcutText()` and `keyText()`
    (through `IActionDispatcher`) for the rename and copy keys.
  - **QML:** `OverlayLayer` loads each overlay asynchronously on first use
    (`created`) and keeps it; the fullscreen chrome is loaded while active.
    Files are named like the widget classes (`FloatingMessage`,
    `FullscreenInfoOverlay`, `ControlsOverlay`, `ImageInfoOverlay`,
    `SaveConfirmOverlay`, `CopyOverlay`, `RenameOverlay`,
    `ColorAdjustmentsOverlay`, `CasSettingsOverlay`), so `qsTr()` uses the
    widget translation contexts. Shared parts: `OverlayPanel` (surface,
    header, close button, header drag inside the window, input blocking),
    `OverlayHeaderButton`, `AdjustmentSliders` (double click resets a
    slider). Fades (message, info bar, controls) are a `hidden` state with a
    fade-out transition: they appear at once, as in the widget UI. The copy
    list's folder picker is `QtQuick.Dialogs.FolderDialog`.
  - **Deviations:** the colour adjustment preview is not throttled (the GPU
    applies it per frame). One floating message serves both the document and
    the folder page (the widget UI has one per page). The fullscreen chrome
    re-shows only when its own settings change, not on every settings
    notification.
  - **Measured** (Release, `smoke.png` 300 x 300, process start to first
    `documentRenderingSettled`, 5 runs each, same session): Quick UI median
    346 ms (337 - 359), widget UI median 550 ms (537 - 552). No overlay is
    created before the first frame.
  - **Flagged, not changed:** `MW` still holds its own copies of the
    notification presentation, info bar texts and overlay rules (removed
    with the widget UI in S4.2); `wynpix.style/Popup.qml` has an unused
    `wynpix.bridges` import (qmllint info). Not verified by hand in the
    running application: the overlays were exercised by the Quick tests
    with the real modules; the application smoke test covers startup only.
  - Tests: `wynpix_tests` gained `OverlayTests` (info bar texts, copy
    targets with a temporary home folder, notification presentation and
    timing, slider texts, colour and CAS editors, copy targets, rename,
    fullscreen chrome timers, coordinator rules). `wynpix_qml_tests` gained
    `tst_overlays.qml` (nothing created at startup, each overlay reachable
    from its request, message fade, image info rows, folder view, copy focus
    and digit keys, row click, rename Enter / Escape / empty name, colour
    adjustments at the pointer, CAS, save confirmation setting, fullscreen
    controls auto-hide); the fixture provides an `OverlayCoordinator` and
    request hooks.

#### S2.4 Directory view adapter and thumbnail strip
- **Goal:** the bottom/side thumbnail panel in QML.
- **Owner:** `DirectoryViewAdapter : QAbstractListModel, IDirectoryView`
  (C++); `DirectoryPresenter` is reused unchanged.
- **Scope:** model roles (path, name, isDir, thumbnail, pending,
  unavailable, selected, dragHover); visible-range reporting from
  `ListView` drives `thumbnailsRequested` (same contract as
  `ThumbnailView::loadVisibleThumbnails`); `ThumbnailItem` (`QQuickItem`)
  creates its texture with `QQuickWindow::createTextureFromImage` in
  `updatePaintNode` from the `Thumbnail` held by the model (no image
  provider, no URL round trip); panel pin/auto-hide, position
  (top/bottom/left/right), styles.
- **Acceptance:** scrolling 10 000 items stays at display refresh rate
  with no GUI-thread decode; `visibleThumbnailsReady` semantics preserved
  for cold start.
- **Delivered:**
  - **Model** (`gui/quick/ui/thumbnails/`, module `wynpix.ui`, free of
    `Settings` and `Thumbnail`): `ThumbnailListModel` (`QAbstractListModel`)
    holds the item state (roles `name`, `info`, `isDir`, `thumbnail`,
    `loaded`, `pending`, `unavailable`, `selected`, `dragHover`) and the
    behaviour of `ThumbnailView` / `ThumbnailStrip`: the view reports its
    scroll offset and extent (`setViewport()`), and every item within
    `kPreloadDistance` (3000 px) that is not loaded, requested or
    unavailable is requested in one batch, nearest first in the scroll
    direction (`thumbnailsNeeded`, the `thumbnailsRequested` contract). It
    also unloads thumbnails outside that range (`unloadThumbs`), emits
    `visibleThumbnailsReady` once per population with the widget rules,
    shifts the cached state on insert and remove, and requests nothing while
    the view is inactive, the panel slides or a scroll animation runs.
    Scrolling is decided there as well: focus centred or kept in view with
    half a cell of margin; the wheel smooth with accumulation and
    acceleration, by item, or by touchpad pixels (the touchpad heuristic);
    the right-button gesture to an end. The pointer rules live there too:
    activation on press, Ctrl toggles, the release selects within a
    multi-selection, drag out after 40 px, back / forward, double click. The
    view runs the scroll animations the model sends (`scrollRequested`).
    `thumbnailStripLayoutFor()` gives the cell and panel geometry of
    `ThumbnailWidget` / `MainPanel::sizeHint()` (`ThumbnailStripLayout`).
  - **`ThumbnailPanelController`** (`wynpix.ui`) ports the pin and auto-hide
    rules of `DocumentWidget` / `SlidePanel`:
    - docked while pinned in a window of at least 800 x 500;
    - floating panels slide in when the pointer enters their area with no
      button held (fullscreen only by setting);
    - floating panels slide out after `panelHideDelayMs` outside the area
      grown by 8 px, or after at least 600 ms when the pointer left the
      window or the window was deactivated;
    - a press in the area keeps the panel hidden until the pointer leaves it;
    - the folder view and window state changes hide the panel at once;
    - the exit button is shown in fullscreen at the top or right.

    The pin button publishes `pinRequested`, which the host stores. The
    controller configures the model: request size `dpr * thumbnailSize`
    (the application's device pixel ratio, like the widget strip), uncropped
    previews, unloading, and the scroll settings.
  - **`ThumbnailItem`** (`wynpix.render`): creates its texture from the
    decoded `QImage` with `QQuickWindow::createTextureFromImage()` in
    `updatePaintNode()`, and only again when `QImage::cacheKey()` changes.
    `ThumbnailMaterial` (`thumbnail.vert` / `.frag`, BATCHABLE for the scene
    graph's batching renderer) draws the 4 px rounded corners with a
    one-device-pixel antialiased edge and the hover highlight (the widget's
    Plus composition at 0.2). `thumbnailDrawSize()` is the size rule of
    `ThumbnailWidget::updateThumbnailDrawPosition()`. On the software
    backend it falls back to a `QSGImageNode`.
  - **Application side:** `DirectoryViewAdapter : ThumbnailListModel,
    IDirectoryView` (`gui/quick/adapters/`) replaces
    `PlaceholderDirectoryView` for the panel (the folder view keeps it until
    S3.1). It hands thumbnails over as `Thumbnail::image()`, a new accessor
    that returns the decoded image without the `QPixmap` conversion of
    `pixmap()`. `QuickUiHost` owns the adapter and the controller, stores
    the pin state and allows the panel content once the first document
    rendering settled. `QuickMainWindowController` forwards pointer moves,
    leave, deactivation, size, fullscreen and view mode to the controller.
    `PanelSettings` gained `unloadThumbnails` and `thumbnailResolution`.
  - **QML:** `MainPanel` (surface, border, slide by 40 px with fade over
    300 ms, OutCubic in and InCubic out, buttons as in the widget
    `ButtonSmall`), `ThumbnailStrip` (`ListView` with `reuseItems`, not
    interactive; one `MouseArea` reports the pointer and the item under it;
    the style's `ScrollBar` with the selection marker), `ThumbnailWidget`
    (cell). `Main.qml` lays the viewer and the panel out in the document
    page: a pinned panel takes its space from the viewer, a floating one
    covers it. The panel surface (and with it the docked space) exists from
    the first frame. The strip is loaded asynchronously only after the first
    document rendering settled, so it adds nothing to startup.
  - **Cold start:** `visibleThumbnailsReady` of the strip keeps the widget
    semantics and is, like the widget strip's, not connected to `UiEvents`.
    Only the folder view takes part in the cold-start reveal; the model is
    ready for S3.1.
  - **Deviations:**
    - No `path` role: `IDirectoryView` is index based and never carries
      paths; name and info come from the thumbnail, as in the widget cell.
    - Long labels elide instead of fading out.
    - Unavailable thumbnails show the error glyph; the widget keeps the
      loading glyph.
    - Shift-click does nothing, as in the widget strip, which never has the
      keyboard focus and so no range anchor.
    - No rubber band: the cells fill the strip.
    - A Qt Quick double click delivers a second press, so the item is
      activated once more than in the widget UI; activating the shown image
      again changes nothing.
  - **Measured** (Release, `smoke.png` 300 x 300, process start to first
    `documentRenderingSettled`, 5 runs each, same session, floating panel
    at the bottom): Quick UI median 343 ms (335 - 385), widget UI median
    528 ms (522 - 547). Pinned panels at all four positions and both styles
    were checked in window captures of the running application, against the
    widget UI with the same settings.
  - **Flagged, not changed:**
    - `Core` does not connect the panel presenter's `draggedOut`, so
      dragging out of the strip does nothing in either UI.
    - Neither UI requests thumbnails again for a new device pixel ratio
      when the window moves to another screen.
    - `ThumbnailStripProxy` and `MainPanel` stay until S4.2.
  - **Not verified by hand in the running application:** hover, wheel
    scrolling and auto-hide, since no input was sent to the desktop. The
    tests drive them. Frame rates while scrolling 10 000 items were not
    measured: the offscreen tests check that delegates and requests stay
    bounded and that `ThumbnailItem` only uploads decoded images.
  - Tests:
    - `wynpix_tests` gained `ThumbnailStripTests`: layout and draw size,
      preload range, direction, blocking, readiness with final pending
      images, insert and remove shifting, unloading, configuration changes,
      reload, focus, smooth / by-item / touchpad wheel, pointer rules, panel
      docking, hover show and hide, the press guard, the window-exit grace,
      fullscreen-only and the exit button, pinning, model configuration, and
      the adapter.
    - `wynpix_qml_tests` gained `tst_thumbnailstrip.qml`: content only after
      the creation permission, docking at the four sides, a 10 000-item
      directory with bounded delegates and requests while focusing and
      wheel scrolling, delivered thumbnails drawn, the extended labels,
      activation by press, the pin button, and the floating slide.
      `tst_mainwindow.qml` checks the docked viewport.
    - `wynpix_render_tests` (D3D11, D3D12, Vulkan) gained the
      `ThumbnailItem` corner and highlight pixel tests, checked against a
      mutated shader.

#### S2.5 Context menu and side/crop panel
- **Goal:** menus and the crop side panel.
- **Owner:** menu structure defined in C++ (`ContextMenuModel` built from
  `ActionManager`), presentation in QML.
- **Scope:** `Menu { popupType: Popup.Native }` with fallback to
  `Popup.Window`; `Menu.separatorsCollapsible`; path selector submenu;
  crop panel with numeric inputs and aspect presets; crop overlay (moved
  from S2.3): selection logic of `CropOverlay` in a C++ model first, handles
  via `Shape`.
- **Acceptance:** menu opens beyond window edges; all shortcuts displayed
  match `ActionManager`.
- **Delivered:**
  - **Menu model** (`gui/quick/ui/menus/`, module `wynpix.ui`, free of
    `Settings` and `ActionManager`): `ContextMenuModel` holds the rows of the
    widget `ContextMenu` in four `ContextMenuEntryList` models (zoom buttons,
    transform buttons, action rows, scripts; roles prefixed `entry*` so they
    do not shadow delegate properties). Shortcuts come from
    `IActionDispatcher::shortcutFor()`, so they are the ones `ActionManager`
    uses; `refreshShortcuts()` re-reads them on `settingsChanged` and updates
    the rows in place (`dataChanged`, no reset). Rules: rows acting on the
    image are disabled without one; CAS settings is listed only with an image
    and the CAS filter; "More" expands in place and collapses whenever the
    menu opens; the menu action toggles the menu; it does not open while the
    viewer takes no input (crop mode), and the folder view closes it. Texts
    keep the widget's `ContextMenu` translation context.
  - **Menu QML:** `ViewerContextMenu.qml` (not `ContextMenu.qml`: that name
    is the Qt Quick Controls `ContextMenu` attached type) is a `Menu` with
    `popupType: Popup.Window`, opening at the pointer through `popup()`; the
    user chose a window menu with widget parity over `Popup.Native`, which
    cannot show the button rows, the destructive tones or the theme. Button
    rows act on press; "More" is not a `MenuItem` (a triggered item closes
    the menu); "Open with..." is a cascading submenu with the scripts and
    "Configure menu". Rows are laid out in a `Column` over `contentModel`:
    the style's `ListView` estimates rows outside its view, which left the
    window too short once "More" expanded. The menu is created on its first
    opening. The style `MenuItem` gained `shortcutText`, a glyph icon
    (`hasGlyph`, `glyph`, `glyphSize`, `glyphColor`) and `labelColor`.
  - **Crop model** (`gui/quick/ui/crop/`): `CropSelection` is the selection
    logic of `CropOverlay` in image pixels (bounded selection, free and
    ratio-locked resize from the opposite anchor, flips across the anchor,
    move, new selections taking their direction from the first travel along
    both axes). A drag applies the whole travel since it began to the
    selection it started from, so fractional pointer moves accumulate
    (the widget applied each move's rounded delta). `CropController` adds
    the mode and the panel: opens in the document view with an image, closes
    in the folder view or without an image; opening and new images reset to
    the free ratio with the whole image; the eight presets of `CropPanel`;
    edited inputs are moved into the image; crop / crop and save close the
    mode and request only a selection that is not empty and not the image
    size; the default action follows the settings and the right click; the
    pointer maps through the image area on screen; handles are drawn while
    nothing is dragged and the selection is at least 90 device pixels.
  - **Crop QML:** `CropOverlay.qml` (tint bands, outline and handles as
    rectangles, not `Shape`: all axis aligned, no extra module to deploy),
    `CropPanel.qml` (inputs in a grid, presets, swap, the default action
    marker of `PushButtonFocusInd`), `SidePanel.qml` (surface at the right
    edge that takes its space from the document page, takes the wheel and
    focuses the width input). Enter, Shift+Enter, Escape and Ctrl+A are
    `Shortcut`s of the crop mode; the focused input is left first, so its
    edit applies.
  - **Viewer and panel:** `ImageViewportController` gained `imageArea()`,
    `imageSize()`, `imageGeometryChanged()`, `setInteractionEnabled()`
    (zoom, scroll and fit actions and presses, double clicks, wheel and pinch
    do nothing while off) and `setExpandSmallImagesInFitMode()`.
    `ThumbnailPanelController::setInteractionEnabled()` hides an unpinned
    panel and keeps it from sliding in, as `DocumentWidget` does.
  - **Application side:** `QuickContextMenuActions` (menu action, scripts
    with a command, displayed image, CAS filter) and `QuickCropActions`
    (`QuickCropContext`; the viewer side effects of `MW::showCropPanel()` /
    `hideCropPanel()`, crop requests to `UiEvents`, the default action to
    `Settings`). `QuickMainWindowController` implements the crop parts of
    `IShellPort` (with the size of the window's screen for the screen preset)
    and passes the view mode to the menu and the crop mode. `QuickUiHost`
    takes the `ScriptManager`.
  - **Path selector submenu:** delivered with the copy / move overlay in
    S2.3 (`CopyTargetsModel`, `FolderDialog`).
  - **Deviations:**
    - While the menu is open, keys navigate it; the widget menu passed them
      to the action shortcuts.
    - Enter with an empty selection closes the crop mode (the widget overlay
      ignored it, the panel closed).
    - "Configure menu" logs that the script settings are not available
      until the Quick settings dialog (S3.3).
  - **Measured** (Release, `smoke.png` 300 x 300, process start to first
    `documentRenderingSettled`, 5 runs each, same session): Quick UI median
    355 ms (348 - 386), widget UI median 559 ms (534 - 599). The menu and
    the crop panel are created on first use.
  - **Not verified by hand in the running application:** opening the menu
    and the crop mode, since no input was sent to the desktop; the tests
    drive them and the offscreen renders (`contextmenu.png`, `crop.png`
    next to `wynpix_qml_tests`) were reviewed. Smoke runs start and close the
    Quick UI without QML warnings.
  - **Flagged, not changed:** the `ThumbnailStrip.qml` scroll bar reports
    TypeErrors (anchors on a null parent) while QML tests tear their windows
    down (S2.4).
  - Tests:
    - `wynpix_tests` gained `ContextMenuTests` (rows, every menu action is an
      application action, shortcut parity and refresh in place, image rows,
      CAS, More, open rules, triggering, scripts) and `CropTests`
      (selection: fit, place, free / locked corner and edge resize, flips,
      move, accumulated travel, new selections; controller: mode, presets,
      custom ratio and swap, inputs, crop validity, default action, pointer
      mapping, handles and cursors), plus the viewport interaction lock, the
      small-image enlargement and the panel gate.
    - `wynpix_qml_tests` gained `tst_contextmenu.qml` (own window larger than
      the test window, shortcuts equal `Actions.shortcutFor()` and follow
      edits, rows, buttons on press, More, disabled image rows, the scripts
      submenu, Escape and the folder view) and `tst_crop.qml` (opening with
      the panel, drawing, inputs, presets, Enter / Shift+Enter / Escape, the
      default action, the wheel, the folder view); `tst_mainwindow.qml`
      checks the docked side panel.

#### S2.6 SVG display path
- **Goal:** sharp SVG at any zoom on the GPU.
- **Owner:** `ImageViewport` chooses the layer; `Loader` decides the
  document type as today.
- **Scope:** default: re-rasterize the SVG with `QSvgRenderer` on a worker
  thread at the settled scale and display through `ImageRenderItem`
  (filters keep working). Evaluate `VectorImage { asynchronous: true;
  preferredRendererType: VectorImage.CurveRenderer }` as an opt-in fast path
  for simple SVGs, switching on `status`.
- **Acceptance:** SVG parity with `QGraphicsSvgItem` rendering; no GUI
  thread stall on complex SVGs.
- **Delivered:**
  - **`SvgRasterizer`** (`components/svgrasterizer/`, in
    `wynpix_viewcomponents`, now linked with `Qt6::Svg`; no UI): `open()`
    reads and parses the document on a private single-thread `QThreadPool`
    and reports `documentReady(defaultSize)` / `documentFailed()`;
    `request()` renders a `SvgRasterRequest` (a rect of the decoded image,
    in image pixels, into a target size) with the same mapping as the Qt SVG
    image plugin's decode, so the raster lines up with the decoded image.
    One task runs at a time and a newer request replaces a queued one; every
    task carries its identity (document generation, request id), which the
    completion callback validates before touching state, and only the
    latest request of the current document is reported. `cancelRequests()`
    and `close()` drop results without waiting; a running task finishes in
    the background (the destructor waits for it). The parsed
    `QSvgRenderer` (animation off; animated SVG stays on the
    `AnimationPlayer` path) lives only on the worker and is detached from
    its thread, so it may be destroyed from either side.
  - **Renderer:** `CropComposition { Over, Replace }` on the crop layer
    (`ImageRenderItem::setUpscaledCrop(..., composition)`,
    `cropComposition()`, `cropSourceRect()`). With `Replace` the image is not
    drawn while the crop is, so transparent SVG pixels do not show the
    blurred image under them; the crop keeps the image's filtering, colour
    adjustments, colour management and checkerboard. Upscayl crops stay
    `Over`.
  - **Viewer** (`ImageViewportController`, the owner of the layer choice):
    a `.svg` path (the widget viewer's rule) opens the document next to the
    decoded image. Once the view settles at any scale other than 1:1, the
    visible rect is requested at the displayed device-pixel size; the
    settled frame (and `renderingSettled()`) is the one showing the raster,
    or the decoded image if rasterizing fails. Any transform change drops
    the raster and the request in flight. The first settle of a document
    does not wait for parsing, so the first frame and the cold-start reveal
    are unchanged; the raster follows with a second settle. SVG documents
    are never AI upscaled, Core's upscaled crops are ignored for them and
    `hideUpscaledCrop()` no longer removes the SVG raster. An image whose
    size no longer matches the document (a 90 degree rotation edit) is
    shown as decoded.
  - **`VectorImage` (CurveRenderer) evaluated, not adopted:** it draws the
    document as its own scene-graph item outside `ImageRenderItem`, so the
    scaling filters, colour adjustments, display colour management, the
    transparency checkerboard, panorama mode and viewport grabs of the image
    would not apply, and a second placement path would have to follow
    `ViewTransform`. That contradicts the single GPU path (section 2,
    principle 3) for a speed-up that only simple documents get; the worker
    raster already keeps the GUI thread free. Revisit only if re-raster
    latency on very large zooms becomes a complaint.
  - **Deviations:**
    - During zoom, pan and resize the decoded image is shown scaled until
      the view settles (the widget item repainted the vector every frame,
      on the GUI thread).
    - The raster is shown at `round(rect size * scale)`, so its net scale
      can differ from 1 by less than one pixel over the raster; for very
      small rasters the GPU filter then resamples it slightly.
    - Like the widget viewer: `.svgz` is not rasterized again, and mirror
      or 180 degree edits of an SVG (same size) show the unedited document
      once settled.
  - **Measured** (Release, 4000 x 3000 SVG with 60000 shapes, process start
    to first `documentRenderingSettled`, one run each): Quick UI 2695 ms
    (first frame at 2216 ms, decoded image; the raster settles about
    480 ms later on the worker), widget UI 2954 ms. Both are dominated by
    the Loader's decode of the document, which is unchanged. A 160 x 120
    icon enlarged in fit mode renders sharp.
  - **Flagged, not changed:** the document is parsed twice (Loader decode
    and `SvgRasterizer`); `ImageStatic::loadGeneric()` still manages its
    `QImage` with `new` / `delete`.
  - Tests:
    - `wynpix_tests` gained `SvgRasterizerTests` (file / size / request
      rules, a partial raster equals the matching part of a whole-document
      raster and differs from a stretched decode, only the latest request
      is reported, cancel and close, reopening, broken and missing files)
      and viewport tests (raster replaces the image when settled with the
      visible rect, Core's crop calls do not touch it, dropped on zoom and
      requested again, none at 1:1, no upscale requests, edited images).
    - `wynpix_render_tests` gained `replacingCropHidesTheImage` (`Over`
      blends, `Replace` hides the image also under transparent crop pixels;
      mutation-checked) and `viewportShowsTheSvgRasterAtTheDisplayedSize`
      (the presented frame matches `QSvgRenderer` at the displayed size
      within 2 levels, settles on that frame, and the scaled decode of a
      non-SVG path does not match); passes on D3D11, D3D12 and Vulkan.

### Phase 3: Folder view and dialogs

#### S3.1 Folder view
- **Goal:** the folder browser in QML.
- **Owner:** second `DirectoryViewAdapter` instance for the grid;
  `FileSystemModelCustom` reused as the `TreeView` model; bookmarks as a
  `QRangeModel` with drag-and-drop (6.12).
- **Scope:** `GridView` with rubber-band selection, range/toggle selection,
  keyboard navigation and type-ahead (forwarded to the presenter), drag-out
  (`Drag.mimeData` with `text/uri-list` built by `MimePayloadManager`),
  drop-into; top bar: path, sort `ComboBox`, format filter, name filter
  `SearchField`, icon size `Slider`; splitter (`SplitView`).
- **Acceptance:** feature parity checklist for `FolderView`,
  `FolderGridView`, `BookmarksWidget`; thumbnail cache and cold start
  readiness unchanged.
- **Delivered** (two commits: the `ThumbnailListModel` refactor, then the
  folder view):
  - **Refactor:** `ThumbnailListModel` lays items out in rows
    (`ThumbnailScrollConfig::columns`, `leadingSpace`, `preloadDistance`,
    `focusShowsNeighbours`), selects on press and activates by double click
    (`ItemActivation::OnDoubleClick`) and keeps a Shift range anchor
    (`beginRangeSelection()` / `selectRangeTo()`, the rule of
    `ThumbnailView::addSelectionRange()`). The strip keeps its behaviour
    with the defaults (one column, 3000 px, activation on press).
  - **Grid** (`gui/quick/ui/folderview/`, module `wynpix.ui`, free of
    `Settings`): `folderGridLayoutFor()` gives the geometry of
    `FolderGridView::gridGeometry()` and the labelled `ThumbnailWidget`
    (`FolderGridLayout`, its cell as a `ThumbnailStripLayout`).
    `FolderGridController` configures the model (icon size times the device
    pixel ratio, no crop, unloading, 2300 px preload) and owns the input
    rules of `FolderGridView`: arrows with the column memory of Up / Down,
    Page Up / Down by four rows, Home / End, Shift ranges, Ctrl+A keeping
    the current item, Enter, Backspace ("goUp"), type-ahead for printable
    text; the rubber band over the background (Ctrl toggles against the
    selection at its start); the context menu on a right click that was no
    scroll gesture; Ctrl + wheel zoom by 16 px within 128 - 512 and the log2
    slider snapping to 256, stored on release; copy / move drops with their
    target; the selected image count for the batch button.
  - **`FolderViewController`** (`wynpix.ui`): the top bar state (path, file
    and folder sorting, name filter published 150 ms after the last edit),
    the places panel (shown while enabled in a view of at least 600 px,
    splitter width, collapsible sections; the slider hides below 510 px),
    the folder tree (a `QFileSystemModel` the host provides on the first
    activation; it follows the grid's directory, lists the folders above
    it and asks the view to scroll to it), drops onto tree folders and
    bookmarks, and the readiness for the cold-start reveal.
    `BookmarksModel` is a `QRangeModelAdapter` over the bookmarks (add once,
    remove, up / down, drag move, the current folder highlighted, the home
    folder in an empty list). `FormatFilterModel` ports the rules of
    `FormatFilterComboBox` over `allFormatCategories()` (`formatgroups.cpp`
    moved to `wynpix_viewcomponents`, used by both UIs).
  - **Application side:** a second `DirectoryViewAdapter` replaces
    `PlaceholderDirectoryView` (removed) as Core's folder view.
    `QuickFolderViewActions` creates `FileSystemModelCustom` on the first
    activation, sends sorting, filters, folder selections, drops, batch
    conversion and both readiness signals to `UiEvents`, turns the grid's
    requests into the directory view's signals (type-ahead, drag hover,
    drops, open selected) and stores the icon size, the panel layout and the
    bookmarks. `QuickMainWindowController` implements the folder parts of
    `IShellPort` (path, sorting indicators, tree refresh) and drives the
    view's active and fullscreen state. `FolderViewSettings` gained
    `folderSortingMode`, `formatFilter` and `bookmarks`.
  - **QML:** `FolderView` (top bar, `SplitView`), `FolderGridView`
    (`GridView` with `reuseItems`, one `MouseArea`, the rubber band, the
    context menu as `Popup.Window`), `PlacesPanel` (bookmark rows, a
    `TreeView` over the name column), `FormatFilterComboBox` (categorized
    popup window). `ThumbnailWidget` draws the cells of both the strip and
    the grid (cell layout and surface colours are set by the view). `Main`
    creates the folder view with an asynchronous `Loader` on its first
    activation; documents opened at startup do not create it. The
    window-wide drop area lies below the page, so the folder view's drop
    areas take their drops. The view keeps the translation contexts
    `FolderView`, `FolderGridView` and `FormatFilterComboBox`.
  - **Drag out:** the grid's drag out reaches `Core::onDraggedOut()`, which
    already builds the payload with `MimePayloadManager` and runs the
    `QDrag`; no QML `Drag.mimeData` is needed (a deviation from the scope
    above).
  - **Cold start:** the grid reports `visibleThumbnailsReady` with the
    strip's rules once its view is active; the tree is ready once the view
    is laid out and the parent of the directory was listed (or the panel or
    the tree is hidden). Like the widget view, which exists only once the
    window is shown, the Quick view is never ready before it is laid out;
    otherwise the ready signal arrived before the cold-start controller
    waited for it and the reveal fell back to its 2000 ms timeout.
    `ColdStartWindowController::revealWindow()` logs the startup milestone
    "cold-start window revealed".
  - **Measured** (Release, a folder of 60 PNG images and 3 subfolders
    opened in folder view, 1280 x 800 window, process start to "cold-start
    window revealed", 5 runs each, same session): Quick UI median 587 ms
    (569 - 598), widget UI median 725 ms (715 - 772); with the places panel
    hidden the Quick UI took 584 ms. Listing the folders above the current
    one costs about 175 ms before the first frame (412 ms when only the
    parent is listed); it is kept so that the tree is complete when shown,
    as in the widget UI. Window captures of the running application matched
    the widget UI (grid, labels, top bar, bookmarks, expanded tree).
  - **Deviations:**
    - Cells are rounded to whole pixels (the widget grid used fractional
      row heights for icon sizes that are not multiples of four).
    - Zooming keeps the current item in view without the widget's extra
      40 px margin.
    - Dragging a bookmark down puts it in front of the row under the drop
      point; the widget moved it one row further.
    - A root folder bookmark is named by its path (the widget showed an
      empty name).
    - The tree keeps other branches expanded when it follows the grid; the
      widget collapsed all when the previous folder's parent was collapsed.
    - A Qt Quick double click delivers a second press, as in the strip; it
      only selects the item again.
  - **Flagged, not changed:**
    - `FileSystemModelCustom` reads the global `Settings` and produces a
      `QPixmap` decoration the Quick tree does not use; it lives in
      `gui/folderview/`, so S4.2 has to move it instead of deleting it.
    - `BookmarksWidget::readSettings()` re-adds every bookmark on each
      settings change.
    - `ThumbnailStrip.qml` logs "Cannot read property ... of null" for its
      scroll bar anchors when the strip is destroyed in the QML tests
      (already before this stage).
  - **Not verified by hand in the running application:** pointer, keyboard,
    drag and drop and the popups, since no input was sent to the desktop.
    The tests drive them.
  - Tests:
    - `wynpix_tests` gained `FolderViewTests` (grid layout and centring,
      model configuration, press and double click, arrows with the column
      memory, page keys, Shift ranges and Shift-click, Ctrl+A, Enter /
      Backspace / type-ahead, the rubber band with and without Ctrl, the
      context menu and the gesture, Ctrl + wheel zoom, the slider snap and
      storing on release, drops and drag hover, the selected image count,
      the format filter rules, the bookmarks, the places panel rules,
      settings, the name filter delay, sorting requests, the first
      activation, readiness with and without the tree, listing of the
      folders above, drops onto bookmarks and tree folders, home and the
      bookmark dialog folder) and row tests in `ThumbnailStripTests`.
    - `wynpix_qml_tests` gained `tst_folderview.qml` (bounded delegates and
      requests in a 10 000-item directory, double click, keys and
      type-ahead, the rubber band, the context menu request, a drop with its
      target, the name filter, the compact top bar, the places panel);
      `tst_mainwindow.qml` checks that the folder view is created on its
      first activation.

#### S3.2 Small dialogs
- **Goal:** resize, file replace, rename, shortcut creator, script editor.
- **Owner:** each dialog gets a C++ view-model (`QObject`) holding
  validation and result building; QML is presentation only.
- **Scope:** `Dialog` + `DialogButtonBox.defaultButton`; native
  `FileDialog`/`FolderDialog`/`MessageDialog` from `QtQuick.Dialogs` for
  save paths and confirmations.
- **Acceptance:** `IDialogPort` fully implemented by the Quick UI.
- **Delivered** (one commit):
  - **Re-scoped:** the shortcut creator and the script editor moved to
    S3.3, since `SettingsDialog` is their only caller. The batch converter
    and printing stay declined until S3.4 (`QuickDialogPort` forwards them
    to `DecliningDialogPort`). Rename was already ported in S2.3
    (`RenameOverlay`); the text prompt of the port (Core's "Add folder") is
    the `TextInputDialog` here.
  - **View-models** (`gui/quick/ui/dialogs/`, module `wynpix.ui`, free of
    `Settings`): `DialogSession` is the base of every dialog (`open`,
    `created` for creation on first use, `finished`; one request at a time,
    a request while open is declined with a warning; the request is
    published before the dialog opens; late answers are ignored).
    `runModalDialog()` waits for the answer in a nested `QEventLoop`, as
    `QDialog::exec()` does, so Core keeps its blocking calls; when the loop
    ends otherwise (`QCoreApplication::exit()` ends every running loop) the
    request is abandoned with the declined answer.
    `ConfirmationDialogModel` (Yes default, Escape / close answer No),
    `FileReplaceDialogModel` (Yes / No with "Apply to all" only for
    multiple collisions, Cancel stops the operation, Escape / close skip
    the item as the widget dialog's reject did, abandoned cancels),
    `ResizeDialogModel` (all rules of `ResizeDialog`: percent 1 - 1600,
    sides following the edited one in single precision, common sizes, fit /
    fill the primary screen, reset, filter list with MKS 2021 default,
    Upscayl only with models and only for upscales, request only for a
    changed size plus the preferences to store), `SavePathDialogModel`
    (filters from `saveFileFiltersFor()`, a non-local URL is rejected with
    a warning) and `TextInputDialogModel`, owned by `DialogCoordinator`.
  - **Shared helper:** `utils/savefilefilters` (in
    `wynpix_viewcomponents`) builds the "Save File as..." filters and the
    filter of the suggested suffix; `MW::getSaveFileName()` uses it.
  - **Application side:** `QuickDialogPort` replaces `DecliningDialogPort`
    as the Quick UI's dialog port. It gathers the inputs (writable formats,
    primary screen size, `Settings::availableUpscaylModels()` and the
    stored resize preferences), starts the request, waits with
    `runModalDialog()` and stores the Upscayl preferences of an accepted
    resize.
  - **QML:** `DialogWindow` is a separate application-modal `Window` (like
    an exec()'d `QDialog`: the main window takes no input and ignores the
    close button meanwhile), centred on the main window, sized to its
    content. Enter presses the focused push button and otherwise accepts
    (a focused spin box loses the focus first, so its typed text is
    committed); Escape and the close button dismiss. `ConfirmationDialog`,
    `FileReplaceDialog`, `ResizeDialog`, `TextInputDialog` and
    `SaveFileDialog` (`FileDialog` from `QtQuick.Dialogs`, the native
    Windows dialog) are created by `DialogLayer` in `Main` on their first
    request. `wynpix.style` gained `DialogButtonBox` and the dialog margins.
    The QML files keep the widget class names as translation contexts
    (the save dialog's title was in the `MW` context).
  - **Deviations:**
    - Dialogs are `Window`s, not `Dialog` popups, and confirmations use the
      themed `ConfirmationDialog` instead of `MessageDialog`, which has no
      native implementation on Windows in Qt 6.12 (its fallback is not
      themed).
    - Keys are handled by propagation to the dialog's focus scope, not by
      window `Shortcut`s: `QWindow::isActive()` is true for every dialog
      window whose transient parent family has the focus, so the shortcuts
      of several dialog windows were ambiguous and none fired.
    - The resize dialog's fields are always enabled; editing the percent
      or a side selects its mode (the widget dialog started with every
      field enabled and switched only through its radio buttons). Sides
      are clamped to 1 - 65535 (the widget dialog could request a side of
      0).
    - Long source paths are elided in the middle and destinations wrap at
      600 px (the widget dialog grew with the source path).
    - Enter on a focused, closed combo box does not accept the dialog
      (Qt Quick's `ComboBox` takes the key).
  - **Flagged, not changed:** `ResizeDialog` and `BatchConverterDialog`
    scan the Upscayl models themselves instead of calling
    `Settings::availableUpscaylModels()`; the JPEG save filter keeps its
    `*jpe` pattern (missing dot) for parity.
  - **Not verified by hand in the running application:** opening the
    dialogs, since no input was sent to the desktop; the tests drive them.
    Startup is unchanged (dialogs are created on first use): Release, a
    300 x 300 PNG, process start to "first document rendering settled", 5
    runs each: Quick UI median 389 ms, widget UI median 545 ms.
  - Tests: `wynpix_tests` gained `DialogTests` (save filters, the session
    rules and the modal wait, every view-model's answers and the resize
    rules); `wynpix_qml_tests` gained `tst_dialogs.qml` (creation on first
    request, Enter / Escape / buttons / close for the confirmation, file
    replace with "Apply to all" and Cancel, a typed width committed by
    Enter, common size and Upscayl model, the text prompt, a second
    request while open); `tst_mainwindow.qml` passes the coordinator.

#### S3.3 Settings dialog
- **Goal:** split the 4300-line `SettingsDialog` into a settings view-model
  and QML pages, with the shortcut creator and script editor dialogs it
  opens (moved here from S3.2).
- **Owner:** `SettingsEditorModel` (C++): load, validate, apply, reset
  defaults, shortcut table (`QRangeModel` + `SortFilterProxyModel`),
  scripts list, theme editor; QML pages per tab, loaded lazily.
- **Scope:** done in two commits: (a) refactor, extracting the model and
  switching the widget dialog to it; (b) QML pages.
- **Acceptance:** applying settings from either UI yields identical
  `Wynpix.ini` content.
- **Delivered** (one commit, as the user chose, instead of the planned
  two):
  - **Shared editor** (`components/settingseditor/`, in
    `wynpix_viewcomponents`, free of `Settings`; used by both UIs):
    `SettingsValues` holds one value object (gadget) per page
    (`GeneralSettings`, `ViewSettings`, `ThemeSettings`,
    `ControlsSettings`, `AdvancedSettings`, `UpscaleSettings`). Slider
    values keep the slider units of the widget dialog, so both UIs store
    the same values. `SettingsScales` holds the ranges, conversions, snapping
    and value texts, and `SettingsOptions` the combo box / radio choices
    with their stored values (texts in the `SettingsDialog` translation
    context). `SettingsEditorModel` loads, normalizes (fallbacks and range
    limits as the widget sliders applied them), keeps the draft, applies
    it, previews the theme values, and owns `ShortcutTableModel` (sorted by
    action, a shortcut bound once), the script list (`QStringListModel`),
    `ShortcutEditorModel` and `ScriptEditorModel` (on `EditorSession`:
    `open` / `created` / accepted / rejected). The model talks to the
    application only through two ports, `ISettingsValueStore` and
    `IShortcutScriptStore`.
  - **Application side:** `AppSettingsStore` / `AppShortcutStore`
    (`gui/settingseditor/`) read and store through `Settings`,
    `ActionManager` and `ScriptManager` exactly as the widget dialog did.
    `static_assert`s tie the dialog ranges to the `Settings` limits. The
    accent scheme copy, which was written out three times, now exists once.
    Clearing the thumbnail cache is a signal that each UI forwards to Core.
    Because both UIs apply through `SettingsEditorModel::apply()` and these
    stores, the acceptance (identical `Wynpix.ini`) holds by
    construction.
  - **Widget UI:** `SettingsDialog` keeps its generated layout. It shows the
    model's values, collects the edited ones on Apply / OK, and runs its
    rules, labels, snapping, previews, shortcut and script edits through
    the model. The shortcut table and script list are `QTableView` /
    `QListView` on the models. `ShortcutCreatorDialog` and
    `ScriptEditorDialog` are views of their view-models, with the
    non-Windows `.sh` note removed.
  - **Quick UI:** `SettingsDialogController` (`gui/quick/ui/settings/`,
    with `QML_FOREIGN` / `QML_VALUE_TYPE` registrations of the editor types;
    `wynpix_viewcomponents` now extracts its metatypes) opens
    `SettingsDialog.qml`. It is an application-modal, resizable window
    with the sidebar, OK / Apply / Cancel and eight page files
    (`Settings*Page.qml`). The building blocks are `SettingsPage`,
    `SettingsSection`, `SettingsSliderRow`, `SettingsComboBox`,
    `SettingsRadioRow` and `SettingsNote`. Pages edit the draft by value
    type write-back (`editor.general.loopSlideshow = checked`). Each page
    is created on its first show while the window is open and dropped when
    it closes, so every opening shows the reloaded values; the window
    itself is created on first use. `ShortcutCreatorDialog.qml` captures
    keys, mouse buttons and the wheel in `ShortcutCaptureField` through the
    new `Actions.wheelShortcutText` / `mousePressShortcutText` /
    `mouseReleaseShortcutText`. `ScriptEditorDialog.qml` picks the
    executable with a `FileDialog`, the View page picks the ICC profile
    with a `FileDialog`, and the Theme page picks the accent with a
    `ColorDialog`. `QuickUiHost` opens the window on `openSettings` and on
    the context menu's "Configure menu" (Scripts page), replacing the
    logged stub.
  - **Kept for parity:** Cancel does not undo the previewed theme values
    (theme mode, accent, black background, thumbnail bar opacity); scripts
    are added, edited and removed at once; editing a script under a new
    name adds it and keeps the old one. "Reset to defaults" restored the
    default shortcuts at once as well; later it moved into the shortcut
    table, so it waits for Apply and Cancel undoes it.
  - **Fixed:**
    - A stored language that is not offered now falls back to English. The
      widget dialog's `setCurrentText("en_US")` never matched an entry, so
      it fell back to the system language.
    - Zoom step and CAS values are rounded, not truncated, when shown
      (0.29 showed as 28 and drifted down on every Apply).
    - The shortcut creator accepts only once a shortcut was entered. The
      placeholder "[Enter shortcut]" could be stored as a shortcut, and a
      script binding could be made without scripts.
    - The thumbnail bar opacity preview stores the same double as Apply
      (it stored a float before).
    - Without Upscayl models the stored model name is kept (it was
      overwritten with an empty name).
  - **Deviations:**
    - No `SortFilterProxyModel` (the dialog has no filter field) and a
      `QAbstractTableModel` instead of `QRangeModel` (replacing a bound
      shortcut removes a second row).
    - The Quick window shows the application and Qt versions as text
      without their icons.
    - Radio buttons follow their value only (`checkable: false`), since
      the rows are not siblings.
    - "Get more models" is a new translatable string. The widget label was
      an HTML string.
  - **Flagged, not changed:** the language list keeps the native name of
    the shipped Russian translation (`ru_RU.ts`) as data.
  - **Not verified by hand in the running application:** opening the
    settings window, since no input was sent to the desktop; the tests
    drive it. Startup creates nothing new (the window and pages are created
    on first use): Release, a 300 x 300 PNG, process start to "first
    document rendering settled". The machine was loaded and noisy, so HEAD
    and this change were measured interleaved, 3 runs each, several
    rounds. Quick UI medians: HEAD 378 - 403 ms, this change 392 - 510 ms.
    Widget UI medians: HEAD 742 - 749 ms, this change 763 - 865 ms. Both
    UIs rose together, and the widget UI runs no new code at startup.
  - Tests: `wynpix_tests` gained `SettingsEditorTests` (scales and texts,
    load / apply round trip, fallbacks and range limits, the Upscayl rules,
    theme previews, the shortcut table, the creator and the script editor,
    `SettingsDialogController`) over in-memory stores
    (`tests/fakes/fakesettingsstores.h`). `wynpix_qml_tests` gained
    `tst_settings.qml`: lazy pages, OK / Apply / Cancel / Escape, values
    reloaded on reopening, the theme preview, a key and a mouse button
    captured as shortcuts, a new script, clearing the cache.
    `tst_mainwindow.qml` passes the controller.

#### S3.4 Batch converter, print, map overlay
- **Goal:** the remaining complex windows.
- **Owner:** `BatchConverter` component (unchanged) + `BatchConverterModel`;
  `PrintController` keeps `QPrinter`/`QPrintDialog` (native Windows dialog,
  reason for keeping `QApplication`); map overlay drawn with
  `QCanvasPainterItem` or tile `Image`s depending on its current content.
- **Acceptance:** batch progress updates do not stall the UI; printing
  output identical; map overlay parity.
- **Delivered** (one commit, as the user chose):
  - **Map overlay:** `MapOverlay` was compiled but never created (the
    widget viewer has no minimap), so parity holds without a port. It was
    deleted as dead code, together with `DecliningDialogPort`
    (`interimports.*`), which has no user left.
  - **Shared batch rules** (`components/batchconverter/`, in
    `wynpix_viewcomponents`, free of `Settings`): `BatchJob`,
    `AspectFitMode`, `RotationAngle` and the new `BatchItemState` moved to
    `batchjob.h`. `BatchJobRules` holds what the widget dialog decided
    inline: the output formats, quality scales and the stored defaults per
    format, the percent / side rules and limits, common sizes, filters,
    rotations, the colour sliders and their conversion, the pattern check,
    the initial output folder, the start checks in the widget's order with
    their messages, the job handed to the converter (`finalJob()`) and the
    status texts (widget translation contexts kept). `BatchConverter`
    reports `BatchItemState` instead of a translated status string, which
    the dialog compared with `tr("Processing...")` / `tr("Done")`.
  - **Shared printing** (`components/printing/imageprintsetup`, in
    `wynpix_viewcomponents`, which now links `Qt6::PrintSupport`):
    `imagePrintRect()`, the preview rendering and `ImagePrintSetup`, which
    owns the selected printer (`std::unique_ptr`, replacing the raw pointer
    with manual `delete`) and the A4 PDF printer, and prints / exports. Both
    print dialogs print through it, so the output is identical by
    construction.
  - **Widget UI:** `BatchConverterDialog` and `PrintDialog` use the shared
    rules and printing. `LinkedSliderSpin` takes a `ColorSliderSpec` (its
    8-argument constructor broke the 5-argument rule). The batch dialog
    lists the Upscayl models from `Settings::availableUpscaylModels()`.
  - **Quick UI** (`gui/quick/ui/dialogs/`, module `wynpix.ui`):
    - `BatchConverterDialogModel` (`DialogSession`) holds the draft,
      validation, start / stop / close and the messages. It converts through
      a `BatchConversionService` (an abstract `QObject` with the converter's
      signals plus thumbnails), because `BatchConverter` depends on
      application code (ImageLib, Upscayl). The application's
      `AppBatchConversionService` wraps `BatchConverter` and `Thumbnailer`.
      One service is made per request, and destroying it stops a running
      batch and lets both delete themselves, as the widget dialog did.
    - `BatchQueueModel` lists the files. The image headers and file sizes
      are read on the model's own one-thread pool and applied in groups of
      32, with a scan generation that drops the results of a previous
      queue. Each progress report changes one row, so the window stays
      responsive (the acceptance).
    - `PrintDialogModel` (`DialogSession`) handles the printer, orientation,
      colour, fit to page, the preview and the PDF export. It reports the
      preferences to store on every close.
    - `QuickDialogPort` gathers the inputs (stored qualities, Upscayl models
      and preferences, printers through `QPrinterInfo`, print preferences,
      the screen's device pixel ratio). It stores the batch's Upscayl
      preferences after a started batch and the print preferences after
      every close.
  - **QML:** `BatchConverterDialog.qml` is an application-modal, resizable
    window with the queue (`BatchItemWidget.qml` rows with `ThumbnailItem`
    thumbnails), the settings column (`LinkedSliderSpin.qml` colour rows),
    `FolderDialog` for the output folder, a progress bar and an in-window
    message window. `PrintDialog.qml` is a `DialogWindow` with the page
    preview (`ThumbnailItem` plus a page frame on light themes) and a
    `FileDialog` for the PDF. Both are loaded on first use by
    `DialogLayer`. `wynpix.style` gained `ProgressBar`.
  - **Fixed:**
    - Choosing another printer kept the page orientation. The new
      `QPrinter` was portrait while "Landscape" stayed checked.
    - The printer list shows the printer that is used. The widget dialog
      showed the default printer while printing on the last one.
    - A failed print or export reports the error and keeps the dialog
      open. The widget dialog closed silently.
    - Batch target sides stay within 1 - 65535. A small percent of a small
      image gave a 0 px target and a resize error.
    - The Quick batch dialog keeps the stored batch model when no models
      exist (as S3.3 did for the settings).
  - **Deviations:**
    - The batch warnings and the completion message are a window of the
      batch dialog instead of `QMessageBox`es.
    - The Quick batch dialog stores the Upscayl preferences when it closes
      after a started batch. The widget dialog stored them at the start.
    - The colour value suffix is a label next to the spin box.
    - Not a `QPrintDialog`, which the plan assumed: the widget print dialog
      never used one, so `PrintDialogModel` follows the widget dialog.
  - **Flagged, not changed:** the widget `BatchItemWidget` still reads every
    image header on the GUI thread when the dialog opens; `ResizeDialog`
    still scans the Upscayl models itself.
  - **Not verified by hand in the running application:** opening the
    dialogs, since no input was sent to the desktop; the tests drive them.
    No real printer was printed on. Startup is unchanged (the dialogs are
    created on first use). Release, a 300 x 300 PNG, process start to
    "first document rendering settled": Quick UI median 371 ms (5 runs),
    widget UI median 733 ms (3 runs), no warnings.
  - Tests: `wynpix_tests` gained `BatchPrintTests`. It covers the batch
    rules, the queue (worker header reads, selection, dropped scans,
    states, thumbnails), the batch view-model over
    `FakeBatchConversionService` (`tests/fakes/`) with defaults, quality,
    resize, colour, start checks, run / stop / close / start failure and
    preferences, the print rectangle, a PDF export, the oriented grayscale
    preview, and the print view-model (no printers, export and failure,
    last printer). `wynpix_qml_tests` gained `tst_batchprint.qml`, which
    covers the queue window, collapsed colour sliders, the warning window,
    convert / stop / finish through the buttons, and the print window with
    and without printers.

### Phase 4: Switch-over and removal

#### S4.1 Parity review and default switch
- **Goal:** Quick UI becomes the default; `--ui=widgets` remains for one
  release as a fallback.
- **Scope:** full parity checklist (every action, every overlay, every
  dialog, every setting); performance report (cold start, first frame,
  zoom/pan frame times with `qmlprofiler` and `QSG_RENDER_TIMING`,
  thumbnail scroll, memory, VRAM); `qmllint` clean.
- **Delivered:**
  - **Default switch:** `utils/uimode` (`UiMode`, `defaultUiMode`, the
    names `quick` / `widgets`, case-insensitive parsing) is shared by
    `main.cpp` and `Settings::uiMode()`. `--ui` has no default value any
    more. Without it, the hidden setting `userInterface` (`[General]` in
    `Wynpix.ini`) decides, and the Quick UI is the default.
    - An unknown setting value logs a warning and starts the Quick UI.
    - An unknown `--ui` value exits with an error.
    - A failed Quick start still exits with an error: no automatic
      fallback to the widget UI (user's choice).
    - Checked on the real executable for all nine flag and setting
      combinations.
  - **qmllint:** `all_qmllint` reports nothing. The two unused imports
    it noted (`style/Popup.qml`, `ui/FolderGridView.qml`) were removed.
  - **Parity review.** Checked against their handlers in both UIs: every
    port of `UiPorts`, every `UiEvents` signal, every `ActionManager`
    action, every widget overlay, dialog and panel, and every `Settings`
    getter the widget UI reads.
    - Ports: all implemented.
    - `UiEvents`: all emitted, with two exceptions.
      `scalingRequested` is replaced by `upscaleRequested` by design (the
      GPU scales). `showFoldersChanged` is not emitted by the widget
      `FolderView` either.
    - Overlays, dialogs and panels: zoom indicator and click zones (S1.6),
      the S2.3 overlays, panel (S2.4), context menu and crop (S2.5),
      folder view (S3.1), dialogs (S3.2, S3.4), settings (S3.3). The
      window opens on the last display.
    - Dead in both UIs, for S4.2 to remove: `UiEvents::showFoldersChanged`,
      the widget `InfoBar` (`gui/panels/infobar`, never created) and the
      `infoBarWindowed` setting.
  - **Gaps found and closed:**
    1. `toggleUpscayl`, `cycleUpscaylModel` and `toggleHdrToneMapping` did
       nothing in the Quick UI, and neither did the context menu's Upscayl
       row. `ViewerToggles` (`components/viewertoggles`, in
       `wynpix_viewcomponents`, free of `Settings` behind
       `IViewerToggleStore`; `AppViewerToggleStore` implements it over
       `Settings`) now holds the logic `MW` had inline: the toggles, their
       messages (widget translation context `MW` kept), model cycling from
       the last chosen model, and the change announcement after
       `kModelSwitchDelayMs`. Both UIs use it. `MW` lost the three slots,
       its timer and the pending model.
    2. The Quick UI ignored `autoResizeWindow`. MW's computation became
       `windowGeometryFittingContent()` (pure) and is applied by
       `WindowStateController::fitToContent()`. `QuickViewerPort` is now a
       `QObject` that emits `documentShown(QSize)`, which
       `QuickMainWindowController::fitWindowToDocument()` applies when the
       setting is on. `MW::preShowResize()` uses the same function.
       Checked on the real executable: a 300x200 image gives a 300x200
       window, and a 6000x3000 image a 2304x1152 window on a 2560x1400
       work area (90 % limit).
  - **Found, not fixed** (widget UI only; it is removed in S4.2): with
    `autoResizeWindow`, the widget UI does not fit a window that is still
    hidden at cold start. `MW::preShowResize()` stores the geometry in the
    settings, but `MW::showWindowed()` no longer restores it on a plain
    show. The Quick UI fits the hidden window.
  - **Tests:** `wynpix_tests` gained `UiModeTests` and `ViewerTogglesTests`,
    and `WindowStateTests` gained the window fitting cases. All suites pass,
    with the render tests on D3D11, D3D12 and Vulkan.
  - **Performance report** (2026-10-10, release build, D3D11, 2560x1440 at
    165 Hz). Startup and memory: five interleaved cold starts per UI on a
    4000x3000 JPEG, memory sampled 5 s after launch, medians:

    | | Quick | Widgets |
    |---|---|---|
    | First document rendering settled | 425 ms | 617 ms |
    | Window revealed | 431 ms | 617 ms |
    | Working set | 270 MB | 297 MB |
    | Private bytes | 440 MB | 591 MB |
    | Dedicated VRAM | 151 MB | 275 MB |

    Frame times (Quick UI only): `QSG_RENDER_TIMING` (whole milliseconds)
    while 40 wheel events were posted to the window, 50 ms apart.

    | | Frame interval median | p95 | Max | Frames over 25 ms |
    |---|---|---|---|---|
    | Zoom in and out (the JPEG) | 6 ms | 7 ms | 7 ms | 0 |
    | Folder grid scroll (400 images) | 6 ms | 7 ms | 8 ms | 0 |

    Both run at the 165 Hz refresh rate. Once the input stops, the window
    renders no further frames. Memory after the grid scroll:
    working set 234 MB, dedicated VRAM 96 MB.
  - **Deviations:**
    - `qmlprofiler` was not used. The release build has no QML debugging,
      and the user chose not to make a separate QML-debug build.
    - The widget UI's frame times were not measured; it has no counterpart
      of `QSG_RENDER_TIMING`.

#### S4.2 Remove the widget UI
- **Goal:** delete the legacy code paths.
- **Scope:** remove `gui/` widget classes, `ProxyStyle`, `FilterPixmapItem`,
  `PanoramaGraphicsItem`, old GLSL shaders, `Qt6::OpenGLWidgets`,
  `Qt6::SvgWidgets`, `AA_UseDesktopOpenGL`, the `*Proxy` deferral classes,
  `IDirectoryView` widget implementations; keep `Qt6::Widgets` +
  `PrintSupport` only for printing.
- **Acceptance:** no `QWidget` subclass remains outside the print path;
  deploy size and startup measured.
- **Delivered:**
  - **Widget UI removed:** everything under `gui/` except `ports/`,
    `controllers/` (`ColdStartWindowController`), `settingseditor/`,
    `quick/`, `idirectoryview.h`, `uimetrics.h` and `dialogsurfaces.h`
    (removed later with `Theme.surfaces`).
    That is `MW`, `CentralWidget`, `ContextMenu`, `customwidgets/`,
    `dialogs/`, `overlays/` (with the `*Proxy` deferral classes),
    `panels/` (with the dead `InfoBar`), `viewers/` (`ImageViewerV2`,
    `FilterPixmapItem`, `PanoramaGraphicsItem`), `folderview/` (the widget
    `IDirectoryView` implementations) and `widgetui/`. Also removed:
    - `ProxyStyle` and `utils/displayutils`.
    - The old GLSL shaders and `res/styles/style-template.qss`.
    - The tree view arrows and the unused logos in `resources.qrc`.
    - `Settings::loadStylesheet()` (application palette and stylesheet).
    - `AA_UseDesktopOpenGL`.
    - `Widgets`, `SvgWidgets` and `OpenGLWidgets` from `find_package` and
      from the `wynpix` link.
  - **`QGuiApplication`:** `main.cpp` creates a `QGuiApplication`.
    - Qt Widgets now comes only as a dependency of `PrintSupport`
      (`QPrinter`). `dumpbin /dependents` lists no Widgets DLL for
      `wynpix.exe`.
    - The QML print dialog, the QtQuick.Dialogs file, folder and colour
      dialogs, and the folder tree's `QFileSystemModel` need no
      `QApplication`.
    - No `QWidget` code remains at all, the print path included.
  - **UI selection removed** (user's choice): the `--ui` option, the hidden
    `userInterface` setting, `utils/uimode` and `UiModeTests`.
    - An existing `userInterface` key is ignored.
    - This replaces S4.1's plan to keep `--ui=widgets` for one release.
  - **CPU display path removed** (user's choice; not in the stage's listed
    scope):
    - The `Scaler` component, `UiEvents::scalingRequested`,
      `Core::scalingRequest()` and `onScalingFinished()`, and
      `DirectoryModel::requestScaled()`, `clearScaler()` and
      `scalingFinished`.
    - `IViewerPort::displayPipeline()` and `showScaledImage()`.
    - `DisplayPipeline` together with `DecodeContext::displayPipeline` and
      `Loader::setDisplayPipeline()`.
    - The colour-managed display copies of `ImageStatic`, and
      `Image::getPixmap()`.
    - Every static image now behaves as the former `Gpu` pipeline did: it
      keeps the HDR source, and `DecodedPixels` makes the SDR copy on
      first use.
    - Only the scaler reserved `Cache` items, so the reservations and
      `CacheItem` are gone. `Cache` is a plain path-to-image map.
  - **Dead items from S4.1:** `UiEvents::showFoldersChanged` together with
    `Core::setFoldersDisplay()`, and the `infoBarWindowed` setting with its
    `OverlaySettings` bridge field.
  - **Moved:** `FileSystemModelCustom` (the folder tree model of the Quick
    folder view), from `gui/folderview/` to `gui/quick/adapters/`.
  - **Comments:** comments that described both UIs, `--ui=quick`,
    `WidgetUi` or the stylesheet as current were corrected. Provenance notes
    ("FolderView in the widget UI", "as in the widget viewer") are kept as
    the record of where a behaviour comes from. `BUILDING.md` lists the Qt
    components the build now needs.
  - **Tests:** `DecodedPixelsTests` lost the CPU-pipeline case. All suites
    pass, with the render tests on D3D11, D3D12 and Vulkan, and
    `all_qmllint` reports nothing. On the real executable, a JPEG, a
    Radiance HDR image (tone mapped on the GPU) and a folder all open
    without warnings, and the window keeps the executable's icon.
  - **Deploy size** (2026-10-10): `windeployqt` with the build's flags, into
    an empty folder, for HEAD and for this stage.

    | | Before | After |
    |---|---|---|
    | `wynpix.exe` | 11,190,784 B | 10,104,832 B |
    | Qt deployment and executable | 128,994,567 B (1,471 files) | 127,784,087 B (1,469 files) |

    The deployment no longer has `Qt6OpenGLWidgets.dll` or
    `Qt6SvgWidgets.dll`. `Qt6Widgets.dll` stays for `PrintSupport`.
  - **Startup and memory** (2026-10-10, release build, D3D11): 16
    interleaved cold starts per build on a 4000x3000 JPEG, from complete
    copies of the deployment. Memory sampled 5 s after launch; medians:

    | | Before | After |
    |---|---|---|
    | First document rendering settled | 435 ms | 430 ms |
    | Window revealed | 438 ms | 434 ms |
    | Working set | 213 MB | 212 MB |
    | Private bytes | 345 MB | 345 MB |

    No measurable change. The Quick UI never created widget code, so
    removing it changes the binary size, not the run time.
  - **Found, not fixed:** build folders from before this stage still hold
    `Qt6OpenGLWidgets.dll` and `Qt6SvgWidgets.dll`, which windeployqt does
    not remove.
- **Follow-up** (2026-10-10, two commits after the removal):
  - **Unused Controls styles no longer deployed.**
    - `windeployqt --qmldir` deployed every Qt Quick Controls style, because
      the `QtQuick.Controls` module lists them all as optional imports.
    - Qt's Quick dialog implementations (the accent `ColorDialog`) import
      `QtQuick.Controls` and so loaded the platform default style at run
      time (Windows).
    - `QuickUiHost` now pins the run-time style to Basic
      (`QQuickStyle::setStyle()`, before the engine loads), the base of
      `wynpix.style`. The accent dialog is drawn in Basic instead of
      Windows, and `-style` / `QT_QUICK_CONTROLS_STYLE` no longer change it.
    - The `wynpix` POST_BUILD step passes
      `--skip-plugin-types styles` (Qt Widgets styles; nothing loads them
      under `QGuiApplication`). After windeployqt it removes the Fusion,
      Imagine, Material, Universal, FluentWinUI3 and Windows modules,
      `QtQuick/NativeStyle`, their libraries, and the `styles` folder of
      earlier deployments.
    - No kept library links against a removed one (`dumpbin /dependents`).
      Qt's `qml` tool, run inside the trimmed deployment, cannot load the
      ColorDialog under the default style ("module QtQuick.Controls.Windows
      is not installed") and opens it with Basic.
    - Deployment: 127,784,087 B (1,469 files) -> 106,874,794 B (255 files).
  - **Translations updated** (`lupdate`, now with `-no-obsolete`): the
    entries of the deleted widget classes are gone, and the QML strings that
    were missing are added.
    - lupdate never scanned `wynpix_viewcomponents`, whose texts (settings
      editor, batch rules, viewer toggles, file info) had kept their
      translations only through the widget classes' copies. It is now one
      of the lupdate source targets.
    - lupdate only extracts literal contexts. The texts passed through a
      context constant or a helper (the settings editor models,
      `FormatFilterModel`, the CAS and colour adjustment editors) are
      marked with `QT_TRANSLATE_NOOP`, as `SettingsOptions` already did.
    - lupdate's QML parser drops backslashes from string literals. The
      example paths of the cache exclusion tooltip are therefore a `%1`
      argument, and the existing translations were moved to the `%1` form.
    - Every dropped entry was checked against the sources (same context,
      same text): none is used any more. `en_US.ts` is not in `TS_FILES`
      and stays untouched.
  - **Post-audit fix A-01 (2026-10-13, user's decision): the viewer offers
    GPU filters only.**
    - Removing the CPU display path left the CPU filters Smart (2) and
      MKS2021 (5) in the viewer's filter list and cycle, drawn as plain
      trilinear; 3.6.4 had stored MKS2021 as the default.
    - `ScalingFilterSelection::viewingFilters()` is the one list of viewing
      filters (Nearest, Bilinear, CAS, Smart GPU, MKS2021 GPU); the settings
      list and the filter cycle follow it.
    - `ScalingFilterSelection::forViewing()` maps the stored CPU values on
      every read in `Settings::scalingFilter()`, independent of the version:
      MKS2021 -> MKS2021 GPU (same kernel), Smart -> Smart GPU (a different
      algorithm, an approximation). `imageFilterModeFor()` shows the CPU
      values as their GPU counterparts.
    - Resize, batch conversion and wallpapers keep the CPU filters.
    - **Open:** CPU Smart (`ImageLib::scaled_Smart()`) has no GPU port. If
      it is wanted in the viewer, port it to a shader with a CPU-parity test
      like S1.3; do not bring back a CPU display path.
  - **Post-audit fix A-02 (2026-10-13, user's decision): Window opacity
    works again.**
    - The setting was offered and stored but nothing read it; the Quick
      window was opaque.
    - `MainWindowShell::backgroundOpacity()` owns the rule: the setting
      in windowed document mode, opaque in fullscreen and in folder view
      (3.6.4 `ImageViewerV2::onFullscreenModeChanged()`). `QuickUiHost`
      feeds it at construction and on every settings change.
    - On Direct3D 11/12 (`MainWindowShell::translucentWindow()`, set by
      `QuickUiHost` before the window exists) the window colour stays
      `transparent` for the whole session and a full-window `Rectangle`
      paints the background with the opacity. Pitfall:
      `QQuickWindow::setColor()` rewrites the format's alpha buffer
      whenever the colour's alpha changes, and the format is fixed once
      the native window exists; binding the window colour to the opacity
      made a session that starts in folder view (opaque) lose the alpha
      channel before the first show. With the alpha buffer the scene
      graph presents with premultiplied alpha through DirectComposition,
      in every mode (as 3.6.4 always set `WA_TranslucentBackground`).
    - Pitfall 2: DWM composes the DirectComposition content over the
      window's opaque GDI redirection surface, so the background blended
      with it (uniform grey) instead of with the desktop.
      `QuickUiHost::createTranslucentNativeWindow()` creates the main
      window with `WS_EX_NOREDIRECTIONBITMAP` by setting
      `QT_QPA_DISABLE_REDIRECTION_SURFACE` only around `create()` (a
      user's value is kept); other windows keep their redirection
      surface.
    - Vulkan stays opaque and the opacity is forced to 1 there: NVIDIA on
      Windows offers no premultiplied composite alpha mode. The graphics
      API is read once per session (`mGraphicsApi`), so the API, the
      pipeline cache name and the translucency always agree.
    - The viewer clears to transparent while translucent, so the alpha
      applies once. It now also clears to `backgroundFullscreen` in
      fullscreen; before, it always used `background`.
    - Checked on the Release build (desktop visible through the windowed
      background at 80 %); audit 4.1 item 2 remains for fullscreen,
      folder view and the theme switch.
  - **Post-audit fix A-13 and exit hang (2026-10-13): the exit action hides
    the window and always ends the process.**
    - `Core::forceExit()` hides the window before quitting (`18814806`), so
      it no longer stays on screen, frozen, during teardown.
    - Found with it: once any dialog window had been opened in the
      session, exit hid the window and left the process running in
      `QCoreApplication::exec()` (stack of a hung Release process).
      `QGuiApplication::event(QEvent::Quit)` closes every top-level window
      that still has a platform window and ignores the quit if one refuses
      (Qt 6.12 `qguiapplication.cpp`). The dialog windows are created on
      first use, kept hidden afterwards, and refuse every close
      (`onClosing` turns it into dismiss); the main window refuses too
      (its close event filter turns it into exit or standby).
    - `Core::forceExit()` now calls `QCoreApplication::exit()`, which
      emits `aboutToQuit` (the window placement is still saved) and ends
      every event loop, nested modal ones included, without the
      close-every-window pass. Pitfall: do not go back to `quit()` while
      any window vetoes `QEvent::Close`.
  - **Post-audit fix: cold-start reveal (2026-10-13, user's report: the
    window seemed to draw from the top or from the top-left corner).**
    - Recorded at 60 fps (Desktop Duplication) on the Release build,
    maximized window, folder view at start. Three causes, all visible
    before the fix:
      1. `QuickMainWindowController::setWindowConcealed()` did nothing
         (S2.1), so `ColdStartWindowController` and `Core::raiseWindow()`
         waited for readiness with the window already on screen: the DWM
         open animation (title bar fading in, empty client area) and every
         intermediate frame were visible. Not caused by `1996a528`: with
         the redirection surface back the same run showed a white window
         first.
      2. The folder view is created by an asynchronous `Loader` while the
         hidden window still has the restored size (1356x677); the window
         is maximized right after, but the `Loader` sizes its item only
         when the incubation starts and when it is ready, so the view, its
         readiness (`filesystemViewReady`) and the range of
         `visibleThumbnailsReady` followed the restored size; the view was
         drawn in the top-left corner and grew about 60 ms later.
      3. `GridView` creates its visible cells with
         `QQmlIncubator::AsynchronousIfNested`; inside the asynchronous
         incubation the cells were incubated over the frames after the
         reveal and the grid filled in row by row.
    - Fixes: the port cloaks the window (`DWMWA_CLOAK`, `dwmapi`): it is
      shown, sized and rendered, but DWM does not compose it until
      revealed (Qt 6.12's D3D11/D3D12 back ends ignore
      `DXGI_STATUS_OCCLUDED`, so rendering goes on). Window opacity is no
      option: a layered window does not present a flip-model swap chain.
      `Main.qml` creates the folder view synchronously while the window is
      hidden (`asynchronous: root.visible`), and the view anchors itself to
      the `Loader`; `FolderView.qml` reports its width to the controller
      only once it is complete.
    - Measured (Release, 4 runs, process start to "cold-start window
      revealed"): folder view 446-453 ms (one run 548 ms), before the fix
      with the cloak alone 592-707 ms; document view 386-390 ms. The
      recording after the fix shows the complete folder view, thumbnails
      included, in its first visible frame. All suites pass.
    - Not checked: standby resume and second-instance raise go through the
      same port (`Core::raiseWindow()`), so they are cloaked too now;
      standby was off on the test machine.
  - **Post-audit fix A-03 (2026-10-13): renderer errors and graphics device
    loss reach the user.**
    - `ImageViewportController`, the owner of the view, connects
      `ImageRenderItem::renderError` and publishes each distinct message
      once per shown image as an Error notification (`notificationRequested`,
      the signal `QuickViewerActions` and `ViewerToggles` already use);
      `QuickUiHost` routes it to the notification overlay. The set of
      reported messages is cleared in `reset()`, so a repeating failure
      (every animation frame, every new device) is shown once.
    - New `GraphicsDeviceLossMonitor` (`gui/quick/ui`), attached to the
      window like `FramePresentationTracker`: on `sceneGraphInvalidated`
      (render thread, direct connection) it checks
      `QQuickWindow::rhi()->isDeviceLost()` while the lost QRhi still
      exists, and posts `deviceLost()` to the GUI thread; the controller
      shows a Warning. Qt 6.12 recovers by itself (both render loops tear
      the scene graph down, post a full update and create a new QRhi), so
      only the report was missing. Other invalidations (window destroyed or
      hidden) are not reported.
    - Tests: `unusableDisplayProfileIsReported` (invalid target colour
      space, the ProPhoto case), `renderErrorsAreShownOncePerImage`
      (unit), `sceneGraphTeardownIsNotADeviceLoss` (render suites, three
      back ends). All suites pass (Debug). A real device loss is not
      reproducible in a test; audit 4.1 item 5 remains.
    - Not changed: the message texts of the renderer are English and are
      shown behind a translated "Display error:" prefix; the consecutive-only
      deduplication inside the renderer (A-16) is untouched.
  - **Post-audit fix A-04 (2026-10-13): a failed HDR-to-SDR conversion no
    longer crashes saving and no longer makes edits silently do nothing.**
    - `DirectoryModel::saveFile` takes the SDR copy once, checks it, and
      returns `ImageSaveError::InvalidSourceImage` when it is null; Core
      already shows that as "Could not save file: the image is invalid".
    - `Core::edit_template` skips an image whose SDR copy is unavailable or
      whose edit produced no image (it is neither updated nor saved), shows
      one Error notification for the action, and returns whether every
      selected image was edited; `cropAndSave` saves only after a successful
      crop. `applyColorAdjustments` and `print` report the same failure
      instead of returning silently.
    - Not tested automatically: the SDR conversion of `ImageStatic` is fixed
      (`sdrFromHdr`) and there is no unit fixture for `DirectoryModel` or
      `Core`; the failing conversion itself is covered by
      `tst_decodedpixels` (`aFailedConversionIsReportedOnce`).
  - **Post-audit fix A-06 (2026-10-13): the thumbnail strip keeps its scroll
    bar and current-item marker after a horizontal/vertical side change.**
    - Cause: one `ScrollBar` moved between `ScrollBar.horizontal` and
      `ScrollBar.vertical` of the strip's `ListView`, with `null` in the
      other slot. Emptying a slot makes Qt hide and unparent the bar it held
      (`hideOldItem()`), so the bar was lost and its anchors read a null
      parent; the strip is not recreated on a side change.
    - `ThumbnailStrip.qml` now has an inline `StripScrollBar` (track colour,
      selection marker) and two instances, each attached to its own slot for
      the strip's lifetime; the bar of the other orientation is `AlwaysOff`
      (hidden by the style). No slot is ever emptied at run time.
    - Test: `test_scrollBarSurvivesAnOrientationChange` (Bottom -> Left ->
      Bottom -> Right -> Top on one strip) fails on the old QML and passes
      now; the `ThumbnailStrip.qml` TypeErrors are gone from the suite log.
      `test_deliveredThumbnailsAreDrawn` still fails in Debug, before and
      after the change (A-09).
  - **Post-audit fix A-07 (2026-10-13): the sorting and zoom messages of
    3.6.4 are back.**
    - `QuickMainWindowController::notifySortingChanged()` shows
      "Sorting: ..." in the document view and `notifyFolderSortingChanged()`
      shows "Folder Thumbnails: ..." in the folder view, as
      `MW::onSortingChanged()` / `onFolderSortingChanged()` did; Core's calls
      are unchanged.
    - `QuickViewerActions` runs the fit actions (fit window, width, height,
      1:1, toggle fit mode) through `runFitAction()`, which shows
      "Zoom temporary disabled" while the viewport ignores input (crop mode).
      `ImageViewportController` keeps its own guard; the crop mode's internal
      `fitWindow()` call shows nothing.
    - Translations: `lupdate` added the 13 `MW` entries again, and their
      translations were copied from `bb8df119` for all eight locales. The same
      run also added the A-03 and A-04 texts and dropped the CPU filter names
      left over after A-01.
    - Not tested automatically: neither adapter has a test fixture. Audit
      4.1 item 4 remains the check.
  - **Post-audit fix A-08 (2026-10-13): the riskiest claims have tests that
    fail when the guarded code is removed.** Each audit mutation was applied,
    `wynpix_tests` rebuilt and run (non-zero exit), and the file restored.
    - Render errors reach the GUI thread (S1.1): `tst_rendererrorchannel`
      posts from worker threads and records the sink's thread; it fails when
      `RenderErrorChannel::post()` delivers directly. The render suites
      still render on the test thread; a threaded render loop was not added.
    - One SDR conversion for concurrent readers (S2.1b):
      `concurrentReadersConvertOnce` releases its readers through a
      `std::latch` and holds the first conversion open for an overlap window,
      so readers without the `DecodedPixels::sdr()` lock convert too.
    - Modal wait ends on quit (S3.2):
      `theModalWaitIsAbandonedWhenTheApplicationQuits` runs
      `runModalDialog()` inside `QCoreApplication::exec()` and ends it with
      `QCoreApplication::exit()`.
    - LUT built on the pool (S1.4): `builderBuildsOnThePoolNotTheCallingThread`
      blocks the builder's one-thread pool; no table may appear before the
      pool is released.
    - HDR detection (S1.4, S2.1b): `tst_hdrsource` checks `isHdrImage()` and
      `detectHdrSourceEncoding()` against expectations written out from the
      AVIF/JXL plugin tags and from colour spaces, not against the shared
      code.
    - Stale batch scans (S3.4): `BatchQueueModel::waitForSourceScan()` (new,
      waits for the model's own scan pool) lets
      `queueDropsTheResultsOfAPreviousQueue` post the first queue's headers
      before switching to a file of other size and dimensions.
    - Slideshow after playback (`f84d3c42`):
      `playbackWithoutLoopReportsItsEnd` expects the controller's
      `playbackFinished` with looping off.
    - Identical `Wynpix.ini` (S3.3): the SettingsValues <-> Settings
      mapping moved unchanged from `AppSettingsStore` to
      `SettingsValueMapping` (`gui/settingseditor`), which depends on
      Settings only; `AppSettingsStore` delegates and keeps the shortcut and
      script saving. `tst_settingsvaluemapping` stores two value sets that
      differ in every field and compares the ini with the keys and values
      the 3.6.4 dialog wrote (`saveSettings()` at `bb8df119`; all keys
      checked in the 3.6.4 `settings.cpp`), then loads them back. The test
      binary's Settings writes `conf/`, `cache/` and `thumbnails/` next to
      `wynpix_tests.exe`, as the application does.
    - Still untested: `FileOpController` / `FileOperationTask`,
      `ColdStartWindowController` and `core.cpp` (audit A-08, last item).
  - **Post-audit fix A-09 (2026-10-13): the QML suite is stable in Debug and
    CTest shows the test logs and whole-suite skips.**
    - `test_deliveredThumbnailsAreDrawn` waited for the thumbnail request,
      which the selection makes synchronously, and then read the first
      delegate before the ListView had polished. It now waits for
      `cellOf(view, 0)`; 5 of 5 CTest runs of `wynpix_qml_tests` pass in
      Debug (149 / 149).
    - Cause of the empty logs: on Windows, `QPlainTestLogger` writes to
      `OutputDebugString` unless stderr has a console, and a CTest pipe is
      none. `wynpix_configure_test()` and the backend runs of
      `wynpix_render_tests` set `QT_FORCE_STDERR_LOGGING=1`, which keeps the
      log on stdout; `ctest -V` / `--output-on-failure` now show every
      suite of `wynpix_tests`, so its shared `-o` file is no longer needed.
    - The render runs have `SKIP_REGULAR_EXPRESSION` for a skip in
      `initTestCase` (backend not available), checked with Vulkan on the
      offscreen platform. A single skipped function, as the documented
      Vulkan `viewportGrabMatchesThePresentedImage`, leaves the run passed.
  - **Post-audit fix A-10 (Upscayl) and A-11 (2026-10-14): Upscayl tone-maps
    only its crop, off the GUI thread, and the SDR copy of a cached HDR
    image follows tone mapping changes.**
    - `HdrToneMapper::toSdr(hdr, params)` is the one CPU HDR -> SDR
      conversion (tone mapped when `params.enabled`, otherwise or on failure
      converted to integer sRGB), and `Settings::hdrToneMapParams()` the one
      mapping of the three HDR settings to its params. They replace
      `ImageStatic::sdrFromHdr` and the copy of it in
      `ThumbnailerRunnable`.
    - A-10: `Upscaler::triggerUpscaylProcessing` crops
      `getDecodedImage()`, which never converts, and passes the tone-map
      params with an HDR crop. `UpscalerRunnable::prepareCrop()` converts
      the crop and then applies the size cap (`kMaxCropDimension`, moved
      from `Upscaler`), in the old order; a failed conversion is reported
      through `onTaskFailed`. Exact because every operator works per pixel:
      `convertingACropGivesTheCropOfTheConvertedImage` compares the
      conversion of a crop with the crop of the conversion for the four
      operators, a changed white level and no tone mapping.
    - A-11: `DecodedPixels::sdr(params)` keeps the params of its last
      conversion and rebuilds the copy (dropping the old one) when asked
      for other ones; a failed conversion is tried again only for other
      params. `ImageStatic::getImage()` passes the current settings, so a
      cached image follows a change like the viewer. Test:
      `otherParamsRebuildTheSdrCopy`; with the key check removed, it and
      `aFailedConversionIsReportedOnce` fail.
    - Found by the new test and fixed: the fallback conversion (tone mapping
      off) kept the `HDR_*` text keys, because `convertToFormat()` copies
      all text and the keys were only added, never removed. `isHdr()` reads
      those keys, so an SDR copy, and a file saved from it, could be taken
      for HDR. `toSdr` now copies the pixels into a fresh image.
    - Still open (A-10): Ctrl+C / drag, save, print, wallpaper, edits,
      colour adjustments and AI resize build the full SDR copy on the GUI
      thread on first use. A fix needs an asynchronous SDR provider (a
      component that builds the copy on a pool and finishes the action
      through a queued call), with every caller handling an image that is
      changed, edited or unloaded meanwhile.
    - Kept: Core still reloads the current image on an HDR settings change;
      it is no longer needed for the SDR copy.
  - **Post-audit fix A-12 (2026-10-14): the dialog buttons are translated
    again.**
    - The six dialogs that used `standardButtons` (confirmation, file
      replace, resize, text input, shortcut creator, batch converter
      message) declare their own `Button`s with `qsTr` texts and a
      `DialogButtonBox.buttonRole`. The box still orders them for the
      platform, emits `accepted` / `rejected` by role and focuses the
      default button (`defaultButton` instead of `defaultStandardButton`).
      The file replace dialog tells its answers apart by button, as
      before; the shortcut creator binds OK's `enabled` directly.
    - Not done: shipping and loading Qt's base translations. The deploy
      step keeps `--no-translations`; Qt's own texts stay untranslated
      wherever they are still used.
    - Translations: `lupdate` added 12 entries per locale. Each locale
      takes its 3.6.4 `FileReplaceDialog` / `ResizeDialog` translations of
      Yes / No / Cancel / OK (`bb8df119`), for the restored entries and for
      the four contexts that never had them. Only the new entries were
      filled; existing ones (e.g. the batch converter's "Cancel") keep
      their translations.
    - Test: `tst_dialogs.qml` finds the buttons by role and fails while a
      dialog's box has standard buttons; with `standardButtons` put back
      in `ConfirmationDialog.qml`, `test_confirmationButtons` fails.
  - **Post-audit fix A-14 (2026-10-14): the viewer state is refreshed on
    the three edge paths.**
    - (a) A failed `showAnimation()` ends like `closeImage()`
      (`finishImageRemoval()`): zoom indicator and cursor are updated and
      `imageChanged()` is emitted when an image was shown, so the overlays,
      context menu and crop mode stop acting on the previous image.
    - (b) Leaving the panorama calls `requestScaling()` after the flat
      projection is set: the SVG raster and the upscale request follow
      even when the fit leaves the transform unchanged.
    - (c) When Upscayl is on and the settled view wants no upscale (1:1 or
      below, panorama, animation, SVG), the viewer emits
      `upscaleWithdrawn()` (`UiEvents::upscaleWithdrawn`), which Core
      connects to the new `Upscaler::cancelPendingPreview()`. It aborts
      only a debounced or running request; unlike `invalidatePreview()`
      (3.6.4) it emits nothing otherwise, because `upscaleAborted()` hides
      every notification and the signal fires on each settle below 1:1.
      `setUpscaledCrop()` drops a crop that arrives while
      `wantsUpscale()` is false; the upscaler's generation check already
      drops results of aborted requests.
    - Tests: `failedAnimationOpenReportsTheImageGone`,
      `leavingPanoramaSettlesTheFlatView`, `zoomingOutWithdrawsTheUpscale`;
      with the three changes reverted, all three fail. `Upscaler` has no
      unit test (its destructor tears down the global Upscayl instance);
      `cancelPendingPreview()` is covered by review only.
  - **Post-audit fix A-15 (2026-10-14): a broken animation no longer stops
    the slideshow.**
    - Measured first: a file-backed `QMovie` (Qt 6.12) decodes a
      truncated GIF without an error. Cut inside a frame, it shows the
      partial frame; cut after a frame's image descriptor, `frameCount()`
      still counts that frame, but `jumpToNextFrame()` succeeds and
      returns to frame 0 instead. Playback without looping therefore never
      reached the declared last frame and never reported
      `playbackFinished()`, and `showFrame()` of such a frame (stepping
      back from frame 0) looped forever on the GUI thread.
    - `AnimationPlayer` detects the wrap (the frame number did not
      increase). Playing, the decodable frames are the whole animation: it
      finishes without looping and continues from frame 0 with looping.
      Stepping, the unreachable frame is reported as a decode error.
    - `playbackError()` carries an `AnimationPlaybackError` (kind, file,
      1-based frame, decoder reason) instead of English text;
      `ImageViewportController` phrases it with `tr()` (no 3.6.4
      translation exists: the widget viewer showed no such message) and
      emits `playbackFailed()`. `QuickUiHost` forwards it to
      `UiEvents::playbackFailed`; Core starts the slideshow timer, so the
      broken file stays for one interval like a static image. The timer
      also keeps the advance out of the synchronous `guiSetImage()` call
      that reports an open failure. A failure never also reports
      `playbackFinished()`, so the slideshow cannot advance twice.
    - Tests: `truncatedFileEndsAfterItsDecodableFrames`,
      `truncatedFileLoopsItsDecodableFrames`,
      `steppingPastTruncatedEndReportsError` (it hangs without the fix),
      and `playbackFailed()` in `failedAnimationOpenReportsTheImageGone`.
      Core's timer start is covered by review only (no Core test harness).
  - **Post-audit fix A-16 (2026-10-14): a failed upload is not retried every
    frame, and each render error is reported once per image.**
    - `ImageLayer` remembers the key of a failed upload (generation, tile
      size limit, conversion) and skips `upload()` while the key matches;
      before, `release()` zeroed the uploaded tile size limit, so every
      frame re-created the tiles that succeed and failed again. A new
      image, another tile size limit or conversion, or `release()` (device
      rebuild) retries it.
    - `RenderErrorReporter` keeps the set of messages reported for the
      shown image instead of the last one. It is cleared when a layer
      starts uploading a new image (`LayerSource::animationFrame` false,
      new generation), no longer on every successful upload, so the
      frames of an animation, which are uploaded one by one, report a
      repeating failure once.
    - `RenderDevice::colorLut()` releases the replaced table with
      `deleteLater()` (`FrameTexture`), since commands of the frame being
      recorded may still read it.
    - Test: `untileableImageReportsError` checks that an animation frame
      failing the same way is not reported again and a new image is; the
      second check fails with the old reporter. The skipped retry and the
      LUT release are covered by review only (no counter of failed uploads;
      no current pipeline replaces the LUT inside one frame).
  - **Post-migration improvement (2026-10-14): the exact downsample reduces
    only the visible part of a tile.**
    - Before, `buildReducedTile()` reduced the whole tile even when only a
      part of it was on screen, so every settle below 1:1 (Bilinear, CAS,
      Smart) read the whole image. Each pass of the chain now keeps the
      whole tile's geometry (`RenderDevice::BoxPass`: source and destination
      grids, the source texture's grid origin, the rendered window;
      `dstOffset` / `srcOffset` in `boxreduce.frag`) and renders only the
      part of its grid that the next pass reads, so the texels are those of
      the whole tile's chain. The last window is the drawn texture rectangle
      plus `kReducedWindowMargin` (2) texels, since the bilinear sampling and
      the plain sharpening taps read one texel beyond the drawn ones. The
      cache serves while its window holds the drawn part.
    - Measured (temporary benchmark, D3D11, 2560 x 1440 frame, one settled
      frame with a new scale; GPU time in ms): RTX 3060, 102 MP at 0.75
      5.4 -> 1.1, at 0.5 2.9 -> 1.1, 24 MP at 0.75 1.9 -> 1.1; WARP, 102 MP
      at 0.75 552 -> 45, at 0.5 249 -> 45, 24 MP at 0.75 136 -> 45. A wholly
      visible image (fit) costs the same as before (102 MP: 3.7 ms on the
      3060, 318 ms on WARP).
    - Test: `partlyVisibleExactDownsample` (plain and CAS) compares a partly
      visible image at two pan positions with the CPU reference and the tiled
      image with the untiled one; with `kReducedWindowMargin` set to 0 the
      CAS row fails (18 levels at the window edge).
  - **Post-migration improvement (2026-10-14): MKS2021 (GPU) is drawn while
    zooming in.**
    - The viewer redraws once more 80 ms after the view stops: until then
      it draws the mip chain (bilinear above 1:1), then the exact downsample
      (below 1:1) or MKS2021. Measured per zoom frame (every cache rebuilt;
      GPU ms, frame ms in a Debug build): above 1:1 MKS2021 costs about
      +0.3 ms on the RTX 3060 and +20..28 ms on WARP, independent of the
      image size (its work is bounded by the screen); below 1:1 it costs
      2.8..8.8 ms on the 3060 and 70..480 ms on WARP for 24..102 MP at fit.
    - Rule (`layerFilter()`): above 1:1 MKS2021 is drawn without waiting
      for `settled` unless the device is a software rasterizer
      (`QRhiDriverInfo::CpuDevice`: WARP, which Qt Quick falls back to on
      D3D when the hardware device or swapchain cannot be created, or
      lavapipe / SwiftShader on Vulkan; `RenderDevice::hasLiveResamplePass()`)
      or the image is a frame of a playing animation
      (`LayerSource::animationFrame`; playback never settles, and the widget
      viewer never resampled it either). Below 1:1 nothing changes.
      `settled` and its 80 ms keep driving the SVG raster and the Upscayl
      request.
    - Rejected as too complex for a cosmetic gain: a pool of offscreen
      targets, a cost model with per-GPU budgets, and an adaptive fallback
      timed per frame (non-deterministic, and a quality switch in the middle
      of a gesture is itself a visible step).
    - Measured first, in Release: above 1:1 a resampled zoom frame takes
      2.2..2.5 ms longer than a bilinear one on the RTX 3060, of which
      0.3..0.4 ms on the GPU; the rest is the per-frame textures, render
      targets, bindings and weight tables (about 8 ms in Debug). Small
      enough to keep them per frame.
    - Tests: `mks2021AboveOneToOneIsDrawnWhileMoving` (resampled while
      moving and panned, unchanged once settled),
      `mks2021BelowOneToOneFollowsSettledState` (the former
      `mks2021FollowsSettledState`, now at 37%),
      `mks2021WaitsForSettleDuringPlayback`, and
      `mks2021WaitsForSettleOnSoftwareRasterizer` on a scene created with
      `QRhi::PreferSoftwareRenderer` (WARP on D3D11 and D3D12; skipped on
      Vulkan without a CPU device). With the software or the playback
      condition removed, the matching test fails.
  - **Post-audit fix A-18 (2026-10-14): leftovers, stale docs and
    build/deploy gaps.**
    - Removed as unused: the bridge fields `PanelSettings::showSubfolders`,
      `FolderViewSettings::sortFolders` and `showHiddenFiles` (Core and the
      settings editor read them from `Settings` directly),
      `Settings::hasUpscaylModels()`, `SharedResources` / `shrRes` (its
      `directorypresenter.h` include now names `<QPixmap>`), and the
      `QPixmap*` metatype registration in `main.cpp`.
    - Comments that described removed code as current now say what 3.6.4
      did or name the current code: `fileopcontroller.cpp` (no Scaler
      pool), `quickcropactions.h`, `quickfolderviewactions.h`,
      `imageviewportcontroller.h`, `thumbnailerrunnable.cpp` (no scaler),
      `appservices.cpp` (no IconWidget / StyledComboBox).
      `fileoptaskrunnable.h` no longer calls the blocking calls safe and
      points to the teardown hang P-1, which stays open.
    - `README.md`: the GPU badge names Direct3D 11 / 12 and Vulkan; the
      filter sections list the viewer's GPU filters (MKS2021 the default,
      CAS, Smart sharpen, Nearest, Bilinear) and the CPU filters of the
      batch converter and the resize dialog.
    - `CMakeLists.txt` looks up `Test` and `QuickTest` only with
      `WYNPIX_BUILD_TESTS=ON`.
    - Deploy: the post-build step removes `Qt6OpenGLWidgets.dll` and
      `Qt6SvgWidgets.dll` from reused output folders
      (`_unused_qt_libraries`), and the installer deletes them from an
      existing installation (`[InstallDelete]`).
    - Checked: the 35 app objects that include a changed header compile
      (Debug, cl run directly; the app does not link in Debug); all five
      CTest entries pass. The installer and a `WYNPIX_BUILD_TESTS=OFF`
      configure without the test packages were not run.
  - **Post-audit fix P-1 (2026-10-14): exiting during a move no longer
    hangs.** `FileOperationTask` reaches the GUI thread (`moveFileTo()`,
    `removeDir()`, the replace prompt) through `invokeOnGuiThread()`
    instead of `Qt::BlockingQueuedConnection`. The worker waits on a
    semaphore in `kGuiCallCancelPollMs` slices and re-checks the
    cancellation flag `~FileOpController()` sets after the event loop has
    ended; a call still queued then skips its work, and the result lives in
    shared state so an abandoned call never writes into the worker's stack.
    Cancelled steps return without reporting errors.
    - Not built or run; check with audit test step 9.

---

## 5. Cross-Cutting Rules for Every Stage

- **Threads:** decode, scale, LUT generation and thumbnailing stay on the
  existing pools. The render thread only consumes immutable snapshots
  handed over in `synchronize()`/`updatePaintNode()`. No `QQmlEngine`
  access from worker threads.
- **Startup:** heavy QML (folder view, dialogs, settings) behind
  `Loader { active: false; asynchronous: true }`; only the viewer and the
  messages overlay are created before the first frame (exception: a
  folder view created while the window is still hidden is created
  synchronously, see S4.2 "Post-audit fix: cold-start reveal"). Pipeline cache is
  loaded before the first window is exposed.
- **Errors:** RHI failures (`QRhi::isDeviceLost`, resource `create()`
  returning false, shader load failures) are reported through explicit
  signals to the owning controller and surfaced via `INotificationPort`.
- **Constants:** shader defaults, thresholds and timings are named
  `constexpr` values (C++) or `readonly property` values in a QML constants
  singleton.
- **Translations:** QML strings use `qsTr()`; contexts stay stable so
  existing `.ts` entries can be migrated.
- **Line endings:** new files are CRLF in the working tree; verify with
  `git ls-files --eol` after each stage.
- **Commits:** refactoring commits (Phase 0, S3.3a) separate from feature
  commits.

---

## 6. Stage Dependency Overview

```
S0.1 -> S0.2 -> S0.3 -> S0.4 -> S0.5
                          |       |
                          v       v
                S1.1 -> S1.2 -> S1.3 -> S1.4 -> S1.5 -> S1.6
                                                         |
                                                         v
                S2.1 -> S2.1b -> S2.2 -> {S2.3, S2.4, S2.5, S2.6}
                                                         |
                                                         v
                                  S3.1 -> S3.2 -> S3.3 -> S3.4
                                                         |
                                                         v
                                                  S4.1 -> S4.2
```

---

## 7. Deferred Follow-ups (not scheduled)

### F1. qmlcachegen AOT coverage audit

- **Context:** every Quick module (`wynpix_ui`, `wynpix_style`,
  `wynpix_bridges`, `wynpix_render`) is built with `qt_add_qml_module`, so
  qmlcachegen precompiles all QML to bytecode and compiles bindings and
  functions to C++ where their types can be resolved statically. Bindings
  it cannot resolve fall back silently to the QML interpreter/JIT; the
  build does not fail and `all_qmllint` stays clean, because the
  `compiler` warning category is not reported at its default level.
  `pragma ComponentBehavior: Bound` is present in 25 of 80 QML files.
- **Goal:** know which bindings and functions are not compiled to C++ and
  fix the ones on hot paths (viewer overlays, thumbnail delegates,
  folder view grid, panels shown during image switching).
- **Steps:**
  1. Run `qmllint` on each module with the `compiler` category raised to
     warning level and collect the per-binding reasons into this
     section.
  2. Classify by cause: untyped `var` properties and function
     parameters, unqualified access to outer scopes, missing
     `ComponentBehavior: Bound` in files with delegates, dynamic property
     access on untyped objects, context properties.
  3. Fix delegates and per-frame/per-switch bindings first; leave
     one-shot code (settings window, dialogs) unless the fix is trivial.
- **Constraints:** no behaviour change; refactoring-only commits separate
  from features. Verify cold start without a file argument and that
  `all_qmllint` stays clean.
- **Out of scope:** `qmltc` (see 3.3).
- **Delivered** (2026-10-14):
  - **Method:** the qmllint `compiler` category is not a reliable measure.
    Its 391 warnings are mostly "Member X of Y can be shadowed" (non-FINAL
    members of C++ and QML types), and qmlcachegen still compiles those
    bindings. The ground truth is the per-file `*.aotstats` that
    qmlcachegen writes with `--dump-aot-stats` (already passed by
    `qt_add_qml_module`) under `<module>/.rcc/qmlcache/`;
    `codegenResult` 0 means compiled to C++.
  - **Coverage:** 3005 of 3132 bindings and functions (96 %) were compiled
    before the fixes, 3030 of 3129 after them. The remaining 99 are listed
    below by cause.
  - **Fixed** (no behaviour change):
    - Style controls read their own properties through `parent`, typed as
      `QQuickItem` (`IconGlyph`, `PopupBackground`, `SpinStepIndicator`,
      `Slider`, `SearchField`, `MenuItem`); they now use ids. `IconGlyph`
      is in every button and menu item.
    - `ThumbnailWidget.dropHoverColor` (a delegate) and the
      `ImageViewport` zone arrow colours used `Qt.rgba()`, which returns
      `QVariant`; they are hex literals of the same 16-bit values.
    - `CropPanel.showSelection()` (runs on every selection change while
      dragging) reads all fields of the selection before writing the
      inputs, so the value type reference is not "affected by side
      effects".
    - `PlacesPanel` `columnWidthProvider` is a typed function, so its body
      is compiled (only the one-shot binding that creates the closure is
      not).
    - Unused imports removed from `FolderView`, `PlacesPanel` and
      `SettingsGeneralPage` (qmllint `unused-imports` infos).
  - **Left interpreted, by cause:**
    - *Qt 6.12.0 limitation, 29 entries:* any call of a method that
      returns `QColor` is rejected with "internal conversion with
      incompatible or ambiguous types: call to method X, returning QColor
      -> call to method X, returning QColor". This covers the new typed
      `Color.transparent/rgba/lighter/blend` singleton, `Qt.rgba/alpha/
      darker` (which return `QVariant`) and our own
      `labelTextColor()` invokables. Reproduced with a minimal file
      outside the project. The only form that compiles is an inline block
      such as `{ let c = Theme.colors.accent; c.a = opacity; return c }`
      (a helper function does not compile: write-back to a parameter). Not
      applied: the bindings are evaluated at creation and on theme
      changes, and the block form would replace a readable call in ~25
      places. Re-check after the next Qt update.
    - *Untyped model data, 28 entries:* `modelData` / `model` / `spec`
      of type `var` in `LinkedSliderSpin`, `AdjustmentSliders`,
      `SettingsRadioRow`, `SettingsComboBox`, `FormatFilterComboBox`,
      `SettingsDialog`, the `ComboBox`/`SearchField` style delegates.
      Mostly one-shot (constant specs). `AdjustmentSliders` re-reads
      `model.value` while dragging, but a typed `value` role would shadow
      `Slider.value`; one interpreted binding per slider is not worth the
      role rename.
    - *Event handlers and one-shot code, the rest:* `MainPanel` anchors
      chosen by a ternary (`QVariant` anchor lines), `PlacesPanel`
      functions that pass `QModelIndex` (stored as `QVariant`; run on a
      folder change), `ViewerContextMenu` `instanceof` (not implemented by
      qmlcachegen), drop / context menu handlers with "argument affected by
      side effects", `Qt.binding()` closures, `StyleConstants` and
      `CropOverlay` alpha colours (`Qt.rgba()`; a hex literal would round
      the alpha to 8 bits), settings pages.
  - **Re-check:** build `all_qmllint` (or the module targets) and sum
    `codegenResult != 0` over
    `out/build/<preset>/src/gui/quick/*/.rcc/qmlcache/*.aotstats`.
  - **Verified:** `all_qmllint` clean (no infos), `wynpix_tests` and
    `wynpix_qml_tests` pass (Debug), cold start without a file argument
    reaches "cold-start window revealed" with no warnings in the log.
