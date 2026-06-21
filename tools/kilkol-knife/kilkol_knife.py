#!/usr/bin/env python3
"""
KilKol Knife  --  LightBurn G-code  ->  drag-knife / pen G-code for FluidNC.

Part of the Simian Tactical Toolbox, by Rev. J. Money  (jmscnc.com).

LightBurn is a laser program: it already splits every job into TRAVEL (laser
off) and CUT (laser on). That maps 1:1 onto a drag knife / pen:

    laser ON  (G1, power S>0)  ->  blade/pen DOWN
    laser OFF (G0, or S0/M5)   ->  blade/pen UP

So you design in LightBurn (cut layers in **Line** mode, not Fill), hit
"Save GCode", and feed that file to this tool. Out comes FluidNC-ready pen
g-code with Z up/down, optional drag-knife blade-offset compensation, and
overcut on closed shapes.

Machine Z convention (the LokLik iCraft on FluidNC): **Z0 = UP**, negative = DOWN.

Modes:
  knife-comp    drag knife + blade-offset compensation + overcut (sharp corners)
  knife-nocomp  drag knife, pen up/down only (fine for big/simple shapes)
  pen-draw      pen / marker, no comp, light touch (drawing, not cutting)

GUI:  py kilkol_knife.py                  (double-click)
CLI:  py kilkol_knife.py in.gc out.gc --mode knife-comp --offset 0.25 --overcut 1.0

Blade-offset compensation is ported from Inkcut (GPLv3). This program is GPLv3.
"""
import sys, os, re, math, argparse

APP_NAME = "KilKol Knife"
VERSION = "1.0"

# ----------------------------- mode presets -----------------------------------
MODES = {
    "knife-comp": dict(
        label="Knife - blade compensation", comp=True,
        z_down=-1.5, cut_feed=800, offset=0.25, overcut=1.0),
    "knife-nocomp": dict(
        label="Knife - no compensation", comp=False,
        z_down=-1.5, cut_feed=800, offset=0.0, overcut=0.0),
    "pen-draw": dict(
        label="Pen / Marker - draw (no comp)", comp=False,
        z_down=-1.0, cut_feed=1500, offset=0.0, overcut=0.0),
}
Z_UP_DEFAULT = 0.0
Z_FEED_DEFAULT = 1000
CUTOFF_DEFAULT = 20.0      # corner angle (deg) above which a swivel arc is added

NUM = r'-?\d+\.?\d*'


def _val(axis, s):
    m = re.search(axis + r'(' + NUM + r')', s)
    return float(m.group(1)) if m else None


# ------------------------------- parsing --------------------------------------
def parse_strokes(text):
    """LightBurn laser g-code -> list of strokes.

    Each stroke is a list of (x, y) points: a pen-DOWN polyline that starts at
    the point where the head plunges. Tracks the modal motion mode (G0/G1) and
    laser power (S, M5); a move is 'cutting' iff it's a feed move with power on.
    Works whether LightBurn emits M3 (constant) or M4 (dynamic) power.
    """
    strokes, cur = [], None
    pos = [0.0, 0.0]
    motion, power = 0, 0.0
    for raw in text.splitlines():
        s = raw.strip()
        if not s or s[0] in ';(%':
            continue
        u = s.upper()
        if re.search(r'\bG0?0\b', u):
            motion = 0
        if re.search(r'\bG0?1\b', u):
            motion = 1
        sv = _val('S', u)
        if sv is not None:
            power = sv
        if re.search(r'\bM0?5\b', u):
            power = 0.0
        x, y = _val('X', u), _val('Y', u)
        if x is None and y is None:
            continue
        nx = x if x is not None else pos[0]
        ny = y if y is not None else pos[1]
        cutting = (motion == 1) and (power > 0)
        if cutting:
            if cur is None:
                cur = [(pos[0], pos[1])]
            cur.append((nx, ny))
        elif cur is not None:
            strokes.append(cur)
            cur = None
        pos = [nx, ny]
    if cur is not None:
        strokes.append(cur)
    return strokes


# ------------------------------ geometry --------------------------------------
def _ang(p, q):
    return math.atan2(q[1] - p[1], q[0] - p[0])


def _closed(pts, tol=0.05):
    return math.hypot(pts[0][0] - pts[-1][0], pts[0][1] - pts[-1][1]) <= tol


def add_overcut(pts, dist):
    """For a CLOSED contour, continue past the start point along the first
    segment by `dist`, so the trailing blade fully severs the closure (kills the
    little uncut tab at the start/stop point)."""
    if dist <= 0 or len(pts) < 3 or not _closed(pts):
        return pts
    a = _ang(pts[0], pts[1])
    return list(pts) + [(pts[0][0] + dist * math.cos(a),
                         pts[0][1] + dist * math.sin(a))]


def blade_offset(pts, r, cutoff_deg=CUTOFF_DEFAULT, arc_steps=10):
    """Drag-knife blade-offset compensation (ported from Inkcut's blade_offset
    filter, line-segment case -- LightBurn output is already flattened to lines).

    A swivel knife's tip trails the holder's pivot by the offset `r`, like a
    caster. So each commanded (holder) point is shifted FORWARD along the
    entering travel direction by `r`, which puts the trailing tip on the design
    line. At a corner sharper than `cutoff_deg`, a small arc of radius `r` is
    inserted about the corner so the blade swivels to the new heading instead of
    rounding the corner.
    """
    if r <= 0 or len(pts) < 2:
        return list(pts)
    cutoff = math.radians(cutoff_deg)
    out = []
    a0 = _ang(pts[0], pts[1])
    out.append((pts[0][0] + r * math.cos(a0), pts[0][1] + r * math.sin(a0)))
    prev = a0
    for i in range(1, len(pts)):
        p, q = pts[i - 1], pts[i]
        if p == q:
            continue
        seg = _ang(p, q)
        diff = seg - prev
        while diff > math.pi:
            diff -= 2 * math.pi
        while diff <= -math.pi:
            diff += 2 * math.pi
        if abs(diff) > cutoff:                  # swivel arc about the corner p
            steps = max(1, int(round(arc_steps * abs(diff) / math.pi)))
            for s in range(1, steps + 1):
                aa = prev + diff * s / steps
                out.append((p[0] + r * math.cos(aa), p[1] + r * math.sin(aa)))
        out.append((q[0] + r * math.cos(seg), q[1] + r * math.sin(seg)))
        prev = seg
    return out


# ------------------------------- emit -----------------------------------------
def convert(text, mode="knife-comp", z_up=Z_UP_DEFAULT, z_down=-1.5,
            cut_feed=800, z_feed=Z_FEED_DEFAULT, offset=0.25, overcut=1.0,
            cutoff=CUTOFF_DEFAULT, travel_g0=True, travel_feed=5000, home=True):
    """Return (gcode_text, n_strokes, n_cuts)."""
    m = MODES.get(mode, MODES["knife-comp"])
    comp = m["comp"]
    strokes = parse_strokes(text)
    out = [
        "; %s v%s  --  converted from LightBurn laser g-code" % (APP_NAME, VERSION),
        "; mode: %s   (Z0 = up, negative = down)" % m["label"],
        "G21", "G90", "G94",
        "G1 Z%.4f F%d" % (z_up, z_feed),        # ensure up to start
    ]
    n_cuts = 0
    for st in strokes:
        if len(st) < 2:
            continue
        pts = st
        if comp:
            pts = add_overcut(pts, overcut)
            pts = blade_offset(pts, offset, cutoff)
        x0, y0 = pts[0]
        if travel_g0:
            out.append("G0 X%.4f Y%.4f" % (x0, y0))
        else:
            out.append("G1 X%.4f Y%.4f F%d" % (x0, y0, travel_feed))
        out.append("G1 Z%.4f F%d" % (z_down, z_feed))            # down
        for (x, y) in pts[1:]:
            out.append("G1 X%.4f Y%.4f F%d" % (x, y, cut_feed))
        out.append("G1 Z%.4f F%d" % (z_up, z_feed))              # up
        n_cuts += 1
    if home:
        out.append("G0 X0 Y0")
    out.append("M30")
    return "\n".join(out) + "\n", len(strokes), n_cuts


# -------------------------------- CLI -----------------------------------------
def run_cli(argv):
    ap = argparse.ArgumentParser(
        prog="kilkol_knife",
        description="Convert LightBurn laser g-code to drag-knife / pen g-code.")
    ap.add_argument("input", help="LightBurn .gc/.nc/.gcode file")
    ap.add_argument("output", help="output pen/blade g-code file")
    ap.add_argument("--mode", choices=list(MODES), default="knife-comp")
    ap.add_argument("--z-up", type=float, default=Z_UP_DEFAULT)
    ap.add_argument("--z-down", type=float, default=None,
                    help="plunge depth (mm, negative). default per mode")
    ap.add_argument("--cut-feed", type=int, default=None)
    ap.add_argument("--z-feed", type=int, default=Z_FEED_DEFAULT)
    ap.add_argument("--offset", type=float, default=None,
                    help="blade offset (mm) for knife-comp")
    ap.add_argument("--overcut", type=float, default=None,
                    help="overcut (mm) for knife-comp closed shapes")
    ap.add_argument("--cutoff", type=float, default=CUTOFF_DEFAULT)
    ap.add_argument("--travel-feed", type=int, default=5000)
    ap.add_argument("--g1-travel", action="store_true",
                    help="use G1 travels at --travel-feed instead of G0 rapids")
    ap.add_argument("--no-home", action="store_true")
    a = ap.parse_args(argv)
    d = MODES[a.mode]
    kw = dict(
        mode=a.mode, z_up=a.z_up,
        z_down=a.z_down if a.z_down is not None else d["z_down"],
        cut_feed=a.cut_feed if a.cut_feed is not None else d["cut_feed"],
        z_feed=a.z_feed,
        offset=a.offset if a.offset is not None else d["offset"],
        overcut=a.overcut if a.overcut is not None else d["overcut"],
        cutoff=a.cutoff, travel_g0=not a.g1_travel, travel_feed=a.travel_feed,
        home=not a.no_home)
    with open(a.input, "r", errors="replace") as f:
        text = f.read()
    gcode, n_strokes, n_cuts = convert(text, **kw)
    with open(a.output, "w") as f:
        f.write(gcode)
    print("%s: %s -> %s" % (APP_NAME, a.input, a.output))
    print("  mode=%s  strokes=%d  cuts=%d  (offset=%.3f overcut=%.3f z_down=%.3f)"
          % (a.mode, n_strokes, n_cuts, kw["offset"], kw["overcut"], kw["z_down"]))


# -------------------------------- GUI -----------------------------------------
def run_gui():
    import tkinter as tk
    from tkinter import ttk, filedialog, messagebox

    root = tk.Tk()
    root.title("%s v%s" % (APP_NAME, VERSION))
    root.geometry("620x560")

    state = {"in": tk.StringVar(), "out": tk.StringVar(),
             "mode": tk.StringVar(value="knife-comp")}
    fields = {}

    def add_row(parent, label, key, default, row):
        ttk.Label(parent, text=label).grid(row=row, column=0, sticky="w", pady=2)
        v = tk.StringVar(value=str(default))
        ttk.Entry(parent, textvariable=v, width=12).grid(
            row=row, column=1, sticky="w", padx=6)
        fields[key] = v
        return v

    # ---- menu ----
    menubar = tk.Menu(root)
    filem = tk.Menu(menubar, tearoff=0)
    filem.add_command(label="Open LightBurn g-code...", command=lambda: pick_in())
    filem.add_command(label="Set output file...", command=lambda: pick_out())
    filem.add_separator()
    filem.add_command(label="Convert", command=lambda: do_convert())
    filem.add_separator()
    filem.add_command(label="Quit", command=root.destroy)
    menubar.add_cascade(label="File", menu=filem)
    helpm = tk.Menu(menubar, tearoff=0)
    helpm.add_command(label="About", command=lambda: messagebox.showinfo(
        "About",
        "%s v%s\nPart of the Simian Tactical Toolbox -- Rev. J. Money\n"
        "jmscnc.com\n\nLightBurn laser g-code -> drag-knife / pen g-code for "
        "FluidNC.\nBlade-offset comp ported from Inkcut. GPLv3." % (APP_NAME, VERSION)))
    menubar.add_cascade(label="Help", menu=helpm)
    root.config(menu=menubar)

    # ---- files ----
    frm = ttk.Frame(root, padding=10)
    frm.pack(fill="x")
    ttk.Label(frm, text="LightBurn g-code:").grid(row=0, column=0, sticky="w")
    ttk.Entry(frm, textvariable=state["in"], width=48).grid(row=0, column=1, padx=4)
    ttk.Button(frm, text="Browse", command=lambda: pick_in()).grid(row=0, column=2)
    ttk.Label(frm, text="Output g-code:").grid(row=1, column=0, sticky="w", pady=4)
    ttk.Entry(frm, textvariable=state["out"], width=48).grid(row=1, column=1, padx=4)
    ttk.Button(frm, text="Browse", command=lambda: pick_out()).grid(row=1, column=2)

    # ---- mode ----
    mf = ttk.LabelFrame(root, text="Mode", padding=10)
    mf.pack(fill="x", padx=10, pady=6)
    for i, (k, m) in enumerate(MODES.items()):
        ttk.Radiobutton(mf, text=m["label"], value=k, variable=state["mode"],
                        command=lambda: apply_mode()).grid(
            row=i, column=0, sticky="w")

    # ---- params ----
    pf = ttk.LabelFrame(root, text="Parameters", padding=10)
    pf.pack(fill="x", padx=10, pady=6)
    add_row(pf, "Z up (mm)", "z_up", Z_UP_DEFAULT, 0)
    add_row(pf, "Z down / plunge (mm)", "z_down", -1.5, 1)
    add_row(pf, "Cut feed (mm/min)", "cut_feed", 800, 2)
    add_row(pf, "Z feed (mm/min)", "z_feed", Z_FEED_DEFAULT, 3)
    add_row(pf, "Blade offset (mm)", "offset", 0.25, 4)
    add_row(pf, "Overcut (mm)", "overcut", 1.0, 5)
    add_row(pf, "Corner angle (deg)", "cutoff", CUTOFF_DEFAULT, 6)

    # ---- log ----
    logf = ttk.Frame(root, padding=10)
    logf.pack(fill="both", expand=True)
    ttk.Button(logf, text="Convert", command=lambda: do_convert()).pack(anchor="w")
    log = tk.Text(logf, height=8, wrap="word")
    log.pack(fill="both", expand=True, pady=6)

    def logmsg(s):
        log.insert("end", s + "\n")
        log.see("end")

    def apply_mode():
        d = MODES[state["mode"].get()]
        fields["z_down"].set(str(d["z_down"]))
        fields["cut_feed"].set(str(d["cut_feed"]))
        fields["offset"].set(str(d["offset"]))
        fields["overcut"].set(str(d["overcut"]))

    def pick_in():
        p = filedialog.askopenfilename(
            title="LightBurn g-code",
            filetypes=[("G-code", "*.gc *.gcode *.nc *.ngc"), ("All", "*.*")])
        if p:
            state["in"].set(p)
            if not state["out"].get():
                base, _ = os.path.splitext(p)
                state["out"].set(base + "_pen.gc")

    def pick_out():
        p = filedialog.asksaveasfilename(
            title="Output g-code", defaultextension=".gc",
            filetypes=[("G-code", "*.gc *.gcode *.nc"), ("All", "*.*")])
        if p:
            state["out"].set(p)

    def do_convert():
        try:
            inp, outp = state["in"].get(), state["out"].get()
            if not inp or not outp:
                messagebox.showwarning("Missing", "Pick an input and output file.")
                return
            kw = dict(
                mode=state["mode"].get(),
                z_up=float(fields["z_up"].get()),
                z_down=float(fields["z_down"].get()),
                cut_feed=int(float(fields["cut_feed"].get())),
                z_feed=int(float(fields["z_feed"].get())),
                offset=float(fields["offset"].get()),
                overcut=float(fields["overcut"].get()),
                cutoff=float(fields["cutoff"].get()))
            with open(inp, "r", errors="replace") as f:
                text = f.read()
            gcode, n_strokes, n_cuts = convert(text, **kw)
            with open(outp, "w") as f:
                f.write(gcode)
            logmsg("OK  %s -> %s" % (os.path.basename(inp), os.path.basename(outp)))
            logmsg("    mode=%s  strokes=%d  cuts=%d" %
                   (kw["mode"], n_strokes, n_cuts))
            if kw["mode"] == "knife-comp":
                logmsg("    blade offset=%.3f mm  overcut=%.3f mm" %
                       (kw["offset"], kw["overcut"]))
            logmsg("    Load it in gSender and cut. (Z0=up, %.2f=down)" % kw["z_down"])
        except Exception as e:
            messagebox.showerror("Convert failed", str(e))
            logmsg("ERROR: %s" % e)

    apply_mode()
    logmsg("%s v%s -- feed me a LightBurn 'Save GCode' file (Line-mode layers)."
           % (APP_NAME, VERSION))
    root.mainloop()


def main():
    if len(sys.argv) > 1:
        run_cli(sys.argv[1:])
    else:
        run_gui()


if __name__ == "__main__":
    main()
