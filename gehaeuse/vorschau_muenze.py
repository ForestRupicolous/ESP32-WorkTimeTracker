#!/usr/bin/env python3
"""
Vorschaubild fuer die Muenze (Standard: 8 Seiten, 22 mm): erzeugt das Modell mit muenze_gehaeuse.py und zeichnet es
mit einem kleinen Z-Buffer-Rasterisierer (numpy, orthografisch, ohne Zusatzprogramme). Damit ist vorschau_muenze8.png
reproduzierbar.

    python3 vorschau_muenze.py                              # -> vorschau_muenze8.png (Modell allein)
    python3 vorschau_muenze.py --stick ../ref/StickS3.stl   # + StickS3 in der Tasche (make ref)

Ansichten: Ruecken unten (Pause), Rueckseite, aufgestellt mit Seite 1 oben, USB-Seite oben, Tasche ohne Stick
(beide Innenwaende mit dem eingravierten Text).
"""
import argparse

import numpy as np

import muenze_gehaeuse as mg


def unit(v):
    v = np.asarray(v, float)
    return v / np.linalg.norm(v)


def triangles(m):
    mesh = m.to_mesh()
    v = np.asarray(mesh.vert_properties)[:, :3]
    return v[np.asarray(mesh.tri_verts)]


def render(parts, d, up, size=720, ss=2, bg=(1.0, 1.0, 1.0)):
    """parts: Liste (Dreiecke (n,3,3), Grundfarbe). d: Richtung vom Objekt zur Kamera, up: Bildoben."""
    d, up = unit(d), unit(up)
    right = unit(np.cross(-d, up))
    true_up = np.cross(right, -d)
    basis = np.stack([right, true_up, d], axis=1)
    tris = np.concatenate([t for t, _ in parts])
    base = np.concatenate([np.tile(c, (len(t), 1)) for t, c in parts])
    P = tris @ basis                                   # (n,3,3): Bild-x, Bild-y, Tiefe (gross = naeher an der Kamera)
    nrm = np.cross(tris[:, 1] - tris[:, 0], tris[:, 2] - tris[:, 0])
    ln = np.linalg.norm(nrm, axis=1)
    keep = ln > 1e-12
    nrm = nrm[keep] / ln[keep][:, None]
    P, base = P[keep], base[keep]
    facing = nrm @ d
    vis = facing > 0                                   # Rueckseiten weglassen
    P, base, nrm, facing = P[vis], base[vis], nrm[vis], facing[vis]
    key = unit(0.55 * d - 0.5 * right + 0.65 * true_up)
    lam = np.clip(nrm @ key, 0, None)
    col = np.clip(base * (0.30 + 0.55 * lam[:, None] + 0.30 * facing[:, None]), 0, 1)

    W = H = size * ss
    xy = P[:, :, :2].reshape(-1, 2)
    lo, hi = xy.min(0), xy.max(0)
    sc = 0.92 * W / max(hi[0] - lo[0], hi[1] - lo[1])
    off = np.array([W / 2, H / 2]) - sc * (lo + hi) / 2
    X = P[:, :, 0] * sc + off[0]
    Y = H - (P[:, :, 1] * sc + off[1])
    Z = P[:, :, 2]
    zbuf = np.full((H, W), -1e18)
    img = np.empty((H, W, 3))
    img[:] = bg
    for i in np.argsort(Z.mean(1)):                   # grob von hinten nach vorne (der Z-Buffer entscheidet)
        x0, x1, x2 = X[i]; y0, y1, y2 = Y[i]
        xmin, xmax = int(max(0, np.floor(min(x0, x1, x2)))), int(min(W - 1, np.ceil(max(x0, x1, x2))))
        ymin, ymax = int(max(0, np.floor(min(y0, y1, y2)))), int(min(H - 1, np.ceil(max(y0, y1, y2))))
        if xmax < xmin or ymax < ymin:
            continue
        den = (y1 - y2) * (x0 - x2) + (x2 - x1) * (y0 - y2)
        if abs(den) < 1e-12:
            continue
        gx, gy = np.meshgrid(np.arange(xmin, xmax + 1) + 0.5, np.arange(ymin, ymax + 1) + 0.5)
        w0 = ((y1 - y2) * (gx - x2) + (x2 - x1) * (gy - y2)) / den
        w1 = ((y2 - y0) * (gx - x2) + (x0 - x2) * (gy - y2)) / den
        w2 = 1 - w0 - w1
        eps = -1e-4
        inside = (w0 >= eps) & (w1 >= eps) & (w2 >= eps)
        if not inside.any():
            continue
        z = w0 * Z[i, 0] + w1 * Z[i, 1] + w2 * Z[i, 2]
        sl = (slice(ymin, ymax + 1), slice(xmin, xmax + 1))
        upd = inside & (z > zbuf[sl])
        zbuf[sl][upd] = z[upd]
        img[sl][upd] = col[i]
    # Kanten: Tiefensprung ueber 0,35 mm (Silhouette, Gravuren, Stufen) dunkel nachzeichnen, 2 Pixel breit
    gap = zbuf < -1e17
    edge = np.zeros((H, W), bool)
    for axis in (0, 1):
        jump = np.abs(np.diff(zbuf, axis=axis)) > 0.35
        lo_sl = [slice(None), slice(None)]; hi_sl = [slice(None), slice(None)]
        lo_sl[axis] = slice(None, -1); hi_sl[axis] = slice(1, None)
        edge[tuple(lo_sl)] |= jump
        edge[tuple(hi_sl)] |= jump
    edge &= ~gap
    img[edge] *= 0.5
    return img.reshape(size, ss, size, ss, 3).mean((1, 3))     # Supersampling


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--seiten', type=int, default=mg.N_RIM)
    ap.add_argument('--dicke', type=float, default=mg.THICKNESS)
    ap.add_argument('--stick', metavar='STICK_STL', help='StickS3-Strukturmodell (make ref), wird in die Tasche gesetzt')
    ap.add_argument('-o', default='vorschau_muenze8.png')
    a = ap.parse_args()
    mg.configure(a.seiten, a.dicke)
    r, usb = mg.resolve_usb('auto')
    m, _ = mg.build(r, usb)
    shell = [(triangles(m), np.array([0.93, 0.60, 0.25]))]
    with_stick = list(shell)
    if a.stick:
        V = mg.load_stick(a.stick)
        P = np.column_stack(mg.stick_to_model(V.reshape(-1, 3)[:, 0], V.reshape(-1, 3)[:, 1], V.reshape(-1, 3)[:, 2]))
        with_stick.append((P.reshape(-1, 3, 3), np.array([0.38, 0.41, 0.48])))
    k_up = mg.N_RIM // 2                              # Geometrie-Index der Seite "1" (Normale -Y)
    n1, n3 = mg.face_normal(k_up), mg.face_normal(mg.usb_face())
    up = lambda n: 0.45 * n + np.array([0, 0, 0.89])  # Kamera ueber der oben liegenden Seite, von vorn
    views = [
        ('Ruecken unten = Pause', with_stick, [0, -0.55, 0.83], [0, 1, 0]),
        ('Rueckseite (geschlossen)', shell, [0, -0.5, -0.87], [0, -1, 0]),
        (f'aufgestellt: Seite {mg.face_label(k_up)} oben', with_stick, up(n1), n1),
        (f'Seite {mg.face_label(mg.usb_face())} oben: USB-C-Loch', with_stick, up(n3), n3),
        ('Tasche ohne Stick: Wand +Y', shell, [0, -0.77, 0.64], [0, 0, 1]),
        ('Tasche ohne Stick: Wand -Y', shell, [0, 0.77, 0.64], [0, 0, 1]),
    ]
    import matplotlib
    matplotlib.use('Agg')
    import matplotlib.pyplot as plt
    fig, axs = plt.subplots(2, 3, figsize=(15, 10.4))
    for ax, (title, parts, d, upv) in zip(axs.ravel(), views):
        ax.imshow(np.clip(render(parts, d, upv), 0, 1))
        ax.set_title(title, fontsize=11)
        ax.axis('off')
    fig.tight_layout()
    fig.savefig(a.o, dpi=120)
    print('->', a.o)


if __name__ == '__main__':
    main()
