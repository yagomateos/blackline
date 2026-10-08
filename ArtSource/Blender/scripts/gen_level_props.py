"""Coberturas, contenedor y vehículos del nivel gris (Bloque 4) con su geometría y medidas finales.

Uso:
  blender -b --factory-startup -P gen_level_props.py -- <carpeta_salida> [preview_dir]
Genera un FBX por malla (con sus colisiones UCX_) en <carpeta_salida>:
  SM_Cover_Sandbag_Straight (120 x 55 x 115), SM_Cover_Sandbag_Corner, SM_Cover_Jersey (300 x 61 x 107),
  SM_Cover_TWall (150 x 120 x 370), SM_Container_20ft (606 x 244 x 259), SM_Container_20ft_Open,
  SM_Veh_Van_Civil (530 x 200 x 230), SM_Veh_Van_Civil_Open, SM_Veh_Sedan_Wreck (450 x 180 x 140).
Medidas de juego: Docs/Assets_Blender.md (cobertura baja 115-120, parcial 80-110, alta >= 190).
Pivote en la base, centrado. Convención de coordenadas: bl_kit.py (Unreal, cm).
"""
import math
import os
import random
import sys

sys.path.append(os.path.dirname(os.path.abspath(__file__)))
import bpy
from mathutils import Matrix, Vector

import bl_lib
from bl_kit import Kit, W

# Materiales por slot (el script de Unreal los sustituye por los suyos según el nombre)
M_CONCRETE = ("MI_Prop_Concrete", (0.5, 0.5, 0.48, 1), 0.9, 0.0)
M_FABRIC = ("MI_Prop_Sandbag", (0.45, 0.4, 0.3, 1), 0.95, 0.0)
M_STEEL = ("MI_Prop_ContainerSteel", (0.25, 0.3, 0.4, 1), 0.6, 0.6)
M_WOOD = ("MI_Prop_Plywood", (0.4, 0.3, 0.2, 1), 0.8, 0.0)
M_PAINT = ("MI_Veh_Paint", (0.8, 0.8, 0.78, 1), 0.4, 0.3)
M_GLASS = ("MI_Veh_Glass", (0.02, 0.03, 0.035, 1), 0.05, 0.0)
M_RUBBER = ("MI_Veh_Rubber", (0.02, 0.02, 0.02, 1), 0.9, 0.0)
M_TRIM = ("MI_Veh_Trim", (0.05, 0.05, 0.05, 1), 0.6, 0.0)
M_CHROME = ("MI_Veh_Steel", (0.6, 0.6, 0.6, 1), 0.35, 1.0)
M_BURNT = ("MI_Veh_Burnt", (0.12, 0.08, 0.06, 1), 0.9, 0.5)

OUT = "."
PREVIEW = None


def save(kit):
    kit.build()
    tris = kit.export(os.path.join(OUT, kit.name + ".fbx"))
    print(f"[level_props] {kit.name}: {tris} tris, {len(kit.collisions)} colisiones")
    if PREVIEW:
        preview(kit.mesh, os.path.join(PREVIEW, kit.name + ".png"))


# ---------------------------------------------------------------------------
# Sacos terreros: 9 hiladas de sacos de 60 x 30 x 13 con aparejo trabado, dos filas en profundidad
# ---------------------------------------------------------------------------

def sandbag_run(kit, rng, length, rows=9, depth_rows=2, transform=None, prefix="Bag"):
    """Muro de sacos a lo largo de +X desde x=0 (longitud en cm). Devuelve las piezas creadas."""
    pieces = []
    bag_l, bag_d, bag_h = 60.0, 29.0, 12.8
    for layer in range(rows):
        z = layer * bag_h * 0.98 + bag_h / 2
        inset = layer * 0.9                       # el muro se estrecha hacia arriba (talud)
        offset = 0.0 if layer % 2 == 0 else bag_l / 2
        for row in range(depth_rows):
            y = (row - (depth_rows - 1) / 2) * (bag_d - 2.0)
            y += (-inset if row == 0 else inset) * 0.5
            x = -offset
            k = 0
            while x < length - 1:
                x0, x1 = max(x, 0.0), min(x + bag_l, length)
                if x1 - x0 > 12:
                    cx = (x0 + x1) / 2 + rng.uniform(-1.5, 1.5)
                    sx = (x1 - x0) - 2.0 + rng.uniform(-2, 1)
                    sz = bag_h * rng.uniform(0.92, 1.08)
                    p = kit.rounded_box(f"{prefix}{layer}_{row}_{k}", cx, y + rng.uniform(-1.5, 1.5), z, sx, bag_d * rng.uniform(0.94, 1.02), sz,
                                        radius=5.5, segments=3)
                    m = Matrix.Translation((cx, y, z)) @ Matrix.Rotation(math.radians(rng.uniform(-2.5, 2.5)), 4, 'Z') \
                        @ Matrix.Rotation(math.radians(rng.uniform(-2, 2)), 4, 'X') @ Matrix.Translation((-cx, -y, -z))
                    if transform is not None:
                        m = transform @ m
                    kit.transform(p, m)
                    pieces.append(p)
                x += bag_l
                k += 1
    return pieces


def sandbag_straight():
    kit = Kit("SM_Cover_Sandbag_Straight", [M_FABRIC])
    rng = random.Random(11)
    sandbag_run(kit, rng, 120.0, transform=Matrix.Translation((-60, 0, 0)))
    kit.collision_box(-60, 60, -28, 28, 0, 115)
    save(kit)


def sandbag_corner():
    kit = Kit("SM_Cover_Sandbag_Corner", [M_FABRIC])
    rng = random.Random(12)
    # tramo a lo largo de X y otro a lo largo de Y que se cruzan en la esquina (0, 0)
    sandbag_run(kit, rng, 120.0, transform=Matrix.Translation((-28, 0, 0)), prefix="BagA")
    sandbag_run(kit, rng, 92.0, transform=Matrix.Translation((0, 28, 0)) @ Matrix.Rotation(math.radians(90), 4, 'Z'), prefix="BagB")
    kit.collision_box(-28, 92, -28, 28, 0, 115)
    kit.collision_box(-28, 28, 28, 120, 0, 115)
    save(kit)


# ---------------------------------------------------------------------------
# Barrera jersey (perfil F de 107 cm) con huecos para la carretilla y argollas
# ---------------------------------------------------------------------------

JERSEY = [(-30.5, 0), (30.5, 0), (30.5, 7.5), (18.0, 33.0), (7.5, 107.0), (-7.5, 107.0), (-18.0, 33.0), (-30.5, 7.5)]


def jersey():
    kit = Kit("SM_Cover_Jersey", [M_CONCRETE, M_CHROME])
    j = kit.prism("Body", JERSEY, 'x', -150, 150, 0, bevel=0.012, segments=2)
    kit.cut(j, *[kit.box(f"Fork{k}", x - 15, x + 15, -40, 40, -1, 8, keep=False) for k, x in enumerate((-90, 90))],
            kit.box("Joint", -151, -146, -10, 10, 20, 90, keep=False))
    for k, x in enumerate((-110, 110)):  # argollas de izado
        kit.lathe(f"Eye{k}", [(0, 3.0), (2.2, 3.0), (2.2, 1.6), (0, 1.6)], (x, 0, 107), (0, 0, 1), seg=12, closed=True, mat=1)
    kit.collision_hull(JERSEY, -150, 150)
    save(kit)


# ---------------------------------------------------------------------------
# T-wall: muro de hormigón en T invertida (cobertura alta), 370 cm
# ---------------------------------------------------------------------------

def twall():
    kit = Kit("SM_Cover_TWall", [M_CONCRETE, M_CHROME])
    kit.box("Base", -75, 75, -60, 60, 0, 32, 0, bevel=0.015)
    kit.prism("Haunch", [(-60, 32), (60, 32), (16, 52), (-16, 52)], 'x', -75, 75, 0, bevel=0.01)
    kit.prism("Wall", [(-14, 50), (14, 50), (10, 370), (-10, 370)], 'x', -75, 75, 0, bevel=0.012)
    kit.lathe("Lift", [(0, 5), (4, 5), (4, 3), (0, 3)], (0, 0, 370), (1, 0, 0), seg=12, closed=True, mat=1)
    kit.collision_box(-75, 75, -60, 60, 0, 50)
    kit.collision_box(-75, 75, -14, 14, 50, 372)
    save(kit)


# ---------------------------------------------------------------------------
# Contenedor ISO de 20 pies: chapa corrugada, postes, carriles, puertas con barras de cierre
# ---------------------------------------------------------------------------

CL, CW, CH = 606.0, 244.0, 259.0


def corrugated(kit, name, length, height, z0, axis_len, mat, pitch=27.8, depth=3.6, thick=0.6):
    """Panel corrugado vertical: perfil (a lo largo, profundidad) extruido en Z. axis_len 'x' o 'y'."""
    n = int(length / pitch)
    pitch = length / n
    outer, inner = [], []
    for i in range(n):
        a = i * pitch
        for t, d in ((0.0, 0.0), (0.2, depth), (0.5, depth), (0.7, 0.0)):
            outer.append((a + t * pitch - length / 2, d))
    outer.append((length / 2, 0.0))
    inner = [(p, d - thick) for p, d in outer]
    poly = outer + list(reversed(inner))
    if axis_len == 'x':
        pts = poly
    else:
        pts = [(d, p) for p, d in poly]
    return kit.prism(name, pts, 'z', z0, z0 + height, mat, bevel=0.0)


def container(open_doors=False):
    name = "SM_Container_20ft_Open" if open_doors else "SM_Container_20ft"
    kit = Kit(name, [M_STEEL, M_WOOD, M_RUBBER])
    hx, hy = CL / 2, CW / 2
    post = 15.0
    # Postes de esquina y carriles superior/inferior
    for sx in (-1, 1):
        for sy in (-1, 1):
            kit.box(f"Post{sx}{sy}", sx * hx - (post if sx > 0 else 0), sx * hx + (0 if sx > 0 else post),
                    sy * hy - (post if sy > 0 else 0), sy * hy + (0 if sy > 0 else post), 0, CH, 0, bevel=0.006)
            # piezas de esquina (cantoneras)
            for z0 in (0, CH - 12):
                kit.box(f"Cast{sx}{sy}{z0}", sx * hx - (18 if sx > 0 else 0), sx * hx + (0 if sx > 0 else 18),
                        sy * hy - (16 if sy > 0 else 0), sy * hy + (0 if sy > 0 else 16), z0, z0 + 12, 0, bevel=0.004)
    for sy in (-1, 1):
        y0, y1 = (hy - 12, hy) if sy > 0 else (-hy, -hy + 12)
        kit.box(f"RailBot{sy}", -hx + post, hx - post, y0, y1, 0, 16, 0, bevel=0.004)
        kit.box(f"RailTop{sy}", -hx + post, hx - post, y0 + (2 if sy > 0 else 0), y1 - (0 if sy > 0 else -2), CH - 10, CH, 0, bevel=0.004)
    for sx in (-1, 1):
        x0, x1 = (hx - 12, hx) if sx > 0 else (-hx, -hx + 12)
        kit.box(f"EndBot{sx}", x0, x1, -hy + post, hy - post, 0, 20, 0, bevel=0.004)
        kit.box(f"EndTop{sx}", x0, x1, -hy + post, hy - post, CH - 22, CH, 0, bevel=0.004)
    # Paredes corrugadas laterales, trasera y techo
    for sy in (-1, 1):
        w = corrugated(kit, f"Side{sy}", CL - 2 * post, CH - 26, 16, 'x', 0)
        kit.transform(w, Matrix.Translation((0, sy * (hy - 4), 0)) @ (Matrix.Rotation(math.pi, 4, 'Z') if sy < 0 else Matrix()))
    back = corrugated(kit, "Back", CW - 2 * post, CH - 42, 20, 'y', 0)
    kit.transform(back, Matrix.Translation((-hx + 4, 0, 0)) @ Matrix.Rotation(math.pi, 4, 'Z'))
    kit.box("Roof", -hx + post, hx - post, -hy + 10, hy - 10, CH - 6, CH - 3, 0, bevel=0.0)
    kit.box("Floor", -hx + 10, hx - 10, -hy + 10, hy - 10, 12, 16, 1, bevel=0.0)
    # Puertas (extremo +X): dos hojas con nervios, barras de cierre y bisagras
    door_w = hy - post
    for side in (-1, 1):
        hinge_y = side * (hy - post)
        if open_doors:
            # abiertas 260°: pegadas al lateral
            m = Matrix.Translation((hx, hinge_y, 0)) @ Matrix.Rotation(math.radians(-side * 260), 4, 'Z') @ Matrix.Translation((-hx, -hinge_y, 0))
        else:
            m = Matrix()
        y_in = hinge_y - side * door_w
        y0, y1 = min(hinge_y, y_in), max(hinge_y, y_in)
        parts = [kit.box(f"Door{side}", hx - 3, hx, y0 + 0.5, y1 - 0.5, 20, CH - 22, 0, bevel=0.003)]
        for k in range(4):   # nervios horizontales de la puerta
            z = 45 + k * 55
            parts.append(kit.box(f"DoorRib{side}{k}", hx, hx + 2.5, y0 + 4, y1 - 4, z, z + 6, 0, bevel=0.003))
        for k, frac in enumerate((0.3, 0.75)):  # barras de cierre verticales
            yb = y0 + (y1 - y0) * frac
            parts.append(kit.lathe(f"Bar{side}{k}", [(18, 1.6), (CH - 18, 1.6)], (hx + 5, yb, 0), (0, 0, 1), seg=8, mat=0))
            for z in (40, CH - 40):
                parts.append(kit.box(f"BarClip{side}{k}{z}", hx, hx + 6, yb - 3, yb + 3, z - 3, z + 3, 0, bevel=0.002))
        for z in (40, 120, 200):  # bisagras
            parts.append(kit.box(f"Hinge{side}{z}", hx - 1, hx + 4, hinge_y - side * 6 - 3, hinge_y - side * 6 + 3, z, z + 10, 0, bevel=0.002))
        for p in parts:
            kit.transform(p, m)
    if open_doors:
        kit.collision_box(-hx, hx, -hy, -hy + 6, 0, CH)       # laterales
        kit.collision_box(-hx, hx, hy - 6, hy, 0, CH)
        kit.collision_box(-hx, -hx + 6, -hy, hy, 0, CH)       # fondo
        kit.collision_box(-hx, hx, -hy, hy, CH - 8, CH)       # techo
        kit.collision_box(-hx, hx, -hy, hy, 0, 16)            # suelo
        kit.collision_box(hx - door_w - 2, hx + 2, hy + 1, hy + 8, 18, CH - 20)     # puertas abiertas contra los laterales
        kit.collision_box(hx - door_w - 2, hx + 2, -hy - 8, -hy - 1, 18, CH - 20)
    else:
        kit.collision_box(-hx, hx + 6, -hy, hy, 0, CH)
    save(kit)


# ---------------------------------------------------------------------------
# Furgón civil (tipo furgoneta grande): carrocería por perfil lateral, pasos de rueda, cristales, ruedas
# ---------------------------------------------------------------------------

VAN_PROFILE = [(-265, 36), (-265, 222), (-258, 228), (128, 228), (150, 222), (212, 136), (248, 120), (262, 104),
               (266, 72), (266, 42), (258, 34)]


def wheel(kit, name, x, y, z, radius, width, side, tire=True):
    """Rueda con neumático (goma) y llanta de acero; side = +1 lado derecho (+Y), -1 izquierdo."""
    d = (0, side, 0)
    if tire:
        kit.lathe(name + "Tire", [(-width / 2, radius * 0.62), (-width / 2, radius * 0.92), (-width / 2 + 3, radius), (width / 2 - 3, radius),
                                  (width / 2, radius * 0.92), (width / 2, radius * 0.62)], (x, y, z), d, seg=28, mat=2 if name.startswith("Van") else 0)
    kit.lathe(name + "Rim", [(-width / 2 + 2, radius * 0.63), (width / 2 - 1, radius * 0.63), (width / 2 - 1, radius * 0.25), (width / 2 + 1, radius * 0.18)],
              (x, y, z), d, seg=20, mat=3 if name.startswith("Van") else 0)


def van(open_doors=False):
    name = "SM_Veh_Van_Civil_Open" if open_doors else "SM_Veh_Van_Civil"
    kit = Kit(name, [M_PAINT, M_GLASS, M_RUBBER, M_CHROME, M_TRIM])
    hw = 100.0
    body = kit.prism("Body", VAN_PROFILE, 'y', -hw, hw, 0, bevel=0.06, segments=4)
    cutters = []
    for k, wx in enumerate((-170, 178)):   # pasos de rueda
        cutters.append(kit.lathe(f"Arch{k}", [(-hw - 5, 42), (hw + 5, 42)], (wx, 0, 38), (0, 1, 0), seg=24, keep=False))
    if open_doors:
        # caja de carga hueca y abierta por detrás
        cutters.append(kit.box("Cargo", -268, 95, -hw + 4, hw - 4, 40, 222, keep=False))
    kit.cut(body, *cutters)
    # Cristales (opacos oscuros, ligeramente salientes): parabrisas, ventanillas de cabina, traseras
    kit.prism("Windshield", [(150, 222), (212, 137), (215, 139), (153, 224)], 'y', -hw + 10, hw - 10, 1, bevel=0.0)
    for sy in (-1, 1):
        y0 = sy * (hw + 0.4)
        kit.prism(f"CabWin{sy}", [(105, 150), (190, 150), (150, 214), (105, 214)], 'y', min(y0, y0 - sy * 1.2), max(y0, y0 - sy * 1.2), 1, bevel=0.0)
        # líneas de puertas (ranuras finas): delantera y corredera
        for gx in (100, 210):
            kit.cut(body, kit.box(f"Gap{sy}{gx}", gx - 0.6, gx + 0.6, sy * hw - 2, sy * hw + 2, 45, 215, keep=False))
        kit.box(f"Handle{sy}", 92, 104, sy * (hw + 0.5) - 1, sy * (hw + 0.5) + 1, 150, 155, 4, bevel=0.002)
        my0, my1 = (hw + 2, hw + 16) if sy > 0 else (-hw - 16, -hw - 2)
        kit.box(f"Mirror{sy}", 178, 186, my0, my1, 160, 182, 4, bevel=0.01)
        kit.box(f"SideMolding{sy}", -255, 245, sy * (hw + 0.6) - 1.2, sy * (hw + 0.6) + 1.2, 70, 80, 4, bevel=0.004)
    if not open_doors:
        kit.prism("RearWin", [(-50, 150), (50, 150), (50, 210), (-50, 210)], 'x', -266.2, -265.0, 1, bevel=0.0)
    # Paragolpes, rejilla, faros, matrícula
    kit.box("BumperF", 255, 272, -hw + 2, hw - 2, 38, 62, 4, bevel=0.02)
    kit.box("BumperR", -272, -258, -hw + 2, hw - 2, 38, 58, 4, bevel=0.02)
    kit.box("Grille", 262, 268, -60, 60, 74, 100, 4, bevel=0.01)
    for sy in (-1, 1):
        kit.box(f"Headlight{sy}", 255, 266, sy * 64 - 16, sy * 64 + 16, 92, 108, 3, bevel=0.01)
        kit.box(f"Taillight{sy}", -268, -264, sy * 90 - 6, sy * 90 + 6, 120, 175, 1, bevel=0.004)
    kit.box("PlateF", 271, 273, -26, 26, 44, 56, 4, bevel=0.0)
    # Ruedas
    for wx in (-170, 178):
        for sy in (-1, 1):
            wheel(kit, f"VanW{wx}{sy}", wx, sy * (hw - 14), 36, 36, 23, sy)
    # Puertas traseras abiertas (hoja de 100 cm abierta 100°) o cerradas (con su cristal)
    if open_doors:
        kit.box("CargoFloor", -262, 95, -hw + 4, hw - 4, 40, 44, 4, bevel=0.0)
        for side in (-1, 1):
            hinge = (-265, side * (hw - 2))
            leaf = kit.box(f"RearDoor{side}", -269, -265, min(side * (hw - 2), 0), max(side * (hw - 2), 0), 40, 222, 0, bevel=0.01)
            win = kit.box(f"RearDoorWin{side}", -270, -269, min(side * 50, side * 10), max(side * 50, side * 10), 150, 208, 1, bevel=0.0)
            m = Matrix.Translation((hinge[0], hinge[1], 0)) @ Matrix.Rotation(math.radians(side * 100), 4, 'Z') @ Matrix.Translation((-hinge[0], -hinge[1], 0))
            kit.transform(leaf, m)
            kit.transform(win, m)
        kit.collision_box(-265, 266, -hw, -hw + 5, 30, 228)
        kit.collision_box(-265, 266, hw - 5, hw, 30, 228)
        kit.collision_box(95, 272, -hw, hw, 30, 228)
        kit.collision_box(-265, 95, -hw, hw, 30, 44)
        kit.collision_box(-265, 95, -hw, hw, 222, 228)
    else:
        kit.collision_box(-272, 272, -hw, hw, 30, 228)
    save(kit)


# ---------------------------------------------------------------------------
# Turismo calcinado: carrocería baja sobre las llantas, habitáculo hueco sin cristales
# ---------------------------------------------------------------------------

SEDAN_PROFILE = [(-225, 32), (-228, 70), (-215, 96), (-160, 102), (-95, 135), (40, 138), (95, 104), (210, 92), (226, 72),
                 (224, 38), (210, 30)]


def sedan_wreck():
    kit = Kit("SM_Veh_Sedan_Wreck", [M_BURNT])
    hw = 88.0
    rng = random.Random(5)
    body = kit.prism("Body", SEDAN_PROFILE, 'y', -hw, hw, 0, bevel=0.05, segments=3)
    cutters = [kit.lathe(f"Arch{k}", [(-hw - 5, 36), (hw + 5, 36)], (wx, 0, 28), (0, 1, 0), seg=20, keep=False) for k, wx in enumerate((-140, 140))]
    # habitáculo vacío y huecos de ventanillas / parabrisas
    cutters.append(kit.box("Cabin", -150, 80, -hw + 5, hw - 5, 45, 133, keep=False))
    cutters.append(kit.prism("WinSides", [(-90, 104), (35, 104), (35, 128), (-85, 128)], 'y', -hw - 5, hw + 5, keep=False))
    cutters.append(kit.prism("Windshield", [(42, 132), (92, 106), (82, 104), (35, 130)], 'y', -hw + 10, hw - 10, keep=False))
    cutters.append(kit.prism("RearScreen", [(-150, 104), (-100, 130), (-93, 132), (-142, 106)], 'y', -hw + 10, hw - 10, keep=False))
    kit.cut(body, *cutters)
    # pilares que quedan (A, B, C) ya forman parte del cuerpo; restos: asientos calcinados y llantas sin neumático
    for k, x in enumerate((-95, 0)):
        kit.box(f"SeatFrame{k}", x - 25, x + 25, -60, 60, 45, 60, 0, bevel=0.01)
        kit.box(f"SeatBack{k}", x - 28, x - 22, -60, 60, 60, 105, 0, bevel=0.01)
    # Sin neumáticos la carrocería cae ~10 cm y descansa sobre las llantas; ligeramente torcida
    sink = Matrix.Rotation(math.radians(1.5), 4, 'X') @ Matrix.Rotation(math.radians(-0.8), 4, 'Y') @ Matrix.Translation((0, 0, -10))
    for p in kit.parts:
        kit.transform(p, sink)
    for wx in (-140, 140):
        for sy in (-1, 1):
            wheel(kit, f"SedW{wx}{sy}", wx, sy * (hw - 14), 20, 20, 17, sy, tire=False)
    kit.collision_box(-228, 228, -hw, hw, 0, 88)
    kit.collision_box(-160, 95, -hw + 5, hw - 5, 88, 128)
    save(kit)


# ---------------------------------------------------------------------------
# Objetivo de la misión (Bloque 6): portátil rugerizado y disco duro de Varek
# ---------------------------------------------------------------------------

M_PLASTIC = ("MI_Obj_Plastic", (0.05, 0.05, 0.05, 1), 0.6, 0.0)
M_ALU = ("MI_Obj_Aluminium", (0.55, 0.56, 0.58, 1), 0.35, 1.0)
M_SCREEN = ("MI_Obj_Screen", (0.02, 0.05, 0.06, 1), 0.1, 0.0)
M_LED = ("MI_Obj_LED", (0.1, 1.0, 0.3, 1), 0.3, 0.0)


def laptop():
    """Portátil rugerizado abierto (35 x 26 cm): base con esquinas de goma, pantalla a 105° con imagen tenue."""
    kit = Kit("SM_Obj_Laptop_Rugged", [M_PLASTIC, M_SCREEN, M_RUBBER])
    kit.box("Base", -17.5, 17.5, -13, 13, 0, 3.2, 0, bevel=0.004)
    kit.box("Keyboard", -14.5, 14.5, -11, 2, 3.2, 3.5, 2, bevel=0.001)
    kit.box("Touchpad", -4, 4, 3.5, 10, 3.2, 3.4, 2, bevel=0.0005)
    for sx in (-1, 1):
        for sy in (-1, 1):
            kit.box(f"Bumper{sx}{sy}", sx * 17.5 - 2.2, sx * 17.5 + 2.2, sy * 13 - 2.2, sy * 13 + 2.2, -0.2, 3.6, 2, bevel=0.008)
    lid = kit.box("Lid", -17.5, 17.5, -13.5, -12.0, 0, 25, 0, bevel=0.004)
    screen = kit.box("Screen", -15, 15, -12.05, -11.9, 2.5, 22.5, 1, bevel=0.0)
    hinge = Matrix.Translation((0, -13, 3.2)) @ Matrix.Rotation(math.radians(-15), 4, 'X') @ Matrix.Translation((0, 13, -3.2))
    for p in (lid, screen):
        kit.transform(p, hinge)
    kit.collision_box(-18, 18, -14, 14, 0, 4)
    save(kit)


def hard_drive():
    """Disco duro externo (15 x 10 x 2 cm), carcasa de aluminio con LED verde: se ve aunque esté en penumbra."""
    kit = Kit("SM_Obj_HardDrive", [M_ALU, M_PLASTIC, M_LED])
    kit.box("Shell", -7.5, 7.5, -5, 5, 0, 2.0, 0, bevel=0.004, segments=3)
    kit.box("Bumper", -7.8, 7.8, -5.3, 5.3, 0.6, 1.4, 1, bevel=0.003)
    kit.box("Port", 7.5, 7.9, -1.2, 1.2, 0.6, 1.4, 1, bevel=0.0005)
    kit.box("LED", 5.5, 6.5, -4.0, -3.6, 2.0, 2.15, 2, bevel=0.0)
    kit.collision_box(-8, 8, -5.5, 5.5, 0, 2.2)
    save(kit)


def preview(obj, path):
    scene = bpy.context.scene
    scene.render.engine = 'BLENDER_WORKBENCH'
    scene.display.shading.light = 'STUDIO'
    scene.display.shading.color_type = 'MATERIAL'
    scene.display.shading.show_cavity = True
    scene.render.resolution_x, scene.render.resolution_y = 1200, 800
    scene.world = scene.world or bpy.data.worlds.new("W")
    scene.world.color = (0.55, 0.57, 0.6)
    cam_data = bpy.data.cameras.new("PrevCam")
    cam_data.type = 'ORTHO'
    dims = obj.dimensions
    cam_data.ortho_scale = max(dims.x, dims.y, dims.z) * 1.35
    cam = bpy.data.objects.new("PrevCam", cam_data)
    scene.collection.objects.link(cam)
    scene.camera = cam
    center = Vector((0, 0, dims.z / 2))
    d = Vector((0.8, -0.65, 0.45)).normalized()
    cam.location = center + d * (max(dims) * 3)
    cam.rotation_euler = (-d).to_track_quat('-Z', 'Y').to_euler()
    for o in bpy.context.scene.objects:
        if o.name.startswith("UCX_"):
            o.hide_render = True
    scene.render.filepath = path
    bpy.ops.render.render(write_still=True)
    bpy.data.objects.remove(cam)


BUILDERS = {
    "SM_Cover_Sandbag_Straight": sandbag_straight,
    "SM_Cover_Sandbag_Corner": sandbag_corner,
    "SM_Cover_Jersey": jersey,
    "SM_Cover_TWall": twall,
    "SM_Container_20ft": lambda: container(False),
    "SM_Container_20ft_Open": lambda: container(True),
    "SM_Veh_Van_Civil": lambda: van(False),
    "SM_Veh_Van_Civil_Open": lambda: van(True),
    "SM_Veh_Sedan_Wreck": sedan_wreck,
    "SM_Obj_Laptop_Rugged": laptop,
    "SM_Obj_HardDrive": hard_drive,
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
