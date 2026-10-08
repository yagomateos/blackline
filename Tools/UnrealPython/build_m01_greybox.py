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


def start_point(name, x, y, yaw=0.0):
    tp = actors.spawn_actor_from_class(unreal.TargetPoint, unreal.Vector(x, y, 0), unreal.Rotator(roll=0, pitch=0, yaw=yaw))
    tp.set_actor_label(f"BLTest_Start_{name}")
    tp.tags = [unreal.Name(f"BLTest_Start_{name}")]
    return tp


def checkpoint(cid, x0, x1, y0, y1, yaw):
    """Volumen de checkpoint que cubre todo el paso (x0..x1, y0..y1); se reaparece en su centro mirando a yaw."""
    cp = actors.spawn_actor_from_class(unreal.BLCheckpointVolume, unreal.Vector((x0 + x1) / 2, (y0 + y1) / 2, 120))
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
    building("C3", 17000, 19500, 1300, 2900, 960)
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


def enemy(label, squad, x, y, yaw, patrol=()):
    e = actors.spawn_actor_from_class(unreal.BLEnemyCharacter, unreal.Vector(x, y, 96), unreal.Rotator(roll=0, pitch=0, yaw=yaw))
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


def marker(tag, x, y):
    tp = actors.spawn_actor_from_class(unreal.TargetPoint, unreal.Vector(x, y, 0))
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
            if len(c) >= 7 and not line.startswith("#") and c[1] == "radio":
                out[c[0]] = (c[2], c[6])
    return out


VOICE_LINES = load_voice_lines()


def radio(line_id):
    speaker, text = VOICE_LINES[line_id]
    line = unreal.BLRadioLine()
    line.set_editor_property("speaker", speaker)
    line.set_editor_property("text", text)
    voice_path = f"/Game/Audio/Voice/Radio/VO_{line_id}"
    if unreal.EditorAssetLibrary.does_asset_exist(voice_path):
        line.set_editor_property("voice", unreal.load_asset(voice_path))
    else:
        log(f"AVISO: sin voz para {line_id}")
    return line


def objective_def(text, kind, tag="", squad="", radius=400.0, marker_on=True, on_start=(), on_complete=()):
    o = unreal.BLObjective()
    o.set_editor_property("text", text)
    o.set_editor_property("type", kind)
    o.set_editor_property("target_tag", tag)
    o.set_editor_property("squad_id", squad)
    o.set_editor_property("radius", radius)
    o.set_editor_property("show_marker", marker_on)
    o.set_editor_property("radio_on_start", list(on_start))
    o.set_editor_property("radio_on_complete", list(on_complete))
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
                  on_complete=[radio("M01_Disco")]),
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
    for x, y, mesh, yaw in ((12450, 1150, "SM_Prop_Rubble_B", 30), (15700, 160, "SM_Prop_Rubble_A", 110),
                            (13100, 150, "SM_Prop_Rubble_C", 300), (18200, 1170, "SM_Prop_Rubble_C", 75)):
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
    x0, x1, y0, y1 = -200, 21200, -8200, 4200
    nav = actors.spawn_actor_from_class(unreal.NavMeshBoundsVolume, unreal.Vector((x0 + x1) / 2, (y0 + y1) / 2, 400))
    nav.set_actor_scale3d(unreal.Vector((x1 - x0) / 200, (y1 - y0) / 200, 1000 / 200))
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
    for name, b in (("Limite_N", (-300, 21300, -8300, -8240)), ("Limite_S", (-300, 21300, 4240, 4300)),
                    ("Limite_O", (-300, -240, -8300, 4300)), ("Limite_E", (21240, 21300, -8300, 4300))):
        box(name, b[0], b[1], b[2], b[3], 0, 800, M_WALL)
    box("Relleno_Oeste", -240, 1460, -8240, -1600, 0, 600, M_WALL)
    finish_buildings()
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
           ("Objetivo", unreal.Vector(20300, 600, 100))]
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
