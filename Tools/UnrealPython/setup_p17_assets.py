"""Pistola P-17 (arma secundaria): importa malla, cargador, vaina 9 mm, texturas, sonidos y crea DA_P17. Idempotente.

Requisitos: FBX de ArtSource/Blender/scripts/gen_p17.py (--bake), WAV de Tools/audio/gen_p17_sfx.py, y los assets
compartidos de las armas (setup_weapon_assets.py: M_Weapon_Master, fogonazo, latón; setup_audio_fx.py: atenuaciones).

  UnrealEditor-Cmd.exe Blackline.uproject -run=pythonscript -script="Tools/UnrealPython/setup_p17_assets.py"
"""
import glob
import json
import os
import unreal

PROJECT = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())
FBX_DIR = os.path.join(PROJECT, "ArtSource", "Blender", "export")
TEX_DIR = os.path.join(PROJECT, "ArtSource", "Textures", "Weapons")
WAV_DIR = os.path.join(PROJECT, "ArtSource", "Audio", "SFX", "Weapons", "P17")
P17 = "/Game/Weapons/P17"
AUDIO = "/Game/Audio/Weapons/P17"

tools = unreal.AssetToolsHelpers.get_asset_tools()
lib = unreal.EditorAssetLibrary
mel = unreal.MaterialEditingLibrary


def log(m):
    unreal.log_warning(f"[BL_P17] {m}")


def load(path):
    return unreal.load_asset(path) if lib.does_asset_exist(path) else None


def must(path):
    a = load(path)
    if not a:
        raise RuntimeError(f"Falta {path}")
    return a


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
# Sonido
# ---------------------------------------------------------------------------

def import_audio():
    att_fire = must("/Game/Audio/Settings/SA_WeaponFire")
    att_far = must("/Game/Audio/Settings/SA_WeaponFar")
    att_casing = must("/Game/Audio/Settings/SA_Casing")
    con_fire = must("/Game/Audio/Settings/SCon_WeaponFire")
    # (prefijo, atenuación, concurrencia, volumen): sin atenuación = 2D para el jugador
    rules = [("SW_P17_Fire_Close3D_", att_fire, con_fire, 1.0), ("SW_P17_Fire_Close_", None, con_fire, 0.95),
             ("SW_P17_Fire_Distant_", att_far, con_fire, 0.9), ("SW_P17_Tail_", None, None, 0.7),
             ("SW_P17_MagDrop_", att_casing, None, 0.7), ("SW_P17_", None, None, 0.8)]
    out = {}
    for wav in sorted(glob.glob(os.path.join(WAV_DIR, "*.wav"))):
        for a in import_file(wav, AUDIO):
            if not isinstance(a, unreal.SoundWave):
                continue
            name = a.get_name()
            for prefix, att, con, vol in rules:
                if name.startswith(prefix):
                    a.set_editor_property("attenuation_settings", att)
                    a.set_editor_property("concurrency_set", set([con]) if con else set())
                    a.set_editor_property("volume", vol)
                    break
            lib.save_loaded_asset(a)
            out[name] = a
    log(f"Sonidos: {len(out)}")
    return out


# ---------------------------------------------------------------------------
# Materiales: instancias del maestro de armas con las máscaras horneadas de la P-17
# ---------------------------------------------------------------------------

SURFACES = {
    # nitrurado negro satinado: la corredera se desgasta en aristas y estrías
    "MI_P17_Slide": dict(BaseColor=(0.028, 0.028, 0.03), EdgeColor=(0.30, 0.30, 0.31), EdgeWear=0.45, Grime=0.4,
                         Roughness=0.38, EdgeRoughness=0.28, Metallic=0.35, EdgeMetallic=0.95, DetailStrength=0.2,
                         SmudgeAmount=0.18, SpeckleAmount=0.0, ScratchAmount=0.12),
    # polímero negro texturizado (punteado del puño)
    "MI_P17_Frame": dict(BaseColor=(0.032, 0.032, 0.03), EdgeColor=(0.09, 0.09, 0.09), EdgeWear=0.3, Grime=0.45,
                         Roughness=0.66, EdgeRoughness=0.5, Metallic=0.0, EdgeMetallic=0.0, DetailStrength=0.5,
                         SmudgeAmount=0.14, SpeckleAmount=0.7, ScratchAmount=0.05),
    # acero (cañón, palancas, cargador)
    "MI_P17_Steel": dict(BaseColor=(0.05, 0.05, 0.052), EdgeColor=(0.45, 0.45, 0.46), EdgeWear=0.5, Grime=0.5,
                         Roughness=0.33, EdgeRoughness=0.25, Metallic=0.6, EdgeMetallic=1.0, DetailStrength=0.2,
                         SmudgeAmount=0.2, SpeckleAmount=0.0, ScratchAmount=0.15),
}


def make_materials():
    tex = None
    for a in import_file(os.path.join(TEX_DIR, "T_P17_Masks.png"), f"{P17}/Textures"):
        if isinstance(a, unreal.Texture2D):
            a.set_editor_property("srgb", False)
            a.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_MASKS)
            lib.save_loaded_asset(a)
            tex = a
    if not tex:
        raise RuntimeError("No se importó T_P17_Masks")
    with open(os.path.join(TEX_DIR, "T_P17_Masks.json")) as f:
        scale = float(json.load(f)["uv_scale_cm"])
    master = must("/Game/Weapons/AR7/Materials/M_Weapon_Master")
    # El cargador que cae se dibuja con InstancedStaticMesh (UBLDebrisPoolComponent)
    if not master.get_editor_property("used_with_instanced_static_meshes"):
        master.set_editor_property("used_with_instanced_static_meshes", True)
        mel.recompile_material(master)
        lib.save_loaded_asset(master)
    out = {}
    for name, params in SURFACES.items():
        folder = f"{P17}/Materials"
        mi = load(f"{folder}/{name}") or tools.create_asset(name, folder, unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
        mi.set_editor_property("parent", master)
        mel.set_material_instance_texture_parameter_value(mi, "Masks", tex)
        for p, v in params.items():
            if isinstance(v, tuple):
                mel.set_material_instance_vector_parameter_value(mi, p, unreal.LinearColor(*v, 1))
            else:
                mel.set_material_instance_scalar_parameter_value(mi, p, v)
        mel.set_material_instance_scalar_parameter_value(mi, "DetailTiling", scale / 3.0)
        mel.set_material_instance_scalar_parameter_value(mi, "SmudgeTiling", scale / 25.0)
        lib.save_loaded_asset(mi)
        out[name] = mi
    out["MI_P17_Tritium"] = must("/Game/Weapons/AR7/Materials/M_AR7_Tritium")
    log(f"Materiales: escala UV {scale:.1f} cm")
    return out


# ---------------------------------------------------------------------------
# Mallas
# ---------------------------------------------------------------------------

def import_static(name, dest, materials, collision=False):
    mesh = None
    for a in import_file(os.path.join(FBX_DIR, name + ".fbx"), dest):
        if isinstance(a, unreal.StaticMesh):
            mesh = a
        elif isinstance(a, unreal.MaterialInterface) and a.get_path_name().startswith(dest):
            lib.delete_loaded_asset(a)   # material autogenerado
    if mesh is None:
        raise RuntimeError(f"No se importó {name}")
    new = []
    for m in mesh.get_editor_property("static_materials"):
        slot = str(m.get_editor_property("material_slot_name"))
        mat = materials.get(slot, materials.get("*"))
        new.append(unreal.StaticMaterial(material_interface=mat, material_slot_name=slot, uv_channel_data=m.get_editor_property("uv_channel_data")))
    mesh.set_editor_property("static_materials", new)
    ns = mesh.get_editor_property("nanite_settings")
    ns.set_editor_property("enabled", False)
    mesh.set_editor_property("nanite_settings", ns)
    if collision:
        unreal.EditorStaticMeshLibrary.remove_collisions(mesh)
        unreal.EditorStaticMeshLibrary.add_simple_collisions(mesh, unreal.ScriptCollisionShapeType.BOX)
    lib.save_loaded_asset(mesh)
    return mesh


def import_pistol(materials):
    dest = f"{P17}/Meshes"
    mesh = None
    for a in import_file(os.path.join(FBX_DIR, "SK_P17.fbx"), dest):
        if isinstance(a, unreal.SkeletalMesh):
            mesh = a
    if mesh is None:
        raise RuntimeError("No se importó SK_P17")
    autos, new = [], []
    for m in mesh.get_editor_property("materials"):
        slot = str(m.get_editor_property("material_slot_name"))
        auto = m.get_editor_property("material_interface")
        if slot not in materials:
            raise RuntimeError(f"Slot desconocido en SK_P17: {slot}")
        new.append(unreal.SkeletalMaterial(material_interface=materials[slot], material_slot_name=slot,
                                           uv_channel_data=m.get_editor_property("uv_channel_data")))
        if auto and auto.get_path_name().startswith(dest):
            autos.append(auto)
    mesh.set_editor_property("materials", new)
    lib.save_loaded_asset(mesh)
    for a in autos:
        lib.delete_loaded_asset(a)
    # LOD para la tercera persona (mismo ajuste que el AR-7)
    ls = load("/Game/Weapons/Shared/LODSettings_Weapon")
    if ls:
        mesh.set_editor_property("lod_settings", ls)
        sub = unreal.get_editor_subsystem(unreal.SkeletalMeshEditorSubsystem)
        sub.regenerate_lod(mesh, 3, True, False)
        lib.save_loaded_asset(mesh)
    b = mesh.get_bounds()
    log(f"SK_P17: bounds origin={b.origin} extent={b.box_extent}")
    return mesh


# ---------------------------------------------------------------------------
# DA_P17
# ---------------------------------------------------------------------------

def timed(t, sound):
    s = unreal.BLTimedSound()
    s.set_editor_property("time", t)
    s.set_editor_property("sound", sound)
    return s


def sounds_in(folder):
    out = []
    for p in sorted(lib.list_assets(folder, recursive=False)):
        a = unreal.load_asset(p.split(".")[0])
        if isinstance(a, unreal.SoundWave):
            out.append(a)
    return out


def rot(pitch, yaw, roll):
    return unreal.Rotator(pitch=pitch, yaw=yaw, roll=roll)


def make_data(snd, mesh, mag, casing):
    path = f"{P17}/DA_P17"
    da = load(path)
    if not da:
        factory = unreal.DataAssetFactory()
        factory.set_editor_property("data_asset_class", unreal.BLWeaponData)
        da = tools.create_asset("DA_P17", P17, unreal.BLWeaponData, factory)

    def many(prefix):
        return [s for n, s in sorted(snd.items()) if n.startswith(prefix)]

    poses = unreal.BLWeaponPoses()
    # Desplazamientos en espacio de cámara respecto a ADS (ajustados con capturas de la prueba Pistol)
    for k, v in dict(hip_location=unreal.Vector(-3, 3, -2.5), hip_rotation=rot(1, -3, -4),
                     sprint_location=unreal.Vector(-14, 3, -18), sprint_rotation=rot(-40, -10, -15),
                     mantle_location=unreal.Vector(-12, 4, -16), mantle_rotation=rot(-30, -10, -20),
                     reload_grip_location=unreal.Vector(40, 2, -9), reload_rotation=rot(15, -20, 32),
                     equip_location=unreal.Vector(-14, 10, -32), equip_rotation=rot(-55, -10, 20),
                     pose_reference_aim_distance=34.0).items():
        poses.set_editor_property(k, v)

    insert = 0.55
    anim = "/Game/Characters/Mannequins/Anims/Pistol/"
    props = {
        "display_name": unreal.Text("P-17"),
        "mesh": mesh,
        "muzzle_socket": "Muzzle", "eject_socket": "Eject", "left_hand_socket": "HandGrip_L", "sight_socket": "Sight",
        "mag_socket": "Mag", "magazine_bone": "magazine", "magazine_mesh": mag,
        "charging_handle_socket": "slide_rear",
        "aim_eye_distance": 34.0,
        "left_hand_grip_rotation": rot(90, 0, 0),
        # La mano del Mannequin queda ~4 cm alta y 2 cm adelantada en un puño de pistola, y 10° más vertical
        "right_hand_offset": unreal.Vector(0, -2, -4), "right_hand_rotation": rot(0, 0, -10),
        "poses": poses,
        # Disparo: semiautomática 9 mm
        "fire_mode": unreal.BLFireMode.SEMI, "rounds_per_minute": 420.0,
        "damage": 24.0, "headshot_multiplier": 2.5, "limb_multiplier": 0.75,
        "max_range": 8000.0, "noise_range": 3000.0, "impact_noise_range": 700.0,
        "falloff_start": 1500.0, "falloff_end": 4000.0, "falloff_min_multiplier": 0.55, "sprint_to_fire_time": 0.08,
        "magazine_size": 17, "max_reserve_ammo": 85, "start_reserve_ammo": 51,
        "hip_spread": 1.6, "aim_spread": 0.12, "move_spread": 1.0, "air_spread": 3.0,
        "spread_per_shot": 0.6, "max_bloom": 2.5, "bloom_recovery": 9.0, "aim_bloom_multiplier": 0.2,
        # Retroceso: salto seco por disparo que vuelve casi entero (pistola: mucho "flip", poca deriva)
        "recoil_vertical": 1.1, "recoil_horizontal": 0.35, "recoil_horizontal_bias": 0.1,
        "recoil_first_shot_multiplier": 1.0, "recoil_ramp_shots": 1, "recoil_aim_multiplier": 0.85,
        "recoil_apply_speed": 40.0, "recoil_recovery_fraction": 0.85, "recoil_recovery_speed": 9.0, "recoil_recovery_delay": 0.06,
        "camera_kick": rot(16, 0, 4), "weapon_kick_location": unreal.Vector(-40, 0, 22),
        "weapon_kick_rotation": rot(65, 6, 10), "visual_kick_aim_multiplier": 0.55,
        # Recarga: táctica 1,55 s / en vacío 1,9 s; el cargador entra al 55 %, la corredera se suelta al 77 %
        "reload_time": 1.55, "empty_reload_time": 1.9, "chamber_round": True, "reload_ammo_insert_time": insert,
        "reload_style": unreal.BLReloadStyle.PISTOL,
        "bolt_bone": "slide", "bolt_travel": 2.4, "bolt_cycle_time": 0.06, "bolt_release_time": insert + 0.22,
        "trigger_bone": "trigger", "trigger_travel": 0.35,
        "idle_anim": unreal.load_asset(anim + "MF_Pistol_Idle_ADS"),
        "reload_anim": unreal.load_asset(anim + "MM_Pistol_Reload"),
        "equip_anim": unreal.load_asset(anim + "MM_Pistol_Equip"),
        "equip_time": 0.45, "holster_time": 0.3,
        "fire_sounds": many("SW_P17_Fire_Close_"),
        "fire_tail_sounds": many("SW_P17_Tail_"),
        "fire_sounds3d": many("SW_P17_Fire_Close3D_"),
        "fire_distant_sounds": many("SW_P17_Fire_Distant_"),
        "distant_fire_distance": 2500.0,
        "dry_fire_sound": snd["SW_P17_DryFire"],
        "reload_sounds": [timed(0.05, snd["SW_P17_MagOut"]), timed(insert - 0.02, snd["SW_P17_MagIn"])],
        "empty_reload_sounds": [timed(0.05, snd["SW_P17_MagOut"]), timed(insert - 0.02, snd["SW_P17_MagIn"]),
                                timed(insert + 0.155, snd["SW_P17_SlidePull"]), timed(insert + 0.215, snd["SW_P17_SlideRelease"])],
        "magazine_drop_sound": snd["SW_P17_MagDrop_01"],
        "casing_sounds": sounds_in("/Game/Audio/Casings/Concrete"),
        "handling_sounds": sounds_in("/Game/Audio/Foley"),
        "surface_effects": must("/Game/FX/DA_SurfaceEffects"),
        "muzzle_flash_mesh": must("/Game/Weapons/AR7/Meshes/SM_AR7_MuzzleFlash"),
        "muzzle_flash_scale": 0.55, "muzzle_flash_duration": 0.025, "muzzle_light_intensity": 5000.0,
        "casing_mesh": casing,
        "impact_scale": 0.8,
    }
    for k, v in props.items():
        da.set_editor_property(k, v)
    lib.save_loaded_asset(da)
    log(f"DA_P17 listo: {len(props['fire_sounds'])} disparos cercanos, {len(props['fire_distant_sounds'])} lejanos")


def main():
    snd = import_audio()
    mats = make_materials()
    mesh = import_pistol(mats)
    mag = import_static("SM_P17_Mag", f"{P17}/Meshes", mats, collision=True)
    casing = import_static("SM_Casing_9mm", "/Game/FX/Meshes", {"*": must("/Game/FX/Materials/M_Brass")})
    make_data(snd, mesh, mag, casing)
    log("OK")


main()
