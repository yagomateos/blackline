"""Vuelca duración, frames y notifies de animaciones al log ([BL_Inspect]).

Uso: UnrealEditor-Cmd.exe Blackline.uproject -ExecutePythonScript="Tools/UnrealPython/inspect_anims.py"
Editar ANIMS para inspeccionar otras.
"""
import unreal

ROOT = "/Game/Characters/Mannequins/Anims/Rifle/"
ANIMS = [
    "MF_Rifle_Idle_ADS", "MM_Rifle_Fire", "MM_Rifle_Reload", "MM_Rifle_Equip", "MM_Rifle_DryFire",
    "Jog/MF_Rifle_Jog_Fwd", "Walk/MF_Rifle_Walk_Fwd",
]


def log(m):
    unreal.log(f"[BL_Inspect] {m}")


for name in ANIMS:
    anim = unreal.load_asset(ROOT + name)
    if not anim:
        log(f"No existe {name}")
        continue
    length = anim.get_editor_property("sequence_length") if hasattr(anim, "sequence_length") else unreal.AnimationLibrary.get_sequence_length(anim)
    frames = unreal.AnimationLibrary.get_num_frames(anim)
    log(f"== {name}: {length:.3f} s, {frames} frames, clase {anim.get_class().get_name()}")
    for ev in unreal.AnimationLibrary.get_animation_notify_events(anim):
        notify = ev.get_editor_property("notify")
        nname = ev.get_editor_property("notify_name")
        t = ev.get_editor_property("trigger_time_offset")
        log(f"   notify {nname} ({notify.get_class().get_name() if notify else '-'}) @ {ev.get_time():.3f}")
    for curve in unreal.AnimationLibrary.get_animation_curve_names(anim, unreal.RawCurveTrackTypes.RCT_FLOAT):
        log(f"   curva {curve}")
