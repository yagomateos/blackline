"""Crea M_Env_Triplanar_AT (suelos sin repetición visible, ver setup_audio_fx.py) y pasa a él todas las instancias
que colgaban de M_Env_Triplanar (suelos, paredes, contenedores y las de las misiones 2-5). No reimporta nada.
Uso: UnrealEditor-Cmd.exe Blackline.uproject -run=pythonscript -script=".../update_env_antitile.py"
"""
import os
import unreal

HERE = os.path.dirname(os.path.abspath(__file__))
src = open(os.path.join(HERE, "setup_audio_fx.py"), encoding="utf-8").read()
src = src[:src.rindex("\nmain()")]   # solo las funciones, sin ejecutar el script completo
ns = {"__file__": os.path.join(HERE, "setup_audio_fx.py"), "__name__": "bl_setup_audio_fx"}
exec(compile(src, "setup_audio_fx.py", "exec"), ns)

lib = unreal.EditorAssetLibrary
tex = {"concrete_wall_008_diff_2k": None, "concrete_wall_008_nor_dx_2k": None, "concrete_wall_008_arm_2k": None}
for k in list(tex):
    tex[k] = unreal.load_asset(f"/Game/Environment/Textures/{k}")
master = ns["make_env_master"](tex)
old = "/Game/Environment/Materials/M_Env_Triplanar"
moved = 0
reg = unreal.AssetRegistryHelpers.get_asset_registry()
for data in reg.get_assets_by_class(unreal.TopLevelAssetPath("/Script/Engine", "MaterialInstanceConstant")):
    path = str(data.package_name)
    if not path.startswith("/Game/"):
        continue
    mi = unreal.load_asset(path)
    parent = mi.get_editor_property("parent") if mi else None
    if parent and parent.get_path_name().split(".")[0] == old:
        mi.set_editor_property("parent", master)
        lib.save_loaded_asset(mi)
        moved += 1
unreal.log_warning(f"[BL_AT] {master.get_name()} listo; instancias cambiadas: {moved}")
