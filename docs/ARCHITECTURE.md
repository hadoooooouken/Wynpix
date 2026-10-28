# wynpix: Architecture and Development Notes

What still applies from the finished Qt Widgets to Qt Quick migration, the
typography work and the SSE2 build work, the measured memory of large HDR
images, and the issues still open. The plans and the migration audit
themselves are in the git history.

## 1. Structure

```
main.cpp
 └─ QApplication (QtWidgets is kept for QPrintDialog / QPrinter only)
     ├─ AppServices   owns the long-lived services (std::unique_ptr)
     ├─ Core          mediator; talks to the UI through the ports only
     ├─ UI ports      src/gui/ports: INotificationPort, IDialogPort,
     │                IViewerPort, IShellPort, IWindowPort, IViewModePort
     │                (outbound) and UiEvents (inbound signals)
     └─ QuickUiHost   implements the ports with the Qt Quick UI
          ├─ wynpix.ui       application shell (QML + controllers)
          ├─ wynpix.style    the controls' look (Button, ComboBox, ...)
          ├─ wynpix.bridges  Theme, Settings, Actions, ... for QML
          └─ wynpix.render   ImageRenderItem (QQuickRhiItem) and its shaders
```

The QML modules are static libraries linked into `wynpix.exe`.

Principles:

1. **No business logic in QML.** QML binds properties, lays out items, runs
   visual animations and forwards user intent to C++. Decisions (what to
   load or save, validation, error handling) live in C++ components, for
   example `components/settingseditor` for the settings window.
2. **Core gains no responsibilities.** New features are separate `QObject`
   components behind explicit interfaces.
3. **One GPU path.** Every pixel on screen goes through the Qt Quick scene
   graph on QRhi (Direct3D 11 by default). Image filtering, tone mapping,
   colour management and downsampling are RHI shaders.
4. **Render-thread discipline.** Qt Quick on Windows uses the threaded render
   loop. Renderer objects (`QQuickRhiItemRenderer`, `QSGNode`s) never touch
   `Core`, `Settings` or components. Data crosses only in `synchronize()` /
   `updatePaintNode()`, while the GUI thread is blocked, as immutable values
   (`std::shared_ptr<const QImage>`, parameter structs).

## 2. Rules

- **Threads:** decoding, scaling, LUT generation and thumbnailing stay on
  their pools. No `QQmlEngine` access from worker threads.
- **Memory:** two budgets bound decoded pixels. Both are sized for 16 GiB
  of RAM and scaled to the RAM of the machine (`scaledToInstalledMemory`,
  `utils/decodebudget.h`); large HDR and EXR folders are expected on
  machines with 32 GB of RAM and 8 GB of VRAM. Preloading stops when the
  cache holds 1.5 GiB at 16 GiB, at most 8 GiB (`PreloadBudgetAt16GiB`,
  `directorymodel.cpp`). Thumbnails from readers that cannot scale on read
  (`.hdr`, `.exr`, PNG) share 1 GiB at 16 GiB, at most 4 GiB, of full-size
  decodes, counted with the copy `QImage::smoothScaled()` converts them to
  (`FullDecodeBudgetAt16GiB`, `thumbnailerrunnable.cpp`). Both use the
  header estimate.
- **Startup:** heavy QML (folder view, dialogs, settings) sits behind
  `Loader { active: false; asynchronous: true }`; only the viewer and the
  messages overlay exist before the first frame. A folder view created while
  the window is still hidden is created synchronously. The pipeline cache is
  loaded before the first window is exposed.
- **Errors:** RHI failures (device loss, a failed `create()`, a shader that
  does not load) reach the owning controller through explicit signals and
  the user through `INotificationPort`.
- **Constants:** thresholds, timings and shader defaults are named
  `constexpr` values in C++ or `readonly property` values in a QML constants
  singleton (`StyleConstants`).
- **Translations:** QML strings use `qsTr()` with stable contexts. Translated
  strings are whole sentences with `%1` arguments, never concatenated pieces.

## 3. Theme and Style

- **Colours:** `Theme.colors` holds the colour scheme entries
  (`themestore.h`) under camelCase names and the colours derived from them:
  the accent states and the text on the accent (`gui/accentcontrast.h`), the
  translucent fills and strokes of neutral controls (`gui/neutralfills.h`)
  and the selection tints (`gui/selectionfills.h`). `Theme.tokens`
  (`gui/semantictokens.h`) names the same colours by role (surface, stroke,
  fill, accent, text, icon, status); most QML still reads `Theme.colors`.
- **Fixed sizes and factors:** `StyleConstants` (corner radii, paddings,
  opacities, durations). Sizes that scale with the font come from
  `Theme.metrics`.
- **Text:** `Theme.fonts` has four roles named after the Windows 11 type
  ramp: `caption`, `body`, `bodyStrong` and `title` (`gui/uimetrics.h`).
  Body is the Windows system font size (9 pt, 12 px), not the Fluent 14 px
  body, which reads too large and thin at 100 % scaling; a larger Windows
  text size scales the ramp up. Segoe UI Variable is used where Windows has
  it (Windows 11), Segoe UI otherwise. Apart from the pixel sizes of icon
  font glyphs, QML sets no font sizes or weights of its own; plain body
  text sets no font at all.

## 4. QML Compilation

Every module is built with `qt_add_qml_module`, so qmlcachegen compiles the
QML ahead of time; bindings it cannot type fall back silently to the
interpreter. qmllint's `compiler` category is not a reliable measure. The
ground truth is the `*.aotstats` files under
`out/build/<preset>/src/gui/quick/*/.rcc/qmlcache/`: an entry with
`codegenResult` 0 was compiled to C++.

Known limitation of Qt 6.12.0: a call of a method that returns `QColor`
(`Color.transparent()`, `Qt.rgba()`, `Qt.darker()`, ...) is never compiled.
Re-check after a Qt update.

## 5. SIMD Code and the SSE2 Build

The release ships an AVX2 build and an SSE2 build from one source tree; the
instruction set is a build parameter (`BUILDING.md`, "Dual AVX2 / SSE2
Release Procedure").

- On MSVC, removing `/arch:AVX2` is not enough: `_mm256_*` intrinsics still
  compile without it and crash on a CPU without AVX2. Every hand-written
  AVX2 path needs a compile-time alternative.
- MSVC defines `__AVX2__` only under `/arch:AVX2`; that macro selects the
  code path. ISA `#if` blocks are allowed: the codebase avoids conditional
  compilation for the OS (it targets Windows only), not for the ISA.
- The minimum CPU of the SSE2 build is Intel Core 2 (Merom: SSE3 and SSSE3,
  no SSE4.1). Code outside `__AVX2__` guards uses nothing newer than SSSE3.
- Every shipped DLL must be built for the same ISA: one AVX2 dependency is
  enough to crash the SSE2 build on an old CPU.
- **Maintenance rule:** a change to a function inside an
  `#if defined(__AVX2__)` block is applied to its `#else` branch in the same
  commit. The intrinsics live in `utils/imagelib.cpp`,
  `utils/hdrtonemapper.cpp`, `3rdparty/kimageformats/hdr.cpp` and
  `3rdparty/djvudec/djvu.c`.

## 6. Large HDR and EXR Images

Decoded sizes: `.hdr` 16 bytes per pixel (`RGBX32FPx4`), `.exr` 8 bytes per
pixel. The memory budgets are in section 2. Measured on 2026-10-29 (Release
build, 32 GB of RAM, folders of five images, thumbnail cache empty), peak
private memory:

| Folder        | First open | Rapid navigation | Steady state |
|---------------|-----------:|-----------------:|-------------:|
| 24 MP `.exr`  |     3.5 GB |           3.5 GB |  1.3–1.6 GB  |
| 50 MP `.exr`  |     3.2 GB |           3.2 GB |  1.5–2.4 GB  |
| 24 MP `.hdr`  |     3.1 GB |           3.1 GB |  1.3–2.1 GB  |
| 50 MP `.hdr`  |     3.5 GB |           5.1 GB |  1.9–3.6 GB  |

GPU memory is about 0.5 GB for a 24 MP image and 1 GB for 50 MP; the idle
app uses 0.2 GB. Without the thumbnail budget, the first open peaked at
4.8, 9.3, 3.1 and 5.8 GB.

- **Rapid navigation** is not bounded: the current image always loads,
  outside the preload budget, and a `.hdr` / `.exr` read already inside
  `QImageReader::read()` runs to the end when its load is cancelled, so up
  to `MaxLoaderThreads` (3) decodes overlap. Kept: 5 GB fits the 32 GB
  machines. If it must be bounded, the loader's full-size decodes go under
  a `DecodeBudget` as well.
- **Images over 16384 px** are downscaled for display (`kMaxDimension`,
  `imagestatic.cpp`), but the `.hdr` and `.exr` readers cannot scale on
  read: Qt decodes the full image first, and the decode must fit
  `memoryAllocationLimit`. Its default is 2048 MB on 16 GiB of RAM, scaled to
  the machine in steps of 512 MB (`defaultMemoryAllocationLimitMB`,
  `settings.cpp`); a value saved in the ini is kept.

  | Panorama `.hdr`  | Decoded  | Opens with                | Peak / loaded |
  |------------------|---------:|---------------------------|--------------:|
  | 16384 × 8192     | 2048 MiB | any RAM                   | 6.9 / 4.7 GB  |
  | 20480 × 10240    |   3.2 GB | 32 GB of RAM              | 6.7 / 4.7 GB  |
  | 24576 × 12288    |   4.5 GB | 64 GB, or a raised limit  |  not measured |

  Both use 2.5 GB of GPU memory. `ScaledSize` support in the `.hdr` and
  `.exr` readers, which read scan lines and can downscale them as they go,
  would lift the limit and cut the peak.

## 7. Open Issues

- **HDR tone mapping on the GUI thread.** The first CPU use of an HDR image
  (copy to the clipboard, save, print, wallpaper, edits, AI resize) tone-maps
  the whole image on the GUI thread. Upscayl tone-maps only its crop, on the
  upscaler pool. Fix direction: build the SDR copy on a worker and finish
  with a queued call.
- **Untested components.** `FileOpController` / `FileOperationTask`,
  `ColdStartWindowController` and `core.cpp` are in no test target.
- **`check-isa.ps1` gaps:**
  - The dependencies rebuilt for SSE2 (`kimg_avif`, `kimg_jxl`, `kimg_heif`,
    `kimg_ff`, `jpeg62`, `zlib1`, `deflate`, `zstd`) are allowlisted as whole
    files, so one built for AVX2 by mistake passes.
  - A failing `dumpbin` reports `[OK]`: its exit code and the number of
    parsed lines are not checked.
  - The mnemonic regex also matches mnemonics made of hex letters (`add`,
    `adc`, `dec`); the byte group should be `(?:[0-9A-F]{2} )+` with a
    case-sensitive `-cmatch`.
  - AVX code in prebuilt static libraries (`kimg_exr`, `kimg_jph`,
    `kimg_raw`) is reported as CRT dispatch without proof; only the SDE run
    covers these plugins.
- **Two colour APIs.** QML reads `Theme.colors`; `Theme.tokens` duplicates
  about twenty of its colours by role and is used in a few places. Either
  move QML to the tokens and drop the duplicates from `Theme.colors`, or
  drop the unused tokens.
