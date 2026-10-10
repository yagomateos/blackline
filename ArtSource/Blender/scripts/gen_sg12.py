"""SG-12 "Mastín": escopeta de corredera del calibre 12 (malla esquelética) para BLACKLINE, arma de CQB.

Uso:
  blender -b --factory-startup -P gen_sg12.py -- <salida.fbx> [preview.png] [--bake]

Mismas convenciones que gen_ar7.py (reutiliza sus primitivas): cm, +Y adelante, +Z arriba, -X = lado derecho
(ventana de expulsión), +X = lado izquierdo (el que ve el jugador). Origen = parte superior del pistolete, con el
mismo pistolete que el AR-7: la mano derecha del Mannequin agarra las dos armas igual.

Escopeta de servicio con culata de pistolete sintética: cajón de acero fresado con ventana de expulsión a la
derecha y portilla de carga abajo (se ve el elevador), cañón de 47 cm, depósito tubular de 7 cartuchos con
abrazadera y tapón, guardamanos con estrías que corre sobre el depósito con sus dos barras de acción, cerrojo
visible por la ventana, mira trasera de anillo fantasma con orejas y delantera de fibra óptica sobre rampa,
portacartuchos de 4 en el lado izquierdo, botón de seguro transversal, retén de la corredera, cantonera de goma.

Huesos:
  root
   ├─ pump (guardamanos + barras de acción + cerrojo: retrocede PUMP_TRAVEL al bombear y queda atrás sin cartuchos)
   ├─ trigger
   └─ Sight (centro del anillo trasero), Muzzle, Eject, HandGrip_L (palma bajo el guardamanos), LoadPort (portilla)
Materiales: MI_SG12_Metal, MI_SG12_Polymer, MI_SG12_Rubber, MI_SG12_Fiber, MI_SG12_Hull (cartuchos), MI_SG12_Brass.
También exporta SM_Shell_12ga (cartucho suelto: mano en la recarga) y SM_Hull_12ga (vaina vacía expulsada).
"""
import json
import math
import os
import sys

sys.path.append(os.path.dirname(os.path.abspath(__file__)))
import bmesh
import bpy
from mathutils import Vector
import bl_lib
import gen_ar7 as K

BORE_Z = 6.6        # eje del cañón
REC_TOP = 9.0       # parte superior del cajón
REC_REAR, REC_FRONT = -6.0, 19.0
REC_HW = 1.55       # cajón de 31 mm
SIGHT_Z = 10.4      # centro del anillo trasero = punta de la fibra delantera
TUBE_Z = 3.4        # eje del depósito tubular
MUZZLE_Y = 66.0
PUMP_Y0, PUMP_Y1 = 29.0, 49.0   # guardamanos en reposo
PUMP_TRAVEL = 9.0

METAL, POLY, RUBBER, FIBER, HULL, BRASS = range(6)
K.W = lambda x, y, z: Vector((x / 100.0, -y / 100.0, z / 100.0))
K.PARTS = []
PARTS = K.PARTS
W = K.W


# ---------------------------------------------------------------------------
# Geometría
# ---------------------------------------------------------------------------

def build_receiver():
    # sección con los cantos superiores achaflanados (como un cajón fresado real)
    sec = [(-REC_HW, 1.0), (REC_HW, 1.0), (REC_HW, 7.9), (REC_HW - 0.45, REC_TOP), (-REC_HW + 0.45, REC_TOP), (-REC_HW, 7.9)]
    rec = K.prism("Receiver", sec, 'y', REC_REAR, REC_FRONT, METAL, bevel=0.0018, segments=2)
    cutters = [
        # ventana de expulsión (derecha) hasta el eje: se ve el cerrojo y la recámara
        K.box("PortCut", -REC_HW - 0.3, 0.2, 5.0, 13.6, 4.7, 8.35, keep=False),
        # portilla de carga (abajo): se ve el elevador
        K.box("LoadPortCut", -1.18, 1.18, 3.4, 14.6, 0.5, 2.25, keep=False),
        # rebajes decorativos de los lados (fresado del cajón)
        K.side("FlatL", [(-3.5, 3.0), (3.5, 3.0), (3.5, 7.2), (-3.5, 7.2)], 0.1, x_center=REC_HW, keep=False),
        K.side("FlatR", [(-3.5, 3.0), (3.0, 3.0), (3.0, 7.2), (-3.5, 7.2)], 0.1, x_center=-REC_HW, keep=False),
    ]
    K.cut(rec, *cutters)
    # Elevador visible por la portilla
    K.box("Carrier", -0.95, 0.95, 4.0, 13.9, 2.25, 2.6, METAL, bevel=0.0004, segments=1)
    # pasadores del grupo del gatillo (cabeza a la izquierda)
    for k, y in enumerate((-2.8, 13.2)):
        K.lathe(f"TrigPin{k}", [(-REC_HW - 0.04, 0.24), (REC_HW + 0.04, 0.24), (REC_HW + 0.08, 0.18)],
                origin=(0, y, 1.8), direction=(1, 0, 0), seg=10, mat=METAL)
    # Guardamonte (pieza del grupo del gatillo) con seguro transversal y retén de la corredera
    tg = K.side("TriggerPlate", [(-5.6, 1.05), (3.3, 1.05), (3.3, 0.3), (2.4, -0.6),
                                 (-4.4, -0.6), (-5.6, 0.2)], 1.25, mat=POLY, bevel=0.0015, segments=2)
    K.cut(tg, K.box("TrigSlot", -0.4, 0.4, 0.9, 2.1, -0.8, 1.2, keep=False))
    K.band("TriggerGuard", [(2.25, -0.4), (2.15, -3.0), (1.75, -3.95), (0.8, -4.3), (-0.6, -4.2), (-1.4, -3.75), (-1.55, -3.0)],
           0.65, 0.6, mat=POLY, bevel=0.0015, segments=2)
    K.side("Trigger", [(1.15, -0.4), (1.75, -0.4), (1.72, -1.6), (1.5, -2.6), (1.05, -3.25), (0.8, -3.15), (1.12, -2.4),
                       (1.22, -1.5)], 0.3, mat=METAL, bone="trigger", bevel=0.0006, segments=1)
    K.lathe("Safety", [(-1.45, 0.42), (1.5, 0.42), (1.62, 0.36)], origin=(0, 2.9, 0.2), direction=(1, 0, 0), seg=14,
            mat=METAL, bevel=0.0003)
    K.side("ActionLock", [(-0.6, -0.15), (0.2, -0.15), (0.35, -1.3), (-0.3, -1.6), (-0.6, -0.9)], 0.12, x_center=1.32,
           mat=METAL, bevel=0.0003, segments=1)


def build_barrel():
    K.lathe("Barrel", [(REC_FRONT - 1.0, 0.93), (REC_FRONT - 1.0, 1.08), (MUZZLE_Y - 0.4, 1.05), (MUZZLE_Y, 1.0),
                       (MUZZLE_Y, 0.93)], origin=(0, 0, BORE_Z), seg=32, closed=True, mat=METAL)
    # Depósito tubular: tubo, tapón con moleteado y anilla para la correa
    K.lathe("MagTube", [(REC_FRONT - 1.0, 1.1), (59.6, 1.1)], origin=(0, 0, TUBE_Z), seg=24, mat=METAL)
    cap = K.lathe("MagCap", [(59.4, 1.28), (62.0, 1.28), (62.6, 1.05), (62.8, 0.5)], origin=(0, 0, TUBE_Z), seg=24,
                  mat=METAL, bevel=0.0004, segments=1)
    K.cut(cap, *[K.box(f"Knurl{k}", -0.08, 0.08, 59.6, 61.8, TUBE_Z + 1.12, TUBE_Z + 1.5, keep=False) for k in range(1)])
    K.lathe("SlingLoop", [(0.0, 0.62), (0.0, 0.82), (0.25, 0.82), (0.25, 0.62)], origin=(0, 61.0, TUBE_Z - 1.9),
            direction=(1, 0, 0), seg=16, closed=True, mat=METAL)
    # Abrazadera cañón-depósito
    clamp = K.prism("BarrelClamp", [(-1.35, TUBE_Z - 1.35), (1.35, TUBE_Z - 1.35), (1.35, BORE_Z + 0.7), (0.9, BORE_Z + 1.3),
                                    (-0.9, BORE_Z + 1.3), (-1.35, BORE_Z + 0.7)], 'y', 54.6, 56.8, METAL, bevel=0.0006, segments=1)
    K.cut(clamp, K.lathe("ClampBore", [(54.0, 1.06), (57.4, 1.06)], origin=(0, 0, BORE_Z), seg=24, keep=False),
          K.lathe("ClampTube", [(54.0, 1.11), (57.4, 1.11)], origin=(0, 0, TUBE_Z), seg=24, keep=False))
    K.lathe("ClampScrew", [(1.3, 0.3), (1.5, 0.3), (1.56, 0.24)], origin=(0, 55.7, 5.0), direction=(1, 0, 0), seg=10, mat=METAL)


def build_sights():
    # Trasera: anillo fantasma con dos orejas de protección sobre una base atornillada al cajón
    K.prism("RearBase", [(-1.15, REC_TOP - 0.05), (1.15, REC_TOP - 0.05), (1.0, 9.55), (-1.0, 9.55)], 'y', -4.6, 0.6,
            METAL, bevel=0.0006, segments=1)
    for s in (-1, 1):
        K.prism(f"RearEar{s}", [(s * 0.95, 9.5), (s * 1.15, 9.5), (s * 1.15, 11.2), (s * 1.0, 11.75), (s * 0.82, 11.75),
                                (s * 0.82, 9.5)], 'y', -3.6, -1.0, METAL, bevel=0.0004, segments=1)
    K.lathe("GhostRing", [(0.0, 0.32), (0.0, 0.62), (0.45, 0.62), (0.45, 0.32)], origin=(0, -2.5, SIGHT_Z), seg=24,
            closed=True, mat=METAL, bevel=0.0002, segments=1)
    K.box("RingPost", -0.18, 0.18, -2.5, -2.05, 9.5, SIGHT_Z - 0.6, METAL, bevel=0.0002, segments=1)
    # Delantera: rampa soldada al cañón y hoja con fibra óptica
    K.side("FrontRamp", [(60.4, BORE_Z + 0.9), (64.6, BORE_Z + 0.9), (64.6, BORE_Z + 1.9), (61.6, BORE_Z + 1.9)], 0.45,
           mat=METAL, bevel=0.0004, segments=1)
    K.side("FrontBlade", [(62.6, BORE_Z + 1.8), (64.4, BORE_Z + 1.8), (64.4, SIGHT_Z), (63.0, SIGHT_Z)], 0.38,
           mat=METAL, bevel=0.0003, segments=1)
    K.lathe("FiberRod", [(62.9, 0.13), (64.45, 0.13)], origin=(0, 0, SIGHT_Z - 0.2), seg=10, mat=FIBER)


def taper_y(obj, y_front, y_back, factor):
    """Estrecha el objeto en X hacia delante: factor por delante de y_front, ancho completo por detrás de y_back (cm)."""
    for v in obj.data.vertices:
        y = -v.co.y * 100.0
        a = min(max((y - y_back) / (y_front - y_back), 0.0), 1.0)
        a = a * a * (3 - 2 * a)
        v.co.x *= factor + (1.0 - factor) * (1.0 - a)


def build_stock():
    """Culata sintética de pistolete: el mismo pistolete del AR-7 unido a la culata por debajo del cajón."""
    grip = K.side("PistolGrip", [(-0.4, 1.0), (-0.4, -0.1), (-0.6, -1.2), (-0.95, -2.2), (-1.7, -3.0), (-1.65, -4.0),
                                 (-2.4, -6.5), (-3.5, -9.5), (-4.3, -11.5), (-4.8, -12.2), (-5.8, -12.5), (-7.9, -12.2),
                                 (-8.5, -11.5), (-7.7, -8.0), (-6.5, -4.2), (-6.4, -2.6), (-7.0, -1.6), (-7.6, 1.0)],
                  1.45, mat=POLY, bevel=0.0055, segments=3)
    panel = [(-2.3, -4.2), (-3.4, -8.2), (-5.6, -8.2), (-5.0, -4.2)]
    K.cut(grip, K.side("GripPanelL", panel, 0.2, x_center=1.5, keep=False),
          K.side("GripPanelR", panel, 0.2, x_center=-1.5, keep=False))
    # culata maciza: lomo casi recto a la altura del cajón (la mejilla apoya con la cabeza erguida), quilla que baja
    # desde el pistolete hasta la cantonera; más fina por delante
    stock = K.side("Stock", [(REC_REAR + 0.2, REC_TOP - 0.2), (-12.0, REC_TOP - 0.1), (-34.8, 8.4), (-35.2, 7.9),
                             (-35.2, -5.4), (-34.6, -6.0), (-31.0, -6.0), (-16.0, -2.9), (-9.0, -1.4), (-7.0, -1.6),
                             (-6.4, -2.6), (-6.5, 1.0), (REC_REAR + 0.2, 1.0)],
                   1.85, mat=POLY, bevel=0.005, segments=3)
    taper_y(stock, -6.0, -16.0, 0.82)
    K.cut(stock,
          K.side("StockPanelL", [(-15.0, 6.2), (-29.5, 5.8), (-29.5, 0.2), (-17.5, 1.6)], 0.15, x_center=1.85 * 0.97, keep=False),
          K.side("StockPanelR", [(-15.0, 6.2), (-29.5, 5.8), (-29.5, 0.2), (-17.5, 1.6)], 0.15, x_center=-1.85 * 0.97, keep=False),
          K.lathe("QDCup", [(1.4, 0.55), (2.2, 0.55)], origin=(0, -31.5, -4.0), direction=(1, 0, 0), seg=16, keep=False))
    K.lathe("QDRing", [(1.42, 0.55), (1.55, 0.55)], origin=(0, -31.5, -4.0), direction=(1, 0, 0), seg=16, mat=METAL)
    pad = K.side("ButtPad", [(-35.1, 8.35), (-37.4, 8.2), (-37.5, -5.7), (-35.1, -5.95)], 2.0, mat=RUBBER, bevel=0.004, segments=3)
    K.cut(pad, *[K.box(f"PadGroove{k}", -3, 3, -37.8, -37.25, -4.8 + k * 1.2, -4.8 + k * 1.2 + 0.4, keep=False) for k in range(11)])
    K.lathe("StockScrew", [(1.8, 0.3), (1.95, 0.3), (2.0, 0.24)], origin=(0, -8.5, 5.5), direction=(1, 0, 0), seg=10, mat=METAL)


def build_pump():
    """Guardamanos con estrías, barras de acción y cerrojo (todo en el hueso pump)."""
    sec = [(-2.0, 0.7), (2.0, 0.7), (2.35, 1.4), (2.35, 4.9), (1.75, 5.85), (-1.75, 5.85), (-2.35, 4.9), (-2.35, 1.4)]
    fore = K.prism("Forend", sec, 'y', PUMP_Y0, PUMP_Y1, POLY, "pump", bevel=0.004, segments=3)
    cutters = [K.lathe("ForendBore", [(PUMP_Y0 - 1, 1.14), (PUMP_Y1 + 1, 1.14)], origin=(0, 0, TUBE_Z), seg=20, keep=False)]
    # estrías transversales a los dos lados y por debajo (agarre)
    for k in range(9):
        y = PUMP_Y0 + 3.2 + k * 1.6
        for s in (-1, 1):
            cutters.append(K.box(f"Rib{k}{s}", s * 2.15, s * 2.6, y, y + 0.6, 1.6, 4.8, keep=False))
        cutters.append(K.box(f"RibB{k}", -1.6, 1.6, y, y + 0.6, 0.4, 0.9, keep=False))
    K.cut(fore, *cutters)
    # topes de los extremos (más anchos: la mano no resbala)
    for k, (y0, y1) in enumerate(((PUMP_Y0, PUMP_Y0 + 1.4), (PUMP_Y1 - 1.4, PUMP_Y1))):
        K.prism(f"ForendLip{k}", [(-2.15, 0.35), (2.15, 0.35), (2.5, 1.2), (2.5, 5.0), (1.9, 6.05), (-1.9, 6.05), (-2.5, 5.0),
                                  (-2.5, 1.2)], 'y', y0, y1, POLY, "pump", bevel=0.003, segments=2)
    # tubo interior del guardamanos (asoma por detrás) y barras de acción hasta el cerrojo
    K.lathe("ActionSleeve", [(REC_FRONT - 4.0, 1.13), (REC_FRONT - 4.0, 1.25), (PUMP_Y0 + 0.5, 1.25), (PUMP_Y0 + 0.5, 1.13)],
            origin=(0, 0, TUBE_Z), seg=24, closed=True, mat=METAL, bone="pump")
    for s in (-1, 1):
        K.box(f"ActionBar{s}", s * 1.12, s * 1.38, 6.0, PUMP_Y0 + 0.5, TUBE_Z - 0.25, TUBE_Z + 0.25, METAL, "pump",
              bevel=0.0002, segments=1)
    # cerrojo (se ve por la ventana de expulsión): cara con extractor
    bolt = K.box("Bolt", -1.25, 0.6, 5.1, 13.4, 4.85, 8.2, METAL, "pump", bevel=0.0005, segments=1)
    K.cut(bolt, K.box("BoltGroove", -1.4, -1.0, 6.0, 12.5, 6.2, 6.8, keep=False))
    K.box("Extractor", -1.3, -1.12, 12.6, 13.6, 6.9, 7.7, METAL, "pump", bevel=0.0002, segments=1)


def shell_parts(name, origin, axis, length=7.0, bone="root", spent=False):
    """Cartucho del 12 (2¾"): culote de latón con pestaña y cebo, vaina roja estriada, cierre en estrella."""
    o = origin
    K.lathe(name + "Base", [(0.0, 1.07), (0.12, 1.12), (0.2, 1.07), (0.25, 1.02), (1.25, 1.02), (1.3, 0.99)],
            origin=o, direction=axis, seg=20, mat=BRASS, bone=bone, bevel=0.0001, segments=1)
    K.lathe(name + "Primer", [(-0.02, 0.28), (0.04, 0.28)], origin=o, direction=axis, seg=12, mat=BRASS, bone=bone)
    tip = length - (0.5 if spent else 0.0)
    prof = [(1.25, 1.0), (tip - 0.25, 1.0), (tip, 0.9 if not spent else 1.04)]
    if not spent:
        prof.append((tip + 0.02, 0.35))
    K.lathe(name + "Hull", prof, origin=o, direction=axis, seg=20, mat=HULL, bone=bone)


def build_saddle():
    """Portacartuchos de 4 en el lado izquierdo (el que ve el jugador)."""
    plate = K.side("SaddlePlate", [(1.0, 1.2), (14.0, 1.2), (14.0, 8.6), (1.0, 8.6)], 0.14, x_center=REC_HW + 0.16,
                   mat=POLY, bevel=0.0008, segments=1)
    K.cut(plate, *[K.lathe(f"SaddleBolt{k}", [(REC_HW, 0.25), (REC_HW + 0.5, 0.25)], origin=(0, y, 7.6), direction=(1, 0, 0),
                           seg=8, keep=False) for k, y in enumerate((3.0, 12.0))])
    for k in range(4):
        y = 2.7 + k * 3.0
        shell_parts(f"SaddleShell{k}", (REC_HW + 1.45, y, 8.6), (0, 0, -1), length=7.0)
        # pinza elástica que sujeta cada cartucho
        K.box(f"SaddleClip{k}", REC_HW + 0.25, REC_HW + 2.6, y - 1.2, y + 1.2, 4.2, 5.4, POLY, bevel=0.0006, segments=1)


def build_geometry():
    build_receiver()
    build_barrel()
    build_sights()
    build_stock()
    build_pump()
    build_saddle()


BONES = {
    # nombre: (padre, posición en cm)
    "root": (None, (0, 0, 0)),
    "pump": ("root", (0, (PUMP_Y0 + PUMP_Y1) / 2, TUBE_Z)),
    "trigger": ("root", (0, 1.3, -0.4)),
    "Sight": ("root", (0, -2.5, SIGHT_Z)),
    "Muzzle": ("root", (0, MUZZLE_Y + 0.2, BORE_Z)),
    "Eject": ("root", (-1.8, 9.2, 6.6)),
    "HandGrip_L": ("root", (0, 38.5, -0.6)),   # palma bajo el guardamanos (6,6 cm bajo el eje del depósito... y del cañón 7,2)
    "LoadPort": ("root", (0, 9.0, 0.4)),       # portilla de carga: la mano mete los cartuchos aquí
}


def build_armature():
    arm_data = bpy.data.armatures.new("SG12_Rig")
    arm = bpy.data.objects.new("Armature", arm_data)
    bpy.context.scene.collection.objects.link(arm)
    bpy.context.view_layer.objects.active = arm
    bpy.ops.object.mode_set(mode='EDIT')
    for name, (parent, pos) in BONES.items():
        b = arm_data.edit_bones.new(name)
        b.head = W(*pos)
        b.tail = W(pos[0], pos[1], pos[2] + 1.5)
        if parent:
            b.parent = arm_data.edit_bones[parent]
    bpy.ops.object.mode_set(mode='OBJECT')
    return arm


def join_parts(name):
    for obj, bone in PARTS:
        vg = obj.vertex_groups.new(name=bone)
        vg.add(list(range(len(obj.data.vertices))), 1.0, 'REPLACE')
    bpy.ops.object.select_all(action='DESELECT')
    for obj, _ in PARTS:
        obj.select_set(True)
    mesh_obj = PARTS[0][0]
    bpy.context.view_layer.objects.active = mesh_obj
    bpy.ops.object.join()
    mesh_obj.name = mesh_obj.data.name = name
    bpy.ops.object.select_all(action='DESELECT')
    mesh_obj.select_set(True)
    bpy.context.view_layer.objects.active = mesh_obj
    bpy.ops.object.shade_smooth()
    wn = mesh_obj.modifiers.new("WeightedNormal", 'WEIGHTED_NORMAL')
    wn.keep_sharp = True
    tri = mesh_obj.modifiers.new("Triangulate", 'TRIANGULATE')
    tri.quad_method = 'BEAUTY'
    tri.ngon_method = 'BEAUTY'
    tri.keep_custom_normals = True
    bl_lib.apply_modifiers(mesh_obj)
    return mesh_obj


def export_shell(out_dir, name, spent):
    """Cartucho suelto (eje +X como las vainas de los demás calibres, origen en el culote)."""
    PARTS.clear()
    shell_parts("S", (0, 0, 0), (0, 1, 0), spent=spent)
    K.apply_bevels()
    obj = join_parts(name)
    obj.vertex_groups.clear()
    obj.rotation_euler = (0, 0, math.radians(90))   # +Y (Blender -Y) -> +X de Unreal
    bpy.ops.object.transform_apply(rotation=True)
    bpy.ops.object.mode_set(mode='EDIT')
    bpy.ops.mesh.select_all(action='SELECT')
    bpy.ops.uv.smart_project(angle_limit=math.radians(55), island_margin=0.01)
    bpy.ops.object.mode_set(mode='OBJECT')
    bl_lib.export_fbx([obj], os.path.join(out_dir, name + ".fbx"))
    obj.hide_render = True
    print(f"[gen_sg12] {name}: {bl_lib.triangle_count(obj)} tris")


def render_preview(obj, path):
    scene = bpy.context.scene
    scene.render.engine = 'BLENDER_WORKBENCH'
    scene.display.shading.light = 'STUDIO'
    scene.display.shading.color_type = 'MATERIAL'
    scene.display.shading.show_cavity = True
    scene.display.shading.cavity_type = 'BOTH'
    scene.render.resolution_x, scene.render.resolution_y = 1600, 900
    scene.world = bpy.data.worlds.new("W")
    scene.world.color = (0.55, 0.57, 0.6)
    cam_data = bpy.data.cameras.new("Cam")
    cam_data.type = 'ORTHO'
    cam = bpy.data.objects.new("Cam", cam_data)
    scene.collection.objects.link(cam)
    scene.camera = cam
    center = W(0, 14, 2)
    views = (("_right", Vector((-1, 0, 0.0)), 1.1, center), ("_left", Vector((1, 0, 0.0)), 1.1, center),
             ("_34", Vector((0.8, 0.55, 0.45)), 1.0, center),
             ("_detail", Vector((0.85, 0.35, 0.5)), 0.4, W(0, 6, 4)),
             ("_under", Vector((0.3, 0.2, -1.0)), 0.4, W(0, 9, 2)),
             ("_rear", Vector((0.0, 1.0, 0.04)), 0.12, W(0, 0, SIGHT_Z)))   # lo que ve el jugador en ADS
    for suffix, direction, ortho, c in views:
        d = direction.normalized()
        cam_data.ortho_scale = ortho
        cam.location = c + d * 2.0
        cam.rotation_euler = (-d).to_track_quat('-Z', 'Y').to_euler()
        scene.render.filepath = path.replace(".png", f"{suffix}.png")
        bpy.ops.render.render(write_still=True)
        print(f"[gen_sg12] Preview: {scene.render.filepath}")


def main():
    argv = sys.argv[sys.argv.index("--") + 1:] if "--" in sys.argv else []
    out = os.path.abspath(argv[0] if argv else os.path.join(os.path.dirname(__file__), "..", "export", "SK_SG12.fbx"))
    preview = next((a for a in argv[1:] if not a.startswith("--")), None)
    out_dir = os.path.dirname(out)
    os.makedirs(out_dir, exist_ok=True)

    bl_lib.reset_scene()
    K.MATERIALS = [
        bl_lib.get_material("MI_SG12_Metal", (0.035, 0.035, 0.037, 1), 0.5, 0.7),
        bl_lib.get_material("MI_SG12_Polymer", (0.04, 0.04, 0.04, 1), 0.7, 0.0),
        bl_lib.get_material("MI_SG12_Rubber", (0.015, 0.015, 0.015, 1), 0.9, 0.0),
        bl_lib.get_material("MI_SG12_Fiber", (1.0, 0.2, 0.05, 1), 0.3, 0.0),
        bl_lib.get_material("MI_SG12_Hull", (0.42, 0.04, 0.03, 1), 0.55, 0.0),
        bl_lib.get_material("MI_SG12_Brass", (0.78, 0.56, 0.24, 1), 0.3, 1.0),
    ]
    export_shell(out_dir, "SM_Shell_12ga", spent=False)
    export_shell(out_dir, "SM_Hull_12ga", spent=True)

    PARTS.clear()
    build_geometry()
    K.apply_bevels()
    mesh_obj = join_parts("SK_SG12")
    bpy.ops.object.mode_set(mode='EDIT')
    bpy.ops.mesh.select_all(action='SELECT')
    bpy.ops.uv.smart_project(angle_limit=math.radians(55), island_margin=0.004, area_weight=0.0)
    bpy.ops.uv.pack_islands(rotate=True, margin=0.003)
    bpy.ops.object.mode_set(mode='OBJECT')
    scale = K.uv_scale_cm(mesh_obj)
    print(f"[gen_sg12] UV: {scale:.1f} cm por unidad de UV")

    if "--bake" in argv:
        tex = os.path.join(out_dir, "T_SG12_Masks.png")
        K.bake_masks(mesh_obj, tex, size=2048)
        with open(tex.replace(".png", ".json"), "w") as f:
            json.dump({"uv_scale_cm": scale}, f)

    arm = build_armature()
    mesh_obj.parent = arm
    mod = mesh_obj.modifiers.new("Armature", 'ARMATURE')
    mod.object = arm
    print(f"[gen_sg12] SK_SG12: {bl_lib.triangle_count(mesh_obj)} tris, {len(BONES)} huesos")

    bpy.ops.object.select_all(action='DESELECT')
    arm.select_set(True)
    mesh_obj.select_set(True)
    bpy.context.view_layer.objects.active = arm
    bpy.ops.export_scene.fbx(
        filepath=out, use_selection=True, apply_unit_scale=True, apply_scale_options='FBX_SCALE_UNITS',
        axis_forward='-Y', axis_up='Z', object_types={'MESH', 'ARMATURE'}, mesh_smooth_type='FACE',
        use_tspace=True, add_leaf_bones=False, primary_bone_axis='Y', secondary_bone_axis='X',
        armature_nodetype='NULL', use_armature_deform_only=False, bake_anim=False)
    print(f"[gen_sg12] Exportado: {out}")

    if preview:
        render_preview(mesh_obj, preview)


if __name__ == "__main__":
    main()
