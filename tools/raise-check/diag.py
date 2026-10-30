"""Adds or removes the temporary TMPDIAG timing log of the window raise path.

    python -I diag.py apply    # before building the debug variant to measure
    python -I diag.py remove   # before committing; leaves other edits alone

Each edit replaces one exact block; a block that is not found once aborts
the whole run without writing anything (the code moved: update the block)."""
import sys

ROOT = r"E:\wynpix\src"
EDITS = [
    ("core.cpp",
     "  m_lastRaiseRequestTimer.start();\n",
     "  qWarning().noquote() << \"TMPDIAG raiseWindow\" << pathReceived << \"active\" << false\n"
     "                       << \"visible\" << ui.window.isWindowVisible();\n"
     "  m_lastRaiseRequestTimer.start();\n"),
    ("core.cpp",
     "  if (IsIconic(hwnd)) {\n    ShowWindow(hwnd, SW_RESTORE);\n  }\n",
     "  qWarning() << \"TMPDIAG raiseWindow after drain+showGui; delayed\" << needsDelayedReveal\n"
     "             << \"iconic\" << bool(IsIconic(hwnd)) << \"settled\" << ui.viewer.isRenderingSettled();\n"
     "  if (IsIconic(hwnd)) {\n    ShowWindow(hwnd, SW_RESTORE);\n"
     "    qWarning() << \"TMPDIAG SW_RESTORE done\";\n  }\n"),
    ("core.cpp",
     "    if (FAILED(DwmFlush()))\n"
     "      qWarning() << \"DwmFlush failed; revealing the raised window anyway\";\n",
     "    qWarning() << \"TMPDIAG DwmFlush begin\";\n"
     "    if (FAILED(DwmFlush()))\n"
     "      qWarning() << \"DwmFlush failed; revealing the raised window anyway\";\n"
     "    qWarning() << \"TMPDIAG DwmFlush end\";\n"),
    (r"gui\quick\ui\imageviewportcontroller.cpp",
     "void ImageViewportController::onFramePresented() {\n",
     "void ImageViewportController::onFramePresented() {\n"
     "    qWarning().noquote() << \"TMPDIAG framePresented\" << mFilePath;\n"),
    (r"gui\quick\ui\imageviewportcontroller.cpp",
     "    syncDevicePixelRatio();\n    const QSize oldSize",
     "    qWarning().noquote() << \"TMPDIAG showImage\" << filePath;\n"
     "    syncDevicePixelRatio();\n    const QSize oldSize"),
    (r"gui\quick\adapters\quickmainwindowcontroller.cpp",
     "void QuickMainWindowController::setWindowConcealed(bool concealed) {\n",
     "void QuickMainWindowController::setWindowConcealed(bool concealed) {\n"
     "    qWarning() << \"TMPDIAG setWindowConcealed\" << concealed << \"was\" << windowCloaked\n"
     "               << \"visible\" << window.isVisible();\n"),
]


def main():
    if len(sys.argv) != 2 or sys.argv[1] not in ("apply", "remove"):
        sys.exit(__doc__)
    apply = sys.argv[1] == "apply"
    files = {}
    for name, original, instrumented in EDITS:
        path = ROOT + "\\" + name
        if path not in files:
            files[path] = open(path, newline="").read()
        text = files[path]
        nl = "\r\n" if "\r\n" in text else "\n"
        old, new = (original, instrumented) if apply else (instrumented, original)
        old, new = old.replace("\n", nl), new.replace("\n", nl)
        if text.count(old) != 1:
            sys.exit(f"{name}: block found {text.count(old)} times, nothing written:\n{old}")
        files[path] = text.replace(old, new)
    for path, text in files.items():
        open(path, "w", newline="").write(text)
    print(("applied" if apply else "removed"), len(EDITS), "edits")


main()
