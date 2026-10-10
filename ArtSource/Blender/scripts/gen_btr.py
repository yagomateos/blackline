"""Blindado de ruedas 8x8 de la Columna (Bloque 11, fase 8). Diseño propio (inspirado en los BTR genéricos).

Uso:  blender -b --factory-startup -P gen_btr.py -- <carpeta_salida>
Salida: SM_BTR_Hull (casco + ruedas, Nanite, colisión UCX) y SM_BTR_Turret (torreta; pivote en el eje de giro,
mira a +X) y SM_BTR_Gun (cañón; pivote en los muñones, x 60 z 27 de la torreta). Medidas: 760 x 290 x 230 cm (casco), torreta Ø 150.
"""
import math
import os
import sys

sys.path.append(os.path.dirname(os.path.abspath(__file__)))
import bpy
from mathutils import Matrix

import bl_lib
from bl_kit import Kit

M_PAINT = ("MI_BTR_Paint", (0.18, 0.2, 0.13, 1), 0.65, 0.3)
M_RUBBER = ("MI_Veh_Rubber", (0.02, 0.02, 0.02, 1), 0.9, 0.0)
M_TRIM = ("MI_Veh_Trim", (0.05, 0.05, 0.05, 1), 0.6, 0.0)
M_GLASS = ("MI_Veh_GlassOpaque", (0.01, 0.015, 0.02, 1), 0.05, 0.0)


def hull(out):
    k = Kit("SM_BTR_Hull", [M_PAINT, M_RUBBER, M_TRIM, M_GLASS])
    L, W = 760.0, 290.0
    # Casco: perfil lateral (x, z) con morro en cuña y laterales inclinados
    side = [(-380, 55), (330, 55), (380, 100), (300, 175), (-360, 205), (-380, 175)]
    k.prism("Hull", side, 'y', -W / 2 + 18, W / 2 - 18, 0, bevel=0.03)
    k.prism("HullTop", [(-360, 205), (300, 175), (290, 195), (-350, 222)], 'y', -W / 2 + 40, W / 2 - 40, 0, bevel=0.02)
    for s in (-1, 1):
        # Faldones laterales inclinados sobre las ruedas
        k.prism(f"Skirt{s}", [(-370, 95), (360, 95), (330, 175), (-350, 195)], 'y', s * (W / 2 - 20) - 9 * (s < 0), s * (W / 2 - 20) + 9 * (s > 0), 0, bevel=0.015)
        # Escotillas laterales, troneras y luces
        k.box(f"Door{s}", -40, 40, s * W / 2 - 3 * (s > 0), s * W / 2 + 3 * (s < 0), 110, 170, 2, bevel=0.005)
        for i, x in enumerate((-250, -130, 120, 230)):
            k.box(f"Port{s}{i}", x - 8, x + 8, s * W / 2 - 2 * (s > 0), s * W / 2 + 2 * (s < 0), 150, 165, 2, bevel=0.0)
        # 8 ruedas
        for i, x in enumerate((-285, -145, 115, 255)):
            k.lathe(f"Wheel{s}{i}", [(-24, 50), (-26, 56), (-22, 62), (22, 62), (26, 56), (24, 50)],
                    (x, s * (W / 2 - 34), 62), (0, 1, 0), seg=24, closed=True, mat=1)
            k.lathe(f"Rim{s}{i}", [(-20, 50.5), (20, 50.5)], (x, s * (W / 2 - 34), 62), (0, 1, 0), seg=20, mat=2)
            k.lathe(f"Hub{s}{i}", [(0, 20), (6, 18)], (x, s * (W / 2 - 10), 62), (0, s, 0), seg=12, mat=2)
    # Parabrisas blindados (rendijas) y escotillas del conductor
    k.box("Visor", 300, 312, -90, 90, 172, 186, 3, bevel=0.0)
    for y in (-55, 55):
        k.lathe(f"Hatch{y}", [(0, 32), (6, 30)], (240, y, 192), (0, 0, 1), seg=16, mat=0)
    # Faros, cajas de herramientas, cables de remolque
    for y in (-110, 110):
        k.box(f"Light{y}", 360, 372, y - 10, y + 10, 120, 135, 2, bevel=0.003)
    k.box("Toolbox", -330, -200, -100, 100, 222, 240, 0, bevel=0.01)
    k.collision_box(-380, 380, -W / 2, W / 2, 0, 222)
    k.build()
    print(f"[btr] SM_BTR_Hull: {k.export(os.path.join(out, 'SM_BTR_Hull.fbx'))} tris")


def turret(out):
    k = Kit("SM_BTR_Turret", [M_PAINT, M_TRIM])
    k.lathe("Ring", [(0, 75), (18, 72), (40, 58), (48, 30), (50, 0)], (0, 0, 0), (0, 0, 1), seg=24, mat=0)
    k.box("Sight", -20, 10, 35, 55, 30, 52, 1, bevel=0.005)
    for y in (-50, 50):
        k.box(f"Smoke{y}", -30, -10, y - 18, y + 18, 15, 30, 1, bevel=0.004)
    k.build()
    print(f"[btr] SM_BTR_Turret: {k.export(os.path.join(out, 'SM_BTR_Turret.fbx'))} tris")


def gun(out):
    """Mantelete + cañón + ametralladora coaxial; pivote en los muñones (x 60, z 27 de la torreta). Boca en x 276."""
    k = Kit("SM_BTR_Gun", [M_PAINT, M_TRIM])
    k.box("Mantlet", -20, 20, -28, 28, -15, 15, 0, bevel=0.01)
    k.lathe("Barrel", [(0, 7), (230, 6), (232, 8), (255, 8), (256, 5)], (20, 0, 1), (1, 0, 0), seg=12, mat=1)
    k.lathe("Coax", [(0, 2.5), (60, 2.5)], (20, 18, -5), (1, 0, 0), seg=8, mat=1)
    k.build()
    print(f"[btr] SM_BTR_Gun: {k.export(os.path.join(out, 'SM_BTR_Gun.fbx'))} tris")


def main():
    argv = sys.argv[sys.argv.index("--") + 1:] if "--" in sys.argv else []
    out = os.path.abspath(argv[0] if argv else os.path.join(os.path.dirname(__file__), "..", "export", "Vehicles"))
    os.makedirs(out, exist_ok=True)
    bl_lib.reset_scene()
    hull(out)
    bl_lib.reset_scene()
    turret(out)
    bl_lib.reset_scene()
    gun(out)


if __name__ == "__main__":
    main()
