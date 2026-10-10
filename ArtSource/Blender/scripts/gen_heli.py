"""Helicóptero de transporte medio del Gobierno (Bloque 11, fase 9). Diseño propio.

Uso:  blender -b --factory-startup -P gen_heli.py -- <carpeta_salida>
Salida (pivote en el suelo bajo el mástil; morro hacia +X):
  SM_Heli_Body      fuselaje, cola, patines, puertas laterales abiertas y ametralladora de puerta (lado +Y)
  SM_Heli_Rotor     rotor principal de 4 palas (pivote en el buje, gira en Z), Ø 1500
  SM_Heli_TailRotor rotor de cola de 2 palas (pivote en el buje, gira en Y), Ø 260
  SM_Heli_RotorDisc / SM_Heli_TailRotorDisc  discos de desenfoque (translúcidos) que se ven a régimen de vuelo
Medidas: 1720 cm de largo con la cola, cabina 220 de ancho, mástil a 395 cm.
"""
import math
import os
import sys

sys.path.append(os.path.dirname(os.path.abspath(__file__)))
from mathutils import Matrix

import bl_lib
from bl_kit import Kit

M_PAINT = ("MI_Heli_Paint", (0.12, 0.13, 0.12, 1), 0.6, 0.25)
M_TRIM = ("MI_Veh_Trim", (0.05, 0.05, 0.05, 1), 0.6, 0.0)
M_GLASS = ("MI_Veh_GlassOpaque", (0.01, 0.015, 0.02, 1), 0.05, 0.0)
M_DARK = ("MI_Heli_Interior", (0.11, 0.12, 0.1, 1), 0.85, 0.0)

ZC = 170.0     # eje del fuselaje


def squash(k, obj, sy, sz, zc=ZC):
    """Aplana una pieza de revolución (sección elíptica) alrededor del eje del fuselaje."""
    m = Matrix.Translation((0, 0, zc)) @ Matrix.Diagonal((1.0, sy, sz, 1.0)) @ Matrix.Translation((0, 0, -zc))
    return k.transform(obj, m)


def body(out):
    k = Kit("SM_Heli_Body", [M_PAINT, M_TRIM, M_GLASS, M_DARK])
    prof = [(640, 8), (620, 45), (580, 80), (500, 108), (380, 118), (100, 120), (-150, 118), (-330, 100), (-470, 66), (-520, 50)]
    hull = squash(k, k.lathe("Fuselage", prof, (0, 0, ZC), (1, 0, 0), seg=28, mat=0), 0.92, 0.95)
    # Hueco de las puertas laterales (cabina abierta): se recorta y se pone un interior oscuro
    cut_l = k.box("DoorCut0", -120, 170, -140, -60, 70, 260, keep=False)
    cut_r = k.box("DoorCut1", -120, 170, 60, 140, 70, 260, keep=False)
    k.cut(hull, cut_l, cut_r)
    # Interior: mamparos delante y detrás, techo, y bancos espalda con espalda en el centro
    k.box("CabinFwd", 165, 180, -105, 105, 62, 275, 3, bevel=0.0)
    k.box("CabinAft", -135, -120, -105, 105, 62, 275, 3, bevel=0.0)
    k.box("CabinRoof", -135, 180, -105, 105, 262, 276, 3, bevel=0.0)
    k.box("SeatBack", -110, 160, -12, 12, 68, 180, 3, bevel=0.005)
    for s_ in (-1, 1):
        k.box(f"Seat{s_}", -110, 160, min(s_ * 12, s_ * 58), max(s_ * 12, s_ * 58), 105, 115, 1, bevel=0.004)
    k.box("CabinFloor", -125, 175, -104, 104, 58, 68, 1, bevel=0.0)
    # Cabina de pilotaje: cristal del morro (mitad superior del morro)
    glass = squash(k, k.lathe("Canopy", [(612, 30), (590, 72), (530, 101), (430, 116), (360, 119)], (0, 0, ZC), (1, 0, 0), seg=28, mat=2), 0.93, 0.96)
    k.cut(glass, k.box("CanopyCut", 300, 660, -200, 200, -100, ZC + 5, keep=False))
    # Cola
    k.lathe("Boom", [(-500, 52), (-800, 36), (-1060, 24), (-1080, 18)], (0, 0, ZC + 40), (1, 0, 0), seg=16, mat=0)
    k.prism("Fin", [(-1010, 205), (-1080, 205), (-1140, 430), (-1095, 440)], 'y', -6, 6, 0, bevel=0.01)
    k.box("Stab", -900, -840, -130, 130, 205, 213, 0, bevel=0.01)
    k.box("TailGearbox", -1105, -1075, 4, 20, 365, 395, 1, bevel=0.004)
    # Motores y mástil
    k.box("EngineDeck", -260, 260, -70, 70, 270, 330, 0, bevel=0.025)
    for y in (-42, 42):
        k.lathe(f"Intake{y}", [(0, 26), (20, 28), (230, 28), (260, 20)], (40, y, 335), (-1, 0, 0), seg=14, mat=0)
        k.lathe(f"Exhaust{y}", [(0, 20), (40, 18)], (-240, y * 1.2, 335), (-1, 0, 0), seg=12, mat=1)
    k.lathe("Mast", [(0, 22), (65, 14)], (0, 0, 330), (0, 0, 1), seg=12, mat=1)
    # Patines
    for s in (-1, 1):
        y = s * 135
        k.lathe(f"Skid{s}", [(-260, 6), (250, 6)], (0, y, 8), (1, 0, 0), seg=10, mat=1)
        k.lathe(f"SkidTip{s}", [(0, 6), (40, 5)], (250, y, 8), (1, 0, 0.6), seg=10, mat=1)
        for x in (-150, 180):
            k.prism(f"Strut{s}{x}", [(x - 8, 8), (x + 8, 8), (x + 8, 70), (x - 8, 70)], 'y', min(y, s * 80), max(y, s * 80), 1, bevel=0.0)
    # Ametralladora de puerta (lado derecho, +Y) con su brazo
    k.box("GunArm", 100, 116, 85, 150, 150, 160, 1, bevel=0.0)
    k.box("GunBody", 70, 150, 140, 160, 158, 176, 1, bevel=0.003)
    k.lathe("GunBarrel", [(0, 3), (90, 3), (95, 4)], (150, 150, 168), (1, 0, 0), seg=8, mat=1)
    k.box("GunBox", 80, 112, 160, 178, 140, 160, 1, bevel=0.003)
    # Luces, antenas, peldaños
    k.box("StepL", 0, 60, -128, -104, 28, 34, 1, bevel=0.0)
    k.box("StepR", 0, 60, 104, 128, 28, 34, 1, bevel=0.0)
    k.lathe("Antenna", [(0, 1.2), (90, 0.6)], (-300, 0, 300), (0, 0, 1), seg=6, mat=1)
    # Puertas correderas abiertas: desplazadas hacia atrás por fuera del fuselaje, con su ventanilla
    for s_ in (-1, 1):
        yo = s_ * 121
        k.box(f"DoorPanel{s_}", -400, -110, min(yo, yo + s_ * 5), max(yo, yo + s_ * 5), 72, 258, 0, bevel=0.004)
        k.box(f"DoorWindow{s_}", -330, -200, min(yo + s_ * 5, yo + s_ * 6), max(yo + s_ * 5, yo + s_ * 6), 160, 235, 2, bevel=0.0)
        k.box(f"DoorRail{s_}", -420, 175, min(yo, yo + s_ * 4), max(yo, yo + s_ * 4), 258, 264, 1, bevel=0.0)
    # Colisión en carcasa: morro, parte de atrás, suelo, techo y respaldo central; los huecos de las puertas quedan
    # libres para entrar (antes una caja maciza tapaba la cabina entera y era imposible subir)
    k.collision_box(170, 600, -112, 112, 55, 300)          # cabina de pilotaje
    k.collision_box(-330, -120, -112, 112, 55, 300)        # parte de atrás de la cabina
    k.collision_box(-120, 170, -104, 104, 40, 68)          # suelo
    k.collision_box(-120, 170, -112, 112, 262, 300)        # techo
    k.collision_box(-110, 160, -12, 12, 68, 180)           # respaldo de los bancos
    k.collision_box(-1080, -330, -40, 40, 150, 260)        # cola
    k.collision_box(-260, 260, -60, 60, 0, 40)             # bajo el suelo (entre los patines)
    for s_ in (-1, 1):
        k.collision_box(0, 60, min(s_ * 104, s_ * 128), max(s_ * 104, s_ * 128), 0, 34)   # peldaño (sube a 34 y luego al suelo)
    k.build()
    print(f"[heli] SM_Heli_Body: {k.export(os.path.join(out, 'SM_Heli_Body.fbx'))} tris")


def airfoil(chord, thick=0.12, n=10):
    """Perfil simétrico tipo NACA 00xx (x de 0 a 1 a lo largo de la cuerda) en (x, z) centrado al 25 %."""
    pts_up, pts_lo = [], []
    for i in range(n + 1):
        x = (1 - math.cos(math.pi * i / n)) / 2        # más puntos en el borde de ataque
        t = 5 * thick * (0.2969 * math.sqrt(x) - 0.126 * x - 0.3516 * x * x + 0.2843 * x ** 3 - 0.1036 * x ** 4)
        pts_up.append(((x - 0.25) * chord, t * chord))
        pts_lo.append(((x - 0.25) * chord, -t * chord))
    return pts_up + list(reversed(pts_lo[1:-1]))


def blade(k, name, r0, r1, chord0, chord1, twist0, twist1, sweep_tip=0.0, mat=0, axis='z', thick=0.12, droop=0.0):
    """Pala con perfil aerodinámico, estrechamiento, torsión y punta en flecha (antes: un tablón plano de 6 cm).
    axis 'z': gira alrededor de Z, envergadura en +X, cuerda en Y (borde de ataque hacia -Y, sentido de giro antihorario
    visto desde arriba). axis 'y': rotor de cola, gira alrededor de Y, envergadura en +X, cuerda en Z."""
    import bmesh
    from bl_kit import W
    stations = []
    n_st = 9
    for j in range(n_st):
        u = j / (n_st - 1)
        r = r0 + (r1 - r0) * u
        # Estrecha en el último 15 % (punta) y barre hacia atrás
        tip = max(0.0, (u - 0.85) / 0.15)
        chord = (chord0 + (chord1 - chord0) * u) * (1 - 0.45 * tip * tip)
        off = sweep_tip * tip * tip
        tw = math.radians(twist0 + (twist1 - twist0) * u)
        stations.append((r, chord, off, tw, droop * u * u))
    bm = bmesh.new()
    rings = []
    for r, chord, off, tw, dz in stations:
        ring = []
        for cx, cz in airfoil(chord, thick):
            # Torsión alrededor del eje de paso (25 % de la cuerda)
            yy = cx * math.cos(tw) - cz * math.sin(tw) + off
            zz = cx * math.sin(tw) + cz * math.cos(tw)
            if axis == 'z':
                ring.append(bm.verts.new(W(r, yy, zz - dz)))
            else:
                ring.append(bm.verts.new(W(r, zz, yy)))
        rings.append(ring)
    m = len(rings[0])
    for a, b in zip(rings, rings[1:]):
        for i in range(m):
            jx = (i + 1) % m
            bm.faces.new((a[i], a[jx], b[jx], b[i]))
    bm.faces.new(rings[0])
    bm.faces.new(list(reversed(rings[-1])))
    return k._finish(bm, name, mat, 0.0, 1, True)


def rotor(out):
    # Rotor principal de 4 palas, Ø 1500: cabeza articulada (horquillas, bielas de paso, plato oscilante) y palas con
    # perfil, torsión de -8° y punta en flecha; caída estática de 8 cm en la punta (palas largas y flexibles)
    k = Kit("SM_Heli_Rotor", [M_TRIM, ("MI_Heli_RotorMetal", (0.18, 0.18, 0.18, 1), 0.35, 0.9), ("MI_Heli_BladeTip", (0.55, 0.5, 0.12, 1), 0.5, 0.0)])
    k.lathe("Mast", [(-60, 14), (-5, 14), (0, 20)], (0, 0, 0), (0, 0, 1), seg=20, mat=1)
    k.lathe("Swashplate", [(-48, 34), (-40, 36), (-34, 30)], (0, 0, 0), (0, 0, 1), seg=24, mat=1)
    k.lathe("Hub", [(-12, 34), (8, 36), (20, 24), (30, 10), (34, 0)], (0, 0, 0), (0, 0, 1), seg=24, mat=0)
    for i in range(4):
        rot = Matrix.Rotation(math.radians(90 * i), 4, 'Z')
        # Horquilla (grip) y manguito de la pala
        g = k.box(f"Grip{i}", 22, 78, -14, 14, -9, 9, 1, bevel=0.004)
        k.transform(g, rot)
        cuff = k.lathe(f"Cuff{i}", [(0, 10), (30, 9), (40, 6)], (75, 0, 0), (1, 0, 0), seg=12, mat=0)
        k.transform(cuff, rot)
        # Biela de paso del plato a la horquilla
        link = k.lathe(f"PitchLink{i}", [(0, 2.2), (40, 2.2)], (48, -18, -40), (0, 0, 1), seg=8, mat=1)
        k.transform(link, rot)
        b = blade(k, f"Blade{i}", 100, 750, 54, 48, 6, -2, sweep_tip=10, mat=0, droop=8)
        k.transform(b, rot)
        tipcap = blade(k, f"Tip{i}", 715, 750, 50, 48, -1.5, -2, sweep_tip=10, mat=2, thick=0.125, droop=8)
        k.transform(tipcap, rot)
    k.build()
    print(f"[heli] SM_Heli_Rotor: {k.export(os.path.join(out, 'SM_Heli_Rotor.fbx'))} tris")


def tail_rotor(out):
    k = Kit("SM_Heli_TailRotor", [M_TRIM, ("MI_Heli_RotorMetal", (0.18, 0.18, 0.18, 1), 0.35, 0.9), ("MI_Heli_BladeTip", (0.55, 0.5, 0.12, 1), 0.5, 0.0)])
    k.lathe("Hub", [(-12, 12), (6, 13), (14, 6)], (0, 0, 0), (0, 1, 0), seg=16, mat=1)
    for i in range(2):
        rot = Matrix.Rotation(math.radians(180 * i), 4, 'Y')
        b = blade(k, f"Blade{i}", 14, 130, 22, 18, 10, 4, sweep_tip=3, mat=0, axis='y')
        k.transform(b, rot)
        t = blade(k, f"Tip{i}", 118, 130, 19, 18, 4.5, 4, sweep_tip=3, mat=2, axis='y', thick=0.125)
        k.transform(t, rot)
    k.build()
    print(f"[heli] SM_Heli_TailRotor: {k.export(os.path.join(out, 'SM_Heli_TailRotor.fbx'))} tris")


def disc(out, name, r0, r1, axis, seg=72, rings=8, cone=0.0):
    """Disco de desenfoque del rotor (lo que ve una cámara a régimen de vuelo): anillo plano con UV radial
    (U = radio de 0 a 1, V = ángulo de 0 a 1). Lo pinta M_Rotor_Blur (translúcido)."""
    import bmesh
    from bl_kit import W
    k = Kit(name, [("MI_Rotor_Blur", (0.05, 0.05, 0.05, 1), 0.6, 0.0)])
    bm = bmesh.new()
    uv = bm.loops.layers.uv.new("UVMap")
    grid = []
    for j in range(rings + 1):
        r = r0 + (r1 - r0) * j / rings
        row = []
        for i in range(seg + 1):
            a = 2 * math.pi * i / seg
            x, y = r * math.cos(a), r * math.sin(a)
            # Conicidad de las palas en vuelo (~3° hacia arriba): de perfil se ve la elipse fina del rotor
            cz = (r - r0) * cone
            row.append((bm.verts.new(W(x, y, cz) if axis == 'z' else W(x, cz, y)), j / rings, i / seg))
        grid.append(row)
    for j in range(rings):
        for i in range(seg):
            q = (grid[j][i], grid[j][i + 1], grid[j + 1][i + 1], grid[j + 1][i])
            f = bm.faces.new([v for v, _, _ in q])
            for loop, (_, u, v) in zip(f.loops, q):
                loop[uv].uv = (u, v)
    bm.verts.index_update()
    for f in bm.faces:
        f.material_index = 0
    obj = bl_lib.bm_to_object(bm, name, k.mats)
    k.mesh = obj          # sin build(): conserva el UV radial (build hace proyección cúbica)
    path = os.path.join(out, name + ".fbx")
    print(f"[heli] {name}: {k.export(path)} tris")


def main():
    argv = sys.argv[sys.argv.index("--") + 1:] if "--" in sys.argv else []
    out = os.path.abspath(argv[0] if argv else os.path.join(os.path.dirname(__file__), "..", "export", "Vehicles"))
    os.makedirs(out, exist_ok=True)
    bl_lib.reset_scene()
    body(out)
    bl_lib.reset_scene()
    rotor(out)
    bl_lib.reset_scene()
    tail_rotor(out)
    bl_lib.reset_scene()
    disc(out, "SM_Heli_RotorDisc", 60, 760, 'z', cone=0.05)
    bl_lib.reset_scene()
    disc(out, "SM_Heli_TailRotorDisc", 14, 132, 'y', seg=40, rings=4)


if __name__ == "__main__":
    main()
