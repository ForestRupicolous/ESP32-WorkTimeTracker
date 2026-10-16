#!/usr/bin/env python3
"""
Zeitprisma – Sechskant-Gehaeuse fuer den M5Stack StickS3 (SKU K150)

Parametrisches Modell, erzeugt eine druckfertige STL.
    pip install manifold3d numpy
    python3 prisma_gehaeuse.py                 # -> zeitprisma_gehaeuse.stl
    python3 prisma_gehaeuse.py --check STL     # + Passungspruefung gegen M5-Strukturmodell

Koordinaten (Modell):  X = Breite, Y = Laengsachse (USB-C-Ende bei Y=0),
                       Z = Hoehe, Seite 0 (Display) zeigt nach +Z.
Die STL wird so ausgegeben, dass Seite 3 (gegenueber Display) auf dem Druckbett liegt:
keine Stuetzen noetig.

Seitennummern: 0 = Display (Pause/Status, wenn oben), 1..5 = Projekte
(Wuerfelaugen eingraviert), im Uhrzeigersinn von +Y aus gesehen.
"""
import argparse, math, struct, sys
import numpy as np
from manifold3d import Manifold, CrossSection, JoinType, OpType

# ------------------------------------------------------------------ Parameter
# StickS3 (aus offiziellem M5-Strukturmodell vermessen, docs: 48.0 x 24.0 x 15.0 mm)
STICK_W      = 24.0    # Breite
STICK_L      = 48.0    # Laenge
STICK_T      = 14.08   # Koerperdicke ohne Taste
BTN_PROTRUDE = 0.92    # Taste A ragt vorne heraus
MAGNET_Y     = 24.4    # Mitte Magnet auf Rueckseite, ab USB-Ende

FIT          = 0.30    # Spiel je Seite (PLA/PETG, 0.4er Duese)
RECESS       = 1.50    # Stick-Front liegt so tief unter Seite 0 (Taste beruehrt Tisch nicht)
FRAME_MIN    = 2.00    # Rahmenbreite neben der Tasche an Seite 0
WALL_END     = 2.40    # Stirnwaende
EDGE_CHAMFER = 0.60    # Fase an allen Aussenkanten
ENTRY_CHAMF  = 0.60    # Einfuehrfase Tasche

WASHER_D     = 16.0    # Stahlscheibe M8 DIN 125 (16 x 1.6) – haelt Stick ueber dessen Magnet
WASHER_T     = 1.6

USB_NOTCH_W  = 13.5    # Aussparung fuer USB-C-Stecker (Umspritzung ~12 x 6.5)
USB_NOTCH_D  = 8.5     # Tiefe ab Stick-Front
GRIP_NOTCH_W = 14.0    # Griffmulde am oberen Ende zum Herausnehmen
GRIP_NOTCH_D = 7.0

SIDE_HOLE_D  = 5.0     # Zugang zu den Seitentastern (Kugelschreiber/Stift)
# Seitentaster-Mitten in Stick-Koordinaten (sx, sy, sz); sz=0 ist die Front
SIDE_BUTTONS = [(0.0, 8.25, -6.5),     # Taster an Seite sx=0
                (24.0, 24.5, -6.25)]   # Taster an Seite sx=24

PIP_D        = 4.0     # Wuerfelaugen
PIP_DEPTH    = 0.8
PIP_PITCH    = 7.0
PIP_FROM_TOP = 11.0    # Abstand Augenmuster-Mitte vom oberen Ende

SEG = 64               # Kreissegmente

# ------------------------------------------------------------- abgeleitet
POCKET_HW = STICK_W / 2 + FIT
POCKET_L  = STICK_L + 2 * FIT
L         = POCKET_L + 2 * WALL_END
# halbe Flaechenbreite von Seite 0 muss Tasche + Rahmen + Fase tragen
S_HALF    = POCKET_HW + FRAME_MIN + EDGE_CHAMFER
R_IN      = S_HALF / math.tan(math.radians(30))     # Inkreisradius (halbe Schluesselweite)
R_OUT     = R_IN / math.cos(math.radians(30))
Z_FRONT   = R_IN - RECESS                            # Hoehe der Stick-Front
Z_FLOOR   = Z_FRONT - STICK_T                        # Taschenboden (Stick liegt auf)
Y0        = WALL_END + FIT                           # Stick-Unterkante (USB-Ende)

FACE_ANGLES = [90 - 60 * k for k in range(6)]        # Normalenwinkel in XZ, Seite 0 = +Z


def stick_to_model(sx, sy, sz):
    return (sx - STICK_W / 2, Y0 + sy, Z_FRONT + sz)


# ------------------------------------------------------------- Bausteine
def hex_cs(r_in, chamfer):
    """Sechseck (Inkreisradius r_in) mit gefasten Ecken, in (X, Z)."""
    r_out = r_in / math.cos(math.radians(30))
    verts = [(r_out * math.cos(math.radians(a)), r_out * math.sin(math.radians(a)))
             for a in range(0, 360, 60)]
    pts = []
    for i, v in enumerate(verts):
        prev, nxt = verts[i - 1], verts[(i + 1) % 6]
        for nb in (prev, nxt):
            d = np.subtract(nb, v)
            d /= np.linalg.norm(d)
            pts.append(tuple(np.add(v, d * chamfer)))
    # Reihenfolge: prev-Seite, next-Seite -> CCW
    return CrossSection([pts])


def prism_x_z(cs, y0, length):
    """Extrudiert einen (X,Z)-Querschnitt entlang +Y von y0 bis y0+length."""
    m = Manifold.extrude(cs, length)          # Querschnitt (X,Y) -> 3D, Laenge entlang +Z
    m = m.rotate((90, 0, 0))                  # (x, y, z) -> (x, -z, y)
    return m.translate((0, y0 + length, 0))


def box(x0, x1, y0, y1, z0, z1):
    return Manifold.cube((x1 - x0, y1 - y0, z1 - z0)).translate((x0, y0, z0))


def cyl_along(p, n, d, h0, h1):
    """Zylinder Durchmesser d entlang Richtung n, von p+h0*n bis p+h1*n."""
    n = np.asarray(n, float); n /= np.linalg.norm(n)
    c = Manifold.cylinder(h1 - h0, d / 2, d / 2, SEG)            # entlang +Z
    # Rotation +Z -> n
    z = np.array([0, 0, 1.0])
    v = np.cross(z, n); s = np.linalg.norm(v); cth = float(np.dot(z, n))
    if s < 1e-9:
        R = np.eye(3) if cth > 0 else np.diag([1, -1, -1])
    else:
        vx = np.array([[0, -v[2], v[1]], [v[2], 0, -v[0]], [-v[1], v[0], 0]])
        R = np.eye(3) + vx + vx @ vx * ((1 - cth) / s ** 2)
    p0 = np.asarray(p, float) + n * h0
    M = np.hstack([R, p0.reshape(3, 1)])
    return c.transform(M.tolist())


# ------------------------------------------------------------- Modell
def build():
    # Grundkoerper mit gefasten Laengs- und Stirnkanten
    outer = hex_cs(R_IN, EDGE_CHAMFER)
    inner = outer.offset(-EDGE_CHAMFER, JoinType.Miter)
    body = Manifold.batch_hull([
        prism_x_z(outer, EDGE_CHAMFER, L - 2 * EDGE_CHAMFER),
        prism_x_z(inner, 0, L),
    ])

    cuts = []
    # Tasche (nach oben offen) + Einfuehrfase
    cuts.append(box(-POCKET_HW, POCKET_HW, WALL_END, WALL_END + POCKET_L, Z_FLOOR, R_IN + 1))
    cuts.append(Manifold.batch_hull([
        box(-POCKET_HW, POCKET_HW, WALL_END, WALL_END + POCKET_L, R_IN - ENTRY_CHAMF, R_IN - ENTRY_CHAMF + 0.01),
        box(-POCKET_HW - ENTRY_CHAMF - 1, POCKET_HW + ENTRY_CHAMF + 1,
            WALL_END - ENTRY_CHAMF - 1, WALL_END + POCKET_L + ENTRY_CHAMF + 1, R_IN + 1, R_IN + 1.01),
    ]))
    # Stahlscheibe im Taschenboden unter dem Magneten
    wx, wy, _ = stick_to_model(STICK_W / 2, MAGNET_Y, 0)
    cuts.append(cyl_along((wx, wy, Z_FLOOR), (0, 0, 1), WASHER_D + 0.4, -(WASHER_T + 0.2), 0.5))
    # USB-C-Aussparung unten, Griffmulde oben
    cuts.append(box(-USB_NOTCH_W / 2, USB_NOTCH_W / 2, -1, WALL_END + 0.5,
                    Z_FRONT - USB_NOTCH_D, R_IN + 1))
    cuts.append(box(-GRIP_NOTCH_W / 2, GRIP_NOTCH_W / 2, WALL_END + POCKET_L - 0.5, L + 1,
                    Z_FRONT - GRIP_NOTCH_D, R_IN + 1))
    # Zugangsloecher Seitentaster
    for sx, sy, sz in SIDE_BUTTONS:
        p = stick_to_model(sx, sy, sz)
        nx = -1.0 if sx < STICK_W / 2 else 1.0
        cuts.append(cyl_along((nx * POCKET_HW, p[1], p[2]), (nx, 0, 0), SIDE_HOLE_D, -0.5, R_OUT))
    # Wuerfelaugen auf Seiten 1..5
    pip_layout = {
        1: [(0, 0)],
        2: [(-1, -1), (1, 1)],
        3: [(-1, -1), (0, 0), (1, 1)],
        4: [(-1, -1), (-1, 1), (1, -1), (1, 1)],
        5: [(-1, -1), (-1, 1), (0, 0), (1, -1), (1, 1)],
    }
    yc = L - PIP_FROM_TOP
    for k in range(1, 6):
        a = math.radians(FACE_ANGLES[k])
        n = np.array([math.cos(a), 0, math.sin(a)])
        t = np.array([-math.sin(a), 0, math.cos(a)])
        for u, v in pip_layout[k]:
            p = n * R_IN + t * (u * PIP_PITCH / 2) + np.array([0, yc + v * PIP_PITCH / 2, 0])
            cuts.append(cyl_along(p, n, PIP_D, -PIP_DEPTH, 1.0))

    return body - Manifold.batch_boolean(cuts, OpType.Add)


def write_stl(m, path):
    mesh = m.to_mesh()
    v = np.asarray(mesh.vert_properties)[:, :3]
    f = np.asarray(mesh.tri_verts)
    tri = v[f]
    nrm = np.cross(tri[:, 1] - tri[:, 0], tri[:, 2] - tri[:, 0])
    nrm /= np.linalg.norm(nrm, axis=1)[:, None] + 1e-12
    with open(path, 'wb') as fh:
        fh.write(b'Zeitprisma StickS3 Gehaeuse'.ljust(80, b' '))
        fh.write(struct.pack('<I', len(f)))
        rec = np.zeros(len(f), dtype=[('n', '<f4', 3), ('v', '<f4', (3, 3)), ('a', '<u2')])
        rec['n'] = nrm; rec['v'] = tri
        fh.write(rec.tobytes())


def for_print(m):
    """Seite 3 aufs Bett: Modell um R_IN anheben, Mitte auf XY=0."""
    return m.translate((0, -L / 2, R_IN))


# ------------------------------------------------------------- Pruefung
def load_stick(path):
    b = open(path, 'rb').read()
    n = struct.unpack('<I', b[80:84])[0]
    t = np.frombuffer(b, dtype=[('n', '<f4', 3), ('v', '<f4', (3, 3)), ('a', '<u2')], count=n, offset=84)
    tri = t['v'].astype(float)
    tri = tri[tri.mean(1)[:, 0] < 26]          # Teil 0 = komplette Baugruppe
    return tri                                  # Stick-Koordinaten: x 0..24, y 0..48, z -14.08..+0.92


def check(m, stick_path):
    ok = True
    def rep(name, cond, detail):
        nonlocal ok
        ok &= bool(cond)
        print(f"  [{'OK ' if cond else 'FEHLER'}] {name}: {detail}")

    tri = load_stick(stick_path)
    V = tri.reshape(-1, 3)
    P = np.column_stack(stick_to_model(V[:, 0], V[:, 1], V[:, 2]))
    eps = 1e-3
    inx = np.all(np.abs(P[:, 0]) <= POCKET_HW + eps)
    iny = np.all((P[:, 1] >= WALL_END - eps) & (P[:, 1] <= WALL_END + POCKET_L + eps))
    inz = np.all(P[:, 2] >= Z_FLOOR - eps)
    rep('Stick vollstaendig in Tasche (X)', inx, f'|x|max={np.abs(P[:,0]).max():.2f} <= {POCKET_HW:.2f}')
    rep('Stick vollstaendig in Tasche (Y)', iny, f'y={P[:,1].min():.2f}..{P[:,1].max():.2f}, Tasche {WALL_END:.2f}..{WALL_END+POCKET_L:.2f}')
    rep('Stick liegt auf Boden', inz and abs(P[:, 2].min() - Z_FLOOR) < 0.05, f'z_min={P[:,2].min():.2f}, Boden={Z_FLOOR:.2f}')
    top = P[:, 2].max()
    rep('Taste A unter Flaeche 0 (Display nach unten liegt nichts auf)', top < R_IN - 0.1,
        f'Taste oben {top:.2f}, Flaeche {R_IN:.2f} -> {R_IN-top:.2f} mm Luft')
    # Stichproben: Punkte im Stick duerfen nicht im Material liegen -> Winding number
    rng = np.random.default_rng(1)
    samp = P[rng.choice(len(P), 4000, replace=False)]
    # leicht nach innen ziehen (Oberflaechenpunkte liegen genau auf Taschenwand)
    ctr = np.array([0, Y0 + STICK_L / 2, Z_FRONT - STICK_T / 2])
    samp = ctr + (samp - ctr) * 0.995
    # Material-Test ueber kleine Wuerfel
    probes = Manifold.batch_boolean([Manifold.cube((0.05, 0.05, 0.05), True).translate(tuple(p)) for p in samp[::8]],
                                    OpType.Add)
    inter = (probes ^ m).volume()
    rep('Keine Kollision Stick/Gehaeuse (Stichprobe)', inter < 1e-6, f'Schnittvolumen {inter:.6f} mm3')

    # Rahmen Seite 0 und Waende
    frame = S_HALF - EDGE_CHAMFER - POCKET_HW
    rep('Rahmen an Seite 0 (flach)', frame >= 1.2, f'{frame:.2f} mm (+{EDGE_CHAMFER} Fase)')
    # Wand Seitentaster-Loch: Laenge des Kanals
    for sx, sy, sz in SIDE_BUTTONS:
        x, y, z = stick_to_model(sx, sy, sz)
        # Abstand von Taschenwand bis Aussenflaeche in +-X auf Hoehe z
        a = math.radians(30)
        xs = (R_IN - z * math.sin(a)) / math.cos(a)   # Flaeche 1/5 (Normale +-30 Grad), gilt fuer z>=0
        rep(f'Tasterkanal bei sx={sx:.0f}', xs - POCKET_HW < 10, f'{xs-POCKET_HW:.1f} mm lang, D={SIDE_HOLE_D}')
    # USB
    usb_x = (np.array([6.5, 17.0]) - STICK_W / 2)
    usb_z = Z_FRONT + np.array([-5.5, -2.0])
    rep('USB-C-Buchse frei', abs(usb_x).max() < USB_NOTCH_W / 2 - 0.5 and usb_z.min() > Z_FRONT - USB_NOTCH_D + 1,
        f'Buchse x{usb_x.round(1)} z{(usb_z-Z_FRONT).round(1)} in Aussparung {USB_NOTCH_W}x{USB_NOTCH_D}')
    # Boden unter Scheibe
    rep('Material unter Stahlscheibe', Z_FLOOR - WASHER_T - 0.2 + R_IN > 3,
        f'{Z_FLOOR - WASHER_T - 0.2 + R_IN:.1f} mm')
    return ok


def stability(m, stick_mass=20.0, fill=0.45, washer_mass=2.5):
    """Schwerpunkt (XZ) von Gehaeuse + Stick + Scheibe.
    fill: effektiver Fuellgrad des Drucks (Waende + Infill), PLA 1.24 g/cm3."""
    mesh = m.to_mesh()
    v = np.asarray(mesh.vert_properties)[:, :3]
    t = v[np.asarray(mesh.tri_verts)]
    vol6 = np.einsum('ij,ij->i', t[:, 0], np.cross(t[:, 1], t[:, 2]))   # 6*Tetraedervolumen
    com_shell = (vol6[:, None] * t.sum(1)).sum(0) / (4 * vol6.sum())
    m_shell = vol6.sum() / 6 / 1000 * 1.24 * fill
    parts = [(m_shell, com_shell[[0, 2]]),
             (stick_mass, np.array([0.0, Z_FRONT - STICK_T / 2])),
             (washer_mass, np.array([0.0, Z_FLOOR - WASHER_T / 2]))]
    mt = sum(p[0] for p in parts)
    c = sum(p[0] * p[1] for p in parts) / mt
    return m_shell, c


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--check', metavar='STICK_STL', help='M5 Strukturmodell StickS3.stl')
    ap.add_argument('-o', default='zeitprisma_gehaeuse.stl')
    a = ap.parse_args()

    m = build()
    print(f"Sechskant: Schluesselweite {2*R_IN:.1f} mm, Eckmass {2*R_OUT:.1f} mm, Laenge {L:.1f} mm")
    print(f"Flaechenbreite {2*S_HALF/1:.1f} mm, Stick-Front {RECESS} mm unter Seite 0")
    print(f"Volumen {m.volume()/1000:.1f} cm3, Dreiecke {m.num_tri()}, Status {m.status()}")
    ms, c = stability(m)
    print(f"Gewicht ca. {ms:.0f} g Gehaeuse (+20 g Stick), Schwerpunkt x={c[0]:+.1f} z={c[1]:+.1f} mm")
    flat_half = S_HALF - EDGE_CHAMFER
    for k in range(6):
        th = math.radians(FACE_ANGLES[(k + 3) % 6])          # aufliegende Seite
        tang = np.array([-math.sin(th), math.cos(th)])
        off = abs(float(c @ tang))
        print(f"  Seite {k} oben: Schwerpunkt {off:.1f} mm neben Auflagemitte, "
              f"Kipp-Reserve {flat_half-off:.1f} mm")
    if a.check:
        print('Passungspruefung:')
        if not check(m, a.check):
            sys.exit(1)
    write_stl(for_print(m), a.o)
    print('->', a.o)


if __name__ == '__main__':
    main()
