"""Crea /Game/Maps/Menu/L_MainMenu (Bloque 10): nivel vacío con ABLMenuGameMode; todo el menú es Slate (C++).

Ejecutar: UnrealEditor.exe Blackline.uproject -ExecCmds="py <ruta>/Tools/UnrealPython/build_menu_map.py" -unattended
"""
import unreal

MAP = "/Game/Maps/Menu/L_MainMenu"
level_ed = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)

if unreal.EditorAssetLibrary.does_asset_exist(MAP):
    level_ed.load_level(MAP)
else:
    level_ed.new_level(MAP)
world = unreal.EditorLevelLibrary.get_editor_world()
old = actors.get_all_level_actors()
if old:
    actors.destroy_actors(old)
world.get_world_settings().set_editor_property("default_game_mode", unreal.BLMenuGameMode)
level_ed.save_current_level()
unreal.log("[BL_Menu] Mapa del menú guardado")
unreal.SystemLibrary.quit_editor()
