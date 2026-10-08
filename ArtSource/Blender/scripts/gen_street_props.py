"""Atrezo de calle (Bloque 8): lo que llena una calle de ciudad en guerra. Medidas reales, Nanite, colisión UCX.

Uso:  blender -b --factory-startup -P gen_street_props.py -- <carpeta_salida Props> [preview_dir] [--only=...]
Mallas (pivote en la base, centradas; coordenadas de Unreal, ver bl_kit.py):
  SM_Prop_Dumpster      contenedor de basura 180 x 110 x 125 con tapas y ruedas (cobertura baja)
  SM_Prop_Drum          bidón de 200 l (58 x 88) con aros
  SM_Prop_TireStack     pila de 3 neumáticos
  SM_Prop_Pallet        palé europeo 120 x 80 x 14  ·  SM_Prop_PalletStack (6 palés, ladeados)
  SM_Prop_Crate         caja de madera 100 x 80 x 70
  SM_Prop_PowerPole     poste de hormigón de 900 cm con cruceta y aisladores
  SM_Prop_Cable         tramo de cable de 1000 cm con catenaria (se escala en X a la distancia entre postes)
  SM_Prop_Rubble_A/B/C  montones de escombro (cascotes, losa rota, ladrillos) por semilla
  SM_Prop_TrashBags     bolsas de basura amontonadas
  SM_Prop_Bollard       bolardo de hormigón
"""
import math
import os
import random
import sys

sys.path.append(os.path.dirname(os.path.abspath(__file__)))
import bpy
from mathutils import Matrix, Vector

import bl_lib
from bl_kit import Kit

M_CONCRETE = ("MI_Prop_Concrete", (0.5, 0.5, 0.48, 1), 0.9, 0.0)
M_WOOD = ("MI_Prop_Plywood", (0.4, 0.3, 0.2, 1), 0.8, 0.0)
M_GREEN = ("MI_Prop_DumpsterGreen", (0.2, 0.3, 0.2, 1), 0.6, 0.3)
M_RED = ("MI_Prop_DrumRed", (0.5, 0.15, 0.1, 1), 0.5, 0.4)
M_RUBBER = ("MI_Veh_Rubber", (0.02, 0.02, 0.02, 1), 0.9, 0.0)
M_TRIM = ("MI_Veh_Trim", (0.05, 0.05, 0.05, 1), 0.6, 0.0)
M_STEEL = ("MI_Veh_Steel", (0.6, 0.6, 0.6, 1), 0.35, 1.0)
M_PLASTIC = ("MI_Obj_Plastic", (0.03, 0.03, 0.03, 1), 0.5, 0.0)
M_BRICK = ("MI_Prop_Brick", (0.45, 0.25, 0.18, 1), 0.9, 0.0)

OUT = "."
PREVIEW = None


def rot(m, x, y, z, rx=0.0, ry=0.0, rz=0.0):
    """Matriz Unreal: gira (grados) alrededor de (x, y, z)."""
    return Matrix.Translation((x, y, z)) @ Matrix.Rotation(math.radians(rz), 4, 'Z') @ Matrix.Rotation(math.radians(ry), 4, 'Y') \
        @ Matrix.Rotation(math.radians(rx), 4, 'X') @ Matrix.Translation((-x, -y, -z)) @ m


def save(kit):
    kit.build()
    tris = kit.export(os.path.join(OUT, kit.name + ".fbx"))
    print(f"[street] {kit.name}: {tris} tris, {len(kit.collisions)} colisiones")
    if PREVIEW:
        preview(kit.mesh, os.path.join(PREVIEW, kit.name + ".png"))


def dumpster():
    k = Kit("SM_Prop_Dumpster", [M_GREEN, M_TRIM, M_RUBBER])
    # Cuerpo trapezoidal (más ancho arriba) con refuerzos
    k.prism("Body", [(-48, 18), (48, 18), (55, 118), (-55, 118)], 'x', -90, 90, 0, bevel=0.012)
    k.box("Rim", -92, 92, -57, 57, 114, 120, 0, bevel=0.01)
    for x in (-60, 0, 60):
        k.prism(f"Rib{x}", [(-50, 22), (50, 22), (57, 112), (-57, 112)], 'x', x - 3, x + 3, 0, bevel=0.006)
    # Tapas: una cerrada y otra entreabierta
    k.box("Lid0", -90, 0, -57, 57, 120, 124, 1, bevel=0.01)
    lid = k.box("Lid1", 0, 90, -57, 57, 120, 124, 1, bevel=0.01)
    k.transform(lid, rot(Matrix(), 0, 57, 122, rx=-18))
    for sx in (-1, 1):
        for sy in (-1, 1):
            k.box(f"Caster{sx}{sy}", sx * 75 - 6, sx * 75 + 6, sy * 40 - 6, sy * 40 + 6, 8, 18, 1, bevel=0.003)
            k.lathe(f"Wheel{sx}{sy}", [(-3, 7.5), (3, 7.5)], (sx * 75, sy * 40, 7.5), (0, 1, 0), seg=14, mat=2)
    for sy in (-1, 1):
        k.box(f"Handle{sy}", -70, 70, sy * 58 - 3 * (sy < 0), sy * 58 + 3 * (sy > 0), 95, 99, 1, bevel=0.003)
    k.collision_box(-92, 92, -57, 57, 0, 124)
    save(k)


def drum():
    k = Kit("SM_Prop_Drum", [M_RED, M_STEEL])
    k.lathe("Shell", [(0, 28.5), (88, 28.5)], (0, 0, 0), (0, 0, 1), seg=24, mat=0)
    for z in (1, 29, 59, 87):
        k.lathe(f"Ring{z}", [(z - 1.2, 29.6), (z + 1.2, 29.6)], (0, 0, 0), (0, 0, 1), seg=24, mat=0)
    k.lathe("Bung", [(88, 3), (90, 3)], (14, 8, 0), (0, 0, 1), seg=10, mat=1)
    k.collision_hull([(-29, 0), (29, 0), (29, 90), (-29, 90)], -29, 29)
    save(k)


def tire(k, name, x, y, z, rx=0.0, rz=0.0):
    t = k.lathe(name, [(-10, 25), (-11, 31), (-9, 36), (9, 36), (11, 31), (10, 25)], (0, 0, 0), (0, 0, 1), seg=24, closed=True, mat=0)
    k.transform(t, Matrix.Translation((x, y, z)) @ Matrix.Rotation(math.radians(rz), 4, 'Z') @ Matrix.Rotation(math.radians(rx), 4, 'X'))


def tire_stack():
    k = Kit("SM_Prop_TireStack", [M_RUBBER])
    tire(k, "T0", 0, 0, 11)
    tire(k, "T1", 3, -2, 33, rx=2)
    tire(k, "T2", -4, 3, 55, rx=-3)
    tire(k, "T3", 60, 20, 30, rx=80, rz=25)       # uno apoyado de canto al lado
    k.collision_box(-37, 37, -37, 37, 0, 66)
    save(k)


def pallet_parts(k, prefix, m):
    for i, y in enumerate((-36, 0, 36)):
        k.transform(k.box(f"{prefix}Block{i}", -60, 60, y - 5, y + 5, 2, 12, 0, bevel=0.003), m)
    for i in range(5):
        x = -50 + i * 25
        k.transform(k.box(f"{prefix}Top{i}", x - 7, x + 7, -40, 40, 12, 14.2, 0, bevel=0.002), m)
    for i, x in enumerate((-55, 0, 55)):
        k.transform(k.box(f"{prefix}Bot{i}", x - 6, x + 6, -40, 40, 0, 2, 0, bevel=0.002), m)


def pallet():
    k = Kit("SM_Prop_Pallet", [M_WOOD])
    pallet_parts(k, "P", Matrix())
    k.collision_box(-60, 60, -40, 40, 0, 14.2)
    save(k)


def pallet_stack():
    k = Kit("SM_Prop_PalletStack", [M_WOOD])
    rng = random.Random(3)
    for i in range(6):
        m = Matrix.Translation((rng.uniform(-4, 4), rng.uniform(-3, 3), i * 14.4)) @ Matrix.Rotation(math.radians(rng.uniform(-6, 6)), 4, 'Z')
        pallet_parts(k, f"S{i}", m)
    k.collision_box(-64, 64, -44, 44, 0, 87)
    save(k)


def crate():
    k = Kit("SM_Prop_Crate", [M_WOOD])
    for i in range(5):
        z = i * 14
        for sy in (-1, 1):
            k.box(f"SideY{i}{sy}", -50, 50, sy * 40 - 2 * (sy > 0), sy * 40 + 2 * (sy < 0), z + 0.5, z + 13.5, 0, bevel=0.003)
        for sx in (-1, 1):
            k.box(f"SideX{i}{sx}", sx * 50 - 2 * (sx > 0), sx * 50 + 2 * (sx < 0), -38, 38, z + 0.5, z + 13.5, 0, bevel=0.003)
    for sx in (-1, 1):
        for sy in (-1, 1):
            k.box(f"Corner{sx}{sy}", sx * 50 - 6 * (sx > 0) - 1 * (sx < 0), sx * 50 + 6 * (sx < 0) + 1 * (sx > 0),
                  sy * 40 - 6 * (sy > 0) - 1 * (sy < 0), sy * 40 + 6 * (sy < 0) + 1 * (sy > 0), 0, 70, 0, bevel=0.003)
    k.box("Lid", -51, 51, -41, 41, 70, 72.5, 0, bevel=0.004)
    k.collision_box(-51, 51, -41, 41, 0, 72.5)
    save(k)


def power_pole():
    k = Kit("SM_Prop_PowerPole", [M_CONCRETE, M_STEEL, M_PLASTIC])
    k.prism("Pole", [(-14, 0), (14, 0), (8, 900), (-8, 900)], 'x', -11, 11, 0, bevel=0.01)
    k.box("CrossArm", -6, 6, -110, 110, 840, 852, 1, bevel=0.004)
    for y in (-95, 0, 95):
        k.lathe(f"Ins{y}", [(0, 3.5), (4, 6), (8, 3.5), (12, 6), (16, 3.5)], (0, y, 852), (0, 0, 1), seg=10, mat=2)
    k.box("Brace", -4, 4, -60, 60, 790, 796, 1, bevel=0.003)
    k.box("Box", 12, 32, -15, 15, 300, 345, 1, bevel=0.006)       # caja de registro
    k.collision_box(-12, 12, -15, 15, 0, 900)
    save(k)


def cable():
    """Tramo de 1000 cm en +X desde el origen con catenaria (flecha 55 cm). Sin colisión."""
    k = Kit("SM_Prop_Cable", [M_TRIM])
    n = 24
    prof = []
    for i in range(n + 1):
        x = i / n * 1000.0
        z = -55.0 * (1.0 - ((x - 500.0) / 500.0) ** 2)
        prof.append((x, z))
    # tubo a lo largo de la curva: segmentos rectos cortos
    for i in range(n):
        (x0, z0), (x1, z1) = prof[i], prof[i + 1]
        d = Vector((x1 - x0, 0, z1 - z0))
        seg = k.lathe(f"C{i}", [(0, 1.0), (d.length + 0.3, 1.0)], (x0, 0, z0), (d.x, 0, d.z), seg=6, mat=0)
    save(k)


def rubble(name, seed, size=150.0):
    k = Kit(name, [M_CONCRETE, M_BRICK, M_STEEL])
    rng = random.Random(seed)
    # Losa rota inclinada con varillas
    sx, sy = rng.uniform(0.4, 0.55) * size, rng.uniform(0.28, 0.4) * size
    slab = k.box("Slab", -sx / 2, sx / 2, -sy / 2, sy / 2, 0, 14, 0, bevel=0.02)
    k.transform(slab, rot(Matrix(), 0, 0, 7, rx=rng.uniform(-18, 18), ry=rng.uniform(8, 22), rz=rng.uniform(0, 180)) @ Matrix.Translation((0, 0, 18)))
    for i in range(4):
        r = k.lathe(f"Rebar{i}", [(0, 0.7), (rng.uniform(25, 60), 0.7)], (rng.uniform(-sx / 3, sx / 3), rng.uniform(-sy / 3, sy / 3), 25),
                    (rng.uniform(-1, 1), rng.uniform(-1, 1), rng.uniform(0.2, 0.8)), seg=5, mat=2)
    # Cascotes
    for i in range(rng.randint(45, 60)):
        s = rng.uniform(6, 26) * (1.0 if rng.random() < 0.8 else 1.5)
        a = rng.uniform(0, math.tau)
        d = rng.uniform(0, size * 0.55) ** 0.9
        x, y = math.cos(a) * d, math.sin(a) * d
        z = max(0.0, (size * 0.32 - d * 0.62)) * rng.uniform(0.5, 1.0)
        mat = 1 if rng.random() < 0.35 else 0
        if mat:   # ladrillo (24 x 11,5 x 7, a veces partido)
            bl = 24.0 * rng.choice((1.0, 1.0, 0.6, 0.45))
            c = k.box(f"Chunk{i}", -bl / 2, bl / 2, -5.75, 5.75, -3.5, 3.5, mat, bevel=0.003)
            sz = 7.0
        else:
            sz = s * rng.uniform(0.5, 0.9)
            c = k.box(f"Chunk{i}", -s / 2, s / 2, -s * 0.35, s * 0.35, -sz / 2, sz / 2, mat, bevel=0.006)
        if not mat:   # cascote irregular: vértices desplazados (los ladrillos se quedan rectos)
            for v in c.data.vertices:
                v.co += Vector((rng.uniform(-1, 1), rng.uniform(-1, 1), rng.uniform(-1, 1))) * s * 0.0014
        k.transform(c, Matrix.Translation((x, y, z + sz / 2)) @ Matrix.Rotation(rng.uniform(0, math.tau), 4, 'Z')
                    @ Matrix.Rotation(rng.uniform(-0.6, 0.6), 4, 'X') @ Matrix.Rotation(rng.uniform(-0.6, 0.6), 4, 'Y'))
    k.collision_hull([(-size * 0.45, 0), (size * 0.45, 0), (size * 0.2, size * 0.32), (-size * 0.2, size * 0.32)], -size * 0.45, size * 0.45)
    save(k)


def trash_bags():
    k = Kit("SM_Prop_TrashBags", [M_PLASTIC])
    rng = random.Random(9)
    for i, (x, y, z) in enumerate(((0, 0, 0), (45, 10, 0), (20, -35, 0), (15, -5, 38), (-40, 25, 0))):
        s = rng.uniform(0.85, 1.1)
        b = k.rounded_box(f"Bag{i}", x, y, z + 22 * s, 50 * s, 42 * s, 44 * s, radius=18 * s, mat=0, segments=3)
        k.transform(b, rot(Matrix(), x, y, z, rx=rng.uniform(-10, 10), ry=rng.uniform(-10, 10), rz=rng.uniform(0, 90)))
        k.box(f"Knot{i}", x - 5, x + 5, y - 5, y + 5, z + 42 * s, z + 52 * s, 0, bevel=0.02)
    k.collision_box(-60, 70, -55, 50, 0, 80)
    save(k)


def bollard():
    k = Kit("SM_Prop_Bollard", [M_CONCRETE])
    k.lathe("B", [(0, 15), (70, 13), (76, 9), (80, 0.5)], (0, 0, 0), (0, 0, 1), seg=16, mat=0)
    k.collision_box(-15, 15, -15, 15, 0, 80)
    save(k)


def preview(obj, path):
    scene = bpy.context.scene
    scene.render.engine = 'BLENDER_WORKBENCH'
    scene.display.shading.light = 'STUDIO'
    scene.display.shading.color_type = 'MATERIAL'
    scene.display.shading.show_cavity = True
    scene.render.resolution_x, scene.render.resolution_y = 800, 600
    scene.world = scene.world or bpy.data.worlds.new("W")
    scene.world.color = (0.55, 0.57, 0.6)
    cam_data = bpy.data.cameras.new("PrevCam")
    cam_data.type = 'ORTHO'
    dims = obj.dimensions
    cam_data.ortho_scale = max(dims.x, dims.y, dims.z) * 1.3
    cam = bpy.data.objects.new("PrevCam", cam_data)
    scene.collection.objects.link(cam)
    scene.camera = cam
    center = obj.matrix_world @ (sum((Vector(b) for b in obj.bound_box), Vector()) / 8)
    d = Vector((0.8, -0.65, 0.45)).normalized()
    cam.location = center + d * (max(dims) * 3)
    cam.rotation_euler = (-d).to_track_quat('-Z', 'Y').to_euler()
    for o in scene.objects:
        if o.name.startswith("UCX_"):
            o.hide_render = True
    scene.render.filepath = path
    bpy.ops.render.render(write_still=True)
    bpy.data.objects.remove(cam)


BUILDERS = {
    "SM_Prop_Dumpster": dumpster,
    "SM_Prop_Drum": drum,
    "SM_Prop_TireStack": tire_stack,
    "SM_Prop_Pallet": pallet,
    "SM_Prop_PalletStack": pallet_stack,
    "SM_Prop_Crate": crate,
    "SM_Prop_PowerPole": power_pole,
    "SM_Prop_Cable": cable,
    "SM_Prop_Rubble_A": lambda: rubble("SM_Prop_Rubble_A", 11, 150),
    "SM_Prop_Rubble_B": lambda: rubble("SM_Prop_Rubble_B", 23, 220),
    "SM_Prop_Rubble_C": lambda: rubble("SM_Prop_Rubble_C", 37, 110),
    "SM_Prop_TrashBags": trash_bags,
    "SM_Prop_Bollard": bollard,
}


def main():
    global OUT, PREVIEW
    argv = sys.argv[sys.argv.index("--") + 1:] if "--" in sys.argv else []
    OUT = os.path.abspath(argv[0] if argv else os.path.join(os.path.dirname(__file__), "..", "export", "Props"))
    PREVIEW = os.path.abspath(argv[1]) if len(argv) > 1 and not argv[1].startswith("--") else None
    only = [a.split("=", 1)[1] for a in argv if a.startswith("--only=")]
    os.makedirs(OUT, exist_ok=True)
    if PREVIEW:
        os.makedirs(PREVIEW, exist_ok=True)
    for name, fn in BUILDERS.items():
        if only and name not in only[0].split(","):
            continue
        bl_lib.reset_scene()
        fn()


if __name__ == "__main__":
    main()
