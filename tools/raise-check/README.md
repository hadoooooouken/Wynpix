# raise-check

Automated check of the window raise path (`Core::raiseWindow()`): does a raised
window ever show the previous image, and do simultaneous launches switch the
image in view. Not part of the build; a local tool for the debug preset.

Scenarios (numbering of Task 1 in core-tasks.md):

- s2: window closed to standby, launched with an image from another folder
- s4: window minimized, launched with another image
- s3: window on screen, launched with another image (`--extra`)
- s6: three launches at once from standby, as Enter on three files (`--extra`)

Test images are solid red, blue and green (`imgs/`), so every captured frame
is classified automatically.

## Run

Needs Python with Pillow and NumPy, ffmpeg on PATH. Takes over the screen for
about 3 minutes with `--extra`; do not use the PC meanwhile.

```
python -I tools/raise-check/diag.py apply        # temporary TMPDIAG timing log
<build preset wynpix-x64-debug, target wynpix>
python -I tools/raise-check/run_raise.py <out> 12 --extra
python -I tools/raise-check/classify.py <out>/rec.mkv <out>/frames.txt
python -I tools/raise-check/analyze.py <out>      # recording: bad runs per scenario
python -I tools/raise-check/reveals.py <out>/app.log  # log: latency, folding, early reveals
python -I tools/raise-check/gaps.py <out>/app.log     # log: per raise, visible vs new frame
python -I tools/raise-check/diag.py remove       # before committing
```

`run_raise.py` switches the debug config to standby + maximized for the run and
restores it afterwards. Use an `<out>` folder outside the repository (the
session scratchpad).

## Reading the results

- The recording is 60 fps; a flash shorter than 16.7 ms can be missed. The
  log is the proof: a window must become visible only after the frame of the
  last requested image (`reveals.py`: early reveals = 0; `gaps.py`:
  visible-minus-presented > 0).
- A run flagged bad without the old image in it may be a slow second
  instance (the request arrived after the 1.2 s analysis window); check
  events.txt and app.log before blaming the app.

## Baseline (debug build, 2026-10-31, 12 runs per scenario, fe23b156)

s2, s4, s3, s6: 0 bad; all 36 reveals after the new frame; raise visible
156-198 ms after the request; three simultaneous launches fold into one
reveal.
