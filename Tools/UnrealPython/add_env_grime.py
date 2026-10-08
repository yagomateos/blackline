"""Bloque 8: añade suciedad y variación al maestro triplanar de entorno (M_Env_Triplanar). Idempotente.

  - Variación de tono a gran escala (ruido de 4-12 m): rompe la repetición de las texturas de 2-3 m.
  - Chorretones verticales en las paredes (agua de lluvia que baja de cornisas y alféizares).
  - Suciedad a ras de suelo en las paredes (salpicaduras, polvo de la calle): los primeros ~1,1 m.
  Parámetros: GrimeStrength (0 = desactivado; los MI pueden bajarlo), GroundDirtHeight.
Se inserta entre el color/rugosidad actuales y la salida del material (no rehace el resto del grafo).

Uso: UnrealEditor-Cmd.exe Blackline.uproject -run=pythonscript -script="<ruta>/Tools/UnrealPython/add_env_grime.py"
"""
import unreal

lib = unreal.EditorAssetLibrary
mel = unreal.MaterialEditingLibrary
PATH = "/Game/Environment/Materials/M_Env_Triplanar"

MACROS = (
    "#ifndef BL_GRIME\n#define BL_GRIME 1\n"
    "#define BLH(p) frac(sin(dot(p, float2(127.1, 311.7))) * 43758.5453)\n"
    "#define BLSF(p) (frac(p) * frac(p) * (3.0 - 2.0 * frac(p)))\n"
    "#define BLVN(p) lerp(lerp(BLH(floor(p)), BLH(floor(p) + float2(1, 0)), BLSF(p).x), "
    "lerp(BLH(floor(p) + float2(0, 1)), BLH(floor(p) + float2(1, 1)), BLSF(p).x), BLSF(p).y)\n"
    "#endif\n"
)
COMMON = (
    "float3 n = normalize(WN);"
    "float vert = saturate(1.0 - abs(n.z) * 1.4);"                       # 1 en paredes, 0 en suelos/techos
    "float2 t = normalize(float2(-n.y, n.x) + 1e-4);"
    "float u = dot(WP.xy, t);"
    "float2 pm = (vert > 0.5 ? float2(u, WP.z) : WP.xy) / 520.0;"
    "float macro = BLVN(pm) * 0.65 + BLVN(pm * 2.7 + 17.0) * 0.35;"
    "float s = BLVN(float2(u / 38.0, WP.z / 420.0));"
    "float streak = saturate((s - 0.5) * 2.6) * vert * (0.55 + 0.45 * BLVN(float2(u / 300.0, 3.0)));"
    "float g = saturate(1.0 - WP.z / GH); g = g * g * vert;"
)
BC_CODE = MACROS + COMMON + (
    "float3 c = BC * lerp(0.8, 1.12, macro * S + (1.0 - S) * 0.6);"
    "c = lerp(c, c * 0.58, streak * 0.4 * S);"
    "c = lerp(c, c * float3(0.62, 0.56, 0.5), g * 0.65 * S);"
    "return c;"
)
R_CODE = MACROS + COMMON + "return saturate(R + (g * 0.12 + streak * 0.08 - (macro - 0.5) * 0.06) * S);"


def custom(m, x, y, code, inputs, out):
    c = mel.create_material_expression(m, unreal.MaterialExpressionCustom, x, y)
    c.set_editor_property("code", code)
    c.set_editor_property("output_type", out)
    ins = []
    for n in inputs:
        ci = unreal.CustomInput()
        ci.set_editor_property("input_name", n)
        ins.append(ci)
    c.set_editor_property("inputs", ins)
    return c


def main():
    m = unreal.load_asset(PATH)
    if "GrimeStrength" in [str(n) for n in mel.get_scalar_parameter_names(m)]:
        unreal.log("[BL_Grime] Ya aplicado")
        return
    bc_in = mel.get_material_property_input_node(m, unreal.MaterialProperty.MP_BASE_COLOR)
    r_in = mel.get_material_property_input_node(m, unreal.MaterialProperty.MP_ROUGHNESS)
    wp = mel.create_material_expression(m, unreal.MaterialExpressionWorldPosition, -300, -400)
    wn = mel.create_material_expression(m, unreal.MaterialExpressionVertexNormalWS, -300, -300)
    st = mel.create_material_expression(m, unreal.MaterialExpressionScalarParameter, -300, -200)
    st.set_editor_property("parameter_name", "GrimeStrength")
    st.set_editor_property("default_value", 1.0)
    gh = mel.create_material_expression(m, unreal.MaterialExpressionScalarParameter, -300, -100)
    gh.set_editor_property("parameter_name", "GroundDirtHeight")
    gh.set_editor_property("default_value", 110.0)
    bc = custom(m, 100, -300, BC_CODE, ["BC", "WP", "WN", "S", "GH"], unreal.CustomMaterialOutputType.CMOT_FLOAT3)
    ro = custom(m, 100, 600, R_CODE, ["R", "WP", "WN", "S", "GH"], unreal.CustomMaterialOutputType.CMOT_FLOAT1)
    mel.connect_material_expressions(bc_in, "", bc, "BC")
    mel.connect_material_expressions(r_in, "", ro, "R")
    for node in (bc, ro):
        mel.connect_material_expressions(wp, "", node, "WP")
        mel.connect_material_expressions(wn, "", node, "WN")
        mel.connect_material_expressions(st, "", node, "S")
        mel.connect_material_expressions(gh, "", node, "GH")
    mel.connect_material_property(bc, "", unreal.MaterialProperty.MP_BASE_COLOR)
    mel.connect_material_property(ro, "", unreal.MaterialProperty.MP_ROUGHNESS)
    mel.recompile_material(m)
    lib.save_loaded_asset(m)
    unreal.log("[BL_Grime] OK")


main()
