"""Drives standby (scenario 2) and minimized (scenario 4) raises of the debug
wynpix build while ffmpeg records the screen; --extra adds a visible window
(scenario 3) and three simultaneous launches from standby (scenario 6).
Writes events.txt with the time of each launch relative to the recording
start, app.log and rec.mkv into OUT.

    python -I run_raise.py <OUT dir> <runs per scenario> [--extra]

The debug config gets standbyMode=true and maximizedWindow=true for the run
and is restored byte for byte afterwards."""
import atexit
import ctypes
import ctypes.wintypes as wt
import os
import subprocess
import sys
import time

OUT = sys.argv[1]
N = int(sys.argv[2])
HERE = os.path.dirname(os.path.abspath(__file__))
EXE = r"E:\wynpix\out\build\wynpix-x64-debug\src\wynpix.exe"
CONF = r"E:\wynpix\out\build\wynpix-x64-debug\src\conf"
IMG = {
    "red": os.path.join(HERE, "imgs", "A", "red.png"),
    "blue": os.path.join(HERE, "imgs", "B", "blue.png"),
}
saved_conf = {}


def set_conf(name, old, new):
    path = os.path.join(CONF, name)
    data = open(path, "rb").read()
    saved_conf[path] = data
    open(path, "wb").write(data.replace(old, new))


def restore_conf():
    subprocess.run(["taskkill", "/IM", "wynpix.exe", "/F"],
                   stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    time.sleep(0.5)
    for path, data in saved_conf.items():
        open(path, "wb").write(data)


atexit.register(restore_conf)
set_conf("Wynpix.ini", b"standbyMode=false", b"standbyMode=true")
set_conf("savedState.ini", b"maximizedWindow=false", b"maximizedWindow=true")
user32 = ctypes.windll.user32
WM_CLOSE = 0x0010
SW_MINIMIZE = 6


def main_window(pid):
    found = []
    proc = ctypes.WINFUNCTYPE(wt.BOOL, wt.HWND, wt.LPARAM)

    def cb(hwnd, _):
        wpid = wt.DWORD()
        user32.GetWindowThreadProcessId(hwnd, ctypes.byref(wpid))
        if wpid.value == pid and user32.GetWindow(hwnd, 4) == 0:  # GW_OWNER
            buf = ctypes.create_unicode_buffer(256)
            user32.GetWindowTextW(hwnd, buf, 256)
            cls = ctypes.create_unicode_buffer(256)
            user32.GetClassNameW(hwnd, cls, 256)
            if "Qt" in cls.value and buf.value:
                found.append((hwnd, buf.value))
        return True

    user32.EnumWindows(proc(cb), 0)
    return found


os.makedirs(OUT, exist_ok=True)
env = dict(os.environ,
           QT_FORCE_STDERR_LOGGING="1",
           QT_LOGGING_RULES="wynpix.startup.info=true",
           QT_MESSAGE_PATTERN="%{time HH:mm:ss.zzz} %{message}")
ff = subprocess.Popen(
    ["ffmpeg", "-hide_banner", "-loglevel", "error", "-f", "gdigrab", "-framerate", "60",
     "-draw_mouse", "0", "-i", "desktop", "-vf", "scale=960:-1", "-c:v", "libx264",
     "-preset", "ultrafast", "-qp", "0", "-y", os.path.join(OUT, "rec.mkv")],
    stdin=subprocess.PIPE)
t0 = time.perf_counter()
events = open(os.path.join(OUT, "events.txt"), "w")


def ev(name):
    line = f"{time.perf_counter() - t0:.3f} {time.strftime('%H:%M:%S')} {name}"
    events.write(line + "\n")
    events.flush()


time.sleep(1.0)
log = open(os.path.join(OUT, "app.log"), "w")
current = "red"
ev("cold red")
primary = subprocess.Popen([EXE, IMG[current]], stdout=log, stderr=subprocess.STDOUT, env=env)
time.sleep(4.0)
wins = main_window(primary.pid)
ev(f"windows {wins}")
hwnd = wins[0][0]


def launch(scenario):
    global current
    current = "blue" if current == "red" else "red"
    ev(f"{scenario} launch {current}")
    subprocess.run([EXE, IMG[current]], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    ev(f"{scenario} second instance exited")
    time.sleep(1.8)


for i in range(N):
    ev("close")
    user32.PostMessageW(hwnd, WM_CLOSE, 0, 0)
    time.sleep(1.3)
    launch("s2")

for i in range(N):
    ev("minimize")
    user32.ShowWindow(hwnd, SW_MINIMIZE)
    time.sleep(1.3)
    launch("s4")

if "--extra" in sys.argv:
    for i in range(N):
        launch("s3")

    # Three launches at once from standby, as Enter on three selected files.
    green = os.path.join(HERE, "imgs", "C", "green.png")
    for i in range(N):
        ev("close")
        user32.PostMessageW(hwnd, WM_CLOSE, 0, 0)
        time.sleep(1.3)
        ev("s6 launch3 red,green,blue")
        procs = [subprocess.Popen([EXE, p], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
                 for p in (IMG["red"], green, IMG["blue"])]
        for p in procs:
            p.wait()
        ev("s6 second instances exited")
        time.sleep(2.0)
    current = "blue"

ev("end")
time.sleep(0.5)
ff.stdin.write(b"q")
ff.stdin.flush()
ff.wait(timeout=60)
primary.kill()
events.close()
