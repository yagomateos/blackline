"""Bloque 9: marca los usos que necesitan los materiales creados por script y vuelve a guardarlos. Idempotente.

En el editor, si a un material le falta un uso (Nanite, instancias, malla esquelética) se compila al vuelo y se
avisa ("missing usage flag ... Default Material will be used in game"); en una build empaquetada se vería el
material por defecto. Aquí se marcan en los maestros y se guardan también las instancias.

Uso: UnrealEditor-Cmd.exe Blackline.uproject -run=pythonscript -script="<ruta>/Tools/UnrealPython/fix_material_usage.py"
"""
import unreal

lib = unreal.EditorAssetLibrary
mel = unreal.MaterialEditingLibrary
FOLDERS = ["/Game/Environment", "/Game/Characters/Enemy", "/Game/FX", "/Game/Weapons"]
# Usos por carpeta: entorno y equipo van en mallas Nanite (y las fachadas en instancias)
FLAGS = ["used_with_nanite", "used_with_instanced_static_meshes"]


def main():
    fixed, saved = 0, 0
    for folder in FOLDERS:
        for path in lib.list_assets(folder, recursive=True, include_folder=False):
            asset = unreal.load_asset(path.split(".")[0])
            if isinstance(asset, unreal.Material):
                # Los materiales translúcidos/aditivos/decals no pueden ir en Nanite: solo instancias
                blend = asset.get_editor_property("blend_mode")
                domain = asset.get_editor_property("material_domain")
                opaque = blend in (unreal.BlendMode.BLEND_OPAQUE, unreal.BlendMode.BLEND_MASKED) and domain == unreal.MaterialDomain.MD_SURFACE
                changed = False
                for flag in FLAGS:
                    if flag == "used_with_nanite" and not opaque:
                        continue
                    if not asset.get_editor_property(flag):
                        asset.set_editor_property(flag, True)
                        changed = True
                if changed:
                    mel.recompile_material(asset)
                    fixed += 1
                    unreal.log(f"[BL_MatUsage] {asset.get_path_name()}")
                lib.save_loaded_asset(asset, only_if_is_dirty=False)
                saved += 1
            elif isinstance(asset, (unreal.MaterialInstanceConstant, unreal.StaticMesh)):
                lib.save_loaded_asset(asset, only_if_is_dirty=False)
                saved += 1
    unreal.log(f"[BL_MatUsage] OK: {fixed} maestros corregidos, {saved} assets guardados")


main()
