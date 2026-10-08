"""Genera un bloque de viviendas de hormigón de 4 plantas (estilo años 70) para el benchmark.

Uso:
  blender -b -P gen_benchmark_building.py -- <salida.fbx> [seed]

Resultado: SM_BM_Apartment_A (~60-90k tris, pensado para Nanite), 4 materiales:
  0 MI_Wall, 1 MI_Trim (hormigón), 2 MI_Frame (marcos metal), 3 MI_Glass
"""
import math
import os
import random
import sys

sys.path.append(os.path.dirname(os.path.abspath(__file__)))
import bmesh
from mathutils import Matrix
import bl_lib

WALL, TRIM, FRAME, GLASS = 0, 1, 2, 3

W = 12.0          # ancho fachada (m)
D = 12.0          # fondo
FLOORS = 4
FH = 3.2          # altura planta
T = 0.3           # grosor muro
WIN_W, WIN_H, SILL = 1.4, 1.6, 0.9
WINS_PER_FLOOR = 3


def facade(bm, rng):
    """Fachada en coordenadas locales: X ancho, Z arriba, exterior hacia +Y, muro en y∈[-T,0]."""
    verts = []
    spacing = W / WINS_PER_FLOOR
    xs = [-W / 2 + spacing * (i + 0.5) for i in range(WINS_PER_FLOOR)]
    yc = -T / 2
    for f in range(FLOORS):
        z0 = f * FH
        # franja inferior (hasta alféizar) y superior (dintel hasta forjado)
        verts += bl_lib.add_box(bm, (0, yc, z0 + SILL / 2), (W, T, SILL), WALL)
        top = FH - SILL - WIN_H
        verts += bl_lib.add_box(bm, (0, yc, z0 + SILL + WIN_H + top / 2), (W, T, top), WALL)
        # pilares entre ventanas
        edges = [-W / 2] + [x for xc in xs for x in (xc - WIN_W / 2, xc + WIN_W / 2)] + [W / 2]
        for i in range(0, len(edges), 2):
            a, b = edges[i], edges[i + 1]
            if b - a > 0.01:
                verts += bl_lib.add_box(bm, ((a + b) / 2, yc, z0 + SILL + WIN_H / 2), (b - a, T, WIN_H), WALL)
        # cornisa de forjado
        verts += bl_lib.add_box(bm, (0, 0.06, z0 + FH - 0.1), (W + 0.12, 0.12, 0.2), TRIM)
        for xc in xs:
            zc = z0 + SILL + WIN_H / 2
            # alféizar
            verts += bl_lib.add_box(bm, (xc, 0.05, z0 + SILL - 0.04), (WIN_W + 0.15, 0.22, 0.08), TRIM)
            # marco (4 perfiles + parteluz) retranqueado
            fy = -0.12
            p = 0.06
            verts += bl_lib.add_box(bm, (xc, fy, z0 + SILL + p / 2), (WIN_W, 0.07, p), FRAME)
            verts += bl_lib.add_box(bm, (xc, fy, z0 + SILL + WIN_H - p / 2), (WIN_W, 0.07, p), FRAME)
            verts += bl_lib.add_box(bm, (xc - WIN_W / 2 + p / 2, fy, zc), (p, 0.07, WIN_H), FRAME)
            verts += bl_lib.add_box(bm, (xc + WIN_W / 2 - p / 2, fy, zc), (p, 0.07, WIN_H), FRAME)
            verts += bl_lib.add_box(bm, (xc, fy, zc), (0.05, 0.06, WIN_H), FRAME)
            # cristal
            verts += bl_lib.add_box(bm, (xc, fy - 0.01, zc), (WIN_W - 0.1, 0.01, WIN_H - 0.1), GLASS)
            # fondo oscuro (interior falso) para que no se vea el vacío
            verts += bl_lib.add_box(bm, (xc, -T + 0.02, zc), (WIN_W, 0.02, WIN_H), WALL)
            # detalles aleatorios: aire acondicionado / persiana
            r = rng.random()
            if r < 0.25 and f > 0:
                verts += bl_lib.add_box(bm, (xc + 0.3, 0.25, z0 + SILL - 0.35), (0.8, 0.35, 0.55), FRAME)
            elif r < 0.45:
                ph = rng.uniform(0.3, 1.2)
                verts += bl_lib.add_box(bm, (xc, -0.03, z0 + SILL + WIN_H - ph / 2), (WIN_W, 0.04, ph), TRIM)
    return verts


def build(seed=1):
    rng = random.Random(seed)
    bm = bmesh.new()
    H = FLOORS * FH
    # 4 fachadas rotadas alrededor del centro
    for i in range(4):
        vs = facade(bm, rng)
        rot = Matrix.Rotation(math.radians(90 * i), 4, 'Z')
        mat = rot @ Matrix.Translation((0, D / 2, 0))
        bmesh.ops.transform(bm, matrix=mat, verts=vs)
    # cubierta, peto y depósito
    bl_lib.add_box(bm, (0, 0, H - 0.1), (W - 0.2, D - 0.2, 0.2), TRIM)
    for i in range(4):
        vs = bl_lib.add_box(bm, (0, D / 2 - 0.15, H + 0.4), (W + 0.1, 0.3, 0.8), TRIM)
        bmesh.ops.transform(bm, matrix=Matrix.Rotation(math.radians(90 * i), 4, 'Z'), verts=vs)
    bl_lib.add_cylinder(bm, (2.5, -2.0, H + 1.2), 0.9, 2.0, 24, FRAME)
    bl_lib.add_box(bm, (-3.0, 2.0, H + 1.2), (2.0, 2.5, 2.4), WALL)  # caja de escalera
    bmesh.ops.remove_doubles(bm, verts=bm.verts, dist=0.0005)

    mats = [
        bl_lib.get_material("MI_Wall", (0.55, 0.52, 0.48, 1)),
        bl_lib.get_material("MI_Trim", (0.40, 0.40, 0.40, 1)),
        bl_lib.get_material("MI_Frame", (0.12, 0.12, 0.13, 1), roughness=0.5, metallic=0.6),
        bl_lib.get_material("MI_Glass", (0.02, 0.03, 0.04, 1), roughness=0.05),
    ]
    obj = bl_lib.bm_to_object(bm, "SM_BM_Apartment_A", mats)
    bl_lib.add_bevel(obj, width=0.015, segments=2)
    bl_lib.apply_modifiers(obj)
    return obj


if __name__ == "__main__":
    argv = sys.argv[sys.argv.index("--") + 1:] if "--" in sys.argv else []
    out = argv[0] if argv else os.path.join(os.path.dirname(__file__), "..", "export", "SM_BM_Apartment_A.fbx")
    seed = int(argv[1]) if len(argv) > 1 else 1
    bl_lib.reset_scene()
    obj = build(seed)
    print(f"[gen_benchmark_building] Triángulos: {bl_lib.triangle_count(obj)}")
    print(f"[gen_benchmark_building] Dimensiones (m): {tuple(round(d, 2) for d in obj.dimensions)}")
    bl_lib.export_fbx([obj], os.path.abspath(out))
