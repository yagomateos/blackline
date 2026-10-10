"""Busca z-fighting (parpadeo) en los mapas: pares de mallas visibles cuyas caras superiores (suelo) están a la
misma altura (< 0,6 cm) y se solapan en planta. Escribe Saved/zfighting_<mapa>.txt.
Con BL_ZFIX=1 alarga 1,5 cm hacia arriba la más pequeña de cada par (la base no se mueve) y guarda el mapa.
Hay que pasarlo tras regenerar un mapa con su build_mXX_*.py.
Uso: UnrealEditor-Cmd.exe Blackline.uproject -run=pythonscript -script=".../find_zfighting.py"
"""
import os
import unreal

MAPS = ["/Game/Maps/M01/L_M01_AmanecerRoto", "/Game/Maps/M02/L_M02_Manifiesto", "/Game/Maps/M03/L_M03_Ria",
        "/Game/Maps/M04/L_M04_FuegoCruzado", "/Game/Maps/M05/L_M05_LineaNegra"]
FIX = os.environ.get("BL_ZFIX") == "1"
PROJECT = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())
les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)


def faces(actor):
    """(x0, x1, y0, y1, ztop, zbot, comp) de cada malla estática visible del actor, alineada a ejes."""
    out = []
    for c in actor.get_components_by_class(unreal.StaticMeshComponent):
        if not c.is_visible() or actor.is_hidden_ed() or c.get_editor_property("hidden_in_game") or actor.get_editor_property("hidden"):
            continue
        if isinstance(c, unreal.InstancedStaticMeshComponent):
            continue
        rot = c.get_world_rotation()
        if abs(rot.pitch) > 0.5 or abs(rot.roll) > 0.5 or (abs(rot.yaw) % 90) > 0.5 and (abs(rot.yaw) % 90) < 89.5:
            continue
        o, e, _r = unreal.SystemLibrary.get_component_bounds(c)
        out.append((o.x - e.x, o.x + e.x, o.y - e.y, o.y + e.y, o.z + e.z, o.z - e.z, c))
    return out


for path in MAPS:
    les.load_level(path)
    items = []
    for a in eas.get_all_level_actors():
        if isinstance(a, unreal.StaticMeshActor):
            for f in faces(a):
                items.append((a, f))
    items.sort(key=lambda t: t[1][4])
    pairs = []
    for i in range(len(items)):
        a, f = items[i]
        j = i + 1
        while j < len(items) and items[j][1][4] - f[4] < 0.6:
            b, g = items[j]
            if b != a:
                ox = min(f[1], g[1]) - max(f[0], g[0])
                oy = min(f[3], g[3]) - max(f[2], g[2])
                if ox > 5 and oy > 5 and ox * oy > 500:
                    pairs.append((a, f, b, g, ox * oy))
            j += 1
    name = path.split("/")[-1]
    lines = [f"{name}: {len(pairs)} pares coplanares solapados"]
    fixed = set()
    for a, f, b, g, area in sorted(pairs, key=lambda p: -p[4]):
        lines.append(f"  z={f[4]:.1f}  {a.get_actor_label()} / {b.get_actor_label()}  solape {area / 10000:.1f} m2")
        if FIX:
            # Sube la pieza más pequeña (encima: alfombra sobre el suelo) 1,5 cm
            sa = (f[1] - f[0]) * (f[3] - f[2])
            sb = (g[1] - g[0]) * (g[3] - g[2])
            small = a if sa < sb else b
            if small.get_actor_label() not in fixed:
                # Más alta 1,5 cm por arriba con la base donde estaba (no se despega del suelo)
                h = (f[4] - f[5]) if small == a else (g[4] - g[5])
                sc = small.get_actor_scale3d()
                if h > 0.5:
                    small.set_actor_scale3d(unreal.Vector(sc.x, sc.y, sc.z * (h + 1.5) / h))
                    loc = small.get_actor_location()
                    small.set_actor_location(unreal.Vector(loc.x, loc.y, loc.z + 0.75), False, False)
                    fixed.add(small.get_actor_label())
    if FIX and fixed:
        les.save_current_level()
        lines.append(f"  subidas {len(fixed)} piezas 1,5 cm")
    with open(os.path.join(PROJECT, "Saved", f"zfighting_{name}.txt"), "w", encoding="utf-8") as fh:
        fh.write("\n".join(lines))
    unreal.log_warning("[BL_Z] " + lines[0] + (f" (subidas {len(fixed)})" if FIX else ""))
unreal.log_warning("[BL_Z] OK")
