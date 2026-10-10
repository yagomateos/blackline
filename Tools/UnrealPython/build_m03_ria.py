"""Construye /Game/Maps/M03/L_M03_Ria: misión 3 "Ría" (casco viejo y muelles, 07:20, niebla). Ver Docs/Mision3_Ria.md.

Trazado (cm; +X = hacia donde se avanza; la ría queda al sur, y < -2000):
  Orilla oeste     x -6000..-3400: plazuela con el furgón de inserción.
  Puente viejo     x -3400..-1000 sobre un canal 6,5 m más abajo (SM_StoneBridge); dos centinelas al otro lado.
  Casco viejo      x -1000..5600: calle mayor (y 400..1000) con soportales, plaza pequeña (x 1500..2700) con control;
                   callejón (x 2700..3000) baja al paseo de la ribera (y -2000..-1400).
  Casas            x 3000..5600: tres casas del práctico (2 plantas + buhardilla) con puertas ABLDoor; la tercera atrancada.
  Plaza grande     x 5800..8800 con el campanario (x 7300, y 2600) y su tirador; la lonja al sur, soportales al oeste.
  Pescadores       x 8800..11500: paseo con barcas, cajas y redes.
  Muelle exterior  x 11500..14500, espigón hasta y -3200: el barco sin bandera amarrado (ABLScriptedMover).
  Almacenes        x 11000..14500, y 0..3000: salida hasta el furgón de BLACKLINE.

Ejecutar: UnrealEditor.exe Blackline.uproject -ExecCmds="py <ruta>/Tools/UnrealPython/build_m03_ria.py" -unattended
"""
import os
import sys

import unreal

sys.path.insert(0, os.path.join(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir()), "Tools", "UnrealPython"))
import bl_levelkit as K
from bl_levelkit import box, prop, marker, radio, enemy, wave, objective_def, REACH, USE, CLEAR, DEFEND

MAP_PATH = "/Game/Maps/M03/L_M03_Ria"
TOWN = "/Game/Environment/OldTown/"
lc = unreal.LinearColor
RIA_Z, CANAL_Z = -350.0, -650.0
W_WATER = K.mat("M_Env_Water")
FH = 320.0
WT = 25.0


def town(mesh, x, y, yaw=0.0, z=0.0, label=None, scale=None):
    return prop(mesh, x, y, yaw, z, label, path=TOWN, scale=scale)


def water(name, x0, x1, y0, y1, z):
    a = box(name, x0, x1, y0, y1, z - 20, z, W_WATER)
    a.static_mesh_component.set_collision_profile_name("NoCollision")
    a.static_mesh_component.set_editor_property("can_ever_affect_navigation", False)


def wall_x(name, y0, y1, xa, xb, z0, z1, holes, m):
    """Muro a lo largo de X con huecos [(x0, x1, zb, zt)] (igual que en la misión 1)."""
    x = xa
    for i, (h0, h1, hb, ht) in enumerate(sorted(holes)):
        if h0 > x:
            box(f"{name}_S{i}", x, h0, y0, y1, z0, z1, m)
        if hb > z0:
            box(f"{name}_B{i}", h0, h1, y0, y1, z0, hb, m)
        if ht < z1:
            box(f"{name}_T{i}", h0, h1, y0, y1, ht, z1, m)
        x = h1
    if x < xb:
        box(f"{name}_End", x, xb, y0, y1, z0, z1, m)


def wall_y(name, x0, x1, ya, yb, z0, z1, holes, m):
    y = ya
    for i, (h0, h1, hb, ht) in enumerate(sorted(holes)):
        if h0 > y:
            box(f"{name}_S{i}", x0, x1, y, h0, z0, z1, m)
        if hb > z0:
            box(f"{name}_B{i}", x0, x1, h0, h1, z0, hb, m)
        if ht < z1:
            box(f"{name}_T{i}", x0, x1, h0, h1, ht, z1, m)
        y = h1
    if y < yb:
        box(f"{name}_End", x0, x1, y, yb, z0, z1, m)


# ---------------------------------------------------------------------------
# Terreno, agua y límites
# ---------------------------------------------------------------------------
def terrain():
    box("Suelo_OrillaOeste", -6000, -3400, -2000, 3000, -2, 0, K.M_ASPHALT)
    box("Suelo_Casco", -1000, 14500, -2000, 3000, -2, 0, K.M_FLOOR)
    box("Espigon", 11500, 14500, -3200, -2000, -2, 0, K.M_FLOOR)
    box("Fondo", -9000, 30000, -15000, 9000, -1500, -1450, K.M_DIRT)
    # Canal bajo el puente (6,5 m) y la ría al sur (3,5 m)
    water("Agua_Canal", -3400, -1000, -15000, 9000, CANAL_Z)
    for name, x0, x1 in (("Canal_MuroO", -3460, -3400), ("Canal_MuroE", -1000, -940)):
        box(name, x0, x1, -2000, 3000, CANAL_Z - 400, 0, K.M_WALL)
    water("Agua_Ria", -9000, 30000, -15000, -2000, RIA_Z)
    box("Muelle_Muro", -6000, 11500, -2060, -2000, RIA_Z - 400, 0, K.M_WALL)
    box("Espigon_Muro", 11440, 14560, -3260, -3200, RIA_Z - 400, 0, K.M_WALL)
    box("Espigon_MuroE", 14500, 14560, -3260, -2000, RIA_Z - 400, 0, K.M_WALL)
    # Bordes: nadie se cae al agua (barandilla visual en el paseo + muro invisible)
    for i, x in enumerate(range(-6000, 11500, 250)):
        box(f"Paseo_Poste{i}", x - 5, x + 5, -1995, -1985, 0, 100, K.M_METAL)
    box("Paseo_Baranda", -6000, 11500, -1996, -1984, 92, 100, K.M_METAL)
    K.hidden_blocker("Borde_Ria", -6000, 11500, -2060, -2000, 0, 800)
    K.hidden_blocker("Borde_Espigon_S", 11440, 14560, -3260, -3200, 0, 800)
    K.hidden_blocker("Borde_Espigon_E", 14500, 14560, -3260, 3000, 0, 800)
    # (con el hueco del puente, y 250..950)
    for name, x0, x1 in (("O", -3460, -3400), ("E", -1000, -940)):
        K.hidden_blocker(f"Borde_Canal_{name}_S", x0, x1, -2000, 250, 0, 800, nav=False)
        K.hidden_blocker(f"Borde_Canal_{name}_N", x0, x1, 950, 3000, 0, 800, nav=False)
    K.hidden_blocker("Limite_N", -6000, 14560, 3000, 3060, 0, 1200)
    K.hidden_blocker("Limite_O", -6060, -6000, -2000, 3000, 0, 1200)
    # Puente viejo: el tablero a z 0 enlaza las dos orillas
    town("SM_StoneBridge", -2200, 600, 0, label="Puente_Viejo")
    # Orilla oeste: plazuela con fachadas
    K.facade_building("O_1", -6000, -3460, 1300, 3000, 1280, "Cream", 1 | 8, 1 | 8, street_mask=8, seed=3101)
    K.facade_building("O_2", -6000, -3460, -2000, -200, 960, "Ochre", 1 | 4, 1 | 4, street_mask=4, seed=3102)
    prop("SM_Veh_Van_Civil_Open", -5200, 600, 0, label="Furgon_Insercion")
    K.player_start(-4800, 600, 0)
    K.start_point("Puente", -4800, 600, 0)
    K.checkpoint("Puente", -4500, -3500, 0, 1200, 0)


# ---------------------------------------------------------------------------
# Casco viejo: calle mayor, soportales, plaza pequeña
# ---------------------------------------------------------------------------
def old_town():
    # Manzana norte con soportales a la calle y la plaza pequeña abierta en medio
    K.facade_building("N_1", -940, 1500, 1300, 3000, 1280, "Ochre", 15, 15, street_mask=8, seed=3201, no_ground=True, z=420)
    # Cierre bajo la manzana elevada (sin él, desde la plaza y el canal se veía el vacío bajo los soportales)
    box("N_1_CierreE", 1490, 1500, 1000, 3000, 0, 420, K.mat("MI_Fac_Wall_Ochre"))
    box("N_1_CierreO", -940, -930, 1000, 3000, 0, 420, K.mat("MI_Fac_Wall_Ochre"))
    K.facade_building("N_2", 2700, 5600, 2200, 3000, 1120, "Salmon", 8 | 2, 8 | 2, street_mask=8, seed=3202)
    for i, x in enumerate(range(-900, 1500, 400)):
        town("SM_Arcade", x, 1000, 0, label=f"Soportal_N_{i}")
    box("Soportal_N_Fondo", -940, 1500, 1290, 1300, 0, 420, K.mat("MI_Fac_Wall_Ochre"))
    # Manzana sur (entre la calle y el paseo), cortada por el callejón que baja a la ribera
    K.facade_building("S_1", -940, 1500, -1400, 400, 960, "Cream", 1 | 2 | 4 | 8, 4 | 8, street_mask=4, seed=3203)
    K.facade_building("S_2", 1500, 2700, -1400, 400, 1120, "Grey", 1 | 4 | 8, 4 | 8, street_mask=4, seed=3204)
    K.covers_for_building(1500, 2700, -1400, 400)
    # Calle mayor: coche, contenedor de basura, escombros
    prop("SM_Veh_Sedan_Wreck", 600, 700, 8, label="Coche_Calle")
    prop("SM_Prop_Dumpster", -300, 520, 90)
    # Plaza pequeña con control de la Columna (sacos) y fuente
    box("Plaza_Suelo", 1500, 2700, 1000, 2200, 0, 3, K.M_PLASTER)
    K.facade_building("Plaza_Fondo", 1500, 2700, 2200, 3000, 1280, "Green", 4, 4, street_mask=4, seed=3205)
    prop("SM_Cover_Sandbag_Corner", 2300, 1500, 180, label="Sacos_Plaza_1")
    prop("SM_Cover_Sandbag_Straight", 2350, 1250, 90, label="Sacos_Plaza_2")
    box("Fuente", 1950, 2150, 1650, 1850, 0, 70, K.M_WALL)
    K.covers_for_box(1950, 2150, 1650, 1850, True)
    # Barricada de contenedores al final de la calle: el camino sigue por el callejón de la ribera (las casas)
    for i, (y, yaw) in enumerate(((700, 90), (1300, 91), (1900, 89))):
        prop("SM_Container_20ft", 3050, y, yaw, label=f"Barricada_{i}", material=K.CONTAINER_MATS[i % 4])
    K.facade_building("N_3", 3200, 5600, 400, 2200, 960, "Cream", 8 | 1, 8 | 1, seed=3206)
    for i, (x, y, yaw) in enumerate(((0, 1150, 90), (2600, 1150, 90), (-600, 250, -90))):
        K.sodium_light(f"Farola_Casco_{i}", x, y, yaw, intensity=5.0)
    marker("BLObj_Puente", -700, 600)
    marker("BLObj_Plaza", 2100, 1300)
    K.start_point("Casco", -600, 600, 0)
    K.checkpoint("Casco", -940, -400, 400, 1000, 0)


# ---------------------------------------------------------------------------
# Casas del práctico (CQB): 2 plantas + buhardilla, puertas ABLDoor en la fachada del paseo
# ---------------------------------------------------------------------------
def house(n, x0, x1, wall, barred=False):
    y0, y1 = -1400, -500
    W = K.mat(f"MI_Fac_Wall_{wall}")
    door_x = (x0 + x1) / 2 + 100
    for k in range(3):
        z, zt = k * FH, (k + 1) * FH if k < 2 else 900
        front = [(door_x - 85, door_x + 85, 0, 235)] if k == 0 else [(x0 + 150, x0 + 290, z + 100, z + 230), (x1 - 290, x1 - 150, z + 100, z + 230)]
        if k == 0:
            front.append((x0 + 120, x0 + 260, 100, 230))
        wall_x(f"Casa{n}_S{k}", y0, y0 + WT, x0, x1, z, zt, front, W)
        wall_x(f"Casa{n}_N{k}", y1 - WT, y1, x0, x1, z, zt, [(x0 + 300, x0 + 420, z + 110, z + 220)] if k < 2 else [], W)
        wall_y(f"Casa{n}_O{k}", x0, x0 + WT, y0 + WT, y1 - WT, z, zt, [], W)
        wall_y(f"Casa{n}_E{k}", x1 - WT, x1, y0 + WT, y1 - WT, z, zt, [(-1100, -980, z + 110, z + 220)] if k == 1 else [], W)
    # Forjados con el hueco de la escalera (pegada al muro norte, subiendo hacia +X). Cada tramo arranca de un rellano
    # de 1,25 m junto al muro oeste (si arranca pegado al muro, el NavMesh no conecta el primer peldaño: solo se entra
    # de lado por una franja más estrecha que el radio del agente) y desemboca en el forjado norte.
    for k, z in ((1, FH), (2, 2 * FH)):
        box(f"Casa{n}_Forjado{k}_S", x0 + WT, x1 - WT, y0 + WT, -800, z - 20, z, K.M_WOOD)
        box(f"Casa{n}_Forjado{k}_N", x0 + 598, x1 - WT, -800, y1 - WT, z - 20, z, K.M_WOOD)   # empieza donde acaba el tramo
        box(f"Casa{n}_Rellano{k}", x0 + WT, x0 + 150, -800, y1 - WT, z - 20, z, K.M_WOOD)     # arranque del tramo siguiente
        K.stair_flight_x(f"Casa{n}_Esc{k}", -790, -530, x0 + 150, 1, (k - 1) * FH, steps=16, rise=FH / 16, m=K.M_WOOD)
    box(f"Casa{n}_Suelo", x0, x1, y0, y1, 0, 3, K.M_WOOD)
    box(f"Casa{n}_Tejado", x0 - 30, x1 + 30, y0 - 40, y1 + 30, 900, 930, K.mat("MI_Env_RustyMetal"))
    box(f"Casa{n}_Alero", x0 - 30, x1 + 30, y0 - 80, y0 - 40, 880, 900, K.M_WOOD)
    # Muebles: mesa, aparador y cama (coberturas bajas); bombilla tenue por planta
    box(f"Casa{n}_Mesa", x0 + 250, x0 + 400, y0 + 250, y0 + 350, 0, 75, K.M_WOOD)
    K.covers_for_box(x0 + 250, x0 + 400, y0 + 250, y0 + 350, True)
    box(f"Casa{n}_Aparador", x1 - 120, x1 - 40, y0 + 200, y0 + 500, 0, 110, K.M_WOOD)
    box(f"Casa{n}_Cama", x0 + 300, x0 + 500, y0 + 100, y0 + 300, FH, FH + 50, K.M_PLASTER)
    for k in range(3):
        K.point_light(f"Casa{n}_Luz{k}", (x0 + x1) / 2, -950, k * FH + (280 if k < 2 else 230), (1.0, 0.72, 0.45), 14.0, 550.0, draw_distance=2500.0)
    # Puerta del paseo (bisagra a la izquierda del hueco; la hoja se abre hacia dentro o fuera según quien empuje)
    tag = "BLObjective_PuertaPractico" if barred else None
    K.door_with_frame(f"Casa{n}_Puerta", door_x - 75, y0 + 12, -90, barred=barred, tag=tag)
    return door_x


def houses():
    box("Callejon_Ribera", 2700, 3000, -1400, 400, 0, 2, K.M_FLOOR)
    for i, (x0, x1, wall) in enumerate(((3000, 3800, "Cream"), (3900, 4700, "Salmon"), (4800, 5600, "Ochre"))):
        house(i + 1, x0, x1, wall, barred=(i == 2))
    # Entre las casas, tapias (no se rodean por detrás)
    for x0, x1 in ((3800, 3900), (4700, 4800)):
        box(f"Tapia_{x0}", x0, x1, -1400, -500, 0, 400, K.M_WALL)
    # Detrás de las casas (y -500..400), manzana cerrada
    K.facade_building("Casas_Fondo", 3000, 5600, -500, 400, 1120, "Grey", 4 | 1, 4 | 1, street_mask=4, seed=3301)
    # El cuaderno en la buhardilla de la tercera
    # (fuera del hueco de la escalera: en y > -800 la buhardilla solo tiene suelo en los rellanos)
    box("Buhardilla_Mesa", 5350, 5550, -1200, -1000, 2 * FH, 2 * FH + 75, K.M_WOOD)
    K.interactable("Cuaderno_Practico", "BLObjective_Cuaderno", 5450, -1100, 2 * FH + 80, "Coger el cuaderno del práctico", hold=1.2,
                   pickup=True, mesh="/Game/Environment/Props/SM_Obj_HardDrive")
    # Paseo de la ribera: bancos, farolas, cajas
    for i, x in enumerate((3300, 4400, 5300)):
        K.sodium_light(f"Farola_Paseo_{i}", x, -1850, 90, intensity=5.0)
    marker("BLObj_Casas", 3400, -1700)
    marker("BLObj_Casa3", 5200, -1000)
    K.start_point("Casas", 2850, -1000, -90)
    K.checkpoint("Casas", 2700, 3000, -1400, 400, -90)


# ---------------------------------------------------------------------------
# Plaza grande con el campanario (tirador), lonja y soportales
# ---------------------------------------------------------------------------
def big_square():
    box("PlazaGrande_Suelo", 5800, 8800, -1400, 2200, 0, 3, K.M_PLASTER)
    town("SM_BellTower", 7300, 2600, 0, label="Campanario")
    K.facade_building("Iglesia", 7650, 8800, 2200, 3000, 1500, "Concrete", 8 | 1, 8 | 1, street_mask=8, seed=3401)
    K.facade_building("PlazaGrande_O", 5600, 6950, 2200, 3000, 1120, "Cream", 8 | 2, 8 | 2, street_mask=8, seed=3402)
    # Soportales a lo largo del oeste de la plaza: ruta cubierta del ángulo del campanario
    for i, y in enumerate(range(-1000, 2200, 400)):
        town("SM_Arcade", 6150, y, 90, label=f"Soportal_Plaza_{i}")     # el fondo (300) queda hacia -X
    # Lonja: nave abierta junto al agua (tejado a 6 m sobre pilares) — cobertura desde arriba
    for i, x in enumerate(range(6000, 8600, 500)):
        for y in (-1350, -650):
            box(f"Lonja_Pilar_{i}_{y}", x - 25, x + 25, y - 25, y + 25, 0, 600, K.M_WALL)
    box("Lonja_Tejado", 5950, 8550, -1400, -600, 600, 640, K.M_RUST)
    for i, (x, y) in enumerate(((6500, -1000), (7300, -1100), (8100, -950))):
        town("SM_FishCrates", x, y, 20 * i, label=f"Lonja_Cajas_{i}")
    # Puestos, fuente y coches: coberturas para cruzar la plaza bajo el láser
    for i, (x, y) in enumerate(((6600, 400), (7700, 300), (6900, 1300), (8200, 1200))):
        box(f"Puesto_{i}", x - 120, x + 120, y - 70, y + 70, 0, 100, K.M_WOOD)
        box(f"Puesto_{i}_Toldo", x - 140, x + 140, y - 90, y + 90, 220, 230, K.M_PLASTER)
        K.covers_for_box(x - 120, x + 120, y - 70, y + 70, True)
    box("Fuente_Grande", 7150, 7450, 700, 1000, 0, 90, K.M_WALL)
    K.covers_for_box(7150, 7450, 700, 1000, True)
    prop("SM_Veh_Sedan_Wreck", 8400, 300, 75, label="Coche_Plaza")
    # Campanas (suenan al empezar el objetivo) — actor guionizado sin malla en lo alto del campanario
    K.mover("Campanas", 7300, 2600, 2300, start_sound="/Game/Audio/Ambience/Distant/SW_Bells_Toll", tag="BLBells")
    marker("BLObj_Campanario", 7300, 2100)
    K.start_point("Campanario", 5700, -1700, 30)
    K.checkpoint("Campanario", 5600, 6000, -1900, -1400, 0)


# ---------------------------------------------------------------------------
# Muelle de pescadores, muelle exterior con el barco y almacenes
# ---------------------------------------------------------------------------
def docks():
    for i, (x, y, yaw) in enumerate(((9300, -2700, 5), (10300, -2750, -8), (11000, -2650, 180))):
        town("SM_FishingBoat", x, y, yaw, z=RIA_Z, label=f"Barca_{i}")
    for i, (x, y) in enumerate(((9200, -1700), (9900, -1500), (10600, -1750), (11200, -1300))):
        town("SM_FishCrates", x, y, 30 * i, label=f"Muelle_Cajas_{i}")
    for i, (x, y, yaw, c) in enumerate(((9500, -800, 0, 1), (12200, -2600, 90, 0), (13300, -2400, 10, 3), (13900, -2750, 0, 2),
                                        (12600, -2100, 0, 1))):
        prop("SM_Container_20ft", x, y, yaw, label=f"Contenedor_Muelle_{i}", material=K.CONTAINER_MATS[c])
    for i, x in enumerate(range(11800, 14400, 700)):
        box(f"Bolardo_Espigon_{i}", x - 30, x + 30, -3180, -3120, 0, 55, K.M_RUST)
    # Almacenes al norte (salida) con su callejón hasta el furgón
    K.facade_building("Almacen_1", 9000, 11000, 400, 3000, 900, "Concrete", 1 | 8, 1 | 8, seed=3501)
    K.facade_building("Almacen_2", 11300, 12900, 600, 3000, 1000, "Grey", 2 | 8 | 1, 2 | 8 | 1, seed=3502)
    K.facade_building("Almacen_3", 13700, 14500, -1200, 3000, 900, "Concrete", 2, 2, seed=3503)
    prop("SM_Veh_Van_Civil", 13300, 2500, 90, label="Furgon_BLACKLINE")
    # El barco sin bandera, amarrado al espigón (proa hacia +X); zarpa al empezar la defensa
    ship_z = RIA_Z
    K.mover("Barco_Sin_Bandera", 12500, -4100, ship_z, 0, mesh=TOWN + "SM_Ship_Cargo",
            # recto hacia delante (al girar junto al espigón la popa barría el muelle) y luego ría abajo
            path=[(21000, -4300, ship_z), (27000, -8000, ship_z), (34000, -11000, ship_z)], speed=420.0, accel=18.0, turn=2.5,
            start_sound="/Game/Audio/Vehicles/SW_Ship_Horn", hide_at_end=True, tag="BLShip", delay=3.0)
    # La baliza: en el costado del casco, junto a la amarra de proa (al pie del espigón)
    # (sobre el bolardo del borde: más allá, el muro invisible del borde tapaba la línea de vista para usarla)
    K.interactable("Baliza", "BLObjective_Baliza", 13900, -3150, 75, "Colocar la baliza en el casco", hold=4.0)
    for name, x, y in (("BLObj_Pescadores", 10000, -1600), ("BLObj_Espigon", 13000, -2700), ("BLObj_Furgon", 13300, 2300)):
        marker(name, x, y)
    for x, y in ((8600, -1000), (8900, -500)):
        marker("BLWave_Lonja", x, y)
    for x, y in ((13200, 800), (13400, 300)):
        marker("BLWave_Almacenes", x, y)
    K.ambient("SW_AmbZ_Gulls_Loop", 10000, -2200, 200, 1.0, "Amb_Gaviotas_0")
    K.ambient("SW_AmbZ_Gulls_Loop", 3000, -2200, 200, 0.7, "Amb_Gaviotas_1")
    for i, x in enumerate((-2000, 4000, 9000, 13000)):
        K.ambient("SW_AmbZ_Water_Loop", x, -2050, 0, 1.0, f"Amb_Ria_{i}")
    K.start_point("Muelle", 8900, -1700, 0)
    K.checkpoint("Muelle", 8800, 9200, -2000, -1400, 0)
    K.start_point("Baliza", 13600, -2900, 0)


# ---------------------------------------------------------------------------
# Enemigos y misión
# ---------------------------------------------------------------------------
def enemies():
    enemy("Puente_1", "Puente", -650, 380, 180)
    enemy("Puente_2", "Puente", -700, 850, 175)
    enemy("Calle_Patrulla", "Calles", 1300, 700, 180, [(1300, 700), (-400, 700)])
    enemy("Plaza_1", "Plaza", 2250, 1400, 180)
    enemy("Plaza_2", "Plaza", 2400, 1150, 200)
    enemy("Plaza_3", "Plaza", 1800, 2000, -90)
    # Casas 1 y 2 (abajo y arriba) y la del práctico (dos en la planta baja, uno en la buhardilla)
    enemy("Casa1_Abajo", "Casas", 3500, -1000, 90)
    enemy("Casa1_Arriba", "Casas", 3650, -1050, 180, z=FH)
    enemy("Casa2_Abajo", "Casas", 4200, -900, -90)
    enemy("Casa2_Arriba", "Casas", 4500, -1150, 180, z=FH)
    enemy("Casa3_Abajo_1", "Casa3", 5100, -1100, -90)
    enemy("Casa3_Abajo_2", "Casa3", 5400, -800, -90)
    enemy("Casa3_Buhardilla", "Casa3", 5000, -1000, 0, z=2 * FH)
    # Campanario: el tirador de Corvane (no se mueve) y dos milicianos en la plaza
    # (pegado al arco sur: más atrás, el alféizar le tapaba la plaza)
    enemy("Campanario_Tirador", "Campanario", 7300, 2400, -90, z=1950, role="sniper")
    enemy("PlazaG_1", "Campanario", 7700, 400, 180)
    enemy("PlazaG_2", "Campanario", 8200, 1300, 200)
    # Pescadores: tres milicianos y un operador que los dirige
    enemy("Pesc_1", "Pescadores", 9500, -1700, 180)
    enemy("Pesc_2", "Pescadores", 10300, -1400, 170)
    enemy("Pesc_3", "Pescadores", 10700, -1800, 190, flashlight=False)
    enemy("Pesc_Operador", "Pescadores", 11200, -1100, 180, role="operator")
    # Muelle exterior: cuatro operadores de Corvane en el espigón
    for i, (x, y) in enumerate(((12400, -2300), (13000, -2950), (13600, -2200), (14100, -2900))):
        enemy(f"Corvane_{i}", "Corvane", x, y, 180, role="operator")


def mission():
    objectives = [
        objective_def("Cruza el puente viejo", REACH, "BLObj_Puente", radius=450.0, on_start=[radio("M03_Puente")]),
        objective_def("Atraviesa el casco viejo hasta la plaza pequeña", REACH, "BLObj_Plaza", radius=600.0, on_start=[radio("M03_Calles")]),
        objective_def("Despeja las casas de la ribera", CLEAR, "BLObj_Casas", squad="Casas", on_start=[radio("M03_Casas")]),
        objective_def("Entra en la casa del práctico", USE, "BLObjective_PuertaPractico", on_start=[radio("M03_Atrancada")]),
        objective_def("Despeja la casa del práctico", CLEAR, "BLObj_Casa3", squad="Casa3"),
        objective_def("Coge el cuaderno del práctico (buhardilla)", USE, "BLObjective_Cuaderno",
                      on_complete=[radio("M03_Cuaderno"), radio("M03_CuadernoTorre")]),
        objective_def("Neutraliza al tirador del campanario", CLEAR, "BLObj_Campanario", squad="Campanario", activate=["BLBells"],
                      on_start=[radio("M03_Tirador"), radio("M03_TiradorTorre")], on_complete=[radio("M03_TiradorOk")]),
        objective_def("Llega al muelle de pescadores", REACH, "BLObj_Pescadores", radius=600.0,
                      on_complete=[radio("M03_Muelle"), radio("M03_MuelleTorre")]),
        objective_def("Elimina a los operadores de Corvane del espigón", CLEAR, "BLObj_Espigon", squad="Corvane",
                      on_start=[radio("M03_Operadores")]),
        objective_def("Coloca la baliza en el casco del barco", USE, "BLObjective_Baliza", on_complete=[radio("M03_Baliza")]),
        objective_def("Resiste en el espigón mientras el barco zarpa", DEFEND, "BLObj_Espigon", squad="Refuerzos", min_duration=35.0,
                      activate=["BLShip"], on_start=[radio("M03_Zarpa")],
                      waves=[wave(["BLWave_Lonja"], 3, 4.0, True),
                             wave(["BLWave_Almacenes"], 3, 8.0, False, operators=2)]),
        objective_def("Llega al furgón", REACH, "BLObj_Furgon", radius=400.0, on_start=[radio("M03_Furgon")]),
    ]
    K.director("RÍA", objectives,
               briefing=[radio("M03_Brief_01"), radio("M03_Brief_02"), radio("M03_Brief_03")],
               debriefing=[radio("M03_Debrief")],
               phases={"Casco": 1, "Casas": 2, "Campanario": 6, "Muelle": 7, "Baliza": 9})


def build():
    terrain()
    old_town()
    houses()
    big_square()
    docks()
    enemies()
    mission()
    K.dawn_fog_lighting()


K.run(MAP_PATH, build, nav_bounds=(-6000, 14500, -3200, 3000, -100, 1100),
      nav_checks=[("Inicio", (-4800, 600, 100)), ("Puente", (-2200, 600, 100)), ("FinPuente", (-700, 600, 100)),
                  ("PlazaPequena", (2100, 1300, 100)), ("Callejon", (2850, -1000, 100)), ("Casa1", (3500, -1000, 100)),
                  ("Casa1_Arriba", (3650, -1050, FH + 100)), ("Casa3", (5100, -1100, 100)), ("Buhardilla", (5300, -900, 2 * FH + 100)),
                  ("PlazaGrande", (7000, 0, 100)), ("Pescadores", (10000, -1600, 100)), ("Espigon", (13000, -2700, 100)),
                  ("Furgon", (13300, 2200, 100))],
      tag="BL_M03")
