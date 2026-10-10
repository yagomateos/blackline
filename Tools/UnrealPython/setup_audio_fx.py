"""Crea/actualiza audio, efectos de impacto por superficie y materiales de entorno. Idempotente.

  - Sonidos: ArtSource/Audio/SFX/** -> /Game/Audio/** (atenuación, reverb, concurrencia, volumen).
  - Texturas de efectos (ArtSource/Textures/FX) y de entorno (ArtSource/Textures/PolyHaven).
  - Mallas de escombros y SM_FX_Quad (ArtSource/Blender/export).
  - Materiales: M_FX_Dust, M_FX_Spark, M_Decal_Impact (+ MI por superficie), M_Env_Triplanar (+ MI),
    vidrio y escombros. Materiales físicos PM_<Superficie>.
  - DA_SurfaceEffects (UBLSurfaceEffectsData).

Uso: UnrealEditor-Cmd.exe Blackline.uproject -run=pythonscript -script="Tools/UnrealPython/setup_audio_fx.py"
Los materiales solo se crean si no existen (para regenerar uno, borrarlo antes).
"""
import glob
import os
import unreal

PROJECT = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())
SFX_DIR = os.path.join(PROJECT, "ArtSource", "Audio", "SFX")
FX_TEX_DIR = os.path.join(PROJECT, "ArtSource", "Textures", "FX")
PH_DIR = os.path.join(PROJECT, "ArtSource", "Textures", "PolyHaven")
FBX_DIR = os.path.join(PROJECT, "ArtSource", "Blender", "export")

tools = unreal.AssetToolsHelpers.get_asset_tools()
lib = unreal.EditorAssetLibrary
mel = unreal.MaterialEditingLibrary

SURFACES = ["Concrete", "Metal", "Wood", "Glass", "Dirt"]


def log(m):
    unreal.log(f"[BL_AudioFX] {m}")


def load(path):
    return unreal.load_asset(path) if lib.does_asset_exist(path) else None


def import_file(filename, dest, options=None):
    task = unreal.AssetImportTask()
    task.filename = filename
    task.destination_path = dest
    task.automated = True
    task.replace_existing = True
    task.save = True
    if options:
        task.options = options
    tools.import_asset_tasks([task])
    return [unreal.load_asset(p) for p in task.imported_object_paths]


def get_or_create(name, folder, cls, factory):
    path = f"{folder}/{name}"
    return load(path) or tools.create_asset(name, folder, cls, factory)


# ---------------------------------------------------------------------------
# Audio
# ---------------------------------------------------------------------------

def attenuation(name, inner, falloff, lpf_far=4000.0, reverb=True, occlusion=False):
    att = get_or_create(name, "/Game/Audio/Settings", unreal.SoundAttenuation, unreal.SoundAttenuationFactory())
    s = att.get_editor_property("attenuation")
    s.set_editor_property("attenuation_shape_extents", unreal.Vector(inner, 0, 0))
    s.set_editor_property("falloff_distance", falloff)
    s.set_editor_property("distance_algorithm", unreal.AttenuationDistanceModel.NATURAL_SOUND)
    s.set_editor_property("spatialize", True)
    # Absorción del aire: los sonidos lejanos pierden agudos
    s.set_editor_property("attenuate_with_lpf", True)
    s.set_editor_property("lpf_radius_min", inner)
    s.set_editor_property("lpf_radius_max", inner + falloff)
    s.set_editor_property("lpf_frequency_at_min", 20000.0)
    s.set_editor_property("lpf_frequency_at_max", lpf_far)
    s.set_editor_property("enable_reverb_send", reverb)
    s.set_editor_property("enable_occlusion", occlusion)
    if occlusion:
        s.set_editor_property("occlusion_low_pass_filter_frequency", 1500.0)
        s.set_editor_property("occlusion_volume_attenuation", 0.6)
    att.set_editor_property("attenuation", s)
    lib.save_loaded_asset(att)
    return att


def concurrency(name, max_count):
    c = get_or_create(name, "/Game/Audio/Settings", unreal.SoundConcurrency, unreal.SoundConcurrencyFactory())
    s = c.get_editor_property("concurrency")
    s.set_editor_property("max_count", max_count)
    s.set_editor_property("resolution_rule", unreal.MaxConcurrentResolutionRule.STOP_OLDEST)
    c.set_editor_property("concurrency", s)
    lib.save_loaded_asset(c)
    return c


def import_audio():
    att = {
        "impact": attenuation("SA_Impact", 150, 4000, 3000, True, True),
        "casing": attenuation("SA_Casing", 60, 900, 6000, True, False),
        "step": attenuation("SA_Footstep", 100, 1500, 5000, True, False),
        "fire": attenuation("SA_WeaponFire", 800, 12000, 5000, True, False),
        "far": attenuation("SA_WeaponFar", 2000, 40000, 2500, True, False),
    }
    con = {
        "impact": concurrency("SCon_Impacts", 10),
        "casing": concurrency("SCon_Casings", 5),
        "step": concurrency("SCon_Footsteps", 4),
        "fire": concurrency("SCon_WeaponFire", 8),
    }
    # (patrón de ruta relativa, atenuación, concurrencia, volumen, bucle)
    rules = [
        ("Weapons/AR7/SW_AR7_Fire_Close_", None, "fire", 1.0, False),
        ("Weapons/AR7/SW_AR7_Tail_", None, None, 0.8, False),
        ("Weapons/AR7/SW_AR7_Fire_Close3D_", "fire", "fire", 1.0, False),
        ("Weapons/AR7/SW_AR7_Fire_Distant_", "far", "fire", 1.0, False),
        ("Weapons/AR7/", None, None, 0.8, False),           # recarga, gatillo
        ("Impacts/", "impact", "impact", 0.75, False),
        ("Casings/", "casing", "casing", 0.45, False),
        ("Footsteps/", "step", "step", 0.6, False),
        ("Foley/", None, None, 0.35, False),
        ("Ambience/", None, None, 0.5, True),
        ("UI/", None, None, 0.55, False),                     # hitmarker, objetivo, recoger (2D)
        ("Radio/", None, None, 0.6, False),                   # radio de la misión (2D)
        ("Player/SW_Player_Heartbeat", None, None, 0.7, True),
        ("Player/", None, None, 0.8, False),                  # daño recibido (2D)
    ]
    count = 0
    for wav in sorted(glob.glob(os.path.join(SFX_DIR, "**", "*.wav"), recursive=True)):
        rel = os.path.relpath(wav, SFX_DIR).replace("\\", "/")
        dest = "/Game/Audio/" + os.path.dirname(rel)
        for a in import_file(wav, dest):
            if not isinstance(a, unreal.SoundWave):
                continue
            for pattern, a_key, c_key, vol, loop in rules:
                if rel.startswith(pattern):
                    a.set_editor_property("attenuation_settings", att[a_key] if a_key else None)
                    a.set_editor_property("concurrency_set", set([con[c_key]]) if c_key else set())
                    a.set_editor_property("volume", vol)
                    a.set_editor_property("looping", loop)
                    break
            lib.save_loaded_asset(a)
            count += 1
    log(f"Sonidos importados: {count}")


def sounds(folder, prefix=""):
    """Lista de SoundWaves de una carpeta de /Game/Audio (ordenadas)."""
    out = []
    for p in sorted(lib.list_assets(f"/Game/Audio/{folder}", recursive=False)):
        a = unreal.load_asset(p.split(".")[0])
        if isinstance(a, unreal.SoundWave) and a.get_name().startswith(prefix):
            out.append(a)
    return out


# ---------------------------------------------------------------------------
# Texturas
# ---------------------------------------------------------------------------

def import_texture(path, dest, kind):
    t = None
    for a in import_file(path, dest):
        if isinstance(a, unreal.Texture2D):
            t = a
    if not t:
        raise RuntimeError(f"No se importó {path}")
    if kind == "normal":
        t.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_NORMALMAP)
        t.set_editor_property("srgb", False)
    elif kind == "mask":
        t.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_MASKS)
        t.set_editor_property("srgb", False)
    else:
        t.set_editor_property("srgb", True)
    lib.save_loaded_asset(t)
    return t


def import_textures():
    tex = {}
    for f in sorted(glob.glob(os.path.join(FX_TEX_DIR, "*.png"))):
        name = os.path.splitext(os.path.basename(f))[0]
        tex[name] = import_texture(f, "/Game/FX/Textures", "normal" if name.endswith("_N") else "color")
    for f in sorted(glob.glob(os.path.join(PH_DIR, "*.jpg"))):
        name = os.path.splitext(os.path.basename(f))[0]
        kind = "normal" if "_nor_" in name else "mask" if "_arm_" in name else "color"
        tex[name] = import_texture(f, "/Game/Environment/Textures", kind)
    log(f"Texturas: {len(tex)}")
    return tex


# ---------------------------------------------------------------------------
# Materiales
# ---------------------------------------------------------------------------

def new_material(name, folder):
    if lib.does_asset_exist(f"{folder}/{name}"):
        return None
    return tools.create_asset(name, folder, unreal.Material, unreal.MaterialFactoryNew())


def expr(mat, cls, x, y, **props):
    e = mel.create_material_expression(mat, cls, x, y)
    for k, v in props.items():
        e.set_editor_property(k, v)
    return e


def custom(mat, x, y, code, inputs, out_type=unreal.CustomMaterialOutputType.CMOT_FLOAT1):
    c = expr(mat, unreal.MaterialExpressionCustom, x, y, code=code, output_type=out_type)
    ins = []
    for name in inputs:
        ci = unreal.CustomInput()
        ci.set_editor_property("input_name", name)
        ins.append(ci)
    c.set_editor_property("inputs", ins)
    return c


def finish(mat):
    mel.recompile_material(mat)
    lib.save_loaded_asset(mat)
    return mat


def instance_data(mat, idx, x, y):
    return expr(mat, unreal.MaterialExpressionPerInstanceCustomData, x, y, data_index=idx)


def make_fx_materials(tex):
    folder = "/Game/FX/Materials"
    # ---- Polvo / humo: translúcido iluminado, atlas 2x2, color y opacidad por instancia
    m = new_material("M_FX_Dust", folder)
    if m:
        m.set_editor_property("blend_mode", unreal.BlendMode.BLEND_TRANSLUCENT)
        m.set_editor_property("two_sided", True)
        m.set_editor_property("translucency_lighting_mode", unreal.TranslucencyLightingMode.TLM_VOLUMETRIC_PER_VERTEX_NON_DIRECTIONAL)
        m.set_editor_property("used_with_instanced_static_meshes", True)
        uv = expr(m, unreal.MaterialExpressionTextureCoordinate, -900, 0)
        frame = instance_data(m, 1, -900, 120)
        atlas_uv = custom(m, -650, 0, "float2 o = float2(fmod(F, 2.0), floor(F / 2.0)) * 0.5; return UV * 0.5 + o;", ["UV", "F"], unreal.CustomMaterialOutputType.CMOT_FLOAT2)
        mel.connect_material_expressions(uv, "", atlas_uv, "UV")
        mel.connect_material_expressions(frame, "", atlas_uv, "F")
        ts = expr(m, unreal.MaterialExpressionTextureSample, -400, 0, texture=tex["T_FX_SmokeAtlas"])
        mel.connect_material_expressions(atlas_uv, "", ts, "UVs")
        col = custom(m, -400, 300, "return float3(R, G, B);", ["R", "G", "B"], unreal.CustomMaterialOutputType.CMOT_FLOAT3)
        for i, n in enumerate(("R", "G", "B")):
            mel.connect_material_expressions(instance_data(m, 2 + i, -650, 300 + i * 80), "", col, n)
        bc = expr(m, unreal.MaterialExpressionMultiply, -150, 0)
        mel.connect_material_expressions(ts, "RGB", bc, "A")
        mel.connect_material_expressions(col, "", bc, "B")
        mel.connect_material_property(bc, "", unreal.MaterialProperty.MP_BASE_COLOR)
        # opacidad = alfa * opacidad de la instancia * fundido al tocar superficies (partícula suave)
        fade = expr(m, unreal.MaterialExpressionDepthFade, -400, 500, fade_distance_default=25.0)
        op = custom(m, -150, 200, "return saturate(A * O * D);", ["A", "O", "D"])
        mel.connect_material_expressions(ts, "A", op, "A")
        mel.connect_material_expressions(instance_data(m, 0, -400, 420), "", op, "O")
        mel.connect_material_expressions(fade, "", op, "D")
        mel.connect_material_property(op, "", unreal.MaterialProperty.MP_OPACITY)
        mel.connect_material_property(expr(m, unreal.MaterialExpressionConstant, -150, 400, r=1.0), "", unreal.MaterialProperty.MP_ROUGHNESS)
        finish(m)
    # ---- Chispas: aditivas, estela con núcleo caliente
    m = new_material("M_FX_Spark", folder)
    if m:
        m.set_editor_property("blend_mode", unreal.BlendMode.BLEND_ADDITIVE)
        m.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
        m.set_editor_property("two_sided", True)
        m.set_editor_property("used_with_instanced_static_meshes", True)
        uv = expr(m, unreal.MaterialExpressionTextureCoordinate, -700, 0)
        code = ("float across = exp(-pow((UV.x - 0.5) * 5.0, 2.0));"
                "float along = smoothstep(0.0, 0.25, UV.y) * smoothstep(1.0, 0.6, UV.y);"
                "float core = pow(across, 3.0);"
                "float3 c = lerp(float3(R, G, B), float3(1.0, 0.95, 0.8), core);"
                "return c * across * along * I * 60.0;")
        c = custom(m, -400, 0, code, ["UV", "I", "R", "G", "B"], unreal.CustomMaterialOutputType.CMOT_FLOAT3)
        mel.connect_material_expressions(uv, "", c, "UV")
        for i, n in enumerate(("I", "R", "G", "B")):
            mel.connect_material_expressions(instance_data(m, i, -700, 120 + i * 80), "", c, n)
        mel.connect_material_property(c, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
        finish(m)
    # ---- Decal de impacto (maestro) + instancias por superficie
    m = new_material("M_Decal_Impact", folder)
    if m:
        m.set_editor_property("material_domain", unreal.MaterialDomain.MD_DEFERRED_DECAL)
        m.set_editor_property("blend_mode", unreal.BlendMode.BLEND_TRANSLUCENT)
        bc = expr(m, unreal.MaterialExpressionTextureSampleParameter2D, -500, 0, parameter_name="Color", texture=tex["T_Decal_Concrete_BC"])
        nm = expr(m, unreal.MaterialExpressionTextureSampleParameter2D, -500, 300, parameter_name="Normal", texture=tex["T_Decal_Concrete_N"],
                  sampler_type=unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL)
        rough = expr(m, unreal.MaterialExpressionScalarParameter, -300, 500, parameter_name="Roughness", default_value=0.85)
        metal = expr(m, unreal.MaterialExpressionScalarParameter, -300, 600, parameter_name="Metallic", default_value=0.0)
        spec = expr(m, unreal.MaterialExpressionScalarParameter, -300, 700, parameter_name="Specular", default_value=0.5)
        mel.connect_material_property(bc, "RGB", unreal.MaterialProperty.MP_BASE_COLOR)
        mel.connect_material_property(bc, "A", unreal.MaterialProperty.MP_OPACITY)
        mel.connect_material_property(nm, "RGB", unreal.MaterialProperty.MP_NORMAL)
        mel.connect_material_property(rough, "", unreal.MaterialProperty.MP_ROUGHNESS)
        mel.connect_material_property(metal, "", unreal.MaterialProperty.MP_METALLIC)
        mel.connect_material_property(spec, "", unreal.MaterialProperty.MP_SPECULAR)
        finish(m)
    decal_master = load(f"{folder}/M_Decal_Impact")
    decals = {}
    params = {"Concrete": (0.9, 0.0), "Metal": (0.35, 0.8), "Wood": (0.8, 0.0), "Glass": (0.1, 0.0), "Dirt": (0.95, 0.0)}
    for s in SURFACES:
        mi = get_or_create(f"MI_Decal_{s}", folder, unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
        mi.set_editor_property("parent", decal_master)
        mel.set_material_instance_texture_parameter_value(mi, "Color", tex[f"T_Decal_{s}_BC"])
        mel.set_material_instance_texture_parameter_value(mi, "Normal", tex[f"T_Decal_{s}_N"])
        mel.set_material_instance_scalar_parameter_value(mi, "Roughness", params[s][0])
        mel.set_material_instance_scalar_parameter_value(mi, "Metallic", params[s][1])
        lib.save_loaded_asset(mi)
        decals[s] = mi
    # Sangre (salpicadura detrás de un impacto en carne): húmeda, poco rugosa
    mi = get_or_create("MI_Decal_Blood", folder, unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
    mi.set_editor_property("parent", decal_master)
    mel.set_material_instance_texture_parameter_value(mi, "Color", tex["T_Decal_Blood_BC"])
    mel.set_material_instance_texture_parameter_value(mi, "Normal", tex["T_Decal_Blood_N"])
    mel.set_material_instance_scalar_parameter_value(mi, "Roughness", 0.22)
    mel.set_material_instance_scalar_parameter_value(mi, "Metallic", 0.0)
    lib.save_loaded_asset(mi)
    decals["Blood"] = mi
    return decals


def simple_pbr(name, folder, color, rough, metal, ism=False, translucent_opacity=None):
    path = f"{folder}/{name}"
    existing = load(path)
    if existing:
        return existing
    m = new_material(name, folder)
    if ism:
        m.set_editor_property("used_with_instanced_static_meshes", True)
    if translucent_opacity is not None:
        m.set_editor_property("blend_mode", unreal.BlendMode.BLEND_TRANSLUCENT)
        m.set_editor_property("translucency_lighting_mode", unreal.TranslucencyLightingMode.TLM_SURFACE_PER_PIXEL_LIGHTING)
        mel.connect_material_property(expr(m, unreal.MaterialExpressionConstant, -300, 300, r=translucent_opacity), "", unreal.MaterialProperty.MP_OPACITY)
    mel.connect_material_property(expr(m, unreal.MaterialExpressionConstant3Vector, -300, 0, constant=unreal.LinearColor(*color, 1.0)), "", unreal.MaterialProperty.MP_BASE_COLOR)
    mel.connect_material_property(expr(m, unreal.MaterialExpressionConstant, -300, 120, r=rough), "", unreal.MaterialProperty.MP_ROUGHNESS)
    mel.connect_material_property(expr(m, unreal.MaterialExpressionConstant, -300, 200, r=metal), "", unreal.MaterialProperty.MP_METALLIC)
    return finish(m)


# Antirrepetición (2026-10-09, el usuario veía la cuadrícula del suelo): la proyección de arriba (suelos) mezcla dos
# muestras, la normal y otra girada 37° a 0,71 de escala, con una máscara de baja frecuencia; el color base lleva
# además una variación de tono a gran escala. Paredes (proyecciones laterales): una muestra, como antes.
_AT_TOP = (
    "float2 q = p.xy;"
    "float2 r = float2(q.x * 0.8 - q.y * 0.6, q.x * 0.6 + q.y * 0.8) * 0.71 + float2(0.37, 0.61);"
    "float k = sin(WP.x * 0.00041 + 1.7 * sin(WP.y * 0.00029)) * sin(WP.y * 0.00037 + 1.3 * sin(WP.x * 0.00023));"
    "float m = smoothstep(-0.2, 0.2, k);")
TRIPLANAR_SAMPLE = (
    "float3 n = pow(abs(WN), 4.0); n /= (n.x + n.y + n.z);"
    "float3 p = WP / Tile;" + _AT_TOP +
    "float4 a = Tex.Sample(TexSampler, p.zy);"
    "float4 b = Tex.Sample(TexSampler, p.xz);"
    "float4 c = lerp(Tex.Sample(TexSampler, q), Tex.Sample(TexSampler, r), m);"
    "return (a * n.x + b * n.y + c * n.z).rgb;")
TRIPLANAR_SAMPLE_BC = TRIPLANAR_SAMPLE.replace(
    "return (a * n.x + b * n.y + c * n.z).rgb;",
    "float g = sin(WP.x * 0.00013 + 2.1 * sin(WP.y * 0.00011)) * sin(WP.y * 0.00017 + 1.9 * sin(WP.x * 0.00009));"
    "return (a * n.x + b * n.y + c * n.z).rgb * lerp(0.84, 1.07, saturate(0.5 + 0.5 * g));")
TRIPLANAR_NORMAL = (
    # Mezcla "whiteout" de normales triplanares -> normal en espacio de mundo
    "float3 w = pow(abs(WN), 4.0); w /= (w.x + w.y + w.z);"
    "float3 p = WP / Tile;" + _AT_TOP +
    "float3 tx = Tex.Sample(TexSampler, p.zy).xyz * 2.0 - 1.0;"
    "float3 ty = Tex.Sample(TexSampler, p.xz).xyz * 2.0 - 1.0;"
    "float3 t0 = Tex.Sample(TexSampler, q).xyz * 2.0 - 1.0;"
    "float3 t1 = Tex.Sample(TexSampler, r).xyz * 2.0 - 1.0;"
    "t1.xy = float2(0.8 * t1.x + 0.6 * t1.y, -0.6 * t1.x + 0.8 * t1.y);"   # deshace el giro de la muestra girada
    "float3 tz = normalize(lerp(t0, t1, m));"
    "tx.xy *= Strength; ty.xy *= Strength; tz.xy *= Strength;"
    "tx = float3(tx.xy + WN.zy, abs(tx.z) * WN.x);"
    "ty = float3(ty.xy + WN.xz, abs(ty.z) * WN.y);"
    "tz = float3(tz.xy + WN.xy, abs(tz.z) * WN.z);"
    "return normalize(tx.zyx * w.x + ty.xzy * w.y + tz.xyz * w.z);")


ENV_MASTER = "M_Env_Triplanar_AT"


def make_env_master(tex):
    folder = "/Game/Environment/Materials"
    m = new_material(ENV_MASTER, folder)
    if m:
        m.set_editor_property("tangent_space_normal", False)
        wp = expr(m, unreal.MaterialExpressionWorldPosition, -1100, 0)
        wn = expr(m, unreal.MaterialExpressionVertexNormalWS, -1100, 150)
        tile = expr(m, unreal.MaterialExpressionScalarParameter, -1100, 250, parameter_name="TileSize", default_value=200.0)
        strength = expr(m, unreal.MaterialExpressionScalarParameter, -1100, 350, parameter_name="NormalStrength", default_value=1.0)
        default = tex["concrete_wall_008_diff_2k"]
        objs = {}
        for i, (pname, t) in enumerate((("BaseColorMap", default), ("NormalMap", tex["concrete_wall_008_nor_dx_2k"]), ("ARMMap", tex["concrete_wall_008_arm_2k"]))):
            o = expr(m, unreal.MaterialExpressionTextureObjectParameter, -1100, 500 + i * 200, parameter_name=pname, texture=t)
            if pname == "NormalMap":
                o.set_editor_property("sampler_type", unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL)
            elif pname == "ARMMap":
                o.set_editor_property("sampler_type", unreal.MaterialSamplerType.SAMPLERTYPE_MASKS)
            objs[pname] = o

        def tri(code, tex_obj, y, extra=()):
            c = custom(m, -700, y, code, ["Tex", "WP", "WN", "Tile"] + list(extra), unreal.CustomMaterialOutputType.CMOT_FLOAT3)
            mel.connect_material_expressions(tex_obj, "", c, "Tex")
            mel.connect_material_expressions(wp, "", c, "WP")
            mel.connect_material_expressions(wn, "", c, "WN")
            mel.connect_material_expressions(tile, "", c, "Tile")
            return c
        bc = tri(TRIPLANAR_SAMPLE_BC, objs["BaseColorMap"], 0)
        nrm = tri(TRIPLANAR_NORMAL, objs["NormalMap"], 250, ["Strength"])
        mel.connect_material_expressions(strength, "", nrm, "Strength")
        arm = tri(TRIPLANAR_SAMPLE, objs["ARMMap"], 500)
        tint = expr(m, unreal.MaterialExpressionVectorParameter, -500, -150, parameter_name="Tint", default_value=unreal.LinearColor(1, 1, 1, 1))
        bct = expr(m, unreal.MaterialExpressionMultiply, -350, 0)
        mel.connect_material_expressions(bc, "", bct, "A")
        mel.connect_material_expressions(tint, "", bct, "B")
        mel.connect_material_property(bct, "", unreal.MaterialProperty.MP_BASE_COLOR)
        mel.connect_material_property(nrm, "", unreal.MaterialProperty.MP_NORMAL)
        rmul = expr(m, unreal.MaterialExpressionScalarParameter, -500, 700, parameter_name="RoughnessScale", default_value=1.0)
        mmul = expr(m, unreal.MaterialExpressionScalarParameter, -500, 800, parameter_name="MetallicScale", default_value=1.0)
        split = custom(m, -350, 500, "return float3(ARM.r, saturate(ARM.g * RS), saturate(ARM.b * MS));", ["ARM", "RS", "MS"], unreal.CustomMaterialOutputType.CMOT_FLOAT3)
        mel.connect_material_expressions(arm, "", split, "ARM")
        mel.connect_material_expressions(rmul, "", split, "RS")
        mel.connect_material_expressions(mmul, "", split, "MS")
        ao = expr(m, unreal.MaterialExpressionComponentMask, -150, 450, r=True, g=False, b=False, a=False)
        rg = expr(m, unreal.MaterialExpressionComponentMask, -150, 550, r=False, g=True, b=False, a=False)
        mt = expr(m, unreal.MaterialExpressionComponentMask, -150, 650, r=False, g=False, b=True, a=False)
        for e, prop in ((ao, unreal.MaterialProperty.MP_AMBIENT_OCCLUSION), (rg, unreal.MaterialProperty.MP_ROUGHNESS), (mt, unreal.MaterialProperty.MP_METALLIC)):
            mel.connect_material_expressions(split, "", e, "")
            mel.connect_material_property(e, "", prop)
        finish(m)
    return load(f"{folder}/{ENV_MASTER}")


def make_physical_materials():
    pms = {}
    surface_enum = {n: i + 1 for i, n in enumerate(("Concrete", "Metal", "Wood", "Glass", "Dirt", "Flesh"))}
    for name, st in surface_enum.items():
        pm = get_or_create(f"PM_{name}", "/Game/Environment/PhysicalMaterials", unreal.PhysicalMaterial, unreal.PhysicalMaterialFactoryNew())
        unreal.BLScriptLibrary.set_physical_surface(pm, st)  # el enum no se expone bien a Python
        lib.save_loaded_asset(pm)
        pms[name] = pm
    return pms


def make_env_instances(master, tex, pms):
    # nombre: (textura base de Poly Haven, tamaño de repetición en cm, tinte, superficie, escala de rugosidad)
    defs = {
        "MI_Env_ConcreteWall": ("concrete_wall_008", 250, (1, 1, 1), "Concrete", 1.0),
        "MI_Env_ConcreteFloor": ("concrete_floor_worn_001", 300, (1, 1, 1), "Concrete", 1.0),
        "MI_Env_Plaster": ("plastered_wall_04", 220, (1, 1, 1), "Concrete", 1.0),
        "MI_Env_Asphalt": ("asphalt_02", 400, (1, 1, 1), "Concrete", 1.0),
        "MI_Env_MetalPlate": ("metal_plate_02", 150, (1, 1, 1), "Metal", 1.0),
        "MI_Env_RustyMetal": ("rusty_metal_02", 200, (1, 1, 1), "Metal", 1.0),
        "MI_Env_WoodPlanks": ("old_planks_02", 200, (1, 1, 1), "Wood", 1.0),
        "MI_Env_Dirt": ("dirt", 300, (1, 1, 1), "Dirt", 1.0),
    }
    out = {}
    for name, (tid, tile, tint, surface, rs) in defs.items():
        mi = get_or_create(name, "/Game/Environment/Materials", unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
        mi.set_editor_property("parent", master)
        mel.set_material_instance_texture_parameter_value(mi, "BaseColorMap", tex[f"{tid}_diff_2k"])
        mel.set_material_instance_texture_parameter_value(mi, "NormalMap", tex[f"{tid}_nor_dx_2k"])
        mel.set_material_instance_texture_parameter_value(mi, "ARMMap", tex[f"{tid}_arm_2k"])
        mel.set_material_instance_scalar_parameter_value(mi, "TileSize", tile)
        mel.set_material_instance_scalar_parameter_value(mi, "RoughnessScale", rs)
        mel.set_material_instance_vector_parameter_value(mi, "Tint", unreal.LinearColor(*tint, 1))
        mi.set_editor_property("phys_material", pms[surface])
        lib.save_loaded_asset(mi)
        out[name] = mi
    # Cristal: translúcido, muy liso
    glass = simple_pbr("M_Env_Glass", "/Game/Environment/Materials", (0.55, 0.62, 0.65), 0.05, 0.0, translucent_opacity=0.25)
    glass.set_editor_property("phys_material", pms["Glass"])
    lib.save_loaded_asset(glass)
    out["M_Env_Glass"] = glass
    log(f"Materiales de entorno: {len(out)}")
    return out


# ---------------------------------------------------------------------------
# Mallas y tabla de superficies
# ---------------------------------------------------------------------------

def import_mesh(name, dest, material):
    fbx = os.path.join(FBX_DIR, name + ".fbx")
    mesh = None
    for a in import_file(fbx, dest):
        if isinstance(a, unreal.StaticMesh):
            mesh = a
        elif isinstance(a, unreal.MaterialInterface) and a.get_path_name().startswith(dest):
            lib.delete_loaded_asset(a)
    for i in range(mesh.get_num_sections(0)):
        mesh.set_material(i, material)
    ns = mesh.get_editor_property("nanite_settings")
    ns.set_editor_property("enabled", False)
    mesh.set_editor_property("nanite_settings", ns)
    lib.save_loaded_asset(mesh)
    return mesh


def surface_effect(**kw):
    e = unreal.BLSurfaceEffect()
    for k, v in kw.items():
        e.set_editor_property(k, v)
    return e


def make_surface_table(decals, debris):
    lc = unreal.LinearColor
    da = load("/Game/FX/DA_SurfaceEffects")
    if not da:
        f = unreal.DataAssetFactory()
        f.set_editor_property("data_asset_class", unreal.BLSurfaceEffectsData)
        da = tools.create_asset("DA_SurfaceEffects", "/Game/FX", unreal.BLSurfaceEffectsData, f)
    v2 = unreal.Vector2D
    table = {
        "concrete": surface_effect(
            impact_sounds=sounds("Impacts/Concrete"), casing_sounds=sounds("Casings/Concrete"), footstep_sounds=sounds("Footsteps/Concrete"),
            decal=decals["Concrete"], decal_size=5.0,
            dust_count=6, dust_color=lc(0.62, 0.6, 0.56), dust_opacity=0.85, dust_size=v2(12, 85), dust_life=v2(1.0, 2.2), dust_speed=170.0,
            jet_count=6, jet_speed=800.0, debris_mesh=debris["chip"], debris_count=6, debris_speed=450.0, debris_scale=v2(0.6, 1.6)),
        "metal": surface_effect(
            impact_sounds=sounds("Impacts/Metal"), casing_sounds=sounds("Casings/Metal"), footstep_sounds=sounds("Footsteps/Metal"),
            decal=decals["Metal"], decal_size=3.5,
            dust_count=2, dust_color=lc(0.35, 0.34, 0.33), dust_opacity=0.6, dust_size=v2(6, 40), dust_life=v2(0.5, 1.0), dust_speed=100.0,
            spark_count=18, spark_speed=1600.0, jet_count=2, jet_speed=400.0, spark_color=lc(1.0, 0.55, 0.15)),
        "wood": surface_effect(
            impact_sounds=sounds("Impacts/Wood"), casing_sounds=sounds("Casings/Wood"), footstep_sounds=sounds("Footsteps/Wood"),
            decal=decals["Wood"], decal_size=4.0,
            dust_count=3, dust_color=lc(0.55, 0.45, 0.33), dust_opacity=0.7, dust_size=v2(8, 45), dust_life=v2(0.6, 1.3), dust_speed=120.0,
            jet_count=4, jet_speed=600.0, debris_mesh=debris["splinter"], debris_count=7, debris_speed=500.0, debris_scale=v2(0.6, 1.4)),
        "glass": surface_effect(
            impact_sounds=sounds("Impacts/Glass"), casing_sounds=sounds("Casings/Concrete"), footstep_sounds=sounds("Footsteps/Concrete"),
            decal=decals["Glass"], decal_size=9.0,
            dust_count=1, dust_color=lc(0.85, 0.88, 0.9), dust_opacity=0.25, dust_size=v2(5, 25), dust_life=v2(0.3, 0.6), dust_speed=80.0,
            debris_mesh=debris["glass"], debris_count=12, debris_speed=420.0, debris_scale=v2(0.4, 1.3)),
        "dirt": surface_effect(
            impact_sounds=sounds("Impacts/Dirt"), casing_sounds=sounds("Casings/Dirt"), footstep_sounds=sounds("Footsteps/Dirt"),
            decal=decals["Dirt"], decal_size=6.0,
            dust_count=7, dust_color=lc(0.42, 0.33, 0.24), dust_opacity=0.9, dust_size=v2(15, 95), dust_life=v2(1.0, 2.0), dust_speed=220.0,
            jet_count=7, jet_speed=950.0, debris_mesh=debris["clod"], debris_count=10, debris_speed=550.0, debris_scale=v2(0.5, 1.5)),
        # Personajes: neblina de sangre oscura, rápida y corta (sin marca en el cuerpo) + salpicadura en la pared de detrás
        "flesh": surface_effect(
            impact_sounds=sounds("Impacts/Flesh"), casing_sounds=sounds("Casings/Dirt"), footstep_sounds=sounds("Footsteps/Concrete"),
            dust_count=4, dust_color=lc(0.22, 0.015, 0.012), dust_opacity=0.8, dust_size=v2(6, 38), dust_life=v2(0.25, 0.55), dust_speed=140.0,
            jet_count=5, jet_speed=500.0,
            splatter_decal=decals["Blood"], splatter_distance=220.0, splatter_size=60.0),
    }
    for k, v in table.items():
        da.set_editor_property(k, v)
    lib.save_loaded_asset(da)
    log("DA_SurfaceEffects listo")
    return da


def main():
    import_audio()
    tex = import_textures()
    decals = make_fx_materials(tex)
    pms = make_physical_materials()
    master = make_env_master(tex)
    make_env_instances(master, tex, pms)
    mats = {
        "chip": simple_pbr("M_Debris_Concrete", "/Game/FX/Materials", (0.36, 0.35, 0.33), 0.9, 0.0, ism=True),
        "splinter": simple_pbr("M_Debris_Wood", "/Game/FX/Materials", (0.5, 0.36, 0.22), 0.85, 0.0, ism=True),
        "glass": simple_pbr("M_Debris_Glass", "/Game/FX/Materials", (0.75, 0.82, 0.86), 0.05, 0.0, ism=True),
        "clod": simple_pbr("M_Debris_Dirt", "/Game/FX/Materials", (0.2, 0.15, 0.1), 0.95, 0.0, ism=True),
    }
    debris = {
        "chip": import_mesh("SM_ImpactChip", "/Game/FX/Meshes", mats["chip"]),
        "splinter": import_mesh("SM_Debris_Splinter", "/Game/FX/Meshes", mats["splinter"]),
        "glass": import_mesh("SM_Debris_GlassShard", "/Game/FX/Meshes", mats["glass"]),
        "clod": import_mesh("SM_Debris_Clod", "/Game/FX/Meshes", mats["clod"]),
    }
    import_mesh("SM_FX_Quad", "/Game/FX/Meshes", load("/Game/FX/Materials/M_FX_Dust"))
    make_surface_table(decals, debris)
    log("OK")


main()
