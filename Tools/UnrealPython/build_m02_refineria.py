"""Construye /Game/Maps/M02/L_M02_Manifiesto: misión 2 "Manifiesto" (refinería de Kessra, 03:40). Ver Docs/Mision2_Manifiesto.md.

Trazado (cm; +X = este, hacia donde se avanza; +Y = sur). Recinto y -3500..3500, de canal a canal (x -3000..26000):
  Canal oeste (x < 0)   embarcadero de inserción y lancha amarrada; valla del recinto en x 300 con un hueco cortado.
  Parque de tanques     x 800..6200: 6 depósitos con muretes de contención; avenida central iluminada (sodio) y
                        callejones oscuros entre muretes y bajo el rack de tuberías del norte (ruta de sigilo).
  Patio de carga        x 6500..10500: contenedores, camiones, grúa pórtico, brasero con 3 guardias.
  Almacén 7             x 11000..14500 (y -2500..2500, 11 m): contenedores dentro, pasarela alta al norte (tirador
                        de supresión cuando salta la alarma), puerta de personal al oeste y puerta trasera al este.
  Zona de proceso       x 14500..19800: casa de compresores (norte) y casa de bombas + torres (sur) dejan una sola
                        calle; la tubería de gas revienta y la corta (ABLHazardEvent) -> rodeo por la zanja de
                        tuberías (y 2400..2800, 1,6 m bajo el suelo). Antorcha fuera del recinto, al norte.
  Muelle este           x 19800..21800 y embarcadero (x 21800..22600) en la dársena: extracción en lancha.

Ejecutar (necesita frames para el NavMesh):
  UnrealEditor.exe Blackline.uproject -ExecCmds="py <ruta>/Tools/UnrealPython/build_m02_refineria.py" -unattended
"""
import os
import sys

import unreal

sys.path.insert(0, os.path.join(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir()), "Tools", "UnrealPython"))
import bl_levelkit as K
from bl_levelkit import box, ref, prop, marker, radio, enemy, wave, objective_def, REACH, USE, USE_ALL, DEFEND

MAP_PATH = "/Game/Maps/M02/L_M02_Manifiesto"
lc = unreal.LinearColor
Y0, Y1 = -3500, 3500
WATER_Z = -120.0
W_WATER = K.mat("M_Env_Water")
M_PLATE = K.M_METAL


def water(name, x0, x1, y0, y1):
    a = box(name, x0, x1, y0, y1, WATER_Z - 20, WATER_Z, W_WATER)
    a.static_mesh_component.set_collision_profile_name("NoCollision")
    a.static_mesh_component.set_editor_property("can_ever_affect_navigation", False)


# ---------------------------------------------------------------------------
# Suelo, límites y canales
# ---------------------------------------------------------------------------
def ground_and_bounds():
    # Suelo en piezas: la zanja de tuberías (x 15748..19752, y 2380..2820) queda hundida
    box("Suelo_Oeste", 0, 15748, Y0, Y1, -2, 0, K.M_ASPHALT)
    box("Suelo_Proceso", 15748, 19752, Y0, 2380, -2, 0, K.M_FLOOR)
    box("Suelo_ProcesoSur", 15748, 19752, 2820, Y1, -2, 0, K.M_DIRT)
    box("Suelo_Este", 19752, 21800, Y0, Y1, -2, 0, K.M_FLOOR)
    box("Suelo_Fondo", -6000, 30000, -9000, 9000, -400, -360, K.M_DIRT)
    # Muro perimetral norte y sur (4 m) con bloqueo invisible hasta 9 m
    for name, (y0, y1) in (("N", (Y0 - 60, Y0)), ("S", (Y1, Y1 + 60))):
        box(f"Muro_Perimetro_{name}", 0, 21800, y0, y1, 0, 400, K.M_WALL)
        K.hidden_blocker(f"Limite_{name}", 0, 21800, y0, y1, 400, 900)
    # Canal oeste (inserción): agua, muro del muelle y la orilla de enfrente con naves
    water("Agua_CanalOeste", -6000, 0, -9000, 9000)
    box("Muelle_Oeste", -60, 0, Y0, Y1, -400, 0, K.M_WALL)
    box("Orilla_Enfrente", -3200, -3000, -9000, 9000, -400, 300, K.M_WALL)
    K.facade_building("Nave_Enfrente_1", -5200, -3200, -4200, -800, 900, "Concrete", 2 | 8, 2 | 8, seed=901)
    K.facade_building("Nave_Enfrente_2", -5200, -3200, 600, 4600, 1200, "Grey", 2 | 8, 2 | 8, seed=902)
    # Embarcadero de inserción (tablones) y bloqueos del borde del agua salvo el embarcadero
    box("Embarcadero_Oeste", -900, 0, 300, 700, -20, 0, K.M_WOOD)
    for i, x in enumerate((-850, -450)):
        box(f"Embarcadero_Pilote{i}", x - 15, x + 15, 280, 310, -400, 0, K.M_WOOD)
        box(f"Embarcadero_PiloteS{i}", x - 15, x + 15, 690, 720, -400, 0, K.M_WOOD)
    K.hidden_blocker("Borde_Oeste_N", -60, 0, Y0, 300, 0, 800)
    K.hidden_blocker("Borde_Oeste_S", -60, 0, 700, Y1, 0, 800)
    K.hidden_blocker("Borde_Embarcadero_N", -930, 0, 270, 300, 0, 400)
    K.hidden_blocker("Borde_Embarcadero_S", -930, 0, 700, 730, 0, 400)
    K.hidden_blocker("Borde_Embarcadero_O", -930, -900, 270, 730, 0, 400)
    lancha = unreal.get_editor_subsystem(unreal.EditorActorSubsystem).spawn_actor_from_class(
        unreal.BLBoat, unreal.Vector(-480, 950, WATER_Z), unreal.Rotator(roll=0, pitch=0, yaw=180))
    lancha.set_actor_label("Lancha_Insercion")
    lancha.set_editor_property("start_docked", True)
    # Dársena este (extracción): agua, muro, embarcadero y bloqueos
    water("Agua_Darsena", 21800, 30000, -9000, 9000)
    box("Muelle_Este", 21800, 21860, Y0, Y1, -400, 0, K.M_WALL)
    box("Embarcadero_Este", 21800, 22600, -200, 200, -20, 0, K.M_WOOD)
    for i, x in enumerate((22000, 22400)):
        for y in (-215, 215):
            box(f"Embarcadero_Este_Pilote{i}{y}", x - 15, x + 15, y - 15, y + 15, -400, 0, K.M_WOOD)
    K.hidden_blocker("Borde_Este_N", 21800, 21860, Y0, -200, 0, 800)
    K.hidden_blocker("Borde_Este_S", 21800, 21860, 200, Y1, 0, 800)
    K.hidden_blocker("Borde_EmbE_N", 21800, 22630, -230, -200, 0, 400)
    K.hidden_blocker("Borde_EmbE_S", 21800, 22630, 200, 230, 0, 400)
    K.hidden_blocker("Borde_EmbE_E", 22600, 22630, -230, 230, 0, 400)
    # Orilla lejana de la dársena: grúas y naves en silueta
    K.facade_building("Nave_Darsena", 27500, 30500, -5000, 1000, 1400, "Concrete", 2, 2, seed=903)
    for i, (gx, gy) in enumerate(((27000, 2500), (27000, 3400))):
        box(f"Grua_Darsena_Pata{i}", gx - 50, gx + 50, gy - 50, gy + 50, -360, 2600, K.M_RUST)
    box("Grua_Darsena_Pluma", 23500, 29000, 2880, 3020, 2600, 2740, K.M_RUST)


# ---------------------------------------------------------------------------
# Fase 1-2: valla y parque de tanques
# ---------------------------------------------------------------------------
TANKS = [(1800, -2000), (3600, -2000), (5400, -2000), (1800, 2000), (3600, 2000), (5400, 2000)]


def fence_and_tanks():
    # Valla del recinto en x 300 (paneles de 400 a lo largo de Y); el panel de y 500..900 está cortado
    for i, y in enumerate(range(Y0, Y1, 400)):
        if y == 500:
            ref("SM_Ref_Fence", 300, y + 30, 90, z=0, label="Valla_Cortada", scale=(0.25, 1, 1))     # resto doblado
            continue
        ref("SM_Ref_Fence", 300, y, 90, label=f"Valla_{i}")
    # Depósitos con muretes de contención (80 cm: cobertura baja; se saltan) abiertos hacia la avenida
    for i, (cx, cy) in enumerate(TANKS):
        ref("SM_Ref_Tank", cx, cy, 25 * i, label=f"Deposito_{i}")
        K.covers_for_round(cx, cy, 630, 8, False)
        s = 1 if cy > 0 else -1
        near = cy - s * 800        # lado de la avenida
        far = cy + s * 800
        box(f"Murete_{i}_Fondo", cx - 800, cx + 800, min(far, far + s * 25), max(far, far + s * 25), 0, 80, K.M_WALL)
        for side, x in (("O", cx - 800), ("E", cx + 800 - 25)):
            box(f"Murete_{i}_{side}", x, x + 25, min(near, far), max(near, far), 0, 80, K.M_WALL)
        box(f"Murete_{i}_FrenteO", cx - 800, cx - 150, min(near, near - s * 25), max(near, near - s * 25), 0, 80, K.M_WALL)
        box(f"Murete_{i}_FrenteE", cx + 150, cx + 800, min(near, near - s * 25), max(near, near - s * 25), 0, 80, K.M_WALL)
        for px in (cx - 500, cx + 500):
            K.cover_point(px, near - s * 60, px, cy, True)
    # Rack de tuberías al norte (ruta de sigilo por debajo) y tubería baja al sur
    for i, x in enumerate(range(800, 10500, 1000)):
        ref("SM_Ref_PipeRack", x, -3180, 0, label=f"Rack_Norte_{i}")
    for i, x in enumerate(range(900, 6000, 1000)):
        ref("SM_Ref_PipeLow", x, 3250, 0, label=f"Tuberia_Sur_{i}")
    # Avenida iluminada (farolas de sodio con "BLLit": quien cruce por debajo se ve) y una torre de focos encendida
    for i, x in enumerate((1200, 2700, 4500, 6000)):
        K.sodium_light(f"Farola_Avenida_{i}", x, -1100 if i % 2 else 1100, 90 if i % 2 else -90, shadows=(i == 1), intensity=9.0, lit=True)
    K.alarm_light("Foco_Tanques", 6300, -2900, 120, start_on=True, intensity=1800.0, radius=4200.0, cone=28.0)
    # Atrezo: casetas, bidones, palés, contenedor de obra
    box("Caseta_Valvulas", 2500, 2800, -150, 150, 0, 260, K.M_PLASTER)
    K.covers_for_box(2500, 2800, -150, 150, False)
    for x, y in ((1150, -900), (1200, -840), (4350, 900), (6050, -950)):
        prop("SM_Prop_Drum", x, y, K.rng.uniform(0, 360))
    prop("SM_Prop_PalletStack", 3000, 1150, 10)
    prop("SM_Prop_PalletStack", 4900, -1150, -5)
    K.player_start(-500, 500, 0)
    K.start_point("Canal", -500, 500, 0)
    K.checkpoint("Canal", 300, 900, 300, 900, 0)
    K.start_point("Tanques", 900, 600, 0)
    K.checkpoint("Tanques", 800, 1300, -1300, 1300, 0)


# ---------------------------------------------------------------------------
# Fase 3: patio de carga
# ---------------------------------------------------------------------------
def loading_yard():
    rows = {-2300: [(7000, 0, 1), (8400, 0, 0), (9800, 0, 1)],
            -1150: [(7300, 90, 0), (9500, 0, 0)],
            1150: [(7000, 0, 0), (8800, 90, 1)],
            2400: [(7600, 0, 1), (9300, 0, 0), (10300, 90, 0)]}
    n = 0
    for y, items in rows.items():
        for x, yaw, stack in items:
            for level in range(stack + 1):
                prop("SM_Container_20ft", x + K.rng.uniform(-20, 20), y + K.rng.uniform(-15, 15), yaw + K.rng.uniform(-3, 3),
                     z=level * 259, label=f"Contenedor_Patio_{n}", material=K.rng.choice(K.CONTAINER_MATS))
                n += 1
    prop("SM_Veh_Van_Civil", 10200, -700, 90, label="Camion_1")
    prop("SM_Veh_Van_Civil", 10350, 900, 85, label="Camion_2")
    # Grúa pórtico parada sobre el patio
    # (patas en y ±2600: más al norte atravesaban el rack de tuberías)
    for i, (gx, gy) in enumerate(((7900, -2600), (7900, 2600), (8700, -2600), (8700, 2600))):
        box(f"Grua_Patio_Pata{i}", gx - 40, gx + 40, gy - 40, gy + 40, 0, 1800, K.M_RUST)
    for i, gx in enumerate((7900, 8700)):
        box(f"Grua_Patio_Viga{i}", gx - 45, gx + 45, -2650, 2650, 1800, 1900, K.M_RUST)
    box("Grua_Patio_Carro", 7850, 8750, -400, 400, 1720, 1800, K.M_METAL)
    # Brasero con 3 guardias (la luz del fuego también delata: "BLLit")
    prop("SM_Prop_Drum", 8500, 0, 0, label="Brasero")
    fire = K.smoke("Fuego_Brasero", 8500, 0, 0, fire=True, fire_extent=unreal.Vector(16, 16, 88), flame_size=48.0,
                   light_intensity=4500.0, max_flames=10, max_particles=14, life=6.0, start_size=unreal.Vector2D(30, 50),
                   end_size=unreal.Vector2D(200, 320), rise_speed=110.0, spawn_radius=12.0, color=lc(0.12, 0.11, 0.1, 1),
                   opacity=0.4, wind=unreal.Vector(30, 10, 0))
    fire.tags = [unreal.Name("BLLit")]
    K.alarm_light("Foco_Patio_1", 6700, -3000, 50, siren=True)
    K.alarm_light("Foco_Patio_2", 10300, 3050, -130)
    K.start_point("Carga", 6700, 0, 0)
    K.checkpoint("Carga", 6500, 6900, -2000, 2000, 0)


# ---------------------------------------------------------------------------
# Fase 4-5: almacén 7 (fotos, alarma, tirador de supresión)
# ---------------------------------------------------------------------------
WX0, WX1, WY0, WY1, WH = 11000, 14500, -2500, 2500, 1100


def warehouse():
    T = 30
    # Muros con la puerta de personal (oeste, y 1700..1900) y la puerta trasera (este, y -1600..-1400)
    box("Almacen_MuroO_N", WX0, WX0 + T, WY0, 1700, 0, WH, M_PLATE)
    box("Almacen_MuroO_S", WX0, WX0 + T, 1900, WY1, 0, WH, M_PLATE)
    box("Almacen_MuroO_Dintel", WX0, WX0 + T, 1700, 1900, 250, WH, M_PLATE)
    box("Almacen_Persiana", WX0 - 8, WX0, -1000, 1000, 0, 600, K.M_RUST)          # puerta de carga cerrada
    box("Almacen_MuroE_N", WX1 - T, WX1, WY0, -1600, 0, WH, M_PLATE)
    box("Almacen_MuroE_S", WX1 - T, WX1, -1400, WY1, 0, WH, M_PLATE)
    box("Almacen_MuroE_Dintel", WX1 - T, WX1, -1600, -1400, 250, WH, M_PLATE)
    box("Almacen_MuroN", WX0, WX1, WY0, WY0 + T, 0, WH, M_PLATE)
    box("Almacen_MuroS", WX0, WX1, WY1 - T, WY1, 0, WH, M_PLATE)
    box("Almacen_Suelo", WX0, WX1, WY0, WY1, 0, 2, K.M_FLOOR)
    box("Almacen_Cubierta", WX0 - 40, WX1 + 40, WY0 - 40, WY1 + 40, WH, WH + 25, K.M_RUST)
    for i, x in enumerate(range(WX0 + 500, WX1, 700)):
        box(f"Almacen_Cercha{i}", x - 15, x + 15, WY0, WY1, WH - 90, WH - 60, K.M_METAL)
    K.covers_for_building(WX0, WX1, WY0, WY1)
    # Pasarela alta al norte (z 450) con escalera desde el suelo
    # (la escalera de 25 peldaños acaba justo en el borde de la pasarela; la barandilla sur deja libre su llegada)
    K.catwalk("Almacen_Pasarela", WX0 + 200, WX1 - 200, WY0 + T, WY0 + 330, 450, rails="E")
    box("Almacen_Pasarela_BarS", WX0 + 500, WX1 - 200, WY0 + 324, WY0 + 330, 545, 555, K.M_METAL)
    K.stair_flight("Almacen_Pasarela_Esc", WX0 + 200, WX0 + 480, WY0 + 330 + 25 * 28, -1, 0, steps=25, rise=18.0, m=K.M_METAL)
    # Contenedores dentro (los tres del manifiesto con su número en el objetivo) y palés
    inner = [(11900, -900, 0, 0, "KSR 4471"), (13000, 900, 0, 1, "KSR 4472"), (13600, -500, 0, 2, "KSR 0918"),
             (11900, 1100, 0, 3, None), (12600, -1700, 0, 0, None), (13900, 1700, 90, 1, None), (12300, 300, 0, 2, None)]
    for i, (x, y, yaw, c, code) in enumerate(inner):
        mesh = "SM_Container_20ft_Open" if code == "KSR 0918" else "SM_Container_20ft"
        prop(mesh, x, y, yaw, label=f"Contenedor_Almacen_{i}", material=K.CONTAINER_MATS[c])
    prop("SM_Container_20ft", 12600, -1700, 2, z=259, label="Contenedor_Almacen_Apilado", material=K.CONTAINER_MATS[3])
    # El 0918, abierto: cajas de drones delante y dos municiones sueltas sobre un palé
    for i, (x, y, yaw) in enumerate(((14050, -500, 90), (14100, -150, 80), (14050, -850, 95))):
        ref("SM_Ref_Crate_Drones", x, y, yaw, label=f"Caja_Drones_{i}")
    prop("SM_Prop_Pallet", 14250, 250, 10, label="Pale_Municion")
    ref("SM_Ref_Munition", 14250, 250, 15, z=15, label="Municion_Suelta_1")
    ref("SM_Ref_Munition", 14230, 330, -10, z=15, label="Municion_Suelta_2")
    # Fotos: el objetivo cuenta las tres (en cualquier orden); cada una tiene su frase
    for i, (x, y, line) in enumerate(((12250, -900, "M02_Foto1"), (13350, 900, "M02_Foto2"), (14000, -380, "M02_Foto3"))):
        K.interactable(f"Foto_{i + 1}", "BLObjective_Foto", x, y, 150, "Fotografiar el contenedor", hold=1.5, photo=True,
                       use_radio=[radio(line)])
    # Luz: tres fluorescentes tenues (la mitad del almacén queda a oscuras)
    for i, (x, y) in enumerate(((11700, 0), (13200, -1300), (13900, 1200))):
        K.point_light(f"Almacen_Luz{i}", x, y, WH - 120, (0.78, 0.9, 1.0), 6.0, 900.0, lit=True)
    K.reverb_zone("Acustica_Almacen", WX0, WX1, WY0, WY1, WH, "RE_Interior", 2.0, 0.6)
    marker("BLObj_Almacen", WX0 - 150, 1800)
    marker("BLObj_AlmacenDentro", 12400, 0)
    marker("BLWave_Pasarela", 12800, WY0 + 180, 450)
    for x, y in ((10500, 1500), (10500, 2100), (10700, 1000)):
        marker("BLWave_AlmacenO", x, y)
    marker("BLWave_AlmacenN", 12400, -3000)        # entre el muro del almacén y el perímetro (llegan por el rack)
    K.start_point("Almacen", 11200, 1800, 0)
    K.checkpoint("Almacen", WX0 + 50, WX0 + 600, 1300, 2300, 0)


# ---------------------------------------------------------------------------
# Fase 6-7: zona de proceso (dron, tubería de gas, zanja)
# ---------------------------------------------------------------------------
def process_area():
    # Casa de compresores (norte) y de bombas (sur): entre las dos, una sola calle (y -1000..1000)
    K.facade_building("Compresores", 15800, 19700, Y0, -1000, 800, "Concrete", 4 | 2, 4 | 2, seed=1201)
    K.facade_building("Bombas", 15800, 17600, 1000, 2300, 600, "Grey", 8 | 2, 8 | 2, seed=1202)
    # Recinto de las torres de destilación (vallado: no se pasa)
    for i, y in enumerate((950, 2350)):
        for j, x in enumerate(range(17600, 19700, 400)):
            ref("SM_Ref_Fence", x, y - 4, 0, label=f"Valla_Torres_{i}_{j}")
    for j, y in enumerate(range(950, 2350, 400)):
        ref("SM_Ref_Fence", 19700, min(y, 1950), 90, label=f"Valla_Torres_E_{j}")
    for i, x in enumerate((18100, 19100)):
        ref("SM_Ref_Column", x, 1650, 30 * i, label=f"Torre_{i}")
    # Racks sobre la calle y sobre la zanja
    # (centrados en y 2650: con 2600 los pilares norte caían dentro de la casa de bombas)
    for i, x in enumerate((15900, 17000, 18100)):
        ref("SM_Ref_PipeRack", x, 2650, 0, label=f"Rack_Zanja_{i}")
    ref("SM_Ref_PipeRack", 15100, -2600, 90, label="Rack_Trasera")
    # Zanja de tuberías: 1,6 m bajo el suelo entre muros de 4 m (y 2400..2800), escaleras en los extremos
    box("Zanja_Suelo", 16000, 19500, 2400, 2800, -200, -160, K.M_FLOOR)
    box("Zanja_MuroN", 15748, 19752, 2380, 2400, -200, 250, K.M_WALL)
    box("Zanja_MuroS", 15748, 19752, 2800, 2820, -200, 250, K.M_WALL)
    box("Zanja_CierreO", 15700, 15748, 2820, Y1, 0, 250, K.M_WALL)     # la franja sur queda cerrada
    box("Zanja_CierreE", 19752, 19800, 2820, Y1, 0, 250, K.M_WALL)
    K.stair_flight_x("Zanja_EscO", 2400, 2800, 16000, -1, -160, steps=9, rise=160.0 / 9)
    K.stair_flight_x("Zanja_EscE", 2400, 2800, 19500, 1, -160, steps=9, rise=160.0 / 9)
    for i, x in enumerate((16100, 17150, 18200)):
        ref("SM_Ref_PipeLow", x, 2690, 0, z=-160, label=f"Zanja_Tuberia_{i}")
    # La tubería de gas que revienta: cruza la calle a la altura del rack (x 17450) y bloquea al estallar
    ref("SM_Ref_PipeLow", 16950, -880, 0, label="Tuberia_Gas")      # junto a la fachada de compresores
    hz = unreal.get_editor_subsystem(unreal.EditorActorSubsystem).spawn_actor_from_class(unreal.BLHazardEvent, unreal.Vector(17450, 0, 0))
    hz.set_actor_label("Evento_Gas")
    hz.set_editor_property("fire_points", [unreal.Vector(17450, -650, 0), unreal.Vector(17450, 0, 0), unreal.Vector(17450, 650, 0)])
    hz.set_editor_property("trigger_radius", 1800.0)
    hz.set_editor_property("radio", [radio("M02_Gas")])
    hz.tags = [unreal.Name("BLGasLeak")]
    blk = K.hidden_blocker("Gas_Bloqueo", 17250, 17650, -1000, 1000, 0, 500, nav=False)
    blk.tags = [unreal.Name("BLHazardBlock")]
    # Antorcha (fuera del recinto, al norte): llama enorme que ilumina de naranja toda la zona
    ref("SM_Ref_FlareStack", 17500, -6200, 0, label="Antorcha")
    K.smoke("Llama_Antorcha", 17500, -6200, 4320, fire=True, fire_extent=unreal.Vector(60, 60, 140), flame_size=260.0,
            light_intensity=60000.0, fire_light_radius=14000.0, max_flames=20, max_particles=24, life=14.0,
            start_size=unreal.Vector2D(200, 300), end_size=unreal.Vector2D(1600, 2400), rise_speed=260.0, spawn_radius=60.0,
            color=lc(0.08, 0.07, 0.065, 1), opacity=0.5, wind=unreal.Vector(90, 30, 0))
    K.ambient("SW_AmbZ_Flare_Loop", 17500, -3600, 600, 1.0, "Amb_Antorcha")
    # Focos de la alarma, farolas de la calle y un camión cisterna aparcado
    K.alarm_light("Foco_Proceso_1", 15000, -3100, 60, siren=True)
    K.alarm_light("Foco_Proceso_2", 20400, 3050, -120)
    for i, x in enumerate((15300, 18600)):
        K.sodium_light(f"Farola_Proceso_{i}", x, -900, 90, intensity=8.0, lit=True)
    prop("SM_Veh_Van_Civil", 16300, 450, 5, label="Furgon_Proceso")
    marker("BLObj_Trasera", 14900, -1500)
    marker("BLObj_Proceso", 20300, 600)
    K.start_point("Proceso", 14900, -1500, 0)
    K.checkpoint("Trasera", WX1 + 30, 15300, -2200, -800, 0)
    K.checkpoint("Proceso", 19800, 20400, -1000, 1500, 0)


# ---------------------------------------------------------------------------
# Fase 8: muelle este (extracción)
# ---------------------------------------------------------------------------
def east_quay():
    for i, (x, y, yaw, c) in enumerate(((20600, -1800, 0, 0), (20900, 1900, 90, 2), (21200, -600, 10, 1))):
        prop("SM_Container_20ft", x, y, yaw, label=f"Contenedor_Muelle_{i}", material=K.CONTAINER_MATS[c])
    prop("SM_Cover_Jersey", 21400, 600, 80, label="Jersey_Muelle")
    prop("SM_Cover_Sandbag_Corner", 21550, -300, 0, label="Sacos_Muelle")
    for i, y in enumerate(range(-3000, 3400, 1100)):
        box(f"Bolardo_Este_{i}", 21700, 21760, y - 30, y + 30, 0, 55, K.M_RUST)
    K.alarm_light("Foco_Muelle", 21400, -3100, 100)
    K.ambient("SW_AmbZ_Water_Loop", 21900, -1500, 0, 1.0, "Amb_Darsena_0")
    K.ambient("SW_AmbZ_Water_Loop", 21900, 1800, 0, 1.0, "Amb_Darsena_1")
    marker("BLObj_Muelle", 21500, 0)
    for x, y in ((20300, -3100), (20800, -3100)):
        marker("BLWave_MuelleN", x, y)
    for x, y in ((20300, 3100), (20900, 3100)):
        marker("BLWave_MuelleS", x, y)
    A = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    boat = A.spawn_actor_from_class(unreal.BLBoat, unreal.Vector(26000, 6000, WATER_Z), unreal.Rotator(roll=0, pitch=0, yaw=200))
    boat.set_actor_label("Lancha_Extraccion")
    boat.set_editor_property("path", [unreal.Vector(26000, 6000, WATER_Z), unreal.Vector(24600, 1400, WATER_Z),
                                      unreal.Vector(23600, 520, WATER_Z), unreal.Vector(22400, 520, WATER_Z)])
    boat.set_editor_property("board_offset", unreal.Vector(-60, 140, 150))
    boat.tags = [unreal.Name("BLBoat"), unreal.Name("BLBoat_Board")]
    K.interactable("Subir_Lancha", "BLObjective_Boat", 22400, 380, 40, "Subir a la lancha", hold=1.0)
    K.start_point("Muelle", 20500, 0, 0)


# ---------------------------------------------------------------------------
# Enemigos, dron, misión, ambiente
# ---------------------------------------------------------------------------
def enemies_and_drone():
    # Parque de tanques: dos patrullas con linterna por los callejones y un centinela bajo la farola de la avenida
    # (rutas por el borde de la avenida, los extremos libres y el callejón bajo el rack / junto a la tubería sur)
    enemy("Tanques_Patrulla_N", "Tanques", 700, -1300, 0, [(700, -1300), (6350, -1300), (6350, -3000), (700, -3000)], flashlight=True)
    enemy("Tanques_Patrulla_S", "Tanques", 6350, 1300, 180, [(6350, 1300), (700, 1300), (700, 3000), (6350, 3000)], flashlight=True)
    enemy("Tanques_Centinela", "Tanques", 4500, -300, 180)
    # Patio de carga: tres en el brasero y una patrulla con linterna entre los contenedores
    enemy("Carga_Brasero_1", "Carga", 8380, -120, 30)
    enemy("Carga_Brasero_2", "Carga", 8620, 110, -150)
    enemy("Carga_Brasero_3", "Carga", 8480, 180, -80)
    enemy("Carga_Patrulla", "Carga", 7000, -1700, 0, [(7000, -1700), (10300, -1700), (10300, 1700), (7000, 1700)], flashlight=True)
    # Almacén: un vigilante con linterna
    enemy("Almacen_Vigilante", "Almacen", 11500, 0, 0, [(11500, 0), (14100, 300), (14100, -1300), (11500, -1300)], flashlight=True)
    # Zona de proceso y muelle
    enemy("Proceso_Calle_1", "Proceso", 16600, -500, 180)
    enemy("Proceso_Calle_2", "Proceso", 16700, 450, 180, flashlight=True)
    enemy("Proceso_Este", "Proceso", 19900, 1500, 180)
    enemy("Muelle_1", "Muelle", 21300, -900, 180)
    enemy("Muelle_2", "Muelle", 21300, 1000, 180, flashlight=True)
    # Dron de Corvane: vuelta a la zona de proceso a 10 m
    A = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    d = A.spawn_actor_from_class(unreal.BLDrone, unreal.Vector(15200, -2000, 1000))
    d.set_actor_label("Dron_Corvane")
    d.set_editor_property("path", [unreal.Vector(15200, -2000, 1000), unreal.Vector(19600, -1800, 1100),
                                   unreal.Vector(20600, 1800, 1000), unreal.Vector(16000, 2200, 950)])
    d.tags = [unreal.Name("BLDrone")]


def mission():
    marker("BLObj_Valla", 650, 600)
    marker("BLObj_Tanques", 6400, 0)
    objectives = [
        objective_def("Cruza la valla del recinto", REACH, "BLObj_Valla", radius=320.0),
        objective_def("Atraviesa el parque de tanques sin que te vean", REACH, "BLObj_Tanques", radius=450.0,
                      on_start=[radio("M02_Tanques")]),
        objective_def("Rodea el patio de carga hasta el almacén 7", REACH, "BLObj_Almacen", radius=280.0,
                      on_start=[radio("M02_Carga")]),
        objective_def("Fotografía los contenedores del manifiesto", USE_ALL, "BLObjective_Foto",
                      on_start=[radio("M02_Almacen")], on_complete=[radio("M02_Revelacion")]),
        objective_def("Resiste dentro del almacén", DEFEND, "BLObj_AlmacenDentro", squad="Alarma", min_duration=40.0,
                      activate=["BLAlarm"], on_start=[radio("M02_Alarma")],
                      waves=[wave(["BLWave_Pasarela"], 1, 2.0, False, [radio("M02_Ametralladora")], gunners=1),
                             wave(["BLWave_AlmacenO"], 4, 6.0, True, [radio("M02_Refuerzos")], flashlights=True),
                             wave(["BLWave_AlmacenO", "BLWave_AlmacenN"], 3, 8.0, False, flashlights=True)]),
        objective_def("Sal por la puerta trasera", REACH, "BLObj_Trasera", radius=300.0, activate=["BLDrone"],
                      on_complete=[radio("M02_Dron"), radio("M02_DronTorre")]),
        objective_def("Cruza la zona de proceso hasta el muelle", REACH, "BLObj_Proceso", radius=500.0, activate=["BLGasLeak"]),
        objective_def("Aguanta en el muelle hasta que llegue la lancha", DEFEND, "BLObj_Muelle", squad="Muelle", min_duration=40.0,
                      activate=["BLBoat"], on_start=[radio("M02_Canal")],
                      waves=[wave(["BLWave_MuelleN"], 3, 4.0, False, flashlights=True),
                             wave(["BLWave_MuelleS"], 3, 8.0, True, [radio("M02_Ametralladora")], gunners=1)]),
        objective_def("Sube a la lancha", USE, "BLObjective_Boat", activate=["BLBoat_Board"], on_start=[radio("M02_Lancha")]),
    ]
    K.director("MANIFIESTO", objectives,
               briefing=[radio("M02_Brief_01"), radio("M02_Brief_02"), radio("M02_Brief_03")],
               debriefing=[radio("M02_Debrief")],
               phases={"Tanques": 1, "Carga": 2, "Almacen": 3, "Proceso": 5, "Muelle": 7},
               alarm_on_detection=True, alarm_radio=[radio("M02_Detectado")])


def atmosphere():
    night = K.night_lighting()
    night.set_editor_property("dark_sight_radius", 2600.0)
    for i, (x, y) in enumerate(((3600, 0), (8500, -2800), (17000, 0), (12700, 2600))):
        K.ambient("SW_AmbZ_Refinery_Loop", x, y, 300, 0.9, f"Amb_Refineria_{i}")
    K.ambient("SW_AmbZ_Water_Loop", -100, 500, 0, 1.0, "Amb_CanalOeste")
    # Vapor de las torres y humo de la refinería al fondo
    for i, (x, y) in enumerate(((18100, 1650), (19100, 1650))):
        K.smoke(f"Vapor_Torre_{i}", x, y, 3300, max_particles=12, life=10.0, start_size=unreal.Vector2D(100, 160),
                end_size=unreal.Vector2D(600, 900), rise_speed=120.0, color=lc(0.35, 0.34, 0.33, 1), opacity=0.25, spawn_radius=40.0)
    K.smoke("Humo_Lejano_Refineria", 24000, -9000, 0, color=lc(0.05, 0.045, 0.04, 1), opacity=0.5)


def build():
    ground_and_bounds()
    fence_and_tanks()
    loading_yard()
    warehouse()
    process_area()
    east_quay()
    enemies_and_drone()
    mission()
    atmosphere()


K.run(MAP_PATH, build, nav_bounds=(-1000, 22700, -3600, 3600, -300, 1300),
      nav_checks=[("Canal", (-500, 500, 100)), ("Valla", (650, 600, 100)), ("FinTanques", (6400, 0, 100)),
                  ("PuertaAlmacen", (10850, 1800, 100)), ("Contenedor0918", (14000, -380, 100)), ("Pasarela", (12800, -2300, 550)),
                  ("Trasera", (14900, -1500, 100)), ("Zanja", (17500, 2500, -60)), ("FinProceso", (20300, 600, 100)),
                  ("Muelle", (21500, 0, 100)), ("Embarcadero", (22300, 0, 100))],
      tag="BL_M02")
