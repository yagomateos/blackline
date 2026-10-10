"""Precisión en ADS (2026-10-09): ajusta la dispersión apuntando de las armas cuyo asset la fija a mano.
El AR-7 usa los valores por defecto de UBLWeaponData (AimSpread 0,06, AimBloomMultiplier 0,15)."""
import unreal

VALUES = {
    "/Game/Weapons/P17/DA_P17": {"aim_spread": 0.12, "aim_bloom_multiplier": 0.2},
}

for path, props in VALUES.items():
    data = unreal.load_asset(path)
    if not data:
        unreal.log_warning(f"[BL_ADS] falta {path}")
        continue
    for k, v in props.items():
        data.set_editor_property(k, v)
    unreal.EditorAssetLibrary.save_loaded_asset(data, only_if_is_dirty=False)
    unreal.log_warning(f"[BL_ADS] {path}: " + ", ".join(f"{k}={data.get_editor_property(k)}" for k in props))

ar7 = unreal.load_asset("/Game/Weapons/AR7/DA_AR7") or unreal.load_asset("/Game/Weapons/DA_AR7")
if ar7:
    unreal.log_warning(f"[BL_ADS] AR-7: aim_spread={ar7.get_editor_property('aim_spread')} aim_bloom={ar7.get_editor_property('aim_bloom_multiplier')}")
unreal.log_warning("[BL_ADS] OK")
