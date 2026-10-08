"""Construye /Game/Maps/Benchmark/L_Benchmark: una calle urbana densa para medir rendimiento.

Ejecutar desde el editor:
  UnrealEditor.exe Blackline.uproject -ExecutePythonScript="Tools/UnrealPython/build_benchmark_map.py"

Contenido representativo del peor caso de la misión 1:
- ~100 bloques de viviendas Nanite (54k tris c/u) en ambos lados de una avenida y calles traseras
- ~200 props no-Nanite (coches/barriles/barreras con formas básicas)
- Sol bajo + SkyLight en tiempo real + atmósfera + niebla
- 12 luces de farola (3 con sombra)
"""
import os
import random
import unreal

PROJECT_DIR = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())
FBX = os.path.join(PROJECT_DIR, "ArtSource", "Blender", "export", "SM_BM_Apartment_A.fbx")
MESH_DIR = "/Game/Benchmark/Meshes"
MAP_PATH = "/Game/Maps/Benchmark/L_Benchmark"

asset_lib = unreal.EditorAssetLibrary
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
level_ed = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
rng = random.Random(7)


def log(msg):
    unreal.log(f"[BL_Benchmark] {msg}")


def import_building():
    task = unreal.AssetImportTask()
    task.filename = FBX
    task.destination_path = MESH_DIR
    task.automated = True
    task.replace_existing = True
    task.save = True
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
    paths = list(task.imported_object_paths)
    log(f"Importados: {paths}")
    mesh = None
    for p in paths:
        a = unreal.load_asset(p)
        if isinstance(a, unreal.StaticMesh):
            mesh = a
    if mesh is None:
        raise RuntimeError("No se importó ningún StaticMesh")
    ns = mesh.get_editor_property("nanite_settings")
    ns.set_editor_property("enabled", True)
    mesh.set_editor_property("nanite_settings", ns)
    asset_lib.save_loaded_asset(mesh)
    bounds = mesh.get_bounding_box()
    log(f"Mesh {mesh.get_path_name()} bounds={bounds.max - bounds.min} cm")
    return mesh


def spawn_mesh(mesh, loc, rot=(0, 0, 0), scale=(1, 1, 1), mobility=unreal.ComponentMobility.STATIC):
    a = actors.spawn_actor_from_class(unreal.StaticMeshActor, unreal.Vector(*loc),
                                      unreal.Rotator(roll=rot[0], pitch=rot[1], yaw=rot[2]))
    comp = a.static_mesh_component
    comp.set_mobility(mobility)
    comp.set_static_mesh(mesh)
    a.set_actor_scale3d(unreal.Vector(*scale))
    return a


def build():
    building = import_building()
    cube = unreal.load_asset("/Engine/BasicShapes/Cube.Cube")
    cyl = unreal.load_asset("/Engine/BasicShapes/Cylinder.Cylinder")

    level_ed.new_level(MAP_PATH)

    # Suelo (200 x 200 m)
    spawn_mesh(cube, (0, 0, -10), scale=(200, 200, 0.2))

    # Edificios: avenida principal en y=0 (14 m de ancho) + calles traseras
    step = 1350.0
    rows_y = [-1340, 1340, -2900, 2900, -4300, 4300]
    count = 0
    for y in rows_y:
        for i in range(-8, 9):
            x = i * step + rng.uniform(-40, 40)
            zs = rng.choice([1.0, 1.0, 1.25, 1.5])
            spawn_mesh(building, (x, y, 0), rot=(0, 0, rng.choice([0, 90, 180, 270])), scale=(1, 1, zs))
            count += 1
    log(f"Edificios: {count}")

    # Props: coches (cajas), barriles, barreras
    for i in range(90):
        side = rng.choice([-1, 1])
        x = rng.uniform(-11000, 11000)
        spawn_mesh(cube, (x, side * 450, 75), rot=(0, 0, rng.uniform(-8, 8)), scale=(4.2, 1.8, 1.5))
    for i in range(60):
        spawn_mesh(cyl, (rng.uniform(-11000, 11000), rng.uniform(-600, 600), 45), scale=(0.6, 0.6, 0.9))
    for i in range(50):
        spawn_mesh(cube, (rng.uniform(-11000, 11000), rng.uniform(-200, 200), 40),
                   rot=(0, 0, rng.uniform(0, 180)), scale=(2.0, 0.6, 0.8))

    # Iluminación de amanecer
    sun = actors.spawn_actor_from_class(unreal.DirectionalLight, unreal.Vector(0, 0, 2000),
                                        unreal.Rotator(roll=0, pitch=-12, yaw=-35))
    sc = sun.get_component_by_class(unreal.DirectionalLightComponent)
    sc.set_mobility(unreal.ComponentMobility.MOVABLE)
    sc.set_intensity(6.0)
    sc.set_light_color(unreal.LinearColor(1.0, 0.82, 0.65, 1.0))
    sc.set_editor_property("atmosphere_sun_light", True)

    sky = actors.spawn_actor_from_class(unreal.SkyLight, unreal.Vector(0, 0, 1500))
    skc = sky.get_component_by_class(unreal.SkyLightComponent)
    skc.set_mobility(unreal.ComponentMobility.MOVABLE)
    skc.set_editor_property("real_time_capture", True)

    actors.spawn_actor_from_class(unreal.SkyAtmosphere, unreal.Vector(0, 0, 0))
    fog = actors.spawn_actor_from_class(unreal.ExponentialHeightFog, unreal.Vector(0, 0, 0))
    fc = fog.get_component_by_class(unreal.ExponentialHeightFogComponent)
    fc.set_editor_property("fog_density", 0.03)

    ppv = actors.spawn_actor_from_class(unreal.PostProcessVolume, unreal.Vector(0, 0, 0))
    ppv.set_editor_property("unbound", True)

    # Farolas de sodio
    for i in range(12):
        x = -9000 + i * 1600
        pl = actors.spawn_actor_from_class(unreal.PointLight, unreal.Vector(x, 600 * (1 if i % 2 else -1), 600))
        pc = pl.get_component_by_class(unreal.PointLightComponent)
        pc.set_mobility(unreal.ComponentMobility.MOVABLE)
        pc.set_editor_property("intensity_units", unreal.LightUnits.CANDELAS)
        pc.set_intensity(120.0)
        pc.set_attenuation_radius(1500.0)
        pc.set_light_color(unreal.LinearColor(1.0, 0.6, 0.25, 1.0))
        pc.set_cast_shadows(i in (3, 6, 9))

    # Punto de vista: a pie de calle mirando a lo largo de la avenida
    actors.spawn_actor_from_class(unreal.PlayerStart, unreal.Vector(-9500, 0, 120),
                                  unreal.Rotator(roll=0, pitch=0, yaw=0))

    level_ed.save_current_level()
    log("Mapa guardado: " + MAP_PATH)


try:
    build()
    log("OK")
except Exception as e:
    unreal.log_error(f"[BL_Benchmark] ERROR: {e}")
finally:
    unreal.SystemLibrary.quit_editor()
