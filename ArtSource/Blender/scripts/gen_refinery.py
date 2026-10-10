"""Atrezo industrial de la refinería de Kessra (misión 2 "Manifiesto"). Diseño propio, unidades de Unreal (cm).

Uso:  blender -b --factory-startup -P gen_refinery.py -- <carpeta_salida>
Salida (pivote en el suelo, centrado salvo que se diga):
  SM_Ref_Tank          depósito de techo cónico Ø1200 x 1100, escalera helicoidal exterior y barandilla arriba
  SM_Ref_PipeRack      tramo de 1000 (a lo largo de X) del rack de tuberías: 2 pórticos, 2 niveles, 7 tubos
  SM_Ref_PipeLow       tramo de 1000 de tubería baja sobre durmientes (la zanja de tuberías)
  SM_Ref_FlareStack    antorcha: fuste Ø140 x 4200 con plataforma y tirantes (la llama la pone ABLSmokeEmitter)
  SM_Ref_Column        torre de destilación Ø320 x 3200 con plataformas cada 800 y escalera de gato
  SM_Ref_FloodTower    torre de focos (poste 1150 + plataforma + 2 proyectores); los focos los pone ABLAlarmLight
  SM_Ref_Fence         panel de valla de 400 (postes, malla en tres hilos y concertina arriba)
  SM_Ref_Crate_Drones  caja de madera abierta con dos municiones merodeadoras dentro (y su tapa apoyada)
  SM_Ref_Munition      munición merodeadora suelta (tubo con alas en X), para mesas y estanterías
"""
import math
import os
import sys

sys.path.append(os.path.dirname(os.path.abspath(__file__)))
from mathutils import Matrix

import bl_lib
from bl_kit import Kit

PAINT = ("MI_Ref_TankPaint", (0.55, 0.55, 0.52, 1), 0.6, 0.2)
RUST = ("MI_Env_RustyMetal", (0.3, 0.17, 0.1, 1), 0.8, 0.5)
STEEL = ("MI_Env_MetalPlate", (0.35, 0.35, 0.36, 1), 0.5, 0.8)
TRIM = ("MI_Veh_Trim", (0.05, 0.05, 0.05, 1), 0.6, 0.0)
YELLOW = ("MI_Ref_PipeYellow", (0.62, 0.45, 0.05, 1), 0.55, 0.2)
GREY = ("MI_Ref_PipeGrey", (0.25, 0.26, 0.27, 1), 0.55, 0.4)
WOOD = ("MI_Env_WoodPlanks", (0.4, 0.3, 0.2, 1), 0.8, 0.0)
OLIVE = ("MI_Ref_MunitionOlive", (0.16, 0.18, 0.12, 1), 0.55, 0.2)
LABEL = ("MI_Ref_Label", (0.75, 0.72, 0.6, 1), 0.7, 0.0)


def ring_rail(k, name, z, r, posts, mat, seg=32):
    """Barandilla circular: pasamanos + listón intermedio + montantes."""
    k.lathe(f"{name}_Top", [(0, r - 2), (0, r + 2), (4, r + 2), (4, r - 2)], (0, 0, z + 100), (0, 0, 1), seg=seg, closed=True, mat=mat)
    k.lathe(f"{name}_Mid", [(0, r - 1.5), (0, r + 1.5), (3, r + 1.5), (3, r - 1.5)], (0, 0, z + 50), (0, 0, 1), seg=seg, closed=True, mat=mat)
    for i in range(posts):
        a = 2 * math.pi * i / posts
        x, y = r * math.cos(a), r * math.sin(a)
        k.box(f"{name}_Post{i}", x - 2, x + 2, y - 2, y + 2, z, z + 104, mat, bevel=0.0)


def tank(out):
    k = Kit("SM_Ref_Tank", [PAINT, RUST, STEEL, TRIM])
    R, H = 600.0, 1100.0
    k.lathe("Shell", [(0, R), (H, R)], (0, 0, 0), (0, 0, 1), seg=48, mat=0)
    k.lathe("Roof", [(H, R + 6), (H + 6, R + 6), (H + 110, 60), (H + 120, 0)], (0, 0, 0), (0, 0, 1), seg=48, mat=0)
    # Anillos de refuerzo (chapas) y zócalo de hormigón oxidado
    for z in (270, 550, 830):
        k.lathe(f"Band{z}", [(z, R), (z, R + 4), (z + 8, R + 4), (z + 8, R)], (0, 0, 0), (0, 0, 1), seg=48, closed=True, mat=1)
    k.lathe("Base", [(-20, R + 30), (20, R + 30)], (0, 0, 0), (0, 0, 1), seg=48, mat=1)
    # Escalera helicoidal exterior: peldaños + zanca, de 0 a la plataforma de arriba (un cuarto de vuelta)
    steps = 52
    for i in range(steps):
        a = math.radians(-20 + 100 * i / steps)
        z = H * (i + 1) / steps
        ca, sa = math.cos(a), math.sin(a)
        p = k.box(f"Step{i}", R + 4, R + 84, -14, 14, z - 4, z, 2, bevel=0.0)
        k.transform(p, Matrix.Rotation(a, 4, 'Z'))
        q = k.box(f"Rail{i}", R + 82, R + 86, -2, 2, z, z + 100, 3, bevel=0.0)
        k.transform(q, Matrix.Rotation(a, 4, 'Z'))
    ring_rail(k, "TopRail", H + 25, R - 40, 24, 3)
    # Bocas de hombre, tubería de llenado y venteos
    k.lathe("Manway", [(0, 40), (12, 40), (12, 30)], (R, 0, 120), (1, 0, 0), seg=20, mat=2)
    k.lathe("FillPipe", [(0, 14), (260, 14)], (R, 300, 60), (1, 0, 0), seg=14, mat=2)
    for i, (x, y) in enumerate(((150, 100), (-200, -220))):
        k.lathe(f"Vent{i}", [(0, 16), (70, 16), (74, 24), (84, 24)], (x, y, H + 80), (0, 0, 1), seg=14, mat=2)
    for a in range(0, 360, 45):
        k.collision_box(-R, R, -R * 0.42, R * 0.42, 0, H + 60, rot_z_deg=a)
    k.build()
    print(f"[ref] SM_Ref_Tank: {k.export(os.path.join(out, 'SM_Ref_Tank.fbx'))} tris")


def pipe_rack(out):
    k = Kit("SM_Ref_PipeRack", [STEEL, YELLOW, GREY, RUST])
    L, W = 1000.0, 320.0
    for x in (60, L - 60):
        for y in (-W, W):
            k.box(f"Col{x}{y}", x - 15, x + 15, y - 15, y + 15, 0, 640, 0, bevel=0.004)
        for z in (420, 620):
            k.box(f"Beam{x}{z}", x - 12, x + 12, -W - 15, W + 15, z, z + 24, 0, bevel=0.004)
        k.box(f"Brace{x}", x - 6, x + 6, -W, W, 200, 212, 0, bevel=0.0)
    # Largueros entre pórticos
    for y in (-W, W):
        k.box(f"Stringer{y}", 0, L, y - 8, y + 8, 610, 620, 0, bevel=0.0)
    # Tubos (y, z, radio, material)
    for i, (y, z, r, m) in enumerate(((-230, 444, 22, 1), (-150, 444, 16, 2), (-60, 444, 30, 2), (70, 444, 14, 1),
                                      (180, 444, 26, 3), (-120, 644, 20, 2), (110, 644, 34, 1))):
        k.lathe(f"Pipe{i}", [(0, r), (L, r)], (0, y, z + r), (1, 0, 0), seg=16, mat=m)
        for x in (60, L - 60):
            k.lathe(f"Flange{i}{x}", [(0, r + 5), (10, r + 5)], (x + 40, y, z + r), (1, 0, 0), seg=16, mat=0)
    for x in (60, L - 60):
        for y in (-W, W):
            k.collision_box(x - 16, x + 16, y - 16, y + 16, 0, 640)
    k.collision_box(0, L, -W - 15, W + 15, 420, 700)
    k.build()
    print(f"[ref] SM_Ref_PipeRack: {k.export(os.path.join(out, 'SM_Ref_PipeRack.fbx'))} tris")


def pipe_low(out):
    k = Kit("SM_Ref_PipeLow", [GREY, YELLOW, RUST])
    L = 1000.0
    for x in (100, 500, 900):
        k.box(f"Sleeper{x}", x - 20, x + 20, -110, 110, 0, 30, 2, bevel=0.004)
    for i, (y, r, m) in enumerate(((-60, 24, 0), (10, 18, 1), (70, 28, 0))):
        k.lathe(f"Pipe{i}", [(0, r), (L, r)], (0, y, 30 + r), (1, 0, 0), seg=16, mat=m)
    k.collision_box(0, L, -110, 110, 0, 95)
    k.build()
    print(f"[ref] SM_Ref_PipeLow: {k.export(os.path.join(out, 'SM_Ref_PipeLow.fbx'))} tris")


def flare_stack(out):
    k = Kit("SM_Ref_FlareStack", [GREY, STEEL, RUST])
    H = 4200.0
    k.lathe("Stack", [(0, 85), (300, 85), (330, 70), (H, 70), (H + 30, 80), (H + 60, 80)], (0, 0, 0), (0, 0, 1), seg=24, mat=0)
    k.lathe("Tip", [(H + 60, 82), (H + 140, 82)], (0, 0, 0), (0, 0, 1), seg=24, mat=2)
    k.lathe("Platform", [(H - 260, 190), (H - 245, 190)], (0, 0, 0), (0, 0, 1), seg=24, mat=1)
    ring_rail(k, "PlatRail", H - 245, 186, 12, 1, seg=24)
    k.box("Ladder", 70, 78, -24, 24, 0, H - 250, 1, bevel=0.0)
    for i in range(3):
        a = math.radians(120 * i + 30)
        x, y = 1800 * math.cos(a), 1800 * math.sin(a)
        top = H * 0.66
        k.lathe(f"Guy{i}", [(0, 3), (math.sqrt(x * x + y * y + top * top), 3)], (x, y, 0), (-x, -y, top), seg=6, mat=1)
    k.collision_box(-90, 90, -90, 90, 0, H + 140)
    k.build()
    print(f"[ref] SM_Ref_FlareStack: {k.export(os.path.join(out, 'SM_Ref_FlareStack.fbx'))} tris")


def column(out):
    k = Kit("SM_Ref_Column", [PAINT, STEEL, RUST, TRIM])
    R, H = 160.0, 3200.0
    k.lathe("Skirt", [(0, R + 20), (300, R + 10)], (0, 0, 0), (0, 0, 1), seg=32, mat=2)
    k.lathe("Shell", [(300, R), (H, R), (H + 90, R * 0.6), (H + 110, 30), (H + 160, 30)], (0, 0, 0), (0, 0, 1), seg=32, mat=0)
    for z in (800, 1600, 2400, 3150):
        k.lathe(f"Plat{z}", [(z, R + 4), (z, R + 140), (z + 10, R + 140), (z + 10, R + 4)], (0, 0, 0), (0, 0, 1), seg=32, closed=True, mat=1)
        ring_rail(k, f"Rail{z}", z + 10, R + 136, 16, 3)
    k.box("Ladder", R + 10, R + 18, -24, 24, 0, H, 1, bevel=0.0)
    for i, z in enumerate((600, 1400, 2200, 2900)):
        k.lathe(f"Nozzle{i}", [(0, 22), (120, 22), (126, 32), (134, 32)], (0, 0, z), (1 if i % 2 else -1, 0.3, 0), seg=14, mat=1)
    for a in (0, 45):
        k.collision_box(-R - 20, R + 20, -R * 0.42 - 8, R * 0.42 + 8, 0, H + 160, rot_z_deg=a)
    k.build()
    print(f"[ref] SM_Ref_Column: {k.export(os.path.join(out, 'SM_Ref_Column.fbx'))} tris")


def flood_tower(out):
    k = Kit("SM_Ref_FloodTower", [GREY, STEEL, TRIM])
    k.lathe("Pole", [(0, 22), (1100, 14)], (0, 0, 0), (0, 0, 1), seg=12, mat=0)
    k.box("Plinth", -45, 45, -45, 45, 0, 40, 1, bevel=0.006)
    k.box("Platform", -60, 60, -90, 90, 1100, 1112, 1, bevel=0.002)
    k.box("Rail", -62, 62, -92, -88, 1112, 1190, 1, bevel=0.0)
    for y in (-50, 50):
        # Proyector rectangular mirando a +X, algo hacia abajo
        p = k.box(f"Lamp{y}", -20, 22, y - 30, y + 30, 1120, 1170, 2, bevel=0.006)
        k.transform(p, Matrix.Translation((40, 0, 1145)) @ Matrix.Rotation(math.radians(30), 4, 'Y') @ Matrix.Translation((-40, 0, -1145)))
    k.box("Ladder", -30, -22, 22, 60, 40, 1100, 1, bevel=0.0)
    k.collision_box(-45, 45, -45, 45, 0, 1100)
    k.build()
    print(f"[ref] SM_Ref_FloodTower: {k.export(os.path.join(out, 'SM_Ref_FloodTower.fbx'))} tris")


def fence(out):
    k = Kit("SM_Ref_Fence", [STEEL, RUST])
    L = 400.0
    for x in (0, L):
        k.lathe(f"Post{x}", [(0, 5), (250, 5)], (x, 0, 0), (0, 0, 1), seg=10, mat=0)
        k.box(f"Arm{x}", x - 3, x + 3, -3, 40, 245, 250, 0, bevel=0.0)
    # Malla: rejilla de alambres (Nanite: el número de triángulos no importa; se ve a través)
    for z in range(15, 240, 18):
        k.box(f"WireH{z}", 0, L, -0.6, 0.6, z, z + 1.2, 0, bevel=0.0)
    for x in range(10, int(L), 18):
        k.box(f"WireV{x}", x, x + 1.2, -0.6, 0.6, 10, 240, 0, bevel=0.0)
    # Concertina: espiral aproximada con anillos inclinados
    for i in range(14):
        x = 15 + i * (L - 30) / 13
        ring = k.lathe(f"Coil{i}", [(0, 20), (1.5, 20), (1.5, 21.5), (0, 21.5)], (0, 0, 0), (1, 0, 0), seg=14, closed=True, mat=1)
        k.transform(ring, Matrix.Translation((x, 22, 268)) @ Matrix.Rotation(math.radians(18), 4, 'Z'))
    k.collision_box(0, L, -4, 4, 0, 250)
    k.build()
    print(f"[ref] SM_Ref_Fence: {k.export(os.path.join(out, 'SM_Ref_Fence.fbx'))} tris")


def munition_parts(k, ox, oy, oz, yaw=0.0):
    """Munición merodeadora: tubo Ø18 x 120, ojiva con cámara, alas en X plegables, hélice de empuje."""
    parts = [
        k.lathe("Body", [(0, 4), (8, 9), (100, 9), (112, 6), (120, 3)], (0, 0, 0), (1, 0, 0), seg=16, mat=0),
        k.lathe("Seeker", [(0, 6), (-10, 6), (-14, 3)], (0, 0, 0), (1, 0, 0), seg=12, mat=2),
        k.lathe("Hub", [(120, 3), (126, 3)], (0, 0, 0), (1, 0, 0), seg=8, mat=2),
        k.box("Label", 40, 70, -9.4, -9.0, -4, 4, 3, bevel=0.0),
    ]
    for a in (45, 135, 225, 315):
        w = k.box(f"Wing{a}", 30, 80, -1, 1, 8, 24, 0, bevel=0.0)   # alas plegadas (se abren al lanzarla)
        k.transform(w, Matrix.Rotation(math.radians(a), 4, 'X'))
        parts.append(w)
    for p in parts:
        k.transform(p, Matrix.Translation((ox, oy, oz)) @ Matrix.Rotation(math.radians(yaw), 4, 'Z'))


def crate_drones(out):
    k = Kit("SM_Ref_Crate_Drones", [OLIVE, WOOD, TRIM, LABEL])
    L, W, H, T = 180.0, 90.0, 70.0, 3.0
    # Caja abierta (paredes finas de tablas), con divisorias de espuma oscura
    k.box("Floor", -L / 2, L / 2, -W / 2, W / 2, 0, T + 8, 1, bevel=0.003)
    for s in (-1, 1):
        k.box(f"Side{s}", -L / 2, L / 2, s * W / 2 - (T if s > 0 else 0), s * W / 2 + (T if s < 0 else 0), 0, H, 1, bevel=0.003)
        k.box(f"End{s}", s * L / 2 - (T if s > 0 else 0), s * L / 2 + (T if s < 0 else 0), -W / 2, W / 2, 0, H, 1, bevel=0.003)
    k.box("Foam", -L / 2 + T, L / 2 - T, -W / 2 + T, W / 2 - T, 8, 30, 2, bevel=0.0)
    for y in (-22, 22):
        munition_parts(k, -60, y, 38)
    # Tapa apoyada contra un lateral, con la plantilla pintada
    # Tapa en el suelo junto a la caja (girada), con la plantilla pintada a la vista
    lid = k.box("Lid", -L / 2, L / 2, -W / 2, W / 2, 0, T, 1, bevel=0.003)
    stencil = k.box("Stencil", -55, 55, -25, 25, T, T + 0.4, 3, bevel=0.0)
    for p in (lid, stencil):
        k.transform(p, Matrix.Translation((10, W + 25, 0)) @ Matrix.Rotation(math.radians(8), 4, 'Z'))
    k.collision_box(-L / 2, L / 2, -W / 2, W / 2, 0, H)
    k.build()
    print(f"[ref] SM_Ref_Crate_Drones: {k.export(os.path.join(out, 'SM_Ref_Crate_Drones.fbx'))} tris")


def munition(out):
    k = Kit("SM_Ref_Munition", [OLIVE, WOOD, TRIM, LABEL])
    munition_parts(k, -60, 0, 10)
    k.collision_box(-75, 75, -40, 40, 0, 55)
    k.build()
    print(f"[ref] SM_Ref_Munition: {k.export(os.path.join(out, 'SM_Ref_Munition.fbx'))} tris")


def main():
    argv = sys.argv[sys.argv.index("--") + 1:] if "--" in sys.argv else []
    out = os.path.abspath(argv[0] if argv else os.path.join(os.path.dirname(__file__), "..", "export", "Refinery"))
    os.makedirs(out, exist_ok=True)
    for fn in (tank, pipe_rack, pipe_low, flare_stack, column, flood_tower, fence, crate_drones, munition):
        bl_lib.reset_scene()
        fn(out)


if __name__ == "__main__":
    main()
