"""Construye /Game/Maps/Dev/L_Dev_Movement: nivel de pruebas del movimiento FPS.

Estaciones (cada una con un TargetPoint "BLTest_Start_<Nombre>" que usa BLAutoTestComponent):
  - Pista abierta en +X desde el PlayerStart: andar, sprint, agacharse, ADS, lean, salto.
  - Mantle100 / Mantle150: obstáculos de 100 y 150 cm (deben poder encaramarse).
  - Muro250: muro de 250 cm (NO debe permitir mantle).
  - Escaleras: 6 peldaños de 20 cm + rellano a 120 cm.
  - LeanPared: pared a la derecha a 36 cm del centro del jugador (debe limitar la inclinación).
  - Tiro: galería de tiro, muro a 10 m y cajas con física a 6 m (prueba de armas).
  - Dianas: tres maniquíes (ABLTargetDummy) a 9 m delante de un muro (salpicaduras) y un checkpoint
    (ABLCheckpointVolume) a la entrada de la estación (prueba de combate).

Ejecutar:  UnrealEditor.exe Blackline.uproject -ExecutePythonScript="Tools/UnrealPython/build_dev_movement_map.py"
"""
import unreal

MAP_PATH = "/Game/Maps/Dev/L_Dev_Movement"
CUBE = unreal.load_asset("/Engine/BasicShapes/Cube.Cube")  # 100 cm, pivote centrado
ENV = "/Game/Environment/Materials/"
# Materiales reales (Poly Haven, triplanar) con su superficie física; si aún no existen, la rejilla de Epic
MAT = unreal.load_asset(ENV + "MI_Env_ConcreteWall") or unreal.load_asset("/Game/LevelPrototyping/Materials/MI_PrototypeGrid_Gray.MI_PrototypeGrid_Gray")
MAT_DARK = unreal.load_asset(ENV + "MI_Env_ConcreteFloor") or unreal.load_asset("/Game/LevelPrototyping/Materials/MI_PrototypeGrid_TopDark.MI_PrototypeGrid_TopDark")
MAT_PLASTER = unreal.load_asset(ENV + "MI_Env_Plaster") or MAT
MAT_METAL = unreal.load_asset(ENV + "MI_Env_MetalPlate") or MAT
MAT_RUST = unreal.load_asset(ENV + "MI_Env_RustyMetal") or MAT
MAT_WOOD = unreal.load_asset(ENV + "MI_Env_WoodPlanks") or MAT
MAT_DIRT = unreal.load_asset(ENV + "MI_Env_Dirt") or MAT
MAT_GLASS = unreal.load_asset(ENV + "M_Env_Glass") or MAT

actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
level_ed = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)


def log(m):
    unreal.log(f"[BL_DevMap] {m}")


def box(name, x0, x1, y0, y1, z0, z1, mat=MAT, tags=()):
    """Caja definida por sus límites en cm."""
    cx, cy, cz = (x0 + x1) / 2, (y0 + y1) / 2, (z0 + z1) / 2
    a = actors.spawn_actor_from_class(unreal.StaticMeshActor, unreal.Vector(cx, cy, cz))
    a.set_actor_label(name)
    c = a.static_mesh_component
    c.set_static_mesh(CUBE)
    c.set_material(0, mat)
    a.set_actor_scale3d(unreal.Vector((x1 - x0) / 100, (y1 - y0) / 100, (z1 - z0) / 100))
    if tags:
        a.tags = [unreal.Name(t) for t in tags]
    # Superficie física explícita (la de las instancias de material no llega a la colisión simple)
    name = mat.get_name() if mat else ""
    surface = "Metal" if "Metal" in name else "Wood" if "Wood" in name else "Dirt" if "Dirt" in name else "Glass" if "Glass" in name else "Concrete"
    pm = unreal.load_asset(f"/Game/Environment/PhysicalMaterials/PM_{surface}")
    if pm:
        c.set_phys_material_override(pm)
    return a


def start_point(name, x, y, yaw=0.0):
    tp = actors.spawn_actor_from_class(unreal.TargetPoint, unreal.Vector(x, y, 0),
                                       unreal.Rotator(roll=0, pitch=0, yaw=yaw))
    tp.set_actor_label(f"BLTest_Start_{name}")
    tp.tags = [unreal.Name(f"BLTest_Start_{name}")]
    return tp


def build():
    level_ed.new_level(MAP_PATH)
    # Si el mapa ya estaba cargado (mapa de inicio del editor), new_level no lo vacía: borrar todo
    old = actors.get_all_level_actors()
    if old:
        actors.destroy_actors(old)
        log(f"Borrados {len(old)} actores previos")

    # Suelo 100 x 100 m
    box("Suelo", -3000, 7000, -5000, 5000, -20, 0, MAT_DARK)

    # Jugador: pista abierta hacia +X
    actors.spawn_actor_from_class(unreal.PlayerStart, unreal.Vector(0, 0, 100), unreal.Rotator(roll=0, pitch=0, yaw=0))

    R = 34  # radio de la cápsula
    # Mantle 100 cm
    box("Mantle100", 300, 500, -1650, -1350, 0, 100)
    start_point("Mantle100", 300 - R - 30, -1500)
    # Mantle 150 cm
    box("Mantle150", 300, 500, -2350, -2050, 0, 150)
    start_point("Mantle150", 300 - R - 30, -2200)
    # Muro 250 cm (no se puede encaramar)
    box("Muro250", 300, 350, -3050, -2750, 0, 250)
    start_point("Muro250", 300 - R - 30, -2900)
    # Escaleras: 6 peldaños de 20 cm y 35 cm de huella + rellano largo a 120 cm
    for i in range(6):
        box(f"Peldaño{i + 1}", 300 + 35 * i, 300 + 35 * (i + 1), -3700, -3500, 0, 20 * (i + 1))
    box("Rellano", 510, 1810, -3700, -3500, 0, 120)
    start_point("Escaleras", 150, -3600)
    # Pared para limitar el lean a la derecha (+Y es la derecha mirando a +X)
    wall_face = -1500 + 36
    box("ParedLean", 1800, 2300, wall_face, wall_face + 50, 0, 300)
    start_point("LeanPared", 2000, -1500)

    # Galería de tiro (+Y): muro de impactos a 10 m y cajas con física
    start_point("Tiro", 0, 2200)
    box("MuroTiro", 1000, 1060, 1600, 2800, 0, 350)
    box("MesaTiro", 560, 640, 1900, 2500, 0, 90, MAT_WOOD)
    # Dianas de materiales (a 8 m, a la izquierda del muro): acero, madera, cristal, tierra
    box("DianaMetal", 790, 800, 1250, 1450, 20, 200, MAT_METAL, ("BLTarget_Metal",))
    box("PosteMetal", 800, 810, 1340, 1360, 0, 200, MAT_RUST)
    box("DianaMadera", 790, 805, 1000, 1200, 0, 220, MAT_WOOD, ("BLTarget_Wood",))
    box("DianaCristal", 798, 800, 750, 950, 60, 210, MAT_GLASS, ("BLTarget_Glass",))
    box("MarcoCristal", 795, 803, 740, 960, 50, 60, MAT_RUST)
    box("DianaTierra", 700, 900, 400, 650, 0, 70, MAT_DIRT, ("BLTarget_Dirt",))
    box("DianaHormigon", 790, 830, 120, 320, 0, 230, MAT, ("BLTarget_Concrete",))
    start_point("Materiales", 0, 900, -0.0)
    for i, y in enumerate((2000, 2200, 2400)):
        c = box(f"CajaFisica{i + 1}", 575, 625, y - 25, y + 25, 92, 142, MAT_WOOD)
        comp = c.static_mesh_component
        comp.set_mobility(unreal.ComponentMobility.MOVABLE)
        comp.set_simulate_physics(True)

    # Dianas humanas (+Y, más allá de la galería): maniquíes con salud, reacción física y ragdoll
    start_point("Dianas", 0, 4000)
    box("MuroDianas", 1050, 1100, 3500, 4500, 0, 300, MAT_PLASTER)
    for i, y in enumerate((3750, 4000, 4250)):
        d = actors.spawn_actor_from_class(unreal.BLTargetDummy, unreal.Vector(900, y, 93), unreal.Rotator(roll=0, pitch=0, yaw=180))
        d.set_actor_label(f"Diana{i + 1}")
        d.tags = [unreal.Name(f"BLDummy_{i + 1}")]
    cp = actors.spawn_actor_from_class(unreal.BLCheckpointVolume, unreal.Vector(0, 3300, 120))
    cp.set_actor_label("CheckpointDianas")
    cp.set_editor_property("checkpoint_id", "Dianas")
    start_point("Checkpoint", 0, 3300)

    # Decorado mínimo para dar escala visual en las capturas
    box("Cobertura1", 1500, 1600, 600, 900, 0, 100)
    box("Cobertura2", 2600, 2700, -700, -400, 0, 100)
    box("MuroFondo", 5000, 5100, -2000, 2000, 0, 400, MAT_PLASTER)

    # Iluminación
    # Amanecer frío y nublado (diseño aprobado): sol bajo y débil, cielo gris, niebla, colores apagados
    sun = actors.spawn_actor_from_class(unreal.DirectionalLight, unreal.Vector(0, 0, 1000),
                                        unreal.Rotator(roll=0, pitch=-16, yaw=-60))
    sc = sun.get_component_by_class(unreal.DirectionalLightComponent)
    sc.set_mobility(unreal.ComponentMobility.MOVABLE)
    sc.set_intensity(2.2)
    sc.set_light_color(unreal.LinearColor(1.0, 0.86, 0.72, 1))   # luz rasante algo cálida, filtrada por nubes
    sc.set_editor_property("atmosphere_sun_light", True)
    sky = actors.spawn_actor_from_class(unreal.SkyLight, unreal.Vector(0, 0, 800))
    skc = sky.get_component_by_class(unreal.SkyLightComponent)
    skc.set_mobility(unreal.ComponentMobility.MOVABLE)
    skc.set_editor_property("real_time_capture", True)
    skc.set_intensity(1.3)
    skc.set_light_color(unreal.LinearColor(0.8, 0.88, 1.0, 1))   # relleno frío de cielo cubierto
    actors.spawn_actor_from_class(unreal.SkyAtmosphere, unreal.Vector(0, 0, 0))
    fog = actors.spawn_actor_from_class(unreal.ExponentialHeightFog, unreal.Vector(0, 0, 0))
    fc = fog.get_component_by_class(unreal.ExponentialHeightFogComponent)
    fc.set_editor_property("fog_density", 0.045)
    fc.set_editor_property("fog_height_falloff", 0.12)
    fc.set_editor_property("start_distance", 800.0)
    fc.set_editor_property("fog_inscattering_luminance", unreal.LinearColor(0.42, 0.47, 0.53, 1))
    ppv = actors.spawn_actor_from_class(unreal.PostProcessVolume, unreal.Vector(0, 0, 0))
    ppv.set_editor_property("unbound", True)
    s = ppv.get_editor_property("settings")
    v4 = unreal.Vector4
    for name, value in (("auto_exposure_bias", -0.2),
                        ("color_saturation", v4(0.78, 0.78, 0.8, 1.0)),       # desaturado
                        ("color_contrast", v4(1.12, 1.12, 1.12, 1.0)),        # más contraste
                        ("color_gain_shadows", v4(0.92, 0.97, 1.05, 1.0)),    # sombras frías
                        ("color_gain_highlights", v4(1.04, 1.0, 0.95, 1.0)),  # luces algo cálidas
                        ("vignette_intensity", 0.45),
                        ("film_grain_intensity", 0.06),
                        ("bloom_intensity", 0.45)):
        s.set_editor_property("override_" + name, True)
        s.set_editor_property(name, value)
    ppv.set_editor_property("settings", s)

    level_ed.save_current_level()
    log("Mapa guardado: " + MAP_PATH)


try:
    build()
    log("OK")
except Exception as e:
    unreal.log_error(f"[BL_DevMap] ERROR: {e}")
