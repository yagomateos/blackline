"""Captura una imagen del viewport del editor desde el render (no desde la pantalla).

Uso:
  UnrealEditor.exe Blackline.uproject <mapa> -ExecutePythonScript="Tools/UnrealPython/editor_screenshot.py"
Variables de entorno opcionales:
  BL_SHOT_LOC="x,y,z"  BL_SHOT_ROT="pitch,yaw,roll"  BL_SHOT_NAME="nombre"
Por defecto usa la posición/rotación del primer PlayerStart.
La imagen se guarda en <Proyecto>/Saved/Screenshots/BL/<nombre>.png
"""
import os
import unreal

WAIT_FRAMES = 240  # deja converger Lumen, streaming y auto-exposición
state = {"frames": 0, "handle": None, "shot": False}

name = os.environ.get("BL_SHOT_NAME", "shot")
out_dir = os.path.join(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_saved_dir()),
                       "Screenshots", "BL")
os.makedirs(out_dir, exist_ok=True)
out_path = os.path.join(out_dir, name + ".png")


def setup_camera():
    loc_env, rot_env = os.environ.get("BL_SHOT_LOC"), os.environ.get("BL_SHOT_ROT")
    if loc_env:
        x, y, z = (float(v) for v in loc_env.split(","))
        p, yw, r = (float(v) for v in (rot_env or "0,0,0").split(","))
        loc, rot = unreal.Vector(x, y, z), unreal.Rotator(roll=r, pitch=p, yaw=yw)
    else:
        actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors()
        starts = [a for a in actors if isinstance(a, unreal.PlayerStart)]
        if not starts:
            raise RuntimeError("No hay PlayerStart y no se indicó BL_SHOT_LOC")
        loc = starts[0].get_actor_location() + unreal.Vector(0, 0, 60)
        rot = starts[0].get_actor_rotation()
    unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).set_level_viewport_camera_info(loc, rot)
    # Viewport en modo juego: sin iconos ni rejilla del editor
    unreal.SystemLibrary.execute_console_command(None, "ShowFlag.Game 1")


def tick(dt):
    state["frames"] += 1
    if state["frames"] == 5:
        setup_camera()
    elif state["frames"] == WAIT_FRAMES and not state["shot"]:
        state["shot"] = True
        unreal.AutomationLibrary.take_high_res_screenshot(1600, 900, out_path)
        unreal.log(f"[BL_Shot] Captura solicitada: {out_path}")
    elif state["frames"] >= WAIT_FRAMES + 120:
        unreal.unregister_slate_post_tick_callback(state["handle"])
        unreal.SystemLibrary.quit_editor()


state["handle"] = unreal.register_slate_post_tick_callback(tick)
