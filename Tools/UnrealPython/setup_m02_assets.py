"""Assets de la misión 2 "Manifiesto": importa las mallas de gen_refinery.py y gen_m02_vehicles.py y crea sus
materiales (instancias de M_Prop_Simple; el metal y la madera reutilizan los MI_Env_* triplanares). Idempotente.

  ArtSource/Blender/export/Refinery/SM_Ref_*.fbx -> /Game/Environment/Refinery
  ArtSource/Blender/export/Vehicles/SM_Drone_*    -> /Game/Vehicles/Drone
  ArtSource/Blender/export/Vehicles/SM_Boat_RHIB  -> /Game/Vehicles/Boat

Uso: UnrealEditor-Cmd.exe Blackline.uproject -run=pythonscript -script="<ruta>/Tools/UnrealPython/setup_m02_assets.py"
"""
import glob
import os
import unreal

PROJECT = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())
EXPORT = os.path.join(PROJECT, "ArtSource", "Blender", "export")
MAT = "/Game/Environment/Materials"
REF = "/Game/Environment/Refinery"
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


# Materiales propios (pintura de depósitos gastada, tuberías por código de color, dron y lancha)
mats = {
    "MI_Ref_TankPaint": tint("MI_Ref_TankPaint", REF, (0.36, 0.36, 0.34), 0.62, 0.15),
    "MI_Ref_PipeYellow": tint("MI_Ref_PipeYellow", REF, (0.42, 0.29, 0.03), 0.55, 0.2),
    "MI_Ref_PipeGrey": tint("MI_Ref_PipeGrey", REF, (0.16, 0.17, 0.18), 0.5, 0.45),
    "MI_Ref_MunitionOlive": tint("MI_Ref_MunitionOlive", REF, (0.09, 0.1, 0.065), 0.5, 0.2),
    "MI_Ref_Label": tint("MI_Ref_Label", REF, (0.62, 0.6, 0.5), 0.7, 0.0),
    "MI_Drone_Body": tint("MI_Drone_Body", "/Game/Vehicles/Drone", (0.035, 0.038, 0.042), 0.42, 0.1),
    "MI_Boat_Tube": tint("MI_Boat_Tube", "/Game/Vehicles/Boat", (0.05, 0.053, 0.056), 0.78, 0.0),
    "MI_Boat_Hull": tint("MI_Boat_Hull", "/Game/Vehicles/Boat", (0.12, 0.125, 0.12), 0.5, 0.1),
}
OWN = set(mats)
for n in ("MI_Env_RustyMetal", "MI_Env_MetalPlate", "MI_Env_WoodPlanks", "MI_Veh_Trim", "MI_Veh_GlassOpaque", "MI_Veh_Rubber"):
    mats[n] = load(f"{MAT}/{n}")
    if not mats[n]:
        unreal.log_warning(f"[BL_M02Assets] falta {MAT}/{n}")

jobs = [(f, REF) for f in sorted(glob.glob(os.path.join(EXPORT, "Refinery", "SM_Ref_*.fbx")))]
jobs += [(os.path.join(EXPORT, "Vehicles", f"{n}.fbx"), "/Game/Vehicles/Drone") for n in ("SM_Drone_Body", "SM_Drone_Prop")]
jobs += [(os.path.join(EXPORT, "Vehicles", "SM_Boat_RHIB.fbx"), "/Game/Vehicles/Boat")]
count = 0
for fbx, dest in jobs:
    task = unreal.AssetImportTask()
    task.filename = fbx
    task.destination_path = dest
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
                m = mats.get(slot)
                if not m:
                    unreal.log_warning(f"[BL_M02Assets] {a.get_name()}: ranura sin material {slot}")
                new.append(unreal.StaticMaterial(material_interface=m, material_slot_name=slot,
                                                 uv_channel_data=sm.get_editor_property("uv_channel_data")))
            a.set_editor_property("static_materials", new)
            ns = a.get_editor_property("nanite_settings")
            ns.set_editor_property("enabled", True)
            a.set_editor_property("nanite_settings", ns)
            lib.save_loaded_asset(a)
            count += 1
        elif isinstance(a, unreal.MaterialInterface) and a.get_name() not in OWN and not a.get_path_name().startswith(MAT):
            lib.delete_loaded_asset(a)   # materiales que crea el importador a partir del FBX (se usan los nuestros)
unreal.log(f"[BL_M02Assets] Mallas importadas: {count}")
unreal.log("[BL_M02Assets] OK")
