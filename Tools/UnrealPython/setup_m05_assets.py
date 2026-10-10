"""Assets de la misión 5 "Línea negra": mallas de gen_m05.py y la pintura negra del helicóptero de Corvane. Idempotente.

  ArtSource/Blender/export/Terminal/SM_*.fbx -> /Game/Environment/Terminal
  MI_Heli_Corvane (negro mate)               -> /Game/Vehicles/Heli

Uso: UnrealEditor-Cmd.exe Blackline.uproject -run=pythonscript -script="<ruta>/Tools/UnrealPython/setup_m05_assets.py"
Antes: setup_m03_assets.py (reutiliza MI_Boat_HullWhite).
"""
import glob
import os
import unreal

PROJECT = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())
EXPORT = os.path.join(PROJECT, "ArtSource", "Blender", "export", "Terminal")
ENV = "/Game/Environment/Materials"
TERM = "/Game/Environment/Terminal"
tools = unreal.AssetToolsHelpers.get_asset_tools()
lib = unreal.EditorAssetLibrary
mel = unreal.MaterialEditingLibrary


def load(p):
    return unreal.load_asset(p) if lib.does_asset_exist(p) else None


def new_mi(name, dest, parent):
    mi = load(f"{dest}/{name}") or tools.create_asset(name, dest, unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
    mi.set_editor_property("parent", parent)
    return mi


def tint(name, dest, color, rough, metal):
    mi = new_mi(name, dest, load(f"{ENV}/M_Prop_Simple"))
    mel.set_material_instance_vector_parameter_value(mi, "Color", unreal.LinearColor(*color, 1))
    mel.set_material_instance_scalar_parameter_value(mi, "Roughness", rough)
    mel.set_material_instance_scalar_parameter_value(mi, "Metallic", metal)
    lib.save_loaded_asset(mi)
    return mi


def glow(name, dest, color, emissive):
    mi = new_mi(name, dest, load(f"{ENV}/M_Prop_Emissive"))
    mel.set_material_instance_vector_parameter_value(mi, "Color", unreal.LinearColor(*color, 1))
    mel.set_material_instance_vector_parameter_value(mi, "Emissive", unreal.LinearColor(*emissive, 1))
    lib.save_loaded_asset(mi)
    return mi


tint("MI_Heli_Corvane", "/Game/Vehicles/Heli", (0.012, 0.012, 0.013), 0.5, 0.3)
mats = {
    "MI_Term_RackBlack": tint("MI_Term_RackBlack", TERM, (0.012, 0.013, 0.015), 0.5, 0.6),
    "MI_Term_LED": glow("MI_Term_LED", TERM, (0.02, 0.1, 0.03), (0.4, 6.0, 1.2)),
    "MI_Term_Thermite": tint("MI_Term_Thermite", TERM, (0.22, 0.2, 0.14), 0.6, 0.3),
    "MI_Term_Tape": tint("MI_Term_Tape", TERM, (0.4, 0.28, 0.02), 0.7, 0.0),
    "MI_Term_CraneRed": tint("MI_Term_CraneRed", TERM, (0.25, 0.04, 0.02), 0.65, 0.3),
    "MI_Term_PadGrey": tint("MI_Term_PadGrey", TERM, (0.09, 0.09, 0.09), 0.8, 0.0),
    "MI_Term_PaintYellow": tint("MI_Term_PaintYellow", TERM, (0.42, 0.32, 0.02), 0.7, 0.0),
    "MI_Term_Screen": glow("MI_Term_Screen", TERM, (0.01, 0.02, 0.03), (0.3, 0.8, 1.4)),
}
OWN = set(mats)
aliases = {"MI_Env_MetalPlate": f"{ENV}/MI_Env_MetalPlate", "MI_Env_WoodPlanks": f"{ENV}/MI_Env_WoodPlanks",
           "MI_Veh_GlassOpaque": f"{ENV}/MI_Veh_GlassOpaque", "MI_Obj_Plastic": f"{ENV}/MI_Obj_Plastic",
           "MI_Boat_HullWhite": "/Game/Environment/OldTown/MI_Boat_HullWhite"}
for slot, path in aliases.items():
    mats[slot] = load(path)
    if not mats[slot]:
        unreal.log_warning(f"[BL_M05Assets] falta {path}")

count = 0
for fbx in sorted(glob.glob(os.path.join(EXPORT, "SM_*.fbx"))):
    task = unreal.AssetImportTask()
    task.filename = fbx
    task.destination_path = TERM
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
                if not mats.get(slot):
                    unreal.log_warning(f"[BL_M05Assets] {a.get_name()}: ranura sin material {slot}")
                new.append(unreal.StaticMaterial(material_interface=mats.get(slot), material_slot_name=slot,
                                                 uv_channel_data=sm.get_editor_property("uv_channel_data")))
            a.set_editor_property("static_materials", new)
            ns = a.get_editor_property("nanite_settings")
            ns.set_editor_property("enabled", True)
            a.set_editor_property("nanite_settings", ns)
            lib.save_loaded_asset(a)
            count += 1
        elif isinstance(a, unreal.MaterialInterface) and a.get_name() not in OWN and a.get_path_name().startswith(TERM):
            lib.delete_loaded_asset(a)
unreal.log(f"[BL_M05Assets] Mallas importadas: {count}")
unreal.log("[BL_M05Assets] OK")
