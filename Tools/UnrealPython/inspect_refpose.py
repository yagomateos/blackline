"""Vuelca la pose de referencia (espacio de componente) de algunos huesos del Mannequin al log ([BL_RefPose]).
Sirve para modelar en Blender piezas que se enganchan a huesos (equipo del miliciano, Bloque 8).

Uso: UnrealEditor-Cmd.exe Blackline.uproject -run=pythonscript -script="<ruta>/Tools/UnrealPython/inspect_refpose.py"
"""
import unreal

MESH = "/Game/Characters/Mannequins/Meshes/SKM_Manny_Simple"
BONES = ["pelvis", "spine_01", "spine_03", "spine_05", "neck_01", "head", "clavicle_l", "upperarm_l", "upperarm_r", "thigh_l", "thigh_r"]

mesh = unreal.load_asset(MESH)
pose = unreal.AnimPoseExtensions.get_reference_pose(mesh.skeleton)
for b in BONES:
    t = unreal.AnimPoseExtensions.get_bone_pose(pose, b, unreal.AnimPoseSpaces.WORLD)
    r = t.rotation.rotator()
    unreal.log(f"[BL_RefPose] {b}: loc=({t.translation.x:.2f}, {t.translation.y:.2f}, {t.translation.z:.2f}) "
               f"rot=(p {r.pitch:.1f}, y {r.yaw:.1f}, r {r.roll:.1f})")
b = mesh.get_bounds()
unreal.log(f"[BL_RefPose] bounds origin={b.origin} extent={b.box_extent}")
