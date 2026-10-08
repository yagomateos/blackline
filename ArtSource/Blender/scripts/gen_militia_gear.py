"""Equipo del miliciano de la Columna Vesk (Bloque 8): piezas rígidas enganchadas a huesos del Mannequin.

Uso:  blender -b --factory-startup -P gen_militia_gear.py -- <carpeta_salida> [preview_dir]

Espacios (cm, convención de Unreal de bl_kit.py):
  - SM_Militia_Vest y SM_Militia_Helmet se modelan en el espacio de la MALLA del Mannequin en su pose de referencia
    (X = izquierda del personaje, Y = adelante, Z = arriba; pecho/spine_05 en z 141, cabeza en z 162).
    ABLEnemyCharacter las engancha a spine_05 / head con la inversa de la pose de referencia del hueso.
  - SM_Militia_Armband se modela en el espacio LOCAL del hueso upperarm_l (X a lo largo del brazo).
Sin colisión (las balas usan el Physics Asset del cuerpo).
"""
import math
import os
import sys

sys.path.append(os.path.dirname(os.path.abspath(__file__)))
import bpy
from mathutils import Matrix, Vector

import bl_lib
from bl_kit import Kit

M_FABRIC = ("MI_Gear_Fabric", (0.25, 0.25, 0.18, 1), 0.95, 0.0)
M_WEBBING = ("MI_Gear_Webbing", (0.12, 0.12, 0.1, 1), 0.9, 0.0)
M_HELMET = ("MI_Gear_Helmet", (0.22, 0.25, 0.18, 1), 0.65, 0.2)
M_BLACK = ("MI_Gear_Black", (0.02, 0.02, 0.02, 1), 0.9, 0.0)
M_PLASTIC = ("MI_Obj_Plastic", (0.03, 0.03, 0.03, 1), 0.5, 0.0)

OUT = "."
PREVIEW = None


def save(kit):
    kit.build()
    tris = kit.export(os.path.join(OUT, kit.name + ".fbx"))
    print(f"[gear] {kit.name}: {tris} tris")
    if PREVIEW:
        preview(kit.mesh, os.path.join(PREVIEW, kit.name + ".png"))


def vest():
    """Portaplacas con bolsas de cargador, radio y cinturón ancho. Pecho del Mannequin: x +-17, y -13..+12."""
    k = Kit("SM_Militia_Vest", [M_FABRIC, M_WEBBING, M_PLASTIC])
    # Paneles delantero y trasero (algo curvados: tres tramos)
    for name, y0, y1 in (("Front", 11.0, 15.5), ("Back", -17.0, -12.5)):
        k.rounded_box(f"{name}C", 0, (y0 + y1) / 2, 127, 22, y1 - y0, 40, radius=1.6, mat=0)
        for s in (-1, 1):
            p = k.rounded_box(f"{name}S{s}", s * 13.5, (y0 + y1) / 2 - (1.6 if name == "Front" else -1.6), 125, 9, y1 - y0, 34, radius=1.4, mat=0)
            k.transform(p, Matrix.Translation((s * 13.5, (y0 + y1) / 2, 125)) @ Matrix.Rotation(math.radians(-s * 18 * (1 if name == "Front" else -1)), 4, 'Z')
                        @ Matrix.Translation((-s * 13.5, -(y0 + y1) / 2, -125)))
    # Faja lateral (une delante y detrás) y hombreras
    for s in (-1, 1):
        k.rounded_box(f"Cummer{s}", s * 17.5, -0.5, 116, 3.5, 25, 18, radius=1.2, mat=1)
        strap = k.prism(f"Strap{s}", [(12.5, 146), (13.5, 149.5), (2, 152.5), (-6, 152.5), (-12.5, 149), (-12.5, 146)], 'x', s * 9 - 2.2, s * 9 + 2.2, 1, bevel=0.006)
    # Bolsas de cargador (3) con tapa y cinta, bolsa administrativa arriba, radio a la izquierda
    for i, x in enumerate((-9.5, 0.0, 9.5)):
        k.rounded_box(f"Mag{i}", x, 18.5, 116, 8.6, 6.0, 17, radius=1.2, mat=0)
        k.rounded_box(f"MagFlap{i}", x, 21.2, 123.5, 8.8, 1.2, 5.5, radius=0.5, mat=0)
        k.box(f"MagTab{i}", x - 1.2, x + 1.2, 21.6, 22.3, 118, 124, 1, bevel=0.002)
    k.rounded_box("Admin", 0, 17.2, 137, 20, 3.4, 12, radius=1.0, mat=0)
    k.rounded_box("Radio", 18.5, 6, 132, 4.5, 7, 13, radius=0.8, mat=2)
    k.lathe("Antenna", [(0, 0.45), (26, 0.3)], (19, 4, 138), (0.05, -0.15, 1), seg=6, mat=2)
    k.rounded_box("RadioPouch", 18.8, 6, 128, 5.5, 8, 10, radius=1.0, mat=0)
    # Hileras de MOLLE en la espalda
    for r in range(4):
        k.box(f"Molle{r}", -10, 10, -18.2, -17.0, 112 + r * 7, 114 + r * 7, 1, bevel=0.0)
    save(k)


def helmet():
    """Casco de acero antiguo con funda tensada y barboquejo. Cabeza: centro aprox. (0, 1.5, 168), radio ~11."""
    k = Kit("SM_Militia_Helmet", [M_HELMET, M_WEBBING])
    prof = [(0.0, 14.2), (1.0, 14.0), (4.0, 13.4), (7.0, 12.0), (10.0, 9.6), (12.5, 6.5), (14.0, 3.2), (14.6, 0.5)]
    shell = k.lathe("Shell", prof, (0, 1.5, 165.5), (0, 0, 1), seg=28, mat=0)
    k.transform(shell, Matrix.Translation((0, 1.5, 165.5)) @ Matrix.Rotation(math.radians(-8), 4, 'X') @ Matrix.Translation((0, -1.5, -165.5)))
    k.lathe("Rim", [(-0.6, 14.6), (0.6, 14.6), (0.6, 13.8), (-0.6, 13.8)], (0, 1.5, 165.5), (0, 0, 1), seg=28, closed=True, mat=1)
    for s in (-1, 1):
        k.box(f"Chin{s}", s * 11.2 - 0.6, s * 11.2 + 0.6, 0.5, 2.5, 150, 165, 1, bevel=0.0)
    save(k)


def armband():
    """Brazalete negro de la Columna en el brazo izquierdo (espacio del hueso upperarm_l: X a lo largo del brazo)."""
    k = Kit("SM_Militia_Armband", [M_BLACK])
    k.lathe("Band", [(9.0, 6.4), (15.0, 6.2), (15.0, 5.7), (9.0, 5.9)], (0, 0, 0), (1, 0, 0), seg=18, closed=True, mat=0)
    save(k)


def preview(obj, path):
    scene = bpy.context.scene
    scene.render.engine = 'BLENDER_WORKBENCH'
    scene.display.shading.light = 'STUDIO'
    scene.display.shading.color_type = 'MATERIAL'
    scene.render.resolution_x, scene.render.resolution_y = 700, 700
    scene.world = scene.world or bpy.data.worlds.new("W")
    scene.world.color = (0.55, 0.57, 0.6)
    cam_data = bpy.data.cameras.new("PrevCam")
    cam_data.type = 'ORTHO'
    dims = obj.dimensions
    cam_data.ortho_scale = max(dims.x, dims.y, dims.z) * 1.4
    cam = bpy.data.objects.new("PrevCam", cam_data)
    scene.collection.objects.link(cam)
    scene.camera = cam
    center = obj.matrix_world @ (sum((Vector(b) for b in obj.bound_box), Vector()) / 8)
    d = Vector((0.6, -0.75, 0.3)).normalized()      # Blender -Y = Unreal +Y (frente del personaje)
    cam.location = center + d * (max(dims) * 4)
    cam.rotation_euler = (-d).to_track_quat('-Z', 'Y').to_euler()
    scene.render.filepath = path
    bpy.ops.render.render(write_still=True)
    bpy.data.objects.remove(cam)


def main():
    global OUT, PREVIEW
    argv = sys.argv[sys.argv.index("--") + 1:] if "--" in sys.argv else []
    OUT = os.path.abspath(argv[0] if argv else os.path.join(os.path.dirname(__file__), "..", "export", "Gear"))
    PREVIEW = os.path.abspath(argv[1]) if len(argv) > 1 else None
    os.makedirs(OUT, exist_ok=True)
    if PREVIEW:
        os.makedirs(PREVIEW, exist_ok=True)
    for fn in (vest, helmet, armband):
        bl_lib.reset_scene()
        fn()


if __name__ == "__main__":
    main()
