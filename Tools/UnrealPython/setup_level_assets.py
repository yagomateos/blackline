"""Importa los props del nivel (Bloque 4) y crea sus materiales. Idempotente.

  - FBX de ArtSource/Blender/export/Props (gen_level_props.py) -> /Game/Environment/Props, Nanite, colisión UCX.
  - Materiales: triplanares de entorno (M_Env_Triplanar, texturas Poly Haven) con tinte por variante
    (contenedores de 4 colores, sacos, coche calcinado) y un maestro simple (pintura, cristal opaco, goma...).
  Requiere setup_audio_fx.py (texturas, M_Env_Triplanar, materiales físicos).

Uso:
  UnrealEditor-Cmd.exe Blackline.uproject -run=pythonscript -script="<ruta>/Tools/UnrealPython/setup_level_assets.py"
"""
import glob
import os

import unreal

PROJECT = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())
FBX_DIR = os.path.join(PROJECT, "ArtSource", "Blender", "export", "Props")
DEST = "/Game/Environment/Props"
MAT_DIR = "/Game/Environment/Materials"
TEX = "/Game/Environment/Textures/"

tools = unreal.AssetToolsHelpers.get_asset_tools()
lib = unreal.EditorAssetLibrary
mel = unreal.MaterialEditingLibrary


def log(m):
    unreal.log(f"[BL_Level] {m}")


def load(path):
    return unreal.load_asset(path) if lib.does_asset_exist(path) else None


def get_or_create(name, folder, cls, factory):
    return load(f"{folder}/{name}") or tools.create_asset(name, folder, cls, factory)


def pm(surface):
    return load(f"/Game/Environment/PhysicalMaterials/PM_{surface}")


# ---------------------------------------------------------------------------
# Materiales
# ---------------------------------------------------------------------------

def triplanar(name, tex_id, tile, tint, surface, rough=1.0):
    """Instancia del maestro triplanar de entorno (no depende de las UV)."""
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


def simple_master():
    """Maestro PBR de color plano con parámetros (pintura, cristal opaco, goma, plástico, acero)."""
    path = f"{MAT_DIR}/M_Prop_Simple"
    if lib.does_asset_exist(path):
        return load(path)
    m = tools.create_asset("M_Prop_Simple", MAT_DIR, unreal.Material, unreal.MaterialFactoryNew())

    def expr(cls, x, y, **props):
        e = mel.create_material_expression(m, cls, x, y)
        for k, v in props.items():
            e.set_editor_property(k, v)
        return e

    c = expr(unreal.MaterialExpressionVectorParameter, -400, 0, parameter_name="Color", default_value=unreal.LinearColor(0.5, 0.5, 0.5, 1))
    r = expr(unreal.MaterialExpressionScalarParameter, -400, 200, parameter_name="Roughness", default_value=0.5)
    mt = expr(unreal.MaterialExpressionScalarParameter, -400, 300, parameter_name="Metallic", default_value=0.0)
    mel.connect_material_property(c, "", unreal.MaterialProperty.MP_BASE_COLOR)
    mel.connect_material_property(r, "", unreal.MaterialProperty.MP_ROUGHNESS)
    mel.connect_material_property(mt, "", unreal.MaterialProperty.MP_METALLIC)
    mel.recompile_material(m)
    lib.save_loaded_asset(m)
    return m


def emissive_master():
    """Maestro con color + emisión (pantallas, LEDs)."""
    path = f"{MAT_DIR}/M_Prop_Emissive"
    if lib.does_asset_exist(path):
        return load(path)
    m = tools.create_asset("M_Prop_Emissive", MAT_DIR, unreal.Material, unreal.MaterialFactoryNew())

    def expr(cls, x, y, **props):
        e = mel.create_material_expression(m, cls, x, y)
        for k, v in props.items():
            e.set_editor_property(k, v)
        return e

    c = expr(unreal.MaterialExpressionVectorParameter, -400, 0, parameter_name="Color", default_value=unreal.LinearColor(0.02, 0.02, 0.02, 1))
    e = expr(unreal.MaterialExpressionVectorParameter, -400, 150, parameter_name="Emissive", default_value=unreal.LinearColor(0, 1, 0, 1))
    r = expr(unreal.MaterialExpressionScalarParameter, -400, 300, parameter_name="Roughness", default_value=0.2)
    mel.connect_material_property(c, "", unreal.MaterialProperty.MP_BASE_COLOR)
    mel.connect_material_property(e, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    mel.connect_material_property(r, "", unreal.MaterialProperty.MP_ROUGHNESS)
    mel.recompile_material(m)
    lib.save_loaded_asset(m)
    return m


def emissive(name, color, emissive_color, rough):
    mi = get_or_create(name, MAT_DIR, unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
    mi.set_editor_property("parent", emissive_master())
    mel.set_material_instance_vector_parameter_value(mi, "Color", unreal.LinearColor(*color, 1))
    mel.set_material_instance_vector_parameter_value(mi, "Emissive", unreal.LinearColor(*emissive_color, 1))
    mel.set_material_instance_scalar_parameter_value(mi, "Roughness", rough)
    lib.save_loaded_asset(mi)
    return mi


def simple(name, color, rough, metal, surface):
    mi = get_or_create(name, MAT_DIR, unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
    mi.set_editor_property("parent", simple_master())
    mel.set_material_instance_vector_parameter_value(mi, "Color", unreal.LinearColor(*color, 1))
    mel.set_material_instance_scalar_parameter_value(mi, "Roughness", rough)
    mel.set_material_instance_scalar_parameter_value(mi, "Metallic", metal)
    mi.set_editor_property("phys_material", pm(surface))
    lib.save_loaded_asset(mi)
    return mi


def make_materials():
    mats = {
        "MI_Prop_Concrete": load(f"{MAT_DIR}/MI_Env_ConcreteWall"),
        "MI_Prop_Sandbag": triplanar("MI_Env_Sandbag", "dirt", 90, (1.05, 0.95, 0.72), "Dirt", 1.0),
        "MI_Prop_ContainerSteel": triplanar("MI_Env_Container_Blue", "rusty_metal_02", 250, (0.32, 0.45, 0.62), "Metal", 0.9),
        "MI_Prop_Plywood": load(f"{MAT_DIR}/MI_Env_WoodPlanks"),
        "MI_Veh_Paint": simple("MI_Veh_Paint_White", (0.62, 0.62, 0.6), 0.32, 0.0, "Metal"),
        "MI_Veh_Glass": simple("MI_Veh_GlassOpaque", (0.012, 0.015, 0.018), 0.04, 0.0, "Glass"),
        "MI_Veh_Rubber": simple("MI_Veh_Rubber", (0.02, 0.02, 0.02), 0.85, 0.0, "Dirt"),
        "MI_Veh_Trim": simple("MI_Veh_Trim", (0.03, 0.03, 0.03), 0.55, 0.0, "Metal"),
        "MI_Veh_Steel": simple("MI_Veh_Steel", (0.55, 0.55, 0.55), 0.35, 1.0, "Metal"),
        "MI_Veh_Burnt": triplanar("MI_Env_BurntMetal", "rusty_metal_02", 180, (0.32, 0.26, 0.22), "Metal", 1.0),
        # Objetivo (Bloque 6)
        "MI_Obj_Plastic": simple("MI_Obj_Plastic", (0.035, 0.036, 0.034), 0.55, 0.0, "Metal"),
        "MI_Obj_Aluminium": simple("MI_Obj_Aluminium", (0.5, 0.51, 0.53), 0.32, 1.0, "Metal"),
        "MI_Obj_Screen": emissive("MI_Obj_Screen", (0.01, 0.015, 0.02), (0.05, 0.12, 0.16), 0.05),
        "MI_Obj_LED": emissive("MI_Obj_LED", (0.05, 0.3, 0.1), (0.4, 6.0, 1.2), 0.3),
        # Atrezo de calle (Bloque 8)
        "MI_Prop_DumpsterGreen": triplanar("MI_Env_Container_Green", "rusty_metal_02", 250, (0.3, 0.42, 0.3), "Metal", 0.9),
        "MI_Prop_DrumRed": triplanar("MI_Env_Container_Red", "rusty_metal_02", 250, (0.62, 0.25, 0.2), "Metal", 0.9),
        "MI_Prop_Brick": triplanar("MI_Env_Brick", "concrete_wall_008", 70, (0.78, 0.45, 0.32), "Concrete", 1.0),
    }
    # Variantes de color de contenedor (las asigna el script del nivel por instancia)
    for name, tint in (("Red", (0.62, 0.25, 0.2)), ("Green", (0.3, 0.42, 0.3)), ("Orange", (0.75, 0.45, 0.22))):
        triplanar(f"MI_Env_Container_{name}", "rusty_metal_02", 250, tint, "Metal", 0.9)
    return mats


# ---------------------------------------------------------------------------
# Mallas
# ---------------------------------------------------------------------------

def import_prop(fbx, mats):
    task = unreal.AssetImportTask()
    task.filename = fbx
    task.destination_path = DEST
    task.automated = True
    task.replace_existing = True
    task.save = True
    tools.import_asset_tasks([task])
    mesh = None
    autos = []
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
        mi = mats.get(slot)
        if not mi:
            raise RuntimeError(f"{mesh.get_name()}: slot sin material {slot}")
        new.append(unreal.StaticMaterial(material_interface=mi, material_slot_name=slot, uv_channel_data=m.get_editor_property("uv_channel_data")))
    mesh.set_editor_property("static_materials", new)
    ns = mesh.get_editor_property("nanite_settings")
    ns.set_editor_property("enabled", True)
    mesh.set_editor_property("nanite_settings", ns)
    lib.save_loaded_asset(mesh)
    for a in autos:
        lib.delete_loaded_asset(a)
    # Colisión: las UCX del FBX (cajas/convexos). Si no llegaron, caja automática.
    body = mesh.get_editor_property("body_setup")
    agg = body.get_editor_property("agg_geom") if body else None
    n = len(agg.get_editor_property("convex_elems")) + len(agg.get_editor_property("box_elems")) if agg else 0
    if n == 0:
        unreal.EditorStaticMeshLibrary.add_simple_collisions(mesh, unreal.ScriptCollisionShapeType.BOX)
        lib.save_loaded_asset(mesh)
        log(f"AVISO {mesh.get_name()}: sin UCX, caja automática")
    b = mesh.get_bounding_box()
    log(f"{mesh.get_name()}: {b.max - b.min} cm, colisiones {n}")
    return mesh


def main():
    mats = make_materials()
    for fbx in sorted(glob.glob(os.path.join(FBX_DIR, "*.fbx"))):
        import_prop(fbx, mats)
    log("OK")


main()
