"""Assets de la misión 4 "Fuego cruzado": mallas de gen_m04.py y uniforme del ejército de Varania. Idempotente.

  ArtSource/Blender/export/Bridge/SM_*.fbx     -> /Game/Environment/Bridge
  ArtSource/Blender/export/MountedGun/SM_*.fbx -> /Game/Weapons/MountedGun
  MI_Army_Body / MI_Army_Sleeves (gris verdoso sobre el Mannequin) -> /Game/Characters/Enemy/Materials

Uso: UnrealEditor-Cmd.exe Blackline.uproject -run=pythonscript -script="<ruta>/Tools/UnrealPython/setup_m04_assets.py"
Antes: setup_m03_assets.py (reutiliza MI_Boat_HullWhite).
"""
import glob
import os
import unreal

PROJECT = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())
EXPORT = os.path.join(PROJECT, "ArtSource", "Blender", "export")
ENV = "/Game/Environment/Materials"
BRIDGE = "/Game/Environment/Bridge"
MG = "/Game/Weapons/MountedGun"
ENEMY = "/Game/Characters/Enemy/Materials"
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


def uniform(name, parent, color, rough):
    mi = new_mi(name, ENEMY, load(parent))
    mel.set_material_instance_vector_parameter_value(mi, "Paint Tint", unreal.LinearColor(*color, 1))
    mel.set_material_instance_vector_parameter_value(mi, "LogoTint", unreal.LinearColor(0, 0, 0, 1))
    mel.set_material_instance_scalar_parameter_value(mi, "EmissivePower", 0.0)
    mel.set_material_instance_scalar_parameter_value(mi, "MetalPaintRoughness", rough)
    mel.set_material_instance_scalar_parameter_value(mi, "MetalPaintMetallic", 0.0)
    lib.save_loaded_asset(mi)


# Ejército de Varania: gris verdoso (distinto del caqui de la Columna y del oliva del jugador)
uniform("MI_Army_Body", "/Game/Characters/Mannequins/Materials/Manny/MI_Manny_01_New", (0.07, 0.085, 0.075), 0.85)
uniform("MI_Army_Sleeves", "/Game/Characters/Mannequins/Materials/Manny/MI_Manny_02_New", (0.05, 0.06, 0.05), 0.85)

mats = {
    "MI_Bridge_Steel": tint("MI_Bridge_Steel", BRIDGE, (0.07, 0.085, 0.08), 0.6, 0.55),
    "MI_MG_Gunmetal": tint("MI_MG_Gunmetal", MG, (0.025, 0.025, 0.028), 0.42, 0.8),
    "MI_Army_Olive": tint("MI_Army_Olive", BRIDGE, (0.07, 0.085, 0.05), 0.7, 0.15),
    "MI_Army_Canvas": tint("MI_Army_Canvas", BRIDGE, (0.11, 0.11, 0.075), 0.95, 0.0),
    "MI_Jet_Grey": tint("MI_Jet_Grey", BRIDGE, (0.17, 0.18, 0.19), 0.45, 0.4),
}
OWN = set(mats)
aliases = {"MI_Old_Stone": f"{ENV}/MI_Env_ConcreteWall", "MI_Env_WoodPlanks": f"{ENV}/MI_Env_WoodPlanks",
           "MI_Env_RustyMetal": f"{ENV}/MI_Env_RustyMetal", "MI_Veh_GlassOpaque": f"{ENV}/MI_Veh_GlassOpaque",
           "MI_Veh_Rubber": f"{ENV}/MI_Veh_Rubber", "MI_Boat_HullWhite": "/Game/Environment/OldTown/MI_Boat_HullWhite"}
for slot, path in aliases.items():
    mats[slot] = load(path)
    if not mats[slot]:
        unreal.log_warning(f"[BL_M04Assets] falta {path}")

jobs = [(f, BRIDGE) for f in sorted(glob.glob(os.path.join(EXPORT, "Bridge", "SM_*.fbx")))]
jobs += [(f, MG) for f in sorted(glob.glob(os.path.join(EXPORT, "MountedGun", "SM_*.fbx")))]
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
                if not mats.get(slot):
                    unreal.log_warning(f"[BL_M04Assets] {a.get_name()}: ranura sin material {slot}")
                new.append(unreal.StaticMaterial(material_interface=mats.get(slot), material_slot_name=slot,
                                                 uv_channel_data=sm.get_editor_property("uv_channel_data")))
            a.set_editor_property("static_materials", new)
            ns = a.get_editor_property("nanite_settings")
            ns.set_editor_property("enabled", True)
            a.set_editor_property("nanite_settings", ns)
            lib.save_loaded_asset(a)
            count += 1
        elif isinstance(a, unreal.MaterialInterface) and a.get_name() not in OWN and a.get_path_name().startswith(dest):
            lib.delete_loaded_asset(a)
unreal.log(f"[BL_M04Assets] Mallas importadas: {count}")
unreal.log("[BL_M04Assets] OK")
