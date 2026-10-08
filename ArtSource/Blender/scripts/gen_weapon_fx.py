"""Mallas pequeñas de efectos de arma: casquillo 5.56, fogonazo y esquirla de impacto.

Uso:
  blender -b --factory-startup -P gen_weapon_fx.py -- <carpeta_salida>

Salida (FBX, un archivo por malla):
  SM_Casing_556       ~60 tris  · 5,7 cm x Ø0,96 cm · eje del casquillo = +X · material MI_Brass
  SM_AR7_MuzzleFlash  ~12 tris  · 22 cm hacia +X desde el origen (boca del cañón) · MI_MuzzleFlash
  SM_ImpactChip       ~80 tris  · ~1,2 cm, irregular · MI_ImpactChip
"""
import math
import os
import random
import sys

sys.path.append(os.path.dirname(os.path.abspath(__file__)))
import bmesh
import bpy
from mathutils import Vector
import bl_lib


def lathe(bm, profile, segments, mat_index=0):
    """Sólido de revolución alrededor del eje X. profile = [(x, radio), ...] de atrás a delante."""
    rings = []
    for x, r in profile:
        ring = []
        for i in range(segments):
            a = 2 * math.pi * i / segments
            ring.append(bm.verts.new((x, r * math.cos(a), r * math.sin(a))))
        rings.append(ring)
    for a, b in zip(rings, rings[1:]):
        for i in range(segments):
            j = (i + 1) % segments
            f = bm.faces.new((a[i], a[j], b[j], b[i]))
            f.material_index = mat_index
    for ring, reverse in ((rings[0], True), (rings[-1], False)):
        f = bm.faces.new(list(reversed(ring)) if reverse else ring)
        f.material_index = mat_index
    bmesh.ops.recalc_face_normals(bm, faces=bm.faces)


def casing():
    bm = bmesh.new()
    # x (m), radio (m): culote con ranura, cuerpo, hombro, cuello
    profile = [
        (0.0000, 0.00470), (0.0012, 0.00470), (0.0014, 0.00400), (0.0028, 0.00400),
        (0.0032, 0.00480), (0.0400, 0.00455), (0.0440, 0.00320), (0.0570, 0.00310),
    ]
    lathe(bm, profile, 8)
    mat = bl_lib.get_material("MI_Brass", (0.78, 0.56, 0.24, 1), 0.3, 1.0)
    obj = bl_lib.bm_to_object(bm, "SM_Casing_556", [mat])
    bpy.context.view_layer.objects.active = obj
    bpy.ops.object.shade_smooth()
    return obj


def muzzle_flash():
    bm = bmesh.new()
    uv = bm.loops.layers.uv.new("UVMap")

    def quad(corners, uvs, mat_index=0):
        verts = [bm.verts.new(c) for c in corners]
        f = bm.faces.new(verts)
        f.material_index = mat_index
        for loop, t in zip(f.loops, uvs):
            loop[uv].uv = t
        return f

    L, W = 0.075, 0.022
    full = [(0, 0), (1, 0), (1, 1), (0, 1)]
    # Dos planos cruzados a lo largo del cañón (llama lateral)
    quad([(0, -W, 0), (L, -W, 0), (L, W, 0), (0, W, 0)], full)
    quad([(0, 0, -W), (L, 0, -W), (L, 0, W), (0, 0, W)], full)
    # Estrella frontal (vista desde detrás, en ADS)
    S = 0.045
    quad([(0.01, -S, -S), (0.01, S, -S), (0.01, S, S), (0.01, -S, S)], full, 1)
    # (sin chorros laterales: con apagallamas de jaula el fogonazo de un 5.56 es pequeño y compacto)
    flame = bl_lib.get_material("MI_MuzzleFlame", (1.0, 0.7, 0.3, 1), 1.0, 0.0)
    star = bl_lib.get_material("MI_MuzzleStar", (1.0, 0.8, 0.5, 1), 1.0, 0.0)
    return bl_lib.bm_to_object(bm, "SM_AR7_MuzzleFlash", [flame, star])


def impact_chip(seed=7):
    rng = random.Random(seed)
    bm = bmesh.new()
    bmesh.ops.create_icosphere(bm, subdivisions=1, radius=0.006)
    for v in bm.verts:
        v.co *= rng.uniform(0.6, 1.25)
    bmesh.ops.scale(bm, vec=Vector((1.0, 0.8, 0.55)), verts=bm.verts)
    mat = bl_lib.get_material("MI_ImpactChip", (0.35, 0.34, 0.32, 1), 0.9, 0.0)
    return bl_lib.bm_to_object(bm, "SM_ImpactChip", [mat])


def splinter(seed=3):
    """Astilla de madera: prisma alargado e irregular (~3 cm)."""
    rng = random.Random(seed)
    bm = bmesh.new()
    bmesh.ops.create_cube(bm, size=1.0)
    bmesh.ops.scale(bm, vec=Vector((0.03, 0.004, 0.0025)), verts=bm.verts)
    for v in bm.verts:
        v.co.y += rng.uniform(-0.0015, 0.0015)
        v.co.z += rng.uniform(-0.001, 0.001)
        if v.co.x > 0:
            v.co.y *= 0.3  # punta
    mat = bl_lib.get_material("MI_Splinter", (0.45, 0.32, 0.2, 1), 0.85, 0.0)
    return bl_lib.bm_to_object(bm, "SM_Debris_Splinter", [mat])


def glass_shard(seed=5):
    """Esquirla de cristal: triángulo fino (~2,5 cm) con algo de grosor."""
    rng = random.Random(seed)
    bm = bmesh.new()
    pts = [(0, 0), (rng.uniform(0.018, 0.026), rng.uniform(-0.004, 0.004)), (rng.uniform(0.006, 0.012), rng.uniform(0.012, 0.02))]
    top = [bm.verts.new((x, y, 0.0006)) for x, y in pts]
    bot = [bm.verts.new((x, y, -0.0006)) for x, y in pts]
    bm.faces.new(top)
    bm.faces.new(list(reversed(bot)))
    for i in range(3):
        j = (i + 1) % 3
        bm.faces.new((top[i], bot[i], bot[j], top[j]))
    bmesh.ops.recalc_face_normals(bm, faces=bm.faces)
    mat = bl_lib.get_material("MI_GlassShard", (0.8, 0.85, 0.9, 1), 0.05, 0.0)
    return bl_lib.bm_to_object(bm, "SM_Debris_GlassShard", [mat])


def dirt_clod(seed=9):
    rng = random.Random(seed)
    bm = bmesh.new()
    bmesh.ops.create_icosphere(bm, subdivisions=1, radius=0.009)
    for v in bm.verts:
        v.co *= rng.uniform(0.55, 1.3)
    mat = bl_lib.get_material("MI_DirtClod", (0.2, 0.15, 0.1, 1), 0.95, 0.0)
    return bl_lib.bm_to_object(bm, "SM_Debris_Clod", [mat])


def fx_quad():
    """Cuadrado 1x1 m (se escala por partícula) mirando a -X (hacia la cámara), con UV."""
    bm = bmesh.new()
    uv = bm.loops.layers.uv.new("UVMap")
    vs = [bm.verts.new((0, y, z)) for y, z in ((-0.5, -0.5), (0.5, -0.5), (0.5, 0.5), (-0.5, 0.5))]
    f = bm.faces.new(vs)
    for loop, t in zip(f.loops, ((0, 0), (1, 0), (1, 1), (0, 1))):
        loop[uv].uv = t
    mat = bl_lib.get_material("MI_FXQuad", (1, 1, 1, 1), 1.0, 0.0)
    return bl_lib.bm_to_object(bm, "SM_FX_Quad", [mat])


if __name__ == "__main__":
    argv = sys.argv[sys.argv.index("--") + 1:] if "--" in sys.argv else []
    out_dir = os.path.abspath(argv[0] if argv else os.path.join(os.path.dirname(__file__), "..", "export"))
    for build in (casing, muzzle_flash, impact_chip, splinter, glass_shard, dirt_clod, fx_quad):
        bl_lib.reset_scene()
        obj = build()
        print(f"[gen_weapon_fx] {obj.name}: {bl_lib.triangle_count(obj)} tris")
        bl_lib.export_fbx([obj], os.path.join(out_dir, obj.name + ".fbx"))
