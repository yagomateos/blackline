"""AR-7 "Halcón": fusil de asalto 5.56 compacto (malla esquelética) para BLACKLINE. Versión 2 (detalle de primera persona).

Uso:
  blender -b --factory-startup -P gen_ar7.py -- <salida.fbx> [preview.png] [--bake]

Convenciones (coordenadas en cm, sistema de Unreal para el arma):
  +Y adelante (cañón), +Z arriba, -X = lado derecho (ventana de expulsión), +X = lado izquierdo (el que ve el jugador).
  Origen = parte superior del pistolete (donde encaja la mano derecha, como el fusil de Epic).
  En Blender: x_bl = x, y_bl = -y, z_bl = z (metros). Ver W().

Huesos (los de referencia sirven de sockets en Unreal; solo se usa su posición):
  root
   ├─ magazine ─ Mag (base del cargador: agarre de la mano en la recarga)
   ├─ bolt_carrier, charging_handle, trigger, selector
   └─ Sight (centro de la apertura trasera), Muzzle, Eject, HandGrip_L
Materiales: MI_AR7_Metal, MI_AR7_Polymer (coyote), MI_AR7_PolymerDark, MI_AR7_Rubber, MI_AR7_Tritium.

v2: perfiles reales en vez de cajas: receptor superior con hombros, raíl Picatinny con ranuras, guardamanos hueco
con ranuras M-LOK (se ve el cañón y el tubo de gases), apagallamas de jaula, cargador curvo tipo PMAG con nervios,
pistolete con dedo y cola de castor, guardamonte curvo, mira trasera de anillo y delantera con capucha cerrada.
Con --bake hornea las máscaras (AO, aristas, cavidades) sin ruido y escribe T_AR7_Masks.json (escala de las UV).
"""
import json
import math
import os
import sys

sys.path.append(os.path.dirname(os.path.abspath(__file__)))
import bmesh
import bpy
from mathutils import Matrix, Vector
import bl_lib

BORE_Z = 7.5        # eje del cañón
RAIL_TOP = 11.2     # parte superior del raíl
SIGHT_Z = 14.5      # línea de mira (centro de la apertura trasera y punta del poste delantero)

METAL, POLY, POLY_DARK, RUBBER, TRITIUM = range(5)


def W(x, y, z):
    """Coordenadas del arma (cm, convención Unreal) -> Blender (m)."""
    return Vector((x / 100.0, -y / 100.0, z / 100.0))


PARTS = []  # (objeto, hueso)


# ---------------------------------------------------------------------------
# Primitivas. keep=False crea un "cortador" (no forma parte del arma ni lleva bisel).
# El bisel se guarda en el objeto y se aplica al final, después de todos los cortes.
# ---------------------------------------------------------------------------

def finish_part(bm, name, mat, bone, bevel, segments, keep=True):
    for f in bm.faces:
        f.material_index = mat
    bmesh.ops.recalc_face_normals(bm, faces=bm.faces)
    obj = bl_lib.bm_to_object(bm, name, MATERIALS)
    if keep:
        obj["bl_bevel"] = (bevel, segments)
        PARTS.append((obj, bone))
    return obj


def prism(name, pts, axis, w0, w1, mat=METAL, bone="root", bevel=0.0025, segments=2, keep=True):
    """Polígono 2D extruido.
    axis 'x': pts = (y, z), extruido en X de w0 a w1 (perfil lateral)
    axis 'y': pts = (x, z), extruido en Y (sección transversal)
    axis 'z': pts = (x, y), extruido en Z (planta)"""
    def P(a, b, c):
        if axis == 'x':
            return W(c, a, b)
        if axis == 'y':
            return W(a, c, b)
        return W(a, b, c)
    bm = bmesh.new()
    v0 = [bm.verts.new(P(a, b, w0)) for a, b in pts]
    v1 = [bm.verts.new(P(a, b, w1)) for a, b in pts]
    bm.faces.new(v0)
    bm.faces.new(list(reversed(v1)))
    n = len(pts)
    for i in range(n):
        j = (i + 1) % n
        bm.faces.new((v0[i], v0[j], v1[j], v1[i]))
    return finish_part(bm, name, mat, bone, bevel, segments, keep)


def side(name, pts, half_w, x_center=0.0, **kw):
    """Perfil lateral (y, z) con anchura total 2*half_w centrado en x_center."""
    return prism(name, pts, 'x', x_center - half_w, x_center + half_w, **kw)


def box(name, x0, x1, y0, y1, z0, z1, mat=METAL, bone="root", bevel=0.0025, segments=2, keep=True):
    return prism(name, [(y0, z0), (y1, z0), (y1, z1), (y0, z1)], 'x', x0, x1, mat, bone, bevel, segments, keep)


def lathe(name, prof, origin=(0.0, 0.0, BORE_Z), direction=(0.0, 1.0, 0.0), seg=24, closed=False,
          mat=METAL, bone="root", bevel=0.0, segments=1, phase=0.5, keep=True):
    """Sólido de revolución. prof = [(t, r)]: t a lo largo del eje, r radio.
    closed=False: perfil abierto con tapas en los extremos. closed=True: la sección es un lazo
    cerrado (p. ej. un tubo hueco o un anillo) y no lleva tapas."""
    d = Vector(direction).normalized()
    up = Vector((0, 0, 1)) if abs(d.z) < 0.9 else Vector((1, 0, 0))
    u = d.cross(up).normalized()
    v = u.cross(d).normalized()
    o = Vector(origin)
    bm = bmesh.new()
    rings = []
    for t, r in prof:
        ring = []
        for i in range(seg):
            a = 2 * math.pi * (i + phase) / seg
            p = o + d * t + (u * math.cos(a) + v * math.sin(a)) * r
            ring.append(bm.verts.new(W(p.x, p.y, p.z)))
        rings.append(ring)
    n = len(rings)
    for k in range(n if closed else n - 1):
        a, b = rings[k], rings[(k + 1) % n]
        for i in range(seg):
            j = (i + 1) % seg
            bm.faces.new((a[i], a[j], b[j], b[i]))
    if not closed:
        bm.faces.new(list(reversed(rings[0])))
        bm.faces.new(rings[-1])
    return finish_part(bm, name, mat, bone, bevel, segments, keep)


def band(name, centerline, thick, half_w, **kw):
    """Banda curva (guardamonte): polilínea (y, z) con grosor, extruida en X."""
    left, right = [], []
    n = len(centerline)
    for i, (y, z) in enumerate(centerline):
        y0, z0 = centerline[max(i - 1, 0)]
        y1, z1 = centerline[min(i + 1, n - 1)]
        ty, tz = y1 - y0, z1 - z0
        ln = math.hypot(ty, tz) or 1.0
        ny, nz = -tz / ln, ty / ln
        left.append((y + ny * thick / 2, z + nz * thick / 2))
        right.append((y - ny * thick / 2, z - nz * thick / 2))
    return side(name, left + list(reversed(right)), half_w, **kw)


def cut(obj, *cutters):
    """Resta uno o varios cortadores (se unen y se aplica una sola booleana exacta)."""
    cutters = [c for c in cutters if c is not None]
    if not cutters:
        return obj
    bpy.ops.object.select_all(action='DESELECT')
    for c in cutters:
        c.select_set(True)
    bpy.context.view_layer.objects.active = cutters[0]
    if len(cutters) > 1:
        bpy.ops.object.join()
    cutter = cutters[0]
    mod = obj.modifiers.new("Cut", 'BOOLEAN')
    mod.operation = 'DIFFERENCE'
    mod.solver = 'EXACT'
    mod.object = cutter
    mod.use_self = len(cutters) > 1  # los cortadores unidos pueden solaparse entre sí
    bl_lib.apply_modifiers(obj)
    bpy.data.objects.remove(cutter)
    return obj


def pin_x(name, y, z, radius=0.3, half_len=1.46, head=0.42, mat=METAL):
    """Pasador transversal con cabeza en el lado izquierdo (+X)."""
    lathe(name, [(-half_len, radius), (half_len - 0.06, radius), (half_len - 0.06, head), (half_len + 0.04, head), (half_len + 0.08, head * 0.8)],
          origin=(0, y, z), direction=(1, 0, 0), seg=12, mat=mat)


def screw_x(name, x, y, z, sign=1, r=0.22):
    """Tornillo Allen visto de lado (cabeza hacia sign*X)."""
    obj = lathe(name, [(0, r), (0.12, r), (0.16, r * 0.8)], origin=(x, y, z), direction=(sign, 0, 0), seg=10)
    hexc = lathe(name + "Hex", [(0.06, r * 0.45), (0.3, r * 0.45)], origin=(x, y, z), direction=(sign, 0, 0), seg=6, keep=False)
    return cut(obj, hexc)


# ---------------------------------------------------------------------------
# Geometría
# ---------------------------------------------------------------------------

RAIL_SECTION = [(-0.80, 10.33), (0.80, 10.33), (0.80, 10.52), (1.06, 10.78), (1.06, 10.93), (0.88, 11.2),
                (-0.88, 11.2), (-1.06, 10.93), (-1.06, 10.78), (-0.80, 10.52)]


def rail(name, y0, y1, skip=()):
    """Raíl Picatinny: sección en cola de milano y ranuras transversales cada 1 cm (las de 'skip' se omiten)."""
    r = prism(name, RAIL_SECTION, 'y', y0, y1, METAL, bevel=0.0006, segments=1)
    cutters = []
    y = y0 + 0.55
    k = 0
    while y + 0.53 < y1 - 0.3:
        if not any(a <= y <= b for a, b in skip):
            cutters.append(box(f"{name}Slot{k}", -1.3, 1.3, y, y + 0.53, 10.92, 11.5, keep=False))
        y += 1.0
        k += 1
    return cut(r, *cutters)


def build_upper():
    # Receptor superior: sección con hombros bajo el raíl
    upper = prism("UpperReceiver", [(-1.6, 5.0), (1.6, 5.0), (1.6, 8.7), (1.3, 9.9), (0.9, 10.36), (-0.9, 10.36),
                                    (-1.3, 9.9), (-1.6, 8.7)], 'y', -11.6, 16.0, bevel=0.0018, segments=2)
    cut(upper,
        box("PortCut", -2.0, -1.05, 3.0, 9.8, 6.2, 8.9, keep=False),                         # ventana de expulsión
        box("ChargingSlot", -0.72, 0.72, -12.0, -10.2, 9.5, 10.6, keep=False),               # alojamiento de la palanca
        box("UpperFrontStep", -2.0, 2.0, 14.6, 16.5, 5.0, 5.5, keep=False))                  # escalón del pivote
    rail("UpperRail", -10.6, 15.6)
    # Cerrojo visible tras la ventana (se anima por hueso)
    bc = box("BoltCarrier", -1.32, -1.0, 3.3, 9.5, 6.35, 8.75, METAL, "bolt_carrier", bevel=0.0006, segments=1)
    cut(bc, box("BoltCarrierCut", -1.5, -1.2, 4.6, 7.2, 8.0, 9.0, keep=False))
    lathe("BoltHead", [(9.2, 0.55), (9.75, 0.55)], origin=(-1.0, 0, 7.55), seg=12, bone="bolt_carrier")
    # Tapa guardapolvo abierta (colgando sobre el receptor inferior) y su bisagra
    dc = side("DustCover", [(3.1, 5.25), (9.7, 5.25), (9.7, 2.95), (3.1, 2.95)], 0.07, x_center=-1.5, bevel=0.0005, segments=1)
    cut(dc, box("DustCoverRib", -1.7, -1.5, 3.6, 9.2, 4.0, 4.25, keep=False))
    lathe("DustCoverHinge", [(2.6, 0.13), (10.2, 0.13)], origin=(-1.62, 0, 5.15), seg=8)
    # Deflector de vainas y asistente de cierre
    prism("BrassDeflector", [(-1.55, -2.6), (-1.55, 2.7), (-2.3, 1.9), (-2.35, -1.1)], 'z', 6.9, 9.95, bevel=0.0015)
    fa_dir = (-0.55, -0.84, 0.0)
    lathe("ForwardAssist", [(0, 0.82), (2.4, 0.82), (2.4, 0.95), (2.7, 0.95)], origin=(-1.35, -3.2, 7.9), direction=fa_dir, seg=16, bevel=0.0006)
    lathe("ForwardAssistButton", [(2.7, 0.62), (3.6, 0.62), (3.75, 0.5)], origin=(-1.35, -3.2, 7.9), direction=fa_dir, seg=16)
    # Palanca de carga (T) con pestillo grande en el lado izquierdo
    prism("ChargingHandle", [(-0.62, -10.3), (0.62, -10.3), (0.62, -12.5), (1.9, -12.7), (2.3, -13.2), (2.2, -13.7),
                             (-1.7, -13.7), (-1.8, -13.2), (-1.4, -12.7), (-0.62, -12.5)], 'z', 9.55, 10.45,
          METAL, "charging_handle", bevel=0.0008, segments=2)
    box("ChargingLatch", 1.2, 2.2, -13.6, -12.8, 10.45, 10.6, METAL, "charging_handle", bevel=0.0004, segments=1)
    # Anilla del cañón / tuerca del guardamanos
    lathe("BarrelNut", [(15.6, 2.25), (16.7, 2.25)], seg=24, bevel=0.0006)


def build_lower():
    lower = side("LowerReceiver", [(-11.6, 5.0), (12.6, 5.0), (12.6, 3.6), (11.8, 2.6), (11.2, 1.6), (11.0, -3.8),
                                   (2.6, -3.8), (2.2, -2.0), (1.9, -0.6), (-4.4, -0.6), (-6.8, -0.4), (-8.8, 0.8),
                                   (-10.6, 2.4), (-11.6, 3.6)], 1.4, bevel=0.0025, segments=2)
    mag_well = [(2.75, 1.2), (9.45, 1.2), (9.45, -6.0), (2.75, -6.0)]
    cut(lower, side("MagWellCut", mag_well, 1.06, keep=False),
        side("LowerLightenL", [(-6.5, 3.6), (-3.6, 3.6), (-3.6, 2.4), (-6.5, 2.4)], 0.2, x_center=1.45, keep=False),
        side("MagWellPanelL", [(3.4, 0.6), (10.5, 0.6), (10.3, -3.2), (3.6, -3.2)], 0.1, x_center=1.45, keep=False),
        side("MagWellPanelR", [(3.4, 0.6), (10.5, 0.6), (10.3, -3.2), (3.6, -3.2)], 0.1, x_center=-1.45, keep=False))
    flare = side("MagWellFlare", [(2.0, -3.3), (11.25, -3.3), (11.5, -4.7), (1.75, -4.7)], 1.62, bevel=0.0015)
    cut(flare, side("MagWellFlareCut", mag_well, 1.06, keep=False))
    # Pasadores (cabeza a la izquierda)
    for k, (py, pz) in enumerate(((-9.0, 3.9), (11.7, 3.9), (-2.5, 1.5), (0.7, 1.5))):
        pin_x(f"Pin{k}", py, pz)
    # Guardamonte curvo
    band("TriggerGuard", [(2.25, -0.7), (2.15, -3.0), (1.75, -3.95), (0.8, -4.3), (-0.6, -4.2), (-1.4, -3.75), (-1.55, -3.0)],
         0.6, 0.55, bevel=0.0015, segments=2)
    side("Trigger", [(1.15, -0.6), (1.75, -0.6), (1.72, -1.6), (1.5, -2.6), (1.05, -3.25), (0.8, -3.15), (1.12, -2.4),
                     (1.22, -1.5)], 0.28, mat=METAL, bone="trigger", bevel=0.0006, segments=1)
    # Selector (izquierda): cubo + palanca con indicador
    lathe("SelectorHub", [(1.38, 0.55), (1.6, 0.55), (1.66, 0.45)], origin=(0, -2.4, 2.6), direction=(1, 0, 0), seg=16,
          bone="selector", bevel=0.0003)
    side("SelectorLever", [(-2.5, 2.95), (-0.5, 2.82), (-0.35, 2.55), (-0.5, 2.3), (-2.5, 2.25)], 0.08, x_center=1.68,
         mat=METAL, bone="selector", bevel=0.0004, segments=1)
    # Retén del cerrojo (izquierda)
    side("BoltCatch", [(0.15, 4.3), (1.25, 4.3), (1.35, 2.2), (0.7, 1.3), (0.15, 2.4)], 0.13, x_center=1.53, bevel=0.0005, segments=1)
    lathe("BoltCatchPin", [(1.38, 0.2), (1.62, 0.2)], origin=(0, 0.6, 3.6), direction=(1, 0, 0), seg=8)
    # Botón del cargador (derecha) con guardas
    lathe("MagRelease", [(1.4, 0.48), (1.62, 0.48), (1.7, 0.4)], origin=(0, 6.6, 1.6), direction=(-1, 0, 0), seg=16)
    for k, z in enumerate((2.4, 0.75)):
        side(f"MagReleaseFence{k}", [(5.4, z + 0.18), (7.8, z + 0.18), (7.8, z - 0.18), (5.4, z - 0.18)], 0.12, x_center=-1.5,
             bevel=0.0004, segments=1)
    # Pistolete con apoyo para el dedo y cola de castor
    grip = side("PistolGrip", [(-0.4, -0.1), (-0.6, -1.2), (-0.95, -2.2), (-1.7, -3.0), (-1.65, -4.0), (-2.4, -6.5),
                               (-3.5, -9.5), (-4.3, -11.5), (-4.8, -12.2), (-5.8, -12.5), (-7.9, -12.2), (-8.5, -11.5),
                               (-7.7, -8.0), (-6.5, -4.2), (-5.7, -2.2), (-6.5, -1.4), (-7.2, -0.6), (-6.8, -0.1)],
                1.45, mat=POLY_DARK, bevel=0.0055, segments=3)
    # paneles de agarre (rebajes poco profundos a ambos lados)
    panel = [(-2.3, -4.2), (-3.4, -8.2), (-5.6, -8.2), (-5.0, -4.2)]
    cut(grip, side("GripPanelL", panel, 0.2, x_center=1.5, keep=False), side("GripPanelR", panel, 0.2, x_center=-1.5, keep=False))
    side("GripCap", [(-4.85, -12.25), (-5.8, -12.55), (-7.9, -12.25), (-8.0, -12.75), (-5.6, -13.05), (-4.75, -12.7)], 1.2,
         mat=POLY_DARK, bevel=0.002, segments=2)


def build_handguard():
    # Guardamanos octogonal hueco (se ve el cañón por las ranuras)
    hg = lathe("Handguard", [(16.7, 2.48), (16.7, 2.92), (40.9, 2.92), (41.35, 2.72), (41.35, 2.48)], seg=8, closed=True,
               mat=POLY, bevel=0.0035, segments=3)
    cutters = []
    for k, y in enumerate((18.2, 22.2, 26.2, 30.2, 34.2, 38.0)):
        L = 2.9 if k < 5 else 2.0
        cutters.append(box(f"MLokL{k}", 2.2, 3.4, y, y + L, BORE_Z - 0.38, BORE_Z + 0.38, keep=False))
        cutters.append(box(f"MLokR{k}", -3.4, -2.2, y, y + L, BORE_Z - 0.38, BORE_Z + 0.38, keep=False))
        cutters.append(box(f"MLokB{k}", -0.38, 0.38, y, y + L, BORE_Z - 3.4, BORE_Z - 2.2, keep=False))
        # ranuras en las caras diagonales inferiores (giradas 45°)
        for s in (-1, 1):
            c = box(f"MLokD{k}{s}", -0.38, 0.38, y, y + L, BORE_Z - 3.4, BORE_Z - 2.2, keep=False)
            c.data.transform(Matrix.Translation(W(0, 0, BORE_Z)) @ Matrix.Rotation(math.radians(45 * s), 4, 'Y')
                             @ Matrix.Translation(-W(0, 0, BORE_Z)))
            cutters.append(c)
    cut(hg, *cutters)
    box("HandguardRailBase", -0.95, 0.95, 16.7, 41.2, 10.0, 10.4, bevel=0.0006, segments=1)
    rail("HandguardRail", 16.7, 41.2, skip=((36.6, 40.0),))
    for s in (-1, 1):
        screw_x(f"HGScrew{s}a", s * 2.62, 17.4, 6.4, sign=s)
        screw_x(f"HGScrew{s}b", s * 2.62, 17.4, 8.6, sign=s)
    # Copa QD para la correa (lado izquierdo, delante)
    lathe("QDCup", [(2.55, 0.32), (2.55, 0.6), (3.0, 0.6), (3.08, 0.5), (3.08, 0.32)], origin=(0, 39.6, 6.3),
          direction=(1, 0, 0), seg=16, closed=True, bevel=0.0003)


def build_barrel():
    lathe("Barrel", [(12.0, 1.0), (39.6, 1.0), (39.9, 0.82), (47.9, 0.76), (48.1, 0.62), (48.6, 0.62)], seg=24)
    lathe("GasBlock", [(34.8, 1.28), (37.2, 1.28)], seg=16, bevel=0.0006)
    box("GasBlockTop", -0.75, 0.75, 34.8, 37.2, 8.0, 9.45, bevel=0.0006, segments=1)
    lathe("GasTube", [(12.0, 0.22), (35.0, 0.22)], origin=(0, 0, 9.05), seg=10)
    lathe("CrushWasher", [(48.1, 1.15), (48.45, 1.15)], seg=24, bevel=0.0003)
    # Apagallamas de jaula: ranuras laterales y superior (abajo cerrado para no levantar polvo)
    hider = lathe("FlashHider", [(48.45, 0.45), (48.45, 0.98), (48.8, 1.12), (53.7, 1.12), (54.0, 0.96), (54.0, 0.62),
                                 (52.0, 0.56)], seg=24, closed=True)
    cutters = []
    for k, ang in enumerate((0, 60, 120, 180)):
        c = box(f"HiderSlot{k}", 0.3, 1.5, 50.0, 53.5, BORE_Z - 0.2, BORE_Z + 0.2, keep=False)
        c.data.transform(Matrix.Translation(W(0, 0, BORE_Z)) @ Matrix.Rotation(math.radians(-ang), 4, 'Y')
                         @ Matrix.Translation(-W(0, 0, BORE_Z)))
        cutters.append(c)
    cut(hider, *cutters)
    hider["bl_bevel"] = (0.0004, 1)


def build_sights():
    # Delantera abatible (desplegada): abrazadera, orejas unidas por la capucha, poste con tritio
    base_sec = [(-1.2, 10.55), (1.2, 10.55), (1.2, 11.6), (0.9, 12.25), (-0.9, 12.25), (-1.2, 11.6)]
    prism("FrontSightBase", base_sec, 'y', 36.8, 39.8, bevel=0.0012)
    for s in (-1, 1):
        prism(f"FrontSightEar{s}", [(s * 0.5, 12.1), (s * 0.92, 12.1), (s * 0.92, 15.75), (s * 0.5, 15.75)], 'y', 37.6, 38.9, bevel=0.0006)
    box("FrontSightHood", -0.92, 0.92, 37.6, 38.9, 15.3, 15.75, bevel=0.0005)
    lathe("FrontSightDrum", [(12.1, 0.38), (12.6, 0.38)], origin=(0, 38.25, 0), direction=(0, 0, 1), seg=12)
    box("FrontSightPost", -0.11, 0.11, 38.1, 38.4, 12.6, SIGHT_Z - 0.35, bevel=0.0002, segments=1)
    box("FrontSightTritium", -0.12, 0.12, 38.08, 38.42, SIGHT_Z - 0.35, SIGHT_Z, TRITIUM, bevel=0.0, segments=1)
    box("FrontSightClamp", 1.2, 1.45, 37.2, 39.4, 10.6, 11.5, bevel=0.0004, segments=1)
    # Trasera: base, anillo de apertura centrado en la línea de mira, orejas protectoras, rueda de deriva
    prism("RearSightBase", [(-1.45, 10.55), (1.45, 10.55), (1.45, 11.8), (1.1, 12.45), (-1.1, 12.45), (-1.45, 11.8)], 'y', -10.0, -6.4, bevel=0.0012)
    lathe("RearSightAperture", [(-8.3, 0.42), (-8.3, 0.85), (-7.75, 0.85), (-7.75, 0.42)], origin=(0, 0, SIGHT_Z), seg=24,
          closed=True, bevel=0.0003)
    box("RearSightStem", -0.38, 0.38, -8.3, -7.75, 12.4, SIGHT_Z - 0.7, bevel=0.0003)
    for s in (-1, 1):
        prism(f"RearSightEar{s}", [(s * 1.08, 12.4), (s * 1.4, 12.4), (s * 1.4, 15.3), (s * 1.25, 15.75), (s * 1.08, 15.75)], 'y', -8.9, -7.1, bevel=0.0005)
    knob = lathe("WindageKnob", [(1.45, 0.48), (1.9, 0.48), (1.95, 0.4)], origin=(0, -9.2, 11.7), direction=(1, 0, 0), seg=16)
    cut(knob, box("WindageNotch", 1.5, 2.1, -9.2 - 0.06, -9.2 + 0.06, 11.7 - 0.6, 11.7 + 0.6, keep=False))


def build_stock():
    lathe("CastleNut", [(-13.0, 1.4), (-13.0, 1.85), (-12.0, 1.85), (-12.0, 1.4)], seg=24, closed=True, bevel=0.0004)
    lathe("BufferTube", [(-12.0, 1.42), (-31.0, 1.42), (-31.4, 1.2)], seg=24)
    stock = side("Stock", [(-20.0, 9.6), (-21.2, 10.5), (-33.0, 11.2), (-33.7, 10.7), (-33.7, -2.3), (-33.1, -2.9),
                           (-30.0, -2.9), (-29.2, -1.9), (-26.2, 3.4), (-23.6, 5.2), (-20.8, 5.7), (-20.0, 6.3)],
                 1.85, mat=POLY, bevel=0.004, segments=3)
    cut(stock, side("StockWindow", [(-27.8, 4.9), (-32.2, 5.0), (-32.3, -1.4), (-30.6, -1.5)], 3.0, keep=False))
    side("StockLock", [(-20.6, 5.8), (-24.2, 5.5), (-24.0, 4.8), (-20.8, 5.1)], 0.7, mat=POLY_DARK, bevel=0.0012)
    pad = side("ButtPad", [(-33.6, 11.15), (-34.85, 11.0), (-34.95, -2.6), (-33.6, -2.85)], 1.95, mat=RUBBER, bevel=0.004, segments=3)
    cut(pad, *[box(f"PadGroove{k}", -3, 3, -35.2, -34.75, -1.6 + k * 1.15, -1.6 + k * 1.15 + 0.35, keep=False) for k in range(11)])


def build_magazine():
    """Cargador curvo de polímero (tipo PMAG): parte que entra en el brocal, cuerpo curvo, nervios y base."""
    side("MagTop", [(2.95, 1.0), (9.25, 1.0), (9.35, -3.8), (3.05, -3.8)], 1.0, mat=POLY_DARK, bone="magazine", bevel=0.0015)

    def edge(t, front):
        if front:
            return (9.4 + 3.9 * t ** 1.5, -3.6 - 15.2 * t)
        return (3.1 + 3.6 * t ** 1.5, -3.6 - 16.6 * t)

    ts = [i / 14 for i in range(15)]
    body = [edge(t, True) for t in ts] + [edge(t, False) for t in reversed(ts)]
    side("MagBody", body, 1.18, mat=POLY_DARK, bone="magazine", bevel=0.003, segments=2)

    def strip(t0, t1, n=4):
        tt = [t0 + (t1 - t0) * i / n for i in range(n + 1)]
        return [edge(t, True) for t in tt] + [edge(t, False) for t in reversed(tt)]

    side("MagLip", strip(0.0, 0.035, 1), 1.32, mat=POLY_DARK, bone="magazine", bevel=0.0012)
    for k in range(5):  # nervios de agarre en la parte baja
        t0 = 0.62 + k * 0.07
        side(f"MagRib{k}", strip(t0, t0 + 0.022, 2), 1.27, mat=POLY_DARK, bone="magazine", bevel=0.0008, segments=1)
    side("MagFloorplate", [(6.5, -19.9), (13.5, -18.5), (13.85, -19.3), (13.4, -19.95), (6.9, -21.2), (6.4, -20.8)], 1.3,
         mat=POLY_DARK, bone="magazine", bevel=0.002)


def build_geometry():
    build_upper()
    build_lower()
    build_handguard()
    build_barrel()
    build_sights()
    build_stock()
    build_magazine()


BONES = {
    # nombre: (padre, posición en cm)
    "root": (None, (0, 0, 0)),
    "magazine": ("root", (0, 6.0, -0.6)),
    "Mag": ("magazine", (0, 9.0, -20.2)),
    "bolt_carrier": ("root", (0, 6.5, 7.6)),
    "charging_handle": ("root", (0, -12.5, 10.0)),
    "trigger": ("root", (0, 1.3, -0.6)),
    "selector": ("root", (1.6, -2.2, 2.5)),
    "Sight": ("root", (0, -8.0, SIGHT_Z)),
    "Muzzle": ("root", (0, 54.0, BORE_Z)),
    "Eject": ("root", (-1.9, 6.5, 7.8)),
    "HandGrip_L": ("root", (0, 25.0, 1.0)),  # ~6,5 cm bajo el eje: la palma queda bajo el guardamanos
}


def build_armature():
    arm_data = bpy.data.armatures.new("AR7_Rig")
    arm = bpy.data.objects.new("Armature", arm_data)  # "Armature": Unreal no lo añade como hueso extra
    bpy.context.scene.collection.objects.link(arm)
    bpy.context.view_layer.objects.active = arm
    bpy.ops.object.mode_set(mode='EDIT')
    for name, (parent, pos) in BONES.items():
        b = arm_data.edit_bones.new(name)
        b.head = W(*pos)
        b.tail = W(pos[0], pos[1], pos[2] + 2.0)
        if parent:
            b.parent = arm_data.edit_bones[parent]
    bpy.ops.object.mode_set(mode='OBJECT')
    return arm


def apply_bevels():
    for obj, _ in PARTS:
        w, s = obj.get("bl_bevel", (0.0, 1))
        if w > 0:
            bl_lib.add_bevel(obj, width=w, segments=int(s), angle_deg=35)
        bl_lib.apply_modifiers(obj)


def uv_scale_cm(obj):
    """cm de superficie por unidad de UV (para que las texturas de detalle tengan tamaño real)."""
    me = obj.data
    uv = me.uv_layers.active.data
    a3, auv = 0.0, 0.0
    for p in me.polygons:
        a3 += p.area
        pts = [uv[li].uv for li in p.loop_indices]
        s = 0.0
        for i in range(len(pts)):
            x0, y0 = pts[i]
            x1, y1 = pts[(i + 1) % len(pts)]
            s += x0 * y1 - x1 * y0
        auv += abs(s) / 2
    return math.sqrt(a3 / max(auv, 1e-9)) * 100.0


def main():
    argv = sys.argv[sys.argv.index("--") + 1:] if "--" in sys.argv else []
    out = os.path.abspath(argv[0] if argv else os.path.join(os.path.dirname(__file__), "..", "export", "SK_AR7.fbx"))
    preview = next((a for a in argv[1:] if not a.startswith("--")), None)

    bl_lib.reset_scene()
    global MATERIALS
    MATERIALS = [
        bl_lib.get_material("MI_AR7_Metal", (0.03, 0.03, 0.033, 1), 0.45, 0.7),
        bl_lib.get_material("MI_AR7_Polymer", (0.30, 0.23, 0.15, 1), 0.7, 0.0),
        bl_lib.get_material("MI_AR7_PolymerDark", (0.035, 0.035, 0.035, 1), 0.6, 0.0),
        bl_lib.get_material("MI_AR7_Rubber", (0.015, 0.015, 0.015, 1), 0.9, 0.0),
        bl_lib.get_material("MI_AR7_Tritium", (1.0, 0.45, 0.05, 1), 0.3, 0.0),
    ]
    build_geometry()
    apply_bevels()

    # Grupos de vértices (100% a su hueso) y unir en una malla
    for obj, bone in PARTS:
        vg = obj.vertex_groups.new(name=bone)
        vg.add(list(range(len(obj.data.vertices))), 1.0, 'REPLACE')
    bpy.ops.object.select_all(action='DESELECT')
    for obj, _ in PARTS:
        obj.select_set(True)
    mesh_obj = PARTS[0][0]
    bpy.context.view_layer.objects.active = mesh_obj
    bpy.ops.object.join()
    mesh_obj.name = "SK_AR7"
    mesh_obj.data.name = "SK_AR7"

    # Normales ponderadas (superficies planas limpias con biseles suaves), triangulado (los n-gonos cóncavos
    # se triangulan aquí y no en Unreal) y UVs con densidad uniforme
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
    bpy.ops.object.select_all(action='DESELECT')
    mesh_obj.select_set(True)
    bpy.context.view_layer.objects.active = mesh_obj
    bpy.ops.object.mode_set(mode='EDIT')
    bpy.ops.mesh.select_all(action='SELECT')
    bpy.ops.uv.smart_project(angle_limit=math.radians(55), island_margin=0.004, area_weight=0.0)
    bpy.ops.uv.pack_islands(rotate=True, margin=0.003)
    bpy.ops.object.mode_set(mode='OBJECT')
    scale = uv_scale_cm(mesh_obj)
    print(f"[gen_ar7] UV: {scale:.1f} cm por unidad de UV")

    # Cargador suelto (se cambia de mano en la recarga): copia de las caras del hueso "magazine" con las mismas UV;
    # origen en el hueso
    mag_obj = extract_magazine(mesh_obj)
    mag_out = os.path.join(os.path.dirname(out), "SM_AR7_Mag.fbx")
    bl_lib.export_fbx([mag_obj], mag_out)
    mag_obj.hide_render = True

    if "--bake" in argv:
        tex = os.path.join(os.path.dirname(out), "T_AR7_Masks.png")
        bake_masks(mesh_obj, tex)
        with open(tex.replace(".png", ".json"), "w") as f:
            json.dump({"uv_scale_cm": scale}, f)

    arm = build_armature()
    mesh_obj.parent = arm
    mod = mesh_obj.modifiers.new("Armature", 'ARMATURE')
    mod.object = arm

    tris = bl_lib.triangle_count(mesh_obj)
    print(f"[gen_ar7] SK_AR7: {tris} tris, {len(BONES)} huesos; SM_AR7_Mag: {bl_lib.triangle_count(mag_obj)} tris")

    os.makedirs(os.path.dirname(out), exist_ok=True)
    bpy.ops.object.select_all(action='DESELECT')
    arm.select_set(True)
    mesh_obj.select_set(True)
    bpy.context.view_layer.objects.active = arm
    bpy.ops.export_scene.fbx(
        filepath=out, use_selection=True, apply_unit_scale=True, apply_scale_options='FBX_SCALE_UNITS',
        axis_forward='-Y', axis_up='Z', object_types={'MESH', 'ARMATURE'}, mesh_smooth_type='FACE',
        use_tspace=True, add_leaf_bones=False, primary_bone_axis='Y', secondary_bone_axis='X',
        armature_nodetype='NULL', use_armature_deform_only=False, bake_anim=False)
    print(f"[gen_ar7] Exportado: {out}")

    if preview:
        render_preview(mesh_obj, preview)


def extract_magazine(weapon):
    mag = weapon.copy()
    mag.data = weapon.data.copy()
    mag.name = mag.data.name = "SM_AR7_Mag"
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


def gaussian_blur(img, sigma):
    """Desenfoque gaussiano separable (numpy) para limpiar el ruido del horneado."""
    import numpy as np
    r = max(1, int(sigma * 3))
    k = np.exp(-0.5 * (np.arange(-r, r + 1) / sigma) ** 2)
    k /= k.sum()
    out = img
    for axis in (0, 1):
        pad = [(0, 0), (0, 0)]
        pad[axis] = (r, r)
        p = np.pad(out, pad, mode='edge')
        acc = np.zeros_like(out)
        for i, w in enumerate(k):
            sl = [slice(None), slice(None)]
            sl[axis] = slice(i, i + out.shape[axis])
            acc += w * p[tuple(sl)]
        out = acc
    return out


def bake_masks(obj, path, size=2048):
    """Hornea máscaras del arma en una textura RGB: R = oclusión ambiental, G = aristas convexas
    (desgaste), B = cavidades (suciedad). Cycles en CPU con muchas muestras + desenfoque suave:
    el ruido del horneado se veía como 'lija' al apuntar."""
    import numpy as np
    scene = bpy.context.scene
    scene.render.engine = 'CYCLES'
    scene.cycles.device = 'CPU'
    scene.cycles.samples = 160
    scene.render.bake.margin = 12
    scene.world = scene.world or bpy.data.worlds.new("BakeWorld")
    bpy.ops.object.select_all(action='DESELECT')
    obj.select_set(True)
    bpy.context.view_layer.objects.active = obj

    def bake(kind, emit_builder=None):
        img = bpy.data.images.new(f"bake_{kind}", size, size, alpha=False, float_buffer=True)
        originals = [s.material for s in obj.material_slots]
        bake_mat = bpy.data.materials.new(f"Bake_{kind}")
        bake_mat.use_nodes = True
        nt = bake_mat.node_tree
        if emit_builder:
            emit_builder(nt)
        tex = nt.nodes.new("ShaderNodeTexImage")
        tex.image = img
        nt.nodes.active = tex
        for s in obj.material_slots:
            s.material = bake_mat
        bpy.ops.object.bake(type=kind)
        for s, m in zip(obj.material_slots, originals):
            s.material = m
        px = np.empty(size * size * 4, dtype=np.float32)
        img.pixels.foreach_get(px)
        return px.reshape(size, size, 4)[:, :, 0]

    def edges(nt):
        # Aristas: diferencia entre la normal real y la normal "redondeada" del nodo Bevel
        out = nt.nodes["Material Output"]
        geo = nt.nodes.new("ShaderNodeNewGeometry")
        bev = nt.nodes.new("ShaderNodeBevel")
        bev.samples = 32
        bev.inputs["Radius"].default_value = 0.0025
        dot = nt.nodes.new("ShaderNodeVectorMath")
        dot.operation = 'DOT_PRODUCT'
        nt.links.new(bev.outputs["Normal"], dot.inputs[0])
        nt.links.new(geo.outputs["Normal"], dot.inputs[1])
        inv = nt.nodes.new("ShaderNodeMath")
        inv.operation = 'SUBTRACT'
        inv.inputs[0].default_value = 1.0
        nt.links.new(dot.outputs["Value"], inv.inputs[1])
        emit = nt.nodes.new("ShaderNodeEmission")
        nt.links.new(inv.outputs["Value"], emit.inputs["Color"])
        nt.links.new(emit.outputs["Emission"], out.inputs["Surface"])

    ao = gaussian_blur(bake('AO'), 1.4)
    edge_raw = gaussian_blur(bake('EMIT', edges), 1.0)
    # convexo = arista y bien expuesta (AO alta) -> desgaste; cóncavo/ocluido -> suciedad
    edge = np.clip(edge_raw * 7.0 - 0.08, 0.0, 1.0) * np.clip((ao - 0.6) * 3.0, 0.0, 1.0)
    edge = edge * edge * (3 - 2 * edge)
    cavity = np.clip((0.82 - ao) * 2.0, 0.0, 1.0) ** 1.5
    rgba = np.stack([ao, edge, cavity, np.ones_like(ao)], axis=-1).astype(np.float32)
    out = bpy.data.images.new("T_AR7_Masks", size, size, alpha=False)
    out.pixels.foreach_set(rgba.ravel())
    out.filepath_raw = path
    out.file_format = 'PNG'
    out.save()
    print(f"[gen_ar7] Máscaras horneadas: {path}")


def render_preview(obj, path):
    """Render Workbench: lado derecho, lado izquierdo (el que ve el jugador), 3/4 y detalle del receptor."""
    scene = bpy.context.scene
    scene.render.engine = 'BLENDER_WORKBENCH'
    scene.display.shading.light = 'STUDIO'
    scene.display.shading.color_type = 'MATERIAL'
    scene.display.shading.show_cavity = True
    scene.display.shading.cavity_type = 'BOTH'
    scene.render.resolution_x, scene.render.resolution_y = 1600, 900
    scene.render.film_transparent = False
    scene.world = bpy.data.worlds.new("W")
    scene.world.color = (0.55, 0.57, 0.6)
    cam_data = bpy.data.cameras.new("Cam")
    cam_data.type = 'ORTHO'
    cam = bpy.data.objects.new("Cam", cam_data)
    scene.collection.objects.link(cam)
    scene.camera = cam
    views = (("_right", Vector((-1, 0, 0.12)), 0.95, W(0, 9, 0)), ("_left", Vector((1, 0, 0.12)), 0.95, W(0, 9, 0)),
             ("_34", Vector((0.8, 0.55, 0.45)), 0.95, W(0, 9, 0)), ("_detail", Vector((0.85, 0.35, 0.5)), 0.42, W(0, -2, 5)))
    for suffix, direction, ortho, center in views:
        d = direction.normalized()
        cam_data.ortho_scale = ortho
        cam.location = center + d * 2.0
        cam.rotation_euler = (-d).to_track_quat('-Z', 'Y').to_euler()
        scene.render.filepath = path.replace(".png", f"{suffix}.png")
        bpy.ops.render.render(write_still=True)
        print(f"[gen_ar7] Preview: {scene.render.filepath}")


if __name__ == "__main__":
    main()
