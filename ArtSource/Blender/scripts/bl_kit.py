"""Primitivas de modelado por script en coordenadas de Unreal (cm, +X adelante, +Y derecha, +Z arriba).

Mismo enfoque que gen_ar7.py (perfiles extruidos, sólidos de revolución, booleanas exactas con bisel aplicado al final),
pero reutilizable para props de entorno. Cada malla se construye en un Kit (sus piezas y materiales) y se exporta
con sus colisiones UCX_ (cajas/convexos simples) a un FBX propio.

Conversión: Unreal (x, y, z) cm -> Blender (x, -y, z) m. Exportado con forward -Y / up Z, Unreal recibe (x, y, z).
"""
import math

import bmesh
import bpy
from mathutils import Matrix, Vector

import bl_lib


def W(x, y, z):
    return Vector((x / 100.0, -y / 100.0, z / 100.0))


class Kit:
    def __init__(self, name, materials):
        """materials: lista de (nombre_slot, color RGBA, rugosidad, metálico)."""
        self.name = name
        self.mats = [bl_lib.get_material(n, c, r, m) for n, c, r, m in materials]
        self.parts = []
        self.collisions = []

    # ------------------------------------------------------------------ piezas
    def _finish(self, bm, name, mat, bevel, segments, keep):
        for f in bm.faces:
            f.material_index = mat
        bmesh.ops.recalc_face_normals(bm, faces=bm.faces)
        obj = bl_lib.bm_to_object(bm, name, self.mats)
        if keep:
            obj["bl_bevel"] = (bevel, segments)
            self.parts.append(obj)
        return obj

    def prism(self, name, pts, axis, w0, w1, mat=0, bevel=0.01, segments=2, keep=True):
        """Polígono extruido. axis 'x': pts=(y,z) a lo largo de X; 'y': pts=(x,z) a lo largo de Y; 'z': pts=(x,y) en Z."""
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
        return self._finish(bm, name, mat, bevel, segments, keep)

    def box(self, name, x0, x1, y0, y1, z0, z1, mat=0, bevel=0.01, segments=2, keep=True):
        return self.prism(name, [(y0, z0), (y1, z0), (y1, z1), (y0, z1)], 'x', x0, x1, mat, bevel, segments, keep)

    def lathe(self, name, prof, origin, direction, seg=16, closed=False, mat=0, bevel=0.0, segments=1, keep=True):
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
                a = 2 * math.pi * (i + 0.5) / seg
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
        return self._finish(bm, name, mat, bevel, segments, keep)

    def rounded_box(self, name, cx, cy, cz, sx, sy, sz, radius, mat=0, segments=3, keep=True):
        """Caja con esquinas muy redondeadas (sacos, cojines): bisel grande en todas las aristas."""
        obj = self.box(name, cx - sx / 2, cx + sx / 2, cy - sy / 2, cy + sy / 2, cz - sz / 2, cz + sz / 2, mat, 0.0, 1, keep)
        if keep:
            obj["bl_bevel"] = (radius / 100.0, segments)
        return obj

    def cut(self, obj, *cutters):
        cutters = [c for c in cutters if c is not None]
        if not cutters:
            return obj
        bpy.ops.object.select_all(action='DESELECT')
        for c in cutters:
            c.select_set(True)
        bpy.context.view_layer.objects.active = cutters[0]
        if len(cutters) > 1:
            bpy.ops.object.join()
        mod = obj.modifiers.new("Cut", 'BOOLEAN')
        mod.operation = 'DIFFERENCE'
        mod.solver = 'EXACT'
        mod.object = cutters[0]
        mod.use_self = len(cutters) > 1
        bl_lib.apply_modifiers(obj)
        bpy.data.objects.remove(cutters[0])
        return obj

    def transform(self, obj, matrix_ue):
        """Aplica una transformación expresada en el espacio de Unreal (cm) a una pieza."""
        S = Matrix.Diagonal((0.01, -0.01, 0.01, 1.0))
        obj.data.transform(S @ matrix_ue @ S.inverted())
        return obj

    # ------------------------------------------------------------------ colisión
    def collision_box(self, x0, x1, y0, y1, z0, z1, rot_z_deg=0.0, pivot=(0.0, 0.0)):
        """Caja de colisión UCX (opcionalmente girada en Z alrededor de pivot)."""
        obj = self.box(f"UCX_{self.name}_{len(self.collisions):02d}", x0, x1, y0, y1, z0, z1, bevel=0.0, keep=False)
        if rot_z_deg:
            px, py = pivot
            m = Matrix.Translation((px, py, 0)) @ Matrix.Rotation(math.radians(rot_z_deg), 4, 'Z') @ Matrix.Translation((-px, -py, 0))
            self.transform(obj, m)
        obj.data.materials.clear()
        self.collisions.append(obj)
        return obj

    def collision_hull(self, pts_yz, x0, x1):
        """Convexo extruido en X (perfil convexo en YZ)."""
        obj = self.prism(f"UCX_{self.name}_{len(self.collisions):02d}", pts_yz, 'x', x0, x1, bevel=0.0, keep=False)
        obj.data.materials.clear()
        self.collisions.append(obj)
        return obj

    # ------------------------------------------------------------------ salida
    def build(self):
        """Biseles, unión, normales ponderadas, triangulado y UV (box project, densidad uniforme). Devuelve la malla."""
        for obj in self.parts:
            w, s = obj.get("bl_bevel", (0.0, 1))
            if w > 0:
                bl_lib.add_bevel(obj, width=w, segments=int(s), angle_deg=35)
            bl_lib.apply_modifiers(obj)
        bpy.ops.object.select_all(action='DESELECT')
        for obj in self.parts:
            obj.select_set(True)
        mesh = self.parts[0]
        bpy.context.view_layer.objects.active = mesh
        if len(self.parts) > 1:
            bpy.ops.object.join()
        mesh.name = mesh.data.name = self.name
        bpy.ops.object.shade_smooth_by_angle(angle=math.radians(35), keep_sharp_edges=True)
        wn = mesh.modifiers.new("WN", 'WEIGHTED_NORMAL')
        wn.keep_sharp = True
        tri = mesh.modifiers.new("Tri", 'TRIANGULATE')
        tri.quad_method = 'BEAUTY'
        tri.ngon_method = 'BEAUTY'
        tri.keep_custom_normals = True
        bl_lib.apply_modifiers(mesh)
        bpy.ops.object.select_all(action='DESELECT')
        mesh.select_set(True)
        bpy.context.view_layer.objects.active = mesh
        bpy.ops.object.mode_set(mode='EDIT')
        bpy.ops.mesh.select_all(action='SELECT')
        bpy.ops.uv.cube_project(cube_size=1.0)   # 1 m por unidad de UV (el material de entorno es triplanar; UV de reserva)
        bpy.ops.object.mode_set(mode='OBJECT')
        self.mesh = mesh
        return mesh

    def export(self, path):
        bl_lib.export_fbx([self.mesh] + self.collisions, path)
        return bl_lib.triangle_count(self.mesh)
