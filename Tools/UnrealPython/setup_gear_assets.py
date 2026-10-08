"""Equipo del miliciano (Bloque 8): importa ArtSource/Blender/export/Gear y crea sus materiales. Idempotente.

M_Gear_Triplanar: como M_Env_Triplanar pero en espacio LOCAL de la malla (las piezas se mueven con el personaje;
en espacio de mundo la textura "nadaría"). Solo color + rugosidad (a la distancia del combate no hace falta normal).
Instancias: MI_Gear_Fabric (tela caqui verdosa), MI_Gear_Webbing (cinchas), MI_Gear_Helmet (acero pintado), MI_Gear_Black.

Uso: UnrealEditor-Cmd.exe Blackline.uproject -run=pythonscript -script="<ruta>/Tools/UnrealPython/setup_gear_assets.py"
"""
import glob
import os

import unreal

PROJECT = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())
FBX_DIR = os.path.join(PROJECT, "ArtSource", "Blender", "export", "Gear")
DEST = "/Game/Characters/Enemy/Gear"
MAT_DIR = "/Game/Characters/Enemy/Materials"
TEX = "/Game/Environment/Textures/"

tools = unreal.AssetToolsHelpers.get_asset_tools()
lib = unreal.EditorAssetLibrary
mel = unreal.MaterialEditingLibrary

SAMPLE = ("float3 n = pow(abs(N), 4.0); n /= (n.x + n.y + n.z);"
          "float3 p = LP / Tile;"
          "float4 a = Tex.Sample(TexSampler, p.yz); float4 b = Tex.Sample(TexSampler, p.xz); float4 c = Tex.Sample(TexSampler, p.xy);"
          "return (a * n.x + b * n.y + c * n.z).rgb;")


def load(path):
    return unreal.load_asset(path) if lib.does_asset_exist(path) else None


def expr(m, cls, x, y, **props):
    e = mel.create_material_expression(m, cls, x, y)
    for k, v in props.items():
        e.set_editor_property(k, v)
    return e


def custom(m, x, y, code, inputs, out=unreal.CustomMaterialOutputType.CMOT_FLOAT3):
    c = expr(m, unreal.MaterialExpressionCustom, x, y, code=code, output_type=out)
    ins = []
    for n in inputs:
        ci = unreal.CustomInput()
        ci.set_editor_property("input_name", n)
        ins.append(ci)
    c.set_editor_property("inputs", ins)
    return c


def master():
    path = f"{MAT_DIR}/M_Gear_Triplanar"
    if lib.does_asset_exist(path):
        return load(path)
    m = tools.create_asset("M_Gear_Triplanar", MAT_DIR, unreal.Material, unreal.MaterialFactoryNew())
    lp = expr(m, unreal.MaterialExpressionLocalPosition, -1100, 0)
    wn = expr(m, unreal.MaterialExpressionVertexNormalWS, -1300, 150)
    ln = expr(m, unreal.MaterialExpressionTransform, -1100, 150,
              transform_source_type=unreal.MaterialVectorCoordTransformSource.TRANSFORMSOURCE_WORLD,
              transform_type=unreal.MaterialVectorCoordTransform.TRANSFORM_LOCAL)
    mel.connect_material_expressions(wn, "", ln, "")
    tile = expr(m, unreal.MaterialExpressionScalarParameter, -1100, 300, parameter_name="TileSize", default_value=40.0)
    tex = expr(m, unreal.MaterialExpressionTextureObjectParameter, -1100, 400, parameter_name="BaseColorMap", texture=load(TEX + "dirt_diff_2k"))
    s = custom(m, -700, 0, SAMPLE, ["Tex", "LP", "N", "Tile"])
    for e, pin in ((tex, "Tex"), (lp, "LP"), (ln, "N"), (tile, "Tile")):
        mel.connect_material_expressions(e, "", s, pin)
    tint = expr(m, unreal.MaterialExpressionVectorParameter, -700, 250, parameter_name="Tint", default_value=unreal.LinearColor(1, 1, 1, 1))
    contrast = expr(m, unreal.MaterialExpressionScalarParameter, -700, 400, parameter_name="Contrast", default_value=0.5)
    bc = custom(m, -350, 0, "float l = dot(T, float3(0.3, 0.59, 0.11)); return Tint.rgb * lerp(1.0, l * 2.2, C);", ["T", "Tint", "C"])
    mel.connect_material_expressions(s, "", bc, "T")
    mel.connect_material_expressions(tint, "", bc, "Tint")
    mel.connect_material_expressions(contrast, "", bc, "C")
    mel.connect_material_property(bc, "", unreal.MaterialProperty.MP_BASE_COLOR)
    rough = expr(m, unreal.MaterialExpressionScalarParameter, -350, 250, parameter_name="Roughness", default_value=0.9)
    metal = expr(m, unreal.MaterialExpressionScalarParameter, -350, 350, parameter_name="Metallic", default_value=0.0)
    mel.connect_material_property(rough, "", unreal.MaterialProperty.MP_ROUGHNESS)
    mel.connect_material_property(metal, "", unreal.MaterialProperty.MP_METALLIC)
    mel.recompile_material(m)
    lib.save_loaded_asset(m)
    return m


def instance(name, tex_id, tile, tint, rough, metal, contrast):
    mi = load(f"{MAT_DIR}/{name}") or tools.create_asset(name, MAT_DIR, unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
    mi.set_editor_property("parent", master())
    mel.set_material_instance_texture_parameter_value(mi, "BaseColorMap", load(TEX + f"{tex_id}_diff_2k"))
    mel.set_material_instance_scalar_parameter_value(mi, "TileSize", tile)
    mel.set_material_instance_vector_parameter_value(mi, "Tint", unreal.LinearColor(*tint, 1))
    mel.set_material_instance_scalar_parameter_value(mi, "Roughness", rough)
    mel.set_material_instance_scalar_parameter_value(mi, "Metallic", metal)
    mel.set_material_instance_scalar_parameter_value(mi, "Contrast", contrast)
    mi.set_editor_property("phys_material", load("/Game/Environment/PhysicalMaterials/PM_Flesh"))
    lib.save_loaded_asset(mi)
    return mi


def main():
    mats = {
        "MI_Gear_Fabric": instance("MI_Gear_Fabric", "dirt", 35, (0.2, 0.2, 0.13), 0.92, 0.0, 0.45),
        "MI_Gear_Webbing": instance("MI_Gear_Webbing", "dirt", 25, (0.09, 0.09, 0.07), 0.9, 0.0, 0.3),
        "MI_Gear_Helmet": instance("MI_Gear_Helmet", "rusty_metal_02", 30, (0.17, 0.19, 0.13), 0.6, 0.3, 0.35),
        "MI_Gear_Black": instance("MI_Gear_Black", "dirt", 20, (0.018, 0.018, 0.018), 0.88, 0.0, 0.2),
        "MI_Obj_Plastic": load("/Game/Environment/Materials/MI_Obj_Plastic"),
    }
    for fbx in sorted(glob.glob(os.path.join(FBX_DIR, "*.fbx"))):
        task = unreal.AssetImportTask()
        task.filename = fbx
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
                ns.set_editor_property("enabled", True)
                a.set_editor_property("nanite_settings", ns)
                lib.save_loaded_asset(a)
                unreal.log(f"[BL_Gear] {a.get_name()}")
            elif isinstance(a, unreal.MaterialInterface) and a.get_path_name().startswith(DEST):
                lib.delete_loaded_asset(a)
    unreal.log("[BL_Gear] OK")


main()
