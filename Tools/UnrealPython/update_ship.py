"""Misión 3 (2026-10-10): el "barco sin bandera" deja de parecer de cartón.

- M_Ship_UV: material PBR por UV (las mallas del kit tienen UV de 1 m por unidad) con texturas reales de Poly Haven:
  chapa pintada (metal_plate_02) y óxido (rusty_metal_02). El barco se mueve al zarpar: con el triplanar de mundo de
  los materiales de entorno la textura "nadaba" sobre el casco.
- Instancias: casco gris acero, superestructura blanca, cubierta rojo óxido, obra viva roja, negro y óxido.
- Reimporta SM_Ship_Cargo (casco con forma, tracas, flotación, barandillas, ojos de buey, anclas...: gen_m03.py).
Uso: UnrealEditor-Cmd.exe Blackline.uproject -run=pythonscript -script=".../update_ship.py"
Después: fix_material_usage.py (usos de Nanite/instancias).
"""
import os

import unreal

HERE = os.path.dirname(os.path.abspath(__file__))
src = open(os.path.join(HERE, "setup_audio_fx.py"), encoding="utf-8").read()
src = src[:src.rindex("\nmain()")]
ns = {"__file__": os.path.join(HERE, "setup_audio_fx.py"), "__name__": "bl_setup_audio_fx"}
exec(compile(src, "setup_audio_fx.py", "exec"), ns)
new_material, expr, finish = ns["new_material"], ns["expr"], ns["finish"]

lib = unreal.EditorAssetLibrary
mel = unreal.MaterialEditingLibrary
tools = unreal.AssetToolsHelpers.get_asset_tools()
ENV = "/Game/Environment/Materials"
TOWN = "/Game/Environment/OldTown"
TEX = "/Game/Environment/Textures"
PROJECT = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())
FBX = os.path.join(PROJECT, "ArtSource", "Blender", "export", "OldTown", "SM_Ship_Cargo.fbx")


def tex(name):
    return unreal.load_asset(f"{TEX}/{name}")


def make_master():
    m = new_material("M_Ship_UV", ENV)
    if m:
        uv = expr(m, unreal.MaterialExpressionTextureCoordinate, -1200, 0)
        scale = expr(m, unreal.MaterialExpressionScalarParameter, -1200, 120, parameter_name="UVScale", default_value=0.5)
        uvs = expr(m, unreal.MaterialExpressionMultiply, -1000, 40)
        mel.connect_material_expressions(uv, "", uvs, "A")
        mel.connect_material_expressions(scale, "", uvs, "B")
        samples = {}
        for i, (pname, t, st) in enumerate((("BaseColorMap", tex("metal_plate_02_diff_2k"), unreal.MaterialSamplerType.SAMPLERTYPE_COLOR),
                                            ("NormalMap", tex("metal_plate_02_nor_dx_2k"), unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL),
                                            ("ARMMap", tex("metal_plate_02_arm_2k"), unreal.MaterialSamplerType.SAMPLERTYPE_MASKS))):
            s = expr(m, unreal.MaterialExpressionTextureSampleParameter2D, -800, i * 260, parameter_name=pname, texture=t, sampler_type=st)
            mel.connect_material_expressions(uvs, "", s, "UVs")
            samples[pname] = s
        tint = expr(m, unreal.MaterialExpressionVectorParameter, -800, -220, parameter_name="Tint", default_value=unreal.LinearColor(1, 1, 1, 1))
        # Pintura: el color de la chapa se desatura y se multiplica por el tinte (el detalle y la suciedad quedan)
        desat = expr(m, unreal.MaterialExpressionDesaturation, -560, -40)
        mel.connect_material_expressions(samples["BaseColorMap"], "RGB", desat, "")
        paint = expr(m, unreal.MaterialExpressionScalarParameter, -800, -320, parameter_name="PaintAmount", default_value=0.8)
        mel.connect_material_expressions(paint, "", desat, "Fraction")
        bc = expr(m, unreal.MaterialExpressionMultiply, -360, -60)
        mel.connect_material_expressions(desat, "", bc, "A")
        mel.connect_material_expressions(tint, "", bc, "B")
        mel.connect_material_property(bc, "", unreal.MaterialProperty.MP_BASE_COLOR)
        mel.connect_material_property(samples["NormalMap"], "RGB", unreal.MaterialProperty.MP_NORMAL)
        rs = expr(m, unreal.MaterialExpressionScalarParameter, -560, 600, parameter_name="RoughnessScale", default_value=1.0)
        ms = expr(m, unreal.MaterialExpressionScalarParameter, -560, 700, parameter_name="MetallicScale", default_value=1.0)
        r = expr(m, unreal.MaterialExpressionMultiply, -360, 560)
        mel.connect_material_expressions(samples["ARMMap"], "G", r, "A")
        mel.connect_material_expressions(rs, "", r, "B")
        mt = expr(m, unreal.MaterialExpressionMultiply, -360, 660)
        mel.connect_material_expressions(samples["ARMMap"], "B", mt, "A")
        mel.connect_material_expressions(ms, "", mt, "B")
        mel.connect_material_property(r, "", unreal.MaterialProperty.MP_ROUGHNESS)
        mel.connect_material_property(mt, "", unreal.MaterialProperty.MP_METALLIC)
        mel.connect_material_property(samples["ARMMap"], "R", unreal.MaterialProperty.MP_AMBIENT_OCCLUSION)
        m.set_editor_property("used_with_nanite", True)
        m.set_editor_property("used_with_instanced_static_meshes", True)
        finish(m)
    return unreal.load_asset(f"{ENV}/M_Ship_UV")


def mi(name, folder, master, tint, uv=0.5, paint=0.8, rough=1.0, metal=0.6, base="metal_plate_02"):
    path = f"{folder}/{name}"
    inst = unreal.load_asset(path) if lib.does_asset_exist(path) else tools.create_asset(name, folder, unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
    inst.set_editor_property("parent", master)
    mel.set_material_instance_texture_parameter_value(inst, "BaseColorMap", tex(f"{base}_diff_2k"))
    mel.set_material_instance_texture_parameter_value(inst, "NormalMap", tex(f"{base}_nor_dx_2k"))
    mel.set_material_instance_texture_parameter_value(inst, "ARMMap", tex(f"{base}_arm_2k"))
    mel.set_material_instance_vector_parameter_value(inst, "Tint", unreal.LinearColor(*tint, 1))
    mel.set_material_instance_scalar_parameter_value(inst, "UVScale", uv)
    mel.set_material_instance_scalar_parameter_value(inst, "PaintAmount", paint)
    mel.set_material_instance_scalar_parameter_value(inst, "RoughnessScale", rough)
    mel.set_material_instance_scalar_parameter_value(inst, "MetallicScale", metal)
    pm = unreal.load_asset("/Game/Environment/PhysicalMaterials/PM_Metal")
    if pm:
        inst.set_editor_property("phys_material", pm)
    lib.save_loaded_asset(inst)
    return inst


master = make_master()
mats = {
    "MI_Ship_Grey": mi("MI_Ship_Grey", TOWN, master, (0.42, 0.45, 0.48), uv=0.35, paint=0.85, rough=1.05, metal=0.45),
    "MI_Ship_Deck": mi("MI_Ship_Deck", TOWN, master, (0.42, 0.2, 0.14), uv=0.5, paint=0.75, rough=1.15, metal=0.3),
    "MI_Boat_HullWhite": mi("MI_Boat_HullWhite", TOWN, master, (0.85, 0.85, 0.82), uv=0.5, paint=0.9, rough=0.95, metal=0.25),
    "MI_Boat_HullRed": mi("MI_Boat_HullRed", TOWN, master, (0.55, 0.09, 0.06), uv=0.35, paint=0.85, rough=1.1, metal=0.2),
    "MI_Ship_Black": mi("MI_Ship_Black", TOWN, master, (0.07, 0.07, 0.07), uv=0.5, paint=0.95, rough=0.9, metal=0.4),
    "MI_Env_RustyMetal": mi("MI_Ship_Rust", TOWN, master, (1.0, 1.0, 1.0), uv=0.5, paint=0.0, rough=1.0, metal=1.0, base="rusty_metal_02"),
    "MI_Veh_GlassOpaque": unreal.load_asset(f"{ENV}/MI_Veh_GlassOpaque"),
}

task = unreal.AssetImportTask()
task.filename = FBX
task.destination_path = TOWN
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
            m = mats.get(slot) or sm.get_editor_property("material_interface")
            if not mats.get(slot):
                unreal.log_warning(f"[BL_Ship] ranura {slot}: se deja {m.get_name() if m else 'ninguno'}")
            new.append(unreal.StaticMaterial(material_interface=m, material_slot_name=slot, uv_channel_data=sm.get_editor_property("uv_channel_data")))
        a.set_editor_property("static_materials", new)
        lib.save_loaded_asset(a)
        unreal.log_warning(f"[BL_Ship] {a.get_name()}: {len(new)} materiales")
    elif isinstance(a, unreal.MaterialInterface) and a.get_path_name().startswith(TOWN) and a.get_name() not in [m.get_name() for m in mats.values() if m]:
        lib.delete_loaded_asset(a)
unreal.log_warning("[BL_Ship] OK")
