"""Helicóptero de transporte medio del Gobierno (Bloque 11, fase 9). Diseño propio.

Uso:  blender -b --factory-startup -P gen_heli.py -- <carpeta_salida>
Salida (pivote en el suelo bajo el mástil; morro hacia +X):
  SM_Heli_Body      fuselaje, cola, patines, puertas laterales abiertas y ametralladora de puerta (lado +Y)
  SM_Heli_Rotor     rotor principal de 4 palas (pivote en el buje, gira en Z), Ø 1500
  SM_Heli_TailRotor rotor de cola de 2 palas (pivote en el buje, gira en Y), Ø 260
Medidas: 1720 cm de largo con la cola, cabina 220 de ancho, mástil a 395 cm.
"""
import math
import os
import sys

sys.path.append(os.path.dirname(os.path.abspath(__file__)))
from mathutils import Matrix

import bl_lib
from bl_kit import Kit

M_PAINT = ("MI_Heli_Paint", (0.12, 0.13, 0.12, 1), 0.6, 0.25)
M_TRIM = ("MI_Veh_Trim", (0.05, 0.05, 0.05, 1), 0.6, 0.0)
M_GLASS = ("MI_Veh_GlassOpaque", (0.01, 0.015, 0.02, 1), 0.05, 0.0)
M_DARK = ("MI_Heli_Interior", (0.03, 0.03, 0.03, 1), 0.85, 0.0)

ZC = 170.0     # eje del fuselaje


def squash(k, obj, sy, sz, zc=ZC):
    """Aplana una pieza de revolución (sección elíptica) alrededor del eje del fuselaje."""
    m = Matrix.Translation((0, 0, zc)) @ Matrix.Diagonal((1.0, sy, sz, 1.0)) @ Matrix.Translation((0, 0, -zc))
    return k.transform(obj, m)


def body(out):
    k = Kit("SM_Heli_Body", [M_PAINT, M_TRIM, M_GLASS, M_DARK])
    prof = [(640, 8), (620, 45), (580, 80), (500, 108), (380, 118), (100, 120), (-150, 118), (-330, 100), (-470, 66), (-520, 50)]
    hull = squash(k, k.lathe("Fuselage", prof, (0, 0, ZC), (1, 0, 0), seg=28, mat=0), 0.92, 0.95)
    # Hueco de las puertas laterales (cabina abierta): se recorta y se pone un interior oscuro
    cut_l = k.box("DoorCut0", -120, 170, -140, -60, 70, 260, keep=False)
    cut_r = k.box("DoorCut1", -120, 170, 60, 140, 70, 260, keep=False)
    k.cut(hull, cut_l, cut_r)
    # Interior: mamparos delante y detrás, techo, y bancos espalda con espalda en el centro
    k.box("CabinFwd", 165, 180, -105, 105, 62, 275, 3, bevel=0.0)
    k.box("CabinAft", -135, -120, -105, 105, 62, 275, 3, bevel=0.0)
    k.box("CabinRoof", -135, 180, -105, 105, 262, 276, 3, bevel=0.0)
    k.box("SeatBack", -110, 160, -12, 12, 68, 180, 3, bevel=0.005)
    for s_ in (-1, 1):
        k.box(f"Seat{s_}", -110, 160, min(s_ * 12, s_ * 58), max(s_ * 12, s_ * 58), 105, 115, 1, bevel=0.004)
    k.box("CabinFloor", -125, 175, -104, 104, 58, 68, 1, bevel=0.0)
    # Cabina de pilotaje: cristal del morro (mitad superior del morro)
    glass = squash(k, k.lathe("Canopy", [(612, 30), (590, 72), (530, 101), (430, 116), (360, 119)], (0, 0, ZC), (1, 0, 0), seg=28, mat=2), 0.93, 0.96)
    k.cut(glass, k.box("CanopyCut", 300, 660, -200, 200, -100, ZC + 5, keep=False))
    # Cola
    k.lathe("Boom", [(-500, 52), (-800, 36), (-1060, 24), (-1080, 18)], (0, 0, ZC + 40), (1, 0, 0), seg=16, mat=0)
    k.prism("Fin", [(-1010, 205), (-1080, 205), (-1140, 430), (-1095, 440)], 'y', -6, 6, 0, bevel=0.01)
    k.box("Stab", -900, -840, -130, 130, 205, 213, 0, bevel=0.01)
    k.box("TailGearbox", -1105, -1075, 4, 20, 365, 395, 1, bevel=0.004)
    # Motores y mástil
    k.box("EngineDeck", -260, 260, -70, 70, 270, 330, 0, bevel=0.025)
    for y in (-42, 42):
        k.lathe(f"Intake{y}", [(0, 26), (20, 28), (230, 28), (260, 20)], (40, y, 335), (-1, 0, 0), seg=14, mat=0)
        k.lathe(f"Exhaust{y}", [(0, 20), (40, 18)], (-240, y * 1.2, 335), (-1, 0, 0), seg=12, mat=1)
    k.lathe("Mast", [(0, 22), (65, 14)], (0, 0, 330), (0, 0, 1), seg=12, mat=1)
    # Patines
    for s in (-1, 1):
        y = s * 135
        k.lathe(f"Skid{s}", [(-260, 6), (250, 6)], (0, y, 8), (1, 0, 0), seg=10, mat=1)
        k.lathe(f"SkidTip{s}", [(0, 6), (40, 5)], (250, y, 8), (1, 0, 0.6), seg=10, mat=1)
        for x in (-150, 180):
            k.prism(f"Strut{s}{x}", [(x - 8, 8), (x + 8, 8), (x + 8, 70), (x - 8, 70)], 'y', min(y, s * 80), max(y, s * 80), 1, bevel=0.0)
    # Ametralladora de puerta (lado derecho, +Y) con su brazo
    k.box("GunArm", 100, 116, 85, 150, 150, 160, 1, bevel=0.0)
    k.box("GunBody", 70, 150, 140, 160, 158, 176, 1, bevel=0.003)
    k.lathe("GunBarrel", [(0, 3), (90, 3), (95, 4)], (150, 150, 168), (1, 0, 0), seg=8, mat=1)
    k.box("GunBox", 80, 112, 160, 178, 140, 160, 1, bevel=0.003)
    # Luces, antenas, peldaños
    k.box("StepL", 0, 60, -128, -112, 50, 56, 1, bevel=0.0)
    k.box("StepR", 0, 60, 112, 128, 50, 56, 1, bevel=0.0)
    k.lathe("Antenna", [(0, 1.2), (90, 0.6)], (-300, 0, 300), (0, 0, 1), seg=6, mat=1)
    k.collision_box(-330, 600, -112, 112, 55, 300)
    k.collision_box(-1080, -330, -40, 40, 150, 260)
    k.collision_box(-260, 260, -140, 140, 0, 55)
    k.build()
    print(f"[heli] SM_Heli_Body: {k.export(os.path.join(out, 'SM_Heli_Body.fbx'))} tris")


def rotor(out):
    k = Kit("SM_Heli_Rotor", [M_TRIM])
    k.lathe("Hub", [(-10, 30), (10, 32), (25, 18), (30, 6)], (0, 0, 0), (0, 0, 1), seg=16, mat=0)
    for i in range(4):
        b = k.prism(f"Blade{i}", [(30, -26), (740, -24), (750, -18), (750, 22), (30, 26)], 'z', -3, 3, 0, bevel=0.002)
        # Caída de la pala (6 cm en la punta) y giro alrededor del buje
        droop = Matrix(((1, 0, 0, 0), (0, 1, 0, 0), (-0.008, 0, 1, 0), (0, 0, 0, 1)))
        k.transform(b, Matrix.Rotation(math.radians(90 * i), 4, 'Z') @ droop)
    k.build()
    print(f"[heli] SM_Heli_Rotor: {k.export(os.path.join(out, 'SM_Heli_Rotor.fbx'))} tris")


def tail_rotor(out):
    k = Kit("SM_Heli_TailRotor", [M_TRIM])
    k.lathe("Hub", [(-8, 10), (8, 10)], (0, 0, 0), (0, 1, 0), seg=12, mat=0)
    for i in range(2):
        b = k.prism(f"Blade{i}", [(8, -12), (128, -10), (130, 10), (8, 12)], 'y', -2, 2, 0, bevel=0.0)
        k.transform(b, Matrix.Rotation(math.radians(180 * i), 4, 'Y'))
    k.build()
    print(f"[heli] SM_Heli_TailRotor: {k.export(os.path.join(out, 'SM_Heli_TailRotor.fbx'))} tris")


def main():
    argv = sys.argv[sys.argv.index("--") + 1:] if "--" in sys.argv else []
    out = os.path.abspath(argv[0] if argv else os.path.join(os.path.dirname(__file__), "..", "export", "Vehicles"))
    os.makedirs(out, exist_ok=True)
    bl_lib.reset_scene()
    body(out)
    bl_lib.reset_scene()
    rotor(out)
    bl_lib.reset_scene()
    tail_rotor(out)


if __name__ == "__main__":
    main()
