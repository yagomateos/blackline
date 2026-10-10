"""Vehículos de la misión 2 "Manifiesto". Diseño propio, unidades de Unreal (cm).

Uso:  blender -b --factory-startup -P gen_m02_vehicles.py -- <carpeta_salida>
Salida:
  SM_Drone_Body   dron de reconocimiento de Corvane (cuadricóptero de 110 cm): cuerpo de fibra, 4 brazos, motores,
                  patas, góndola de cámara con el foco debajo del morro (+X). Pivote en el centro del cuerpo.
  SM_Drone_Prop   hélice bipala Ø46 (pivote en el eje; la gira ABLDrone)
  SM_Boat_RHIB    lancha neumática semirrígida de 720 cm: casco en V, flotadores, consola, asientos de montar,
                  fueraborda y arco de antenas. Pivote en la línea de flotación, proa hacia +X.
"""
import math
import os
import sys

sys.path.append(os.path.dirname(os.path.abspath(__file__)))
from mathutils import Matrix

import bl_lib
from bl_kit import Kit

COMPOSITE = ("MI_Drone_Body", (0.06, 0.065, 0.07, 1), 0.45, 0.1)
TRIM = ("MI_Veh_Trim", (0.05, 0.05, 0.05, 1), 0.6, 0.0)
GLASS = ("MI_Veh_GlassOpaque", (0.01, 0.015, 0.02, 1), 0.05, 0.0)
TUBE = ("MI_Boat_Tube", (0.09, 0.095, 0.1, 1), 0.75, 0.0)
HULL = ("MI_Boat_Hull", (0.2, 0.21, 0.2, 1), 0.5, 0.1)
RUBBER = ("MI_Veh_Rubber", (0.02, 0.02, 0.02, 1), 0.9, 0.0)


def drone(out):
    k = Kit("SM_Drone_Body", [COMPOSITE, TRIM, GLASS])
    k.rounded_box("Body", 0, 0, 0, 44, 30, 14, 5, mat=0)
    k.rounded_box("Battery", -6, 0, 9, 26, 18, 6, 2, mat=1)
    for i, (x, y) in enumerate(((38, 38), (38, -38), (-38, 38), (-38, -38))):
        a = math.atan2(y, x)
        arm = k.box(f"Arm{i}", 0, math.hypot(x, y), -2.5, 2.5, -2, 2, 0, bevel=0.003)
        k.transform(arm, Matrix.Rotation(a, 4, 'Z'))
        k.lathe(f"Motor{i}", [(0, 5), (8, 5), (10, 3)], (x, y, 2), (0, 0, 1), seg=12, mat=1)
        k.lathe(f"Leg{i}", [(0, 1.2), (20, 1.2)], (x * 0.45, y * 0.45, -6), (0, 0, -1), seg=6, mat=1)
    for y in (-14, 14):
        k.lathe(f"Skid{y}", [(0, 1.2), (36, 1.2)], (-18, y, -26), (1, 0, 0), seg=6, mat=1)
    # Góndola de cámara/foco bajo el morro y antenas
    k.lathe("Gimbal", [(0, 7), (6, 8), (12, 6), (14, 0)], (14, 0, -6), (0, 0, -1), seg=16, mat=1)
    k.lathe("Lens", [(0, 3.5), (2, 3.5)], (20, 0, -13), (1, 0, 0), seg=12, mat=2)
    for y in (-9, 9):
        k.lathe(f"Antenna{y}", [(0, 0.6), (14, 0.4)], (-20, y, 6), (0, 0, 1), seg=6, mat=1)
    k.collision_box(-50, 50, -50, 50, -26, 16)
    k.build()
    print(f"[m02] SM_Drone_Body: {k.export(os.path.join(out, 'SM_Drone_Body.fbx'))} tris")


def drone_prop(out):
    k = Kit("SM_Drone_Prop", [COMPOSITE, TRIM])
    k.lathe("Hub", [(0, 2.5), (2.5, 2.5)], (0, 0, 0), (0, 0, 1), seg=10, mat=1)
    for s in (-1, 1):
        b = k.prism(f"Blade{s}", [(2, -2.5), (21, -1.8), (23, 0), (21, 1.8), (2, 2.5)], 'z', 0.6, 1.4, 0, bevel=0.0)
        k.transform(b, Matrix.Rotation(math.radians(90 + 90 * s), 4, 'Z') @ Matrix(((1, 0, 0, 0), (0, 1, 0.12, 0), (0, 0, 1, 0), (0, 0, 0, 1))))
    k.build()
    print(f"[m02] SM_Drone_Prop: {k.export(os.path.join(out, 'SM_Drone_Prop.fbx'))} tris")


def boat(out):
    k = Kit("SM_Boat_RHIB", [HULL, TUBE, TRIM, RUBBER])
    L = 720.0
    # Casco en V (perfil lateral extruido a lo ancho) con quilla y espejo de popa
    k.prism("Hull", [(-360, -40), (240, -40), (360, 10), (330, 40), (-360, 40)], 'y', -110, 110, 0, bevel=0.02)
    k.prism("Keel", [(-360, -55), (250, -55), (330, 0), (240, -40), (-360, -40)], 'y', -12, 12, 0, bevel=0.01)
    # Flotadores: tubo de Ø64 por cada costado que se cierra en la proa
    for s in (-1, 1):
        k.lathe(f"Tube{s}", [(0, 0), (6, 26), (20, 32), (560, 32), (600, 30)], (-380, s * 118, 36), (1, 0, 0), seg=20, mat=1)
        k.lathe(f"Rub{s}", [(0, 4), (520, 4)], (-340, s * 150, 26), (1, 0, 0), seg=8, mat=3)
    bow = k.lathe("BowTube", [(0, 30), (110, 28)], (220, -118, 36), (0.45, 1, 0.08), seg=20, mat=1)
    bow2 = k.lathe("BowTube2", [(0, 30), (110, 28)], (220, 118, 36), (0.45, -1, 0.08), seg=20, mat=1)
    k.lathe("BowCap", [(0, 30), (10, 22), (14, 0)], (300, 0, 46), (1, 0, 0.1), seg=20, mat=1)
    # Cubierta, consola, asientos de montar y arco
    k.box("Deck", -350, 260, -95, 95, 38, 44, 0, bevel=0.004)
    k.box("Console", 30, 110, -45, 45, 44, 150, 0, bevel=0.01)
    k.box("Screen", 100, 108, -38, 38, 125, 165, 2, bevel=0.003)
    for i, x in enumerate((-60, -170)):
        k.rounded_box(f"Jockey{i}", x, 0, 85, 80, 45, 80, 8, mat=3)
    k.box("ArchL", -250, -242, -80, -72, 44, 210, 2, bevel=0.0)
    k.box("ArchR", -250, -242, 72, 80, 44, 210, 2, bevel=0.0)
    k.box("ArchTop", -252, -240, -80, 80, 204, 212, 2, bevel=0.0)
    for y in (-50, 50):
        k.lathe(f"Antenna{y}", [(0, 1), (130, 0.6)], (-246, y, 212), (0, 0, 1), seg=6, mat=2)
    # Fueraborda en el espejo de popa
    k.rounded_box("Cowl", -395, 0, 95, 50, 42, 60, 10, mat=2)
    k.box("Shaft", -400, -385, -8, 8, -50, 70, 2, bevel=0.003)
    k.box("Skeg", -405, -380, -4, 4, -70, -50, 2, bevel=0.0)
    k.collision_box(-400, 380, -150, 150, -40, 70)
    k.build()
    print(f"[m02] SM_Boat_RHIB: {k.export(os.path.join(out, 'SM_Boat_RHIB.fbx'))} tris")


def main():
    argv = sys.argv[sys.argv.index("--") + 1:] if "--" in sys.argv else []
    out = os.path.abspath(argv[0] if argv else os.path.join(os.path.dirname(__file__), "..", "export", "Vehicles"))
    os.makedirs(out, exist_ok=True)
    for fn in (drone, drone_prop, boat):
        bl_lib.reset_scene()
        fn(out)


if __name__ == "__main__":
    main()
