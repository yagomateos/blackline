"""Utilidades compartidas para los generadores de assets de BLACKLINE.

Convenciones:
- Unidades de Blender en metros (escala 1.0). Unreal importa 1 m = 100 cm.
- Eje +Y de Blender = fachada "frontal"; Z arriba.
- Nombres: SM_ (static mesh), SK_ (skeletal), UCX_<nombre> para colisiones custom.
"""
import os
import bpy
import bmesh
from mathutils import Matrix, Vector


def reset_scene():
    bpy.ops.wm.read_factory_settings(use_empty=True)
    scene = bpy.context.scene
    scene.unit_settings.system = 'METRIC'
    scene.unit_settings.scale_length = 1.0


def get_material(name, color=(0.5, 0.5, 0.5, 1.0), roughness=0.8, metallic=0.0):
    mat = bpy.data.materials.get(name)
    if mat is None:
        mat = bpy.data.materials.new(name)
        mat.use_nodes = True
        bsdf = mat.node_tree.nodes.get("Principled BSDF")
        if bsdf:
            bsdf.inputs["Base Color"].default_value = color
            bsdf.inputs["Roughness"].default_value = roughness
            bsdf.inputs["Metallic"].default_value = metallic
        mat.diffuse_color = color
    return mat


def add_box(bm, center, size, mat_index=0):
    """Añade una caja alineada a ejes. center/size en metros."""
    res = bmesh.ops.create_cube(bm, size=1.0)
    verts = res["verts"]
    m = Matrix.Translation(Vector(center)) @ Matrix.Diagonal(Vector((size[0], size[1], size[2], 1.0)))
    bmesh.ops.transform(bm, matrix=m, verts=verts)
    faces = {f for v in verts for f in v.link_faces}
    for f in faces:
        f.material_index = mat_index
    return verts


def add_cylinder(bm, center, radius, depth, segments=16, mat_index=0):
    res = bmesh.ops.create_cone(bm, cap_ends=True, segments=segments,
                                radius1=radius, radius2=radius, depth=depth)
    verts = res["verts"]
    bmesh.ops.translate(bm, vec=Vector(center), verts=verts)
    for f in {f for v in verts for f in v.link_faces}:
        f.material_index = mat_index
    return verts


def bm_to_object(bm, name, materials):
    mesh = bpy.data.meshes.new(name)
    bm.to_mesh(mesh)
    bm.free()
    obj = bpy.data.objects.new(name, mesh)
    bpy.context.scene.collection.objects.link(obj)
    for mat in materials:
        mesh.materials.append(mat)
    return obj


def add_bevel(obj, width=0.01, segments=2, angle_deg=40.0):
    mod = obj.modifiers.new("Bevel", 'BEVEL')
    mod.width = width
    mod.segments = segments
    mod.limit_method = 'ANGLE'
    mod.angle_limit = angle_deg * 3.14159265 / 180.0
    mod.harden_normals = True
    return mod


def apply_modifiers(obj):
    bpy.context.view_layer.objects.active = obj
    obj.select_set(True)
    for mod in list(obj.modifiers):
        bpy.ops.object.modifier_apply(modifier=mod.name)
    obj.select_set(False)


def triangle_count(obj):
    depsgraph = bpy.context.evaluated_depsgraph_get()
    eval_obj = obj.evaluated_get(depsgraph)
    mesh = eval_obj.to_mesh()
    mesh.calc_loop_triangles()
    n = len(mesh.loop_triangles)
    eval_obj.to_mesh_clear()
    return n


def export_fbx(objects, filepath):
    """Exporta objetos seleccionados a FBX con ajustes compatibles con Unreal."""
    os.makedirs(os.path.dirname(filepath), exist_ok=True)
    bpy.ops.object.select_all(action='DESELECT')
    for o in objects:
        o.select_set(True)
    bpy.context.view_layer.objects.active = objects[0]
    bpy.ops.export_scene.fbx(
        filepath=filepath,
        use_selection=True,
        apply_unit_scale=True,
        apply_scale_options='FBX_SCALE_UNITS',
        axis_forward='-Y',
        axis_up='Z',
        object_types={'MESH', 'ARMATURE', 'EMPTY'},
        mesh_smooth_type='FACE',
        use_tspace=True,
        add_leaf_bones=False,
        bake_anim=False,
    )
    print(f"[bl_lib] Exportado: {filepath}")
