"""Crea/actualiza los assets del sistema de armas (Bloque 2). Idempotente.

  - Mallas del arma (fogonazo, casquillo) + materiales; SK_AR7 y su cargador.
  - DA_AR7 (UBLWeaponData). Mientras no exista SK_AR7 usa el fusil de Epic.
  Sonidos, impactos por superficie y materiales de entorno: setup_audio_fx.py (ejecutar antes).

Uso:
  UnrealEditor-Cmd.exe Blackline.uproject -run=pythonscript -script="Tools/UnrealPython/setup_weapon_assets.py"
Requisitos: módulo C++ compilado; FBX generados con gen_weapon_fx.py; WAV con gen_placeholder_sounds.py.
"""
import glob
import os
import unreal

PROJECT = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())
FBX_DIR = os.path.join(PROJECT, "ArtSource", "Blender", "export")
WAV_DIR = os.path.join(PROJECT, "ArtSource", "Audio", "Placeholder")

AUDIO_DIR = "/Game/Audio/Placeholder"
FX_DIR = "/Game/FX"
FX_MESH_DIR = "/Game/FX/Meshes"
FX_MAT_DIR = "/Game/FX/Materials"
AR7_DIR = "/Game/Weapons/AR7"

tools = unreal.AssetToolsHelpers.get_asset_tools()
lib = unreal.EditorAssetLibrary
mel = unreal.MaterialEditingLibrary


def log(m):
    unreal.log(f"[BL_Weapons] {m}")


def load(path):
    return unreal.load_asset(path) if lib.does_asset_exist(path) else None


def import_file(filename, dest):
    task = unreal.AssetImportTask()
    task.filename = filename
    task.destination_path = dest
    task.automated = True
    task.replace_existing = True
    task.save = True
    tools.import_asset_tasks([task])
    return [unreal.load_asset(p) for p in task.imported_object_paths]


# ---------------------------------------------------------------------------
# Audio
# ---------------------------------------------------------------------------

def make_attenuation(name, inner, falloff):
    path = f"{AUDIO_DIR}/{name}"
    att = load(path) or tools.create_asset(name, AUDIO_DIR, unreal.SoundAttenuation, unreal.SoundAttenuationFactory())
    s = att.get_editor_property("attenuation")
    s.set_editor_property("attenuation_shape_extents", unreal.Vector(inner, 0, 0))
    s.set_editor_property("falloff_distance", falloff)
    s.set_editor_property("distance_algorithm", unreal.AttenuationDistanceModel.NATURAL_SOUND)
    att.set_editor_property("attenuation", s)
    lib.save_loaded_asset(att)
    return att


def import_sounds():
    sounds = {}
    for wav in sorted(glob.glob(os.path.join(WAV_DIR, "*.wav"))):
        for a in import_file(wav, AUDIO_DIR):
            if isinstance(a, unreal.SoundWave):
                sounds[a.get_name()] = a
    att_impact = make_attenuation("SA_Impact", 150, 2500)
    att_casing = make_attenuation("SA_Casing", 40, 700)
    make_attenuation("SA_WeaponFire", 600, 15000)  # para disparos de la IA (Bloque 5)
    for name, s in sounds.items():
        if name.startswith("SW_Impact"):
            s.set_editor_property("attenuation_settings", att_impact)
        elif name.startswith("SW_Casing"):
            s.set_editor_property("attenuation_settings", att_casing)
        lib.save_loaded_asset(s)
    log(f"Sonidos: {len(sounds)}")
    return sounds


# ---------------------------------------------------------------------------
# Materiales
# ---------------------------------------------------------------------------

def new_material(name, folder=FX_MAT_DIR):
    """Crea el material vacío. Si ya existe devuelve None y se reutiliza tal cual
    (DeleteAllMaterialExpressions provoca una aserción en UE 5.8; para regenerar, borrar el asset)."""
    path = f"{folder}/{name}"
    if lib.does_asset_exist(path):
        return None
    return tools.create_asset(name, folder, unreal.Material, unreal.MaterialFactoryNew())


def expr(mat, cls, x, y, **props):
    e = mel.create_material_expression(mat, cls, x, y)
    for k, v in props.items():
        e.set_editor_property(k, v)
    return e


def sphere_mask(mat, x, y, radius, hardness):
    """SphereMask sobre las UV, centrada en (0.5, 0.5)."""
    uv = expr(mat, unreal.MaterialExpressionTextureCoordinate, x - 400, y)
    center = expr(mat, unreal.MaterialExpressionConstant2Vector, x - 400, y + 120, r=0.5, g=0.5)
    m = expr(mat, unreal.MaterialExpressionSphereMask, x - 200, y, attenuation_radius=radius, hardness_percent=hardness)
    mel.connect_material_expressions(uv, "", m, "A")
    mel.connect_material_expressions(center, "", m, "B")
    return m


def finish(mat):
    mel.recompile_material(mat)
    lib.save_loaded_asset(mat)
    return mat


REBUILD_FLASH = True  # regenerar siempre el fogonazo (se ajusta a menudo)
FLAME_HLSL = (
    "float u = UV.x; float v = abs(UV.y - 0.5) * 2.0;"
    "float n = frac(sin(dot(floor(float2(u * 7.0, T * 60.0)), float2(12.9898, 78.233))) * 43758.5453);"
    "float w = pow(saturate(1.0 - u), 0.6) * (0.55 + 0.45 * n);"
    "float m = smoothstep(w, w * 0.1, v) * smoothstep(0.0, 0.05, u);"
    "float glow = pow(saturate(1.0 - u), 3.0) * pow(saturate(1.0 - v), 2.0) * 0.35;"
    "float3 c = lerp(float3(1.0, 0.42, 0.1), float3(1.0, 0.88, 0.65), pow(m, 3.0));"
    "return c * (m + glow) * I;")
STAR_HLSL = (
    "float2 p = UV * 2.0 - 1.0; float r = length(p); float a = atan2(p.y, p.x);"
    "float seed = floor(T * 60.0);"
    "float k = 4.0 + step(0.5, frac(sin(seed * 12.9898) * 43758.5453));"
    "float rays = pow(abs(cos(a * k * 0.5 + seed * 1.7)), 6.0);"
    "float len = 0.35 + 0.55 * frac(sin(floor(a * k / 6.2832 + seed) * 78.233) * 43758.5453);"
    "float petals = saturate(1.0 - r / (0.12 + len * rays)) * rays;"
    "float core = pow(saturate(1.0 - r * 2.8), 2.0);"
    "float3 c = float3(1.0, 0.93, 0.8) * core * 2.0 + float3(1.0, 0.58, 0.22) * petals;"
    "return c * I;")


def make_flash_materials():
    """Fogonazo: llama lateral que se estrecha y estrella frontal con rayos; forma distinta cada fotograma."""
    out = {}
    for name, code, inten in (("M_MuzzleFlame", FLAME_HLSL, 9.0), ("M_MuzzleStar", STAR_HLSL, 14.0)):
        if REBUILD_FLASH and lib.does_asset_exist(f"{FX_MAT_DIR}/{name}"):
            lib.delete_asset(f"{FX_MAT_DIR}/{name}")
        m = new_material(name)
        if m is None:
            out[name] = load(f"{FX_MAT_DIR}/{name}")
            continue
        m.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
        m.set_editor_property("blend_mode", unreal.BlendMode.BLEND_ADDITIVE)
        m.set_editor_property("two_sided", True)
        uv = expr(m, unreal.MaterialExpressionTextureCoordinate, -600, 0)
        t = expr(m, unreal.MaterialExpressionTime, -600, 100)
        i = expr(m, unreal.MaterialExpressionScalarParameter, -600, 200, parameter_name="Intensity", default_value=inten)
        c = expr(m, unreal.MaterialExpressionCustom, -300, 0, code=code, output_type=unreal.CustomMaterialOutputType.CMOT_FLOAT3)
        ins = []
        for n in ("UV", "T", "I"):
            ci = unreal.CustomInput()
            ci.set_editor_property("input_name", n)
            ins.append(ci)
        c.set_editor_property("inputs", ins)
        mel.connect_material_expressions(uv, "", c, "UV")
        mel.connect_material_expressions(t, "", c, "T")
        mel.connect_material_expressions(i, "", c, "I")
        mel.connect_material_property(c, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
        out[name] = finish(m)
    return out


def make_materials():
    """Latón de los casquillos (uso ISM) y fogonazo. El resto de efectos vive en setup_audio_fx.py."""
    mats = {}
    brass = load(f"{FX_MAT_DIR}/M_Brass")
    if not brass:
        brass = new_material("M_Brass")
        mel.connect_material_property(expr(brass, unreal.MaterialExpressionConstant3Vector, -300, 0, constant=unreal.LinearColor(0.80, 0.56, 0.22, 1.0)), "", unreal.MaterialProperty.MP_BASE_COLOR)
        mel.connect_material_property(expr(brass, unreal.MaterialExpressionConstant, -300, 120, r=0.3), "", unreal.MaterialProperty.MP_ROUGHNESS)
        mel.connect_material_property(expr(brass, unreal.MaterialExpressionConstant, -300, 200, r=1.0), "", unreal.MaterialProperty.MP_METALLIC)
        brass = finish(brass)
    mats["brass"] = brass
    mats.update(make_flash_materials())
    # Restos de versiones anteriores
    if lib.does_asset_exist(f"{FX_MAT_DIR}/M_MuzzleFlash"):
        lib.delete_asset(f"{FX_MAT_DIR}/M_MuzzleFlash")
    return mats


def import_mesh(name, dest, material):
    fbx = os.path.join(FBX_DIR, name + ".fbx")
    if not os.path.exists(fbx):
        raise RuntimeError(f"Falta {fbx}: ejecutar gen_weapon_fx.py")
    mesh = None
    for a in import_file(fbx, dest):
        if isinstance(a, unreal.StaticMesh):
            mesh = a
        elif isinstance(a, unreal.MaterialInterface) and a.get_path_name().startswith(dest):
            lib.delete_loaded_asset(a)  # material autogenerado por la importación: se usa el nuestro
    if mesh is None:
        raise RuntimeError(f"No se importó {name}")
    for i in range(mesh.get_num_sections(0)):
        if isinstance(material, (list, tuple)):
            mesh.set_material(i, material[min(i, len(material) - 1)])
        elif material:
            mesh.set_material(i, material)
    # Sin Nanite: materiales aditivos (fogonazo) y mallas diminutas instanciadas
    ns = mesh.get_editor_property('nanite_settings')
    ns.set_editor_property('enabled', False)
    mesh.set_editor_property('nanite_settings', ns)
    lib.save_loaded_asset(mesh)
    b = mesh.get_bounding_box()
    log(f"{name}: tamaño {b.max - b.min} cm")
    return mesh


TEX_WEAPON_DIR = os.path.join(PROJECT, "ArtSource", "Textures", "Weapons")


REBUILD_MASTER = True  # v2 (2026-10-08): texturas de detalle propias; regenerar mientras se ajusta
WEAPON_TEX_DIR = "/Game/Weapons/Shared/Textures"


def import_weapon_textures():
    """Máscaras horneadas del AR-7 y texturas de detalle compartidas (gen_ar7.py --bake, gen_weapon_detail.py)."""
    out = {}
    specs = (("T_AR7_Masks", f"{AR7_DIR}/Textures", unreal.TextureCompressionSettings.TC_MASKS, False),
             ("T_Weapon_Micro_N", WEAPON_TEX_DIR, unreal.TextureCompressionSettings.TC_NORMALMAP, False),
             ("T_Weapon_Smudge", WEAPON_TEX_DIR, unreal.TextureCompressionSettings.TC_MASKS, False))
    for name, dest, comp, srgb in specs:
        path = os.path.join(TEX_WEAPON_DIR, name + ".png")
        if not os.path.exists(path):
            raise RuntimeError(f"Falta {path}")
        for a in import_file(path, dest):
            if isinstance(a, unreal.Texture2D):
                a.set_editor_property("srgb", srgb)
                a.set_editor_property("compression_settings", comp)
                lib.save_loaded_asset(a)
                out[name] = a
    return out


def weapon_uv_scale():
    """cm de superficie por unidad de UV del AR-7 (lo escribe gen_ar7.py --bake)."""
    import json
    try:
        with open(os.path.join(TEX_WEAPON_DIR, "T_AR7_Masks.json")) as f:
            return float(json.load(f)["uv_scale_cm"])
    except (OSError, KeyError, ValueError):
        return 110.0


def make_weapon_master(tex):
    """Material maestro de arma: máscaras horneadas (R=AO, G=aristas, B=cavidades) + desgaste de bordes,
    suciedad en cavidades, manchas de rugosidad (grasa/huellas), moteado del polímero, arañazos finos
    y micro-normal de grano fino. Todas las texturas de detalle tienen tamaño real (cm) gracias a la escala de UV."""
    folder = f"{AR7_DIR}/Materials"
    path = f"{folder}/M_Weapon_Master"
    if lib.does_asset_exist(path):
        if not REBUILD_MASTER:
            return load(path)
        lib.delete_asset(path)
    m = tools.create_asset("M_Weapon_Master", folder, unreal.Material, unreal.MaterialFactoryNew())
    # Imprescindible: sin este uso, en el juego el arma (malla esquelética) se dibuja con el material por defecto
    m.set_editor_property("used_with_skeletal_mesh", True)
    uv = expr(m, unreal.MaterialExpressionTextureCoordinate, -1400, 0)

    def tiled(param, default, y):
        mul = expr(m, unreal.MaterialExpressionMultiply, -1200, y)
        mel.connect_material_expressions(uv, "", mul, "A")
        mel.connect_material_expressions(expr(m, unreal.MaterialExpressionScalarParameter, -1400, y + 60, parameter_name=param, default_value=default), "", mul, "B")
        return mul

    masks = expr(m, unreal.MaterialExpressionTextureSampleParameter2D, -1000, 0, parameter_name="Masks", texture=tex["T_AR7_Masks"],
                 sampler_type=unreal.MaterialSamplerType.SAMPLERTYPE_MASKS)
    mel.connect_material_expressions(uv, "", masks, "UVs")
    det = expr(m, unreal.MaterialExpressionTextureSampleParameter2D, -1000, 300, parameter_name="DetailNormal", texture=tex["T_Weapon_Micro_N"],
               sampler_type=unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL)
    mel.connect_material_expressions(tiled("DetailTiling", 36.0, 300), "", det, "UVs")
    sm = expr(m, unreal.MaterialExpressionTextureSampleParameter2D, -1000, 600, parameter_name="Smudge", texture=tex["T_Weapon_Smudge"],
              sampler_type=unreal.MaterialSamplerType.SAMPLERTYPE_MASKS)
    mel.connect_material_expressions(tiled("SmudgeTiling", 4.0, 600), "", sm, "UVs")

    def vparam(name, val, y):
        return expr(m, unreal.MaterialExpressionVectorParameter, -1000, y, parameter_name=name, default_value=unreal.LinearColor(*val, 1))

    def sparam(name, val, y):
        return expr(m, unreal.MaterialExpressionScalarParameter, -1000, y, parameter_name=name, default_value=val)

    def custom_node(code, inputs, out_type, y):
        c = expr(m, unreal.MaterialExpressionCustom, -500, y, code=code, output_type=out_type)
        ins = []
        for n, _ in inputs:
            ci = unreal.CustomInput()
            ci.set_editor_property("input_name", n)
            ins.append(ci)
        c.set_editor_property("inputs", ins)
        for n, (e, pin) in inputs:
            mel.connect_material_expressions(e, pin, c, n)
        return c

    base = vparam("BaseColor", (0.035, 0.035, 0.038), 900)
    edgec = vparam("EdgeColor", (0.55, 0.55, 0.56), 1000)
    wear = sparam("EdgeWear", 0.6, 1100)
    grime = sparam("Grime", 0.5, 1150)
    smudge = sparam("SmudgeAmount", 0.12, 1200)
    speckle = sparam("SpeckleAmount", 0.0, 1225)
    scratch = sparam("ScratchAmount", 0.3, 1250)
    rough = sparam("Roughness", 0.45, 1275)
    erough = sparam("EdgeRoughness", 0.3, 1300)
    metal = sparam("Metallic", 0.2, 1350)
    emetal = sparam("EdgeMetallic", 1.0, 1400)
    dstr = sparam("DetailStrength", 0.35, 1450)

    common = [("M", (masks, "RGB")), ("S", (sm, "RGB")), ("W", (wear, "")), ("G", (grime, "")), ("Sc", (scratch, ""))]
    bc = custom_node("float e = max(M.g * W, S.b * Sc); float c = M.b * G;"
                     "float3 col = lerp(B, B * 0.5, c); col *= lerp(1.0, 0.82, S.g * Sp); col = lerp(col, E, e);"
                     "return col;",
                     common + [("B", (base, "")), ("E", (edgec, "")), ("Sp", (speckle, ""))],
                     unreal.CustomMaterialOutputType.CMOT_FLOAT3, 0)
    rg = custom_node("float e = max(M.g * W, S.b * Sc);"
                     "float r = R + (S.r - 0.5) * Sm + (S.g - 0.5) * Sp * 0.25;"
                     "r = lerp(r, ER, e) + M.b * G * 0.12; return saturate(r);",
                     common + [("R", (rough, "")), ("ER", (erough, "")), ("Sm", (smudge, "")), ("Sp", (speckle, ""))],
                     unreal.CustomMaterialOutputType.CMOT_FLOAT1, 200)
    mt = custom_node("return lerp(Mt, EM, max(M.g * W, S.b * Sc));",
                     common + [("Mt", (metal, "")), ("EM", (emetal, ""))],
                     unreal.CustomMaterialOutputType.CMOT_FLOAT1, 350)
    nm = custom_node("float3 n = N; n.xy *= D; return normalize(n);",
                     [("N", (det, "RGB")), ("D", (dstr, ""))], unreal.CustomMaterialOutputType.CMOT_FLOAT3, 500)
    ao = expr(m, unreal.MaterialExpressionComponentMask, -500, 650, r=True, g=False, b=False, a=False)
    mel.connect_material_expressions(masks, "RGB", ao, "")
    mel.connect_material_property(bc, "", unreal.MaterialProperty.MP_BASE_COLOR)
    mel.connect_material_property(rg, "", unreal.MaterialProperty.MP_ROUGHNESS)
    mel.connect_material_property(mt, "", unreal.MaterialProperty.MP_METALLIC)
    mel.connect_material_property(nm, "", unreal.MaterialProperty.MP_NORMAL)
    mel.connect_material_property(ao, "", unreal.MaterialProperty.MP_AMBIENT_OCCLUSION)
    return finish(m)


# instancia -> parámetros. Anodizado negro satinado (metal), cerakote coyote, polímero negro texturizado, goma.
AR7_SURFACES = {
    "MI_AR7_Metal": dict(BaseColor=(0.032, 0.032, 0.035), EdgeColor=(0.32, 0.32, 0.33), EdgeWear=0.35, Grime=0.45,
                         Roughness=0.42, EdgeRoughness=0.32, Metallic=0.3, EdgeMetallic=0.9, DetailStrength=0.25,
                         SmudgeAmount=0.16, SpeckleAmount=0.0, ScratchAmount=0.08),
    "MI_AR7_Polymer": dict(BaseColor=(0.30, 0.235, 0.155), EdgeColor=(0.45, 0.38, 0.28), EdgeWear=0.4, Grime=0.55,
                           Roughness=0.62, EdgeRoughness=0.5, Metallic=0.0, EdgeMetallic=0.0, DetailStrength=0.35,
                           SmudgeAmount=0.12, SpeckleAmount=0.35, ScratchAmount=0.06),
    "MI_AR7_PolymerDark": dict(BaseColor=(0.035, 0.035, 0.035), EdgeColor=(0.12, 0.12, 0.12), EdgeWear=0.35, Grime=0.4,
                               Roughness=0.6, EdgeRoughness=0.45, Metallic=0.0, EdgeMetallic=0.0, DetailStrength=0.4,
                               SmudgeAmount=0.12, SpeckleAmount=0.6, ScratchAmount=0.1),
    "MI_AR7_Rubber": dict(BaseColor=(0.018, 0.018, 0.018), EdgeColor=(0.03, 0.03, 0.03), EdgeWear=0.0, Grime=0.3,
                          Roughness=0.9, EdgeRoughness=0.9, Metallic=0.0, EdgeMetallic=0.0, DetailStrength=0.5,
                          SmudgeAmount=0.06, SpeckleAmount=0.4, ScratchAmount=0.0),
}


def make_tritium_material():
    """Inserto luminoso de la mira: emisivo naranja, visible en cualquier luz."""
    folder = f"{AR7_DIR}/Materials"
    path = f"{folder}/M_AR7_Tritium"
    if lib.does_asset_exist(path):
        lib.delete_asset(path)  # se recrea (antes no tenía el uso con mallas esqueléticas)
    m = tools.create_asset("M_AR7_Tritium", folder, unreal.Material, unreal.MaterialFactoryNew())
    m.set_editor_property("used_with_skeletal_mesh", True)
    mel.connect_material_property(expr(m, unreal.MaterialExpressionConstant3Vector, -300, 0, constant=unreal.LinearColor(1.0, 0.4, 0.05, 1)), "", unreal.MaterialProperty.MP_BASE_COLOR)
    mel.connect_material_property(expr(m, unreal.MaterialExpressionConstant3Vector, -300, 120, constant=unreal.LinearColor(6.0, 2.2, 0.3, 1)), "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    mel.connect_material_property(expr(m, unreal.MaterialExpressionConstant, -300, 240, r=0.3), "", unreal.MaterialProperty.MP_ROUGHNESS)
    return finish(m)


def make_ar7_instances():
    tex = import_weapon_textures()
    master = make_weapon_master(tex)
    scale = weapon_uv_scale()
    out = {}
    for name, params in AR7_SURFACES.items():
        folder = f"{AR7_DIR}/Materials"
        mi = load(f"{folder}/{name}") or tools.create_asset(name, folder, unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
        mi.set_editor_property("parent", master)
        for p, v in params.items():
            if isinstance(v, tuple):
                mel.set_material_instance_vector_parameter_value(mi, p, unreal.LinearColor(*v, 1))
            else:
                mel.set_material_instance_scalar_parameter_value(mi, p, v)
        # tamaño real: grano de 3 cm, manchas de 25 cm
        mel.set_material_instance_scalar_parameter_value(mi, "DetailTiling", scale / 3.0)
        mel.set_material_instance_scalar_parameter_value(mi, "SmudgeTiling", scale / 25.0)
        lib.save_loaded_asset(mi)
        out[name] = mi
    log(f"Materiales del AR-7: escala UV {scale:.1f} cm")
    return out


AR7_MATERIALS = {
    # slot (nombre del material en Blender) -> (nombre en UE, color, rugosidad, metálico)
    "MI_AR7_Metal": ("M_AR7_Metal", (0.035, 0.035, 0.038), 0.42, 0.75),
    "MI_AR7_Polymer": ("M_AR7_Polymer", (0.30, 0.23, 0.15), 0.72, 0.0),
    "MI_AR7_PolymerDark": ("M_AR7_PolymerDark", (0.04, 0.04, 0.04), 0.62, 0.0),
    "MI_AR7_Rubber": ("M_AR7_Rubber", (0.018, 0.018, 0.018), 0.92, 0.0),
}


def simple_pbr_material(name, folder, color, rough, metal):
    path = f"{folder}/{name}"
    existing = load(path)
    if existing:
        return existing
    mm = new_material(name, folder)
    c = expr(mm, unreal.MaterialExpressionConstant3Vector, -300, 0, constant=unreal.LinearColor(*color, 1.0))
    r = expr(mm, unreal.MaterialExpressionConstant, -300, 120, r=rough)
    mt = expr(mm, unreal.MaterialExpressionConstant, -300, 200, r=metal)
    mel.connect_material_property(c, "", unreal.MaterialProperty.MP_BASE_COLOR)
    mel.connect_material_property(r, "", unreal.MaterialProperty.MP_ROUGHNESS)
    mel.connect_material_property(mt, "", unreal.MaterialProperty.MP_METALLIC)
    return finish(mm)


def make_weapon_lods(mesh):
    """LOD automáticos para el arma en tercera persona (IA, suelo). En primera persona siempre se ve el LOD0."""
    path = "/Game/Weapons/Shared/LODSettings_Weapon"
    ls = load(path) or tools.create_asset("LODSettings_Weapon", "/Game/Weapons/Shared", unreal.SkeletalMeshLODSettings, None)
    groups = []
    for pct, screen in ((1.0, 1.0), (0.4, 0.25), (0.15, 0.08)):  # % de triángulos, tamaño en pantalla
        g = unreal.SkeletalMeshLODGroupSettings()
        r = g.get_editor_property("reduction_settings")
        r.set_editor_property("termination_criterion", unreal.SkeletalMeshTerminationCriterion.SMTC_NUM_OF_TRIANGLES)
        r.set_editor_property("num_of_triangles_percentage", pct)
        g.set_editor_property("reduction_settings", r)
        ss = unreal.PerPlatformFloat()
        ss.set_editor_property("default", screen)
        g.set_editor_property("screen_size", ss)
        groups.append(g)
    ls.set_editor_property("lod_groups", groups)
    lib.save_loaded_asset(ls)
    mesh.set_editor_property("lod_settings", ls)
    sub = unreal.get_editor_subsystem(unreal.SkeletalMeshEditorSubsystem)
    if not sub.regenerate_lod(mesh, len(groups), True, False):
        log("AVISO: no se pudieron generar los LOD del arma")
    lib.save_loaded_asset(mesh)
    log(f"{mesh.get_name()}: {sub.get_lod_count(mesh)} LOD")


def import_ar7():
    """SK_AR7 (gen_ar7.py). Devuelve None si aún no se ha generado el FBX."""
    fbx = os.path.join(FBX_DIR, "SK_AR7.fbx")
    if not os.path.exists(fbx):
        log("SK_AR7.fbx no existe: se usa el fusil de Epic")
        return None
    dest = f"{AR7_DIR}/Meshes"
    mesh = None
    for a in import_file(fbx, dest):
        if isinstance(a, unreal.SkeletalMesh):
            mesh = a
    if mesh is None:
        raise RuntimeError("No se importó SK_AR7")
    # Materiales propios por nombre de slot (maestro con desgaste); se borran los autogenerados
    instances = make_ar7_instances()
    instances["MI_AR7_Tritium"] = make_tritium_material()
    # Ojo: los elementos de la lista son copias; hay que construir una lista nueva y asignarla entera.
    # (Antes se modificaban copias y luego se borraban los autogenerados: las ranuras quedaban vacías y el arma
    # se veía con el material por defecto de Unreal, el gris "moteado".)
    autos, new_mats = [], []
    for m in mesh.get_editor_property("materials"):
        slot = str(m.get_editor_property("material_slot_name"))
        auto = m.get_editor_property("material_interface")
        if slot not in instances:
            raise RuntimeError(f"Slot desconocido en SK_AR7: {slot}")
        new_mats.append(unreal.SkeletalMaterial(material_interface=instances[slot], material_slot_name=slot,
                                                uv_channel_data=m.get_editor_property("uv_channel_data")))
        if auto and auto.get_path_name().startswith(dest):
            autos.append(auto)
    mesh.set_editor_property("materials", new_mats)
    lib.save_loaded_asset(mesh)
    for a in autos:
        lib.delete_loaded_asset(a)
    for m in mesh.get_editor_property("materials"):
        mi = m.get_editor_property("material_interface")
        if not mi or not mi.get_path_name().startswith(f"{AR7_DIR}/Materials"):
            raise RuntimeError(f"SK_AR7: slot {m.get_editor_property('material_slot_name')} sin material propio ({mi})")
    make_weapon_lods(mesh)
    # Comprobar orientación: el cañón debe ir hacia +Y
    skel = unreal.SkeletalMeshEditorSubsystem if hasattr(unreal, "SkeletalMeshEditorSubsystem") else None
    b = mesh.get_bounds()
    log(f"SK_AR7 importado: bounds origin={b.origin} extent={b.box_extent}")
    return mesh


def make_niagara():
    path = f"{FX_DIR}/NS_Impact_Placeholder"
    existing = load(path)
    if existing:
        return existing
    template = unreal.load_asset("/Niagara/DefaultAssets/Templates/Systems/DirectionalBurst")
    if not template:
        log("AVISO: no se encontró la plantilla de Niagara; impactos sin partículas")
        return None
    dup = tools.duplicate_asset("NS_Impact_Placeholder", FX_DIR, template)
    if dup:
        lib.save_loaded_asset(dup)
    return dup


# ---------------------------------------------------------------------------
# DA_AR7
# ---------------------------------------------------------------------------

def timed(t, sound):
    s = unreal.BLTimedSound()
    s.set_editor_property("time", t)
    s.set_editor_property("sound", sound)
    return s


def audio_list(folder, prefix=""):
    out = []
    for p in sorted(lib.list_assets(folder, recursive=False)):
        a = unreal.load_asset(p.split(".")[0])
        if isinstance(a, unreal.SoundWave) and a.get_name().startswith(prefix):
            out.append(a)
    return out


def ar7(prefix):
    return audio_list("/Game/Audio/Weapons/AR7", prefix)


def one(name):
    return load(f"/Game/Audio/Weapons/AR7/{name}")


def make_weapon_data(sounds, mats, meshes, niagara):
    path = f"{AR7_DIR}/DA_AR7"
    da = load(path)
    if not da:
        factory = unreal.DataAssetFactory()
        factory.set_editor_property("data_asset_class", unreal.BLWeaponData)
        da = tools.create_asset("DA_AR7", AR7_DIR, unreal.BLWeaponData, factory)

    def snd(n):
        s = sounds.get(n)
        if not s:
            log(f"AVISO: falta el sonido {n}")
        return s

    def many(prefix):
        return [s for n, s in sorted(sounds.items()) if n.startswith(prefix)]

    sk_ar7 = import_ar7()
    mag_mesh = None
    if sk_ar7:
        mag_mesh = import_mesh("SM_AR7_Mag", f"{AR7_DIR}/Meshes", None)
        dark = load(f"{AR7_DIR}/Materials/MI_AR7_PolymerDark")
        mag_mats = [unreal.StaticMaterial(material_interface=dark, material_slot_name=m.get_editor_property("material_slot_name"),
                                          uv_channel_data=m.get_editor_property("uv_channel_data"))
                    for m in mag_mesh.get_editor_property("static_materials")]
        mag_mesh.set_editor_property("static_materials", mag_mats)
        lib.save_loaded_asset(mag_mesh)
    if sk_ar7:
        mesh, left, eject, sight, eye = sk_ar7, "HandGrip_L", "Eject", "Sight", 13.0
    else:
        # Provisional: fusil de Epic (sin sockets de mira ni expulsión: se usan desplazamientos)
        mesh, left, eject, sight, eye = unreal.load_asset("/Game/Weapons/Rifle/Meshes/SKM_Rifle"), "GripPoint_002", "Eject", "Sight", 14.0

    anim = "/Game/Characters/Mannequins/Anims/Rifle/"
    fire = unreal.load_asset("/Game/Weapons/GrenadeLauncher/Audio/FirstPersonTemplateWeaponFire02")
    props = {
        "display_name": unreal.Text('AR-7 "Halcón"'),
        "mesh": mesh,
        "muzzle_socket": "Muzzle",
        "eject_socket": eject,
        "left_hand_socket": left,
        "sight_socket": sight,
        "aim_eye_distance": eye,
        "magazine_mesh": mag_mesh,
        "mag_socket": "Mag",
        "charging_handle_socket": "charging_handle",
        "idle_anim": unreal.load_asset(anim + "MF_Rifle_Idle_ADS"),
        "reload_anim": unreal.load_asset(anim + "MM_Rifle_Reload"),
        "equip_anim": unreal.load_asset(anim + "MM_Rifle_Equip"),
        # Sonidos reales (CC0) preparados por Tools/audio/process_sfx.py e importados por setup_audio_fx.py
        "fire_sounds": ar7("SW_AR7_Fire_Close_"),
        "fire_tail_sounds": ar7("SW_AR7_Tail_"),
        "fire_sounds3d": ar7("SW_AR7_Fire_Close3D_"),
        "fire_distant_sounds": ar7("SW_AR7_Fire_Distant_"),
        "dry_fire_sound": one("SW_AR7_DryFire"),
        # Momentos (fracción de la recarga) sincronizados con la línea de tiempo de ABLCharacter
        "reload_sounds": [timed(0.15, one("SW_AR7_MagOut")), timed(0.59, one("SW_AR7_MagIn"))],
        "empty_reload_sounds": [timed(0.13, one("SW_AR7_MagOut")), timed(0.55, one("SW_AR7_MagIn")),
                                timed(0.71, one("SW_AR7_BoltRelease"))],
        "casing_sounds": audio_list("/Game/Audio/Casings/Concrete"),
        "handling_sounds": audio_list("/Game/Audio/Foley"),
        "surface_effects": load("/Game/FX/DA_SurfaceEffects"),
        "muzzle_flash_mesh": meshes["flash"],
        "casing_mesh": meshes["casing"],
        "reload_time": 2.0,
        "empty_reload_time": 2.5,
        "reload_ammo_insert_time": 0.59,
    }
    for k, v in props.items():
        da.set_editor_property(k, v)
    lib.save_loaded_asset(da)
    log(f"DA_AR7 listo (malla: {mesh.get_path_name()})")


def ensure_ism_usage(mat):
    """Los casquillos y esquirlas se dibujan con InstancedStaticMesh: el material necesita ese uso."""
    if mat and not mat.get_editor_property("used_with_instanced_static_meshes"):
        mat.set_editor_property("used_with_instanced_static_meshes", True)
        mel.recompile_material(mat)
        lib.save_loaded_asset(mat)


def make_arms_materials():
    """Brazos FP: el Mannequin blanco se viste con tela de uniforme oscura y mate (sin logotipo emisivo)."""
    folder = "/Game/Characters/Player/Materials"
    out = {}
    for name, parent in (("MI_Soldier_Body", "/Game/Characters/Mannequins/Materials/Manny/MI_Manny_01_New"),
                         ("MI_Soldier_Sleeves", "/Game/Characters/Mannequins/Materials/Manny/MI_Manny_02_New")):
        mi = load(f"{folder}/{name}") or tools.create_asset(name, folder, unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
        mi.set_editor_property("parent", unreal.load_asset(parent))
        mel.set_material_instance_vector_parameter_value(mi, "Paint Tint", unreal.LinearColor(0.07, 0.075, 0.055, 1))  # verde oliva oscuro
        mel.set_material_instance_vector_parameter_value(mi, "LogoTint", unreal.LinearColor(0, 0, 0, 1))
        mel.set_material_instance_scalar_parameter_value(mi, "EmissivePower", 0.0)
        mel.set_material_instance_scalar_parameter_value(mi, "MetalPaintRoughness", 0.85)
        mel.set_material_instance_scalar_parameter_value(mi, "MetalPaintMetallic", 0.0)
        lib.save_loaded_asset(mi)
        out[name] = mi
    log("Materiales de soldado listos")
    return out


def main():
    # Requiere haber ejecutado antes setup_audio_fx.py (sonidos, DA_SurfaceEffects)
    make_arms_materials()
    mats = make_materials()
    ensure_ism_usage(mats["brass"])
    meshes = {
        "flash": import_mesh("SM_AR7_MuzzleFlash", f"{AR7_DIR}/Meshes", [mats["M_MuzzleFlame"], mats["M_MuzzleStar"]]),
        "casing": import_mesh("SM_Casing_556", FX_MESH_DIR, mats["brass"]),
    }
    make_weapon_data({}, mats, meshes, None)
    log("OK")


main()
