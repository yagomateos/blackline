"""Marca "Used with Skeletal Mesh" en los materiales base de las armas (mallas esqueléticas). Idempotente.
Sin ese uso, en la build empaquetada el arma se ve con el material por defecto (la SG-12 lo tenía en 3 ranuras).
Uso: UnrealEditor-Cmd.exe Blackline.uproject -run=pythonscript -script=".../fix_skeletal_usage.py"
"""
import unreal

lib = unreal.EditorAssetLibrary
mel = unreal.MaterialEditingLibrary
fixed = 0
for folder in ("/Game/Weapons", "/Game/Characters/Enemy"):
    for path in lib.list_assets(folder, recursive=True, include_folder=False):
        a = unreal.load_asset(path.split(".")[0])
        if not isinstance(a, unreal.SkeletalMesh):
            continue
        for sm in a.get_editor_property("materials"):
            mi = sm.get_editor_property("material_interface")
            base = mi.get_base_material() if mi else None
            if base and not base.get_editor_property("used_with_skeletal_mesh"):
                base.set_editor_property("used_with_skeletal_mesh", True)
                mel.recompile_material(base)
                lib.save_loaded_asset(base)
                fixed += 1
                unreal.log_warning(f"[BL_Skel] {a.get_name()}: {base.get_path_name()} ahora con uso de malla esquelética")
unreal.log_warning(f"[BL_Skel] materiales corregidos: {fixed}")
