"""Vuelca sockets, huesos y bounds de mallas esqueléticas al log ([BL_Inspect]).

Uso: UnrealEditor.exe Blackline.uproject -ExecutePythonScript="Tools/UnrealPython/inspect_meshes.py"
Editar MESHES para inspeccionar otras.
"""
import unreal

MESHES = [
    "/Game/Weapons/Rifle/Meshes/SKM_Rifle",
    "/Game/Characters/Mannequins/Meshes/SKM_Manny_Simple",
]


def log(m):
    unreal.log(f"[BL_Inspect] {m}")


for path in MESHES:
    mesh = unreal.load_asset(path)
    if not mesh:
        log(f"No existe {path}")
        continue
    b = mesh.get_bounds()
    log(f"== {path}  bounds origin={b.origin} extent={b.box_extent}")
    for i in range(mesh.num_sockets()):
        s = mesh.get_socket_by_index(i)
        log(f"  socket {s.socket_name} -> hueso {s.bone_name} loc={s.relative_location} rot={s.relative_rotation}")
    skel = mesh.skeleton
    if skel:
        names = []
        try:
            ref = unreal.SkeletalMeshEditorSubsystem  # noqa: F401 (solo para comprobar disponibilidad)
        except Exception:
            pass
        # Nombres de huesos via la referencia de la malla
        try:
            bone_names = [str(n) for n in unreal.SkeletalMeshLibrary.get_bone_names(mesh)]  # puede no existir
        except Exception:
            bone_names = []
        if not bone_names:
            try:
                bone_names = [str(skel.get_bone_name(i)) for i in range(200)]
            except Exception:
                bone_names = []
        log(f"  huesos ({len(bone_names)}): {', '.join(bone_names[:80])}")
