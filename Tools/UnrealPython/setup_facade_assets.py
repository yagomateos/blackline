"""Kit de fachadas (Bloque 8): importa ArtSource/Blender/export/Facade (gen_facade_kit.py) y crea sus materiales. Idempotente.

  - Mallas -> /Game/Environment/Facade (Nanite, colisión UCX solo en los muros).
  - MI_Fac_Wall_<Color>: enfoscado (plastered_wall_04) en varios tonos y hormigón; los elige el script del nivel.
  - MI_Fac_Frame / Trim / Metal: triplanares de entorno tintados.
  - M_Fac_Glass: cristal opaco; cada instancia (PerInstanceRandom) tiene "interior" distinto: oscuro, cortina,
    reflejo de cielo, y ~7 % con luz cálida encendida (amanecer).
  - M_Fac_Sign: rótulo pintado y descolorido, color por instancia.
Requiere setup_audio_fx.py (M_Env_Triplanar, texturas, materiales físicos).

Uso: UnrealEditor-Cmd.exe Blackline.uproject -run=pythonscript -script="<ruta>/Tools/UnrealPython/setup_facade_assets.py"
"""
import glob
import os

import unreal

PROJECT = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())
FBX_DIR = os.path.join(PROJECT, "ArtSource", "Blender", "export", "Facade")
DEST = "/Game/Environment/Facade"
MAT_DIR = "/Game/Environment/Materials"
TEX = "/Game/Environment/Textures/"

tools = unreal.AssetToolsHelpers.get_asset_tools()
lib = unreal.EditorAssetLibrary
mel = unreal.MaterialEditingLibrary

WALLS = {
    "Cream": ("plastered_wall_04", 240, (1.0, 0.93, 0.8)),
    "Ochre": ("plastered_wall_04", 240, (0.95, 0.78, 0.55)),
    "Salmon": ("plastered_wall_04", 240, (0.95, 0.74, 0.65)),
    "Grey": ("plastered_wall_04", 240, (0.72, 0.74, 0.76)),
    "Green": ("plastered_wall_04", 240, (0.74, 0.8, 0.7)),
    "Concrete": ("concrete_wall_008", 260, (0.95, 0.95, 0.95)),
}


def log(m):
    unreal.log(f"[BL_Facade] {m}")


def load(path):
    return unreal.load_asset(path) if lib.does_asset_exist(path) else None


def get_or_create(name, folder, cls, factory):
    return load(f"{folder}/{name}") or tools.create_asset(name, folder, cls, factory)


def pm(surface):
    return load(f"/Game/Environment/PhysicalMaterials/PM_{surface}")


def triplanar(name, tex_id, tile, tint, surface, rough=1.0):
    mi = get_or_create(name, MAT_DIR, unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
    mi.set_editor_property("parent", load(f"{MAT_DIR}/M_Env_Triplanar"))
    mel.set_material_instance_texture_parameter_value(mi, "BaseColorMap", load(TEX + f"{tex_id}_diff_2k"))
    mel.set_material_instance_texture_parameter_value(mi, "NormalMap", load(TEX + f"{tex_id}_nor_dx_2k"))
    mel.set_material_instance_texture_parameter_value(mi, "ARMMap", load(TEX + f"{tex_id}_arm_2k"))
    mel.set_material_instance_scalar_parameter_value(mi, "TileSize", tile)
    mel.set_material_instance_scalar_parameter_value(mi, "RoughnessScale", rough)
    mel.set_material_instance_vector_parameter_value(mi, "Tint", unreal.LinearColor(*tint, 1))
    mi.set_editor_property("phys_material", pm(surface))
    lib.save_loaded_asset(mi)
    return mi


def ensure_usage(mat):
    """El maestro triplanar se usa ahora con instancias (ISM) y Nanite."""
    changed = False
    for flag in ("used_with_instanced_static_meshes", "used_with_nanite"):
        try:
            if not mat.get_editor_property(flag):
                mat.set_editor_property(flag, True)
                changed = True
        except Exception as e:  # la propiedad puede no existir en esta versión
            log(f"AVISO {flag}: {e}")
    if changed:
        mel.recompile_material(mat)
        lib.save_loaded_asset(mat)


# Materiales que se regeneran aunque existan (al cambiar su grafo)
REBUILD = {"M_Fac_Glass"}


def new_material(name):
    path = f"{MAT_DIR}/{name}"
    if lib.does_asset_exist(path) and name in REBUILD:
        lib.delete_asset(path)
    if lib.does_asset_exist(path):
        return None
    m = tools.create_asset(name, MAT_DIR, unreal.Material, unreal.MaterialFactoryNew())
    m.set_editor_property("used_with_instanced_static_meshes", True)
    return m


def custom(m, x, y, code, inputs, out=unreal.CustomMaterialOutputType.CMOT_FLOAT3):
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


def glass_material():
    m = new_material("M_Fac_Glass")
    if m:
        rnd = mel.create_material_expression(m, unreal.MaterialExpressionPerInstanceRandom, -900, 0)
        wp = mel.create_material_expression(m, unreal.MaterialExpressionWorldPosition, -900, 150)
        # Interior falso por instancia: oscuro / cortina clara / persiana interior; ~7 % con luz encendida
        bc = custom(m, -500, 0,
                    "float r = frac(R * 7.13);"
                    "float3 dark = float3(0.012, 0.014, 0.016);"
                    "float3 curtain = lerp(float3(0.30, 0.28, 0.24), float3(0.22, 0.24, 0.28), frac(R * 31.7));"
                    "float folds = 0.75 + 0.25 * sin(WP.x * 0.35 + WP.y * 0.35);"
                    "return r < 0.35 ? curtain * folds * 0.35 : dark * (0.7 + 0.6 * frac(R * 13.1));",
                    ["R", "WP"])
        em = custom(m, -500, 250,
                    "float r = frac(R * 3.71);"
                    "return r > 0.93 ? float3(1.0, 0.62, 0.32) * (0.25 + 0.6 * frac(R * 17.3)) : float3(0, 0, 0);",
                    ["R"])
        ro = custom(m, -500, 450, "return 0.03 + 0.12 * frac(R * 5.3);", ["R"], unreal.CustomMaterialOutputType.CMOT_FLOAT1)
        for c in (bc, em, ro):
            mel.connect_material_expressions(rnd, "", c, "R")
        mel.connect_material_expressions(wp, "", bc, "WP")
        mel.connect_material_property(bc, "", unreal.MaterialProperty.MP_BASE_COLOR)
        mel.connect_material_property(em, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
        mel.connect_material_property(ro, "", unreal.MaterialProperty.MP_ROUGHNESS)
        spec = mel.create_material_expression(m, unreal.MaterialExpressionConstant, -300, 550)
        spec.set_editor_property("r", 1.0)
        mel.connect_material_property(spec, "", unreal.MaterialProperty.MP_SPECULAR)
        m.set_editor_property("phys_material", pm("Glass"))
        mel.recompile_material(m)
        lib.save_loaded_asset(m)
    return load(f"{MAT_DIR}/M_Fac_Glass")


def sign_material():
    m = new_material("M_Fac_Sign")
    if m:
        rnd = mel.create_material_expression(m, unreal.MaterialExpressionPerInstanceRandom, -900, 0)
        wp = mel.create_material_expression(m, unreal.MaterialExpressionWorldPosition, -900, 150)
        bc = custom(m, -500, 0,
                    "float3 pal[6] = { float3(0.45,0.08,0.06), float3(0.07,0.16,0.35), float3(0.12,0.3,0.14),"
                    " float3(0.6,0.45,0.08), float3(0.5,0.5,0.48), float3(0.08,0.08,0.08) };"
                    "float3 c = pal[(int)floor(frac(R * 9.7) * 5.99)];"
                    "float n = frac(sin(dot(floor(WP.xz / 7.0), float2(12.9898, 78.233))) * 43758.5453);"
                    "float fade = 0.75 + 0.25 * n;"                        # pintura descolorida a manchas
                    "float streak = saturate(0.6 + 0.4 * sin(WP.x * 0.9 + WP.y * 0.9));"
                    "return lerp(c, float3(0.4,0.39,0.36), 0.25) * fade * streak;",
                    ["R", "WP"])
        mel.connect_material_expressions(rnd, "", bc, "R")
        mel.connect_material_expressions(wp, "", bc, "WP")
        mel.connect_material_property(bc, "", unreal.MaterialProperty.MP_BASE_COLOR)
        ro = mel.create_material_expression(m, unreal.MaterialExpressionConstant, -300, 300)
        ro.set_editor_property("r", 0.7)
        mel.connect_material_property(ro, "", unreal.MaterialProperty.MP_ROUGHNESS)
        m.set_editor_property("phys_material", pm("Metal"))
        mel.recompile_material(m)
        lib.save_loaded_asset(m)
    return load(f"{MAT_DIR}/M_Fac_Sign")


def make_materials():
    ensure_usage(load(f"{MAT_DIR}/M_Env_Triplanar"))
    walls = {k: triplanar(f"MI_Fac_Wall_{k}", t, tile, tint, "Concrete") for k, (t, tile, tint) in WALLS.items()}
    mats = {
        "MI_Fac_Wall": walls["Cream"],
        "MI_Fac_Frame": triplanar("MI_Fac_Frame", "plastered_wall_04", 120, (1.05, 1.05, 1.02), "Metal", 0.55),
        "MI_Fac_Trim": triplanar("MI_Fac_Trim", "concrete_wall_008", 180, (1.08, 1.05, 1.0), "Concrete"),
        "MI_Fac_Metal": triplanar("MI_Fac_Metal", "metal_plate_02", 140, (0.62, 0.64, 0.66), "Metal", 1.0),
        "MI_Fac_Glass": glass_material(),
        "MI_Fac_Sign": sign_material(),
    }
    return mats


def import_piece(fbx, mats):
    task = unreal.AssetImportTask()
    task.filename = fbx
    task.destination_path = DEST
    task.automated = True
    task.replace_existing = True
    task.save = True
    tools.import_asset_tasks([task])
    mesh, autos = None, []
    for p in task.imported_object_paths:
        a = unreal.load_asset(p)
        if isinstance(a, unreal.StaticMesh):
            mesh = a
        elif isinstance(a, unreal.MaterialInterface) and a.get_path_name().startswith(DEST):
            autos.append(a)
    if not mesh:
        raise RuntimeError(f"No se importó {fbx}")
    new = []
    for m in mesh.get_editor_property("static_materials"):
        slot = str(m.get_editor_property("material_slot_name"))
        new.append(unreal.StaticMaterial(material_interface=mats[slot], material_slot_name=slot, uv_channel_data=m.get_editor_property("uv_channel_data")))
    mesh.set_editor_property("static_materials", new)
    ns = mesh.get_editor_property("nanite_settings")
    ns.set_editor_property("enabled", True)
    mesh.set_editor_property("nanite_settings", ns)
    lib.save_loaded_asset(mesh)
    for a in autos:
        lib.delete_loaded_asset(a)
    b = mesh.get_bounding_box()
    log(f"{mesh.get_name()}: {b.max - b.min} cm, {len(new)} materiales")
    return mesh


def main():
    mats = make_materials()
    n = 0
    for fbx in sorted(glob.glob(os.path.join(FBX_DIR, "*.fbx"))):
        import_piece(fbx, mats)
        n += 1
    log(f"OK ({n} piezas)")


main()
