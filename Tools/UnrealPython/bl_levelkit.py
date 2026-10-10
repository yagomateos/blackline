"""Kit común para construir niveles de misión por script (misiones 2-5). Sale de build_m01_greybox.py, que NO se
toca (la misión 1 ya está probada): aquí están las mismas piezas (cajas, atrezo con coberturas, edificios de
fachadas, coberturas, marcadores, puntos de inicio, checkpoints, milicianos, radio, objetivos, oleadas, humo/fuego,
ambiente, acústica) y el arranque común (cargar/crear el mapa, construir, generar el NavMesh, comprobar rutas y
guardar).

Uso desde un script de misión (ejecutar con -ExecCmds="py <script>", no con -ExecutePythonScript):
    import bl_levelkit as K
    def build(): ...            # usa K.box, K.prop, K.enemy...
    K.run("/Game/Maps/M02/L_M02_Manifiesto", build, nav_bounds=(x0, x1, y0, y1, z0, z1), nav_checks=[("A", (x, y, z)), ...])
"""
import math
import os
import random

import unreal

CUBE = unreal.load_asset("/Engine/BasicShapes/Cube.Cube")
ENV = "/Game/Environment/Materials/"
PROPS = "/Game/Environment/Props/"
REFINERY = "/Game/Environment/Refinery/"

actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
level_ed = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
rng = random.Random(4242)
LOG_TAG = ["BL_Level"]


def log(m):
    unreal.log(f"[{LOG_TAG[0]}] {m}")


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


def surface_of(m):
    n = m.get_name() if m else ""
    return "Metal" if ("Metal" in n or "Container" in n or "Rust" in n) else "Wood" if "Wood" in n else "Dirt" if "Dirt" in n else "Concrete"


# ---------------------------------------------------------------------------
# Geometría
# ---------------------------------------------------------------------------
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


def hidden_blocker(name, x0, x1, y0, y1, z0, z1, nav=True):
    """Muro invisible (límites, bordes de agua). nav=False: no recorta el NavMesh."""
    a = box(name, x0, x1, y0, y1, z0, z1, M_WALL)
    a.set_actor_hidden_in_game(True)
    # Solo para los cuerpos: no corta la vista (IA, aliados), las balas ni la cámara
    # (perfil propio de DefaultEngine.ini: las respuestas por canal sueltas no se guardaban con el mapa)
    a.static_mesh_component.set_collision_profile_name("BLInvisibleWall")
    if not nav:
        a.static_mesh_component.set_editor_property("can_ever_affect_navigation", False)
    return a


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
    "SM_Prop_PalletStack": [(0, -90, True), (0, 90, True)],
    "SM_Ref_PipeLow": [(500, -165, True), (500, 165, True)],
    "SM_Ref_Crate_Drones": [(0, -100, True), (-140, 0, True), (140, 0, True)],
}
COVER_COUNT = [0]


def cover_point(x, y, toward_x, toward_y, low, z=0.0):
    yaw = math.degrees(math.atan2(toward_y - y, toward_x - x))
    c = actors.spawn_actor_from_class(unreal.BLCoverPoint, unreal.Vector(x, y, z), unreal.Rotator(roll=0, pitch=0, yaw=yaw))
    c.set_editor_property("low_cover", low)
    c.set_actor_label(f"Cobertura_{COVER_COUNT[0]:03d}")
    COVER_COUNT[0] += 1
    return c


def covers_for_box(x0, x1, y0, y1, low, z=0.0):
    cx, cy = (x0 + x1) / 2, (y0 + y1) / 2
    for px, py in ((x0 - 55, cy), (x1 + 55, cy), (cx, y0 - 55), (cx, y1 + 55)):
        cover_point(px, py, cx, cy, low, z)


def covers_for_building(x0, x1, y0, y1):
    """Esquinas de edificio: dos puntos por esquina (uno en cada fachada), coberturas altas para asomarse."""
    cx, cy = (x0 + x1) / 2, (y0 + y1) / 2
    for ex, sx in ((x0, -1), (x1, 1)):
        for ey, sy in ((y0, -1), (y1, 1)):
            cover_point(ex + sx * 55, ey - sy * 90, cx, ey - sy * 90, False)
            cover_point(ex - sx * 90, ey + sy * 55, ex - sx * 90, cy, False)


def covers_for_round(cx, cy, r, n=8, low=False):
    """Coberturas alrededor de un depósito o columna (radio r)."""
    for i in range(n):
        a = 2 * math.pi * (i + 0.5) / n
        cover_point(cx + (r + 55) * math.cos(a), cy + (r + 55) * math.sin(a), cx, cy, low)


def prop(mesh, x, y, yaw=0.0, z=0.0, label=None, material=None, path=None, scale=None):
    sm = unreal.load_asset((path or PROPS) + mesh)
    a = actors.spawn_actor_from_class(unreal.StaticMeshActor, unreal.Vector(x, y, z), unreal.Rotator(roll=0, pitch=0, yaw=yaw))
    a.static_mesh_component.set_static_mesh(sm)
    if material:
        a.static_mesh_component.set_material(0, material)
    if scale:
        a.set_actor_scale3d(unreal.Vector(*scale))
    a.set_actor_label(label or mesh)
    if z < 1.0:
        c, s_ = math.cos(math.radians(yaw)), math.sin(math.radians(yaw))
        for lx, ly, low in COVER_SPECS.get(mesh, []):
            cover_point(x + lx * c - ly * s_, y + lx * s_ + ly * c, x, y, low)
    return a


def ref(mesh, x, y, yaw=0.0, z=0.0, label=None, scale=None):
    """Atrezo de la refinería (gen_refinery.py)."""
    return prop(mesh, x, y, yaw, z, label, path=REFINERY, scale=scale)


def facade_building(name, x0, x1, y0, y1, h, wall, face_mask=15, cornice_mask=15, street_mask=0, seed=1, no_ground=False, z=0.0):
    a = actors.spawn_actor_from_class(unreal.BLBuilding, unreal.Vector((x0 + x1) / 2, (y0 + y1) / 2, z))
    a.set_actor_label(name)
    a.set_editor_property("size", unreal.Vector(x1 - x0, y1 - y0, h))
    a.set_editor_property("seed", seed)
    a.set_editor_property("face_mask", face_mask)
    a.set_editor_property("cornice_mask", cornice_mask)
    a.set_editor_property("street_mask", street_mask)
    a.set_editor_property("no_ground_floor", no_ground)
    a.set_editor_property("wall_material", mat(f"MI_Fac_Wall_{wall}"))
    a.set_editor_property("phys_material", unreal.load_asset("/Game/Environment/PhysicalMaterials/PM_Concrete"))
    return a


def wall_x(name, y0, y1, xa, xb, z0, z1, holes, m):
    """Muro a lo largo de X (grosor y0..y1) con huecos [(x0, x1, zb, zt)] (ventanas y puertas)."""
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
    """Muro a lo largo de Y (grosor x0..x1) con huecos [(y0, y1, zb, zt)]."""
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


def stair_flight(name, xa, xb, y_start, direction, z_base, steps=9, rise=160.0 / 9, run=28.0, m=None):
    for i in range(steps):
        ya = y_start + direction * run * i
        yb = ya + direction * run
        top = z_base + rise * (i + 1)
        box(f"{name}_{i}", xa, xb, min(ya, yb), max(ya, yb), top - 40.0, top, m or M_FLOOR)


def stair_flight_x(name, ya, yb, x_start, direction, z_base, steps=9, rise=160.0 / 9, run=28.0, m=None):
    """Igual que stair_flight pero subiendo a lo largo de X."""
    for i in range(steps):
        xa = x_start + direction * run * i
        xb = xa + direction * run
        top = z_base + rise * (i + 1)
        box(f"{name}_{i}", min(xa, xb), max(xa, xb), ya, yb, top - 40.0, top, m or M_FLOOR)


def catwalk(name, x0, x1, y0, y1, z, rails="NSEW", m=None):
    """Pasarela metálica a altura z con barandillas en los lados indicados."""
    m = m or M_METAL
    box(f"{name}_Suelo", x0, x1, y0, y1, z - 12, z, m)
    if "N" in rails:
        box(f"{name}_BarN", x0, x1, y0, y0 + 6, z + 95, z + 105, m)
    if "S" in rails:
        box(f"{name}_BarS", x0, x1, y1 - 6, y1, z + 95, z + 105, m)
    if "W" in rails:
        box(f"{name}_BarO", x0, x0 + 6, y0, y1, z + 95, z + 105, m)
    if "E" in rails:
        box(f"{name}_BarE", x1 - 6, x1, y0, y1, z + 95, z + 105, m)


# ---------------------------------------------------------------------------
# Luces
# ---------------------------------------------------------------------------
def point_light(name, x, y, z, color, intensity, radius, shadows=False, lit=False, draw_distance=0.0):
    l = actors.spawn_actor_from_class(unreal.PointLight, unreal.Vector(x, y, z))
    l.set_actor_label(name)
    c = l.get_component_by_class(unreal.PointLightComponent)
    c.set_mobility(unreal.ComponentMobility.MOVABLE)
    c.set_editor_property("intensity_units", unreal.LightUnits.CANDELAS)
    c.set_intensity(intensity)
    c.set_light_color(unreal.LinearColor(*color, 1))
    c.set_attenuation_radius(radius)
    c.set_cast_shadows(shadows)
    if draw_distance > 0:
        c.set_editor_property("max_draw_distance", draw_distance)
        c.set_editor_property("max_distance_fade_range", draw_distance * 0.2)
    if lit:
        l.tags = [unreal.Name("BLLit")]   # de noche, quien pase por debajo se ve (ABLNightSettings)
    return l


def sodium_light(name, x, y, yaw, shadows=False, intensity=7.0, radius=1600.0, lit=False):
    """Farola de sodio: poste + brazo + luz puntual naranja."""
    box(name + "_Poste", x - 8, x + 8, y - 8, y + 8, 0, 700, M_METAL)
    dx, dy = math.cos(math.radians(yaw)) * 120, math.sin(math.radians(yaw)) * 120
    box(name + "_Brazo", min(x, x + dx) - 5, max(x, x + dx) + 5, min(y, y + dy) - 5, max(y, y + dy) + 5, 690, 700, M_METAL)
    return point_light(name, x + dx, y + dy, 670, (1.0, 0.55, 0.2), intensity, radius, shadows, lit)


def alarm_light(name, x, y, yaw, start_on=False, intensity=2500.0, radius=5000.0, cone=32.0, siren=False, tower=True):
    """Torre de focos (ABLAlarmLight): apagada hasta la alarma; ilumina al jugador para la IA."""
    a = actors.spawn_actor_from_class(unreal.BLAlarmLight, unreal.Vector(x, y, 0), unreal.Rotator(roll=0, pitch=0, yaw=yaw))
    a.set_actor_label(name)
    a.set_editor_property("start_on", start_on)
    a.set_editor_property("intensity", intensity)
    a.set_editor_property("radius", radius)
    a.set_editor_property("cone_angle", cone)
    if tower:
        a.get_component_by_class(unreal.StaticMeshComponent).set_static_mesh(unreal.load_asset(REFINERY + "SM_Ref_FloodTower"))
    if siren:
        a.set_editor_property("siren_sound", unreal.load_asset("/Game/Audio/Ambience/Zones/SW_AmbZ_Siren_Loop"))
    a.tags = [unreal.Name("BLAlarm"), unreal.Name("BLLit")]
    return a


# ---------------------------------------------------------------------------
# Juego: inicio, checkpoints, marcadores, enemigos, radio, objetivos
# ---------------------------------------------------------------------------
def start_point(name, x, y, yaw=0.0, z=0.0):
    tp = actors.spawn_actor_from_class(unreal.TargetPoint, unreal.Vector(x, y, z), unreal.Rotator(roll=0, pitch=0, yaw=yaw))
    tp.set_actor_label(f"BLTest_Start_{name}")
    tp.tags = [unreal.Name(f"BLTest_Start_{name}")]
    return tp


def player_start(x, y, yaw=0.0, z=0.0):
    actors.spawn_actor_from_class(unreal.PlayerStart, unreal.Vector(x, y, z + 100), unreal.Rotator(roll=0, pitch=0, yaw=yaw))


def checkpoint(cid, x0, x1, y0, y1, yaw, z=0.0):
    cp = actors.spawn_actor_from_class(unreal.BLCheckpointVolume, unreal.Vector((x0 + x1) / 2, (y0 + y1) / 2, 120 + z))
    cp.set_actor_label(f"Checkpoint_{cid}")
    cp.set_editor_property("checkpoint_id", cid)
    cp.set_actor_scale3d(unreal.Vector((x1 - x0) / 300, (y1 - y0) / 300, 1))
    arrow = cp.get_component_by_class(unreal.ArrowComponent)
    arrow.set_world_rotation(unreal.Rotator(roll=0, pitch=0, yaw=yaw), False, False)
    arrow.set_world_scale3d(unreal.Vector(1, 1, 1))
    return cp


def marker(tag, x, y, z=0.0):
    tp = actors.spawn_actor_from_class(unreal.TargetPoint, unreal.Vector(x, y, z))
    tp.set_actor_label(tag)
    tp.tags = [unreal.Name(tag)]
    return tp


ROLES = {"gunner": "GUNNER", "operator": "OPERATOR", "sniper": "SNIPER", "ally": "ALLY"}


def enemy(label, squad, x, y, yaw, patrol=(), z=0.0, flashlight=False, gunner=False, role=None):
    e = actors.spawn_actor_from_class(unreal.BLEnemyCharacter, unreal.Vector(x, y, z + 96), unreal.Rotator(roll=0, pitch=0, yaw=yaw))
    e.set_actor_label(label)
    e.set_editor_property("squad_id", squad)
    e.set_editor_property("patrol_points", [unreal.Vector(px, py, z) for px, py in patrol])
    e.set_editor_property("flashlight", flashlight)
    if gunner:
        role = "gunner"
    if role:
        e.set_editor_property("enemy_role", getattr(unreal.BLEnemyRole, ROLES[role]))
    if role == "ally":
        e.set_editor_property("ai_controller_class", unreal.BLAllyController)   # soldado del ejército (misión 4)
    e.tags = [unreal.Name("BLEnemy"), unreal.Name(f"BLEnemy_{label}")]
    return e


def load_voice_lines():
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
    line.set_editor_property("person", in_person)
    voice_path = f"/Game/Audio/Voice/Radio/VO_{line_id}"
    if unreal.EditorAssetLibrary.does_asset_exist(voice_path):
        line.set_editor_property("voice", unreal.load_asset(voice_path))
    else:
        log(f"AVISO: sin voz para {line_id}")
    return line


def wave(tags, count, delay, smoke=False, radio_lines=(), gunners=0, flashlights=False, operators=0):
    w = unreal.BLWave()
    w.set_editor_property("spawn_tags", [unreal.Name(t) for t in tags])
    w.set_editor_property("count", count)
    w.set_editor_property("delay", delay)
    w.set_editor_property("smoke", smoke)
    w.set_editor_property("radio", list(radio_lines))
    w.set_editor_property("gunners", gunners)
    w.set_editor_property("flashlights", flashlights)
    w.set_editor_property("operators", operators)
    return w


REACH = unreal.BLObjectiveType.REACH
CLEAR = unreal.BLObjectiveType.CLEAR_SQUAD
USE = unreal.BLObjectiveType.INTERACT
USE_ALL = unreal.BLObjectiveType.INTERACT_ALL
DEFEND = unreal.BLObjectiveType.DEFEND


def objective_def(text, kind, tag="", squad="", radius=400.0, marker_on=True, on_start=(), on_complete=(), waves=(),
                  min_duration=60.0, check_height=False, activate=(), progress="", time_limit=0.0, fail_text=""):
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
    o.set_editor_property("progress_label", progress)
    o.set_editor_property("time_limit", time_limit)
    o.set_editor_property("fail_text", fail_text)
    return o


def interactable(label, tag, x, y, z, prompt, hold=1.0, pickup=False, photo=False, mesh=None, yaw=0.0, use_radio=()):
    a = actors.spawn_actor_from_class(unreal.BLInteractable, unreal.Vector(x, y, z), unreal.Rotator(roll=0, pitch=0, yaw=yaw))
    a.set_actor_label(label)
    a.set_editor_property("prompt", prompt)
    a.set_editor_property("hold_time", hold)
    a.set_editor_property("pickup", pickup)
    a.set_editor_property("photo", photo)
    a.set_editor_property("use_radio", list(use_radio))
    if photo:
        a.set_editor_property("use_sound", unreal.load_asset("/Game/Audio/UI/SW_UI_Shutter"))
    if mesh:
        a.get_component_by_class(unreal.StaticMeshComponent).set_static_mesh(unreal.load_asset(mesh))
    a.tags = [unreal.Name(tag)]
    return a


def director(name, objectives, briefing=(), debriefing=(), phases=None, alarm_on_detection=False, alarm_radio=()):
    d = actors.spawn_actor_from_class(unreal.BLMissionDirector, unreal.Vector(0, 0, 300))
    d.set_actor_label("Director_Mision")
    d.set_editor_property("mission_name", name)
    d.set_editor_property("briefing", list(briefing))
    d.set_editor_property("objectives", list(objectives))
    d.set_editor_property("debriefing", list(debriefing))
    if phases:
        d.set_editor_property("phase_objectives", phases)
    d.set_editor_property("alarm_on_detection", alarm_on_detection)
    d.set_editor_property("alarm_radio", list(alarm_radio))
    return d


# ---------------------------------------------------------------------------
# Efectos y audio
# ---------------------------------------------------------------------------
def smoke(label, x, y, z=0.0, **props):
    a = actors.spawn_actor_from_class(unreal.BLSmokeEmitter, unreal.Vector(x, y, z))
    a.set_actor_label(label)
    for k, v in props.items():
        a.set_editor_property(k, v)
    return a


def ambient(sound, x, y, z, volume=1.0, label=None, folder="/Game/Audio/Ambience/Zones/"):
    a = actors.spawn_actor_from_class(unreal.AmbientSound, unreal.Vector(x, y, z))
    a.set_actor_label(label or sound)
    c = a.get_component_by_class(unreal.AudioComponent)
    c.set_sound(unreal.load_asset(folder + sound))
    c.set_editor_property("volume_multiplier", volume)
    c.set_editor_property("auto_activate", True)
    return a


def reverb_zone(label, x0, x1, y0, y1, z1, effect, priority=1.0, ambience=1.0, volume=1.0):
    z = actors.spawn_actor_from_class(unreal.BLReverbZone, unreal.Vector((x0 + x1) / 2, (y0 + y1) / 2, z1 / 2))
    z.set_actor_label(label)
    z.tags = [unreal.Name(label)]
    z.get_component_by_class(unreal.BoxComponent).set_box_extent(unreal.Vector((x1 - x0) / 2, (y1 - y0) / 2, z1 / 2))
    z.set_editor_property("reverb", unreal.load_asset("/Game/Audio/Settings/" + effect))
    z.set_editor_property("priority", priority)
    z.set_editor_property("ambience_scale", ambience)
    z.set_editor_property("reverb_volume", volume)
    return z


def night_lighting(moon_intensity=0.06, sky_intensity=0.35, fog_color=(0.06, 0.045, 0.035), exposure_bias=1.0, glow=(0.9, 0.42, 0.15)):
    """Noche cerrada: luna tenue y fría, cielo oscuro, niebla volumétrica con el resplandor naranja de la refinería."""
    moon = actors.spawn_actor_from_class(unreal.DirectionalLight, unreal.Vector(0, 0, 2000), unreal.Rotator(roll=0, pitch=-38, yaw=-60))
    mc = moon.get_component_by_class(unreal.DirectionalLightComponent)
    mc.set_mobility(unreal.ComponentMobility.MOVABLE)
    mc.set_intensity(moon_intensity)
    mc.set_light_color(unreal.LinearColor(0.55, 0.65, 1.0, 1))
    mc.set_editor_property("atmosphere_sun_light", True)
    sky = actors.spawn_actor_from_class(unreal.SkyLight, unreal.Vector(0, 0, 1500))
    skc = sky.get_component_by_class(unreal.SkyLightComponent)
    skc.set_mobility(unreal.ComponentMobility.MOVABLE)
    skc.set_editor_property("real_time_capture", True)
    skc.set_intensity(sky_intensity)
    skc.set_editor_property("lower_hemisphere_is_black", False)
    skc.set_editor_property("lower_hemisphere_color", unreal.LinearColor(0.02, 0.018, 0.016, 1))
    skc.set_light_color(unreal.LinearColor(0.6, 0.7, 1.0, 1))
    actors.spawn_actor_from_class(unreal.SkyAtmosphere, unreal.Vector(0, 0, 0))
    fog = actors.spawn_actor_from_class(unreal.ExponentialHeightFog, unreal.Vector(0, 0, 0))
    fc = fog.get_component_by_class(unreal.ExponentialHeightFogComponent)
    fc.set_editor_property("fog_density", 0.05)
    fc.set_editor_property("fog_height_falloff", 0.12)
    fc.set_editor_property("start_distance", 600.0)
    fc.set_editor_property("fog_inscattering_luminance", unreal.LinearColor(*fog_color, 1))
    # Niebla volumétrica corta: los haces de linternas, focos y el dron se ven en el aire (coste medido aparte)
    fc.set_editor_property("enable_volumetric_fog", True)
    fc.set_editor_property("volumetric_fog_scattering_distribution", 0.6)
    fc.set_editor_property("volumetric_fog_extinction_scale", 1.2)
    fc.set_editor_property("volumetric_fog_distance", 4000.0)
    ppv = actors.spawn_actor_from_class(unreal.PostProcessVolume, unreal.Vector(0, 0, 0))
    ppv.set_editor_property("unbound", True)
    s = ppv.get_editor_property("settings")
    v4 = unreal.Vector4
    for name, value in (("auto_exposure_bias", exposure_bias),
                        ("auto_exposure_min_brightness", 0.03),
                        ("auto_exposure_max_brightness", 1.5),
                        ("local_exposure_shadow_contrast_scale", 0.7),
                        ("local_exposure_highlight_contrast_scale", 0.6),
                        ("color_saturation", v4(0.82, 0.82, 0.86, 1.0)),
                        ("color_contrast", v4(1.1, 1.1, 1.1, 1.0)),
                        ("color_gain_shadows", v4(0.85, 0.95, 1.12, 1.0)),
                        ("color_gain_highlights", v4(1.08, 1.0, 0.9, 1.0)),
                        ("vignette_intensity", 0.5),
                        ("film_grain_intensity", 0.12),
                        ("bloom_intensity", 0.7)):
        s.set_editor_property("override_" + name, True)
        s.set_editor_property(name, value)
    ppv.set_editor_property("settings", s)
    night = actors.spawn_actor_from_class(unreal.BLNightSettings, unreal.Vector(0, 0, 0))
    night.set_actor_label("Noche")
    return night


def dawn_fog_lighting(sun_yaw=165.0, sun_pitch=-7.0, sun_intensity=2.2, fog_density=0.11, fog_falloff=0.35,
                      fog_color=(0.46, 0.5, 0.55), volumetric=True):
    """Amanecer con niebla espesa pegada al agua (misión 3): sol rasante y tibio, la niebla se come lo lejano."""
    sun = actors.spawn_actor_from_class(unreal.DirectionalLight, unreal.Vector(0, 0, 2000), unreal.Rotator(roll=0, pitch=sun_pitch, yaw=sun_yaw))
    sc = sun.get_component_by_class(unreal.DirectionalLightComponent)
    sc.set_mobility(unreal.ComponentMobility.MOVABLE)
    sc.set_intensity(sun_intensity)
    sc.set_light_color(unreal.LinearColor(1.0, 0.8, 0.62, 1))
    sc.set_editor_property("atmosphere_sun_light", True)
    sky = actors.spawn_actor_from_class(unreal.SkyLight, unreal.Vector(0, 0, 1500))
    skc = sky.get_component_by_class(unreal.SkyLightComponent)
    skc.set_mobility(unreal.ComponentMobility.MOVABLE)
    skc.set_editor_property("real_time_capture", True)
    skc.set_intensity(1.6)
    skc.set_editor_property("lower_hemisphere_is_black", False)
    skc.set_editor_property("lower_hemisphere_color", unreal.LinearColor(0.08, 0.08, 0.08, 1))
    skc.set_light_color(unreal.LinearColor(0.82, 0.88, 1.0, 1))
    actors.spawn_actor_from_class(unreal.SkyAtmosphere, unreal.Vector(0, 0, 0))
    fog = actors.spawn_actor_from_class(unreal.ExponentialHeightFog, unreal.Vector(0, 0, -300))
    fc = fog.get_component_by_class(unreal.ExponentialHeightFogComponent)
    fc.set_editor_property("fog_density", fog_density)
    fc.set_editor_property("fog_height_falloff", fog_falloff)
    fc.set_editor_property("start_distance", 300.0)
    fc.set_editor_property("fog_inscattering_luminance", unreal.LinearColor(*fog_color, 1))
    if volumetric:
        fc.set_editor_property("enable_volumetric_fog", True)
        fc.set_editor_property("volumetric_fog_scattering_distribution", 0.75)   # el sol "abre" la niebla de frente
        fc.set_editor_property("volumetric_fog_distance", 5000.0)
    ppv = actors.spawn_actor_from_class(unreal.PostProcessVolume, unreal.Vector(0, 0, 0))
    ppv.set_editor_property("unbound", True)
    st = ppv.get_editor_property("settings")
    v4 = unreal.Vector4
    for name, value in (("auto_exposure_bias", 0.3),
                        ("local_exposure_shadow_contrast_scale", 0.65),
                        ("color_saturation", v4(0.75, 0.75, 0.78, 1.0)),
                        ("color_contrast", v4(1.08, 1.08, 1.08, 1.0)),
                        ("color_gain_shadows", v4(0.9, 0.96, 1.06, 1.0)),
                        ("color_gain_highlights", v4(1.06, 1.0, 0.94, 1.0)),
                        ("vignette_intensity", 0.4),
                        ("film_grain_intensity", 0.06),
                        ("bloom_intensity", 0.55)):
        st.set_editor_property("override_" + name, True)
        st.set_editor_property(name, value)
    ppv.set_editor_property("settings", st)


def door_with_frame(label, hinge_x, hinge_y, yaw, barred=False, tag=None, scale=1.5):
    """Puerta ABLDoor con su marco (pivote en la bisagra; la hoja va hacia +Y local). scale 1.5 = hueco de 150 (NavMesh)."""
    fr = prop("SM_Door_Frame", hinge_x, hinge_y, yaw, label=label + "_Marco", path="/Game/Environment/OldTown/", scale=(1, scale, 1))
    d = actors.spawn_actor_from_class(unreal.BLDoor, unreal.Vector(hinge_x, hinge_y, 0), unreal.Rotator(roll=0, pitch=0, yaw=yaw))
    d.set_actor_label(label)
    d.set_actor_scale3d(unreal.Vector(1, scale, 1))
    d.set_editor_property("barred", barred)
    if tag:
        d.tags = [unreal.Name(tag)]
    return d


def mover(label, x, y, z, yaw=0.0, mesh=None, path=(), speed=300.0, accel=25.0, turn=3.0, start_sound=None, loop_sound=None,
          hidden=False, hide_at_end=False, tag=None, delay=0.0):
    m = actors.spawn_actor_from_class(unreal.BLScriptedMover, unreal.Vector(x, y, z), unreal.Rotator(roll=0, pitch=0, yaw=yaw))
    m.set_actor_label(label)
    if mesh:
        m.get_component_by_class(unreal.StaticMeshComponent).set_static_mesh(unreal.load_asset(mesh))
    m.set_editor_property("path", [unreal.Vector(*p) for p in path])
    m.set_editor_property("speed", speed)
    m.set_editor_property("acceleration", accel)
    m.set_editor_property("turn_rate", turn)
    m.set_editor_property("start_delay", delay)
    m.set_editor_property("hidden_until_active", hidden)
    m.set_editor_property("hide_at_end", hide_at_end)
    if start_sound:
        m.set_editor_property("start_sound", unreal.load_asset(start_sound))
    if loop_sound:
        m.set_editor_property("loop_sound", unreal.load_asset(loop_sound))
    if tag:
        m.tags = [unreal.Name(tag)]
    return m


# ---------------------------------------------------------------------------
# Arranque común: construir, NavMesh, comprobar rutas, guardar y salir
# ---------------------------------------------------------------------------
def run(map_path, build_fn, nav_bounds, nav_checks, tag="BL_Level"):
    """nav_bounds = (x0, x1, y0, y1, z0, z1); nav_checks = [(nombre, (x, y, z)), ...] recorridos en orden."""
    LOG_TAG[0] = tag
    state = {"frame": 0, "phase": "build", "wait": 0, "seen": False, "nav": None, "handle": None}

    def build():
        if unreal.EditorAssetLibrary.does_asset_exist(map_path):
            if not level_ed.load_level(map_path):
                raise RuntimeError(f"No se pudo cargar {map_path}")
        elif not level_ed.new_level(map_path):
            raise RuntimeError(f"No se pudo crear {map_path}")
        world = unreal.EditorLevelLibrary.get_editor_world()
        if not world.get_path_name().startswith(map_path):
            raise RuntimeError(f"El mapa abierto es {world.get_path_name()}, no {map_path}")
        old = actors.get_all_level_actors()
        if old:
            actors.destroy_actors(old)
        build_fn()
        x0, x1, y0, y1, z0, z1 = nav_bounds
        nav = actors.spawn_actor_from_class(unreal.NavMeshBoundsVolume, unreal.Vector((x0 + x1) / 2, (y0 + y1) / 2, (z0 + z1) / 2))
        nav.set_actor_scale3d(unreal.Vector((x1 - x0) / 200, (y1 - y0) / 200, (z1 - z0) / 200))
        nav.set_actor_label("NavMeshBounds")
        state["nav"] = nav
        log(f"Coberturas: {COVER_COUNT[0]}, actores: {len(actors.get_all_level_actors())}")

    def check():
        world = unreal.EditorLevelLibrary.get_editor_world()
        ok = True
        for (na, a), (nb, b) in zip(nav_checks, nav_checks[1:]):
            path = unreal.NavigationSystemV1.find_path_to_location_synchronously(world, unreal.Vector(*a), unreal.Vector(*b))
            n = len(path.path_points) if path else 0
            partial = path.is_partial() if path else True
            log(f"Ruta {na} -> {nb}: {n} puntos, {(path.get_path_length() if path else 0) / 100:.0f} m, parcial={partial}")
            ok &= n > 1 and not partial
        return ok

    def tick(dt):
        state["frame"] += 1
        if state["phase"] == "build" and state["frame"] == 3:
            try:
                build()
                world = unreal.EditorLevelLibrary.get_editor_world()
                unreal.NavigationSystemV1.get_navigation_system(world).on_navigation_bounds_updated(state["nav"])
                unreal.SystemLibrary.execute_console_command(world, "RebuildNavigation")
                state["phase"] = "nav"
            except Exception as e:
                unreal.log_error(f"[{tag}] ERROR: {e}")
                state["phase"] = "quit"
        elif state["phase"] == "nav":
            state["wait"] += 1
            world = unreal.EditorLevelLibrary.get_editor_world()
            busy = unreal.NavigationSystemV1.is_navigation_being_built_or_locked(world)
            state["seen"] |= busy
            if not busy and (state["seen"] and state["wait"] > 30 or state["wait"] > 900):
                state["phase"] = "quit"
                ok = check()
                level_ed.save_current_level()
                log(f"Mapa guardado: {map_path} (navegación {'OK' if ok else 'CON FALLOS'})")
                log("OK" if ok else "FALLO")
                state["wait"] = 0
            elif state["wait"] > 3000:
                unreal.log_error(f"[{tag}] La navegación no terminó de construirse")
                state["phase"] = "quit"
        elif state["phase"] == "quit":
            state["wait"] += 1
            if state["wait"] > 30:
                unreal.unregister_slate_post_tick_callback(state["handle"])
                unreal.SystemLibrary.quit_editor()

    state["handle"] = unreal.register_slate_post_tick_callback(tick)
