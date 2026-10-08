"""Assets de la IA (Bloque 5): uniforme del miliciano (provisional sobre el Mannequin hasta el pase de arte). Idempotente.

Uso:
  UnrealEditor-Cmd.exe Blackline.uproject -run=pythonscript -script="<ruta>/Tools/UnrealPython/setup_ai_assets.py"
"""
import unreal

tools = unreal.AssetToolsHelpers.get_asset_tools()
lib = unreal.EditorAssetLibrary
mel = unreal.MaterialEditingLibrary
FOLDER = "/Game/Characters/Enemy/Materials"


def militia_material(name, parent, tint, rough):
    path = f"{FOLDER}/{name}"
    mi = unreal.load_asset(path) if lib.does_asset_exist(path) else tools.create_asset(
        name, FOLDER, unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
    mi.set_editor_property("parent", unreal.load_asset(parent))
    mel.set_material_instance_vector_parameter_value(mi, "Paint Tint", unreal.LinearColor(*tint, 1))
    mel.set_material_instance_vector_parameter_value(mi, "LogoTint", unreal.LinearColor(0, 0, 0, 1))
    mel.set_material_instance_scalar_parameter_value(mi, "EmissivePower", 0.0)
    mel.set_material_instance_scalar_parameter_value(mi, "MetalPaintRoughness", rough)
    mel.set_material_instance_scalar_parameter_value(mi, "MetalPaintMetallic", 0.0)
    lib.save_loaded_asset(mi)
    return mi


# Columna Vesk: ropa civil/militar mezclada en tonos caqui y pardos (se distingue del verde oliva del jugador)
militia_material("MI_Militia_Body", "/Game/Characters/Mannequins/Materials/Manny/MI_Manny_01_New", (0.16, 0.13, 0.085), 0.9)
militia_material("MI_Militia_Sleeves", "/Game/Characters/Mannequins/Materials/Manny/MI_Manny_02_New", (0.06, 0.05, 0.04), 0.85)
unreal.log("[BL_AI] OK")
