"""Groups the TMPDIAG log of a run_raise.py --extra run by window reveal.

    python -I reveals.py <OUT dir>/app.log

Prints how long after the request each standby (s2) and minimized (s4)
raise became visible, how many requests each reveal of the three
simultaneous launches (s6) folded, and how many reveals happened before
the frame of their last requested image was presented (must be 0)."""
import re
import sys


def ms(stamp):
    h, m, rest = stamp.split(":")
    return (int(h) * 3600 + int(m) * 60 + float(rest)) * 1000


events = []
for line in open(sys.argv[1], encoding="utf-8", errors="replace"):
    m = re.match(r"(\S+) TMPDIAG (.*)", line.strip())
    if not m:
        continue
    ts, msg = ms(m.group(1)), m.group(2)
    if re.match(r"raiseWindow \S+\.png active", msg):
        events.append(("req", ts, msg.split()[1]))
    elif msg.startswith("framePresented"):
        events.append(("pres", ts, msg.split()[1]))
    elif msg.startswith("setWindowConcealed false was true"):
        events.append(("show", ts, ""))

groups, current = [], []
for event in events:
    current.append(event)
    if event[0] == "show":
        groups.append(current)
        current = []
groups = groups[1:]  # the cold start reveal has no request

early = 0
for group in groups:
    reqs = [e for e in group if e[0] == "req"]
    if not reqs:
        continue
    last = reqs[-1]
    presented = [e[1] for e in group if e[0] == "pres" and e[2] == last[2] and e[1] >= last[1]]
    if not presented or presented[-1] > group[-1][1]:
        early += 1


def latency(gs):
    return [round(g[-1][1] - [e for e in g if e[0] == "req"][0][1]) for g in gs]


print("reveals:", len(groups), " revealed before the last request's frame:", early)
print("s2 request->visible ms:", latency(groups[:12]))
print("s4 request->visible ms:", latency(groups[12:24]))
folded = [len([e for e in g if e[0] == "req"]) for g in groups[24:]]
if folded:
    folded[0] -= 12  # the s3 requests (window visible, no reveal) precede the first burst
print("s6 requests per reveal:", folded,
      " requests after the last reveal:", len([e for e in current if e[0] == "req"]))
print("(the s2/s4/s6 split assumes 12 runs per scenario)")
