"""Granada de fragmentación M-6 (Bloque 11): cuerpo ovoide segmentado, espoleta, palanca y anilla. ~6 x 11 cm.

Uso:  blender -b --factory-startup -P gen_grenade.py -- <carpeta_salida> [preview_dir]
Pivote en el centro de masas. Sin Nanite (se mueve con física y es pequeña). Colisión: esfera aproximada (UCX).
"""
import math
import os
import sys

sys.path.append(os.path.dirname(os.path.abspath(__file__)))
import bpy
from mathutils import Matrix, Vector

import bl_lib
from bl_kit import Kit

M_BODY = ("MI_Grenade_Body", (0.16, 0.19, 0.12, 1), 0.55, 0.3)
M_STEEL = ("MI_Veh_Steel", (0.6, 0.6, 0.6, 1), 0.35, 1.0)
M_TRIM = ("MI_Veh_Trim", (0.05, 0.05, 0.05, 1), 0.6, 0.0)


def main():
    argv = sys.argv[sys.argv.index("--") + 1:] if "--" in sys.argv else []
    out = os.path.abspath(argv[0] if argv else os.path.join(os.path.dirname(__file__), "..", "export"))
    os.makedirs(out, exist_ok=True)
    bl_lib.reset_scene()
    k = Kit("SM_Grenade_M6", [M_BODY, M_STEEL, M_TRIM])
    # Cuerpo: perfil ovoide (eje Z), con una franja amarilla de "explosivo" (material de cuerpo; el color lo da el MI)
    prof = [(-5.2, 0.5), (-5.0, 1.6), (-4.4, 2.5), (-3.2, 3.0), (-1.5, 3.15), (0.5, 3.1), (2.2, 2.8), (3.4, 2.2), (4.0, 1.6), (4.2, 1.1)]
    k.lathe("Body", prof, (0, 0, 0), (0, 0, 1), seg=20, mat=0)
    for z in (-2.6, -0.6, 1.4):
        k.lathe(f"Groove{z}", [(z - 0.12, 3.17), (z + 0.12, 3.17)], (0, 0, 0), (0, 0, 1), seg=20, mat=2)
    # Espoleta y cuello
    k.lathe("Fuze", [(4.1, 1.15), (5.4, 1.1), (5.6, 0.9), (6.4, 0.85), (6.6, 0.4)], (0, 0, 0), (0, 0, 1), seg=14, mat=1)
    # Palanca (cuchara) que baja pegada al cuerpo
    k.prism("Lever", [(1.0, 6.2), (1.5, 6.2), (3.35, 2.0), (3.15, -1.5), (2.85, -1.5), (3.0, 2.0)], 'y', -0.7, 0.7, 1, bevel=0.0)
    # Anilla y pasador
    ring = k.lathe("Ring", [(-0.12, 1.5), (0.12, 1.5), (0.12, 1.25), (-0.12, 1.25)], (0, 0, 0), (1, 0, 0), seg=16, closed=True, mat=1)
    k.transform(ring, Matrix.Translation((-1.8, 0, 5.6)) @ Matrix.Rotation(math.radians(90), 4, 'Y'))
    k.lathe("Pin", [(0, 0.18), (2.6, 0.18)], (-1.9, 0, 5.6), (1, 0, 0), seg=6, mat=1)
    k.collision_box(-3.2, 3.2, -3.2, 3.2, -5.2, 6.6)
    k.build()
    tris = k.export(os.path.join(out, "SM_Grenade_M6.fbx"))
    print(f"[grenade] SM_Grenade_M6: {tris} tris")


if __name__ == "__main__":
    main()
