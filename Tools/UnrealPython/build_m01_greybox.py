"""Construye /Game/Maps/M01/L_M01_AmanecerRoto: nivel gris del vertical slice (fases 1-4 de "Amanecer roto").

Trazado (cm; +X = este, hacia donde se avanza; +Y = sur):
  1 Inserción   calle trasera (x 0..6000, y -400..400): el furgón con las puertas abiertas; al fondo, muro.
  2 Infiltración dos callejones al norte llevan al patio de contenedores del puerto (x 1500..9000, y -6500..-2000):
                filas de contenedores (algunos apilados o abiertos) con varias rutas y puntos ciegos.
  3 Contacto    puerta del patio a la carretera (x 9000..10400): control con sacos, jerseys, caseta, barrera,
                coche calcinado y T-walls entre la puerta y el cruce.
  4 Calle       calle principal hacia el este (x 10400..19500, y -100..1300) con callejones laterales que forman
                rutas de flanqueo por detrás de las manzanas; al final, el local del objetivo (x 19500..21000).
Edificios = cajas (grey-box); coberturas, contenedores y vehículos = mallas finales (setup_level_assets.py).
Checkpoints por fase (ABLCheckpointVolume), NavMesh estático, amanecer frío con el sol de cara (este) y farolas de sodio.
TargetPoints "BLTest_Start_<Nombre>" para la prueba automática "Level".

Ejecutar (necesita frames para construir la navegación; NO usar -ExecutePythonScript, que cierra el editor al terminar):
  UnrealEditor.exe Blackline.uproject -ExecCmds="py <ruta>/Tools/UnrealPython/build_m01_greybox.py" -unattended
"""
import math
import os
import random

import unreal

MAP_PATH = "/Game/Maps/M01/L_M01_AmanecerRoto"
CUBE = unreal.load_asset("/Engine/BasicShapes/Cube.Cube")
ENV = "/Game/Environment/Materials/"
PROPS = "/Game/Environment/Props/"


def mat(name):
    return unreal.load_asset(ENV + name)


M_WALL = mat("MI_Env_ConcreteWall")
M_PLASTER = mat("MI_Env_Plaster")
M_FLOOR = mat("MI_Env_ConcreteFloor")
M_ASPHALT = mat("MI_Env_Asphalt")
M_DIRT = mat("MI_Env_Dirt")
M_RUST = mat("MI_Env_RustyMetal")
M_METAL = mat("MI_Env_MetalPlate")
M_WOOD = mat("MI_Env_WoodPlanks")
CONTAINER_MATS = [mat(f"MI_Env_Container_{c}") for c in ("Blue", "Red", "Green", "Orange")]

actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
level_ed = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
rng = random.Random(2031)
state = {"frame": 0, "phase": "build", "wait": 0, "handle": None, "nav_volume": None, "seen_building": False}


def log(m):
    unreal.log(f"[BL_M01] {m}")


def surface_of(m):
    n = m.get_name() if m else ""
    return "Metal" if ("Metal" in n or "Container" in n) else "Wood" if "Wood" in n else "Dirt" if "Dirt" in n else "Concrete"


def box(name, x0, x1, y0, y1, z0, z1, m=M_WALL, yaw=0.0):
    cx, cy, cz = (x0 + x1) / 2, (y0 + y1) / 2, (z0 + z1) / 2
    a = actors.spawn_actor_from_class(unreal.StaticMeshActor, unreal.Vector(cx, cy, cz), unreal.Rotator(roll=0, pitch=0, yaw=yaw))
    a.set_actor_label(name)
    c = a.static_mesh_component
    c.set_static_mesh(CUBE)
    c.set_material(0, m)
    a.set_actor_scale3d(unreal.Vector((x1 - x0) / 100, (y1 - y0) / 100, (z1 - z0) / 100))
    pm = unreal.load_asset(f"/Game/Environment/PhysicalMaterials/PM_{surface_of(m)}")
    if pm:
        c.set_phys_material_override(pm)
    return a


# Puntos de cobertura alrededor de cada malla (posición local cm, baja/alta). La IA comprueba la protección real
COVER_SPECS = {
    "SM_Cover_Sandbag_Straight": [(0, -75, True), (0, 75, True)],
    "SM_Cover_Sandbag_Corner": [(70, 80, True), (-80, 60, True), (40, -80, True)],
    "SM_Cover_Jersey": [(-85, -75, True), (85, -75, True), (-85, 75, True), (85, 75, True)],
    "SM_Cover_TWall": [(0, -100, False), (0, 100, False)],
    "SM_Container_20ft": [(-230, -175, False), (230, -175, False), (-230, 175, False), (230, 175, False), (-355, 0, False), (355, 0, False)],
    "SM_Container_20ft_Open": [(-230, -175, False), (230, -175, False), (-230, 175, False), (230, 175, False), (-355, 0, False)],
    "SM_Veh_Sedan_Wreck": [(-110, -135, True), (110, -135, True), (-110, 135, True), (110, 135, True)],
    "SM_Veh_Van_Civil_Open": [(-180, -150, False), (180, -150, False), (-180, 150, False), (180, 150, False)],
    "SM_Veh_Van_Civil": [(-180, -150, False), (180, -150, False), (-180, 150, False), (180, 150, False)],
    "SM_Prop_Dumpster": [(0, -110, True), (0, 110, True), (-150, 0, True), (150, 0, True)],
}
COVER_COUNT = [0]


def cover_point(x, y, toward_x, toward_y, low):
    yaw = math.degrees(math.atan2(toward_y - y, toward_x - x))
    c = actors.spawn_actor_from_class(unreal.BLCoverPoint, unreal.Vector(x, y, 0), unreal.Rotator(roll=0, pitch=0, yaw=yaw))
    c.set_editor_property("low_cover", low)
    c.set_actor_label(f"Cobertura_{COVER_COUNT[0]:03d}")
    COVER_COUNT[0] += 1
    return c


def covers_for_box(x0, x1, y0, y1, low):
    """Un punto en el centro de cada cara, a 55 cm."""
    cx, cy = (x0 + x1) / 2, (y0 + y1) / 2
    for px, py in ((x0 - 55, cy), (x1 + 55, cy), (cx, y0 - 55), (cx, y1 + 55)):
        cover_point(px, py, cx, cy, low)


def covers_for_building(x0, x1, y0, y1):
    """Esquinas de edificio: dos puntos por esquina (uno en cada fachada), coberturas altas para asomarse."""
    cx, cy = (x0 + x1) / 2, (y0 + y1) / 2
    for ex, sx in ((x0, -1), (x1, 1)):
        for ey, sy in ((y0, -1), (y1, 1)):
            cover_point(ex + sx * 55, ey - sy * 90, cx, ey - sy * 90, False)
            cover_point(ex - sx * 90, ey + sy * 55, ex - sx * 90, cy, False)


def prop(mesh, x, y, yaw=0.0, z=0.0, label=None, material=None):
    sm = unreal.load_asset(PROPS + mesh)
    a = actors.spawn_actor_from_class(unreal.StaticMeshActor, unreal.Vector(x, y, z), unreal.Rotator(roll=0, pitch=0, yaw=yaw))
    a.static_mesh_component.set_static_mesh(sm)
    if material:
        a.static_mesh_component.set_material(0, material)
    a.set_actor_label(label or mesh)
    if z < 1.0:
        c, s_ = math.cos(math.radians(yaw)), math.sin(math.radians(yaw))
        for lx, ly, low in COVER_SPECS.get(mesh, []):
            cover_point(x + lx * c - ly * s_, y + lx * s_ + ly * c, x, y, low)
    return a


# Zonas por las que se mueve el jugador: las fachadas que dan a ellas tienen locales y portales en la planta baja
STREETS = [(0, 6000, -400, 400), (9000, 10400, -8000, 1600), (10400, 19500, -100, 1300)]
FACADE_WALLS = ["Cream", "Ochre", "Salmon", "Grey", "Green", "Concrete"]
BUILDINGS = []


def building(name, x0, x1, y0, y1, h, m=None, corners=True):
    """Edificio de fachadas modulares (ABLBuilding, Bloque 8). Se crea al final (finish_buildings) para saber
    qué caras tocan a otro edificio. Coberturas altas en sus esquinas."""
    if corners:
        covers_for_building(x0, x1, y0, y1)
    BUILDINGS.append((name, x0, x1, y0, y1, h))


def finish_buildings():
    def touches(face, b, o):
        _, x0, x1, y0, y1, _ = b
        _, a0, a1, b0, b1, _ = o
        if face in (0, 1):   # +X / -X
            x = x1 if face == 0 else x0
            return abs(x - (a0 if face == 0 else a1)) < 1 and min(y1, b1) - max(y0, b0) > 100
        y = y1 if face == 2 else y0
        return abs(y - (b0 if face == 2 else b1)) < 1 and min(x1, a1) - max(x0, a0) > 100

    def in_street(px, py):
        return any(a <= px <= b and c <= py <= d for a, b, c, d in STREETS)

    count = 0
    for i, b in enumerate(BUILDINGS):
        name, x0, x1, y0, y1, h = b
        cx, cy = (x0 + x1) / 2, (y0 + y1) / 2
        face_mask, street_mask = 0, 0
        probes = [(x1 + 300, cy), (x0 - 300, cy), (cx, y1 + 300), (cx, y0 - 300)]
        for f in range(4):
            if not any(touches(f, b, o) for j, o in enumerate(BUILDINGS) if j != i):
                face_mask |= 1 << f
            if in_street(*probes[f]):
                street_mask |= 1 << f
        a = actors.spawn_actor_from_class(unreal.BLBuilding, unreal.Vector(cx, cy, 0))
        a.set_actor_label(name)
        a.set_editor_property("size", unreal.Vector(x1 - x0, y1 - y0, h))
        a.set_editor_property("seed", 1000 + i * 37)
        a.set_editor_property("face_mask", face_mask)
        a.set_editor_property("street_mask", street_mask)
        a.set_editor_property("wall_material", mat(f"MI_Fac_Wall_{rng.choice(FACADE_WALLS)}"))
        a.set_editor_property("phys_material", unreal.load_asset("/Game/Environment/PhysicalMaterials/PM_Concrete"))
        count += a.get_instance_count()
    log(f"Edificios: {len(BUILDINGS)}, instancias de fachada: {count}")


def start_point(name, x, y, yaw=0.0, z=0.0):
    tp = actors.spawn_actor_from_class(unreal.TargetPoint, unreal.Vector(x, y, z), unreal.Rotator(roll=0, pitch=0, yaw=yaw))
    tp.set_actor_label(f"BLTest_Start_{name}")
    tp.tags = [unreal.Name(f"BLTest_Start_{name}")]
    return tp


def checkpoint(cid, x0, x1, y0, y1, yaw, z=0.0):
    """Volumen de checkpoint que cubre todo el paso (x0..x1, y0..y1); se reaparece en su centro mirando a yaw."""
    cp = actors.spawn_actor_from_class(unreal.BLCheckpointVolume, unreal.Vector((x0 + x1) / 2, (y0 + y1) / 2, 120 + z))
    cp.set_actor_label(f"Checkpoint_{cid}")
    cp.set_editor_property("checkpoint_id", cid)
    cp.set_actor_scale3d(unreal.Vector((x1 - x0) / 300, (y1 - y0) / 300, 1))   # caja base de 300 x 300 x 240
    arrow = cp.get_component_by_class(unreal.ArrowComponent)
    arrow.set_world_rotation(unreal.Rotator(roll=0, pitch=0, yaw=yaw), False, False)
    arrow.set_world_scale3d(unreal.Vector(1, 1, 1))
    return cp


def sodium_light(name, x, y, yaw, shadows=False):
    """Farola de sodio (acento naranja del diseño): poste + brazo + luz puntual."""
    box(name + "_Poste", x - 8, x + 8, y - 8, y + 8, 0, 700, M_METAL)
    dx, dy = math.cos(math.radians(yaw)) * 120, math.sin(math.radians(yaw)) * 120
    box(name + "_Brazo", min(x, x + dx) - 5, max(x, x + dx) + 5, min(y, y + dy) - 5, max(y, y + dy) + 5, 690, 700, M_METAL)
    l = actors.spawn_actor_from_class(unreal.PointLight, unreal.Vector(x + dx, y + dy, 670))
    l.set_actor_label(name)
    lc = l.get_component_by_class(unreal.PointLightComponent)
    lc.set_mobility(unreal.ComponentMobility.MOVABLE)
    lc.set_intensity(7.0)                     # candelas: luz de calle tenue al amanecer
    lc.set_light_color(unreal.LinearColor(1.0, 0.55, 0.2, 1))
    lc.set_attenuation_radius(1600.0)
    lc.set_cast_shadows(shadows)
    lc.set_editor_property("intensity_units", unreal.LightUnits.CANDELAS)
    return l


# ---------------------------------------------------------------------------
# Fases
# ---------------------------------------------------------------------------

def phase1_insertion():
    # Calle trasera: 8 m entre fachadas; muro al oeste, verja cerrada al este (obliga a ir por los callejones)
    box("Calle_Trasera", 0, 6000, -400, 400, -2, 0, M_ASPHALT)
    building("N1", 0, 2800, -1600, -400, 960)
    building("N2", 3300, 4800, -1600, -400, 1280)
    building("N3", 5400, 6000, -1600, -400, 1120)
    building("S1", 0, 2200, 400, 1600, 1280)
    building("S2", 2200, 4500, 400, 1600, 960)
    building("S3", 4500, 6000, 400, 1600, 1120)
    box("Muro_Oeste", -100, 0, -400, 400, 0, 350, M_WALL)
    box("Verja_Este", 5960, 6000, -400, 400, 0, 260, M_RUST)
    prop("SM_Veh_Van_Civil_Open", 800, 160, 0, label="Furgon_Insercion")
    prop("SM_Container_20ft", 4300, 250, 2, label="Contenedor_Calle", material=CONTAINER_MATS[3])
    for i, (x, y) in enumerate(((1900, -330), (2500, 335), (3700, -335))):
        prop("SM_Prop_Dumpster", x, y, rng.uniform(-4, 4), label=f"Contenedor_Basura{i}")
    actors.spawn_actor_from_class(unreal.PlayerStart, unreal.Vector(300, -170, 100), unreal.Rotator(roll=0, pitch=0, yaw=0))
    start_point("Fase1", 300, -170, 0)


def phase2_infiltration():
    # Callejones A y B hacia el norte
    box("Callejon_A", 2800, 3300, -1600, -400, -2, 0, M_FLOOR)
    box("Callejon_B", 4800, 5400, -1600, -400, -2, 0, M_FLOOR)
    box("Callejon_B_Valla", 4800, 5400, -1250, -1220, 0, 110, M_RUST)   # obstáculo saltable (mantle)
    # Patio del puerto
    # El patio llega hasta las fachadas traseras de la calle (y = -1600): solo se entra por los callejones
    # y solo se sale a la carretera por la puerta (antes un pasillo junto a las fachadas era un atajo)
    box("Patio", 1500, 9000, -6500, -1600, -2, 0, M_FLOOR)
    box("Patio_Muro_Norte", 1500, 9000, -6540, -6500, 0, 400, M_WALL)
    box("Patio_Muro_Oeste", 1460, 1500, -6540, -1600, 0, 400, M_WALL)
    box("Patio_Valla_Este_N", 8960, 9000, -6500, -4500, 0, 260, M_RUST)
    box("Patio_Valla_Este_S", 8960, 9000, -3700, -1600, 0, 260, M_RUST)
    box("Nave_Puerto", 1600, 3800, -6450, -5200, 0, 1100, M_METAL)       # nave al fondo (silueta)
    # Filas de contenedores (x, y, yaw, apilado, abierto); los huecos forman las rutas norte-sur
    rows = {
        -2700: [(2300, 0, 1, 0), (4250, 0, 0, 0), (6100, 0, 1, 0), (7800, 0, 0, 1)],
        -3500: [(3300, 90, 0, 0), (5300, 0, 2, 0), (7000, 0, 0, 0)],
        -4300: [(2400, 0, 0, 1), (3900, 0, 1, 0), (6000, 0, 0, 0), (8200, 90, 1, 0)],
        -5300: [(4600, 0, 1, 0), (6400, 0, 2, 0), (7900, 0, 0, 0)],
        -6100: [(4300, 0, 0, 0), (6600, 0, 0, 0), (8300, 0, 1, 0)],
    }
    n = 0
    for y, items in rows.items():
        for x, yaw, stack, opened in items:
            yaw += rng.uniform(-3, 3)
            for level in range(stack + 1):
                mesh = "SM_Container_20ft_Open" if (opened and level == 0) else "SM_Container_20ft"
                prop(mesh, x + rng.uniform(-20, 20), y + rng.uniform(-15, 15), yaw + rng.uniform(-2, 2), z=level * 259,
                     label=f"Contenedor_{n}", material=rng.choice(CONTAINER_MATS))
                n += 1
    for i, (x, y) in enumerate(((3050, -3000), (5200, -4800), (7300, -3100), (6900, -5800))):  # palés y bidones (cobertura baja)
        box(f"Pales{i}", x - 60, x + 60, y - 50, y + 50, 0, 115, M_WOOD)
        covers_for_box(x - 60, x + 60, y - 50, y + 50, True)
    start_point("Fase2", 3050, -2300, -90)
    checkpoint("Infiltracion", 2800, 3300, -1500, -1100, -90)


def phase3_roadblock():
    box("Carretera", 9000, 10400, -8000, 1600, -2, 0, M_ASPHALT)
    box("Acera_Oeste", 9000, 9300, -8000, -2000, 0, 15, M_FLOOR)
    box("Acera_Este", 10100, 10400, -8000, -100, 0, 15, M_FLOOR)
    # Manzana entre la calle trasera y la carretera; edificios al este de la carretera
    building("M1", 6000, 9000, -1600, 1600, 1280)
    box("Carretera_Muro_NO", 8960, 9000, -8000, -6500, 0, 400, M_WALL)
    building("E1", 10400, 12000, -8000, -5200, 1440)
    building("E2", 10400, 12000, -5000, -3000, 960)
    building("E3", 10400, 12400, -2800, -100, 1120)
    box("E_Hueco_Muro", 10400, 10440, -5200, -5000, 0, 300, M_WALL)
    box("E_Hueco_Muro2", 10400, 10440, -3000, -2800, 0, 300, M_WALL)
    box("Carretera_Norte", 9000, 10400, -8040, -8000, 0, 500, M_WALL)     # fin del nivel al norte
    box("Carretera_Sur", 9000, 10400, 1600, 1640, 0, 500, M_WALL)
    # Control de carretera entre la puerta del patio (y ~ -4100) y el cruce (y ~ 600)
    cy = -2400
    prop("SM_Cover_Jersey", 9450, cy - 600, 90, label="Jersey_1")
    prop("SM_Cover_Jersey", 9950, cy - 250, 90, label="Jersey_2")
    prop("SM_Cover_Jersey", 9450, cy + 350, 90, label="Jersey_3")
    prop("SM_Cover_Sandbag_Corner", 9250, cy - 150, 0, label="Sacos_O1")
    prop("SM_Cover_Sandbag_Straight", 9370, cy - 210, 0, label="Sacos_O2")
    prop("SM_Cover_Sandbag_Straight", 9250, cy - 30, 90, label="Sacos_O3")
    prop("SM_Cover_Sandbag_Corner", 10230, cy + 150, 180, label="Sacos_E1")
    prop("SM_Cover_Sandbag_Straight", 10110, cy + 210, 0, label="Sacos_E2")
    prop("SM_Cover_Sandbag_Straight", 10230, cy + 30, 90, label="Sacos_E3")
    box("Caseta", 10150, 10350, cy + 500, cy + 700, 0, 260, M_METAL)
    covers_for_box(10150, 10350, cy + 500, cy + 700, False)
    box("Caseta_Techo", 10130, 10370, cy + 480, cy + 720, 260, 275, M_RUST)
    box("Barrera_Poste", 9310, 9350, cy + 880, cy + 920, 0, 110, M_METAL)
    box("Barrera_Brazo", 9350, 10150, cy + 890, cy + 910, 95, 105, M_RUST)
    prop("SM_Veh_Sedan_Wreck", 9700, cy + 1200, 75, label="Coche_Control")
    for i, x in enumerate((9150, 10250)):
        prop("SM_Cover_TWall", x, cy + 1550, 0, label=f"TWall_Control_{i}")
    sodium_light("Farola_Control_1", 9320, cy - 400, 0, shadows=True)
    sodium_light("Farola_Control_2", 10080, cy + 900, 180)
    start_point("Fase3", 9600, -4100, 90)
    checkpoint("Contacto", 9000, 10400, -4500, -3700, 90)


def phase4_street():
    box("Calle_Principal", 10400, 19500, -100, 1300, -2, 0, M_ASPHALT)
    box("Acera_Norte", 10400, 19500, -100, 200, 0, 15, M_FLOOR)
    box("Acera_Sur", 10400, 19500, 1000, 1300, 0, 15, M_FLOOR)
    # Manzana norte con callejón C (x 12400..12900) y D (15400..15900) unidos por un patio trasero (flanqueo)
    building("B2", 12900, 15400, -1700, -100, 960)
    building("B3", 15900, 17800, -1700, -100, 1600)
    building("B4", 17800, 19500, -1700, -100, 1120)
    box("Callejon_C", 12400, 12900, -2700, -100, -2, 0, M_FLOOR)
    box("Callejon_D", 15400, 15900, -2700, -100, -2, 0, M_FLOOR)
    box("Patio_Trasero_N", 12400, 15900, -2700, -1700, -2, 0, M_DIRT)
    box("Patio_Trasero_N_Muro", 12000, 16300, -2840, -2700, 0, 400, M_WALL)
    box("Patio_Trasero_N_MuroO", 12000, 12400, -2700, -1700, 0, 400, M_WALL)
    box("Patio_Trasero_N_MuroE", 15900, 16300, -2700, -1700, 0, 400, M_WALL)
    box("Patio_N_Cobertura", 13600, 14200, -2300, -2200, 0, 115, M_WALL)
    covers_for_box(13600, 14200, -2300, -2200, True)
    # Manzana sur con callejones E (13500..14000) y F (16500..17000) unidos por un pasaje trasero
    building("C1", 10400, 13500, 1300, 2900, 1120)
    building("C2", 14000, 16500, 1300, 2900, 1280)
    apartment_block()   # C3: bloque de viviendas con interiores (Bloque 11, fases 5-6)
    box("Callejon_E", 13500, 14000, 1300, 3900, -2, 0, M_FLOOR)
    box("Callejon_F", 16500, 17000, 1300, 3900, -2, 0, M_FLOOR)
    box("Pasaje_Sur", 13500, 17000, 2900, 3900, -2, 0, M_DIRT)
    box("Pasaje_Sur_Muro", 13100, 17400, 3900, 4040, 0, 400, M_WALL)
    box("Pasaje_Sur_MuroO", 13100, 13500, 2900, 3900, 0, 400, M_WALL)
    box("Pasaje_Sur_MuroE", 17000, 17400, 2900, 3900, 0, 400, M_WALL)
    for i, (x, y) in enumerate(((14600, 3400), (15800, 3200))):
        box(f"Pasaje_Contenedor{i}", x - 90, x + 90, y - 55, y + 55, 0, 120, CONTAINER_MATS[2])
    # Coberturas de la calle (alternando lados para avanzar de una a otra)
    prop("SM_Veh_Sedan_Wreck", 11800, 450, 12, label="Coche_Calle_1")
    prop("SM_Cover_Jersey", 12950, 950, 25, label="Jersey_Calle_1")
    prop("SM_Cover_TWall", 13800, 380, 90, label="TWall_Calle_1")
    prop("SM_Cover_TWall", 13800, 530, 90, label="TWall_Calle_2")
    prop("SM_Veh_Sedan_Wreck", 14700, 900, -165, label="Coche_Calle_2")
    prop("SM_Cover_Jersey", 15500, 330, -15, label="Jersey_Calle_2")
    prop("SM_Container_20ft", 16300, 1080, 4, label="Contenedor_Calle_2", material=CONTAINER_MATS[1])
    prop("SM_Cover_Jersey", 17000, 600, 90, label="Jersey_Calle_3")
    # Posición enemiga de sacos delante del objetivo
    prop("SM_Cover_Sandbag_Corner", 18400, 350, 180, label="Sacos_Calle_1")
    prop("SM_Cover_Sandbag_Straight", 18280, 410, 0, label="Sacos_Calle_2")
    prop("SM_Cover_Sandbag_Straight", 18400, 530, 90, label="Sacos_Calle_3")
    prop("SM_Cover_Sandbag_Straight", 18400, 950, 90, label="Sacos_Calle_4")
    for i, (x, y) in enumerate(((12200, 230), (15100, 1150), (17600, 1130))):
        box(f"Escombros{i}", x - 70, x + 70, y - 60, y + 60, 0, 80, M_WALL, yaw=rng.uniform(0, 90))
        covers_for_box(x - 70, x + 70, y - 60, y + 60, True)
    for i, (x, y, yaw) in enumerate(((11400, 1120, -90), (13300, 120, 90), (16000, 1120, -90), (18000, 120, 90))):
        sodium_light(f"Farola_Calle_{i}", x, y, yaw, shadows=(i == 1))
    start_point("Fase4", 9700, 600, 0)
    checkpoint("Calle", 9000, 10400, -100, 1300, 0)


def objective():
    # Local del objetivo: planta baja abierta (persiana levantada) y plantas superiores macizas
    x0, x1, y0, y1 = 19500, 21000, -100, 1300
    box("Local_Suelo", x0, x1, y0, y1, -2, 0, M_FLOOR)
    box("Local_Fachada_N", x0, x0 + 30, y0, 200, 0, 350, M_PLASTER)
    box("Local_Fachada_S", x0, x0 + 30, 1000, y1, 0, 350, M_PLASTER)
    box("Local_Dintel", x0, x0 + 30, 200, 1000, 300, 350, M_PLASTER)
    box("Local_Persiana", x0 - 10, x0 + 30, 200, 1000, 300, 340, M_RUST)
    box("Local_Pared_N", x0, x1, y0 - 30, y0, 0, 350, M_PLASTER)
    box("Local_Pared_S", x0, x1, y1, y1 + 30, 0, 350, M_PLASTER)
    box("Local_Pared_E", x1 - 30, x1, y0, y1, 0, 350, M_PLASTER)
    # Plantas de viviendas encima del local (fachada hacia la calle y los laterales; detrás, el límite del nivel)
    up = actors.spawn_actor_from_class(unreal.BLBuilding, unreal.Vector((x0 + x1) / 2, (y0 + y1) / 2, 350))
    up.set_actor_label("Local_Plantas")
    up.set_editor_property("size", unreal.Vector(x1 - x0, y1 - y0 + 60, 960))
    up.set_editor_property("seed", 777)
    up.set_editor_property("face_mask", 1 | 2 | 4 | 8)
    up.set_editor_property("street_mask", 0)
    up.set_editor_property("no_ground_floor", True)
    up.set_editor_property("wall_material", mat("MI_Fac_Wall_Ochre"))
    up.set_editor_property("phys_material", unreal.load_asset("/Game/Environment/PhysicalMaterials/PM_Concrete"))
    box("Local_Mostrador", 20100, 20250, 300, 900, 0, 105, M_WOOD)
    covers_for_box(20100, 20250, 300, 900, True)
    box("Local_Mesa", 20550, 20750, 500, 700, 0, 75, M_WOOD)
    # Fluorescente frío en el techo (el interior quedaba negro)
    box("Local_Fluorescente", 20300, 20500, 580, 620, 335, 345, M_METAL)
    lamp = actors.spawn_actor_from_class(unreal.RectLight, unreal.Vector(20400, 600, 333), unreal.Rotator(roll=0, pitch=-90, yaw=0))
    lamp.set_actor_label("Local_Luz")
    lc = lamp.get_component_by_class(unreal.RectLightComponent)
    lc.set_mobility(unreal.ComponentMobility.MOVABLE)
    lc.set_editor_property("intensity_units", unreal.LightUnits.CANDELAS)
    lc.set_intensity(60.0)
    lc.set_light_color(unreal.LinearColor(0.85, 0.95, 1.0, 1))
    lc.set_editor_property("source_width", 200.0)
    lc.set_editor_property("source_height", 30.0)
    lc.set_attenuation_radius(1400.0)
    obj = actors.spawn_actor_from_class(unreal.TargetPoint, unreal.Vector(20650, 600, 80))
    obj.set_actor_label("Objetivo_Disco")
    obj.tags = [unreal.Name("BLObjective_Disco")]
    start_point("Objetivo", 20300, 600, 0)
    checkpoint("Objetivo", 19530, 20970, -70, 1270, 0)


# ---------------------------------------------------------------------------
# Bloque de viviendas (Bloque 11, fases 5-6): manzana C3 con interiores
# ---------------------------------------------------------------------------
BX0, BX1, BY0, BY1 = 17000, 19500, 1300, 2900
CX0, CX1 = 17770, 18630      # tramo de fachada norte que derriba el blindado (fase 8)
FH = 320.0
WT = 25.0


def wall_x(name, y0, y1, xa, xb, z0, z1, holes, m):
    """Muro a lo largo de X (grosor y0..y1) con huecos [(x0, x1, zb, zt)] (ventanas y puertas)."""
    holes = sorted(holes)
    x = xa
    for i, (h0, h1, hb, ht) in enumerate(holes):
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
    """Muro a lo largo de Y (grosor x0..x1) con huecos [(y0, y1, zb, zt)]."""
    holes = sorted(holes)
    y = ya
    for i, (h0, h1, hb, ht) in enumerate(holes):
        if h0 > y:
            box(f"{name}_S{i}", x0, x1, y, h0, z0, z1, m)
        if hb > z0:
            box(f"{name}_B{i}", x0, x1, h0, h1, z0, hb, m)
        if ht < z1:
            box(f"{name}_T{i}", x0, x1, h0, h1, ht, z1, m)
        y = h1
    if y < yb:
        box(f"{name}_End", x0, x1, y, yb, z0, z1, m)


def win(c, z, w=140.0):
    return (c - w / 2, c + w / 2, z + 90.0, z + 240.0)


def door(c, z, w=150.0):
    """Puerta de 150 cm: con 110 el NavMesh (radio del agente 34 + rasterizado de los muros) la daba por cerrada."""
    return (c - w / 2, c + w / 2, z, z + 230.0)


def stair_flight(name, xa, xb, y_start, direction, z_base, steps=9, rise=160.0 / 9, run=28.0, m=None):
    """Tramo de escalera: escalones de huella 28 y contrahuella ~17,8, losa fina (se pasa por debajo)."""
    for i in range(steps):
        ya = y_start + direction * run * i
        yb = ya + direction * run
        top = z_base + rise * (i + 1)
        box(f"{name}_{i}", xa, xb, min(ya, yb), max(ya, yb), top - 40.0, top, m or M_FLOOR)


def furniture(prefix, x, y, z, kind, yaw_x=True):
    """Muebles sencillos (cajas) que sirven de cobertura baja dentro de los pisos."""
    if kind == "mesa":
        box(f"{prefix}_Mesa", x - 60, x + 60, y - 40, y + 40, z, z + 75, M_WOOD)
        covers_for_box(x - 60, x + 60, y - 40, y + 40, True)
    elif kind == "sofa":
        box(f"{prefix}_Sofa", x - 100, x + 100, y - 40, y + 40, z, z + 80, M_DIRT)
        covers_for_box(x - 100, x + 100, y - 40, y + 40, True)
    elif kind == "cama":
        box(f"{prefix}_Cama", x - 75, x + 75, y - 100, y + 100, z, z + 50, M_PLASTER)
    elif kind == "armario":
        box(f"{prefix}_Armario", x - 50, x + 50, y - 30, y + 30, z, z + 200, M_WOOD)
    elif kind == "barricada":   # muebles volcados en el pasillo: cobertura baja
        box(f"{prefix}_Barricada", x - 70, x + 70, y - 40, y + 40, z, z + 110, M_WOOD, yaw=rng.uniform(-15, 15))
        covers_for_box(x - 70, x + 70, y - 40, y + 40, True)


def interior_light(name, x, y, z, warm=True, intensity=30.0, radius=700.0):
    """Luz interior sin sombras (barata): bombilla cálida en los pisos, fluorescente frío en zonas comunes."""
    l = actors.spawn_actor_from_class(unreal.PointLight, unreal.Vector(x, y, z))
    l.set_actor_label(name)
    c = l.get_component_by_class(unreal.PointLightComponent)
    c.set_mobility(unreal.ComponentMobility.MOVABLE)
    c.set_editor_property("intensity_units", unreal.LightUnits.CANDELAS)
    c.set_intensity(intensity)
    c.set_light_color(unreal.LinearColor(1.0, 0.72, 0.45, 1) if warm else unreal.LinearColor(0.82, 0.92, 1.0, 1))
    c.set_attenuation_radius(radius)
    c.set_cast_shadows(False)
    c.set_editor_property("source_radius", 4.0)
    # Solo cuestan de cerca: con 21 luces encendidas en todo el nivel la misión perdía ~4 ms
    c.set_editor_property("max_draw_distance", 2500.0)
    c.set_editor_property("max_distance_fade_range", 500.0)
    return l


def apartment_block():
    """Bloque C3 (x 170-195 m, y 13-29 m): portal a la calle principal, escalera de ida y vuelta al este,
    pasillo central y 4 pisos por planta (2 al norte, 2 al sur), 3 plantas + azotea con caseta de escalera.
    Ventanas abiertas (se dispara por ellas). Muros de 25 cm; enfoscado salmón por fuera y por dentro."""
    covers_for_building(BX0, BX1, BY0, BY1)
    W = mat("MI_Fac_Wall_Salmon")
    IN = M_PLASTER
    x0, x1, y0, y1 = BX0, BX1, BY0, BY1
    SX0, SX1, SY0, SY1 = 18950, x1 - WT, y0 + WT, 1900       # hueco de la escalera
    for k in range(3):
        z = k * FH
        zt = z + FH
        # ---- Fachadas ----
        north = [win(c, z) for c in ((17300, 17700, 18200, 18700) if k else (17700, 18700))] + [win(19200, z, 100)]
        if k == 0:
            north.append(door(18410, z, 180))
        wall_x(f"Bloque_N{k}", y0, y0 + WT, x0, x1, z, zt, north, W)
        wall_x(f"Bloque_S{k}", y1 - WT, y1, x0, x1, z, zt, [win(c, z) for c in (17300, 17800, 18500, 19000, 19300)], W)
        wall_y(f"Bloque_O{k}", x0, x0 + WT, y0 + WT, y1 - WT, z, zt, [win(1600, z), win(2500, z)], W)
        wall_y(f"Bloque_E{k}", x1 - WT, x1, y0 + WT, y1 - WT, z, zt, [win(1600, z, 100), win(2500, z)], W)
        # ---- Pasillo: muros norte (y 1925) y sur (y 2100) con las puertas de los pisos ----
        if k == 0:
            # Planta baja: el norte es el portal (abierto al pasillo); solo el piso A tiene muro
            wall_x(f"Bloque_PN{k}", 1925, 1950, x0 + WT, 17975, z, zt, [door(17760, z)], IN)
        else:
            wall_x(f"Bloque_PN{k}", 1925, 1950, x0 + WT, SX0 - WT, z, zt, [door(17760, z), door(18450, z)], IN)
        # (las puertas no pueden caer sobre un tabique: C entra por su habitación este y D por la oeste)
        wall_x(f"Bloque_PS{k}", 2100, 2125, x0 + WT, x1 - WT, z, zt, [door(17950, z), door(18560, z)], IN)
        # Separación entre pisos y tabiques interiores (con su puerta)
        wall_y(f"Bloque_AB{k}", 17975, 18000, y0 + WT, 1925, z, zt, [] if k else [door(1600, z)], IN)
        wall_y(f"Bloque_CD{k}", 18250, 18275, 2125, y1 - WT, z, zt, [], IN)
        wall_y(f"Bloque_Ct{k}", 17625, 17650, 2125, y1 - WT, z, zt, [door(2450, z)], IN)
        wall_y(f"Bloque_Dt{k}", 18875, 18900, 2125, y1 - WT, z, zt, [door(2650, z)], IN)
        wall_y(f"Bloque_At{k}", 17500, 17525, y0 + WT, 1925, z, zt, [door(1700, z)], IN)
        wall_y(f"Bloque_Esc{k}", SX0 - WT, SX0, y0 + WT, 1925, z, zt, [], IN)     # muro oeste de la escalera
        # ---- Escalera: tramo A hacia el norte, rellano, tramo B hacia el sur ----
        stair_flight(f"Bloque_EscA{k}", SX0 + 10, 19200, 1900, -1, z)
        box(f"Bloque_Rellano{k}", SX0 + 10, SX1 - 10, SY0 + 15, 1648, z + 140, z + 160, M_FLOOR)
        stair_flight(f"Bloque_EscB{k}", 19220, SX1 - 10, 1648, 1, z + 160)
        # ---- Forjado de la planta de arriba (con el hueco de la escalera) ----
        box(f"Bloque_Forjado{k + 1}_O", x0 + WT, SX0, y0 + WT, y1 - WT, zt - 20, zt, M_FLOOR)
        box(f"Bloque_Forjado{k + 1}_E", SX0, x1 - WT, SY1, y1 - WT, zt - 20, zt, M_FLOOR)
        # ---- Luces: bombilla en cada piso (alguna apagada), fluorescentes en pasillo y escalera ----
        ceil = zt - 30
        for i, (lx, ly) in enumerate(((17280, 1620), (18450, 1620), (17350, 2500), (17950, 2500), (18550, 2500), (19150, 2500))):
            if (k * 7 + i) % 5 != 3:
                interior_light(f"Bloque_Luz{k}_{i}", lx, ly, ceil, True, 28.0, 650.0)
        interior_light(f"Bloque_LuzPasillo{k}_O", 17600, 2025, ceil, False, 22.0, 650.0)
        interior_light(f"Bloque_LuzPasillo{k}_E", 18600, 2025, ceil, False, 22.0, 650.0)
        interior_light(f"Bloque_LuzEscalera{k}", 19210, 1500, z + 300, False, 26.0, 700.0)
        # ---- Muebles ----
        furniture(f"Bloque_P{k}A", 17250, 1600, z, "mesa")
        furniture(f"Bloque_P{k}A2", 17760, 1500, z, "armario")
        if k:
            furniture(f"Bloque_P{k}B", 18200, 1550, z, "sofa")
            furniture(f"Bloque_P{k}B2", 18700, 1450, z, "cama")
        furniture(f"Bloque_P{k}C", 17350, 2650, z, "cama" if k != 2 else "mesa")
        furniture(f"Bloque_P{k}C2", 17950, 2500, z, "sofa")
        furniture(f"Bloque_P{k}D", 18550, 2550, z, "mesa")
        furniture(f"Bloque_P{k}D2", 19200, 2700, z, "armario")
        if k == 0:
            furniture("Bloque_PortalBarricada", 18600, 1550, z, "barricada")
    box("Bloque_Suelo", x0, x1, y0, y1, -2, 0, M_FLOOR)
    # Remates de fachada: impostas, alféizares y cornisa (lectura de plantas desde la calle)
    for k in (1, 2):
        z = k * FH
        box(f"Bloque_ImpN{k}", x0 - 6, x1 + 6, y0 - 8, y0, z - 10, z + 8, M_WALL)
        box(f"Bloque_ImpS{k}", x0 - 6, x1 + 6, y1, y1 + 8, z - 10, z + 8, M_WALL)
    # Zócalo a ambos lados del portal (por delante sería un escalón de 50 cm, más que el paso máximo de 42)
    box("Bloque_Zocalo_O", x0 - 4, 18320, y0 - 4, y0, 0, 50, M_WALL)
    box("Bloque_Zocalo_E", 18500, x1 + 4, y0 - 4, y0, 0, 50, M_WALL)
    # ---- Azotea: losa, peto, caseta de la escalera con su puerta ----
    zr = 3 * FH
    box("Bloque_Azotea_O", x0, SX0, y0, y1, zr - 20, zr, M_FLOOR)
    box("Bloque_Azotea_E", SX0, x1, SY1, y1, zr - 20, zr, M_FLOOR)
    for name, b in (("N_O", (x0, CX0, y0, y0 + WT)), ("N_C", (CX0, CX1, y0, y0 + WT)), ("N_E", (CX1, x1, y0, y0 + WT)),
                    ("S", (x0, x1, y1 - WT, y1)), ("O", (x0, x0 + WT, y0, y1)),
                    ("E_N", (x1 - WT, x1, y0, 2350)), ("E_S", (x1 - WT, x1, 2550, y1))):   # hueco este: pasarela a C4
        box(f"Bloque_Peto{name}", b[0], b[1], b[2], b[3], zr, zr + 105, W)
    # Cornisa perimetral (solo por fuera: una losa entera tapaba el hueco de la escalera)
    for name, b in (("N_O", (x0 - 15, CX0, y0 - 15, y0)), ("N_C", (CX0, CX1, y0 - 15, y0)), ("N_E", (CX1, x1 + 15, y0 - 15, y0)),
                    ("S", (x0 - 15, x1 + 15, y1, y1 + 15)),
                    ("O", (x0 - 15, x0, y0, y1)), ("E", (x1, x1 + 15, y0, y1))):
        box(f"Bloque_Cornisa{name}", b[0], b[1], b[2], b[3], zr - 25, zr - 5, M_WALL)
    ph = zr + 280
    box("Bloque_Caseta_O", SX0 - WT, SX0, y0, 1950, zr, ph, W)
    box("Bloque_Caseta_E", x1 - WT, x1, y0, 1950, zr, ph, W)
    box("Bloque_Caseta_N", SX0 - WT, x1, y0, y0 + WT, zr, ph, W)
    wall_x("Bloque_Caseta_S", 1925, 1950, SX0 - WT, x1, zr, ph, [(19230, 19440, zr, zr + 225)], W)
    box("Bloque_Caseta_Techo", SX0 - WT - 10, x1 + 10, y0 - 10, 1960, ph, ph + 20, M_WALL)
    box("Bloque_Caseta_Suelo", SX0, x1 - WT, 1900, 1925, zr - 20, zr, M_FLOOR)
    # ---- Fase 8: lo que derriba el blindado (paño de la 2.ª planta entre ventanas, peto y cornisa encima) ----
    collapse = {"Bloque_N2_S2", "Bloque_N2_B2", "Bloque_N2_T2", "Bloque_N2_S3", "Bloque_PetoN_C", "Bloque_CornisaN_C"}
    for a in actors.get_all_level_actors():
        if a.get_actor_label() in collapse:
            a.static_mesh_component.set_mobility(unreal.ComponentMobility.MOVABLE)
            a.tags = [unreal.Name("BLCollapse")]
    # Escombros que tapan el primer tramo de la escalera (aparecen con el derrumbe si el jugador ya está arriba)
    for i, (bx0, bx1, by0, by1, bz1) in enumerate(((SX0 + 10, 19200, 1650, 1900, 150), (19220, SX1 - 10, 1648, 1800, 230),
                                                    (SX0 + 10, SX1 - 10, SY0 + 15, 1650, 200))):
        r = box(f"Bloque_EscombrosEscalera{i}", bx0, bx1, by0, by1, 0, bz1, M_WALL)
        r.tags = [unreal.Name("BLCollapseBlock")]
        r.static_mesh_component.set_editor_property("can_ever_affect_navigation", False)   # el NavMesh es el de antes del derrumbe
    for i, (tx, tz) in enumerate(((17500, 470), (18000, 800), (18900, 820))):
        marker("BLBTR_Target", tx, y0 - 10, tz)
    # ---- Gameplay: marcadores, Varek y la escuadra que lo retiene ----
    marker("BLObj_Bloque", 18410, 1450)
    start_point("Bloque", 18410, 800, 90)
    marker("BLObj_VarekPiso", 17300, 2600)
    vz = 2 * FH
    varek = actors.spawn_actor_from_class(unreal.BLVarek, unreal.Vector(17200, 2600, vz + 96), unreal.Rotator(roll=0, pitch=0, yaw=0))
    varek.set_actor_label("Varek")
    free = actors.spawn_actor_from_class(unreal.BLInteractable, unreal.Vector(17200, 2600, vz + 60))
    free.set_actor_label("Liberar_Varek")
    free.set_editor_property("prompt", "Liberar a Varek")
    free.set_editor_property("hold_time", 2.0)
    free.set_editor_property("pickup", False)
    free.tags = [unreal.Name("BLObjective_Varek")]
    enemy("Bloque_Portal", "Bloque", 18400, 1700, 90)
    enemy("Bloque_P1_Pasillo", "Bloque", 17800, 2010, 0, z=FH)
    enemy("Bloque_P1_PisoB", "Bloque", 18300, 1650, 180, z=FH)
    enemy("Bloque_P2_Pasillo", "Bloque", 18700, 2010, 180, z=2 * FH)
    enemy("Bloque_P2_PisoC", "Bloque", 17450, 2450, 180, z=2 * FH)
    enemy("Bloque_Azotea", "Bloque", 18300, 2500, 0, z=3 * FH)


# ---------------------------------------------------------------------------
# Fases 8-9 (Bloque 11): huida por los tejados y extracción en el muelle
# ---------------------------------------------------------------------------
def facade_building(name, x0, x1, y0, y1, h, wall, face_mask, cornice_mask, street_mask=0, seed=1):
    a = actors.spawn_actor_from_class(unreal.BLBuilding, unreal.Vector((x0 + x1) / 2, (y0 + y1) / 2, 0))
    a.set_actor_label(name)
    a.set_editor_property("size", unreal.Vector(x1 - x0, y1 - y0, h))
    a.set_editor_property("seed", seed)
    a.set_editor_property("face_mask", face_mask)
    a.set_editor_property("cornice_mask", cornice_mask)
    a.set_editor_property("street_mask", street_mask)
    a.set_editor_property("wall_material", mat(f"MI_Fac_Wall_{wall}"))
    a.set_editor_property("phys_material", unreal.load_asset("/Game/Environment/PhysicalMaterials/PM_Concrete"))
    return a


def rooftops():
    """C4 (pegado al local, a la altura del bloque C3): pasarela de andamio desde la azotea de C3 y escalera de
    incendios de tres tramos por la fachada este, hasta el muelle."""
    zr = 3 * FH
    X0, X1, Y0, Y1 = 19760, 21000, 1330, 2900
    # Fachadas solo al muelle (este) y al callejón (sur); al oeste y al norte, medianeras ciegas
    facade_building("C4", X0, X1, Y0, Y1, zr, "Grey", 1 | 4, 4, seed=4242)
    box("C4_Medianera_O", X0, X0 + 25, Y0, Y1, 0, zr, M_PLASTER)
    box("C4_Junta_N", X0, X1, Y0, Y0 + 25, 0, zr, M_PLASTER)
    # Peto de la azotea: al oeste con el paso de la pasarela, al este con la salida a la escalera de incendios
    box("C4_Peto_O1", X0, X0 + 25, Y0, 2350, zr, zr + 105, M_PLASTER)
    box("C4_Peto_O2", X0, X0 + 25, 2550, Y1, zr, zr + 105, M_PLASTER)
    box("C4_Peto_E1", X1 - 25, X1, Y0, 2290, zr, zr + 105, M_PLASTER)
    box("C4_Peto_E2", X1 - 25, X1, 2455, Y1 - 25, zr, zr + 105, M_PLASTER)
    # Cubierta: depósito de agua, aparatos de aire, caseta y antena (coberturas y silueta)
    box("C4_Deposito", 20250, 20550, 2500, 2800, zr, zr + 180, M_RUST)
    box("C4_Deposito_Pies", 20240, 20560, 2490, 2810, zr, zr + 20, M_METAL)
    for i, (ax, ay) in enumerate(((20000, 1600), (20150, 1600), (20700, 2700))):
        box(f"C4_AC{i}", ax - 50, ax + 50, ay - 35, ay + 35, zr, zr + 90, M_METAL)
        covers_for_box(ax - 50, ax + 50, ay - 35, ay + 35, True)
    box("C4_Caseta", 20500, 20850, 1500, 1800, zr, zr + 260, M_PLASTER)
    box("C4_Caseta_Techo", 20480, 20870, 1480, 1820, zr + 260, zr + 275, M_WALL)
    box("C4_Antena", 20600, 20610, 1600, 1610, zr + 275, zr + 700, M_METAL)
    # Pasarela de andamio entre C3 y C4 (tablones, barandillas y pies hasta el suelo)
    box("Pasarela_Tablones", BX1 - 25, X0 + 40, 2360, 2540, zr - 15, zr, M_WOOD)
    for side, (ya, yb) in (("N", (2350, 2362)), ("S", (2538, 2550))):
        box(f"Pasarela_Baranda{side}", BX1, X0 + 25, ya, yb, zr + 95, zr + 105, M_METAL)
        box(f"Pasarela_Baranda{side}2", BX1, X0 + 25, ya, yb, zr + 45, zr + 52, M_METAL)
        for px in (BX1 + 10, X0 - 10):
            box(f"Pasarela_Pie{side}{px}", px - 5, px + 5, ya, yb, 0, zr + 105, M_METAL)
    for px in (BX1 + 10, X0 - 10):
        box(f"Pasarela_Travesano{px}", px - 5, px + 5, 2350, 2550, zr - 30, zr - 15, M_METAL)
    # Escalera de incendios: tres tramos de 18 escalones (320 cm por planta) en dos calles (A junto a la fachada, B fuera)
    # (separada 105 cm de la fachada: los balcones del kit sobresalen 95)
    # Calles de 160: con 115 el radio del agente (34 por lado) no dejaba NavMesh en los tramos
    XA0, XA1, XB0, XB1 = X1 + 105, X1 + 265, X1 + 275, X1 + 435
    rise = FH / 18
    stair_flight("Incendios_T0", XA0, XA1, 1800, 1, 0, 18, rise, m=M_METAL)
    stair_flight("Incendios_T1", XB0, XB1, 2304, -1, FH, 18, rise, m=M_METAL)
    stair_flight("Incendios_T2", XA0, XA1, 1800, 1, 2 * FH, 18, rise, m=M_METAL)
    box("Incendios_Rellano1", XA0, XB1, 2304, 2440, FH - 20, FH, M_METAL)
    box("Incendios_Rellano2", XA0, XB1, 1664, 1800, 2 * FH - 20, 2 * FH, M_METAL)
    box("Incendios_Rellano3", X1 - 45, XB1, 2290, 2455, zr - 20, zr + 1, M_METAL)   # pisa la azotea: sin hueco sobre la fachada
    box("Incendios_Separador", XA1, XB0, 1800, 2304, 0, zr, M_METAL)
    for k in range(4):
        z = k * FH
        box(f"Incendios_Baranda{k}", XB1, XB1 + 8, 1664, 2455, z + 90, z + 100, M_METAL)
        box(f"Incendios_BarandaFin{k}", XA0, XB1 + 8, 2447, 2455, z + 90, z + 100, M_METAL)
    for i, py in enumerate((1664, 2050, 2447)):
        box(f"Incendios_Pilar{i}", XB1, XB1 + 8, py, py + 8, 0, zr + 100, M_METAL)
    start_point("Azotea", 18500, 2025, 180, z=2 * FH)
    checkpoint("Azotea", 18950, 19475, 1950, 2875, 0, z=zr)
    start_point("Muelle", 21185, 1650, 0)
    checkpoint("Muelle", 21000, 21800, 1000, 2400, 0)


def quay():
    """Muelle (x 210-266 m): explanada de contenedores, nave al norte, grúa pórtico, agua al este y la zona de
    aterrizaje del helicóptero. Las oleadas llegan por una bolsa tras la valla norte y por el callejón G (sur de C4)."""
    lc = unreal.LinearColor
    box("Muelle_Suelo", 21000, 26600, -2000, 4240, -2, 0, M_ASPHALT)
    box("Callejon_G_Suelo", 19750, 21000, 2900, 4240, -2, 0, M_FLOOR)
    box("Callejon_G_MuroO", 19690, 19750, 2900, 4240, 0, 600, M_WALL)
    # Nave del puerto al norte (cierra el muelle; su fachada sur da a la explanada)
    facade_building("Nave_Muelle", 21000, 24500, -2000, -100, 900, "Concrete", 1 | 4, 1 | 4, street_mask=4, seed=5151)
    box("Nave_Cierre_O", 20970, 21000, -2000, -100, 0, 900, M_WALL)
    # Valla norte con la bolsa por la que saltan los milicianos
    box("Valla_N1", 24500, 25000, -2060, -2000, 0, 450, M_WALL)
    box("Valla_N2", 25500, 26600, -2060, -2000, 0, 450, M_WALL)
    box("Bolsa_Suelo", 24900, 25600, -2760, -2000, -2, 0, M_DIRT)
    box("Bolsa_MuroO", 24940, 25000, -2760, -2060, 0, 450, M_WALL)
    box("Bolsa_MuroE", 25500, 25560, -2760, -2060, 0, 450, M_WALL)
    box("Bolsa_MuroN", 24940, 25560, -2760, -2700, 0, 450, M_WALL)
    # Borde del muelle, bolardos y agua (M_Env_Water: refleja el amanecer)
    box("Muelle_Bordillo", 26520, 26600, -2000, 4240, 0, 22, M_WALL)
    box("Muelle_Muro", 26600, 26700, -12000, 12000, -400, 0, M_WALL)
    for i, y in enumerate(range(-1600, 4200, 900)):
        box(f"Bolardo{i}", 26420, 26480, y - 30, y + 30, 0, 55, M_RUST)
    water = box("Agua", 26700, 46000, -14000, 14000, -320, -260, mat("M_Env_Water"))
    water.static_mesh_component.set_collision_profile_name("NoCollision")
    # Contenedores (coberturas alrededor de la zona de aterrizaje; algunos apilados)
    for i, (x, y, yaw, z, c) in enumerate(((22200, 450, 90, 0, 0), (22800, 2200, 0, 0, 1), (22800, 2200, 3, 259, 2),
                                            (23700, 3400, 90, 0, 3), (24100, 600, 0, 0, 2), (24300, 2650, 15, 0, 0),
                                            (21900, 3500, 0, 0, 1), (25900, -1100, 90, 0, 3), (23300, -600, 0, 0, 0),
                                            (23300, -600, -4, 259, 1), (26000, 3350, 0, 0, 2))):
        prop("SM_Container_20ft", x, y, yaw, z=z, label=f"Contenedor_Muelle_{i}", material=CONTAINER_MATS[c])
    prop("SM_Cover_Jersey", 25900, 2100, 80, label="Jersey_Muelle_1")
    prop("SM_Cover_Sandbag_Corner", 24400, 1500, 90, label="Sacos_Muelle_1")
    prop("SM_Cover_Sandbag_Straight", 24400, 1150, 90, label="Sacos_Muelle_2")
    # Grúa pórtico sobre el borde del muelle (silueta contra el amanecer)
    for i, (gx, gy) in enumerate(((25700, 3000), (26400, 3000), (25700, 3900), (26400, 3900))):
        box(f"Grua_Pata{i}", gx - 45, gx + 45, gy - 45, gy + 45, 0, 2300, M_RUST)
    for i, gy in enumerate((3000, 3900)):
        box(f"Grua_Viga{i}", 25650, 26450, gy - 50, gy + 50, 2300, 2420, M_RUST)
    box("Grua_Pluma", 24800, 33000, 3380, 3520, 2420, 2560, M_RUST)
    box("Grua_Cabina", 25900, 26200, 3300, 3600, 2120, 2300, M_METAL)
    box("Grua_Cable", 31500, 31508, 3446, 3454, 900, 2420, M_METAL)
    prop("SM_Container_20ft", 31500, 3450, 8, z=640, label="Contenedor_Colgado", material=CONTAINER_MATS[3])
    # Zona de aterrizaje: humo verde y balizas
    smoke("Humo_LZ", 24850, 1450, 0, color=lc(0.1, 0.32, 0.12, 1), opacity=0.55, max_particles=26, life=11.0,
          start_size=unreal.Vector2D(60, 110), end_size=unreal.Vector2D(450, 800), rise_speed=90.0, spawn_radius=25.0,
          wind=unreal.Vector(-60, 20, 0))
    for i, (bx, by) in enumerate(((24300, 650), (25700, 650), (24300, 1950), (25700, 1950))):
        interior_light(f"Baliza_LZ{i}", bx, by, 15, True, 18.0, 400.0)
    for i, (x, y, yaw) in enumerate(((22200, -50, 90), (24500, 3950, -90), (26350, 600, 180))):
        sodium_light(f"Farola_Muelle_{i}", x, y, yaw, shadows=(i == 0))
    ambient("SW_AmbZ_Water_Loop", 26500, -1000, 0, 1.0, "Amb_Muelle_0")
    ambient("SW_AmbZ_Water_Loop", 26500, 2600, 0, 1.0, "Amb_Muelle_1")
    # Blindado de la Columna (fase 8): espera escondido al oeste de la calle principal
    btr = actors.spawn_actor_from_class(unreal.BLBTR, unreal.Vector(9300, 700, 0), unreal.Rotator(roll=0, pitch=0, yaw=0))
    btr.set_actor_label("Blindado")
    btr.set_editor_property("path", [unreal.Vector(9300, 700, 0), unreal.Vector(11000, 700, 0), unreal.Vector(13250, 700, 0)])
    btr.set_editor_property("collapse_radio", [radio("M01_Derrumbe")])
    btr.tags = [unreal.Name("BLBTR")]
    # Helicóptero de extracción (fase 9): llega desde el mar, estacionario sobre la zona, aterriza para subir
    heli = actors.spawn_actor_from_class(unreal.BLHelicopter, unreal.Vector(40000, 1300, 3500), unreal.Rotator(roll=0, pitch=0, yaw=180))
    heli.set_actor_label("Helicoptero")
    heli.set_editor_property("path", [unreal.Vector(40000, 1300, 3500), unreal.Vector(31000, 1250, 2200)])
    heli.set_editor_property("hover_point", unreal.Vector(25000, 1300, 1150))
    heli.set_editor_property("land_point", unreal.Vector(25000, 1300, 0))
    heli.set_editor_property("hover_yaw", 180.0)
    heli.set_editor_property("approach_delay", 8.0)
    heli.tags = [unreal.Name("BLHeli"), unreal.Name("BLHeli_Land")]
    board = actors.spawn_actor_from_class(unreal.BLInteractable, unreal.Vector(25000, 1300, 110))
    board.set_actor_label("Subir_Helicoptero")
    board.set_editor_property("prompt", "Subir al helicóptero")
    board.set_editor_property("hold_time", 1.0)
    board.set_editor_property("pickup", False)
    board.tags = [unreal.Name("BLObjective_Heli")]


def enemy(label, squad, x, y, yaw, patrol=(), z=0.0):
    e = actors.spawn_actor_from_class(unreal.BLEnemyCharacter, unreal.Vector(x, y, z + 96), unreal.Rotator(roll=0, pitch=0, yaw=yaw))
    e.set_actor_label(label)
    e.set_editor_property("squad_id", squad)
    e.set_editor_property("patrol_points", [unreal.Vector(px, py, 0) for px, py in patrol])
    e.tags = [unreal.Name("BLEnemy"), unreal.Name(f"BLEnemy_{label}")]
    return e


def enemies():
    """12 milicianos (presupuesto: <= 12 vivos, <= 8 en combate). Escuadras por zona."""
    # Patio (infiltración): dos patrullas por los pasillos de contenedores y un centinela en la puerta
    enemy("Patio_Patrulla1", "Patio", 2000, -3100, 0, [(2000, -3100), (6000, -3150), (6000, -4800), (2600, -4800)])
    enemy("Patio_Patrulla2", "Patio", 7400, -5800, 90, [(7400, -5800), (7400, -3900), (4500, -3900), (4500, -5900)])
    enemy("Patio_Puerta", "Patio", 8600, -4100, 180)
    # Control de carretera: dos en los sacos, uno en la caseta y una patrulla
    enemy("Control_SacosO", "Control", 9330, -2330, -90)
    enemy("Control_SacosE", "Control", 10120, -2240, -90)
    enemy("Control_Caseta", "Control", 10040, -1800, -90)
    enemy("Control_Patrulla", "Control", 9700, -3300, 90, [(9700, -3300), (9750, -1350)])
    # Calle principal: posición de sacos, patrulla, centinela en el callejón D y guardia del local
    enemy("Calle_Sacos1", "Calle", 18550, 450, 180)
    enemy("Calle_Sacos2", "Calle", 18560, 800, 180)
    enemy("Calle_Patrulla", "Calle", 16500, 600, 180, [(16500, 600), (13900, 750)])
    enemy("Calle_CallejonD", "Calle", 15650, -1200, 90)
    enemy("Calle_Local", "Calle", 20300, 420, 180)


def marker(tag, x, y, z=0.0):
    tp = actors.spawn_actor_from_class(unreal.TargetPoint, unreal.Vector(x, y, z))
    tp.set_actor_label(tag)
    tp.tags = [unreal.Name(tag)]
    return tp


def load_voice_lines():
    """Frases de radio de Tools/audio/voice_lines.tsv (la misma tabla con la que se generan las voces)."""
    path = os.path.join(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir()), "Tools", "audio", "voice_lines.tsv")
    out = {}
    with open(path, encoding="utf-8") as f:
        for line in f:
            c = line.rstrip("\r\n").split("\t")
            if len(c) >= 7 and not line.startswith("#") and c[1] in ("radio", "persona"):
                out[c[0]] = (c[2], c[6], c[1] == "persona")
    return out


VOICE_LINES = load_voice_lines()


def radio(line_id):
    speaker, text, in_person = VOICE_LINES[line_id]
    line = unreal.BLRadioLine()
    line.set_editor_property("speaker", speaker)
    line.set_editor_property("text", text)
    line.set_editor_property("person", in_person)   # bInPerson (Python le quita "b" e "In")
    voice_path = f"/Game/Audio/Voice/Radio/VO_{line_id}"
    if unreal.EditorAssetLibrary.does_asset_exist(voice_path):
        line.set_editor_property("voice", unreal.load_asset(voice_path))
    else:
        log(f"AVISO: sin voz para {line_id}")
    return line


def wave(tags, count, delay, smoke=False, radio_lines=()):
    w = unreal.BLWave()
    w.set_editor_property("spawn_tags", [unreal.Name(t) for t in tags])
    w.set_editor_property("count", count)
    w.set_editor_property("delay", delay)
    w.set_editor_property("smoke", smoke)
    w.set_editor_property("radio", list(radio_lines))
    return w


def objective_def(text, kind, tag="", squad="", radius=400.0, marker_on=True, on_start=(), on_complete=(), waves=(), min_duration=60.0,
                  check_height=False, activate=()):
    o = unreal.BLObjective()
    o.set_editor_property("text", text)
    o.set_editor_property("type", kind)
    o.set_editor_property("target_tag", tag)
    o.set_editor_property("squad_id", squad)
    o.set_editor_property("radius", radius)
    o.set_editor_property("show_marker", marker_on)
    o.set_editor_property("radio_on_start", list(on_start))
    o.set_editor_property("radio_on_complete", list(on_complete))
    if waves:
        o.set_editor_property("waves", list(waves))
        o.set_editor_property("min_duration", min_duration)
    o.set_editor_property("check_height", check_height)
    o.set_editor_property("activate_tags", [unreal.Name(t) for t in activate])
    return o


def mission():
    """Objetivos de la misión 1 (vertical slice: fases 1-4 + recuperar el disco) y guion de radio."""
    marker("BLObj_Callejon", 3050, -900)
    marker("BLObj_Puerta", 8900, -4100)
    marker("BLObj_Control", 9700, -2250)
    marker("BLObj_Local", 19650, 600)
    # Objetivo: portátil sobre la mesa del local y el disco duro (interactuable) a su lado
    prop("SM_Obj_Laptop_Rugged", 20660, 640, 180, z=75, label="Portatil_Varek")
    hd = actors.spawn_actor_from_class(unreal.BLInteractable, unreal.Vector(20630, 535, 75.5), unreal.Rotator(roll=0, pitch=0, yaw=25))
    hd.set_actor_label("Disco_Varek")
    hd.get_component_by_class(unreal.StaticMeshComponent).set_static_mesh(unreal.load_asset(PROPS + "SM_Obj_HardDrive"))
    hd.set_editor_property("prompt", "Recoger el disco duro de Varek")
    hd.set_editor_property("hold_time", 1.2)
    hd.tags = [unreal.Name("BLObjective_Disco")]

    REACH, CLEAR, USE = unreal.BLObjectiveType.REACH, unreal.BLObjectiveType.CLEAR_SQUAD, unreal.BLObjectiveType.INTERACT
    DEFEND = unreal.BLObjectiveType.DEFEND
    # Fase 7 (contraataque): puntos de aparición de las oleadas (fuera de la vista desde el bloque)
    for i, (x, y) in enumerate(((14300, 450), (14600, 950), (14900, 300))):
        marker("BLWave_Calle", x, y)
    for x, y in ((14700, 3500), (15500, 3300)):
        marker("BLWave_Pasaje", x, y)
    marker("BLWave_Callejon", 16750, 3500)
    # Fases 8-9: azotea, pie de la escalera de incendios, zona de aterrizaje y oleadas del muelle
    marker("BLObj_Azotea", 18250, 2100, 3 * FH)   # centro de la azotea: con radio 1500 vale toda
    marker("BLObj_Escalera", 21185, 1720, 0)
    marker("BLObj_LZ", 25000, 1300, 0)
    for x, y in ((25150, -2450), (25350, -2300)):
        marker("BLWave_MuelleN", x, y)
    for x, y in ((20150, 3500), (20500, 3900)):
        marker("BLWave_MuelleO", x, y)
    d = actors.spawn_actor_from_class(unreal.BLMissionDirector, unreal.Vector(0, 0, 300))
    d.set_actor_label("Director_Mision")
    d.set_editor_property("mission_name", "AMANECER ROTO")
    d.set_editor_property("briefing", [
        radio("M01_Brief_01"),
        radio("M01_Brief_02"),
        radio("M01_Brief_03"),
        radio("M01_Brief_04"),
    ])
    d.set_editor_property("objectives", [
        objective_def("Entra en el puerto por los callejones del norte", REACH, "BLObj_Callejon", radius=350.0),
        objective_def("Cruza el patio de contenedores hasta la carretera", REACH, "BLObj_Puerta", radius=450.0,
                  on_start=[radio("M01_Patio")]),
        objective_def("Neutraliza el control de carretera", CLEAR, "BLObj_Control", squad="Control",
                  on_start=[radio("M01_Control")],
                  on_complete=[radio("M01_ControlOk"),
                               radio("M01_Avanzamos")]),
        objective_def("Avanza por la calle principal hasta el local de Varek", REACH, "BLObj_Local", radius=300.0,
                  on_complete=[radio("M01_Local")]),
        objective_def("Recupera el disco duro de Varek", USE, "BLObjective_Disco",
                  on_complete=[radio("M01_Disco"), radio("M01_VarekBloque")]),
        objective_def("Entra en el bloque de viviendas de enfrente", REACH, "BLObj_Bloque", radius=260.0),
        objective_def("Despeja el bloque y encuentra a Varek (última planta)", CLEAR, "BLObj_VarekPiso", squad="Bloque",
                  on_start=[radio("M01_Bloque")]),
        objective_def("Libera a Varek", USE, "BLObjective_Varek",
                  on_complete=[radio("M01_VarekHabla"), radio("M01_VarekOk")]),
        objective_def("Defiende el bloque con Varek hasta que llegue el apoyo", DEFEND, "BLObj_VarekPiso", squad="Contra",
                  on_start=[radio("M01_Contra")], on_complete=[radio("M01_ContraOk")], min_duration=75.0,
                  waves=[wave(["BLWave_Calle"], 4, 8.0, True, [radio("M01_ContraHumo")]),
                         wave(["BLWave_Pasaje"], 4, 10.0, False, [radio("M01_ContraPasaje")]),
                         wave(["BLWave_Calle", "BLWave_Callejon"], 5, 10.0, True, [radio("M01_ContraUltima")])]),
        # Fase 8: el blindado entra por la calle y derriba la fachada; huida por los tejados
        objective_def("Sube a la azotea del bloque", REACH, "BLObj_Azotea", radius=1500.0, check_height=True, activate=["BLBTR"],
                      on_start=[radio("M01_Blindado")], on_complete=[radio("M01_Azotea")]),
        objective_def("Cruza por los tejados y baja al muelle", REACH, "BLObj_Escalera", radius=260.0, check_height=True,
                      on_start=[radio("M01_Tejados")]),
        # Fase 9: extracción en helicóptero
        objective_def("Llega a la zona de aterrizaje", REACH, "BLObj_LZ", radius=700.0, on_start=[radio("M01_Muelle")]),
        objective_def("Defiende la zona de aterrizaje hasta que llegue el helicóptero", DEFEND, "BLObj_LZ", squad="Muelle",
                      activate=["BLHeli"], on_start=[radio("M01_LZ")], on_complete=[radio("M01_HeliLlega")], min_duration=45.0,
                      waves=[wave(["BLWave_MuelleN"], 3, 6.0, False, [radio("M01_LZNorte")]),
                             wave(["BLWave_MuelleO"], 4, 8.0, True, [radio("M01_LZOeste")]),
                             wave(["BLWave_MuelleN", "BLWave_MuelleO"], 4, 12.0)]),   # con el helicóptero ya encima
        objective_def("Sube al helicóptero", USE, "BLObjective_Heli", activate=["BLHeli_Land"], on_start=[radio("M01_Halcon")]),
    ])
    d.set_editor_property("debriefing", [
        radio("M01_Debrief"),
    ])


def ambient(sound, x, y, z, volume=1.0, label=None):
    """Emisor 3D en bucle (el sonido trae su atenuación: SA_AmbSmall / SA_AmbLarge)."""
    a = actors.spawn_actor_from_class(unreal.AmbientSound, unreal.Vector(x, y, z))
    a.set_actor_label(label or sound)
    c = a.get_component_by_class(unreal.AudioComponent)
    c.set_sound(unreal.load_asset("/Game/Audio/Ambience/Zones/" + sound))
    c.set_editor_property("volume_multiplier", volume)
    c.set_editor_property("auto_activate", True)
    return a


def reverb_zone(label, x0, x1, y0, y1, z1, effect, priority=1.0, ambience=1.0, volume=1.0):
    z = actors.spawn_actor_from_class(unreal.BLReverbZone, unreal.Vector((x0 + x1) / 2, (y0 + y1) / 2, z1 / 2))
    z.set_actor_label(label)
    z.tags = [unreal.Name(label)]   # la prueba Audio la identifica por la etiqueta (también en la build empaquetada)
    z.get_component_by_class(unreal.BoxComponent).set_box_extent(unreal.Vector((x1 - x0) / 2, (y1 - y0) / 2, z1 / 2))
    z.set_editor_property("reverb", unreal.load_asset("/Game/Audio/Settings/" + effect))
    z.set_editor_property("priority", priority)
    z.set_editor_property("ambience_scale", ambience)
    z.set_editor_property("reverb_volume", volume)
    return z


def audio():
    """Bloque 7: ambiente por zona y acústica. El fondo de ciudad en guerra lo pone ABLGameMode y el combate
    lejano UBLAudioSubsystem; aquí solo lo que tiene un sitio concreto."""
    # Agua del muelle al otro lado del muro norte del patio (se oye por encima del muro)
    for i, x in enumerate((2600, 5200, 7800)):
        ambient("SW_AmbZ_Water_Loop", x, -6900, 50, 1.0, f"Amb_Agua_{i}")
    # Zumbido de las farolas de sodio (en la luminaria)
    for i, (x, y) in enumerate(((9440, -2800), (9960, -1500), (11400, 1000), (13300, 240), (16000, 1000), (18000, 240))):
        ambient("SW_AmbZ_Hum_Loop", x, y, 660, 1.0, f"Amb_Farola_{i}")
    # Viento encañonado en los callejones
    for i, (x, y) in enumerate(((3050, -1000), (5100, -1000), (12650, -1400), (15650, -1400), (13750, 2600), (16750, 2600))):
        ambient("SW_AmbZ_WindAlley_Loop", x, y, 250, 0.8, f"Amb_Viento_{i}")
    ambient("SW_AmbZ_Room_Loop", 20500, 600, 200, 1.0, "Amb_Local")
    # Acústica: callejones estrechos (eco corto de las paredes) e interior del local (sala pequeña, la ciudad se apaga)
    for name, b in (("A", (2800, 3300, -1600, -400)), ("B", (4800, 5400, -1600, -400)), ("C", (12400, 12900, -2700, -100)),
                    ("D", (15400, 15900, -2700, -100)), ("E", (13500, 14000, 1300, 3900)), ("F", (16500, 17000, 1300, 3900))):
        reverb_zone(f"Acustica_Callejon_{name}", b[0], b[1], b[2], b[3], 900, "RE_Alley")
    reverb_zone("Acustica_CalleTrasera", 0, 6000, -400, 400, 900, "RE_Alley", 1.0, 1.0, 0.6)
    reverb_zone("Acustica_Local", 19530, 20970, -70, 1270, 340, "RE_Interior", 2.0, 0.55)


def pole_line(name, points, yaw=0.0):
    """Postes de la luz con tres cables de un poste al siguiente (catenaria: SM_Prop_Cable, 1000 cm, escalado)."""
    cable = unreal.load_asset(PROPS + "SM_Prop_Cable")
    for i, (x, y) in enumerate(points):
        prop("SM_Prop_PowerPole", x, y, yaw, label=f"{name}_Poste{i}")
    c, s_ = math.cos(math.radians(yaw)), math.sin(math.radians(yaw))
    for i in range(len(points) - 1):
        (x0, y0), (x1, y1) = points[i], points[i + 1]
        d = math.hypot(x1 - x0, y1 - y0)
        heading = math.degrees(math.atan2(y1 - y0, x1 - x0))
        for k, off in enumerate((-95, 0, 95)):
            ox, oy = -off * s_, off * c          # desplazamiento a lo largo de la cruceta (eje Y local del poste)
            a = actors.spawn_actor_from_class(unreal.StaticMeshActor, unreal.Vector(x0 + ox, y0 + oy, 851), unreal.Rotator(roll=0, pitch=0, yaw=heading))
            a.static_mesh_component.set_static_mesh(cable)
            a.static_mesh_component.set_collision_profile_name("NoCollision")
            a.static_mesh_component.set_editor_property("cast_shadow", k == 1)
            a.set_actor_scale3d(unreal.Vector(d / 1000.0, 1, 1 + rng.uniform(-0.15, 0.25)))
            a.set_actor_label(f"{name}_Cable{i}_{k}")


def dressing():
    """Bloque 8: atrezo de calle (lo que hace que parezca una ciudad habitada y luego abandonada).
    Pegado a fachadas y muros para no cerrar rutas ni coberturas del diseño."""
    P = prop
    # Calle trasera (inserción)
    pole_line("Linea_Trasera", [(600, -270), (2050, -270), (3600, -270), (5650, -270)])
    P("SM_Prop_TrashBags", 1580, 300, 30)
    P("SM_Prop_TrashBags", 3930, -300, 160)
    P("SM_Prop_Rubble_B", 4950, 290, 40)
    P("SM_Prop_Drum", 5700, 320, 0)
    P("SM_Prop_Drum", 5765, 255, 40)
    P("SM_Prop_PalletStack", 1200, -310, 3)
    P("SM_Prop_Rubble_C", 300, 300, 120)
    # Patio del puerto
    for x, y, yaw in ((2450, -5050, 4), (3750, -5080, -8)):
        P("SM_Prop_PalletStack", x, y, yaw)
    P("SM_Prop_Crate", 4050, -5050, 12)
    P("SM_Prop_Crate", 4090, -5040, 3, z=72.5)
    for x, y in ((2100, -4700), (2165, -4630), (2090, -4610), (8650, -2000), (8700, -2070)):
        P("SM_Prop_Drum", x, y, rng.uniform(0, 360))
    P("SM_Prop_TireStack", 7600, -1950, 70)
    P("SM_Prop_TireStack", 1750, -2050, 10)
    P("SM_Prop_Pallet", 5600, -6250, 15)
    P("SM_Prop_Rubble_A", 8700, -6200, 0)
    # Control de carretera
    P("SM_Prop_TireStack", 10250, -900, 30)
    P("SM_Prop_Drum", 9150, -1150, 0)
    P("SM_Prop_Drum", 9180, -1080, 0)
    P("SM_Prop_Rubble_B", 10250, -5800, 200)
    P("SM_Prop_TrashBags", 9120, -6600, 70)
    # Calle principal: postes en la acera sur, basura y escombro al pie de las fachadas
    pole_line("Linea_Calle", [(10900, 1180), (12700, 1180), (15100, 1180), (17600, 1180), (19300, 1180)])
    for x, y, yaw in ((11250, 110, 2), (16900, 100, -3)):
        P("SM_Prop_Dumpster", x, y, yaw)
    for x, y, yaw in ((11550, 120, 80), (14100, 1210, 10), (17150, 110, 200), (18900, 1200, 45)):
        P("SM_Prop_TrashBags", x, y, yaw)
    for x, y, mesh, yaw in ((12450, 1150, "SM_Prop_Rubble_B", 30), (15750, 40, "SM_Prop_Rubble_A", 110),
                            (13100, 150, "SM_Prop_Rubble_C", 300), (17700, 1170, "SM_Prop_Rubble_C", 75)):
        P(mesh, x, y, yaw)
    for y in (150, 1050):
        P("SM_Prop_Bollard", 19430, y)
    # Callejones de flanqueo y patios traseros
    P("SM_Prop_Dumpster", 12650, -2550, 90)
    P("SM_Prop_PalletStack", 15100, -2550, 10)
    P("SM_Prop_TrashBags", 15650, -2450, 0)
    P("SM_Prop_Rubble_B", 14500, 3700, 15)
    P("SM_Prop_TireStack", 16750, 3650, 0)
    P("SM_Prop_Crate", 13750, 3700, 30)


def smoke(label, x, y, z=0.0, **props):
    a = actors.spawn_actor_from_class(unreal.BLSmokeEmitter, unreal.Vector(x, y, z))
    a.set_actor_label(label)
    for k, v in props.items():
        a.set_editor_property(k, v)
    return a


def effects():
    """Bloque 8: columnas de humo lejanas (la ciudad arde más allá de la misión) y fuegos cercanos."""
    lc = unreal.LinearColor
    # Columnas a 150-300 m, por encima de los tejados en varias direcciones (una de cara al sol)
    for i, (x, y, shade) in enumerate(((31000, -3000, 0.14), (16000, -19000, 0.2), (2000, 15000, 0.17), (26000, 12000, 0.12))):
        smoke(f"Humo_Lejano_{i}", x, y, 0, color=lc(shade, shade * 0.95, shade * 0.9, 1), opacity=0.5 + 0.1 * (i % 2),
              wind=unreal.Vector(60 + 20 * i, 20, 0))
    # Coche que aún arde en la calle principal: llamas, luz que parpadea, sonido y humo negro
    smoke("Fuego_Coche_Calle_2", 14700, 900, 0, fire=True, fire_extent=unreal.Vector(110, 50, 45), flame_size=70.0,
          light_intensity=9000.0, max_particles=26, life=9.0, start_size=unreal.Vector2D(70, 110), end_size=unreal.Vector2D(500, 800),
          rise_speed=150.0, spawn_radius=70.0, color=lc(0.07, 0.065, 0.06, 1), opacity=0.65, wind=unreal.Vector(40, 10, 0))
    # Bidón con fuego en el control (calor de los guardias; acento cálido en el amanecer frío)
    smoke("Fuego_Bidon_Control", 9150, -1150, 0, fire=True, fire_extent=unreal.Vector(16, 16, 88), flame_size=48.0,
          light_intensity=3500.0, max_flames=10, max_particles=14, life=6.0, start_size=unreal.Vector2D(30, 50),
          end_size=unreal.Vector2D(200, 320), rise_speed=110.0, spawn_radius=12.0, color=lc(0.12, 0.11, 0.1, 1), opacity=0.4,
          wind=unreal.Vector(30, 10, 0))


def lighting():
    # Amanecer frío: el sol sale por el este (de cara al avanzar), rasante y filtrado por nubes
    sun = actors.spawn_actor_from_class(unreal.DirectionalLight, unreal.Vector(0, 0, 2000), unreal.Rotator(roll=0, pitch=-11, yaw=170))
    sc = sun.get_component_by_class(unreal.DirectionalLightComponent)
    sc.set_mobility(unreal.ComponentMobility.MOVABLE)
    sc.set_intensity(2.6)
    sc.set_light_color(unreal.LinearColor(1.0, 0.82, 0.66, 1))
    sc.set_editor_property("atmosphere_sun_light", True)
    sky = actors.spawn_actor_from_class(unreal.SkyLight, unreal.Vector(0, 0, 1500))
    skc = sky.get_component_by_class(unreal.SkyLightComponent)
    skc.set_mobility(unreal.ComponentMobility.MOVABLE)
    skc.set_editor_property("real_time_capture", True)
    skc.set_intensity(1.9)
    skc.set_editor_property("lower_hemisphere_is_black", False)
    skc.set_editor_property("lower_hemisphere_color", unreal.LinearColor(0.09, 0.085, 0.08, 1))
    skc.set_light_color(unreal.LinearColor(0.8, 0.88, 1.0, 1))
    actors.spawn_actor_from_class(unreal.SkyAtmosphere, unreal.Vector(0, 0, 0))
    fog = actors.spawn_actor_from_class(unreal.ExponentialHeightFog, unreal.Vector(0, 0, 0))
    fc = fog.get_component_by_class(unreal.ExponentialHeightFogComponent)
    fc.set_editor_property("fog_density", 0.04)
    fc.set_editor_property("fog_height_falloff", 0.1)
    fc.set_editor_property("start_distance", 1500.0)
    fc.set_editor_property("fog_inscattering_luminance", unreal.LinearColor(0.42, 0.47, 0.53, 1))
    ppv = actors.spawn_actor_from_class(unreal.PostProcessVolume, unreal.Vector(0, 0, 0))
    ppv.set_editor_property("unbound", True)
    s = ppv.get_editor_property("settings")
    v4 = unreal.Vector4
    for name, value in (("auto_exposure_bias", 0.25),
                        ("local_exposure_shadow_contrast_scale", 0.6),
                        ("local_exposure_highlight_contrast_scale", 0.7),
                        ("local_exposure_detail_strength", 1.15),
                        ("color_saturation", v4(0.78, 0.78, 0.8, 1.0)),
                        ("color_contrast", v4(1.12, 1.12, 1.12, 1.0)),
                        ("color_gain_shadows", v4(0.92, 0.97, 1.05, 1.0)),
                        ("color_gain_highlights", v4(1.04, 1.0, 0.95, 1.0)),
                        ("vignette_intensity", 0.45),
                        ("film_grain_intensity", 0.06),
                        ("bloom_intensity", 0.45)):
        s.set_editor_property("override_" + name, True)
        s.set_editor_property(name, value)
    ppv.set_editor_property("settings", s)


def navigation():
    # Volumen de navegación que cubre todo el recorrido (pinceles de 200 cm: escala = tamaño / 200)
    x0, x1, y0, y1 = -200, 26600, -8200, 4200
    # Hasta 13 m de alto: plantas y azotea del bloque de viviendas
    nav = actors.spawn_actor_from_class(unreal.NavMeshBoundsVolume, unreal.Vector((x0 + x1) / 2, (y0 + y1) / 2, 600))
    nav.set_actor_scale3d(unreal.Vector((x1 - x0) / 200, (y1 - y0) / 200, 1400 / 200))
    nav.set_actor_label("NavMeshBounds")
    state["nav_volume"] = nav


def build():
    # Si el mapa ya existe hay que cargarlo: new_level falla y dejaría abierto el mapa de inicio del editor,
    # que se vaciaría y se sobrescribiría al guardar (pasó con L_Dev_Movement)
    if unreal.EditorAssetLibrary.does_asset_exist(MAP_PATH):
        if not level_ed.load_level(MAP_PATH):
            raise RuntimeError(f"No se pudo cargar {MAP_PATH}")
    elif not level_ed.new_level(MAP_PATH):
        raise RuntimeError(f"No se pudo crear {MAP_PATH}")
    world = unreal.EditorLevelLibrary.get_editor_world()
    if not world.get_path_name().startswith(MAP_PATH):
        raise RuntimeError(f"El mapa abierto es {world.get_path_name()}, no {MAP_PATH}")
    old = actors.get_all_level_actors()
    if old:
        actors.destroy_actors(old)
    box("Suelo_Base", -2000, 23000, -9000, 5000, -40, -2, M_DIRT)
    phase1_insertion()
    phase2_infiltration()
    phase3_roadblock()
    phase4_street()
    objective()
    mission()
    enemies()
    log(f"Puntos de cobertura: {COVER_COUNT[0]}")
    # Límites del nivel (muros perimetrales: nada fuera del recorrido es accesible)
    for name, b in (("Limite_N", (-300, 26660, -8300, -8240)), ("Limite_S", (-300, 26660, 4240, 4300)),
                    ("Limite_O", (-300, -240, -8300, 4300))):
        box(name, b[0], b[1], b[2], b[3], 0, 800, M_WALL)
    # Borde del muelle: muro invisible (no se cae al agua) en lugar del muro del este
    edge = box("Limite_E", 26600, 26660, -8300, 4300, 0, 800, M_WALL)
    edge.set_actor_hidden_in_game(True)
    box("Relleno_Oeste", -240, 1460, -8240, -1600, 0, 600, M_WALL)
    finish_buildings()
    rooftops()
    quay()
    dressing()
    effects()
    lighting()
    audio()
    navigation()
    log(f"Actores: {len(actors.get_all_level_actors())}")


def check_navigation():
    world = unreal.EditorLevelLibrary.get_editor_world()
    pts = [("Fase1", unreal.Vector(300, -170, 100)), ("Fase2", unreal.Vector(3050, -2300, 100)),
           ("Fase3", unreal.Vector(9600, -4100, 100)), ("Fase4", unreal.Vector(9700, 600, 100)),
           ("Objetivo", unreal.Vector(20300, 600, 100)), ("Varek", unreal.Vector(17450, 2400, 740)),
           ("Azotea", unreal.Vector(18300, 2500, 1060)), ("Tejado_C4", unreal.Vector(20400, 2100, 1060)),
           ("Incendios_R3", unreal.Vector(21300, 2370, 1060)), ("Incendios_R2", unreal.Vector(21300, 1730, 740)),
           ("Incendios_R1", unreal.Vector(21300, 2370, 420)), ("Escalera", unreal.Vector(21185, 1720, 100)), ("LZ", unreal.Vector(25000, 1300, 100))]
    ok = True
    for (na, a), (nb, b) in zip(pts, pts[1:]):
        path = unreal.NavigationSystemV1.find_path_to_location_synchronously(world, a, b)
        n = len(path.path_points) if path else 0
        partial = path.is_partial() if path else True
        length = path.get_path_length() if path else 0.0
        log(f"Ruta {na} -> {nb}: {n} puntos, {length / 100:.0f} m, parcial={partial}")
        ok &= n > 1 and not partial
    return ok


def tick(dt):
    if state.get("busy"):
        return
    state["frame"] += 1
    if state["phase"] == "build" and state["frame"] == 3:
        try:
            build()
            # El volumen creado por script no avisa al sistema de navegación: notificar para que genere el NavMesh
            world = unreal.EditorLevelLibrary.get_editor_world()
            unreal.NavigationSystemV1.get_navigation_system(world).on_navigation_bounds_updated(state["nav_volume"])
            unreal.SystemLibrary.execute_console_command(world, "RebuildNavigation")
            state["phase"] = "nav"
            state["wait"] = 0
        except Exception as e:
            unreal.log_error(f"[BL_M01] ERROR: {e}")
            state["phase"] = "quit"
    elif state["phase"] == "nav":
        state["wait"] += 1
        world = unreal.EditorLevelLibrary.get_editor_world()
        building_nav = unreal.NavigationSystemV1.is_navigation_being_built_or_locked(world)
        state["seen_building"] |= building_nav
        if not building_nav and (state["seen_building"] and state["wait"] > 30 or state["wait"] > 900):
            state["phase"] = "quit"   # antes de guardar: el callback puede llegar dos veces en el mismo frame
            log(f"Navegación generada (vista en construcción: {state['seen_building']}, {state['wait']} frames)")
            ok = check_navigation()
            level_ed.save_current_level()
            log(f"Mapa guardado: {MAP_PATH} (navegación {'OK' if ok else 'CON FALLOS'})")
            log("OK" if ok else "FALLO")
            state["phase"] = "quit"
            state["wait"] = 0
        elif state["wait"] > 3000:
            unreal.log_error("[BL_M01] La navegación no terminó de construirse")
            state["phase"] = "quit"
    elif state["phase"] == "quit":
        state["wait"] += 1
        if state["wait"] > 30:
            unreal.unregister_slate_post_tick_callback(state["handle"])
            unreal.SystemLibrary.quit_editor()


state["handle"] = unreal.register_slate_post_tick_callback(tick)
