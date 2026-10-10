"""Granada M-6 (Bloque 11): importa ArtSource/Blender/export/SM_Grenade_M6.fbx (gen_grenade.py) a /Game/Weapons/Grenade.
Sin Nanite (pequeña y con física). Material del cuerpo: verde oliva pintado (M_Prop_Simple).

Uso: UnrealEditor-Cmd.exe Blackline.uproject -run=pythonscript -script="<ruta>/Tools/UnrealPython/setup_grenade_assets.py"
"""
import os
import unreal

PROJECT = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())
FBX = os.path.join(PROJECT, "ArtSource", "Blender", "export", "SM_Grenade_M6.fbx")
DEST = "/Game/Weapons/Grenade"
MAT = "/Game/Environment/Materials"
tools = unreal.AssetToolsHelpers.get_asset_tools()
lib = unreal.EditorAssetLibrary
mel = unreal.MaterialEditingLibrary


def load(p):
    return unreal.load_asset(p) if lib.does_asset_exist(p) else None


body = load(f"{DEST}/MI_Grenade_Body") or tools.create_asset("MI_Grenade_Body", DEST, unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
body.set_editor_property("parent", load(f"{MAT}/M_Prop_Simple"))
mel.set_material_instance_vector_parameter_value(body, "Color", unreal.LinearColor(0.11, 0.13, 0.08, 1))
mel.set_material_instance_scalar_parameter_value(body, "Roughness", 0.55)
mel.set_material_instance_scalar_parameter_value(body, "Metallic", 0.2)
lib.save_loaded_asset(body)
mats = {"MI_Grenade_Body": body, "MI_Veh_Steel": load(f"{MAT}/MI_Veh_Steel"), "MI_Veh_Trim": load(f"{MAT}/MI_Veh_Trim")}

task = unreal.AssetImportTask()
task.filename = FBX
task.destination_path = DEST
task.automated = True
task.replace_existing = True
task.save = True
tools.import_asset_tasks([task])
for p in task.imported_object_paths:
    a = unreal.load_asset(p)
    if isinstance(a, unreal.StaticMesh):
        new = []
        for sm in a.get_editor_property("static_materials"):
            slot = str(sm.get_editor_property("material_slot_name"))
            new.append(unreal.StaticMaterial(material_interface=mats[slot], material_slot_name=slot, uv_channel_data=sm.get_editor_property("uv_channel_data")))
        a.set_editor_property("static_materials", new)
        ns = a.get_editor_property("nanite_settings")
        ns.set_editor_property("enabled", False)
        a.set_editor_property("nanite_settings", ns)
        lib.save_loaded_asset(a)
        unreal.log(f"[BL_Grenade] {a.get_name()}")
    elif isinstance(a, unreal.MaterialInterface) and a.get_path_name().startswith(DEST) and a.get_name() != "MI_Grenade_Body":
        lib.delete_loaded_asset(a)
unreal.log("[BL_Grenade] OK")
