import sys, re
def t(s):
    h, m, rest = s.split(":")
    return (int(h) * 3600 + int(m) * 60 + float(rest)) * 1000
rows = []
cur = None
for line in open(sys.argv[1], encoding="utf-8", errors="replace"):
    m = re.match(r"(\d\d:\d\d:\d\d\.\d+) TMPDIAG (.*)", line.strip())
    if not m:
        continue
    ts, msg = t(m.group(1)), m.group(2)
    if msg.startswith("raiseWindow ") and "after" not in msg and "native" not in msg:
        cur = {"start": ts, "path": msg.split()[1].split("/")[-1]}
        rows.append(cur)
    elif cur is None:
        continue
    elif "iconic" in msg:
        cur["iconic"] = "iconic true" in msg
        cur["delayed"] = "delayed true" in msg
    elif msg.startswith("SW_RESTORE"):
        cur["restore"] = ts
    elif msg.startswith("framePresented") and "presented" not in cur and msg.endswith(cur["path"]):
        cur["presented"] = ts
    elif msg.startswith("setWindowConcealed false was true"):
        cur.setdefault("uncloak", ts)
    elif msg.startswith("raiseWindow native"):
        cur["native"] = ts
    elif msg.startswith("DwmFlush begin"):
        cur["flush0"] = ts
    elif msg.startswith("DwmFlush end"):
        cur["flush1"] = ts
for r in rows:
    vis = r.get("uncloak") if r.get("delayed") else r.get("restore", r.get("native"))
    p = r.get("presented")
    if vis is None or p is None:
        print("incomplete", r)
        continue
    print(f'{r["path"]:9} delayed={r.get("delayed")!s:5} iconic={r.get("iconic")!s:5} '
          f'visible-minus-presented={vis - p:+5.0f} ms  load->presented={p - r["start"]:4.0f} ms'
          + (f'  restore-minus-presented={r["restore"] - p:+4.0f} ms' if "restore" in r else "")
          + (f'  DwmFlush={r["flush1"] - r["flush0"]:.0f} ms' if "flush1" in r else ""))
