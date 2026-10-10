"""Assets de la misión 3 "Ría": mallas de gen_m03.py, uniforme de Corvane y láser del tirador. Idempotente.

  ArtSource/Blender/export/Gear/SM_Corvane_*.fbx -> /Game/Characters/Enemy/Gear
  ArtSource/Blender/export/OldTown/SM_*.fbx      -> /Game/Environment/OldTown
  MI_Corvane_Body / MI_Corvane_Sleeves (negro y gris oscuro sobre el Mannequin) y MI_Laser_Red -> /Game/Characters/Enemy/Materials

Uso: UnrealEditor-Cmd.exe Blackline.uproject -run=pythonscript -script="<ruta>/Tools/UnrealPython/setup_m03_assets.py"
"""
import glob
import os
import unreal

PROJECT = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())
EXPORT = os.path.join(PROJECT, "ArtSource", "Blender", "export")
ENV = "/Game/Environment/Materials"
TOWN = "/Game/Environment/OldTown"
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
    """Igual que el miliciano (setup_ai_assets.py): tinte de la pintura del Mannequin."""
    mi = new_mi(name, ENEMY, load(parent))
    mel.set_material_instance_vector_parameter_value(mi, "Paint Tint", unreal.LinearColor(*color, 1))
    mel.set_material_instance_vector_parameter_value(mi, "LogoTint", unreal.LinearColor(0, 0, 0, 1))
    mel.set_material_instance_scalar_parameter_value(mi, "EmissivePower", 0.0)
    mel.set_material_instance_scalar_parameter_value(mi, "MetalPaintRoughness", rough)
    mel.set_material_instance_scalar_parameter_value(mi, "MetalPaintMetallic", 0.0)
    lib.save_loaded_asset(mi)


# Corvane: negro mate sin insignias; el láser del tirador, rojo emisivo
uniform("MI_Corvane_Body", "/Game/Characters/Mannequins/Materials/Manny/MI_Manny_01_New", (0.012, 0.012, 0.014), 0.8)
uniform("MI_Corvane_Sleeves", "/Game/Characters/Mannequins/Materials/Manny/MI_Manny_02_New", (0.035, 0.036, 0.04), 0.8)
laser = new_mi("MI_Laser_Red", ENEMY, load(f"{ENV}/M_Prop_Emissive"))
mel.set_material_instance_vector_parameter_value(laser, "Color", unreal.LinearColor(0.2, 0.0, 0.0, 1))
mel.set_material_instance_vector_parameter_value(laser, "Emissive", unreal.LinearColor(60.0, 0.6, 0.3, 1))
lib.save_loaded_asset(laser)

mats = {
    "MI_Corvane_Gear": tint("MI_Corvane_Gear", ENEMY, (0.02, 0.02, 0.022), 0.7, 0.05),
    "MI_Old_DoorPaint": tint("MI_Old_DoorPaint", TOWN, (0.05, 0.11, 0.09), 0.72, 0.0),
    "MI_Old_RoofTile": tint("MI_Old_RoofTile", TOWN, (0.25, 0.09, 0.05), 0.75, 0.0),
    "MI_Old_Bronze": tint("MI_Old_Bronze", TOWN, (0.22, 0.15, 0.07), 0.4, 0.9),
    "MI_Boat_HullRed": tint("MI_Boat_HullRed", TOWN, (0.22, 0.035, 0.025), 0.6, 0.0),
    "MI_Boat_HullWhite": tint("MI_Boat_HullWhite", TOWN, (0.42, 0.42, 0.4), 0.6, 0.0),
    "MI_Boat_Net": tint("MI_Boat_Net", TOWN, (0.05, 0.1, 0.08), 0.95, 0.0),
    "MI_Old_CrateBlue": tint("MI_Old_CrateBlue", TOWN, (0.03, 0.09, 0.22), 0.55, 0.0),
    "MI_Ship_Grey": tint("MI_Ship_Grey", TOWN, (0.12, 0.13, 0.14), 0.55, 0.3),
    "MI_Ship_Deck": tint("MI_Ship_Deck", TOWN, (0.08, 0.07, 0.06), 0.8, 0.1),
}
OWN = set(mats)
# Reutilizados: piedra = hormigón triplanar con textura (mejor que un color plano), madera, metal, vidrio, goma, equipo
aliases = {"MI_Old_Stone": f"{ENV}/MI_Env_ConcreteWall", "MI_Env_WoodPlanks": f"{ENV}/MI_Env_WoodPlanks",
           "MI_Env_MetalPlate": f"{ENV}/MI_Env_MetalPlate", "MI_Env_RustyMetal": f"{ENV}/MI_Env_RustyMetal",
           "MI_Veh_GlassOpaque": f"{ENV}/MI_Veh_GlassOpaque", "MI_Veh_Rubber": f"{ENV}/MI_Veh_Rubber",
           "MI_Obj_Plastic": f"{ENV}/MI_Obj_Plastic", "MI_Gear_Webbing": f"{ENEMY}/MI_Gear_Webbing"}
for slot, path in aliases.items():
    mats[slot] = load(path)
    if not mats[slot]:
        unreal.log_warning(f"[BL_M03Assets] falta {path}")

jobs = [(f, "/Game/Characters/Enemy/Gear", False) for f in sorted(glob.glob(os.path.join(EXPORT, "Gear", "SM_Corvane_*.fbx")))]
jobs += [(f, TOWN, True) for f in sorted(glob.glob(os.path.join(EXPORT, "OldTown", "SM_*.fbx")))]
count = 0
for fbx, dest, nanite in jobs:
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
                    unreal.log_warning(f"[BL_M03Assets] {a.get_name()}: ranura sin material {slot}")
                new.append(unreal.StaticMaterial(material_interface=mats.get(slot), material_slot_name=slot,
                                                 uv_channel_data=sm.get_editor_property("uv_channel_data")))
            a.set_editor_property("static_materials", new)
            ns = a.get_editor_property("nanite_settings")
            ns.set_editor_property("enabled", nanite)       # el equipo va en personajes animados: sin Nanite
            a.set_editor_property("nanite_settings", ns)
            lib.save_loaded_asset(a)
            count += 1
        elif isinstance(a, unreal.MaterialInterface) and a.get_name() not in OWN and a.get_path_name().startswith(dest):
            lib.delete_loaded_asset(a)
unreal.log(f"[BL_M03Assets] Mallas importadas: {count}")
unreal.log("[BL_M03Assets] OK")
