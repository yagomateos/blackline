"""Escopeta SG-12 "Mastín": importa malla, cartuchos, texturas, sonidos y crea DA_SG12. Idempotente.

Requisitos: FBX de ArtSource/Blender/scripts/gen_sg12.py (--bake; las máscaras se copian a ArtSource/Textures/Weapons),
WAV de Tools/audio/gen_sg12_sfx.py y los assets compartidos de las armas (setup_weapon_assets.py, setup_audio_fx.py).

  UnrealEditor-Cmd.exe Blackline.uproject -run=pythonscript -script="Tools/UnrealPython/setup_sg12_assets.py"
"""
import glob
import json
import os
import unreal

PROJECT = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())
FBX_DIR = os.path.join(PROJECT, "ArtSource", "Blender", "export")
TEX_DIR = os.path.join(PROJECT, "ArtSource", "Textures", "Weapons")
WAV_DIR = os.path.join(PROJECT, "ArtSource", "Audio", "SFX", "Weapons", "SG12")
SG12 = "/Game/Weapons/SG12"
AUDIO = "/Game/Audio/Weapons/SG12"

tools = unreal.AssetToolsHelpers.get_asset_tools()
lib = unreal.EditorAssetLibrary
mel = unreal.MaterialEditingLibrary


def log(m):
    unreal.log_warning(f"[BL_SG12] {m}")


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
    con_fire = must("/Game/Audio/Settings/SCon_WeaponFire")
    # (prefijo, atenuación, concurrencia, volumen): sin atenuación = 2D para el jugador
    rules = [("SW_SG12_Fire_Close3D_", att_fire, con_fire, 1.0), ("SW_SG12_Fire_Close_", None, con_fire, 1.0),
             ("SW_SG12_Fire_Distant_", att_far, con_fire, 1.0), ("SW_SG12_Tail_", None, None, 0.75),
             ("SW_SG12_", None, None, 0.85)]
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
# Materiales: instancias del maestro de armas con las máscaras horneadas de la SG-12
# ---------------------------------------------------------------------------

SURFACES = {
    # acero pavonado mate (fosfatado): desgaste plateado en aristas, cajón y cañón
    "MI_SG12_Metal": dict(BaseColor=(0.035, 0.035, 0.037), EdgeColor=(0.42, 0.42, 0.43), EdgeWear=0.55, Grime=0.5,
                          Roughness=0.48, EdgeRoughness=0.28, Metallic=0.55, EdgeMetallic=1.0, DetailStrength=0.3,
                          SmudgeAmount=0.2, SpeckleAmount=0.0, ScratchAmount=0.18),
    # polímero negro texturizado (culata y guardamanos)
    "MI_SG12_Polymer": dict(BaseColor=(0.035, 0.035, 0.033), EdgeColor=(0.1, 0.1, 0.1), EdgeWear=0.3, Grime=0.5,
                            Roughness=0.7, EdgeRoughness=0.5, Metallic=0.0, EdgeMetallic=0.0, DetailStrength=0.5,
                            SmudgeAmount=0.16, SpeckleAmount=0.6, ScratchAmount=0.06),
    "MI_SG12_Rubber": dict(BaseColor=(0.016, 0.016, 0.016), EdgeColor=(0.05, 0.05, 0.05), EdgeWear=0.1, Grime=0.6,
                           Roughness=0.9, EdgeRoughness=0.85, Metallic=0.0, EdgeMetallic=0.0, DetailStrength=0.6,
                           SmudgeAmount=0.1, SpeckleAmount=0.3, ScratchAmount=0.0),
    # vaina de plástico rojo de los cartuchos (algo brillante, gastada en la boca)
    "MI_SG12_Hull": dict(BaseColor=(0.32, 0.025, 0.02), EdgeColor=(0.45, 0.12, 0.1), EdgeWear=0.25, Grime=0.3,
                         Roughness=0.45, EdgeRoughness=0.6, Metallic=0.0, EdgeMetallic=0.0, DetailStrength=0.2,
                         SmudgeAmount=0.1, SpeckleAmount=0.0, ScratchAmount=0.05),
}


def make_materials():
    tex = None
    for a in import_file(os.path.join(TEX_DIR, "T_SG12_Masks.png"), f"{SG12}/Textures"):
        if isinstance(a, unreal.Texture2D):
            a.set_editor_property("srgb", False)
            a.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_MASKS)
            lib.save_loaded_asset(a)
            tex = a
    if not tex:
        raise RuntimeError("No se importó T_SG12_Masks")
    with open(os.path.join(TEX_DIR, "T_SG12_Masks.json")) as f:
        scale = float(json.load(f)["uv_scale_cm"])
    master = must("/Game/Weapons/AR7/Materials/M_Weapon_Master")
    out = {}
    for name, params in SURFACES.items():
        folder = f"{SG12}/Materials"
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
    out["MI_SG12_Fiber"] = must("/Game/Weapons/AR7/Materials/M_AR7_Tritium")
    out["MI_SG12_Brass"] = must("/Game/FX/Materials/M_Brass")
    log(f"Materiales: escala UV {scale:.1f} cm")
    return out


# ---------------------------------------------------------------------------
# Mallas
# ---------------------------------------------------------------------------

def import_static(name, dest, materials):
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
        new.append(unreal.StaticMaterial(material_interface=materials[slot], material_slot_name=slot,
                                         uv_channel_data=m.get_editor_property("uv_channel_data")))
    mesh.set_editor_property("static_materials", new)
    ns = mesh.get_editor_property("nanite_settings")
    ns.set_editor_property("enabled", False)
    mesh.set_editor_property("nanite_settings", ns)
    lib.save_loaded_asset(mesh)
    return mesh


def import_shotgun(materials):
    dest = f"{SG12}/Meshes"
    mesh = None
    for a in import_file(os.path.join(FBX_DIR, "SK_SG12.fbx"), dest):
        if isinstance(a, unreal.SkeletalMesh):
            mesh = a
    if mesh is None:
        raise RuntimeError("No se importó SK_SG12")
    autos, new = [], []
    for m in mesh.get_editor_property("materials"):
        slot = str(m.get_editor_property("material_slot_name"))
        auto = m.get_editor_property("material_interface")
        if slot not in materials:
            raise RuntimeError(f"Slot desconocido en SK_SG12: {slot}")
        new.append(unreal.SkeletalMaterial(material_interface=materials[slot], material_slot_name=slot,
                                           uv_channel_data=m.get_editor_property("uv_channel_data")))
        if auto and auto.get_path_name().startswith(dest):
            autos.append(auto)
    mesh.set_editor_property("materials", new)
    lib.save_loaded_asset(mesh)
    for a in autos:
        lib.delete_loaded_asset(a)
    ls = load("/Game/Weapons/Shared/LODSettings_Weapon")
    if ls:
        mesh.set_editor_property("lod_settings", ls)
        sub = unreal.get_editor_subsystem(unreal.SkeletalMeshEditorSubsystem)
        sub.regenerate_lod(mesh, 3, True, False)
        lib.save_loaded_asset(mesh)
    b = mesh.get_bounds()
    log(f"SK_SG12: bounds origin={b.origin} extent={b.box_extent}")
    return mesh


# ---------------------------------------------------------------------------
# DA_SG12
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


def make_data(snd, mesh, shell, hull):
    path = f"{SG12}/DA_SG12"
    da = load(path)
    if not da:
        factory = unreal.DataAssetFactory()
        factory.set_editor_property("data_asset_class", unreal.BLWeaponData)
        da = tools.create_asset("DA_SG12", SG12, unreal.BLWeaponData, factory)

    def many(prefix):
        return [s for n, s in sorted(snd.items()) if n.startswith(prefix)]

    poses = unreal.BLWeaponPoses()
    # Desplazamientos en espacio de cámara respecto a ADS (de partida los del AR-7; ajustados con capturas de la
    # prueba Shotgun). Recarga: girada a la derecha para enseñar la portilla de carga a la mano izquierda.
    for k, v in dict(hip_location=unreal.Vector(6, 4.5, -3), hip_rotation=rot(0.5, -5, -4),
                     sprint_location=unreal.Vector(5, 5, -5), sprint_rotation=rot(1, -22, -30),
                     mantle_location=unreal.Vector(-4, 2, -5), mantle_rotation=rot(-10, -10, -20),
                     reload_grip_location=unreal.Vector(40, 6, -12), reload_rotation=rot(14, -18, 48),
                     equip_location=unreal.Vector(0, 4, -24), equip_rotation=rot(-35, -10, 20),
                     pose_reference_aim_distance=12.0).items():
        poses.set_editor_property(k, v)

    anim = "/Game/Characters/Mannequins/Anims/Rifle/"
    props = {
        "display_name": unreal.Text("SG-12"),
        "mesh": mesh,
        "muzzle_socket": "Muzzle", "eject_socket": "Eject", "left_hand_socket": "HandGrip_L", "sight_socket": "Sight",
        # La "boca del cargador" de la escopeta es la portilla de carga; el "cargador" en la mano, un cartucho
        "mag_socket": "LoadPort", "magazine_bone": "None", "magazine_mesh": shell,
        "aim_eye_distance": 12.0,
        "poses": poses,
        # Disparo: corredera, 9 perdigones 00 (8,4 mm) por cartucho
        "fire_mode": unreal.BLFireMode.PUMP, "rounds_per_minute": 70.0,
        "pellet_count": 9, "pellet_spread": 3.0, "pellet_aim_spread": 2.2,
        "damage": 16.0, "headshot_multiplier": 1.8, "limb_multiplier": 0.75,
        "max_range": 4500.0, "noise_range": 4500.0, "impact_noise_range": 900.0,
        "falloff_start": 700.0, "falloff_end": 2500.0, "falloff_min_multiplier": 0.2, "sprint_to_fire_time": 0.15,
        "magazine_size": 7, "max_reserve_ammo": 32, "start_reserve_ammo": 21,
        "hip_spread": 0.8, "aim_spread": 0.1, "move_spread": 1.0, "air_spread": 3.0,
        "spread_per_shot": 0.5, "max_bloom": 1.5, "bloom_recovery": 4.0, "aim_bloom_multiplier": 0.5,
        # Retroceso: un golpe fuerte por disparo que se recupera mientras se bombea
        "recoil_vertical": 3.2, "recoil_horizontal": 0.6, "recoil_horizontal_bias": 0.15,
        "recoil_first_shot_multiplier": 1.0, "recoil_ramp_shots": 1, "recoil_aim_multiplier": 0.85,
        "recoil_apply_speed": 25.0, "recoil_recovery_fraction": 0.75, "recoil_recovery_speed": 5.0, "recoil_recovery_delay": 0.12,
        "camera_kick": rot(24, 0, 6), "weapon_kick_location": unreal.Vector(-90, 0, 20),
        "weapon_kick_rotation": rot(55, 6, 12), "visual_kick_aim_multiplier": 0.55,
        # Recarga cartucho a cartucho: 0,35 s para girarla, 0,5 s por cartucho (entra al 72 %), 0,95 s el primero por la
        # ventana en vacío (cierra al 72 %), 0,35 s para volver. ReloadSounds: fracción de la subida; EmptyReloadSounds: de
        # la carga por la ventana
        "reload_style": unreal.BLReloadStyle.SHELLS, "chamber_round": True, "reload_ammo_insert_time": 0.72,
        "shell_reload_start_time": 0.35, "shell_insert_time": 0.5, "shell_port_load_time": 0.95, "shell_reload_end_time": 0.35,
        "reload_sounds": [], "empty_reload_sounds": [timed(0.69, snd["SW_SG12_PumpForwardHard"])],
        "shell_insert_sounds": many("SW_SG12_ShellIn_"),
        # Corredera: guardamanos (hueso pump) 9 cm atrás; la vaina sale al final del tirón
        "bolt_bone": "pump", "bolt_travel": 9.0, "bolt_cycle_time": 0.5, "bolt_release_time": 0.7,
        "cycle_sounds": [timed(0.30, snd["SW_SG12_PumpBack"]), timed(0.58, snd["SW_SG12_PumpForward"])],
        "casing_eject_time": 0.42, "left_hand_on_bolt": True,
        "trigger_bone": "trigger", "trigger_travel": 0.3,
        "idle_anim": unreal.load_asset(anim + "MF_Rifle_Idle_ADS"),
        "reload_anim": unreal.load_asset(anim + "MM_Rifle_Reload"),
        "equip_anim": unreal.load_asset(anim + "MM_Rifle_Equip"),
        "equip_time": 0.6, "holster_time": 0.4,
        "fire_sounds": many("SW_SG12_Fire_Close_"),
        "fire_tail_sounds": many("SW_SG12_Tail_"),
        "fire_sounds3d": many("SW_SG12_Fire_Close3D_"),
        "fire_distant_sounds": many("SW_SG12_Fire_Distant_"),
        "distant_fire_distance": 3000.0,
        "dry_fire_sound": snd["SW_SG12_DryFire"],
        "casing_sounds": sounds_in("/Game/Audio/Casings/Concrete"),
        "handling_sounds": sounds_in("/Game/Audio/Foley"),
        "surface_effects": must("/Game/FX/DA_SurfaceEffects"),
        "muzzle_flash_mesh": must("/Game/Weapons/AR7/Meshes/SM_AR7_MuzzleFlash"),
        "muzzle_flash_scale": 1.45, "muzzle_flash_duration": 0.04, "muzzle_light_intensity": 12000.0,
        "casing_mesh": hull,
        "impact_scale": 0.45,
    }
    for k, v in props.items():
        da.set_editor_property(k, v)
    lib.save_loaded_asset(da)
    log(f"DA_SG12 listo: {len(props['fire_sounds'])} disparos cercanos, {len(props['fire_distant_sounds'])} lejanos")


def main():
    snd = import_audio()
    mats = make_materials()
    mesh = import_shotgun(mats)
    shell = import_static("SM_Shell_12ga", f"{SG12}/Meshes", mats)
    hull = import_static("SM_Hull_12ga", f"{SG12}/Meshes", mats)
    make_data(snd, mesh, shell, hull)
    log("OK")


main()
