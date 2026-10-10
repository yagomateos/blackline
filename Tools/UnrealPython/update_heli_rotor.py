"""Helicóptero (2026-10-10): rotores realistas.

- Reimporta SM_Heli_Rotor / SM_Heli_TailRotor (palas con perfil, torsión, punta en flecha; cabeza con horquillas,
  bielas y plato oscilante: gen_heli.py) y los discos de desenfoque SM_Heli_RotorDisc / SM_Heli_TailRotorDisc.
- M_Rotor_Blur: translúcido a dos caras; UV radial (U = radio, V = ángulo). Opacidad: estelas detrás de cada pala
  (Blades sectores), anillo de las puntas, anillos concéntricos tenues y raíz transparente. ABLHelicopter le da la
  opacidad según las vueltas del rotor (a régimen de vuelo se ve el disco; parado o arrancando, las palas).
Uso: UnrealEditor-Cmd.exe Blackline.uproject -run=pythonscript -script=".../update_heli_rotor.py"; luego fix_material_usage.py.
"""
import os

import unreal

HERE = os.path.dirname(os.path.abspath(__file__))
src = open(os.path.join(HERE, "setup_audio_fx.py"), encoding="utf-8").read()
src = src[:src.rindex("\nmain()")]
ns = {"__file__": os.path.join(HERE, "setup_audio_fx.py"), "__name__": "bl_setup_audio_fx"}
exec(compile(src, "setup_audio_fx.py", "exec"), ns)
new_material, expr, custom, finish = ns["new_material"], ns["expr"], ns["custom"], ns["finish"]

lib = unreal.EditorAssetLibrary
mel = unreal.MaterialEditingLibrary
tools = unreal.AssetToolsHelpers.get_asset_tools()
ENV = "/Game/Environment/Materials"
HELI = "/Game/Vehicles/Heli"
PROJECT = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())
SRC = os.path.join(PROJECT, "ArtSource", "Blender", "export", "Vehicles")

BLUR_CODE = """
float r = UV.x;
float a = UV.y;
float sweep = frac(a * Blades - Phase);
// Estela detrás de cada pala: entra suave (sin borde duro) y se desvanece a lo largo del giro
float ghost = smoothstep(0.0, 0.08, sweep) * exp(-sweep * 3.2) + 0.12;
float tip = smoothstep(0.88, 0.965, r) * (1.0 - smoothstep(0.975, 1.0, r));
float rings = 0.97 + 0.03 * sin(r * 260.0);
float root = smoothstep(0.03, 0.25, r);
float edge = 1.0 - smoothstep(0.95, 1.0, r);
return saturate(Opacity * (root * edge * rings * ghost + 0.55 * tip));
"""


def make_blur():
    m = new_material("M_Rotor_Blur_v2", ENV)
    if m:
        m.set_editor_property("blend_mode", unreal.BlendMode.BLEND_TRANSLUCENT)
        m.set_editor_property("two_sided", True)
        m.set_editor_property("translucency_lighting_mode", unreal.TranslucencyLightingMode.TLM_SURFACE)
        uv = expr(m, unreal.MaterialExpressionTextureCoordinate, -900, 200)
        color = expr(m, unreal.MaterialExpressionVectorParameter, -500, -100, parameter_name="Color", default_value=unreal.LinearColor(0.015, 0.016, 0.015, 1))
        op = expr(m, unreal.MaterialExpressionScalarParameter, -900, 320, parameter_name="Opacity", default_value=0.5)
        blades = expr(m, unreal.MaterialExpressionScalarParameter, -900, 420, parameter_name="Blades", default_value=4.0)
        phase = expr(m, unreal.MaterialExpressionScalarParameter, -900, 520, parameter_name="Phase", default_value=0.0)
        c = custom(m, -550, 250, BLUR_CODE, ["UV", "Opacity", "Blades", "Phase"])
        for e, n in ((uv, "UV"), (op, "Opacity"), (blades, "Blades"), (phase, "Phase")):
            mel.connect_material_expressions(e, "", c, n)
        mel.connect_material_property(color, "", unreal.MaterialProperty.MP_BASE_COLOR)
        mel.connect_material_property(c, "", unreal.MaterialProperty.MP_OPACITY)
        rough = expr(m, unreal.MaterialExpressionConstant, -500, 50, r=0.6)
        mel.connect_material_property(rough, "", unreal.MaterialProperty.MP_ROUGHNESS)
        finish(m)
    return unreal.load_asset(f"{ENV}/M_Rotor_Blur_v2")


def mi(name, parent, scalars=None, vectors=None):
    path = f"{HELI}/{name}"
    inst = unreal.load_asset(path) if lib.does_asset_exist(path) else tools.create_asset(name, HELI, unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
    inst.set_editor_property("parent", parent)
    for k, v in (scalars or {}).items():
        mel.set_material_instance_scalar_parameter_value(inst, k, v)
    for k, v in (vectors or {}).items():
        mel.set_material_instance_vector_parameter_value(inst, k, unreal.LinearColor(*v, 1))
    lib.save_loaded_asset(inst)
    return inst


blur = make_blur()
simple = unreal.load_asset(f"{ENV}/M_Prop_Simple")
mats = {
    "MI_Veh_Trim": unreal.load_asset(f"{ENV}/MI_Veh_Trim"),
    "MI_Heli_RotorMetal": mi("MI_Heli_RotorMetal", simple, {"Roughness": 0.35, "Metallic": 0.9}, {"Color": (0.12, 0.12, 0.12)}),
    "MI_Heli_BladeTip": mi("MI_Heli_BladeTip", simple, {"Roughness": 0.5, "Metallic": 0.0}, {"Color": (0.42, 0.36, 0.06)}),
    # Fuselaje: interior gris verdoso (antes casi negro: con las puertas abiertas parecía cerrado)
    "MI_Heli_Paint": unreal.load_asset(f"{HELI}/MI_Heli_Paint"),
    "MI_Veh_GlassOpaque": unreal.load_asset(f"{ENV}/MI_Veh_GlassOpaque"),
    "MI_Heli_Interior": mi("MI_Heli_Interior", simple, {"Roughness": 0.85, "Metallic": 0.0}, {"Color": (0.075, 0.082, 0.068)}),
}
disc_mats = {
    "SM_Heli_RotorDisc": mi("MI_Rotor_Blur", blur, {"Opacity": 0.42, "Blades": 4.0}),
    "SM_Heli_TailRotorDisc": mi("MI_TailRotor_Blur", blur, {"Opacity": 0.45, "Blades": 2.0}),
}
# Versión anterior del material (anillos y cuñas demasiado marcados): ya sin referencias
if lib.does_asset_exist(f"{ENV}/M_Rotor_Blur"):
    lib.delete_asset(f"{ENV}/M_Rotor_Blur")
keep = {m.get_name() for m in list(mats.values()) + list(disc_mats.values()) if m}

for name in ("SM_Heli_Body", "SM_Heli_Rotor", "SM_Heli_TailRotor", "SM_Heli_RotorDisc", "SM_Heli_TailRotorDisc"):
    task = unreal.AssetImportTask()
    task.filename = os.path.join(SRC, name + ".fbx")
    task.destination_path = HELI
    task.automated = True
    task.replace_existing = True
    task.save = True
    tools.import_asset_tasks([task])
    for p in task.imported_object_paths:
        a = unreal.load_asset(p)
        if isinstance(a, unreal.StaticMesh):
            is_disc = name.endswith("Disc")
            new = []
            for sm in a.get_editor_property("static_materials"):
                slot = str(sm.get_editor_property("material_slot_name"))
                m = disc_mats[name] if is_disc else (mats.get(slot) or mats["MI_Veh_Trim"])
                new.append(unreal.StaticMaterial(material_interface=m, material_slot_name=slot, uv_channel_data=sm.get_editor_property("uv_channel_data")))
            a.set_editor_property("static_materials", new)
            nset = a.get_editor_property("nanite_settings")
            nset.set_editor_property("enabled", not is_disc)     # translúcido: sin Nanite
            a.set_editor_property("nanite_settings", nset)
            lib.save_loaded_asset(a)
            unreal.log_warning(f"[BL_Rotor] {a.get_name()}: {[m.get_editor_property('material_interface').get_name() for m in new]}")
        elif isinstance(a, unreal.MaterialInterface) and a.get_path_name().startswith(HELI) and a.get_name() not in keep:
            lib.delete_loaded_asset(a)
unreal.log_warning("[BL_Rotor] OK")
