"""Construye /Game/Maps/M05/L_M05_LineaNegra: misión 5 "Línea negra" (terminal de contenedores y capitanía, 06:30).
Ver Docs/Mision5_LineaNegra.md.

Trazado (cm; +X = este). El muelle y el agua quedan al norte (y < -5000):
  Entrada     x 0..1000: valla del recinto con el hueco por el que entra el equipo.
  Terminal    x 1000..9000: filas de contenedores apilados, grúas STS en el muelle, coches de transporte.
  Patio       x 9000..10500: explanada delante de la capitanía (de aquí vienen refuerzos).
  Capitanía   x 10500..14500, y -2000..2000: 3 plantas (vestíbulo; sala de servidores; despachos) + azotea con
              helipuerto. Escalera de ida y vuelta en la esquina noreste (x 14000..14475, y 1025..1975).
"""
import os
import sys

import unreal

sys.path.insert(0, os.path.join(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir()), "Tools", "UnrealPython"))
import bl_levelkit as K
from bl_levelkit import box, prop, marker, radio, enemy, wave, objective_def, wall_x, wall_y, REACH, USE, USE_ALL, CLEAR, DEFEND

MAP_PATH = "/Game/Maps/M05/L_M05_LineaNegra"
TERM = "/Game/Environment/Terminal/"
lc = unreal.LinearColor
Y0, Y1 = -5000, 5000
FH, WT = 320.0, 25.0
WX0, WX1, WY0, WY1 = 10500, 14500, -2000, 2000
SX0, SX1, SY0, SY1 = 14000, WX1 - WT, 1025, WY1 - WT       # hueco de la escalera
A = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)


def term(mesh, x, y, yaw=0.0, z=0.0, label=None, scale=None):
    return prop(mesh, x, y, yaw, z, label, path=TERM, scale=scale)


# ---------------------------------------------------------------------------
# Terreno, muelle y terminal de contenedores
# ---------------------------------------------------------------------------
def terminal():
    box("Suelo", 0, 16000, Y0, Y1, -2, 0, K.M_ASPHALT)
    w = box("Agua", -4000, 30000, -20000, Y0 - 60, -380, -360, K.mat("M_Env_Water"))
    w.static_mesh_component.set_collision_profile_name("NoCollision")
    w.static_mesh_component.set_editor_property("can_ever_affect_navigation", False)
    box("Muelle_Muro", 0, 16000, Y0 - 60, Y0, -700, 0, K.M_WALL)
    K.hidden_blocker("Borde_Muelle", 0, 16000, Y0 - 60, Y0, 0, 900)
    K.hidden_blocker("Limite_S", 0, 16000, Y1, Y1 + 60, 0, 1200)
    K.hidden_blocker("Limite_E", 16000, 16060, Y0, Y1, 0, 1200)
    # Valla del recinto (oeste) con el hueco cortado por el que entra el equipo
    for i, y in enumerate(range(Y0, Y1, 400)):
        if -200 <= y <= 200:
            continue
        prop("SM_Ref_Fence", 300, y, 90, label=f"Valla_{i}", path="/Game/Environment/Refinery/")
    K.hidden_blocker("Limite_O", -60, 0, Y0, Y1, 0, 1200)
    # Filas de contenedores (apilados de 1 a 3) con pasillos norte-sur cada pocas filas
    rows = {-3300: (1500, 9000), -1800: (1800, 8600), 1800: (1500, 8800), 3300: (2000, 9000)}
    n = 0
    for y, (x0, x1) in rows.items():
        x = x0
        while x < x1:
            if K.rng.random() < 0.15:         # hueco: paso entre filas
                x += 900
                continue
            for level in range(K.rng.choice((1, 1, 2, 2, 3))):
                prop("SM_Container_20ft", x, y + K.rng.uniform(-20, 20), K.rng.uniform(-2, 2), z=level * 259,
                     label=f"Contenedor_{n}", material=K.rng.choice(K.CONTAINER_MATS))
                n += 1
            x += 650
    # Grúas STS en el muelle (la pluma sobre el agua, hacia -Y)
    # (centradas en y -3900: más al norte, una pata caía en el agua)
    for i, x in enumerate((3500, 7200)):
        term("SM_GantryCrane", x, -3900, -90, label=f"Grua_STS_{i}")
    # Coches de transporte (straddle carriers) parados: patas y puente, cobertura alta
    for i, (x, y) in enumerate(((4300, 0), (7600, -400))):
        for dx in (-250, 250):
            for dy in (-180, 180):
                box(f"Straddle_{i}_Pata{dx}{dy}", x + dx - 30, x + dx + 30, y + dy - 30, y + dy + 30, 0, 1000, K.mat("MI_Env_RustyMetal"))
        box(f"Straddle_{i}_Puente", x - 300, x + 300, y - 220, y + 220, 1000, 1150, K.mat("MI_Env_RustyMetal"))
        box(f"Straddle_{i}_Cabina", x + 150, x + 300, y - 200, y - 20, 1150, 1350, K.M_METAL)
    for i, (x, y) in enumerate(((2400, 600), (5800, -700), (8200, 900))):
        prop("SM_Cover_Jersey", x, y, K.rng.uniform(0, 180), label=f"Jersey_{i}")
    for i, (x, y) in enumerate(((1500, 0), (5000, 3000), (8600, -3000), (9800, 0))):
        K.sodium_light(f"Farola_{i}", x, y + 200, 90, intensity=6.0)
    K.player_start(500, 0, 0)
    K.start_point("Inicio", 500, 0, 0)
    marker("BLObj_Terminal", 1500, 0)
    marker("BLObj_Entrada", 6000, 0)
    K.start_point("Terminal", 1500, 0, 0)
    K.checkpoint("Terminal", 300, 1300, -1000, 1000, 0)


# ---------------------------------------------------------------------------
# Capitanía (cuartel de Corvane): 3 plantas + azotea con helipuerto
# ---------------------------------------------------------------------------
def capitania():
    W = K.mat("MI_Fac_Wall_Concrete")
    IN = K.M_PLASTER
    for k in range(3):
        z, zt = k * FH, (k + 1) * FH
        win = lambda c, w=180: (c - w / 2, c + w / 2, z + 90, z + 240)
        west = [win(c) for c in (-1500, -800, 800, 1500)] + ([(-85, 85, 0, 235)] if k == 0 else [win(0)])
        wall_y(f"Cap_O{k}", WX0, WX0 + WT, WY0 + WT, WY1 - WT, z, zt, west, W)
        wall_y(f"Cap_E{k}", WX1 - WT, WX1, WY0 + WT, WY1 - WT, z, zt, [win(-1500), win(-500)], W)
        wall_x(f"Cap_N{k}", WY0, WY0 + WT, WX0, WX1, z, zt, [win(c) for c in (11200, 12300, 13400)] + ([(12900, 13070, 0, 235)] if k == 0 else []), W)
        wall_x(f"Cap_S{k}", WY1 - WT, WY1, WX0, WX1, z, zt, [win(c) for c in (11200, 12300, 13300)], W)
        # Escalera: tramo A hacia +Y, rellano, tramo B de vuelta (la llegada queda al sur del hueco)
        K.stair_flight(f"Cap_EscA{k}", SX0 + 10, 14235, SY0, 1, z)
        box(f"Cap_Rellano{k}", SX0 + 10, SX1 - 10, SY0 + 252, SY1 - 10, z + 140, z + 160, K.M_FLOOR)
        K.stair_flight(f"Cap_EscB{k}", 14255, SX1 - 10, SY0 + 252, -1, z + 160)
        wall_y(f"Cap_EscMuro{k}", SX0 - WT, SX0, SY0 + 200, SY1, z, zt, [], IN)
        # Forjado de la planta de arriba (con el hueco de la escalera)
        box(f"Cap_Forjado{k + 1}_S", WX0 + WT, WX1 - WT, WY0 + WT, SY0, zt - 20, zt, K.M_FLOOR)
        box(f"Cap_Forjado{k + 1}_N", WX0 + WT, SX0, SY0, WY1 - WT, zt - 20, zt, K.M_FLOOR)
        # Luces frías de oficina (tenues: Corvane ha cortado parte de la corriente)
        for i, (lx, ly) in enumerate(((11300, -1200), (12700, 600), (11300, 1300), (13600, -1000))):
            if (k + i) % 3 != 2:
                K.point_light(f"Cap_Luz{k}_{i}", lx, ly, zt - 40, (0.82, 0.92, 1.0), 16.0, 750.0, draw_distance=3000.0)
    box("Cap_Suelo", WX0, WX1, WY0, WY1, 0, 3, K.M_FLOOR)
    # Planta baja: vestíbulo con mostrador y la puerta principal (oeste)
    box("Cap_Mostrador", 11400, 11600, -700, 700, 0, 110, K.M_WOOD)
    K.covers_for_box(11400, 11600, -700, 700, True)
    K.door_with_frame("Cap_PuertaPrincipal", WX0 + 12, -75, 0)
    for i, (x, y, yaw) in enumerate(((12500, 1300, 0), (13300, -1300, 180))):
        term("SM_OfficeDesk", x, y, yaw, label=f"Cap_Mesa_PB{i}")
    # Primera planta: sala de servidores (sureste) con su puerta y las dos cargas de termita
    wall_y("Cap_Servidores_O", 12500, 12525, WY0 + WT, 0, FH, 2 * FH, [(-800, -630, FH, FH + 230)], IN)
    wall_x("Cap_Servidores_N", 0, 25, 12525, WX1 - WT, FH, 2 * FH, [], IN)
    K.door_with_frame("Cap_PuertaServidores", 12512, -790, 0)
    for r, y in enumerate((-1600, -900)):
        for c, x in enumerate(range(12900, 14300, 280)):
            term("SM_ServerRack", x, y, 0, z=FH, label=f"Rack_{r}_{c}")
    for i, (x, y) in enumerate(((13180, -1600), (13740, -900))):
        term("SM_Thermite", x, y, 0, z=FH + 210, label=f"Termita_{i}")
        K.interactable(f"Desactivar_Termita_{i}", "BLObjective_Termita", x, y - 70, FH + 140, "Desactivar la carga de termita", hold=2.5)
    K.ambient("SW_AmbZ_Hum_Loop", 13500, -1200, FH + 150, 1.0, "Amb_Servidores")
    K.point_light("Cap_Servidores_Luz", 13500, -1250, 2 * FH - 50, (0.4, 1.0, 0.6), 10.0, 900.0, draw_distance=3000.0)
    # Segunda planta: despachos; el del inglés al noroeste, con su puerta
    wall_x("Cap_Despacho_S", 1000, 1025, WX0 + WT, 12000, 2 * FH, 3 * FH, [(11115, 11285, 2 * FH, 2 * FH + 230)], IN)
    wall_y("Cap_Despacho_E", 12000, 12025, 1025, WY1 - WT, 2 * FH, 3 * FH, [], IN)
    K.door_with_frame("Cap_PuertaDespacho", 11125, 1012, -90)
    term("SM_OfficeDesk", 11200, 1600, 0, z=2 * FH, label="Mesa_Ingles")
    for i, (x, y) in enumerate(((12800, -1200), (13600, -1200), (13200, 400))):
        term("SM_OfficeDesk", x, y, 90 * i, z=2 * FH, label=f"Cap_Mesa_P2_{i}")
    # Azotea: peto, caseta de la escalera (puerta al sur), helipuerto
    zr = 3 * FH
    for name, b in (("N", (WX0, WX1, WY0, WY0 + WT)), ("S", (WX0, WX1, WY1 - WT, WY1)), ("O", (WX0, WX0 + WT, WY0, WY1)), ("E", (WX1 - WT, WX1, WY0, WY1))):
        box(f"Cap_Peto{name}", b[0], b[1], b[2], b[3], zr, zr + 105, W)
    cz = zr + 280
    box("Cap_Caseta_O", SX0 - WT, SX0, SY0 - WT, WY1, zr, cz, W)
    wall_x("Cap_Caseta_S", SY0 - WT, SY0, SX0 - WT, WX1, zr, cz, [(14255, 14465, zr, zr + 230)], W)
    box("Cap_Caseta_Techo", SX0 - WT - 10, WX1 + 10, SY0 - WT - 10, WY1 + 10, cz, cz + 20, K.M_WALL)
    box("Cap_Azotea_Hueco", SX0, SX1, SY1 - 10, WY1 - WT, zr - 20, zr, K.M_FLOOR)
    term("SM_Helipad", 11800, -900, 0, z=zr, label="Helipuerto")
    marker("BLObj_Capitania", WX0 - 300, 0)
    marker("BLObj_Servidores", 13000, -1250, FH)
    marker("BLObj_Azotea", 12400, 0, zr)
    K.start_point("Capitania", 9800, 0, 0)
    K.checkpoint("Capitania", 9300, 10300, -1000, 1000, 0)
    K.start_point("Servidores", 12300, -1300, 0, z=FH)
    K.checkpoint("Servidores", 12600, 14400, -1950, -100, 0, z=FH)
    K.start_point("Azotea", 13300, -500, 0, z=FH)


# ---------------------------------------------------------------------------
# Personajes, helicópteros y misión
# ---------------------------------------------------------------------------
def characters():
    # Sable 2-2 y 2-3: siguen al jugador y combaten
    for i, (x, y) in enumerate(((300, -250), (250, 250))):
        e = enemy(f"Sable_2{i + 2}", "Equipo", x, y, 0, role="ally")
        e.set_editor_property("follow_player", True)
    # Terminal: milicianos y dos operadores entre los contenedores
    for i, (x, y, yaw) in enumerate(((3300, -900, 180), (4700, 1000, 190), (5600, -2600, 170), (6300, 500, 180), (7800, 2400, 200))):
        enemy(f"Terminal_{i}", "Terminal", x, y, yaw)
    enemy("Terminal_Op_0", "Terminal", 6900, -1000, 180, role="operator")
    enemy("Terminal_Op_1", "Terminal", 8400, 300, 180, role="operator")
    enemy("Terminal_Patrulla", "Terminal", 2500, 2600, 0, [(2500, 2600), (7500, 2600)])
    # Capitanía: operadores en el vestíbulo y la escalera, milicianos en la primera planta
    enemy("Cap_Vestibulo_0", "Capitania", 11000, -600, 180, role="operator")
    enemy("Cap_Vestibulo_1", "Capitania", 12200, 900, 200, role="operator")
    enemy("Cap_P1_0", "Capitania", 11500, 700, 180, z=FH)
    enemy("Cap_P1_1", "Capitania", 12000, -1500, 90, z=FH)
    # El inglés en su despacho (corre al helipuerto cuando empieza el objetivo de la azotea)
    vip = A.spawn_actor_from_class(unreal.BLVIP, unreal.Vector(11300, 1500, 2 * FH + 96), unreal.Rotator(roll=0, pitch=0, yaw=-90))
    vip.set_actor_label("El_Ingles")
    vip.set_editor_property("patrol_points", [unreal.Vector(12200, -300, 3 * FH)])
    vip.tags = [unreal.Name("BLVIP"), unreal.Name("BLVIPRun")]
    K.interactable("Detener_Ingles", "BLObjective_VIP", 12200, -300, 3 * FH + 90, "Detener a «el inglés»", hold=1.5)
    # Oleadas: por la escalera (desde el vestíbulo) y por el patio
    for x, y in ((11200, -1500), (11800, 1500)):
        marker("BLWave_Vestibulo", x, y)
    for x, y in ((9800, -2600), (9800, 2600)):
        marker("BLWave_Patio", x, y)
    # Helicóptero de Corvane (negro, se puede inutilizar) y el de la coalición para la extracción
    hc = A.spawn_actor_from_class(unreal.BLHelicopter, unreal.Vector(12000, -30000, 4000), unreal.Rotator(roll=0, pitch=0, yaw=90))
    hc.set_actor_label("Heli_Corvane")
    hc.set_editor_property("path", [unreal.Vector(12000, -30000, 4000), unreal.Vector(11900, -9000, 2400)])
    hc.set_editor_property("hover_point", unreal.Vector(11800, -900, 3 * FH + 650))
    hc.set_editor_property("land_point", unreal.Vector(11800, -900, 3 * FH + 20))
    hc.set_editor_property("hover_yaw", 0.0)
    hc.set_editor_property("approach_delay", 4.0)
    hc.set_editor_property("damageable", True)
    hc.set_editor_property("health", 700.0)
    hc.set_editor_property("door_gun", False)
    hc.set_editor_property("interact_tag", "None")
    hc.set_editor_property("body_material", unreal.load_asset("/Game/Vehicles/Heli/MI_Heli_Corvane"))
    hc.tags = [unreal.Name("BLHeliCorvane")]
    h1 = A.spawn_actor_from_class(unreal.BLHelicopter, unreal.Vector(4000, -30000, 4000), unreal.Rotator(roll=0, pitch=0, yaw=90))
    h1.set_actor_label("Halcon_1")
    h1.set_editor_property("path", [unreal.Vector(4000, -30000, 4000), unreal.Vector(10000, -8000, 2600)])
    h1.set_editor_property("hover_point", unreal.Vector(12900, 900, 3 * FH + 700))
    h1.set_editor_property("land_point", unreal.Vector(12900, 900, 3 * FH + 20))
    h1.set_editor_property("hover_yaw", 90.0)
    h1.set_editor_property("approach_delay", 2.0)
    h1.tags = [unreal.Name("BLHeli"), unreal.Name("BLHeli_Land")]
    K.interactable("Subir_Halcon", "BLObjective_Heli", 12900, 900, 3 * FH + 110, "Subir al helicóptero", hold=1.0)


def mission():
    objectives = [
        objective_def("Entra en la terminal con tu equipo", REACH, "BLObj_Terminal", radius=500.0, on_start=[radio("M05_Equipo")]),
        objective_def("Despeja la entrada de la terminal", CLEAR, "BLObj_Entrada", squad="Terminal", on_start=[radio("M05_Terminal")]),
        objective_def("Llega a la sala de servidores", REACH, "BLObj_Servidores", radius=700.0, check_height=True,
                      on_start=[radio("M05_Servidores")], time_limit=120.0, fail_text="Han quemado los servidores: los registros se han perdido."),
        objective_def("Desactiva las cargas de termita", USE_ALL, "BLObjective_Termita", on_start=[radio("M05_Termita")]),
        objective_def("Descarga los registros de Corvane", DEFEND, "BLObj_Servidores", squad="Contra", min_duration=50.0, progress="descarga",
                      on_start=[radio("M05_Descarga")], on_complete=[radio("M05_DescargaOk")],
                      waves=[wave(["BLWave_Vestibulo"], 3, 5.0, False, operators=1), wave(["BLWave_Patio"], 4, 8.0, True),
                             wave(["BLWave_Vestibulo", "BLWave_Patio"], 4, 8.0, False, operators=2)]),
        objective_def("Sube a la azotea: el inglés escapa", REACH, "BLObj_Azotea", radius=1800.0, check_height=True,
                      activate=["BLHeliCorvane", "BLVIPRun"], on_start=[radio("M05_Huye")]),
        objective_def("Inutiliza el helicóptero de Corvane (vivo, no dispares al inglés)", unreal.BLObjectiveType.DESTROY, "BLHeliCorvane",
                      on_start=[radio("M05_Helicoptero")], on_complete=[radio("M05_Rendicion")]),
        objective_def("Detén a «el inglés»", USE, "BLObjective_VIP", on_complete=[radio("M05_Detenido")]),
        objective_def("Sube al helicóptero de la coalición", USE, "BLObjective_Heli", activate=["BLHeli", "BLHeli_Land"],
                      on_start=[radio("M05_Extraccion")]),
    ]
    d = K.director("LÍNEA NEGRA", objectives, briefing=[radio("M05_Brief_01"), radio("M05_Brief_02")], debriefing=[radio("M05_Debrief")],
                   phases={"Terminal": 1, "Capitania": 2, "Servidores": 3, "Azotea": 5})
    d.set_editor_property("campaign_finale", True)


def build():
    terminal()
    capitania()
    characters()
    mission()
    K.dawn_fog_lighting(sun_yaw=168.0, sun_pitch=-9.0, sun_intensity=2.5, fog_density=0.045, fog_falloff=0.2,
                        fog_color=(0.42, 0.47, 0.53), volumetric=False)
    K.smoke("Humo_Capitania", 13800, 1500, 3 * FH + 300, color=lc(0.06, 0.055, 0.05, 1), opacity=0.5)   # Corvane quema papeles
    for i, x in enumerate((2000, 6000, 10000, 14000)):
        K.ambient("SW_AmbZ_Water_Loop", x, Y0 + 100, 0, 1.0, f"Amb_Muelle_{i}")


K.run(MAP_PATH, build, nav_bounds=(0, 16000, Y0, Y1, -100, 1300),
      nav_checks=[("Inicio", (500, 0, 100)), ("Terminal", (1500, 0, 100)), ("Entrada", (6000, 0, 100)), ("Patio", (9800, 0, 100)),
                  ("Vestibulo", (11000, 0, 100)), ("Servidores", (13000, -1250, FH + 100)), ("Despacho", (11300, 1500, 2 * FH + 100)),
                  ("Azotea", (12400, 0, 3 * FH + 100))],
      tag="BL_M05")
