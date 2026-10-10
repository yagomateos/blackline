"""Assets de la misión 5 "Línea negra" (terminal de contenedores y capitanía). Diseño propio, unidades de Unreal (cm).

Uso:  blender -b --factory-startup -P gen_m05.py -- <carpeta_salida>
Salida (carpeta Terminal):
  SM_ServerRack    armario de servidores 60 x 100 x 210 con equipos, cables y luces (puerta de rejilla abierta)
  SM_Thermite      carga de termita: dos botes con temporizador y cinta (sobre un servidor)
  SM_GantryCrane   grúa pórtico de muelle (STS) de 40 m: patas, viga, pluma sobre el agua, cabina
  SM_Helipad       helipuerto de azotea: losa con borde, "H" y círculo pintados, luces de borde
  SM_OfficeDesk    mesa de oficina con dos monitores, teclado y silla
"""
import math
import os
import sys

sys.path.append(os.path.dirname(os.path.abspath(__file__)))
from mathutils import Matrix

import bl_lib
from bl_kit import Kit

RACK = ("MI_Term_RackBlack", (0.02, 0.022, 0.025, 1), 0.5, 0.6)
LED = ("MI_Term_LED", (0.1, 0.9, 0.3, 1), 0.3, 0.0)
STEEL = ("MI_Env_MetalPlate", (0.3, 0.3, 0.3, 1), 0.5, 0.8)
CANISTER = ("MI_Term_Thermite", (0.35, 0.33, 0.25, 1), 0.6, 0.3)
TAPE = ("MI_Term_Tape", (0.6, 0.45, 0.05, 1), 0.7, 0.0)
CRANE = ("MI_Term_CraneRed", (0.38, 0.07, 0.04, 1), 0.65, 0.3)
WHITE = ("MI_Boat_HullWhite", (0.6, 0.6, 0.57, 1), 0.6, 0.0)
GLASS = ("MI_Veh_GlassOpaque", (0.01, 0.015, 0.02, 1), 0.05, 0.0)
PAD = ("MI_Term_PadGrey", (0.16, 0.16, 0.16, 1), 0.8, 0.0)
PAINT_Y = ("MI_Term_PaintYellow", (0.6, 0.48, 0.05, 1), 0.7, 0.0)
WOOD = ("MI_Env_WoodPlanks", (0.3, 0.22, 0.15, 1), 0.85, 0.0)
PLASTIC = ("MI_Obj_Plastic", (0.03, 0.03, 0.03, 1), 0.5, 0.0)
SCREEN = ("MI_Term_Screen", (0.05, 0.1, 0.15, 1), 0.2, 0.0)


def save(k, out):
    k.build()
    print(f"[m05] {k.name}: {k.export(os.path.join(out, k.name + '.fbx'))} tris")


def beam(k, name, a, b, w, mat):
    ax, ay, az = a
    bx, by, bz = b
    L = math.dist(a, b)
    obj = k.box(name, 0, L, -w / 2, w / 2, -w / 2, w / 2, mat, bevel=0.0)
    d = (bx - ax, by - ay, bz - az)
    k.transform(obj, Matrix.Translation(a) @ Matrix.Rotation(math.atan2(d[1], d[0]), 4, 'Z') @ Matrix.Rotation(-math.atan2(d[2], math.hypot(d[0], d[1])), 4, 'Y'))
    return obj


def server_rack(out):
    k = Kit("SM_ServerRack", [RACK, LED, STEEL, PLASTIC])
    W, D, H = 60.0, 100.0, 210.0
    for s in (-1, 1):
        k.box(f"Side{s}", s * W / 2 - (3 if s > 0 else 0), s * W / 2 + (3 if s < 0 else 0), -D / 2, D / 2, 0, H, 0, bevel=0.003)
    k.box("Top", -W / 2, W / 2, -D / 2, D / 2, H - 4, H, 0, bevel=0.003)
    k.box("Back", -W / 2, W / 2, D / 2 - 3, D / 2, 0, H, 0, bevel=0.0)
    for i in range(14):
        z = 15 + i * 13.5
        k.box(f"Unit{i}", -W / 2 + 4, W / 2 - 4, -D / 2 + 5, D / 2 - 10, z, z + 9, 3, bevel=0.002)
        for j in range(3):
            k.box(f"Led{i}_{j}", -20 + j * 6, -18 + j * 6, -D / 2 + 4.6, -D / 2 + 5.2, z + 5, z + 7, 1, bevel=0.0)
    door = k.box("Door", -W / 2, W / 2, -2, 0, 0, H, 2, bevel=0.002)
    k.transform(door, Matrix.Translation((W / 2, -D / 2, 0)) @ Matrix.Rotation(math.radians(-100), 4, 'Z') @ Matrix.Translation((-W / 2, 0, 0)))
    k.collision_box(-W / 2, W / 2, -D / 2, D / 2, 0, H)
    save(k, out)


def thermite(out):
    k = Kit("SM_Thermite", [CANISTER, TAPE, PLASTIC, LED])
    for x in (-9, 9):
        k.lathe(f"Can{x}", [(0, 7), (28, 7), (30, 5)], (x, 0, 0), (0, 0, 1), seg=14, mat=0)
    k.box("Tape", -17, 17, -7.5, 7.5, 10, 16, 1, bevel=0.0)
    k.box("Timer", -6, 6, -9, -6, 18, 26, 2, bevel=0.002)
    k.box("Display", -4, 4, -9.4, -9, 20, 24, 3, bevel=0.0)
    k.collision_box(-17, 17, -8, 8, 0, 30)
    save(k, out)


def gantry_crane(out):
    k = Kit("SM_GantryCrane", [CRANE, STEEL, GLASS, WHITE])
    for x in (-900, 900):
        for y in (-800, 800):
            k.box(f"Leg{x}{y}", x - 60, x + 60, y - 60, y + 60, 0, 3500, 0, bevel=0.003)
        k.box(f"Sill{x}", x - 70, x + 70, -900, 900, 0, 150, 0, bevel=0.003)
        beam(k, f"BraceA{x}", (x, -800, 400), (x, 800, 3000), 40, 0)
    for y in (-800, 800):
        k.box(f"Girder{y}", -1000, 1000, y - 70, y + 70, 3400, 3600, 0, bevel=0.003)
    # Pluma sobre el agua (+X) y contrapluma (-X)
    k.box("Boom", -2500, 5500, -250, 250, 3700, 3950, 0, bevel=0.002)
    k.box("Apex", -150, 150, -400, 400, 3600, 5200, 0, bevel=0.003)
    for s in (-1, 1):
        beam(k, f"Stay{s}", (0, s * 300, 5200), (5000, s * 200, 3950), 25, 1)
        beam(k, f"BackStay{s}", (0, s * 300, 5200), (-2300, s * 200, 3950), 25, 1)
    k.box("Trolley", 2000, 2500, -230, 230, 3500, 3700, 1, bevel=0.003)
    k.box("Cab", 2050, 2450, 150, 450, 3180, 3480, 3, bevel=0.006)
    k.box("CabGlass", 2440, 2455, 170, 430, 3220, 3440, 2, bevel=0.0)
    k.box("MachineHouse", -2400, -1200, -300, 300, 3950, 4350, 3, bevel=0.006)
    for x in (-900, 900):
        for y in (-800, 800):
            k.collision_box(x - 60, x + 60, y - 60, y + 60, 0, 3500)
    save(k, out)


def helipad(out):
    k = Kit("SM_Helipad", [PAD, PAINT_Y, WHITE, LED])
    k.lathe("Slab", [(0, 900), (20, 900)], (0, 0, 0), (0, 0, 1), seg=40, mat=0)
    k.lathe("Ring", [(20, 700), (20.5, 700), (20.5, 760), (20, 760)], (0, 0, 0), (0, 0, 1), seg=40, closed=True, mat=1)
    for s in (-1, 1):
        k.box(f"HLeg{s}", s * 180 - 40, s * 180 + 40, -260, 260, 20, 20.6, 2, bevel=0.0)
    k.box("HBar", -140, 140, -40, 40, 20, 20.6, 2, bevel=0.0)
    for i in range(12):
        a = 2 * math.pi * i / 12
        k.lathe(f"Light{i}", [(0, 8), (10, 6)], (870 * math.cos(a), 870 * math.sin(a), 20), (0, 0, 1), seg=8, mat=3)
    k.collision_box(-900, 900, -900, 900, 0, 20)
    save(k, out)


def office_desk(out):
    k = Kit("SM_OfficeDesk", [WOOD, PLASTIC, SCREEN, STEEL])
    k.box("Top", -80, 80, -40, 40, 72, 76, 0, bevel=0.003)
    for x in (-75, 75):
        k.box(f"Leg{x}", x - 3, x + 3, -36, 36, 0, 72, 3, bevel=0.0)
    for i, x in enumerate((-30, 30)):
        mon = k.box(f"Monitor{i}", x - 26, x + 26, -2, 2, 90, 122, 1, bevel=0.003)
        k.transform(mon, Matrix.Translation((x, 15, 0)) @ Matrix.Rotation(math.radians(-12 if i else 12), 4, 'Z') @ Matrix.Translation((-x, 0, 0)))
        k.box(f"Screen{i}", x - 24, x + 24, 12.8, 13.2, 92, 120, 2, bevel=0.0)
        k.box(f"Stand{i}", x - 4, x + 4, 12, 18, 76, 92, 1, bevel=0.0)
    k.box("Keyboard", -22, 22, -20, -6, 76, 78, 1, bevel=0.001)
    k.box("ChairSeat", -25, 25, -95, -45, 45, 52, 1, bevel=0.006)
    k.box("ChairBack", -25, 25, -100, -95, 52, 105, 1, bevel=0.006)
    k.lathe("ChairPost", [(0, 3), (45, 3)], (0, -70, 0), (0, 0, 1), seg=8, mat=3)
    k.collision_box(-80, 80, -40, 40, 0, 76)
    save(k, out)


def main():
    argv = sys.argv[sys.argv.index("--") + 1:] if "--" in sys.argv else []
    out = os.path.abspath(argv[0] if argv else os.path.join(os.path.dirname(__file__), "..", "export", "Terminal"))
    os.makedirs(out, exist_ok=True)
    for fn in (server_rack, thermite, gantry_crane, helipad, office_desk):
        bl_lib.reset_scene()
        fn(out)


if __name__ == "__main__":
    main()
