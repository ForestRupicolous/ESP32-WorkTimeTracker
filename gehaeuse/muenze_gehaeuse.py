#!/usr/bin/env python3
"""
Zeitprisma Muenze – N-Eck-"Muenze" (Standard: 8 Seiten, 22 mm dick) fuer den M5Stack StickS3 (K150)

Der Stick liegt flach in der Mitte, Display nach vorne. Auf dem Ruecken liegend
(Display oben) = Pause. Steht die Muenze auf dem Rand, ist die OBEN liegende
Randseite das aktive Projekt (1..N, im Uhrzeigersinn wie ein Zifferblatt).

    pip install manifold3d numpy matplotlib
    python3 muenze_gehaeuse.py                          # 8 Seiten, 22 mm, USB-Loch falls es passt
    python3 muenze_gehaeuse.py --seiten 12 --dicke 18.6 # fruehere 12-Eck-Variante
    python3 muenze_gehaeuse.py --usb nein               # ohne Ladeloch
    python3 muenze_gehaeuse.py --check StickS3.stl      # + Passungspruefung gegen M5-Strukturmodell

Aufbau: Tasche von vorne, Stahlscheibe im Boden haelt den Stick ueber seinen
Magnetruecken, Ausdrueckloch in der Rueckseite zum Herausnehmen. Seitentaster verdeckt.
USB-C-Loch (--usb auto): nur wenn die Seitenzahl durch 4 teilbar ist (sonst zeigt das
USB-Ende auf eine Ecke) und die Randflaeche breit genug ist, ohne die Muenze zu vergroessern.

Koordinaten: X = Stick-Laengsachse (USB-Ende bei -X), Y = quer, Z = Muenzachse,
Rueckseite bei Z=0 (Druckbett), Vorderseite (Display) bei Z=T.
Randseite k hat die Normale bei 90 - 360/N*k Grad (N = +Y oben; bei 8 Seiten: 2 = +X, 6 = -X/USB).
Druck: so wie modelliert, Rueckseite aufs Bett, ohne Stuetzen (nur Bruecke ueber dem USB-Loch).
"""
import argparse, math, struct, sys
import numpy as np
from manifold3d import Manifold, CrossSection, JoinType, OpType, FillRule

# ------------------------------------------------------------------ Parameter
N_RIM        = 8       # per --seiten
THICKNESS    = 22.0    # per --dicke; Mehrdicke geht in den Boden (Display bleibt RECESS tief)

# StickS3 (offizielles M5-Strukturmodell; Stick-Koordinaten sx 0..24 quer,
# sy 0..48 laengs ab USB-Ende, sz 0 = Front, -14.08 = Rueckseite)
STICK_W, STICK_L, STICK_T = 24.0, 48.0, 14.08
BTN_PROTRUDE = 0.92
MAGNET_SY    = 24.4
USB_SXZ      = (11.7, -4.25)

FIT          = 0.30
POCKET_R     = 1.5     # Eckradius Tasche (Stick-Ecken sind staerker gerundet)
RECESS       = 1.5     # Stick-Front unter der Vorderseite (Taste A beruehrt Tisch nicht)
WALL_MIN     = 1.6
EDGE_CHAMFER = 0.8
ENTRY_CHAMF  = 0.5

WASHER_D, WASHER_T = 16.0, 1.6     # M8 DIN 125
PUSH_HOLE_D  = 10.0                # Ausdrueckloch
PUSH_HOLE_SY = 40.0                # Lage laengs (Stick-Koordinate, weg vom USB-Ende)
USB_HOLE     = (13.5, 7.0, 2.0)    # Breite, Hoehe, Eckradius

DIGIT_H      = 9.0     # wird bei schmalen Flaechen automatisch kleiner
DIGIT_DEPTH  = 0.6
DIGIT_FONT   = dict(family='DejaVu Sans', weight='bold')
SEG = 64

POCKET_HX = STICK_L / 2 + FIT
POCKET_HY = STICK_W / 2 + FIT
FLOOR_MIN = 3.0        # Boden mindestens (Scheibenmulde 1,8 + 1,2)


def configure(n_rim=N_RIM, thickness=THICKNESS):
    """Abgeleitete Groessen setzen (Modul-Globals, damit alle Funktionen sie sehen)."""
    global N_RIM, T, FLOOR, Z_FRONT, TAN
    N_RIM = int(n_rim)
    T = float(thickness)
    FLOOR = T - STICK_T - RECESS
    if FLOOR < FLOOR_MIN - 1e-9:
        raise SystemExit(f'--dicke mindestens {FLOOR_MIN + STICK_T + RECESS:.1f} mm')
    Z_FRONT = FLOOR + STICK_T
    TAN = math.tan(math.pi / N_RIM)


configure()


# ------------------------------------------------------------- Bausteine
def ngon_cs(r_in, chamfer, n=None):
    """Regelmaessiges n-Eck in (X,Y), Seitennormalen bei 90 + k*360/n Grad, Ecken gefast."""
    n = n or N_RIM
    r_out = r_in / math.cos(math.pi / n)
    V = [(r_out * math.cos(math.radians(90 + 180 / n + 360 * k / n)),
          r_out * math.sin(math.radians(90 + 180 / n + 360 * k / n))) for k in range(n)]
    pts = []
    for i, v in enumerate(V):
        for nb in (V[i - 1], V[(i + 1) % n]):
            d = np.subtract(nb, v); d /= np.linalg.norm(d)
            pts.append(tuple(np.add(v, d * chamfer)))
    return CrossSection([pts])


def rrect(hx, hy, r, cx=0.0, cy=0.0):
    cs = CrossSection.square((2 * hx, 2 * hy)).translate((cx - hx, cy - hy))
    return cs.offset(-r, JoinType.Miter).offset(r, JoinType.Round, circular_segments=SEG) if r > 0 else cs


def slab(cs, z0, z1):
    return Manifold.extrude(cs, z1 - z0).translate((0, 0, z0))


def circle(d, cx, cy):
    return CrossSection.circle(d / 2, SEG).translate((cx, cy))


def frame_from(w_axis, origin, up):
    w = np.asarray(w_axis, float); w /= np.linalg.norm(w)
    v = np.asarray(up, float); v = v - w * np.dot(v, w); v /= np.linalg.norm(v)
    u = np.cross(v, w)
    return np.column_stack([u, v, w, np.asarray(origin, float)]).tolist()


def glyph_cs(text, height, max_w):
    from matplotlib.textpath import TextPath
    from matplotlib.font_manager import FontProperties
    fp = FontProperties(**DIGIT_FONT)
    tp = TextPath((0, 0), text, size=10, prop=fp)
    # EvenOdd per FillRule (CrossSection.even_odd gibt es in manifold3d 3.5.4 nicht)
    cs = CrossSection([np.asarray(p) for p in tp.to_polygons() if len(p) >= 3], FillRule.EvenOdd)
    b = cs.bounds()
    ref = TextPath((0, 0), '8', size=10, prop=fp).get_extents()
    s = min(height / ref.height, max_w / (b[2] - b[0]))
    return cs.translate((-(b[0] + b[2]) / 2, -(ref.y0 + ref.y1) / 2)).scale((s, s))


def face_normal(k):
    a = math.radians(90 - 360 / N_RIM * k)
    return np.array([math.cos(a), math.sin(a), 0.0])


def stick_to_model(sx, sy, sz):
    """Stick-Koordinaten -> Modell (USB-Ende bei -X, Display nach +Z)."""
    return (np.asarray(sy) - STICK_L / 2, STICK_W / 2 - np.asarray(sx), Z_FRONT + np.asarray(sz))


def pocket_cs():
    return rrect(POCKET_HX, POCKET_HY, POCKET_R)


def usb_geom():
    ux, uy, uz = stick_to_model(USB_SXZ[0], 0, USB_SXZ[1])
    return float(uy), float(uz)


# ------------------------------------------------------------- Groesse
def solve_r(usb):
    r = 20.0
    pk = pocket_cs().offset(WALL_MIN, JoinType.Round, circular_segments=SEG)
    while (pk - ngon_cs(r, EDGE_CHAMFER)).area() > 1e-6:
        r = round(r + 0.05, 2)
    if usb:   # Loch in der Randseite bei -X braucht seitlich WALL_MIN bis zur Nachbarflaeche
        uy, _ = usb_geom()
        r = max(r, math.ceil((abs(uy) + USB_HOLE[0] / 2 + WALL_MIN + EDGE_CHAMFER) / TAN * 20) / 20)
    return r


def usb_face():
    return 3 * N_RIM // 4          # Randseite mit Normale bei -X (USB-Ende)


def resolve_usb(mode):
    """mode 'ja' | 'nein' | 'auto' -> (r, usb). auto: Loch nur, wenn es die Muenze nicht vergroessert."""
    if mode != 'nein' and N_RIM % 4:
        if mode == 'ja':
            raise SystemExit(f'USB-Loch braucht eine durch 4 teilbare Seitenzahl ({N_RIM} geht nicht)')
        return solve_r(False), False
    if mode == 'nein':
        return solve_r(False), False
    r0, r1 = solve_r(False), solve_r(True)
    if mode == 'ja' or r1 <= r0:
        return r1, True
    return r0, False


# ------------------------------------------------------------- Modell
def build(r, usb):
    outer = ngon_cs(r, EDGE_CHAMFER)
    inner = outer.offset(-EDGE_CHAMFER, JoinType.Miter)
    body = Manifold.batch_hull([slab(outer, EDGE_CHAMFER, T - EDGE_CHAMFER), slab(inner, 0, T)])
    cuts = [slab(pocket_cs(), FLOOR, T + 1),
            Manifold.batch_hull([slab(pocket_cs(), T - ENTRY_CHAMF, T - ENTRY_CHAMF + 0.01),
                                 slab(pocket_cs().offset(ENTRY_CHAMF + 1, JoinType.Round), T + 1, T + 1.01)])]
    mx, my, _ = stick_to_model(STICK_W / 2, MAGNET_SY, 0)
    cuts.append(slab(circle(WASHER_D + 0.4, float(mx), float(my)), FLOOR - WASHER_T - 0.2, FLOOR + 0.5))
    px, py, _ = stick_to_model(STICK_W / 2, PUSH_HOLE_SY, 0)
    cuts.append(slab(circle(PUSH_HOLE_D, float(px), float(py)), -1, FLOOR + 0.5))
    if usb:
        uy, uz = usb_geom()
        w, h, rr = USB_HOLE
        prof = rrect(w / 2, h / 2, rr, -uy, uz)                    # lokal (u=-Y, v=Z)
        tun = Manifold.extrude(prof, r + 2 - POCKET_HX + 0.5)       # entlang lokal +w
        # (u, v, w) -> (X=-w+x0, Y=-u, Z=v): echte Drehung (det=+1)
        tun = tun.transform([[0, 0, -1, -POCKET_HX + 0.5], [-1, 0, 0, 0], [0, 1, 0, 0]])
        cuts.append(tun)
    # Ziffern auf den Randseiten, lesbar von vorne-oben (Ziffer-"oben" zeigt nach hinten, -Z)
    zc = T / 2
    if usb:
        _, uz = usb_geom()
        zc = (EDGE_CHAMFER + uz - USB_HOLE[1] / 2) / 2              # alle Ziffern unterhalb des Lochs
    flat = 2 * r * TAN - 2 * EDGE_CHAMFER
    for k in range(1, N_RIM + 1):
        n = face_normal(k)
        label = f"{k}." if k in (6, 9) else str(k)
        g = Manifold.extrude(glyph_cs(label, DIGIT_H, flat - 3.0), DIGIT_DEPTH + 1).translate((0, 0, -DIGIT_DEPTH))
        cuts.append(g.transform(frame_from(n, n * r + np.array([0, 0, zc]), (0, 0, -1))))
    return body - Manifold.batch_boolean(cuts, OpType.Add), zc


# ------------------------------------------------------------- Ausgabe/Pruefung
def write_stl(m, path, name):
    mesh = m.to_mesh()
    v = np.asarray(mesh.vert_properties)[:, :3]
    tri = v[np.asarray(mesh.tri_verts)]
    nrm = np.cross(tri[:, 1] - tri[:, 0], tri[:, 2] - tri[:, 0])
    nrm /= np.linalg.norm(nrm, axis=1)[:, None] + 1e-12
    with open(path, 'wb') as fh:
        fh.write(name.encode().ljust(80, b' '))
        fh.write(struct.pack('<I', len(tri)))
        rec = np.zeros(len(tri), dtype=[('n', '<f4', 3), ('v', '<f4', (3, 3)), ('a', '<u2')])
        rec['n'] = nrm; rec['v'] = tri
        fh.write(rec.tobytes())


def load_stick(path):
    b = open(path, 'rb').read()
    n = struct.unpack('<I', b[80:84])[0]
    t = np.frombuffer(b, dtype=[('n', '<f4', 3), ('v', '<f4', (3, 3)), ('a', '<u2')], count=n, offset=84)
    tri = t['v'].astype(float)
    return tri[tri.mean(1)[:, 0] < 26]


def stability(m, r, stick_mass=20.0, fill=0.35, washer_mass=2.5):
    mesh = m.to_mesh()
    v = np.asarray(mesh.vert_properties)[:, :3]
    t = v[np.asarray(mesh.tri_verts)]
    vol6 = np.einsum('ij,ij->i', t[:, 0], np.cross(t[:, 1], t[:, 2]))
    com = (vol6[:, None] * t.sum(1)).sum(0) / (4 * vol6.sum())
    m_shell = vol6.sum() / 6 / 1000 * 1.24 * fill
    mx, my, _ = stick_to_model(STICK_W / 2, MAGNET_SY, 0)
    parts = [(m_shell, com), (stick_mass, np.array([0, 0, Z_FRONT - STICK_T / 2])),
             (washer_mass, np.array([float(mx), float(my), FLOOR - WASHER_T / 2]))]
    c = sum(p * q for p, q in parts) / sum(p for p, _ in parts)
    flat_half = r * TAN - EDGE_CHAMFER
    rows = []
    for k in range(1, N_RIM + 1):                       # k oben -> steht auf k+N/2
        down = face_normal((k + N_RIM // 2 - 1) % N_RIM + 1)
        tang = np.array([-down[1], down[0], 0])
        h = r - float(c @ down)
        roll = math.degrees(math.atan2(flat_half - abs(float(c @ tang)), h))
        side = math.degrees(math.atan2(min(c[2] - EDGE_CHAMFER, T - EDGE_CHAMFER - c[2]), h))
        rows.append((k, roll, side))
    return m_shell, c, rows


def check(m, r, usb, stick_path):
    ok = True

    def rep(name, cond, detail):
        nonlocal ok
        ok &= bool(cond)
        print(f"  [{'OK ' if cond else 'FEHLER'}] {name}: {detail}")

    tri = load_stick(stick_path)
    V = tri.reshape(-1, 3)
    P = np.column_stack(stick_to_model(V[:, 0], V[:, 1], V[:, 2]))
    e = 1e-3
    rep('Stick in Tasche (X)', np.abs(P[:, 0]).max() <= POCKET_HX + e, f'|x|max={np.abs(P[:,0]).max():.2f} <= {POCKET_HX:.2f}')
    rep('Stick in Tasche (Y)', np.abs(P[:, 1]).max() <= POCKET_HY + e, f'|y|max={np.abs(P[:,1]).max():.2f} <= {POCKET_HY:.2f}')
    rep('Stick liegt auf Boden', abs(P[:, 2].min() - FLOOR) < 0.05, f'z_min={P[:,2].min():.2f}, Boden={FLOOR:.2f}')
    rep('Taste A unter Vorderseite', P[:, 2].max() < T - 0.3, f'{T - P[:,2].max():.2f} mm Luft, wenn Display nach unten')
    rng = np.random.default_rng(1)
    samp = P[rng.choice(len(P), 4000, replace=False)]
    ctr = np.array([0, 0, Z_FRONT - STICK_T / 2])
    samp = ctr + (samp - ctr) * 0.995
    probes = Manifold.batch_boolean([Manifold.cube((0.05, 0.05, 0.05), True).translate(tuple(p)) for p in samp[::8]], OpType.Add)
    rep('Keine Kollision (Stichprobe)', (probes ^ m).volume() < 1e-6, 'Schnittvolumen 0')
    # Wand um Tasche
    wall = None
    for d in np.arange(0.5, 10, 0.05):
        if (pocket_cs().offset(d, JoinType.Round, circular_segments=SEG) - ngon_cs(r, EDGE_CHAMFER)).area() > 1e-6:
            wall = d - 0.05; break
    rep(f'Wand >= {WALL_MIN} mm um Tasche', wall >= WALL_MIN - 1e-6, f'min. {wall:.2f} mm (Ecken), Stirnwand {r - POCKET_HX:.1f} mm')
    mx, my, _ = stick_to_model(STICK_W / 2, MAGNET_SY, 0)
    px, py, _ = stick_to_model(STICK_W / 2, PUSH_HOLE_SY, 0)
    gap = math.hypot(float(px - mx), float(py - my)) - (WASHER_D + 0.4) / 2 - PUSH_HOLE_D / 2
    rep('Ausdrueckloch neben Scheibe', gap >= 1.5, f'{gap:.1f} mm Steg')
    rep('Boden unter Scheibe', FLOOR - WASHER_T - 0.2 >= 1.0, f'{FLOOR - WASHER_T - 0.2:.1f} mm')
    if usb:
        uy, uz = usb_geom()
        side = r * TAN - EDGE_CHAMFER - (abs(uy) + USB_HOLE[0] / 2)
        top = T - EDGE_CHAMFER - (uz + USB_HOLE[1] / 2)
        rep('USB-Loch Seitenwand', side >= WALL_MIN - 1e-6, f'{side:.2f} mm')
        rep('USB-Loch Steg zur Vorderseite', top >= 1.2, f'{top:.2f} mm (Bruecke beim Druck {USB_HOLE[0]} mm)')
    return ok


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--seiten', type=int, default=N_RIM, help='Anzahl Randseiten (Standard 8)')
    ap.add_argument('--dicke', type=float, default=THICKNESS, help='Dicke in mm (Standard 22)')
    ap.add_argument('--usb', choices=('auto', 'ja', 'nein'), default='auto', help='USB-C-Ladeloch')
    ap.add_argument('--check', metavar='STICK_STL')
    ap.add_argument('-o', default=None)
    a = ap.parse_args()
    configure(a.seiten, a.dicke)
    r, usb = resolve_usb(a.usb)
    m, zc = build(r, usb)
    print(f"{N_RIM}-Eck-Muenze{f' mit USB-Loch (Seite {usb_face()})' if usb else ' ohne USB-Loch'}: "
          f"Schluesselweite {2*r:.1f} mm, Eckmass {2*r/math.cos(math.pi/N_RIM):.1f} mm, Dicke {T:.1f} mm, "
          f"Randflaeche {2*r*TAN:.1f} mm, Boden {FLOOR:.1f} mm")
    print(f"Volumen {m.volume()/1000:.1f} cm3, Status {m.status()}, Genus {m.genus()}")
    ms, c, rows = stability(m, r)
    print(f"Gewicht ca. {ms:.0f} g Druck (35 % Fuellgrad) + 20 g Stick; Schwerpunkt "
          f"x={c[0]:+.1f} y={c[1]:+.1f} z={c[2]:.1f} mm (Mitte {T/2:.1f})")
    roll = min(x[1] for x in rows); side = min(x[2] for x in rows)
    print(f"Stehend auf dem Rand: kippt zur Nachbarseite ab {roll:.1f} Grad, flach um ab {side:.1f} Grad "
          f"(schlechteste Seite)")
    if a.check:
        print('Passungspruefung:')
        if not check(m, r, usb, a.check):
            sys.exit(1)
    out = a.o or f"zeitprisma_muenze{N_RIM}_{T:g}mm{'_usb' if usb else ''}.stl"
    write_stl(m, out, f'Zeitprisma Muenze {N_RIM}')
    print('->', out)


if __name__ == '__main__':
    main()
