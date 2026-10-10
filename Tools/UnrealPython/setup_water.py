"""Agua del muelle (Bloque 11, fase 9): importa T_Water_N (gen_water_texture.py) y crea M_Env_Water. Idempotente.

M_Env_Water: opaco, oscuro y casi espejo (refleja el cielo del amanecer con Lumen); dos capas de la normal
desplazándose en direcciones distintas y a distinta escala para que el oleaje no se repita.

Uso: UnrealEditor-Cmd.exe Blackline.uproject -run=pythonscript -script="<ruta>/Tools/UnrealPython/setup_water.py"
Generar antes: python -I Tools/textures/gen_water_texture.py ArtSource/Textures/Water
"""
import os
import unreal

PROJECT = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())
PNG = os.path.join(PROJECT, "ArtSource", "Textures", "Water", "T_Water_N.png")
TEX = "/Game/Environment/Textures"
MAT = "/Game/Environment/Materials"
tools = unreal.AssetToolsHelpers.get_asset_tools()
lib = unreal.EditorAssetLibrary
mel = unreal.MaterialEditingLibrary


def expr(m, cls, x, y, **props):
    e = mel.create_material_expression(m, cls, x, y)
    for k, v in props.items():
        e.set_editor_property(k, v)
    return e


task = unreal.AssetImportTask()
task.filename = PNG
task.destination_path = TEX
task.automated = True
task.replace_existing = True
task.save = False
tools.import_asset_tasks([task])
tex = unreal.load_asset(f"{TEX}/T_Water_N")
tex.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_NORMALMAP)
tex.set_editor_property("srgb", False)
tex.set_editor_property("lod_group", unreal.TextureGroup.TEXTUREGROUP_WORLD_NORMAL_MAP)
lib.save_loaded_asset(tex)

if lib.does_asset_exist(f"{MAT}/M_Env_Water"):
    lib.delete_asset(f"{MAT}/M_Env_Water")
m = tools.create_asset("M_Env_Water", MAT, unreal.Material, unreal.MaterialFactoryNew())
wp = expr(m, unreal.MaterialExpressionWorldPosition, -1400, 0)
xy = expr(m, unreal.MaterialExpressionComponentMask, -1200, 0, r=True, g=True, b=False, a=False)
mel.connect_material_expressions(wp, "", xy, "")
samples = []
for i, (scale, sx, sy) in enumerate(((1.0 / 900.0, 0.012, 0.005), (1.0 / 2300.0, -0.004, 0.009))):
    mul = expr(m, unreal.MaterialExpressionMultiply, -1000, i * 300, const_b=scale)
    mel.connect_material_expressions(xy, "", mul, "A")
    pan = expr(m, unreal.MaterialExpressionPanner, -800, i * 300, speed_x=sx, speed_y=sy)
    mel.connect_material_expressions(mul, "", pan, "Coordinate")
    ts = expr(m, unreal.MaterialExpressionTextureSample, -550, i * 300, texture=tex,
              sampler_type=unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL)
    mel.connect_material_expressions(pan, "", ts, "UVs")
    samples.append(ts)
blend = expr(m, unreal.MaterialExpressionCustom, -250, 100,
             code="float3 n = float3(A.xy + B.xy * 0.8, A.z * B.z); n.xy *= 0.7; return normalize(n);",
             output_type=unreal.CustomMaterialOutputType.CMOT_FLOAT3)
ins = []
for n in ("A", "B"):
    ci = unreal.CustomInput()
    ci.set_editor_property("input_name", n)
    ins.append(ci)
blend.set_editor_property("inputs", ins)
mel.connect_material_expressions(samples[0], "RGB", blend, "A")
mel.connect_material_expressions(samples[1], "RGB", blend, "B")
mel.connect_material_property(blend, "", unreal.MaterialProperty.MP_NORMAL)
base = expr(m, unreal.MaterialExpressionConstant3Vector, -250, -200, constant=unreal.LinearColor(0.006, 0.011, 0.012, 1))
mel.connect_material_property(base, "", unreal.MaterialProperty.MP_BASE_COLOR)
mel.connect_material_property(expr(m, unreal.MaterialExpressionConstant, -250, 300, r=0.045), "", unreal.MaterialProperty.MP_ROUGHNESS)
mel.connect_material_property(expr(m, unreal.MaterialExpressionConstant, -250, 380, r=0.6), "", unreal.MaterialProperty.MP_SPECULAR)
mel.recompile_material(m)
lib.save_loaded_asset(m)
unreal.log("[BL_Water] OK")
