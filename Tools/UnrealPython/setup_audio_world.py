"""Audio del Bloque 7: voces, balas, ambientes por zona, combate lejano, música, clases de sonido y mezcla. Idempotente.

Importa solo las carpetas nuevas de ArtSource/Audio/SFX (las de setup_audio_fx.py no se tocan):
  Voice/Radio, Voice/Barks, Weapons/Bullet, Ambience/Zones, Ambience/Distant, Music
Crea: SA_Voice, SA_Bullet, SA_Distant, SA_AmbSmall, SA_AmbLarge, SCon_Voice; SC_SFX/SC_Voice/SC_Music/SC_Ambience;
SMix_RadioDuck (baja ambiente y música mientras habla la radio); RE_Alley y RE_Interior (zonas acústicas).

Uso: UnrealEditor-Cmd.exe Blackline.uproject -run=pythonscript -script="Tools/UnrealPython/setup_audio_world.py"
Generar antes los WAV: Tools/audio/gen_voices.py y Tools/audio/gen_world_audio.py.
"""
import glob
import os
import unreal

PROJECT = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())
SFX_DIR = os.path.join(PROJECT, "ArtSource", "Audio", "SFX")
SETTINGS = "/Game/Audio/Settings"

tools = unreal.AssetToolsHelpers.get_asset_tools()
lib = unreal.EditorAssetLibrary


def log(m):
    unreal.log(f"[BL_AudioWorld] {m}")


def load(path):
    return unreal.load_asset(path) if lib.does_asset_exist(path) else None


def get_or_create(name, folder, cls, factory):
    return load(f"{folder}/{name}") or tools.create_asset(name, folder, cls, factory)


def attenuation(name, inner, falloff, lpf_far=4000.0, reverb=True, occlusion=False, lpf=True):
    att = get_or_create(name, SETTINGS, unreal.SoundAttenuation, unreal.SoundAttenuationFactory())
    s = att.get_editor_property("attenuation")
    s.set_editor_property("attenuation_shape_extents", unreal.Vector(inner, 0, 0))
    s.set_editor_property("falloff_distance", falloff)
    s.set_editor_property("distance_algorithm", unreal.AttenuationDistanceModel.NATURAL_SOUND)
    s.set_editor_property("spatialize", True)
    s.set_editor_property("attenuate_with_lpf", lpf)
    s.set_editor_property("lpf_radius_min", inner)
    s.set_editor_property("lpf_radius_max", inner + falloff)
    s.set_editor_property("lpf_frequency_at_min", 20000.0)
    s.set_editor_property("lpf_frequency_at_max", lpf_far)
    s.set_editor_property("enable_reverb_send", reverb)
    s.set_editor_property("enable_occlusion", occlusion)
    if occlusion:
        s.set_editor_property("occlusion_low_pass_filter_frequency", 1200.0)
        s.set_editor_property("occlusion_volume_attenuation", 0.5)
        s.set_editor_property("occlusion_interpolation_time", 0.15)
    att.set_editor_property("attenuation", s)
    lib.save_loaded_asset(att)
    return att


def concurrency(name, max_count):
    c = get_or_create(name, SETTINGS, unreal.SoundConcurrency, unreal.SoundConcurrencyFactory())
    s = c.get_editor_property("concurrency")
    s.set_editor_property("max_count", max_count)
    s.set_editor_property("resolution_rule", unreal.MaxConcurrentResolutionRule.STOP_OLDEST)
    c.set_editor_property("concurrency", s)
    lib.save_loaded_asset(c)
    return c


def sound_class(name):
    sc = get_or_create(name, SETTINGS, unreal.SoundClass, unreal.SoundClassFactory())
    lib.save_loaded_asset(sc)
    return sc


def reverb(name, props):
    re = get_or_create(name, SETTINGS, unreal.ReverbEffect, unreal.ReverbEffectFactory())
    for k, v in props.items():
        re.set_editor_property(k, v)
    lib.save_loaded_asset(re)
    return re


def import_file(filename, dest):
    task = unreal.AssetImportTask()
    task.filename = filename
    task.destination_path = dest
    task.automated = True
    task.replace_existing = True
    task.save = True
    tools.import_asset_tasks([task])
    return [unreal.load_asset(p) for p in task.imported_object_paths]


def main():
    att = {
        "voice": attenuation("SA_Voice", 300, 4500, 3500, True, True),
        "bullet": attenuation("SA_Bullet", 250, 1500, 9000, True, False),
        # Lejanos: se colocan a 150-250 m, así que el volumen apenas cae; la posición da la dirección
        "distant": attenuation("SA_Distant", 30000, 25000, 1500, True, False),
        "amb_small": attenuation("SA_AmbSmall", 150, 1600, 4000, True, True),
        "amb_large": attenuation("SA_AmbLarge", 700, 4000, 3000, True, False),
        # Granada (Bloque 11): explosión que se oye en todo el barrio; rebotes como un impacto pequeño
        "explosion": attenuation("SA_Explosion", 800, 9000, 2500, True, False),
        "bounce": attenuation("SA_GrenadeBounce", 100, 1800, 6000, True, True),
        # Vehículos (fases 8-9): motor del blindado y rotor del helicóptero se oyen desde lejos
        "vehicle": attenuation("SA_Vehicle", 600, 7000, 2500, True, False),
        "rotor": attenuation("SA_Rotor", 1500, 14000, 2000, True, False),
    }
    con_voice = concurrency("SCon_Voice", 3)
    concurrency("SCon_WeaponFire", 12)   # capa cercana + lejana por disparo de la IA
    sc = {k: sound_class(f"SC_{k}") for k in ("SFX", "Voice", "Music", "Ambience")}

    # (patrón, atenuación, concurrencia, clase, volumen, bucle)
    rules = [
        ("Voice/Radio/", None, None, "Voice", 1.0, False),
        ("Voice/Barks/", "voice", con_voice, "Voice", 1.0, False),
        ("Weapons/Bullet/SW_Bullet_Whiz", "bullet", None, "SFX", 0.7, False),
        ("Weapons/Bullet/", "bullet", None, "SFX", 1.0, False),
        ("Weapons/Grenade/SW_Grenade_Explosion", "explosion", None, "SFX", 1.0, False),
        ("Weapons/Grenade/SW_Grenade_Bounce", "bounce", None, "SFX", 0.7, False),
        ("Weapons/Grenade/", None, None, "SFX", 0.8, False),       # anilla y lanzamiento (2D, jugador)
        ("Vehicles/SW_BTR_Engine", "vehicle", None, "SFX", 0.9, True),
        ("Vehicles/SW_BTR_Cannon", "explosion", None, "SFX", 1.0, False),
        ("Vehicles/SW_Heli_Rotor", "rotor", None, "SFX", 1.0, True),
        ("Vehicles/SW_Drone_Buzz", "vehicle", None, "SFX", 0.8, True),      # misión 2
        ("Vehicles/SW_Boat_Outboard", "vehicle", None, "SFX", 0.8, True),
        ("Vehicles/SW_Ship_Horn", "rotor", None, "SFX", 1.0, False),         # misión 3
        ("Vehicles/SW_Jet_Flyby", None, None, "SFX", 1.0, False),           # misión 4: pasada de los cazas (2D)
        ("World/SW_Mortar_Whistle", "explosion", None, "SFX", 0.9, False),
        ("Vehicles/", "vehicle", None, "SFX", 0.9, False),
        ("World/", "amb_small", None, "SFX", 1.0, False),                    # puertas y carga de brecha
        ("Ambience/Zones/SW_AmbZ_Siren", "rotor", None, "Ambience", 0.9, True),
        ("Ambience/Zones/SW_AmbZ_Flare", "amb_large", None, "Ambience", 0.8, True),
        ("Ambience/Zones/SW_AmbZ_Hum", "amb_small", None, "Ambience", 0.3, True),
        ("Ambience/Zones/SW_AmbZ_Fire", "amb_small", None, "Ambience", 0.7, True),
        ("Ambience/Zones/", "amb_large", None, "Ambience", 0.6, True),
        ("Ambience/Distant/SW_Creak", "amb_small", None, "Ambience", 0.5, False),
        ("Ambience/Distant/SW_Dist_Siren", "distant", None, "Ambience", 0.45, False),
        ("Ambience/Distant/SW_Dist_Explosion", "distant", None, "Ambience", 0.9, False),
        ("Ambience/Distant/", "distant", None, "Ambience", 0.6, False),
        ("Music/SW_Mus_Combat_Loop", None, None, "Music", 1.0, True),
        ("Music/SW_Mus_Menu_Loop", None, None, "Music", 0.9, True),
        ("Music/", None, None, "Music", 1.0, False),
        ("UI/", None, None, "SFX", 0.55, False),             # hitmarker, objetivo y menú (Bloque 10), 2D
    ]
    count = 0
    folders = ["Voice", "Weapons/Bullet", "Weapons/Grenade", "Ambience/Zones", "Ambience/Distant", "Music", "UI", "Vehicles", "World"]
    for folder in folders:
        for wav in sorted(glob.glob(os.path.join(SFX_DIR, folder, "**", "*.wav"), recursive=True)):
            rel = os.path.relpath(wav, SFX_DIR).replace("\\", "/")
            for a in import_file(wav, "/Game/Audio/" + os.path.dirname(rel)):
                if not isinstance(a, unreal.SoundWave):
                    continue
                for pattern, a_key, con, cls, vol, loop in rules:
                    if rel.startswith(pattern):
                        a.set_editor_property("attenuation_settings", att[a_key] if a_key else None)
                        a.set_editor_property("concurrency_set", set([con]) if con else set())
                        a.set_editor_property("sound_class_object", sc[cls])
                        a.set_editor_property("volume", vol)
                        a.set_editor_property("looping", loop)
                        break
                lib.save_loaded_asset(a)
                count += 1
    log(f"Sonidos importados: {count}")

    # Clases de los sonidos que ya existían: ambiente de ciudad -> Ambience; radio (clics) -> Voice
    for path, cls in (("/Game/Audio/Ambience/SW_Amb_WarCity_Loop", "Ambience"), ("/Game/Audio/Radio/SW_Radio_In", "Voice"),
                      ("/Game/Audio/Radio/SW_Radio_Out", "Voice")):
        a = load(path)
        if a:
            a.set_editor_property("sound_class_object", sc[cls])
            lib.save_loaded_asset(a)

    # Mezcla de la radio: ambiente y música bajan para que se entienda la voz
    mix = get_or_create("SMix_RadioDuck", SETTINGS, unreal.SoundMix, unreal.SoundMixFactory())
    adjusters = []
    for cls, vol in (("Ambience", 0.5), ("Music", 0.55)):
        adj = unreal.SoundClassAdjuster()
        adj.set_editor_property("sound_class_object", sc[cls])
        adj.set_editor_property("volume_adjuster", vol)
        adjusters.append(adj)
    mix.set_editor_property("sound_class_effects", adjusters)
    mix.set_editor_property("fade_in_time", 0.25)
    mix.set_editor_property("fade_out_time", 0.8)
    lib.save_loaded_asset(mix)

    # Volúmenes de Opciones > Audio: los ajusta UBLUserSettings en tiempo de ejecución (SetSoundMixClassOverride)
    user = get_or_create("SMix_User", SETTINGS, unreal.SoundMix, unreal.SoundMixFactory())
    lib.save_loaded_asset(user)

    # Zonas acústicas
    reverb("RE_Alley", {"density": 1.0, "diffusion": 0.55, "gain": 0.4, "gain_hf": 0.6, "decay_time": 1.1, "decay_hf_ratio": 0.7,
                        "reflections_gain": 1.4, "reflections_delay": 0.008, "late_gain": 1.0, "late_delay": 0.015, "air_absorption_gain_hf": 0.994})
    reverb("RE_Interior", {"density": 1.0, "diffusion": 1.0, "gain": 0.45, "gain_hf": 0.45, "decay_time": 0.65, "decay_hf_ratio": 0.5,
                           "reflections_gain": 1.2, "reflections_delay": 0.004, "late_gain": 1.3, "late_delay": 0.008, "air_absorption_gain_hf": 0.99})
    log("OK")


main()
