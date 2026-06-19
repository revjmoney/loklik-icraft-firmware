#!/usr/bin/env python3
"""
optical_align.py - host-side Print-Then-Cut registration solver.

A clean, hardware-free reimplementation of the reusable math from LOKLiK's
COpticalAlign (GPL). Given three measured registration-mark corners and the
design's nominal mark rectangle, it solves the affine transform
(translation + rotation + per-axis scale) that maps DESIGN coordinates onto
MACHINE coordinates - so a cut follows a previously printed sheet.

The corners can come from ANY source: the optical eye, a webcam + a bit of CV,
or just jogging the head to each mark by eye. The solver doesn't care.

Units are agnostic as long as the corners and the marker size share one unit.
Use mm host-side (the firmware worked internally in si = 0.01 mm).

Original flow it mirrors (COpticalAlign::reviseOffset):
  P1 (left-top)  -> translation
  P1 -> P2 (right-top)   -> X scale + sheet rotation
  P2 -> P3 (right-bottom) -> Y scale
Note: the firmware only ever finds 3 corners; cacleP4() returns empty. 3 is
enough for a full affine, so we keep that.
"""

import math
from dataclasses import dataclass


def line_intersect(p1, p2, p3, p4):
    """Intersection of line(p1,p2) with line(p3,p4).

    Same purpose as COpticalAlign::cacleIntersect, but using the determinant
    form so vertical lines don't need the special-case fudge the original used.
    """
    (x1, y1), (x2, y2) = p1, p2
    (x3, y3), (x4, y4) = p3, p4
    denom = (x1 - x2) * (y3 - y4) - (y1 - y2) * (x3 - x4)
    if abs(denom) < 1e-9:
        raise ValueError("lines are parallel - cannot intersect")
    a = x1 * y2 - y1 * x2
    b = x3 * y4 - y3 * x4
    px = (a * (x3 - x4) - (x1 - x2) * b) / denom
    py = (a * (y3 - y4) - (y1 - y2) * b) / denom
    return (px, py)


def angle_at(p1, p2, vertex):
    """Signed angle in degrees (0..360) of p1 -> vertex -> p2.

    Direct port of COpticalAlign::cacleAngle (atan2(det, dot))."""
    x1, y1 = p1[0] - vertex[0], p1[1] - vertex[1]
    x2, y2 = p2[0] - vertex[0], p2[1] - vertex[1]
    dot = x1 * x2 + y1 * y2
    det = x1 * y2 - y1 * x2
    return math.degrees(math.atan2(det, dot)) % 360


@dataclass
class Registration:
    """The solved sheet pose. apply() maps design -> machine coordinates."""
    offset_x: float       # machine X of the left-top mark (design origin)
    offset_y: float       # machine Y of the left-top mark
    angle_deg: float      # how much the printed sheet is rotated
    scale_x: float        # measured / nominal along X (paper + printer stretch)
    scale_y: float        # measured / nominal along Y

    def apply(self, dx, dy):
        """Map a DESIGN point (relative to the left-top mark) to MACHINE xy."""
        sx, sy = dx * self.scale_x, dy * self.scale_y
        a = math.radians(self.angle_deg)
        rx = sx * math.cos(a) - sy * math.sin(a)
        ry = sx * math.sin(a) + sy * math.cos(a)
        return (self.offset_x + rx, self.offset_y + ry)


def solve(p1_lt, p2_rt, p3_rb, marker_width, marker_height):
    """Solve the registration transform from 3 measured corners.

    p1_lt, p2_rt, p3_rb : measured corner positions in MACHINE coords
    marker_width        : nominal left-top -> right-top distance in the design
    marker_height       : nominal right-top -> right-bottom distance

    Express your toolpath relative to the left-top mark (so the LT mark is
    design (0,0)); if your art measures from elsewhere, subtract that offset
    before calling reg.apply().
    """
    scale_x = math.dist(p1_lt, p2_rt) / marker_width
    scale_y = math.dist(p2_rt, p3_rb) / marker_height
    # rotation = tilt of the top edge (P1 -> P2) off the machine +X axis
    angle = math.degrees(math.atan2(p2_rt[1] - p1_lt[1],
                                    p2_rt[0] - p1_lt[0])) % 360
    return Registration(p1_lt[0], p1_lt[1], angle, scale_x, scale_y)


if __name__ == "__main__":
    # Demo: a 188 x 247 mm mark rectangle, printed sheet shifted to (10,20),
    # rotated 3 deg, and 1% over-scale in X from paper stretch.
    import math as _m
    W, H = 188.0, 247.0
    off = (10.0, 20.0)
    rot = _m.radians(3.0)
    sx, sy = 1.01, 1.00

    def place(dx, dy):
        x, y = dx * sx, dy * sy
        return (off[0] + x * _m.cos(rot) - y * _m.sin(rot),
                off[1] + x * _m.sin(rot) + y * _m.cos(rot))

    P1, P2, P3 = place(0, 0), place(W, 0), place(W, H)
    reg = solve(P1, P2, P3, W, H)
    print("solved:", reg)
    # a design point that should land back on the real sheet
    print("design (50,60) -> machine", reg.apply(50, 60))
    print("expected               ", place(50, 60))
