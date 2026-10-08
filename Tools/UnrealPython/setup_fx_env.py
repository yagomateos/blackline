"""Materiales de efectos ambientales (Bloque 8). Idempotente.

  - M_FX_Flame: llama aditiva sin iluminación para ABLSmokeEmitter (mismos datos por instancia que M_FX_Dust:
    0 intensidad, 1 fotograma del atlas 2x2, 2-4 color). Forma = alfa del atlas de humo, más caliente abajo.

Uso: UnrealEditor-Cmd.exe Blackline.uproject -run=pythonscript -script="<ruta>/Tools/UnrealPython/setup_fx_env.py"
"""
import unreal

tools = unreal.AssetToolsHelpers.get_asset_tools()
lib = unreal.EditorAssetLibrary
mel = unreal.MaterialEditingLibrary
FOLDER = "/Game/FX/Materials"


def expr(m, cls, x, y, **props):
    e = mel.create_material_expression(m, cls, x, y)
    for k, v in props.items():
        e.set_editor_property(k, v)
    return e


def custom(m, x, y, code, inputs, out):
    c = expr(m, unreal.MaterialExpressionCustom, x, y, code=code, output_type=out)
    ins = []
    for n in inputs:
        ci = unreal.CustomInput()
        ci.set_editor_property("input_name", n)
        ins.append(ci)
    c.set_editor_property("inputs", ins)
    return c


def flame():
    if lib.does_asset_exist(f"{FOLDER}/M_FX_Flame"):
        lib.delete_asset(f"{FOLDER}/M_FX_Flame")   # se regenera siempre (sin referencias en assets: la carga ABLSmokeEmitter)
    m = tools.create_asset("M_FX_Flame", FOLDER, unreal.Material, unreal.MaterialFactoryNew())
    m.set_editor_property("blend_mode", unreal.BlendMode.BLEND_ADDITIVE)
    m.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
    m.set_editor_property("two_sided", True)
    m.set_editor_property("used_with_instanced_static_meshes", True)
    uv = expr(m, unreal.MaterialExpressionTextureCoordinate, -900, 0)
    frame = expr(m, unreal.MaterialExpressionPerInstanceCustomData, -900, 120, data_index=1)
    auv = custom(m, -650, 0, "float2 o = float2(fmod(F, 2.0), floor(F / 2.0)) * 0.5; return UV * 0.5 + o;", ["UV", "F"],
                 unreal.CustomMaterialOutputType.CMOT_FLOAT2)
    mel.connect_material_expressions(uv, "", auv, "UV")
    mel.connect_material_expressions(frame, "", auv, "F")
    ts = expr(m, unreal.MaterialExpressionTextureSample, -400, 0, texture=unreal.load_asset("/Game/FX/Textures/T_FX_SmokeAtlas"))
    mel.connect_material_expressions(auv, "", ts, "UVs")
    col = custom(m, -400, 300, "return float3(R, G, B);", ["R", "G", "B"], unreal.CustomMaterialOutputType.CMOT_FLOAT3)
    for i, n in enumerate(("R", "G", "B")):
        mel.connect_material_expressions(expr(m, unreal.MaterialExpressionPerInstanceCustomData, -650, 300 + i * 80, data_index=2 + i), "", col, n)
    inten = expr(m, unreal.MaterialExpressionPerInstanceCustomData, -650, 560, data_index=0)
    fade = expr(m, unreal.MaterialExpressionDepthFade, -400, 650, fade_distance_default=30.0)
    em = custom(m, -150, 100,
                "float a = pow(saturate(A), 1.6);"
                "float hot = saturate(1.4 - UV.y * 1.2);"                       # el núcleo (abajo) es más blanco
                "float3 c = lerp(C, float3(1.0, 0.7, 0.35), hot * 0.35);"
                "float edge = saturate(min(UV.x, 1.0 - UV.x) * 6.0);"
                "return c * a * I * D * edge * (0.9 + 1.6 * hot);",
                ["A", "UV", "C", "I", "D"], unreal.CustomMaterialOutputType.CMOT_FLOAT3)
    mel.connect_material_expressions(ts, "A", em, "A")
    mel.connect_material_expressions(uv, "", em, "UV")
    mel.connect_material_expressions(col, "", em, "C")
    mel.connect_material_expressions(inten, "", em, "I")
    mel.connect_material_expressions(fade, "", em, "D")
    mel.connect_material_property(em, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    mel.recompile_material(m)
    lib.save_loaded_asset(m)


flame()
unreal.log("[BL_FXEnv] OK")
