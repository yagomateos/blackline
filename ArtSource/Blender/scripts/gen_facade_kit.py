"""Kit modular de fachadas (Bloque 8): módulos de 400 cm de ancho x 320 cm por planta que ABLBuilding
coloca con instancias sobre el núcleo de cada edificio.

Uso:  blender -b --factory-startup -P gen_facade_kit.py -- <carpeta_salida> [preview_dir] [--only=SM_A,SM_B]

Convención de cada módulo (coordenadas de Unreal, cm): la fachada mira a +X; el muro ocupa x = -25..0
(el núcleo del edificio queda 25 cm por detrás); el módulo va de y = -200 a 200 y de z = 0 a 320.
Lo que sobresale (alféizares, balcones, rótulos, bajantes) va en x > 0.
Ranuras de material (ABLBuilding pone el muro de cada edificio en la 0):
  0 MI_Fac_Wall · 1 MI_Fac_Frame (carpintería) · 2 MI_Fac_Glass · 3 MI_Fac_Trim (piedra/hormigón)
  4 MI_Fac_Metal (barandillas, persianas, puertas) · 5 MI_Fac_Sign (rótulo pintado)
Colisión: solo los módulos de planta baja (UCX del muro completo); los de arriba no la necesitan.
"""
import math
import os
import sys

sys.path.append(os.path.dirname(os.path.abspath(__file__)))
import bpy
from mathutils import Matrix, Vector

import bl_lib
from bl_kit import Kit

MATS = [
    ("MI_Fac_Wall", (0.75, 0.72, 0.65, 1), 0.9, 0.0),
    ("MI_Fac_Frame", (0.85, 0.85, 0.82, 1), 0.5, 0.0),
    ("MI_Fac_Glass", (0.05, 0.06, 0.07, 1), 0.08, 0.0),
    ("MI_Fac_Trim", (0.6, 0.58, 0.55, 1), 0.85, 0.0),
    ("MI_Fac_Metal", (0.3, 0.32, 0.33, 1), 0.55, 0.8),
    ("MI_Fac_Sign", (0.6, 0.2, 0.15, 1), 0.6, 0.0),
]
WALL, FRAME, GLASS, TRIM, METAL, SIGN = range(6)
W2 = 200.0       # medio ancho del módulo
H = 320.0        # altura de planta
D = 25.0         # grosor del muro de fachada
OUT = "."
PREVIEW = None


def wall_with_opening(k, oy0, oy1, oz0, oz1, z1=H, mat=WALL, name="Wall"):
    """Muro del módulo con un hueco rectangular (cuatro piezas: así quedan los derrames del hueco)."""
    k.box(name + "_L", -D, 0, -W2, oy0, 0, z1, mat, bevel=0.0)
    k.box(name + "_R", -D, 0, oy1, W2, 0, z1, mat, bevel=0.0)
    if oz0 > 0:
        k.box(name + "_B", -D, 0, oy0, oy1, 0, oz0, mat, bevel=0.0)
    if oz1 < z1:
        k.box(name + "_T", -D, 0, oy0, oy1, oz1, z1, mat, bevel=0.0)


def frame(k, oy0, oy1, oz0, oz1, x0=-17.0, w=6.0, mullions=1, transom=None, name="Frame"):
    """Marco de carpintería pegado al derrame, con montantes verticales y travesaño opcional."""
    x1 = x0 + 6.0
    k.box(name + "_L", x0, x1, oy0, oy0 + w, oz0, oz1, FRAME, 0.006)
    k.box(name + "_R", x0, x1, oy1 - w, oy1, oz0, oz1, FRAME, 0.006)
    k.box(name + "_B", x0, x1, oy0, oy1, oz0, oz0 + w, FRAME, 0.006)
    k.box(name + "_T", x0, x1, oy0, oy1, oz1 - w, oz1, FRAME, 0.006)
    for i in range(mullions):
        y = oy0 + (oy1 - oy0) * (i + 1) / (mullions + 1)
        k.box(f"{name}_M{i}", x0 + 0.5, x1 - 0.5, y - 3, y + 3, oz0, oz1, FRAME, 0.005)
    if transom:
        k.box(name + "_Tr", x0 + 0.5, x1 - 0.5, oy0, oy1, transom - 3, transom + 3, FRAME, 0.005)
    k.box(name + "_Glass", x0 + 2.0, x0 + 3.0, oy0 + w, oy1 - w, oz0 + w, oz1 - w, GLASS, 0.0)


def ground_collision(k):
    k.collision_box(-D, 0, -W2, W2, 0, H)


# ---------------------------------------------------------------------------
# Plantas de pisos
# ---------------------------------------------------------------------------

def window_a():
    """Ventana de piso estándar 140 x 150 con alféizar, dintel y caja de persiana."""
    k = Kit("SM_Fac_Window", MATS)
    k.collision_box(-D, 0, -W2, W2, 0, H)
    oy, z0, z1 = 70.0, 90.0, 240.0
    wall_with_opening(k, -oy, oy, z0, z1)
    frame(k, -oy, oy, z0, z1, mullions=1)
    k.box("Sill", -17, 5, -oy - 10, oy + 10, z0 - 6, z0, TRIM, 0.01)
    k.box("Lintel", 0, 2.5, -oy - 8, oy + 8, z1, z1 + 12, TRIM, 0.006)
    k.box("ShutterBox", -17, -8, -oy, oy, z1 - 18, z1, METAL, 0.005)    # caja de la persiana dentro del hueco
    save(k)


def window_double():
    """Ventana ancha de dos hojas 200 x 150 (salón)."""
    k = Kit("SM_Fac_WindowWide", MATS)
    k.collision_box(-D, 0, -W2, W2, 0, H)
    oy, z0, z1 = 100.0, 90.0, 240.0
    wall_with_opening(k, -oy, oy, z0, z1)
    frame(k, -oy, oy, z0, z1, mullions=2)
    k.box("Sill", -17, 5, -oy - 10, oy + 10, z0 - 6, z0, TRIM, 0.01)
    k.box("Lintel", 0, 2.5, -oy - 8, oy + 8, z1, z1 + 12, TRIM, 0.006)
    k.box("ShutterBox", -17, -8, -oy, oy, z1 - 18, z1, METAL, 0.005)
    save(k)


def window_balcony():
    """Puerta-ventana de balcón 120 x 240 (el balcón es otra malla)."""
    k = Kit("SM_Fac_BalconyDoor", MATS)
    k.collision_box(-D, 0, -W2, W2, 0, H)
    oy, z0, z1 = 60.0, 0.0, 240.0
    wall_with_opening(k, -oy, oy, z0, z1)
    frame(k, -oy, oy, z0 + 2, z1, mullions=1, transom=200)
    k.box("Threshold", -18, 2, -oy, oy, 0, 3, TRIM, 0.004)
    k.box("Lintel", 0, 2.5, -oy - 8, oy + 8, z1, z1 + 12, TRIM, 0.006)
    k.box("ShutterBox", -17, -8, -oy, oy, z1 - 18, z1, METAL, 0.005)
    save(k)


def blank():
    k = Kit("SM_Fac_Blank", MATS)
    k.collision_box(-D, 0, -W2, W2, 0, H)
    k.box("Wall", -D, 0, -W2, W2, 0, H, WALL, bevel=0.0)
    k.box("Vent", 0, 2, 120, 150, 250, 268, METAL, 0.004)
    for i in range(4):
        k.box(f"VentSlat{i}", 2, 3, 122, 148, 252 + i * 4, 254 + i * 4, METAL, 0.0)
    save(k)


def balcony():
    """Balcón volado: losa con goterón y barandilla de barrotes. Pivote al nivel del suelo de la planta."""
    k = Kit("SM_Fac_Balcony", MATS)
    k.box("Slab", 0, 95, -140, 140, -16, 0, TRIM, 0.012)
    k.box("Drip", 88, 95, -140, 140, -22, -16, TRIM, 0.004)
    k.box("RailTop", 86, 96, -140, 140, 98, 103, METAL, 0.008)
    k.box("RailMid", 89, 93, -138, 138, 10, 13, METAL, 0.005)
    for s in (-1, 1):
        k.box(f"RailSide{s}", 0, 96, s * 138 - 2.5, s * 138 + 2.5, 98, 103, METAL, 0.008)
        k.box(f"RailSideMid{s}", 0, 92, s * 138 - 1.5, s * 138 + 1.5, 10, 13, METAL, 0.005)
        k.box(f"PostCorner{s}", 88, 94, s * 137 - 3, s * 137 + 3, 0, 98, METAL, 0.006)
        for i in range(1, 8):
            x = i * 11.0
            k.box(f"SideBar{s}_{i}", x - 0.9, x + 0.9, s * 138 - 0.9, s * 138 + 0.9, 13, 98, METAL, 0.0)
    y = -130.0
    while y < 131:
        k.box(f"Bar{int(y)}", 90.1, 91.9, y - 0.9, y + 0.9, 13, 98, METAL, 0.0)
        y += 11.0
    save(k)


def shutter():
    """Persiana enrollable de lamas (pivote arriba en el centro; 100 cm hacia abajo: se escala en Z)."""
    k = Kit("SM_Fac_Shutter", MATS)
    for i in range(20):
        z = -i * 5.0
        k.box(f"Slat{i}", -10, -8, -66, 66, z - 4.6, z, METAL, 0.0)
        k.box(f"SlatLip{i}", -11, -10, -66, 66, z - 1.2, z, METAL, 0.0)
    k.box("Bottom", -11.5, -7.5, -66, 66, -103, -99, METAL, 0.004)
    save(k)


# ---------------------------------------------------------------------------
# Planta baja
# ---------------------------------------------------------------------------

def plinth(k, y0=-W2, y1=W2):
    k.box("Plinth", 0, 3, y0, y1, 0, 45, TRIM, 0.006)


def shop_closed():
    """Local comercial con la persiana metálica bajada y rótulo."""
    k = Kit("SM_Fac_ShopClosed", MATS)
    oy, z1 = 170.0, 265.0
    wall_with_opening(k, -oy, oy, 0, z1)
    for i in range(int(z1 / 6)):
        z = i * 6.0
        k.box(f"Slat{i}", -13, -10.5, -oy + 4, oy - 4, z, z + 5.4, METAL, 0.0)
    for s in (-1, 1):
        k.box(f"Guide{s}", -14, -8, s * (oy - 2) - 3, s * (oy - 2) + 3, 0, z1, METAL, 0.004)
    k.box("Housing", -14, -4, -oy, oy, z1 - 20, z1, METAL, 0.006)
    k.box("Lock", -10.5, -8.5, -12, 12, 6, 14, METAL, 0.003)
    k.box("Sign", 0, 7, -185, 185, 272, 312, SIGN, 0.01)
    k.box("SignTrim", 0, 8, -185, 185, 270, 273, METAL, 0.003)
    plinth(k, -W2, -oy)
    plinth(k, oy, W2)
    ground_collision(k)
    save(k)


def shop_open():
    """Escaparate: carpintería metálica, cristal oscuro y puerta de cristal."""
    k = Kit("SM_Fac_ShopFront", MATS)
    oy, z1 = 170.0, 265.0
    wall_with_opening(k, -oy, oy, 0, z1)
    k.box("Base", -16, -6, -oy, oy, 0, 50, METAL, 0.004)
    frame(k, -oy, oy, 50, z1, x0=-16, w=5, mullions=3, transom=225, name="Front")
    k.box("Sign", 0, 7, -185, 185, 272, 312, SIGN, 0.01)
    k.box("SignTrim", 0, 8, -185, 185, 270, 273, METAL, 0.003)
    plinth(k, -W2, -oy)
    plinth(k, oy, W2)
    ground_collision(k)
    save(k)


def door():
    """Portal de viviendas: puerta metálica con cuarterones, montante de cristal y marquesina."""
    k = Kit("SM_Fac_Door", MATS)
    oy, z1 = 65.0, 280.0
    wall_with_opening(k, -oy, oy, 0, z1)
    k.box("Leaf", -16, -12, -60, 60, 0, 228, METAL, 0.006)
    for i, (za, zb) in enumerate(((20, 100), (120, 210))):
        for s in (-1, 1):
            k.box(f"Panel{i}{s}", -12, -10.5, s * 30 - 22, s * 30 + 22, za, zb, METAL, 0.004)
    k.box("Handle", -10.5, -6, 40, 44, 100, 112, FRAME, 0.003)
    frame(k, -oy, oy, 228, z1, x0=-17, w=5, mullions=1, name="Transom")
    k.box("Jamb", -20, -16, -oy, oy, 0, 230, TRIM, 0.004)
    k.box("Canopy", 0, 45, -95, 95, 290, 298, TRIM, 0.01)
    k.box("Step", 0, 28, -85, 85, 0, 12, TRIM, 0.01)
    plinth(k, -W2, -oy)
    plinth(k, oy, W2)
    k.collision_box(-D, 0, -W2, W2, 0, H)
    k.collision_box(0, 28, -85, 85, 0, 12)
    save(k)


def ground_blank():
    k = Kit("SM_Fac_GroundBlank", MATS)
    k.box("Wall", -D, 0, -W2, W2, 0, H, WALL, bevel=0.0)
    plinth(k)
    # Ventanuco enrejado de trastero
    oy, z0, z1 = 35.0, 190.0, 250.0
    k.cut(k.parts[0], k.box("Hole", -30, 5, -oy, oy, z0, z1, keep=False))
    k.box("Back", -24, -20, -oy, oy, z0, z1, GLASS, 0.0)
    for i in range(6):
        y = -oy + (i + 0.5) * (2 * oy / 6)
        k.box(f"Bar{i}", -5, -3, y - 1, y + 1, z0, z1, METAL, 0.0)
    ground_collision(k)
    save(k)


# ---------------------------------------------------------------------------
# Remates y extras
# ---------------------------------------------------------------------------

def band():
    """Imposta entre plantas (pivote en la línea de forjado; se repite cada módulo)."""
    k = Kit("SM_Fac_Band", MATS)
    k.box("Band", 0, 6, -W2, W2, -10, 8, TRIM, 0.008)
    k.box("BandLip", 0, 8, -W2, W2, 6, 9, TRIM, 0.004)
    save(k)


def cornice():
    """Peto de cubierta con albardilla y cornisa (pivote en la cara superior del núcleo)."""
    k = Kit("SM_Fac_Cornice", MATS)
    k.collision_box(-D, 0, -W2, W2, 0, 104)
    k.box("Parapet", -D, 0, -W2, W2, 0, 95, WALL, bevel=0.0)
    k.box("Coping", -D - 4, 6, -W2, W2, 95, 104, TRIM, 0.01)
    k.box("Cornice", 0, 14, -W2, W2, -12, 0, TRIM, 0.012)
    k.box("Cornice2", 0, 9, -W2, W2, -20, -12, TRIM, 0.008)
    save(k)


def pipe():
    """Bajante pluvial (pivote abajo; 100 cm: se escala en Z) con abrazaderas."""
    k = Kit("SM_Fac_Pipe", MATS)
    k.lathe("Pipe", [(0, 5.0), (100, 5.0)], (9, 0, 0), (0, 0, 1), seg=12, mat=METAL)
    for z in (20, 80):
        k.box(f"Clamp{z}", 0, 15, -6, 6, z - 1.5, z + 1.5, METAL, 0.003)
    save(k)


def ac_unit():
    """Aparato de aire acondicionado con rejilla, ventilador y escuadras."""
    k = Kit("SM_Fac_AC", MATS)
    k.box("Body", 0, 30, -42, 42, 0, 58, FRAME, 0.012)
    k.lathe("FanDisc", [(0, 18.0), (0.5, 18.0)], (27, -8, 29), (1, 0, 0), seg=24, mat=METAL)
    k.lathe("FanRing", [(0, 19.5), (3, 19.5), (3, 17.5), (0, 17.5)], (29, -8, 29), (1, 0, 0), seg=24, closed=True, mat=FRAME)
    for i in range(7):
        y = -24 + i * 5.5
        k.box(f"Grille{i}", 30, 31.5, y - 0.6, y + 0.6, 12, 46, METAL, 0.0)
    k.box("Side", 30, 31, 18, 38, 5, 53, METAL, 0.002)
    for s in (-1, 1):
        k.prism(f"Bracket{s}", [(0, 0), (34, 0), (0, -32)], 'y', s * 36 - 1.5, s * 36 + 1.5, METAL, 0.0)
    k.box("Pipe", -2, 3, 34, 37, -60, 10, METAL, 0.0)
    save(k)


def sign_vertical():
    """Rótulo vertical de comercio en banderola."""
    k = Kit("SM_Fac_SignBlade", MATS)
    k.box("Arm", 0, 70, -2, 2, 0, 4, METAL, 0.002)
    k.box("Arm2", 0, 70, -2, 2, 110, 114, METAL, 0.002)
    k.box("Panel", 15, 65, -6, 6, -10, 124, SIGN, 0.01)
    save(k)


# ---------------------------------------------------------------------------

def save(kit):
    kit.build()
    tris = kit.export(os.path.join(OUT, kit.name + ".fbx"))
    print(f"[facade] {kit.name}: {tris} tris, {len(kit.collisions)} colisiones")
    if PREVIEW:
        preview(kit.mesh, os.path.join(PREVIEW, kit.name + ".png"))


def preview(obj, path):
    scene = bpy.context.scene
    scene.render.engine = 'BLENDER_WORKBENCH'
    scene.display.shading.light = 'STUDIO'
    scene.display.shading.color_type = 'MATERIAL'
    scene.display.shading.show_cavity = True
    scene.render.resolution_x, scene.render.resolution_y = 900, 700
    scene.world = scene.world or bpy.data.worlds.new("W")
    scene.world.color = (0.55, 0.57, 0.6)
    cam_data = bpy.data.cameras.new("PrevCam")
    cam_data.type = 'ORTHO'
    dims = obj.dimensions
    cam_data.ortho_scale = max(dims.x, dims.y, dims.z) * 1.3
    cam = bpy.data.objects.new("PrevCam", cam_data)
    scene.collection.objects.link(cam)
    scene.camera = cam
    center = Vector(obj.matrix_world @ (sum((Vector(b) for b in obj.bound_box), Vector()) / 8))
    d = Vector((0.75, -0.55, 0.3)).normalized()   # fachada (+X de Unreal = +X de Blender) vista en escorzo
    cam.location = center + d * (max(dims) * 3)
    cam.rotation_euler = (-d).to_track_quat('-Z', 'Y').to_euler()
    for o in scene.objects:
        if o.name.startswith("UCX_"):
            o.hide_render = True
    scene.render.filepath = path
    bpy.ops.render.render(write_still=True)
    bpy.data.objects.remove(cam)


BUILDERS = {
    "SM_Fac_Window": window_a,
    "SM_Fac_WindowWide": window_double,
    "SM_Fac_BalconyDoor": window_balcony,
    "SM_Fac_Blank": blank,
    "SM_Fac_Balcony": balcony,
    "SM_Fac_Shutter": shutter,
    "SM_Fac_ShopClosed": shop_closed,
    "SM_Fac_ShopFront": shop_open,
    "SM_Fac_Door": door,
    "SM_Fac_GroundBlank": ground_blank,
    "SM_Fac_Band": band,
    "SM_Fac_Cornice": cornice,
    "SM_Fac_Pipe": pipe,
    "SM_Fac_AC": ac_unit,
    "SM_Fac_SignBlade": sign_vertical,
}


def main():
    global OUT, PREVIEW
    argv = sys.argv[sys.argv.index("--") + 1:] if "--" in sys.argv else []
    OUT = os.path.abspath(argv[0] if argv else os.path.join(os.path.dirname(__file__), "..", "export"))
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
