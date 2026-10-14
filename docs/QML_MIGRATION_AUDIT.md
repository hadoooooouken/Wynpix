# Qt Widgets -> Qt Quick Migration: Independent Audit

Scope: `bb8df119` (Release 3.6.4) .. `f84d3c42` (branch `qml`), 32 commits,
614 files. Rulebook: `AGENTS.md`. Claims checked: every stage's "Delivered"
section of `docs/QML_MIGRATION_PLAN.md`, plus the two later commits
(`f5a5772c`, `f84d3c42`).

Method: seven read-only reviews by area (S4.2 removal and Core, renderer,
viewer, ownership and teardown, QML and view-models, settings and leftovers,
test integrity), each finding re-read in the code by the lead before it was
kept. Qt behaviour was settled from the Qt 6.12 sources in
`E:/Qt/6.12.0/Src`. Builds and tests: the `wynpix-x64-debug` preset (the
`wynpix-x64-release` preset outputs into `release/`, which currently holds a
widget-era deployment, so it was not rebuilt). Nothing in the repository
was changed except this file.

Test runs (Debug, 2026-10-12):

| Suite | Result |
|---|---|
| `wynpix_tests` (25 suites) | pass |
| `wynpix_render_tests` (D3D11) | pass, 192 / 192 |
| `wynpix_render_tests_d3d12` | pass |
| `wynpix_render_tests_vulkan` | pass, 191 + 1 documented skip (`viewportGrabMatchesThePresentedImage`) |
| `wynpix_qml_tests` | **fail**, 144 pass / 1 fail (A-09), 5 of 5 runs |
| `all_qmllint` | clean |

The application target itself does not link in the Debug preset
(`kimg_raw.dll` against the Release-only `libraw_static.lib`, hard-coded the
same way at the baseline), so no Debug app run was possible either.

---

## 1. Verdict

The migration is structurally sound. The Core/port boundary, the
render-thread hand-over in `synchronize()`, ownership and teardown order of
`QuickUiHost`, the task-identity checks of the new asynchronous components
(`SvgRasterizer`, `ColorLutBuilder`, `BatchQueueModel`), translation coverage
and line endings all hold up, and the GPU render suites pass on D3D11, D3D12
and Vulkan. It is not regression-free: one High defect silently downgrades
the viewer for every user upgrading from 3.6.4 (the stored filter value of
every 3.6.4 install now renders as plain trilinear while the settings still
name it), and seven Medium defects lose 3.6.4 behaviour or hide failures
(window opacity, renderer errors, sorting feedback, the strip scroll bar after
a position change, a save crash path, HDR memory, and tests that cannot fail
behind the riskiest claims). No Critical defect was found; the most dangerous
defect seen, an exit hang during a move, predates the migration (P-1).

---

## 2. Findings

Severity counts: Critical 0, High 1, Medium 7, Low 10. Pre-existing defects
that the migration carried over unchanged are listed separately (2.1) and are
not counted.

### A-01 Stored 3.6.4 filter values render as plain trilinear (High)

- **Status (2026-10-13): fixed.** The viewer offers GPU filters only
  (`ScalingFilterSelection::viewingFilters()`); `Settings::scalingFilter()`
  maps stored MKS2021 / Smart to their GPU counterparts on every read. See
  `QML_MIGRATION_PLAN.md`, S4.2 "Post-audit fix A-01". Smart remains an
  approximation until CPU Smart is ported to a shader.

- **Where:** `src/gui/quick/render/imagefiltermode.cpp:16-19`;
  offered in `src/components/settingseditor/settingsoptions.cpp:121-122`
  and in the filter cycle (`src/components/scalingfilter/scalingfilterselection.cpp:4-15`);
  no migration in `src/settings.cpp:1041-1046` or `Core::onUpdate()`
  (`src/core.cpp:768-779`).
- **What is wrong:** `QI_FILTER_SMART` and `QI_FILTER_MKS2021` map to
  `{Trilinear, Sharpening::None, Resampling::None}`, the same as Bilinear.
  3.6.4 wrote `setScalingFilter(QI_FILTER_MKS2021)` on first run
  (`bb8df119:src/core.cpp:860`) and used it as the default
  (`bb8df119:src/settings.cpp:1275`), so practically every existing
  `Wynpix.ini` holds `scalingFilter=5`. S4.2 removed the `Scaler` that
  gave values 2 and 5 their meaning, but kept both entries and added no
  remapping. `onUpdate()` would not run anyway: the project version is still
  3.6.4.0.
- **Failure scenario:** a 3.6.4 user upgrades, opens a 4000 x 3000 JPEG at
  fit-to-window: 3.6.4 showed it MKS2021-resampled; now it is mip-mapped
  trilinear, visibly softer. Settings still shows "Magic Kernel Sharp 2021"
  as selected. "Smart sharpen" looks identical to "Bilinear"; the filter
  cycle announces both as distinct filters. The settings combo box also
  truncates both MKS2021 entries to "Magic Kernel Sharp ...", so the dead
  CPU entry and the working GPU entry cannot be told apart.
- **Violates:** S1.6 claim "the GPU shows every filter"; S4.1 parity review;
  AGENTS §7 (regression safety, root cause first).
- **Fix direction:** one-time migration of stored values (MKS2021 ->
  MKS2021_GPU, SMART -> SMART_GPU); remove the CPU-only entries from the
  viewing filter list and the cycle (keep them for resize and batch).
- **Root cause (added 2026-10-13):** the gap is older than S4.2. S1.2 and
  S1.3 left Smart / MKS2021 as trilinear in the Quick viewer on purpose
  ("their `Scaler` display is S1.6 work if it is ever needed"); S1.6 then
  claimed "the GPU shows every filter" and made `showScaledImage()` a no-op,
  and S4.2 removed the `Scaler` (outside its listed scope) without
  revisiting the two values. The widget viewer hid the gap until then.
- **Caveat on the fix direction (added 2026-10-13):** only MKS2021 has a
  verified GPU equivalent (S1.3: same kernel within tolerance). CPU Smart
  (`ImageLib::scaled_Smart()`: bicubic resample + Gaussian unsharp mask) is
  a different algorithm from Smart (GPU) (shader sharpening over trilinear
  sampling); S1.3 records that it was never ported. SMART -> SMART_GPU is a
  visible approximation, not parity; restoring Smart needs a shader port or
  an explicit decision to drop it.
- **Confidence:** confirmed by code.

### A-02 "Window opacity" setting has no effect (Medium)

- **Status (2026-10-13): fixed, checked on the Release build** (desktop
  visible through the windowed background at 80 %). On Direct3D 11/12 the
  main window keeps a transparent colour, is created without a redirection
  surface, and `MainWindowShell::backgroundOpacity()` applies the setting
  in windowed document mode only. Vulkan stays opaque. See
  `QML_MIGRATION_PLAN.md`, S4.2 "Post-audit fix A-02". Found while fixing
  it: the viewer cleared to `background` in fullscreen too, hiding
  `background_fullscreen` behind the image; fixed with it.

- **Where:** offered at `src/gui/quick/ui/SettingsThemePage.qml:67-74`;
  bridged at `src/gui/quick/bridges/uisettings.h:41,67` and
  `src/gui/quick/adapters/bridgesnapshots.cpp:38`; never read (no QML or
  C++ reader of `viewer.backgroundOpacity`); the window colour is the opaque
  theme background (`src/gui/quick/ui/Main.qml:39-40`).
- **What is wrong:** 3.6.4 made the main window translucent
  (`bb8df119:src/gui/mainwindow.cpp:34-35`) and applied
  `bgColor.setAlphaF(settings->backgroundOpacity())` in windowed mode
  (`bb8df119:src/gui/viewers/imageviewerv2.cpp:297`). First run still
  writes 0.8 (`src/core.cpp:788`).
- **Failure scenario:** set Window opacity to 50 %, Apply, windowed mode:
  the window stays fully opaque.
- **Violates:** S4.1 "every `Settings` getter the widget UI reads" checked
  for parity; S3.3 (setting offered without effect).
- **Fix direction:** implement it (translucent `QQuickWindow` with an alpha
  background in windowed mode only) or remove the slider, the setting and the
  bridge field and record the change.
- **Confidence:** confirmed by code; that the 3.6.4 window was visibly
  translucent is on the manual list (4.1).

### A-03 Renderer errors and device loss never reach the user (Medium)

- **Status (2026-10-13): fixed.** `ImageViewportController` shows each
  `renderError` message once per image as an Error notification;
  `GraphicsDeviceLossMonitor` reports device loss (checked in
  `sceneGraphInvalidated` while the lost QRhi still exists) as a Warning.
  See `QML_MIGRATION_PLAN.md`, S4.2 "Post-audit fix A-03". A real device
  loss remains on the manual list (4.1 item 5).

- **Where:** `src/gui/quick/render/imagerenderitem.cpp:33-37` and
  `:437-443` emit `renderError`; no consumer exists in `src/` (only
  `QSignalSpy`s in `tests/render/tst_imagerenderer.cpp`). Nothing checks
  `QRhi::isDeviceLost()`.
- **What is wrong:** every failure the plan promised to surface ends in
  `qWarning()` and an unconnected signal: texture `create()` failures,
  untileable sizes, missing RGBA16F support for HDR, invalid colour targets
  (the ProPhoto preset, S1.4), LUT build failures.
- **Failure scenario:** choose the "ProPhoto" display profile (invalid
  `QColorSpace`, flagged in S1.4) or open an image whose tiles cannot be
  allocated: the image shows unmanaged or blank, with no message; a Release
  build has no console, so nothing is visible at all.
- **Violates:** plan §5 "Errors" (surfaced via `INotificationPort`), S1.1
  "never swallowed", AGENTS §8.
- **Fix direction:** connect `renderError` in `ImageViewportController` (the
  owner) to the notification port, deduplicated per image; report device loss
  (for example from `sceneGraphInvalidated`) the same way.
- **Confidence:** confirmed by code.

### A-04 Save dereferences a null image after a failed HDR-to-SDR conversion (Medium)

- **Status (2026-10-13): fixed.** `DirectoryModel::saveFile` returns
  `InvalidSourceImage` for a null SDR copy; `Core::edit_template`,
  `applyColorAdjustments` and `print` notify the user instead of failing
  silently, and an image that could not be edited is not saved. See
  `QML_MIGRATION_PLAN.md`, S4.2 "Post-audit fix A-04". The crash path is
  not covered by an automated test (no `DirectoryModel` fixture).

- **Where:** `src/components/directorymodel.cpp:303`
  (`FileOperations::saveImage(*imgStatic->getImage(), ...)`);
  `src/sourcecontainers/imagestatic.cpp:256-258`;
  `src/sourcecontainers/decodedpixels.cpp:24-29, 55-68`.
- **What is wrong:** since S4.2 every static HDR image builds its SDR copy
  lazily; when the conversion fails, `sdr()` returns `nullptr` permanently
  and `saveFile` dereferences it. At the baseline the conversion ran at load
  and always left a non-null pointer. The same null makes edits
  (`Core::edit_template`) silently do nothing.
- **Failure scenario:** a very large HDR image whose 4 B/px SDR copy cannot
  be allocated (`HdrToneMapper` falls back to `convertToFormat`, which also
  fails) -> Save, Crop & save, or a folder-view rotate with save -> crash.
- **Violates:** AGENTS §8 (validate pointers; errors bubble up, not just
  `qWarning`).
- **Fix direction:** null-check in `DirectoryModel::saveFile` and return a
  save error; report a failed conversion to Core, which notifies the user.
- **Confidence:** dereference path confirmed by code; the trigger needs an
  allocation failure (a test with a failing `SdrConversion` would confirm).

### A-05 HDR images keep 2-4x the memory of 3.6.4, bounded only by count (Medium)

- **Status (2026-10-13): fixed, not yet measured.** Preloading is bounded
  by a 1.5 GiB budget of decoded pixels (`PreloadMemoryBudgetBytes` in
  `directorymodel.cpp`, sized for the 16 GB target machine). The current
  image always loads; the budget left after the cached images
  (`Cache::decodedBytes()`, HDR source plus a built SDR copy) is split evenly
  between the neighbours still to load, and the loader skips a neighbour
  over its share from the file header before decoding, or from the decoded
  size when the header gives no dimensions (PDF, DjVu). A skipped neighbour
  loads normally when it becomes current. 24 MP `.hdr`/`.exr` folders still
  preload both neighbours; a 50 MP `.hdr` folder keeps only the current
  image (about 0.8-1 GB instead of 2.4-2.8 GB). Rejected: decoding Radiance
  `.hdr` as half float (`HDR_HALF_QUALITY`) clips values above 65504 that
  3.6.4 tone mapped from float32; dropping the SDR copy of non-current
  images saves about 13% and leaves the HDR sources unbounded. Open:
  measure an HDR folder (manual list 4.1).
- **Where:** `src/sourcecontainers/decodedpixels.cpp:8-22` (HDR source
  kept for the image's cache life; the SDR copy, once built, is never
  dropped); cache bound by count, current/previous/next
  (`src/components/directorymodel.cpp:228-236`); decoder formats
  `src/3rdparty/kimageformats/hdr.cpp:401-403` (RGBX16FPx4 / RGBX32FPx4,
  8-16 B/px), `exr.cpp:226` (RGBA16FPx4, 8 B/px).
- **What is wrong:** 3.6.4 kept one tone-mapped 4 B/px copy per image. Now
  every cached HDR image keeps its 8-16 B/px source plus, after the first
  CPU use (copy, Upscayl, save), the 4 B/px copy; the GPU also keeps one HDR
  source texture per HDR image (S1.4). The S4.2 memory table measured only a
  JPEG.
- **Failure scenario:** browsing a folder of 24 MP Radiance `.hdr` files
  holds about 3 x (384 + 96) MB = 1.4 GB of decoded pixels, against about
  290 MB in 3.6.4; with the 2048 MB allocation limit the worst case is
  several GB before textures.
- **Violates:** S2.1b user decision ("lazily built CPU copy rather than
  keeping both copies": after first use both stay); S4.2 "Startup and memory"
  not measured for the case it changed.
- **Fix direction:** drop the SDR copy when an image stops being current, or
  bound the cache by bytes; measure an HDR folder.
- **Confidence:** retention confirmed by code; byte figures calculated from
  the decoder formats, not measured (manual list 4.1).

### A-06 Thumbnail strip loses its scroll bar after a horizontal/vertical switch (Medium)

- **Status (2026-10-13): fixed.** The strip has one scroll bar per
  orientation, each attached to its own slot for the strip's lifetime; the
  bar of the other orientation is `AlwaysOff`. See `QML_MIGRATION_PLAN.md`,
  S4.2 "Post-audit fix A-06". The line numbers below refer to the old code.

- **Where:** `src/gui/quick/ui/ThumbnailStrip.qml:92-93`
  (`ScrollBar.horizontal: strip.horizontal ? scrollBar : null`,
  `ScrollBar.vertical: ...`) and `:147-150` (anchors on `parent`).
- **What is wrong:** setting an attached `ScrollBar` slot to `null` makes Qt
  hide and unparent the bar (`QQuickControlPrivate::hideOldItem()`); the
  other slot does not restore its visibility, and the anchors then read a
  null parent. The strip is not recreated on a position change, so the state
  persists.
- **Evidence from the test run:** the TypeErrors
  (`ThumbnailStrip.qml:147-150: Cannot read property ... of null`) are logged
  inside `test_pinnedPanelTakesItsSide(left/right)` and
  `test_extendedStyleShowsTheLabels` (Left), not at teardown. The plan's
  diagnosis (S2.5 and S3.1 "Flagged": only while QML tests tear their windows
  down) is wrong.
- **Failure scenario:** Settings -> panel position Bottom -> Left -> Apply:
  the strip's scroll bar and current-item marker disappear until restart.
- **Violates:** S2.4 (panel positions).
- **Fix direction:** never null an attached slot at run time: two bars, one
  per orientation, or a strip recreated per orientation.
- **Confidence:** confirmed by code and test log; the visible effect is on
  the manual list.

### A-07 User feedback messages of 3.6.4 were dropped (Medium)

- **Status (2026-10-13): fixed.** `QuickMainWindowController` announces
  sorting changes in the view they apply to, `QuickViewerActions` reports
  "Zoom temporary disabled" for the fit actions in crop mode, and the `MW`
  translations of 3.6.4 are restored. See `QML_MIGRATION_PLAN.md`, S4.2
  "Post-audit fix A-07". The line numbers below refer to the old code.

- **Where:** `src/gui/quick/adapters/quickmainwindowcontroller.cpp:199-205`
  (sorting notifications only update the folder view);
  `src/gui/quick/ui/imageviewportcontroller.cpp` fit actions return
  silently while interaction is off.
- **What is wrong:** `MW::onSortingChanged` / `onFolderSortingChanged`
  showed "Sorting: By Name (desc.)", "Folder Thumbnails: Newest", ...
  (`bb8df119:src/gui/mainwindow.cpp:329-355`), and the fit actions showed
  "Zoom temporary disabled" in crop mode (`:238-276`). S1.6 deferred the
  latter to S2.3/S2.5; neither delivered it. The S4.2 `lupdate -no-obsolete`
  then dropped the translated entries.
- **Failure scenario:** in document view press the sort-by-name shortcut
  twice: the order flips between ascending and descending with no indication.
  In crop mode, keys 1-4 / Space do nothing and say nothing.
- **Violates:** S4.1 parity review ("Ports: all implemented" holds formally,
  the behaviour does not); S1.6 deferral not closed.
- **Fix direction:** emit the messages through `INotificationPort` from the
  shell adapter and the viewer actions in the `MW` context, restoring the old
  translations.
- **Confidence:** confirmed by code.

### A-08 The riskiest plan claims rest on tests that cannot fail (Medium)

- **Status (2026-10-13): fixed for the eight named claims.** Each mutation
  below now fails `wynpix_tests`; checked by applying it, rebuilding and
  running the suite. `AppSettingsStore`'s value mapping moved to
  `SettingsValueMapping` (Settings only), which a golden-ini test covers. The
  "Untested components" item stays open. See `QML_MIGRATION_PLAN.md`, S4.2
  "Post-audit fix A-08". The line numbers below refer to the old code.

Each item: the claim, the test, and a mutation of production code that still
passes.

- **Render-thread discipline (S1.1):** every render test renders on the test
  thread through `QQuickRenderControl` (`tests/render/offscreenquick.cpp:145-158`),
  so "render thread" and "GUI thread" are the same. Mutation: make
  `RenderErrorChannel::post()` call `deliver()` directly
  (`src/gui/quick/render/rendererrorchannel.cpp:11-32`).
- **Identical `Wynpix.ini` from both UIs (S3.3):** the widget UI is
  gone; `AppSettingsStore` (`src/gui/settingseditor/appsettingsstore.cpp`,
  379 lines) is compiled into no test target; `SettingsEditorTests` round-trip
  through a fake that copies the value object. Mutation: delete any setter
  in `AppSettingsStore::apply()`.
- **One SDR conversion for concurrent readers (S2.1b):**
  `tests/unit/tst_decodedpixels.cpp:76-94` starts eight threads one after
  another with no latch around a microsecond conversion. Mutation: remove the
  `QMutexLocker` in `DecodedPixels::sdr()`.
- **Modal wait abandoned when the application quits (S3.2):**
  `tests/unit/tst_dialogs.cpp:152-159` calls `abandon()` itself, so the
  `QCoreApplication::exit()` branch (`dialogsession.cpp:64-68`) never runs.
  Mutation: delete that branch.
- **LUT never built on the GUI or render thread (S1.4):** no test records the
  building thread. Mutation: run the build lambda inline instead of
  `mPool->start()` (`colorlutbuilder.cpp`).
- **HDR matches the CPU (S1.4, S2.1b):** the reference `hdrtonemapper.cpp`
  ("unchanged" per the plan) now uses the same `utils/hdrsource.h` detection
  as the renderer, and the test images never set the `HDR_*` text keys.
  Mutation: map a transfer or primaries case wrongly in `hdrsource.h`.
- **Stale batch scans dropped (S3.4):** `tst_batchprint.cpp:206-217` queues
  identical files, so stale and fresh results are indistinguishable.
  Mutation: delete the generation check (`batchqueuemodel.cpp:213-214`).
- **Slideshow after animated playback (`f84d3c42`):** no test at all.
  Mutation: delete the `playbackFinished` forwarding connect in
  `imageviewportcontroller.cpp`, the exact bug the commit fixed.
- **Untested components:** `FileOpController` / `FileOperationTask`,
  `ColdStartWindowController` and `core.cpp` are in no test target.
- **Violates:** the acceptance criteria named above; AGENTS §7 (lifetime and
  identity claims without evidence).
- **Fix direction:** thread-identity assertions on a threaded render loop;
  an `AppSettingsStore` golden-ini test against the 3.6.4 keys; latches that
  force overlap; a test that calls `QCoreApplication::exit()`; distinct files
  in the batch queue test; a viewport test for `setLoopPlayback(false)` ->
  `playbackFinished`.
- **Confidence:** confirmed by code (each test and production line re-read).

### A-09 QML suite fails in Debug; CTest hides skips and output (Low)

- **Status (2026-10-13): fixed.** `test_deliveredThumbnailsAreDrawn` waits
  for the delegate; the CTest runs set `QT_FORCE_STDERR_LOGGING=1`, so the
  Qt Test log reaches the captured output, and a render run skipped in
  `initTestCase` is reported as skipped. See `QML_MIGRATION_PLAN.md`, S4.2
  "Post-audit fix A-09". The line numbers below refer to the old code.

- **Where:** `tests/qml/tst_thumbnailstrip.qml:177-180`;
  `tests/render/tst_imagerenderer.cpp:634-636`; `tests/CMakeLists.txt:24-28`,
  `tests/render/CMakeLists.txt:39-43`.
- **What is wrong:**
  - `test_deliveredThumbnailsAreDrawn` fails 5 of 5 in Debug: the
    `tryVerify(thumbnailRequestCount > 0)` is already true on its first check
    (the fixture's `focusOn()` requests synchronously,
    `thumbnaillistmodel.cpp:224`), so the ListView has not polished and
    `cellOf(view, 0)` is null. `test_pressActivatesTheItemUnderThePointer`,
    which waits for the cell, passes: a test synchronization defect, not a
    product one. S4.2's "all suites pass" holds only for the Release run.
  - A backend the machine cannot create hits `QSKIP` in `initTestCase`; with
    no `SKIP_REGULAR_EXPRESSION`/`SKIP_RETURN_CODE`, CTest reports
    `wynpix_render_tests_d3d12` / `_vulkan` as Passed with zero checks.
  - The test executables write nothing to captured stdout, so
    `ctest --output-on-failure` printed an empty log for the failing suite;
    `wynpix_tests` reopens `-o file` per suite, keeping only the last one.
- **Fix direction:** wait on `cellOf(view, 0) !== null`; mark skips for
  CTest; make the runners log to the console or one file per suite.
- **Confidence:** confirmed by test run and code.

### A-10 First CPU use of an HDR image tone-maps it on the GUI thread (Low)

- **Status (2026-10-14): partly fixed.** Upscayl no longer builds the full
  SDR copy: it crops the decoded pixels and `UpscalerRunnable` tone-maps
  only the crop on the upscaler pool. Ctrl+C, save, print, wallpaper, edits
  and AI resize still build the full copy on the GUI thread on first use
  (open follow-up). See `QML_MIGRATION_PLAN.md`, S4.2
  "Post-audit fix A-10 (Upscayl) and A-11".

- **Where:** `src/components/upscaler/upscaler.cpp:731` (GUI-thread timer
  slot, under `stateMutex`) -> `ImageStatic::getImage()` ->
  `DecodedPixels::convertLocked()` -> `HdrToneMapper::applyToneMapping()`,
  which blocks the caller on a semaphore (`src/utils/hdrtonemapper.cpp:868-877`).
  Same for Ctrl+C, save and edits.
- **What is wrong:** 3.6.4 tone mapped on the loader thread at load; now the
  whole image is tone mapped on the GUI thread on first use.
- **Failure scenario:** Upscayl on, first zoom past 1:1 on a 24 MP `.hdr`, or
  the first Ctrl+C: the UI stalls for the full-image tone map.
- **Violates:** AGENTS §6 spirit (heavy work off the GUI thread); S2.1b
  "an upscale runs no CPU scale" (it now runs a CPU tone map instead).
- **Fix direction:** build the SDR copy on a worker and finish with a queued
  call; for Upscayl tone-map only the crop.
- **Confidence:** path confirmed by code; stall length not measured.

### A-11 Cached HDR images keep an SDR copy with old tone mapping (Low)

- **Status (2026-10-14): fixed.** The SDR copy is keyed on the tone-map
  parameters and rebuilt when it is asked for with other ones. See
  `QML_MIGRATION_PLAN.md`, S4.2 "Post-audit fix A-10 (Upscayl) and A-11".

- **Where:** `src/sourcecontainers/decodedpixels.cpp:24-29` (built once),
  `src/sourcecontainers/imagestatic.cpp:211-216` (reads the settings at
  build time), `src/core.cpp:582-584` (only the current image is reloaded).
- **Failure scenario:** HDR image A, Ctrl+C (builds A's copy), go to B,
  toggle tone mapping, back to A (cache hit): the GPU shows the new setting,
  while Ctrl+C, save, print, wallpaper and Upscayl use the old one.
  3.6.4 was stale too, but display and copy matched.
- **Violates:** S2.1b "the reload also refreshes the lazily built SDR copy"
  (current image only); AGENTS §7 (narrowest invalidation).
- **Fix direction:** key the SDR copy on the tone-map parameters, or
  invalidate the copies of all cached images on an HDR settings change.
- **Confidence:** confirmed by code.

### A-12 Dialog standard buttons are no longer translated (Low)

- **Status (2026-10-14): fixed.** The dialogs declare their own buttons
  with `qsTr` texts and button roles; the 3.6.4 translations are restored.
  See `QML_MIGRATION_PLAN.md`, S4.2 "Post-audit fix A-12".

- **Where:** `standardButtons` in `src/gui/quick/ui/FileReplaceDialog.qml:108`,
  `ResizeDialog.qml:218`, `TextInputDialog.qml:46`,
  `ShortcutCreatorDialog.qml:87`, `ConfirmationDialog.qml:44`,
  `BatchConverterDialog.qml:611`; `src/apptranslator.cpp` installs only the
  application translator; the deploy step passes `--no-translations`.
- **What is wrong:** standard-button texts come from Qt's `QPlatformTheme`
  context, for which no translation is loaded. 3.6.4's file-replace and
  resize dialogs used their own `tr("Yes")`, `tr("No")`, `tr("Cancel")`,
  `tr("OK")` (`bb8df119:src/gui/dialogs/filereplacedialog.cpp:65-71`,
  `resizedialog.cpp:288-291`), translated in every shipped `.ts`.
- **Failure scenario:** German UI, copy collision: the dialog shows
  "Yes / No / Cancel" instead of "Ja / Nein / Abbrechen".
- **Violates:** plan §5 "Translations" (contexts stay stable).
- **Fix direction:** explicit buttons with `qsTr` in the dialogs' contexts
  (restore the entries), or ship and load the Qt base translations.
- **Confidence:** confirmed by code and the `.ts` files.

### A-13 Closing the window to exit leaves it visible during teardown (Low)

- **Status (2026-10-13): fixed** (`18814806`, `Core::forceExit()` hides the
  window first). Found after it: exit left the process running once any
  dialog window had been opened, because `quit()` gave up when the hidden
  dialog refused to close; `forceExit()` now calls
  `QCoreApplication::exit()`. See `QML_MIGRATION_PLAN.md`, S4.2
  "Post-audit fix A-13 and exit hang".

- **Where:** `src/gui/quick/adapters/quickmainwindowcontroller.cpp:248-253`
  (`event->ignore(); onCloseRequested();`).
- **What is wrong:** 3.6.4 accepted the close event before invoking exit
  (`bb8df119:src/gui/mainwindow.cpp:782-784`), which hid the window at
  once. Now the window stays on screen, frozen, while `~Core` waits for the
  thumbnailer, cache writer and a running AI resize.
- **Failure scenario:** standby off, close button during thumbnail
  generation: the window remains visible and unresponsive ("Not responding"
  on a long wait) until teardown ends.
- **Violates:** S2.1 "window close ... as `MW::closeEvent`".
- **Fix direction:** hide the window on the exit path before quitting.
- **Confidence:** confirmed by code.

### A-14 Viewer state is not refreshed on three edge paths (Low)

- **Status (2026-10-14): fixed.** A failed animation open emits
  `imageChanged()`, leaving the panorama settles the flat view, and the
  view withdraws a pending AI upscale below 1:1 and drops late crops. See
  `QML_MIGRATION_PLAN.md`, S4.2 "Post-audit fix A-14".

- **(a) Failed animation open:** `src/gui/quick/ui/imageviewportcontroller.cpp:418-421`
  returns after `reset()` without `imageChanged()`; the overlays, context
  menu and crop (`quickoverlayactions`, `quickcontextmenuactions`,
  `quickcropactions`) keep treating the previous image as displayed. Scenario:
  navigate from an image to a GIF that `QMovie` cannot open: blank viewer,
  copy/move overlay and image menu rows still enabled.
- **(b) Leaving panorama:** `imageviewportcontroller.cpp:955-957` only calls
  `applyFitMode()`; when the transform is unchanged nothing calls
  `requestScaling()`, so a zoomed SVG stays a blurry scaled decode until the
  next pan or zoom (S2.6 "raster once the view settles").
- **(c) Upscale not aborted below 1:1:** `wantsUpscale()` requires a scale
  above 1:1 (`imageviewportcontroller.cpp:655-668`), so zooming out to fit
  sends nothing to Core; 3.6.4 called `upscaler->invalidatePreview()` there
  when the Upscayl limit was enabled (`bb8df119:src/core.cpp:2080-2081`).
  An AI run in flight finishes and its crop is drawn over the fitted image.
- **Violates:** IViewerPort state consistency; AGENTS §7 (validate identity
  before applying an async result).
- **Fix direction:** emit `imageChanged()` on the failure path; request
  scaling after leaving panorama; tell Core when the view leaves the upscale
  range and reject crops that do not match the latest request.
- **Confidence:** confirmed by code.

### A-15 The slideshow fix `f84d3c42` is incomplete (Low)

- **Status (2026-10-14): fixed.** A playback failure reaches Core
  (`UiEvents::playbackFailed`), which starts the slideshow timer; a
  truncated GIF ends after its decodable frames; the message is translated
  in `ImageViewportController`. The failure scenario below is not what
  `QMovie` does: see `QML_MIGRATION_PLAN.md`, S4.2 "Post-audit fix A-15".

- **Where:** `src/components/animationplayer/animationplayer.cpp:178-182`
  (a frame that fails to decode pauses without `playbackFinished`);
  `src/gui/quick/quickuihost.cpp:157-160` (`playbackError` goes only to the
  message overlay); `src/core.cpp:2337-2342` (no slideshow timer for
  animations).
- **Failure scenario:** a slideshow reaches a truncated GIF: playback pauses
  on the broken frame and the slideshow never advances. (3.6.4 never advanced
  past any animation, so this is incompleteness, not a regression.) The error
  text shown, `"Cannot open the animation %1: %2"` / `"Cannot decode frame
  ..."` (`animationplayer.cpp:29, 222`), is untranslated.
- **Fix direction:** report playback failure to Core through `UiEvents` and
  advance; translate the message at the UI boundary.
- **Confidence:** confirmed by code.

### A-16 Renderer failure handling repeats work and reports (Low)

- **Status (2026-10-14): fixed.** A failed upload is remembered by key and
  not retried until the image, tile size limit or conversion changes; the
  reporter keeps the messages reported for the shown image; the replaced
  LUT is released with `deleteLater()`. See `QML_MIGRATION_PLAN.md`, S4.2
  "Post-audit fix A-16".

- **Failed upload retried every frame:** the failure paths store the
  generation, but `ImageLayer::release()` zeroes `mUploadedTileSizeLimit`
  (`src/gui/quick/render/imagelayer.cpp:289`), so `needsUpload()` is true
  again (`:310`): every pan or zoom re-creates the tiles that succeed and
  fails again.
- **Deduplication is per consecutive message only**
  (`rendererrorreporter.cpp:5-8`), and a successful upload clears it, so an
  animation can report the same error every frame (S1.1 "once per distinct
  message").
- **Latent lifetime hazard:** `RenderDevice::colorLut()` destroys the previous
  LUT texture immediately (`renderdevice.cpp:473`, `unique_ptr`), although a
  texture used by commands of the current frame must be released with
  `deleteLater()`. No trigger in the current pipeline: the image and the
  upscaled crop resolve to the same LUT (the upscaler copies the colour
  space, `upscaler.cpp:415-416`).
- **Violates:** AGENTS §7 (narrowest operation, physical lifetime).
- **Fix direction:** remember a failed upload key; keep a set of reported
  messages per image; release the replaced LUT with `deleteLater()`.
- **Confidence:** confirmed by code; the stutter is not measured.

### A-17 AGENTS.md violations in new code (Low)

- **Manual `delete`** (§6): `src/gui/quick/render/imagelayer.cpp:361`
  (`createTexture()` returns a raw owning `QRhiTexture*`, deletes it on
  failure).
- **Magic numbers** (§5): slider ranges `-300, 300, 0, 300, -100, 100, ...`
  in `src/gui/quick/ui/overlays/coloradjustmentseditor.cpp:20-26`.
- **Logic in QML** (§2): `src/gui/quick/ui/CopyOverlay.qml:104` builds the
  folder URL as `"file:///" + target.directory`; a folder such as
  `D:/C# projects` parses as `D:/C` plus a fragment, so the picker opens in
  the wrong place. The model should expose a `QUrl`.
- **Fix direction:** `std::unique_ptr<QRhiTexture>`; named range constants;
  `QUrl::fromLocalFile` in the model.
- **Confidence:** confirmed by code.
- **Status (2026-10-14): fixed.** `createTexture()` returns
  `std::unique_ptr<QRhiTexture>`; the slider ranges are named constants;
  `CopyTargetsModel` exposes `directoryUrl` and takes a `QUrl` in
  `setDirectory()`, covered by `copyTargetEditsArePublished` with a `#`
  folder.

### A-18 Leftovers, stale docs and build/deploy gaps (Low)

- Dead after S4.2: bridge fields `PanelSettings::showSubfolders`,
  `FolderViewSettings::sortFolders`, `showHiddenFiles`
  (`src/gui/quick/bridges/uisettings.h`); `Settings::hasUpscaylModels()`;
  `SharedResources` / `shrRes` (created in `appservices.cpp`, no consumer);
  `qRegisterMetaType<QPixmap *>` (`src/main.cpp:106`).
- Comments describing deleted code as current, against the S4.2 claim that
  they were corrected: `components/fileoptask/fileopcontroller.cpp:9-10`
  ("Scaler's ... pools"), `gui/quick/adapters/quickcropactions.h:23`,
  `quickfolderviewactions.h:29`, `gui/quick/ui/imageviewportcontroller.h:33`,
  `components/thumbnailer/thumbnailerrunnable.cpp:132-135` ("the scaler"),
  `components/fileoptask/fileoptaskrunnable.h:78` (blocking call "safe
  here", see P-1).
- `README.md:3` (OpenGL GPU badge), `:153-159` ("MKS2021 (CPU, Default)",
  CAS "runs through OpenGL shaders", "Standard CPU Filters").
- `CMakeLists.txt:36-43`: `Test` and `QuickTest` are REQUIRED even with
  `WYNPIX_BUILD_TESTS=OFF`.
- Deploy: `Qt6OpenGLWidgets.dll` / `Qt6SvgWidgets.dll` survive in reused
  output folders (flagged in S4.2) and the installer packages `release\*`
  without `[InstallDelete]` (`src/wynpix.iss:113`), so upgrades keep
  them.
- **Confidence:** confirmed by code.
- **Status (2026-10-14): fixed.** The dead fields, function, class and
  metatype are removed; the listed comments, the README and the
  `appservices.cpp` comment are corrected (P-1 is fixed separately);
  `Test` / `QuickTest` are required only with tests on; the post-build step
  and the installer (`[InstallDelete]`) remove the two widget DLLs. See
  `QML_MIGRATION_PLAN.md`, S4.2 "Post-audit fix A-18".

### 2.1 Pre-existing defects carried over (not counted)

Found during the review, identical at `bb8df119`; listed because they break
AGENTS.md rules and the migration touched the code around them.

- **P-1 Exit hangs during a move or a pending replace prompt (High).**
  `FileOperationTask` calls the GUI thread with `Qt::BlockingQueuedConnection`
  once per moved file (`fileoptaskrunnable.cpp:247-250`) and for replace
  prompts (`:87-90`); `~FileOpController` runs `pool->waitForDone()`
  (`fileopcontroller.cpp:32-35`) after the event loop ended, before the
  pending metacall can run. Quitting during a move blocks the process
  forever. The header comment calling the blocking call safe
  (`fileoptaskrunnable.h`) is wrong for teardown. AGENTS §7.
  **Status (2026-10-14): fixed.** The task no longer uses
  `Qt::BlockingQueuedConnection`: `invokeOnGuiThread()` in
  `fileoptaskrunnable.cpp` queues the call, keeps its result in shared state
  and waits with `QSemaphore::tryAcquire()`, re-checking the cancellation
  flag every `kGuiCallCancelPollMs`. `~FileOpController()` sets the flag, so
  the worker stops waiting and a call still queued skips the move, the
  directory removal or the prompt. Files moved before the exit stay moved.
- **P-2 Modal loop re-entrancy.** `Core::moveCurrentFile` reads
  `selectedPath()` again after the overwrite confirmation
  (`core.cpp:1518-1520`); the slideshow timer or a single-instance hand-off
  inside the nested loop changes it, and the next file is moved with
  `force=true`. `Core::copyCurrentFile` and `Core::renameCurrentSelection`
  repeat the pattern: the retry copies, or renames, a file the user never
  confirmed and overwrites the destination. The delete and trash commands
  are not affected; they capture `currentSelection()` before the prompt.
  **Status (2026-10-14): fixed.** The three commands keep `selectedPath()` in
  a local `sourcePath` before the first attempt, and the forced retry and
  the move's image restore use it.
- **P-3** `Core` reloads the current image on an HDR settings change
  (`core.cpp:582-584`), dropping unsaved edits.
  **Status (2026-10-14): fixed.** The reload is removed. It was redundant:
  the viewer tone maps the decoded pixels with the current settings
  (`ImageViewportController::applyDisplayColor()`), and `DecodedPixels`
  rebuilds the CPU SDR copy when the tone mapping parameters change. Edited
  pixels keep the mapping they were made with; after Discard the image is
  shown with the new one.
- **P-4** The upscaled crop is accepted by path only (`core.cpp:304-305`), so
  a same-path edit during a run shows a pre-edit crop.
  **Status (2026-10-14): fixed.** `Upscaler` keeps the decoded buffer the
  active task's crop was cut from and reports it in `UpscaleResult`; Core
  shows the crop only while `getDecodedImage()` of the current file is still
  that buffer. An edit, a discard or a reload replaces the buffer; a save
  keeps it, so a run that spans a save is still shown.
- **P-5** `DirectoryPresenter::composeFolderThumbnail` draws the folder cover
  into the folder pixmap at a fractional rectangle (the window rectangle is
  derived from `kFolderIconRenderedScale` and the `kFolderWindow*` fractions)
  with `SmoothPixmapTransform`, and shrinks the full-size cover thumbnail with
  one bilinear pass. Both soften the cover before the view draws it; snapping
  the target rectangle to whole device pixels and downscaling the cover with
  `Qt::SmoothTransformation` beforehand would keep its detail.
  **Status (2026-10-14): fixed.** The cover is composed in device pixels:
  it is resized once with `Qt::SmoothTransformation` to its device size
  (only when larger than the window, so small covers keep their native
  size) and copied 1:1 at a whole-pixel position without
  `SmoothPixmapTransform`. Antialiasing now affects only the rounded clip
  edge. The defect predates the migration (present in `bb8df119`).

---

## 3. Plan claims checked

| Stage | Acceptance criterion / claim | Result | Evidence |
|---|---|---|---|
| S0.1 | Release and Debug presets build; CTest runs | Partly | Debug: test targets build, app does not link `kimg_raw` (pre-existing Release-only libraw); 5 CTest entries registered and run |
| S0.2 | Zero manual `delete` in `main.cpp`; behaviour unchanged | Met | `main.cpp` read; `AppServices` releases in baseline order |
| S0.3 | `core.h` includes only port headers from `gui/` | Met | `core.h` includes `gui/ports/uiports.h` only |
| S0.4 | `ImageViewerV2` behaviour identical; tests cover fit modes, locks, DPR | Met | Side-by-side comparison of `viewtransform.cpp` with `bb8df119:imageviewerv2.cpp`; tests pass |
| S0.5 | Property notification tests; `qmllint` passes | Met | `wynpix_qml_tests` bridges cases pass; `all_qmllint` clean |
| S1.1 | Pixel parity at 100/50/25/300 %, 32000 x 8000 without CPU fallback | Met | Render suites pass on D3D11/D3D12/Vulkan (Debug) |
| S1.1 | No GUI-thread access from the renderer | Met by code, not tested | `imagerenderer.cpp` reads the item only in `synchronize()`; tests are single-threaded (A-08) |
| S1.1 | Errors reported, never swallowed; once per distinct message | Not met | No consumer of `renderError` (A-03); consecutive-only dedup (A-16) |
| S1.1 | New QRhi (window change, device loss) re-uploads | Partly | Outcome holds (Qt recreates the renderer); device loss is never detected or reported |
| S1.2 | Visual parity for every GPU filter mode, three backends | Met | Render suites pass |
| S1.3 | GPU MKS2021 matches the CPU kernel; no `scalingRequested` traffic | Met | Render suites pass; `scalingRequested` removed |
| S1.3 | Settle latency not worse than CPU | Not verifiable here | Measured once by the implementer; no Release app run possible |
| S1.4 | HDR samples match the CPU output | Partly | Tone math independent; detection shared between reference and renderer (A-08) |
| S1.4 | Operator / white level switch without re-decoding | Partly | Renderer re-converts; Core still reloads the file (`core.cpp:582-584`) |
| S1.4 | LUT generation never on the GUI or render thread | Met by code, not tested | `ColorLutBuilder` pool + identity checks; no thread assertion (A-08) |
| S1.5 | Animation playback timing matches the widget viewer | Not verifiable here | Manual list |
| S1.5 | Colour-managed playback allocates nothing per frame | Partly false | No image textures, but each conversion pass creates a render target, pass descriptor, UBO and SRB (`renderdevice.cpp:555-568`) |
| S1.6 | Every viewer action works; `IViewerPort` fully implemented | Partly | All wired; "Zoom temporarily disabled" never delivered (A-07) |
| S2.1 | Cold start / first frame not worse than the widget UI | Not verifiable here | No Release build of this branch; implementer measurements only |
| S2.1 | Window close as `MW::closeEvent` | Partly | Window stays visible on exit (A-13) |
| S2.1b | HDR looks the same as CPU tone mapping; no CPU scale for an upscale | Partly | Pixel tests pass; a full CPU tone map now runs on the GUI thread (A-10) |
| S2.1b | Copied viewport matches the screen | Met | `viewportGrabMatchesThePresentedImage` passes on D3D11/D3D12 (skipped on Vulkan) |
| S2.2 | Gallery in both schemes; runtime theme switch | Met | `tst_style.qml` passes |
| S2.3 | No overlay instantiated at startup | Partly | `CropOverlay` is created unconditionally (`Main.qml:63-66`); cost negligible |
| S2.4 | 10 000 items at refresh rate; cold-start readiness | Partly | Bounded delegates tested; frame rate not measured for 10 000; scroll bar lost on orientation change (A-06) |
| S2.5 | Menu beyond window edges; shortcuts match `ActionManager` | Met in tests | `tst_contextmenu.qml`; never opened by hand (manual list) |
| S2.6 | SVG parity; no GUI-thread stall | Partly | Worker raster with identity checks; not re-rastered after panorama (A-14) |
| S3.1 | Folder view parity checklist | Mostly | Keys and menus match baseline; focus restore unverified (manual list) |
| S3.2 | `IDialogPort` fully implemented by the Quick UI | Met | All methods implemented; button translations lost (A-12, fixed) |
| S3.3 | Identical `Wynpix.ini` from either UI | Not verifiable | Widget UI deleted; `AppSettingsStore` untested (A-08); every 3.6.4 setting is offered, but Window opacity has no effect (A-02) |
| S3.4 | Batch progress does not stall; printing identical; map overlay parity | Met by code | Per-row updates, worker header reads; shared `ImagePrintSetup`; no real printer used |
| S4.1 | `qmllint` clean | Met | Re-run: clean |
| S4.1 | Parity: every `Settings` getter the widget UI read | Not met | A-01, A-02, A-07 |
| S4.2 | No `QWidget` subclass remains | Met | No widget includes or classes |
| S4.2 | All suites pass | Not met in Debug | A-09 |
| S4.2 | Startup and memory measured | Partly | JPEG only; HDR memory changed and not measured (A-05) |
| S4.2 | Comments describing removed code as current corrected | Partly | A-18 |
| S4.2 | Dropped translations are unused | Literally true | The dropped `MW` sorting and zoom entries hid lost behaviour (A-07, A-12) |

---

## 4. Manual test list

Run the Release build of `qml`. Each step: action -> expected result.

1. **Filter migration (A-01):** copy a 3.6.4 `Wynpix.ini` with
   `scalingFilter=5`, open a 4000 x 3000 JPEG at fit, compare with
   "Magic Kernel Sharp 2021 (GPU)" selected -> identical sharpness (today it
   is softer).
2. **Window opacity (A-02):** Window opacity 50 %, windowed mode -> desktop
   visible through the background, as in 3.6.4.
3. **Strip orientation (A-06):** panel Bottom -> Left -> Apply, open a folder
   with more images than fit -> scroll bar and current-item marker visible.
4. **Sorting feedback (A-07):** document view, sort-by-name shortcut twice
   -> "Sorting: By Name" then "Sorting: By Name (desc.)". Crop mode, key 1 ->
   "Zoom temporary disabled".
5. **Renderer errors (A-03):** select the ProPhoto display profile -> a
   visible message. Force a GPU reset (`dxcap -forcetdr` with the Graphics
   Tools feature) -> the image reappears, a message is shown.
6. **HDR memory and stall (A-05, A-10):** folder of 24 MP `.hdr` / EXR files,
   step through five images, note private bytes and dedicated VRAM against
   3.6.4; press Ctrl+C on a fresh image -> no visible UI stall.
7. **Stale copy (A-11):** HDR image A, Ctrl+C, next image, toggle tone
   mapping, back to A, Ctrl+C, paste into an editor -> matches the screen.
8. **Dialog language (A-12):** German UI, copy a file onto an existing one
   -> "Ja / Nein / Abbrechen"; resize dialog -> "OK / Abbrechen".
9. **Exit (A-13, P-1):** standby off, close during thumbnail generation ->
   window disappears at once. Start a large move to another drive and exit
   -> process ends within a moment; the files moved so far are at the
   destination, the rest at the source.
10. **Folder view focus:** type in the name filter, switch to the viewer,
    return to the folder view, press arrows -> grid selection moves (not the
    text cursor).
11. **Viewer edges (A-14):** open a broken GIF after an image -> no image
    rows enabled in the context menu; zoomed SVG, panorama on and off -> SVG
    sharp again without panning; Upscayl limit 200 %, zoom 300 %, fit during
    "AI Upscaling..." -> no AI crop appears.
12. **Slideshow (A-15, f84d3c42):** slideshow over a folder with a normal
    GIF, a truncated GIF and stills -> advances past each animation after one
    loop and past the broken one.
13. **Copy overlay path (A-17):** copy target `D:\C# test` -> folder picker
    opens in that folder.
14. **Never verified by hand per the plan (S2.3-S3.4):** context menu opens
    beyond the window edge; crop mode with mouse and keys; every dialog
    (confirm, replace, resize, text input, save, settings, batch, print);
    printing on a real printer; drag-out to Explorer and drop onto the
    window; floating panel hover show/hide; trackpad and pinch zoom; wheel
    scrolling of 10 000 thumbnails at display refresh rate.
15. **Timing claims (S1.3, S2.1, S4.2):** cold start to "first document
    rendering settled" against 3.6.4 on the same machine with
    `QT_LOGGING_RULES="wynpix.startup.info=true"`.
16. **Animation timing (S1.5):** GIF / WebP / APNG / animated SVG play at the
    same speed as in 3.6.4.

---

## 5. Not covered

- **Running the application:** the Release preset writes into `release/`,
  which holds a widget-era deployment from another branch, and the Debug app
  target does not link (`kimg_raw`). Every runtime claim is therefore either
  a test result or on the manual list.
- **Measurements:** startup, memory, VRAM, frame times and settle latency
  were not re-measured.
- **qmlcachegen AOT coverage:** not checked by the audit. Done afterwards
  (2026-10-14) as F1 in section 7 of `docs/QML_MIGRATION_PLAN.md`: 3030 of
  3129 bindings and functions are compiled to C++; the rest and their
  causes are listed there.
- **Shader numerics:** parity of `convert.frag` constants with
  `hdrtonemapper.cpp` and of `panorama.frag` with the deleted widget shader
  beyond what the render tests compare.
- **Third-party code:** `3rdparty/`, `upscayl-ncnn`, the upscaler's internal
  threading, `shellex`, the installer script beyond its file list.
- **Detailed math** of `ThumbnailListModel` scrolling, `CropSelection`,
  layout helpers and `FormatFilterModel`: skimmed, behaviour taken from the
  tests.
- **Translation quality:** only presence and contexts were checked. The
  unfinished strings (95 to 100 per locale after a fresh `lupdate`) were
  translated afterwards (2026-10-14); no native-speaker review.
- **Linux/macOS branches** in `components/CMakeLists.txt` (pre-existing,
  out of scope for the Windows-only target): removed afterwards
  (2026-10-14); the `UNIX` branch listed watcher sources that no longer
  exist.
