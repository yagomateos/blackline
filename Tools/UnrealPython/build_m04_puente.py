"""Construye /Game/Maps/M04/L_M04_FuegoCruzado: misión 4 "Fuego cruzado" (puente del ferrocarril, 16:40). Ver
Docs/Mision4_FuegoCruzado.md.

Trazado (cm; +X = este, hacia la Columna). El río corre de norte a sur entre x 0 y 9000, 9 m por debajo del tablero:
  Orilla oeste  x -7000..0 (gobierno): camiones (x -6300), trinchera (parapetos de tierra) hasta el puesto de mando
                (x -3500, antena de satélite), búnker de la cabeza de puente (x -800..-200) con la ametralladora,
                posiciones del ejército y el observador con el designador (x -600, y -1800).
  Puente        tres tramos de celosía de 30 m (centros x 1500, 4500, 7500) sobre dos pilas (x 3000, 6000).
                El primero (ABLScriptedMover con caída física) es el que se vuela.
  Orilla este   x 9000..14000 (Columna): ruinas industriales; de aquí salen las oleadas y el BTR.
  Vado          al sur (y -3600): por ahí cruzan los últimos.
"""
import os
import sys

import unreal

sys.path.insert(0, os.path.join(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir()), "Tools", "UnrealPython"))
import bl_levelkit as K
from bl_levelkit import box, prop, marker, radio, enemy, wave, objective_def, REACH, USE, USE_ALL, DEFEND

MAP_PATH = "/Game/Maps/M04/L_M04_FuegoCruzado"
BR = "/Game/Environment/Bridge/"
lc = unreal.LinearColor
RIVER_Z = -800.0
Y0, Y1 = -4000, 4000


def br(mesh, x, y, yaw=0.0, z=0.0, label=None, scale=None):
    return prop(mesh, x, y, yaw, z, label, path=BR, scale=scale)


def berm(name, x0, x1, y0, y1, h=140):
    """Parapeto de tierra (trinchera): cobertura alta para avanzar agachado o de pie."""
    box(name, x0, x1, y0, y1, 0, h, K.M_DIRT)


def terrain():
    # (al sur, y < -3200, el suelo deja sitio a las rampas del vado)
    box("Suelo_Oeste", -7000, 0, -3200, Y1, -2, 0, K.M_DIRT)
    box("Suelo_Oeste_Sur", -7000, -1680, Y0, -3200, -2, 0, K.M_DIRT)
    box("Suelo_Este", 9000, 14000, -3200, Y1, -2, 0, K.M_DIRT)
    box("Suelo_Este_Sur", 10680, 14000, Y0, -3200, -2, 0, K.M_DIRT)
    box("Fondo_Rio", -2000, 16000, -20000, 20000, RIVER_Z - 400, RIVER_Z - 350, K.M_DIRT)
    w = box("Rio", -1000, 10000, -20000, 20000, RIVER_Z - 20, RIVER_Z, K.mat("M_Env_Water"))
    w.static_mesh_component.set_collision_profile_name("NoCollision")
    w.static_mesh_component.set_editor_property("can_ever_affect_navigation", False)
    for name, x0, x1 in (("Talud_O", -80, 0), ("Talud_E", 9000, 9080)):
        box(name, x0, x1, -3200, Y1, RIVER_Z - 400, 0, K.M_WALL)      # el vado queda abierto
    # Bordes: solo se pasa por el puente (y -310..310) y por el vado del sur
    for name, x0, x1 in (("O", -80, 0), ("E", 9000, 9080)):
        K.hidden_blocker(f"Borde_{name}_N", x0, x1, 310, Y1, 0, 900)
        K.hidden_blocker(f"Borde_{name}_S", x0, x1, -3200, -310, 0, 900)
    K.hidden_blocker("Limite_N", -7000, 14000, Y1, Y1 + 60, 0, 1500)
    K.hidden_blocker("Limite_S", -7000, 14000, Y0 - 60, Y0, 0, 1500)
    K.hidden_blocker("Limite_O", -7060, -7000, Y0, Y1, 0, 1500)
    K.hidden_blocker("Limite_E", 14000, 14060, Y0, Y1, 0, 1500)
    # Vado (al sur): una lengua de grava que cruza el río a ras de agua
    box("Vado", -80, 9080, Y0, -3200, RIVER_Z, RIVER_Z + 30, K.M_DIRT)
    K.stair_flight_x("Vado_RampaO", Y0, -3200, -80, -1, RIVER_Z + 30, steps=40, rise=(-RIVER_Z - 30) / 40, run=40.0)
    K.stair_flight_x("Vado_RampaE", Y0, -3200, 9080, 1, RIVER_Z + 30, steps=40, rise=(-RIVER_Z - 30) / 40, run=40.0)


def bridge():
    # Pilas y tramos 2 y 3 fijos; el tramo 1 cae al volarlo
    for i, x in enumerate((3000, 6000)):
        br("SM_RailBridge_Pier", x, 0, 0, label=f"Pila_{i}")
    for i, x in enumerate((4500, 7500)):
        br("SM_RailBridge_Span", x, 0, 0, label=f"Tramo_{i + 2}")
    K.mover("Tramo_1", 1500, 0, 0, mesh=BR + "SM_RailBridge_Span", tag="BLBridge",
            start_sound="/Game/Audio/Weapons/Grenade/SW_Grenade_Explosion_02")
    span = [a for a in unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors() if a.get_actor_label() == "Tramo_1"][0]
    span.set_editor_property("physics_fall", True)
    span.set_editor_property("blast_points", [unreal.Vector(400, -280, 50), unreal.Vector(2600, 280, 50), unreal.Vector(1500, 0, 50)])
    # Cargas en las vigas del primer tramo (objetivo) y el detonador del búnker
    for i, (x, y) in enumerate(((400, -280), (2600, 280))):
        K.interactable(f"Carga_{i}", "BLObjective_Carga", x, y, 70, "Colocar la carga", hold=2.5, mesh="/Game/Environment/OldTown/SM_BreachCharge")
    # Restos en el puente: coche calcinado y un vagón volcado como cobertura de los que cruzan
    prop("SM_Veh_Sedan_Wreck", 5200, -120, 8, label="Coche_Puente")
    box("Vagon_Volcado", 7000, 7800, 80, 300, 30, 300, K.M_RUST)
    K.covers_for_box(7000, 7800, 80, 300, False)


def west_bank():
    # Camiones de la retirada y punto de partida
    for i, (x, y, yaw) in enumerate(((-6300, -1500, 0), (-6300, -700, 4))):
        br("SM_Truck_Army", x, y, yaw, label=f"Camion_{i}")
    K.player_start(-5600, 1500, 0)
    K.start_point("Inicio", -5600, 1500, 0)
    # Trinchera: dos parapetos que forman el pasillo de los camiones al puesto y del puesto al búnker
    for i, (x0, x1) in enumerate(((-6000, -4200), (-3200, -1100))):
        berm(f"Trinchera_N_{i}", x0, x1, 1700, 1820)
        berm(f"Trinchera_S_{i}", x0, x1, 1180, 1300)
        K.covers_for_box(x0, x1, 1700, 1820, False)
    # Puesto de mando: tienda, antena de satélite (el enlace) y mesa de radio
    box("Tienda", -3900, -3300, 1950, 2600, 0, 280, unreal.load_asset(BR + "MI_Army_Canvas") or K.M_PLASTER)
    br("SM_SatDish", -3600, 3000, 200, label="Antena_Satelite")
    box("Mesa_Radio", -3700, -3450, 1400, 1550, 0, 80, K.M_WOOD)
    # Búnker de la cabeza de puente: muros de sacos/hormigón, tronera al puente, techo, entrada por detrás
    box("Bunker_Suelo", -800, -200, -500, 500, 0, 3, K.M_FLOOR)
    box("Bunker_MuroFrente_Bajo", -230, -200, -500, 500, 0, 110, K.M_WALL)
    box("Bunker_MuroFrente_Alto", -230, -200, -500, 500, 175, 230, K.M_WALL)
    for s, (y0, y1) in (("N", (-500, -470)), ("S", (470, 500))):
        box(f"Bunker_Muro{s}", -800, -200, y0, y1, 0, 230, K.M_WALL)
    box("Bunker_MuroAtras_N", -800, -770, -500, -110, 0, 230, K.M_WALL)
    box("Bunker_MuroAtras_S", -800, -770, 110, 500, 0, 230, K.M_WALL)
    box("Bunker_Techo", -830, -170, -530, 530, 230, 270, K.M_WALL)
    for i, y in enumerate((-620, 620)):
        prop("SM_Cover_Sandbag_Straight", -400, y, 0, label=f"Bunker_Sacos_{i}")
    mg = unreal.get_editor_subsystem(unreal.EditorActorSubsystem).spawn_actor_from_class(
        unreal.BLMountedGun, unreal.Vector(-330, 0, 0), unreal.Rotator(roll=0, pitch=0, yaw=0))
    mg.set_actor_label("Ametralladora")
    mg.tags = [unreal.Name("BLObjective_MG")]
    K.interactable("Detonador", "BLObjective_Detonador", -720, -380, 95, "Volar el puente", hold=1.5,
                   mesh="/Game/Environment/Props/SM_Obj_Laptop_Rugged")
    box("Detonador_Caja", -760, -680, -420, -340, 0, 90, K.M_WOOD)
    # Posiciones del ejército a lo largo de la orilla y el observador con el designador
    for i, (x, y, yaw) in enumerate(((-350, -1300, 0), (-350, 1300, 0), (-500, -2300, 10), (-450, 2300, -10))):
        prop("SM_Cover_Sandbag_Corner", x, y, yaw, label=f"Puesto_{i}")
    prop("SM_Cover_Sandbag_Corner", -650, -1800, 20, label="Observatorio")
    des = unreal.get_editor_subsystem(unreal.EditorActorSubsystem).spawn_actor_from_class(
        unreal.BLStrikeDesignator, unreal.Vector(-700, -1900, 0), unreal.Rotator(roll=0, pitch=0, yaw=5))
    des.set_actor_label("Designador")
    des.get_component_by_class(unreal.StaticMeshComponent).set_static_mesh(unreal.load_asset(BR + "SM_Designator"))
    des.set_editor_property("mark_radio", [radio("M04_Marcado")])
    des.set_editor_property("strike_delay", 2.6)     # la pasada coincide con los cazas sobre el blindado
    des.tags = [unreal.Name("BLObjective_Designador")]
    # Restos y atrezo de batalla
    for i, (x, y) in enumerate(((-2500, -900), (-1800, 600), (-4800, -2200))):
        prop("SM_Prop_Rubble_B", x, y, 40 * i)
    for i, (x, y) in enumerate(((-2600, 2400), (-5000, 400))):
        prop("SM_Prop_Drum", x, y, 0)
    marker("BLObj_Puesto", -900, 900)
    marker("BLObj_Bunker", -500, 0)
    marker("BLObj_Camiones", -6200, -1100)
    K.start_point("Puesto", -900, 1000, 0)
    K.start_point("Ametralladora", -600, 200, 0)
    K.start_point("Blindado", -700, -1700, 0)
    K.start_point("Voladura", -150, -100, 0)
    K.start_point("Retirada", -1500, -1500, 180)
    K.checkpoint("Puesto", -1300, -800, 300, 1700, 0)
    K.checkpoint("Bunker", -760, -240, -450, 450, 0)


def east_bank():
    for i, (x0, x1, y0, y1, h, wall, seed) in enumerate(((9500, 11500, 1200, 4000, 960, "Concrete", 4101), (12000, 14000, 800, 4000, 1280, "Grey", 4102),
                                                        (9500, 11000, -4000, -1200, 640, "Concrete", 4103), (11800, 14000, -4000, -800, 900, "Cream", 4104))):
        K.facade_building(f"Ruina_{i}", x0, x1, y0, y1, h, wall, 15, 15, seed=seed)
    for x, y in ((9800, -200), (9800, 200), (10300, 0), (10600, -400)):
        marker("BLWave_Puente", x, y)
    for x, y in ((-2500, -3600), (-3000, -3500)):       # ya han subido la rampa del vado
        marker("BLWave_Vado", x, y)
    btr = unreal.get_editor_subsystem(unreal.EditorActorSubsystem).spawn_actor_from_class(
        unreal.BLBTR, unreal.Vector(12500, 0, 30), unreal.Rotator(roll=0, pitch=0, yaw=180))
    btr.set_actor_label("Blindado")
    btr.set_editor_property("path", [unreal.Vector(12500, 0, 30), unreal.Vector(9500, 0, 30), unreal.Vector(6800, 0, 30)])
    btr.set_editor_property("scripted_shots", 2)
    btr.set_editor_property("collapse_shot", 0)
    btr.set_editor_property("suppress_max_player_x", 20000.0)
    btr.tags = [unreal.Name("BLBTR")]
    for x, y, z in ((-210, -300, 200), (-210, 300, 200), (-350, -1300, 120)):
        marker("BLBTR_Target", x, y, z)
    # Los dos cazas: escondidos hasta que el designador los llama; pasan de sur a norte sobre el blindado
    for i, dx in enumerate((0, 600)):
        K.mover(f"Caza_{i}", 6500 + dx, -20000 - dx * 4, 4200 + dx, yaw=90, mesh=BR + "SM_Jet",
                path=[(6500 + dx, -12000, 2600 + dx), (6500 + dx, 0, 2200 + dx), (6500 + dx, 45000, 5000)],
                speed=12000.0, accel=20000.0, turn=60.0, hidden=True, hide_at_end=True, tag="BLJets",
                start_sound="/Game/Audio/Vehicles/SW_Jet_Flyby" if i == 0 else None)
    # Humo y fuego de la batalla en la orilla este
    K.smoke("Humo_Este_0", 11000, 2500, 0, color=lc(0.08, 0.075, 0.07, 1), opacity=0.55)
    K.smoke("Humo_Este_1", 13000, -2000, 0, color=lc(0.12, 0.11, 0.1, 1), opacity=0.5)
    K.smoke("Fuego_Ruina", 10200, -1100, 0, fire=True, fire_extent=unreal.Vector(160, 60, 30), flame_size=80.0, light_intensity=9000.0,
            max_particles=22, life=9.0, start_size=unreal.Vector2D(80, 120), end_size=unreal.Vector2D(500, 800), rise_speed=150.0,
            spawn_radius=80.0, color=lc(0.07, 0.065, 0.06, 1), opacity=0.6)


def enemies_and_allies():
    # Ejército de Varania (aliados): no se mueven de su puesto
    for i, (x, y, yaw) in enumerate(((-300, -1300, 0), (-300, 1300, 0), (-450, -2300, 10), (-400, 2300, -10), (-280, -380, 0), (-280, 380, 0))):
        enemy(f"Soldado_{i}", "Ejercito", x, y, yaw, role="ally")
    # Mortero sobre la retaguardia (toda la misión) y sobre el primer tramo (mientras se ponen las cargas)
    A = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    m = A.spawn_actor_from_class(unreal.BLMortarBarrage, unreal.Vector(-3500, 500, 0))
    m.set_actor_label("Mortero_Retaguardia")
    m.set_editor_property("extent", unreal.Vector(2800, 3000, 0))
    m.tags = [unreal.Name("BLMortar"), unreal.Name("BLMortar_Stop")]
    mb = A.spawn_actor_from_class(unreal.BLMortarBarrage, unreal.Vector(1500, 0, 0))
    mb.set_actor_label("Mortero_Puente")
    mb.set_editor_property("extent", unreal.Vector(1800, 250, 0))
    mb.set_editor_property("interval", unreal.Vector2D(2.0, 3.5))
    mb.set_editor_property("near_player_chance", 0.6)
    mb.tags = [unreal.Name("BLMortarBridge"), unreal.Name("BLMortarBridge_Stop")]


def mission():
    objectives = [
        objective_def("Llega al puesto del puente por la trinchera", REACH, "BLObj_Puesto", radius=500.0, activate=["BLMortar"],
                      on_start=[radio("M04_Mortero")]),
        objective_def("Defiende la cabeza de puente", DEFEND, "BLObj_Bunker", squad="Oleada", min_duration=55.0, progress="enlace",
                      on_start=[radio("M04_Defensa"), radio("M04_Transmision")],
                      waves=[wave(["BLWave_Puente"], 4, 4.0, True), wave(["BLWave_Puente"], 4, 8.0), wave(["BLWave_Puente"], 5, 8.0, True, gunners=1)]),
        objective_def("Toma la ametralladora del búnker", USE, "BLObjective_MG", on_start=[radio("M04_Ametralladora")]),
        objective_def("Detén la carga sobre el puente", DEFEND, "BLObj_Bunker", squad="Carga", min_duration=40.0,
                      on_start=[radio("M04_Carga")], waves=[wave(["BLWave_Puente"], 6, 3.0, True), wave(["BLWave_Puente"], 7, 6.0)]),
        objective_def("Marca el blindado para la aviación", USE, "BLObjective_Designador", activate=["BLBTR"],
                      on_start=[radio("M04_Blindado"), radio("M04_BlindadoTorre")]),
        objective_def("Coloca las cargas en el primer tramo del puente", USE_ALL, "BLObjective_Carga", activate=["BLMortarBridge"],
                      on_start=[radio("M04_Cargas")], on_complete=[radio("M04_Detonador")]),
        objective_def("Vuela el puente", USE, "BLObjective_Detonador", activate=["BLMortarBridge_Stop"]),
        objective_def("Aguanta hasta que termine la transmisión", DEFEND, "BLObj_Bunker", squad="Vado", min_duration=45.0,
                      progress="transmisión", activate=["BLBridge"], on_start=[radio("M04_Voladura"), radio("M04_Vado")],
                      on_complete=[radio("M04_Final")],
                      waves=[wave(["BLWave_Vado"], 4, 6.0, True), wave(["BLWave_Vado"], 4, 8.0, False, gunners=1)]),
        objective_def("Retírate a los camiones", REACH, "BLObj_Camiones", radius=500.0, activate=["BLMortar_Stop"]),
    ]
    K.director("FUEGO CRUZADO", objectives,
               briefing=[radio("M04_Brief_01"), radio("M04_Brief_02")], debriefing=[radio("M04_Debrief")],
               phases={"Puesto": 1, "Ametralladora": 2, "Blindado": 4, "Voladura": 5, "Retirada": 7})


def build():
    terrain()
    bridge()
    west_bank()
    east_bank()
    enemies_and_allies()
    mission()
    # Tarde gris con humo: sol alto y velado, niebla ligera
    K.dawn_fog_lighting(sun_yaw=200.0, sun_pitch=-28.0, sun_intensity=1.8, fog_density=0.035, fog_falloff=0.2,
                        fog_color=(0.5, 0.5, 0.48), volumetric=False)


K.run(MAP_PATH, build, nav_bounds=(-7000, 14000, Y0, Y1, RIVER_Z - 100, 900),
      nav_checks=[("Inicio", (-5600, 1500, 100)), ("Trinchera", (-2200, 1500, 100)), ("Puesto", (-900, 900, 100)),
                  ("Bunker", (-500, 0, 100)), ("Observatorio", (-600, -1700, 100)), ("Tramo1", (1500, 0, 100)),
                  ("Tramo3", (7500, 0, 100)), ("OrillaEste", (10300, 0, 100)), ("Vado", (4500, -3600, RIVER_Z + 130)),
                  ("SalidaVado", (-2500, -3600, 100)), ("Camiones", (-6200, -1100, 100))],
      tag="BL_M04")
