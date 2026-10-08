"""Borra assets obsoletos (provisionales sustituidos) solo si nadie los referencia."""
import unreal
lib = unreal.EditorAssetLibrary
CANDIDATES = ["/Game/FX/NS_Impact_Placeholder", "/Game/FX/Materials/M_MuzzleFlash", "/Game/FX/Materials/M_ImpactChip",
              "/Game/FX/Materials/M_Decal_BulletHole", "/Game/Weapons/AR7/Materials/M_AR7_Metal", "/Game/Weapons/AR7/Materials/M_AR7_Polymer",
              "/Game/Weapons/AR7/Materials/M_AR7_PolymerDark", "/Game/Weapons/AR7/Materials/M_AR7_Rubber"]
CANDIDATES += [p.split(".")[0] for p in lib.list_assets("/Game/Audio/Placeholder", recursive=True)]
deleted, kept = 0, []
for path in CANDIDATES:
    if not lib.does_asset_exist(path):
        continue
    refs = [r for r in lib.find_package_referencers_for_asset(path, False) if not r.startswith("/Game/Audio/Placeholder")]
    if refs:
        kept.append(f"{path} <- {refs}")
        continue
    if lib.delete_asset(path):
        deleted += 1
unreal.log(f"[BL_Cleanup] borrados {deleted}; conservados {len(kept)}: {kept}")
