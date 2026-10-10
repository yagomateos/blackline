"""Misión 2 (2026-10-10): la lancha de extracción se puede alcanzar y pilotar.

- Atraca pegada al embarcadero (y 390; antes y 520) y el punto de "Subir a la lancha" queda en el borde del
  embarcadero a su altura: antes quedaba a ~250 cm de la cámara y el alcance de interacción es 210 cm (bloqueo).
- Etiqueta BLBoat_Drive y objetivo nuevo "Sal de la dársena con la lancha" (Reach a BLObj_Salida): el jugador la
  pilota (W/S, A/D) hasta la bocana. Idempotente.
Uso: UnrealEditor-Cmd.exe Blackline.uproject -run=pythonscript -script=".../patch_m02_boat_drive.py"
"""
import os
import sys

import unreal

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import bl_levelkit as K  # noqa: E402

MAP = "/Game/Maps/M02/L_M02_Manifiesto"
WATER_Z = -120.0
DOCK_Y = 390.0
EXIT = (29300.0, 8200.0)

les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
les.load_level(MAP)
actors = eas.get_all_level_actors()

boat = next(a for a in actors if a.get_actor_label() == "Lancha_Extraccion")
boat.set_editor_property("path", [unreal.Vector(26000, 6000, WATER_Z), unreal.Vector(24600, 1400, WATER_Z),
                                  unreal.Vector(23600, DOCK_Y, WATER_Z), unreal.Vector(22400, DOCK_Y, WATER_Z)])
# Atracada con la proa a -X: +Y local es -Y del mundo -> y 390 - 175 = 215 (borde del embarcadero), z -120 + 140 = 20
boat.set_editor_property("board_offset", unreal.Vector(-60, 175, 140))
boat.tags = [unreal.Name("BLBoat"), unreal.Name("BLBoat_Board"), unreal.Name("BLBoat_Drive")]

exit_marker = next((a for a in actors if a.get_actor_label() == "BLObj_Salida"), None)
if exit_marker:
    exit_marker.set_actor_location(unreal.Vector(EXIT[0], EXIT[1], WATER_Z + 100), False, False)
else:
    K.marker("BLObj_Salida", EXIT[0], EXIT[1], WATER_Z + 100)

director = next(a for a in actors if isinstance(a, unreal.BLMissionDirector))
objs = list(director.get_editor_property("objectives"))
if not any(str(o.get_editor_property("target_tag")) == "BLObj_Salida" for o in objs):
    objs.append(K.objective_def("Sal de la dársena con la lancha", K.REACH, "BLObj_Salida", radius=700.0,
                                activate=["BLBoat_Drive"]))
    director.set_editor_property("objectives", objs)

les.save_current_level()
unreal.log_warning(f"[BL_M02] lancha atracada en y {DOCK_Y}, {len(objs)} objetivos, destino {EXIT}")
