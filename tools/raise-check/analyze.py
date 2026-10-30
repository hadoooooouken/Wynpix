import sys
d = sys.argv[1]
fr = [tuple(map(float, l.split())) for l in open(d + "/frames.txt")]
COL = {"red": 1, "blue": 2, "green": 3}
res = {}
for l in open(d + "/events.txt"):
    p = l.split()
    if len(p) < 5:
        continue
    t = float(p[0])
    if p[3] == "launch":
        sc, new = p[2], p[4]
        old = "blue" if new == "red" else "red"
        seq = [f for f in fr if t - 0.1 <= f[0] <= t + 1.2]
        newok = any(f[COL[new]] > 0.2 for f in seq)
        if sc == "s3":
            bad = not newok
        else:
            bad = (not newok) or any(f[COL[old]] > 0.0005 for f in seq)
        res.setdefault(sc, []).append(bad)
        if bad:
            print("BAD", sc, t)
    elif p[3] == "launch3":
        seq = [f for f in fr if t - 0.1 <= f[0] <= t + 2.0]
        shown = []
        for f in seq:
            c = max(COL, key=lambda k: f[COL[k]])
            if f[COL[c]] > 0.2 and (not shown or shown[-1] != c):
                shown.append(c)
        bad = len(shown) != 1
        res.setdefault("s6", []).append(bad)
        print("s6", f"t={t:.1f}", "shown:", "->".join(shown) or "nothing")
for sc, v in res.items():
    print(f"{sc}: {len(v)} runs, {sum(v)} bad")
