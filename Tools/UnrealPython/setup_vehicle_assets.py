"""Blindado BTR de la Columna (Bloque 11, fase 8): importa ArtSource/Blender/export/Vehicles/SM_BTR_*.fbx (gen_btr.py)
a /Game/Vehicles/BTR y el helicóptero (gen_heli.py) a /Game/Vehicles/Heli. El casco va con Nanite; la torreta (gira) también. Pintura: verde oliva desgastado (M_Prop_Simple).

Uso: UnrealEditor-Cmd.exe Blackline.uproject -run=pythonscript -script="<ruta>/Tools/UnrealPython/setup_vehicle_assets.py"
"""
import os
import unreal

PROJECT = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())
SRC = os.path.join(PROJECT, "ArtSource", "Blender", "export", "Vehicles")
DEST = "/Game/Vehicles/BTR"
MAT = "/Game/Environment/Materials"
tools = unreal.AssetToolsHelpers.get_asset_tools()
lib = unreal.EditorAssetLibrary
mel = unreal.MaterialEditingLibrary


def load(p):
    return unreal.load_asset(p) if lib.does_asset_exist(p) else None


def tint(name, dest, color, rough, metal):
    mi = load(f"{dest}/{name}") or tools.create_asset(name, dest, unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
    mi.set_editor_property("parent", load(f"{MAT}/M_Prop_Simple"))
    mel.set_material_instance_vector_parameter_value(mi, "Color", unreal.LinearColor(*color, 1))
    mel.set_material_instance_scalar_parameter_value(mi, "Roughness", rough)
    mel.set_material_instance_scalar_parameter_value(mi, "Metallic", metal)
    lib.save_loaded_asset(mi)
    return mi


HELI = "/Game/Vehicles/Heli"
mats = {"MI_BTR_Paint": tint("MI_BTR_Paint", DEST, (0.09, 0.1, 0.065), 0.7, 0.25),
        "MI_Heli_Paint": tint("MI_Heli_Paint", HELI, (0.055, 0.06, 0.058), 0.55, 0.3),
        "MI_Heli_Interior": tint("MI_Heli_Interior", HELI, (0.075, 0.082, 0.068), 0.85, 0.0)}
paint = mats["MI_BTR_Paint"]
OWN = set(mats)
for n in ("MI_Veh_Rubber", "MI_Veh_Trim", "MI_Veh_GlassOpaque"):
    mats[n] = load(f"{MAT}/{n}")
    if not mats[n]:
        unreal.log_warning(f"[BL_Vehicle] falta {MAT}/{n}")

for name, DEST in (("SM_BTR_Hull", DEST), ("SM_BTR_Turret", DEST), ("SM_BTR_Gun", DEST), ("SM_Heli_Body", HELI), ("SM_Heli_Rotor", HELI), ("SM_Heli_TailRotor", HELI)):
    task = unreal.AssetImportTask()
    task.filename = os.path.join(SRC, name + ".fbx")
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
                new.append(unreal.StaticMaterial(material_interface=mats.get(slot) or paint, material_slot_name=slot,
                                                 uv_channel_data=sm.get_editor_property("uv_channel_data")))
            a.set_editor_property("static_materials", new)
            ns = a.get_editor_property("nanite_settings")
            ns.set_editor_property("enabled", True)
            a.set_editor_property("nanite_settings", ns)
            lib.save_loaded_asset(a)
            unreal.log(f"[BL_Vehicle] {a.get_name()} slots={[str(m.get_editor_property('material_slot_name')) for m in new]}")
        elif isinstance(a, unreal.MaterialInterface) and a.get_path_name().startswith(DEST) and a.get_name() not in OWN:
            lib.delete_loaded_asset(a)
unreal.log("[BL_Vehicle] OK")
