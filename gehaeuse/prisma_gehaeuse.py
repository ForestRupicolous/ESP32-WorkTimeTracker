#!/usr/bin/env python3
"""
Zeitprisma – N-Kant-Gehaeuse (Standard: 10 Seiten) fuer den M5Stack StickS3 (SKU K150)

Parametrisches Modell, erzeugt zwei druckfertige STLs:
    pip install manifold3d numpy matplotlib
    python3 prisma_gehaeuse.py                   # Koerper + Deckel
    python3 prisma_gehaeuse.py --check STICK.stl # + Passungspruefung gegen M5-Strukturmodell

Aufbau
  - Der Stick wird von der Stirnseite (Deckelseite) in einen Schacht geschoben. Das
    Display schaut durch ein Fenster in Seite 0, die Lippen neben dem Fenster halten
    ihn. Eine Stahlscheibe im Schachtboden zieht ihn ueber seinen Magnetruecken an.
  - Der Deckel ist ein Steckstopfen (Reibschluss); mit Schraubendreher ueber die
    Tastennut oben heraushebeln.
  - Die Groesse wird automatisch bestimmt: kleinster Inkreisradius, bei dem Fenster
    plus Rahmen auf Seite 0 passen und ueberall WALL_MIN Wand bleibt.

Koordinaten (Modell): X = Breite, Y = Laengsachse (USB-Ende bei Y=0, Deckel bei Y=L),
                      Z = Hoehe, Seite 0 (Display) zeigt nach +Z.
Seitennummern: 0 = Display, 1..N-1 als Ziffern eingraviert (6. und 9. mit Punkt).
Druck: Koerper stehend auf dem USB-Ende, Deckel flach – beides ohne Stuetzen.
"""
import argparse, math, struct, sys
import numpy as np
from manifold3d import Manifold, CrossSection, JoinType, OpType, FillRule

# ------------------------------------------------------------------ Parameter
N_FACES      = 10

# StickS3 (aus offiziellem M5-Strukturmodell vermessen; Stick-Koordinaten:
# sx 0..24 quer, sy 0..48 laengs ab USB-Ende, sz 0 = Front, -14.08 = Rueckseite)
STICK_W      = 24.0
STICK_L      = 48.0
STICK_T      = 14.08
BTN_PROTRUDE = 0.92                # Taste A ragt vorne heraus
BTN_SX       = (6.18, 17.82)       # Taste A quer
BTN_SY       = (9.54, 11.58)       # Taste A laengs
DISP_SY      = (18.4, 43.6)        # aktive Displayflaeche (1,14", ca. 14,2 x 25,2 mm)
DISP_W       = 14.2
MAGNET_SY    = 24.4                # Magnetmitte Rueckseite
USB_SXZ      = (11.7, -4.25)       # Mitte USB-C-Buchse (sx, sz)
SIDE_BUTTONS = [(0.0, 8.25, -6.5),     # Seitentaster sx=0
                (24.0, 24.5, -6.25)]   # Seitentaster sx=24

FIT          = 0.30    # Spiel je Seite (PLA/PETG, 0.4er Duese)
WALL_MIN     = 1.6     # Mindestwand um den Schacht
WALL_END     = 2.4     # Wand am USB-Ende
EDGE_CHAMFER = 0.6     # Fase an allen Aussenkanten

WINDOW_W     = 15.4    # Displayfenster quer (aktive Flaeche 14,2)
WINDOW_SY    = (8.3, 45.2)  # Fenster laengs in Stick-Koordinaten (inkl. Taste A)
WINDOW_R     = 2.0
WINDOW_CHAMF = 0.6     # Fase an der Fensterkante (Finger, Blickwinkel)
FRAME_MIN    = 1.0     # flacher Steg neben Fenster-Fase auf Seite 0
RECESS_RANGE = (2.0, 4.5)   # Lippendicke ueber dem Schacht (= Displaytiefe - 2*FIT)
BTN_GROOVE_H = 1.3     # Nut in der Schachtdecke, damit Taste A beim Einschieben frei laeuft
BTN_GROOVE_C = 0.6     # seitliche Luft der Nut

CAP_T        = 3.0     # Deckelstopfen
CAP_CLR      = 0.05    # Spiel je Seite (Reibschluss; bei zu straff schleifen)
CAP_CHAMF    = 0.4

WASHER_D     = 16.0    # Stahlscheibe M8 DIN 125 (16 x 1.6)
WASHER_T     = 1.6
USB_HOLE     = (14.0, 8.0, 2.0)   # Breite, Hoehe, Eckradius (USB-C-Stecker bis ~12,4 x 6,5)
SIDE_HOLE_D  = 5.5

DIGIT_H      = 8.5     # Ziffernhoehe
DIGIT_DEPTH  = 0.6
DIGIT_FROM_TOP = 12.0  # Ziffernmitte ab Deckelende
DIGIT_FONT   = dict(family='DejaVu Sans', weight='bold')

SEG = 64

# ------------------------------------------------------------- Bausteine
def ngon_cs(r_in, chamfer, n=N_FACES):
    """Regelmaessiges n-Eck (Inkreisradius r_in), Seite 0 oben (+Z), Ecken gefast. (X, Z)"""
    r_out = r_in / math.cos(math.pi / n)
    verts = [(r_out * math.cos(math.radians(90 + 180 / n + 360 * k / n)),
              r_out * math.sin(math.radians(90 + 180 / n + 360 * k / n))) for k in range(n)]
    pts = []
    for i, v in enumerate(verts):
        for nb in (verts[i - 1], verts[(i + 1) % n]):
            d = np.subtract(nb, v)
            d /= np.linalg.norm(d)
            pts.append(tuple(np.add(v, d * chamfer)))
    return CrossSection([pts])


def rect_cs(x0, x1, y0, y1, r=0.0):
    cs = CrossSection.square((x1 - x0, y1 - y0)).translate((x0, y0))
    if r > 0:
        cs = cs.offset(-r, JoinType.Miter).offset(r, JoinType.Round, circular_segments=SEG)
    return cs


def prism_x_z(cs, y0, length):
    """(X,Z)-Querschnitt entlang +Y von y0 bis y0+length extrudieren."""
    m = Manifold.extrude(cs, length).rotate((90, 0, 0))     # (x,y,z) -> (x,-z,y)
    return m.translate((0, y0 + length, 0))


def slab_xy(cs, z0, z1):
    """(X,Y)-Querschnitt von z0 bis z1."""
    return Manifold.extrude(cs, z1 - z0).translate((0, 0, z0))


def box(x0, x1, y0, y1, z0, z1):
    return Manifold.cube((x1 - x0, y1 - y0, z1 - z0)).translate((x0, y0, z0))


def frame_from(z_axis, origin, up=(0.0, 1.0, 0.0)):
    """3x4-Matrix: lokales (u,v,w) -> Welt; w = z_axis, v = up, u = v x w (nicht gespiegelt)."""
    w = np.asarray(z_axis, float); w /= np.linalg.norm(w)
    v = np.asarray(up, float); v = v - w * np.dot(v, w); v /= np.linalg.norm(v)
    u = np.cross(v, w)
    return np.column_stack([u, v, w, np.asarray(origin, float)]).tolist()


def cyl_along(p, n, d, h0, h1):
    n = np.asarray(n, float); n /= np.linalg.norm(n)
    helper = (0, 0, 1.0) if abs(n[2]) < 0.9 else (1.0, 0, 0)
    c = Manifold.cylinder(h1 - h0, d / 2, d / 2, SEG).translate((0, 0, h0))
    return c.transform(frame_from(n, p, helper))


def glyph_cs(text, height):
    from matplotlib.textpath import TextPath
    from matplotlib.font_manager import FontProperties
    tp = TextPath((0, 0), text, size=10, prop=FontProperties(**DIGIT_FONT))
    polys = [np.asarray(p) for p in tp.to_polygons() if len(p) >= 3]
    # EvenOdd per FillRule (CrossSection.even_odd gibt es in manifold3d 3.5.4 nicht)
    cs = CrossSection(polys, FillRule.EvenOdd)
    b = cs.bounds()                       # (xmin, ymin, xmax, ymax)
    ref = TextPath((0, 0), '8', size=10, prop=FontProperties(**DIGIT_FONT)).get_extents()
    s = height / ref.height               # gleiche Ziffernhoehe fuer alle
    return cs.translate((-(b[0] + b[2]) / 2, -(ref.y0 + ref.y1) / 2)).scale((s, s))


# ------------------------------------------------------------- Groesse bestimmen
POCKET_HW = STICK_W / 2 + FIT
POCKET_L  = STICK_L + 2 * FIT
SLOT_H    = STICK_T + 2 * FIT
GROOVE_HW = (BTN_SX[1] - BTN_SX[0]) / 2 + BTN_GROOVE_C
L         = WALL_END + POCKET_L + CAP_T
Y0        = WALL_END + FIT                      # Stick-USB-Ende (mittig im Laengsspiel)
TAN       = math.tan(math.pi / N_FACES)


def slot_profile(z_ceil):
    """Schacht + Tastennut im Querschnitt (X, Z)."""
    return (rect_cs(-POCKET_HW, POCKET_HW, z_ceil - SLOT_H, z_ceil)
            + rect_cs(-GROOVE_HW, GROOVE_HW, z_ceil - 0.01, z_ceil + BTN_GROOVE_H))


def solve_size():
    r = (WINDOW_W / 2 + WINDOW_CHAMF + FRAME_MIN + EDGE_CHAMFER) / TAN
    r = math.ceil(r * 20) / 20
    while r < 100:
        inner = ngon_cs(r, EDGE_CHAMFER).offset(-WALL_MIN, JoinType.Miter)
        rc = RECESS_RANGE[0]
        while rc <= RECESS_RANGE[1] + 1e-9:
            if (slot_profile(r - rc) - inner).area() < 1e-6:
                return r, rc
            rc = round(rc + 0.05, 2)
        r = round(r + 0.05, 2)
    raise SystemExit('keine Loesung')


R_IN, RECESS = solve_size()
R_OUT   = R_IN / math.cos(math.pi / N_FACES)
Z_CEIL  = R_IN - RECESS                 # Schachtdecke = Unterseite der Lippen
Z_FLOOR = Z_CEIL - SLOT_H
Z_FRONT = Z_FLOOR + STICK_T             # Stick-Front (liegt per Magnet am Boden)
FACE_ANGLES = [90 - 360 / N_FACES * k for k in range(N_FACES)]


def stick_to_model(sx, sy, sz):
    return (np.asarray(sx) - STICK_W / 2, Y0 + np.asarray(sy), Z_FRONT + np.asarray(sz))


def face_normal(k):
    a = math.radians(FACE_ANGLES[k])
    return np.array([math.cos(a), 0.0, math.sin(a)])


# ------------------------------------------------------------- Modell
def side_hole_cuts():
    cuts = []
    for sx, sy, sz in SIDE_BUTTONS:
        x, y, z = stick_to_model(sx, sy, sz)
        nx = -1.0 if sx < STICK_W / 2 else 1.0
        cuts.append(cyl_along((nx * (POCKET_HW - 0.5), float(y), float(z)), (nx, 0, 0),
                              SIDE_HOLE_D, 0, R_OUT + 2))
    return cuts


def digit_label(k):
    return f"{k}." if k in (6, 9) else str(k)


def digit_cuts():
    cuts = []
    yc = L - DIGIT_FROM_TOP
    for k in range(1, N_FACES):
        n = face_normal(k)
        g = Manifold.extrude(glyph_cs(digit_label(k), DIGIT_H), DIGIT_DEPTH + 1.0).translate((0, 0, -DIGIT_DEPTH))
        cuts.append(g.transform(frame_from(n, n * R_IN + np.array([0, yc, 0]))))
    return cuts


def build_body():
    outer = ngon_cs(R_IN, EDGE_CHAMFER)
    inner = outer.offset(-EDGE_CHAMFER, JoinType.Miter)
    body = Manifold.batch_hull([prism_x_z(outer, EDGE_CHAMFER, L - 2 * EDGE_CHAMFER),
                                prism_x_z(inner, 0, L)])
    cuts = []
    # Schacht (zum Deckelende offen) + Nut fuer Taste A
    cuts.append(prism_x_z(slot_profile(Z_CEIL), WALL_END, L - WALL_END + 1))
    # Displayfenster mit Fase
    wy0, wy1 = Y0 + WINDOW_SY[0], Y0 + WINDOW_SY[1]
    win = rect_cs(-WINDOW_W / 2, WINDOW_W / 2, wy0, wy1, WINDOW_R)
    cuts.append(slab_xy(win, Z_CEIL - 0.1, R_IN + 1))
    cuts.append(Manifold.batch_hull([slab_xy(win, R_IN - WINDOW_CHAMF, R_IN - WINDOW_CHAMF + 0.01),
                                     slab_xy(win.offset(1 + WINDOW_CHAMF, JoinType.Round), R_IN + 1, R_IN + 1.01)]))
    # Stahlscheibe im Schachtboden
    _, wy, _ = stick_to_model(STICK_W / 2, MAGNET_SY, 0)
    cuts.append(cyl_along((0, float(wy), Z_FLOOR), (0, 0, 1), WASHER_D + 0.4, -(WASHER_T + 0.2), 0.5))
    # USB-C
    ux, _, uz = stick_to_model(*[USB_SXZ[0], 0, USB_SXZ[1]])
    w, h, r = USB_HOLE
    cuts.append(prism_x_z(rect_cs(float(ux) - w / 2, float(ux) + w / 2, float(uz) - h / 2, float(uz) + h / 2, r),
                          -1, WALL_END + 1.5))
    cuts += side_hole_cuts()
    cuts += digit_cuts()
    return body - Manifold.batch_boolean(cuts, OpType.Add)


def cap_profile():
    return rect_cs(-POCKET_HW, POCKET_HW, Z_FLOOR, Z_CEIL).offset(-CAP_CLR, JoinType.Miter)


def build_cap_print():
    """Deckel in Drucklage: Aussenseite auf dem Bett, Einfuehrfase oben."""
    cs = cap_profile()
    cs = cs.translate((0, -(Z_FLOOR + Z_CEIL) / 2))
    return Manifold.batch_hull([slab_xy(cs, 0, CAP_T - CAP_CHAMF),
                                slab_xy(cs.offset(-CAP_CHAMF, JoinType.Miter), CAP_T - 0.01, CAP_T)])


def build_cap_model():
    """Deckel in Einbaulage (fuer Pruefung/Vorschau)."""
    return prism_x_z(cap_profile(), L - CAP_T, CAP_T)


# ------------------------------------------------------------- Ausgabe
def write_stl(m, path, name):
    mesh = m.to_mesh()
    v = np.asarray(mesh.vert_properties)[:, :3]
    f = np.asarray(mesh.tri_verts)
    tri = v[f]
    nrm = np.cross(tri[:, 1] - tri[:, 0], tri[:, 2] - tri[:, 0])
    nrm /= np.linalg.norm(nrm, axis=1)[:, None] + 1e-12
    with open(path, 'wb') as fh:
        fh.write(name.encode().ljust(80, b' '))
        fh.write(struct.pack('<I', len(f)))
        rec = np.zeros(len(f), dtype=[('n', '<f4', 3), ('v', '<f4', (3, 3)), ('a', '<u2')])
        rec['n'] = nrm; rec['v'] = tri
        fh.write(rec.tobytes())


def body_for_print(m):
    """Stehend auf dem USB-Ende: Y -> Z."""
    return m.transform([[1, 0, 0, 0], [0, 0, -1, 0], [0, 1, 0, 0]])   # (x,y,z) -> (x,-z,y)


# ------------------------------------------------------------- Pruefung
def load_stick(path):
    b = open(path, 'rb').read()
    n = struct.unpack('<I', b[80:84])[0]
    t = np.frombuffer(b, dtype=[('n', '<f4', 3), ('v', '<f4', (3, 3)), ('a', '<u2')], count=n, offset=84)
    tri = t['v'].astype(float)
    return tri[tri.mean(1)[:, 0] < 26]          # Teil 0 = komplette Baugruppe


def stability(m, stick_mass=20.0, fill=0.35, washer_mass=2.5):
    mesh = m.to_mesh()
    v = np.asarray(mesh.vert_properties)[:, :3]
    t = v[np.asarray(mesh.tri_verts)]
    vol6 = np.einsum('ij,ij->i', t[:, 0], np.cross(t[:, 1], t[:, 2]))
    com = (vol6[:, None] * t.sum(1)).sum(0) / (4 * vol6.sum())
    m_shell = vol6.sum() / 6 / 1000 * 1.24 * fill
    parts = [(m_shell, com[[0, 2]]), (stick_mass, np.array([0.0, Z_FRONT - STICK_T / 2])),
             (washer_mass, np.array([0.0, Z_FLOOR - WASHER_T / 2]))]
    mt = sum(p[0] for p in parts)
    return m_shell, sum(p[0] * p[1] for p in parts) / mt


def check(body, cap, stick_path):
    import matplotlib.path as mpath
    ok = True

    def rep(name, cond, detail):
        nonlocal ok
        ok &= bool(cond)
        print(f"  [{'OK ' if cond else 'FEHLER'}] {name}: {detail}")

    tri = load_stick(stick_path)
    V = tri.reshape(-1, 3)
    P = np.column_stack(stick_to_model(V[:, 0], V[:, 1], V[:, 2]))
    eps = 1e-3
    rep('Stick im Schacht (X)', np.abs(P[:, 0]).max() <= POCKET_HW + eps, f'|x|max={np.abs(P[:,0]).max():.2f} <= {POCKET_HW:.2f}')
    rep('Stick im Schacht (Y)', P[:, 1].min() >= WALL_END - eps and P[:, 1].max() <= L - CAP_T + eps,
        f'y={P[:,1].min():.2f}..{P[:,1].max():.2f}, frei {WALL_END:.2f}..{L-CAP_T:.2f}')
    rep('Stick liegt auf Schachtboden', abs(P[:, 2].min() - Z_FLOOR) < 0.05, f'z_min={P[:,2].min():.2f}, Boden={Z_FLOOR:.2f}')

    # Taste und Display im Fenster
    bx = np.array(BTN_SX) - STICK_W / 2
    rep('Taste A im Fenster', bx.min() >= -WINDOW_W / 2 and bx.max() <= WINDOW_W / 2
        and BTN_SY[0] >= WINDOW_SY[0] and BTN_SY[1] <= WINDOW_SY[1],
        f'x {bx[0]:.1f}..{bx[1]:.1f}, Fenster +-{WINDOW_W/2:.1f}; '
        f'{R_IN - (Z_FRONT + BTN_PROTRUDE):.1f} mm unter Seite 0')
    rep('Displayflaeche im Fenster', DISP_W <= WINDOW_W and DISP_SY[0] >= WINDOW_SY[0] and DISP_SY[1] <= WINDOW_SY[1],
        f'{DISP_W} mm breit, Fenster {WINDOW_W}; Display {R_IN - Z_FRONT:.2f} mm unter Seite 0')

    # Kollision in Endlage (Stichprobe, Koerper und Deckel)
    rng = np.random.default_rng(1)
    samp = P[rng.choice(len(P), 4000, replace=False)]
    ctr = np.array([0, Y0 + STICK_L / 2, Z_FRONT - STICK_T / 2])
    samp = ctr + (samp - ctr) * 0.995
    probes = Manifold.batch_boolean([Manifold.cube((0.05, 0.05, 0.05), True).translate(tuple(p)) for p in samp[::8]], OpType.Add)
    inter = (probes ^ (body + cap)).volume()
    rep('Keine Kollision in Endlage (Stichprobe)', inter < 1e-6, f'Schnittvolumen {inter:.6f} mm3')

    # Einschieben: Stick-Silhouette (X,Z) muss durch Schacht+Nut passen, auch an die Decke gedrueckt
    prof = slot_profile(Z_CEIL)
    paths = [mpath.Path(np.vstack([p, p[:1]])) for p in prof.offset(0.01, JoinType.Miter).to_polygons()]
    xz = P[:, [0, 2]]
    for lift, name in ((0.0, 'am Boden'), (Z_CEIL - Z_FRONT, 'an der Decke')):
        q = xz + [0, lift]
        inside = np.zeros(len(q), bool)
        for pa in paths:
            inside |= pa.contains_points(q)
        rep(f'Einschieben ({name})', inside.all(), f'{(~inside).sum()} Punkte ausserhalb')

    # Waende, Rahmen
    inner = ngon_cs(R_IN, EDGE_CHAMFER).offset(-WALL_MIN, JoinType.Miter)
    rep(f'Wand >= {WALL_MIN} mm um Schacht', (slot_profile(Z_CEIL) - inner).area() < 1e-6, f'Lippen {RECESS:.2f} mm')
    land = R_IN * TAN - EDGE_CHAMFER - WINDOW_W / 2 - WINDOW_CHAMF
    rep('Flacher Steg neben Fenster', land >= FRAME_MIN - 1e-6, f'{land:.2f} mm je Seite')
    rep('Material ueber Tastennut', RECESS - BTN_GROOVE_H >= WALL_MIN - 1e-6, f'{RECESS - BTN_GROOVE_H:.2f} mm')

    # USB
    ux, _, uz = stick_to_model(USB_SXZ[0], 0, USB_SXZ[1])
    rep('USB-C-Loch', USB_HOLE[0] >= 12.4 + 1 and USB_HOLE[1] >= 6.5 + 1,
        f'{USB_HOLE[0]} x {USB_HOLE[1]} mm um Buchse (x={float(ux):+.1f}, z={float(uz)-Z_FRONT:+.1f} ab Front)')

    # Seitenloecher: Austrittsflaeche, Abstand zu Ziffern
    yc = L - DIGIT_FROM_TOP
    for sx, sy, sz in SIDE_BUTTONS:
        x, y, z = (float(a) for a in stick_to_model(sx, sy, sz))
        nx = -1.0 if sx < STICK_W / 2 else 1.0
        hits = []
        for k in range(N_FACES):
            nk = face_normal(k)
            if nk[0] * nx > 1e-6:
                hits.append(((R_IN - z * nk[2]) / (nk[0] * nx), k))
        dist, k = min(hits)
        gap = abs(y - yc) - SIDE_HOLE_D / 2 - DIGIT_H / 2
        rep(f'Tasterloch sx={sx:.0f}', gap > 1.0, f'Kanal {dist - POCKET_HW:.1f} mm, Austritt Seite {k}, '
            f'{gap:.1f} mm Abstand zur Ziffer')
    return ok


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--check', metavar='STICK_STL', help='M5 Strukturmodell StickS3.stl')
    ap.add_argument('--prefix', default=f'zeitprisma{N_FACES}')
    a = ap.parse_args()

    body = build_body()
    cap = build_cap_model()
    print(f"{N_FACES}-Kant: Schluesselweite {2*R_IN:.1f} mm, Eckmass {2*R_OUT:.1f} mm, Laenge {L:.1f} mm")
    print(f"Flaechenbreite {2*R_IN*TAN:.1f} mm, Lippen {RECESS:.2f} mm, Display {R_IN - Z_FRONT:.2f} mm unter Seite 0")
    print(f"Koerper {body.volume()/1000:.1f} cm3, Status {body.status()}, Genus {body.genus()}")
    ms, c = stability(body + cap)
    print(f"Gewicht ca. {ms:.0f} g Druck (35 % Fuellgrad) + 20 g Stick, Schwerpunkt x={c[0]:+.1f} z={c[1]:+.1f} mm")
    flat_half = R_IN * TAN - EDGE_CHAMFER
    for k in range(N_FACES):
        down = face_normal((k + N_FACES // 2) % N_FACES)
        tang = np.array([-down[2], down[0]])
        off = abs(float(c @ tang))
        height = R_IN - float(c @ down[[0, 2]])          # Schwerpunkt ueber Tisch
        tip = math.degrees(math.atan2(flat_half - off, height))
        print(f"  Seite {k} oben: Schwerpunkt {off:.1f} mm neben Auflagemitte, kippt ab {tip:.1f} Grad")
    if a.check:
        print('Passungspruefung:')
        if not check(body, cap, a.check):
            sys.exit(1)
    write_stl(body_for_print(body), f'{a.prefix}_koerper.stl', 'Zeitprisma Koerper')
    write_stl(build_cap_print(), f'{a.prefix}_deckel.stl', 'Zeitprisma Deckel')
    print('->', f'{a.prefix}_koerper.stl', f'{a.prefix}_deckel.stl')


if __name__ == '__main__':
    main()
