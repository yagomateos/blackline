"""P-17: pistola 9 mm de percutor lanzado (malla esquelética) para BLACKLINE, arma secundaria.

Uso:
  blender -b --factory-startup -P gen_p17.py -- <salida.fbx> [preview.png] [--bake]

Mismas convenciones que gen_ar7.py (reutiliza sus primitivas): cm, +Y adelante, +Z arriba, -X = lado derecho
(ventana de expulsión), +X = lado izquierdo. El diseño se escribe con la parte delantera-superior del puño en
y = 1,05 y al exportar se desplaza OY para que quede donde el puño del AR-7 (y = -0,4): así la mano derecha del
Mannequin (socket HandGrip_R = origen del arma) agarra las dos armas igual.

Huesos:
  root
   ├─ slide (corredera: retrocede en cada disparo y queda abierta con el cargador vacío) ─ slide_rear (estrías: la mano
   │        izquierda la agarra por encima para montarla en la recarga en vacío)
   ├─ trigger
   ├─ magazine (parte superior del cargador, eje del puño) ─ Mag (base del cargador)
   └─ Sight (borde superior de la muesca trasera = línea de mira), Muzzle, Eject, HandGrip_L (palma de la mano de apoyo)
Materiales: MI_P17_Slide (nitrurado negro), MI_P17_Frame (polímero), MI_P17_Steel (cañón, palancas, cargador),
MI_P17_Tritium (punto de la mira delantera).
También exporta SM_P17_Mag (cargador suelto) y SM_Casing_9mm (vaina 9x19).
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

OY = -1.45          # desplazamiento del diseño en Y (ver arriba)
BORE_Z = 2.75       # eje del cañón (bajo: típico de una pistola de percutor)
SLIDE_BOTTOM, SLIDE_TOP = 1.35, 4.15
SLIDE_REAR, SLIDE_FRONT = -4.25, 14.35
SLIDE_HW = 1.27     # corredera de 25,4 mm
SIGHT_Z = 5.0       # parte superior de la muesca trasera y del poste delantero

SLIDE, FRAME, STEEL, TRITIUM = range(4)


def W(x, y, z):
    """Coordenadas de diseño (cm) -> Blender (m), con el desplazamiento OY."""
    return Vector((x / 100.0, -(y + OY) / 100.0, z / 100.0))


# Las primitivas de gen_ar7 usan sus variables globales: se redirigen a las de este arma
K.W = W
K.PARTS = []
PARTS = K.PARTS

# ---------------------------------------------------------------------------
# Puño: marco local a lo largo del eje del cargador (17,7° hacia atrás)
# ---------------------------------------------------------------------------
GRIP_ANGLE = math.radians(17.7)
U = (math.sin(GRIP_ANGLE), math.cos(GRIP_ANGLE))      # hacia arriba por el puño (y, z)
N = (math.cos(GRIP_ANGLE), -math.sin(GRIP_ANGLE))     # hacia delante, perpendicular
G0 = (-4.2, -10.3)                                    # centro de la base del puño


def P(s, t):
    """Punto del puño: s cm hacia arriba por el eje, t cm hacia delante."""
    return (G0[0] + s * U[0] + t * N[0], G0[1] + s * U[1] + t * N[1])


def grip_quad(s0, s1, t0, t1):
    return [P(s0, t0), P(s0, t1), P(s1, t1), P(s1, t0)]


# ---------------------------------------------------------------------------
# Geometría
# ---------------------------------------------------------------------------

def build_slide():
    sec = [(-SLIDE_HW, SLIDE_BOTTOM), (SLIDE_HW, SLIDE_BOTTOM), (SLIDE_HW, 3.55), (0.95, SLIDE_TOP), (-0.95, SLIDE_TOP), (-SLIDE_HW, 3.55)]
    slide = K.prism("Slide", sec, 'y', SLIDE_REAR, SLIDE_FRONT, SLIDE, "slide", bevel=0.0009, segments=2)
    cutters = [
        # ventana de expulsión (arriba y a la derecha)
        K.box("PortCut", -1.6, 0.25, 3.2, 7.6, 2.45, 4.4, keep=False),
        # nariz achaflanada arriba y abajo
        K.side("NoseTop", [(13.5, 4.3), (14.6, 4.3), (14.6, 3.45)], 2.0, keep=False),
        K.side("NoseBottom", [(13.9, 1.25), (14.6, 1.25), (14.6, 1.85)], 2.0, keep=False),
        # boca: cañón y punta de la guía del muelle
        K.lathe("MuzzleHole", [(13.0, 0.72), (14.7, 0.72)], origin=(0, 0, BORE_Z), seg=24, keep=False),
        K.lathe("RodHole", [(13.0, 0.36), (14.7, 0.36)], origin=(0, 0, 1.85), seg=12, keep=False),
        # alojamiento de la tapa trasera (percutor)
        K.box("RearPlateCut", -0.62, 0.62, SLIDE_REAR - 0.1, SLIDE_REAR + 0.12, 1.65, 3.7, keep=False),
    ]
    # estrías traseras (8) y delanteras (5), inclinadas como las de una pistola de servicio
    for k in range(8):
        y = -3.85 + k * 0.42
        for s in (-1, 1):
            cutters.append(K.side(f"RearSerr{k}{s}", [(y, 1.6), (y + 0.17, 1.6), (y + 0.42, 3.5), (y + 0.25, 3.5)], 0.14,
                                  x_center=s * (SLIDE_HW + 0.02), keep=False))
    for k in range(5):
        y = 10.5 + k * 0.42
        for s in (-1, 1):
            cutters.append(K.side(f"FrontSerr{k}{s}", [(y, 1.75), (y + 0.17, 1.75), (y + 0.42, 3.45), (y + 0.25, 3.45)], 0.12,
                                  x_center=s * (SLIDE_HW + 0.02), keep=False))
    K.cut(slide, *cutters)
    # tapa trasera de polímero con muesca (se ve en ADS) y extractor (derecha)
    plate = K.box("RearPlate", -0.58, 0.58, SLIDE_REAR - 0.05, SLIDE_REAR + 0.1, 1.7, 3.65, FRAME, "slide", bevel=0.0004, segments=1)
    K.cut(plate, K.box("RearPlateNotch", -0.2, 0.2, SLIDE_REAR - 0.2, SLIDE_REAR + 0.02, 1.6, 2.05, keep=False))
    K.box("Extractor", -1.34, -1.22, 1.5, 3.25, 3.0, 3.5, STEEL, "slide", bevel=0.0003, segments=1)


def build_sights():
    # Trasera: base en cola de milano y dos orejas con muesca en U, toda negra
    K.prism("RearSightBase", [(-0.72, SLIDE_TOP - 0.05), (0.72, SLIDE_TOP - 0.05), (0.62, 4.55), (-0.62, 4.55)], 'y', -3.95, -2.85,
            STEEL, "slide", bevel=0.0004, segments=1)
    for s in (-1, 1):
        K.prism(f"RearSightEar{s}", [(s * 0.17, 4.5), (s * 0.62, 4.5), (s * 0.62, 4.85), (s * 0.5, SIGHT_Z), (s * 0.17, SIGHT_Z)], 'y',
                -3.95, -3.05, STEEL, "slide", bevel=0.0003, segments=1)
    # cara hacia el tirador con estrías horizontales (no refleja)
    # Delantera: poste con punto de tritio en la cara trasera
    K.prism("FrontSightBase", [(-0.42, SLIDE_TOP - 0.05), (0.42, SLIDE_TOP - 0.05), (0.3, 4.4), (-0.3, 4.4)], 'y', 13.0, 13.95,
            STEEL, "slide", bevel=0.0003, segments=1)
    K.prism("FrontSightPost", [(-0.15, 4.35), (0.15, 4.35), (0.15, SIGHT_Z), (-0.15, SIGHT_Z)], 'y', 13.25, 13.85,
            STEEL, "slide", bevel=0.00025, segments=1)
    K.box("FrontSightDot", -0.1, 0.1, 13.21, 13.27, 4.6, 4.82, TRITIUM, "slide", bevel=0.0, segments=1)


def build_barrel():
    # Cañón (tubo) y bloque de recámara visible por la ventana; no se mueve con la corredera
    K.lathe("Barrel", [(-1.0, 0.45), (-1.0, 0.66), (14.3, 0.66), (14.3, 0.45)], origin=(0, 0, BORE_Z), seg=24, closed=True, mat=STEEL)
    hood = K.box("BarrelHood", -0.64, 0.64, 2.9, 7.45, BORE_Z, 3.98, STEEL, bevel=0.0004, segments=1)
    K.cut(hood, K.lathe("HoodBore", [(2.0, 0.45), (8.0, 0.45)], origin=(0, 0, BORE_Z), seg=16, keep=False),
          K.box("ChamberWindow", -0.12, 0.12, 3.05, 3.35, 3.7, 4.1, keep=False))   # indicador de recámara cargada
    K.lathe("GuideRod", [(11.5, 0.3), (14.25, 0.3), (14.3, 0.24)], origin=(0, 0, 1.85), seg=12, mat=STEEL)


def taper_x(obj, z0, z1, factor):
    """Estrecha el objeto en X de forma suave entre las alturas z0 y z1 (cm)."""
    for v in obj.data.vertices:
        a = min(max((v.co.z * 100.0 - z0) / (z1 - z0), 0.0), 1.0)
        a = a * a * (3 - 2 * a)
        v.co.x *= 1.0 + (factor - 1.0) * a


def build_frame():
    hw = 1.22
    upper = K.side("FrameUpper", [(-4.0, 1.38), (13.2, 1.38), (13.2, 0.05), (12.75, -0.45), (6.35, -0.45), (6.05, -0.1),
                                  (-3.0, -0.1), (-4.0, 0.3)], hw, mat=FRAME, bevel=0.0015, segments=2)
    cutters = [K.box(f"RailSlot{k}", -1.5, 1.5, 8.3 + k * 1.25, 8.75 + k * 1.25, -0.55, -0.28, keep=False) for k in range(3)]
    for s in (-1, 1):
        cutters.append(K.box(f"RailGroove{s}", s * 1.04, s * 1.5, 6.6, 13.4, -0.18, 0.1, keep=False))
        # ranura de la palanca de desmontaje
        cutters.append(K.box(f"TakedownSlot{s}", s * 1.0, s * 1.5, 3.3, 4.5, 0.5, 0.8, keep=False))
    K.cut(upper, *cutters)
    for s in (-1, 1):
        K.box(f"TakedownTab{s}", s * 1.05, s * 1.27, 3.5, 4.3, 0.55, 0.75, STEEL, bevel=0.0002, segments=1)
    # Guardamonte de frente recto
    K.band("TriggerGuard", [(0.9, -2.85), (5.25, -3.05), (5.95, -2.85), (6.25, -2.3), (6.25, -0.35)], 0.5, 0.58,
           mat=FRAME, bevel=0.0012, segments=2)
    # Puño: frente y dorso paralelos, abultamiento para la palma y cola de castor bajo la corredera
    front = [P(s, 2.6 + 0.12 * (1 - s / 9.0)) for s in (9.0, 7.0, 5.0, 3.0, 1.2, 0.0)]
    back = [P(0.0, -2.72), P(1.2, -2.7), P(3.0, -2.8), P(5.0, -2.95), P(7.0, -2.85), P(8.8, -2.62)]
    top = [(-4.1, -1.0), (-4.45, -0.55), (-4.95, -0.25), (-5.08, 0.15), (-4.65, 0.42), (-3.9, 0.45), (1.05, 0.45), (1.05, -2.85)]
    grip = K.side("Grip", front + back + top, 1.5, mat=FRAME, bevel=0.004, segments=3)
    taper_x(grip, -3.2, -0.2, 0.82)   # arriba se estrecha hasta el ancho del marco (sin escalón bajo la corredera)
    panel = grip_quad(1.4, 7.6, -2.0, 1.9)
    K.cut(grip,
          K.side("GripPanelL", panel, 0.12, x_center=1.5, keep=False),
          K.side("GripPanelR", panel, 0.12, x_center=-1.5, keep=False),
          K.side("MagWellCut", grip_quad(-1.0, 4.0, -1.75, 1.75), 1.17, keep=False))
    # Botón del cargador (izquierda, tras el guardamonte) y retén de la corredera (izquierda, sobre el gatillo)
    K.box("MagRelease", 1.2, 1.42, 0.05, 0.85, -1.85, -1.25, FRAME, bevel=0.0004, segments=1)
    K.side("SlideStop", [(0.4, 0.78), (2.7, 0.88), (3.0, 1.28), (0.5, 1.32)], 0.07, x_center=1.3, mat=STEEL, bevel=0.0003, segments=1)
    for k, y in enumerate((-2.6, 5.0)):  # pasadores del marco (izquierda)
        K.lathe(f"FramePin{k}", [(1.18, 0.2), (1.3, 0.2), (1.34, 0.16)], origin=(0, y, 0.55), direction=(1, 0, 0), seg=10, mat=STEEL)
    # Gatillo con seguro de lámina
    K.side("Trigger", [(2.6, -0.15), (3.15, -0.15), (3.15, -0.9), (3.0, -1.6), (2.72, -2.15), (2.47, -2.25), (2.57, -1.9),
                       (2.7, -1.3), (2.62, -0.6)], 0.3, mat=FRAME, bone="trigger", bevel=0.0005, segments=1)
    K.side("TriggerSafety", [(2.66, -0.7), (2.74, -0.7), (2.6, -1.85), (2.5, -1.85)], 0.09, mat=STEEL, bone="trigger", bevel=0.0, segments=1)


MAG_TOP_S = 11.9


def build_magazine():
    """Cargador de doble hilera: cuerpo de acero (casi todo dentro del puño) y base de polímero que asoma."""
    K.side("MagBody", grip_quad(0.0, MAG_TOP_S, -1.62, 1.62), 1.1, mat=STEEL, bone="magazine", bevel=0.001, segments=1)
    base = K.side("MagBase", [P(-0.62, -1.85), P(-0.62, 1.95), P(-0.25, 2.05), P(0.05, 1.8), P(0.05, -1.85)], 1.32,
                  mat=FRAME, bone="magazine", bevel=0.0015, segments=2)
    K.cut(base, K.side("MagBaseGroove", grip_quad(-0.45, -0.3, -1.5, 1.5), 0.2, x_center=1.32, keep=False))
    # labios y primera bala (se ven al sacar el cargador)
    K.side("MagLips", grip_quad(MAG_TOP_S - 0.35, MAG_TOP_S, -1.5, 1.5), 1.18, mat=STEEL, bone="magazine", bevel=0.0005, segments=1)
    tip = P(MAG_TOP_S + 0.25, 0.4)
    K.lathe("MagRound", [(-1.0, 0.45), (0.3, 0.45), (0.7, 0.3), (0.9, 0.12)], origin=(0, tip[0], tip[1]), direction=(0, N[0], N[1]),
            seg=12, mat=STEEL, bone="magazine")


def build_geometry():
    build_slide()
    build_sights()
    build_barrel()
    build_frame()
    build_magazine()


MAG_TOP = P(MAG_TOP_S, 0.0)
MAG_BASE = P(-0.62, 0.0)
BONES = {
    # nombre: (padre, posición de diseño en cm)
    "root": (None, (0, -OY, 0)),       # origen del arma
    "slide": ("root", (0, 4.0, BORE_Z)),
    "slide_rear": ("slide", (0, -2.4, SLIDE_TOP + 0.3)),
    "trigger": ("root", (0, 2.8, -0.3)),
    "magazine": ("root", (0, MAG_TOP[0], MAG_TOP[1])),
    "Mag": ("magazine", (0, MAG_BASE[0], MAG_BASE[1])),
    "Sight": ("root", (0, -3.5, SIGHT_Z)),
    "Muzzle": ("root", (0, 14.4, BORE_Z)),
    "Eject": ("root", (-1.4, 5.4, 3.7)),
    "HandGrip_L": ("root", (2.3, -1.9, -5.2)),   # palma de la mano de apoyo, sobre el lado izquierdo del puño
}


def build_armature():
    arm_data = bpy.data.armatures.new("P17_Rig")
    arm = bpy.data.objects.new("Armature", arm_data)
    bpy.context.scene.collection.objects.link(arm)
    bpy.context.view_layer.objects.active = arm
    bpy.ops.object.mode_set(mode='EDIT')
    for name, (parent, pos) in BONES.items():
        b = arm_data.edit_bones.new(name)
        b.head = W(*pos)
        b.tail = W(pos[0], pos[1], pos[2] + 1.0)
        if parent:
            b.parent = arm_data.edit_bones[parent]
    bpy.ops.object.mode_set(mode='OBJECT')
    return arm


def extract_magazine(weapon):
    from mathutils import Matrix
    mag = weapon.copy()
    mag.data = weapon.data.copy()
    mag.name = mag.data.name = "SM_P17_Mag"
    bpy.context.scene.collection.objects.link(mag)
    vg = weapon.vertex_groups["magazine"].index
    bm = bmesh.new()
    bm.from_mesh(mag.data)
    deform = bm.verts.layers.deform.active
    bmesh.ops.delete(bm, geom=[v for v in bm.verts if vg not in v[deform]], context='VERTS')
    bm.to_mesh(mag.data)
    bm.free()
    mag.vertex_groups.clear()
    mag.data.transform(Matrix.Translation(-W(*BONES["magazine"][1])))
    return mag


def casing_9mm(out_dir):
    """Vaina 9x19 (eje +X, como SM_Casing_556): 1,92 cm x Ø0,99 cm, con ranura del extractor."""
    bm = bmesh.new()
    prof = [(0.0, 0.00495), (0.0011, 0.00495), (0.0013, 0.0042), (0.0023, 0.0042), (0.0026, 0.00490), (0.0192, 0.00478)]
    rings = []
    for x, r in prof:
        rings.append([bm.verts.new((x, r * math.cos(2 * math.pi * i / 10), r * math.sin(2 * math.pi * i / 10))) for i in range(10)])
    for a, b in zip(rings, rings[1:]):
        for i in range(10):
            bm.faces.new((a[i], a[(i + 1) % 10], b[(i + 1) % 10], b[i]))
    bm.faces.new(list(reversed(rings[0])))
    bm.faces.new(rings[-1])
    bmesh.ops.recalc_face_normals(bm, faces=bm.faces)
    obj = bl_lib.bm_to_object(bm, "SM_Casing_9mm", [bl_lib.get_material("MI_Brass", (0.78, 0.56, 0.24, 1), 0.3, 1.0)])
    bpy.context.view_layer.objects.active = obj
    bpy.ops.object.shade_smooth()
    bl_lib.export_fbx([obj], os.path.join(out_dir, "SM_Casing_9mm.fbx"))
    obj.hide_render = True
    print(f"[gen_p17] SM_Casing_9mm: {bl_lib.triangle_count(obj)} tris")


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
    center = W(0, 4.8, -3.2)
    views = (("_right", Vector((-1, 0, 0.0)), 0.3, center), ("_left", Vector((1, 0, 0.0)), 0.3, center),
             ("_34", Vector((0.8, 0.55, 0.45)), 0.32, center),
             ("_rear", Vector((0.0, 1.0, 0.12)), 0.09, W(0, 0, 4.0)))    # lo que ve el jugador en ADS
    for suffix, direction, ortho, c in views:
        d = direction.normalized()
        cam_data.ortho_scale = ortho
        cam.location = c + d * 1.0
        cam.rotation_euler = (-d).to_track_quat('-Z', 'Y').to_euler()
        scene.render.filepath = path.replace(".png", f"{suffix}.png")
        bpy.ops.render.render(write_still=True)
        print(f"[gen_p17] Preview: {scene.render.filepath}")


def main():
    argv = sys.argv[sys.argv.index("--") + 1:] if "--" in sys.argv else []
    out = os.path.abspath(argv[0] if argv else os.path.join(os.path.dirname(__file__), "..", "export", "SK_P17.fbx"))
    preview = next((a for a in argv[1:] if not a.startswith("--")), None)

    bl_lib.reset_scene()
    K.MATERIALS = [
        bl_lib.get_material("MI_P17_Slide", (0.03, 0.03, 0.032, 1), 0.4, 0.7),
        bl_lib.get_material("MI_P17_Frame", (0.04, 0.04, 0.04, 1), 0.65, 0.0),
        bl_lib.get_material("MI_P17_Steel", (0.05, 0.05, 0.05, 1), 0.35, 0.9),
        bl_lib.get_material("MI_P17_Tritium", (1.0, 0.45, 0.05, 1), 0.3, 0.0),
    ]
    build_geometry()
    K.apply_bevels()

    for obj, bone in PARTS:
        vg = obj.vertex_groups.new(name=bone)
        vg.add(list(range(len(obj.data.vertices))), 1.0, 'REPLACE')
    bpy.ops.object.select_all(action='DESELECT')
    for obj, _ in PARTS:
        obj.select_set(True)
    mesh_obj = PARTS[0][0]
    bpy.context.view_layer.objects.active = mesh_obj
    bpy.ops.object.join()
    mesh_obj.name = mesh_obj.data.name = "SK_P17"

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
    bpy.ops.object.mode_set(mode='EDIT')
    bpy.ops.mesh.select_all(action='SELECT')
    bpy.ops.uv.smart_project(angle_limit=math.radians(55), island_margin=0.004, area_weight=0.0)
    bpy.ops.uv.pack_islands(rotate=True, margin=0.003)
    bpy.ops.object.mode_set(mode='OBJECT')
    scale = K.uv_scale_cm(mesh_obj)
    print(f"[gen_p17] UV: {scale:.1f} cm por unidad de UV")

    out_dir = os.path.dirname(out)
    os.makedirs(out_dir, exist_ok=True)
    mag_obj = extract_magazine(mesh_obj)
    bl_lib.export_fbx([mag_obj], os.path.join(out_dir, "SM_P17_Mag.fbx"))
    mag_obj.hide_render = True
    casing_9mm(out_dir)

    if "--bake" in argv:
        tex = os.path.join(out_dir, "T_P17_Masks.png")
        K.bake_masks(mesh_obj, tex, size=1024)
        with open(tex.replace(".png", ".json"), "w") as f:
            json.dump({"uv_scale_cm": scale}, f)

    arm = build_armature()
    mesh_obj.parent = arm
    mod = mesh_obj.modifiers.new("Armature", 'ARMATURE')
    mod.object = arm
    print(f"[gen_p17] SK_P17: {bl_lib.triangle_count(mesh_obj)} tris, {len(BONES)} huesos; SM_P17_Mag: {bl_lib.triangle_count(mag_obj)} tris")

    bpy.ops.object.select_all(action='DESELECT')
    arm.select_set(True)
    mesh_obj.select_set(True)
    bpy.context.view_layer.objects.active = arm
    bpy.ops.export_scene.fbx(
        filepath=out, use_selection=True, apply_unit_scale=True, apply_scale_options='FBX_SCALE_UNITS',
        axis_forward='-Y', axis_up='Z', object_types={'MESH', 'ARMATURE'}, mesh_smooth_type='FACE',
        use_tspace=True, add_leaf_bones=False, primary_bone_axis='Y', secondary_bone_axis='X',
        armature_nodetype='NULL', use_armature_deform_only=False, bake_anim=False)
    print(f"[gen_p17] Exportado: {out}")

    if preview:
        render_preview(mesh_obj, preview)


if __name__ == "__main__":
    main()
