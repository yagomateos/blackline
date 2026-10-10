"""Assets de la misión 4 "Fuego cruzado" (puente del ferrocarril). Diseño propio, unidades de Unreal (cm).

Uso:  blender -b --factory-startup -P gen_m04.py -- <carpeta_salida>
Salida (carpeta Bridge salvo el arma, que va a MountedGun):
  SM_RailBridge_Span  tramo de celosía de acero de 3000 x 700: tablero con traviesas y vía, dos vigas de celosía
                      (montantes, diagonales, cordones) y arriostramiento superior. Pivote en el centro del tablero (z 0).
  SM_RailBridge_Pier  pila de piedra (400 x 900, 1100 de alto hasta el tablero); pivote arriba (z 0) en el centro
  SM_MG_Tripod        afuste: trípode con caja de munición (pivote en el suelo; muñón a 118)
  SM_MG_Gun           ametralladora pesada (pivote en el muñón, boca a +150 en X, asas de pala atrás)
  SM_Designator       designador láser en trípode con prismáticos y radio
  SM_Jet              caza de ataque (fuselaje, alas en flecha, derivas dobles, depósitos); morro hacia +X
  SM_Truck_Army       camión militar con caja de lona
  SM_SatDish          antena de satélite en trípode con su caja de equipo (el enlace de TORRE)
"""
import math
import os
import sys

sys.path.append(os.path.dirname(os.path.abspath(__file__)))
from mathutils import Matrix

import bl_lib
from bl_kit import Kit

STEEL = ("MI_Bridge_Steel", (0.18, 0.2, 0.19, 1), 0.6, 0.6)
RUST = ("MI_Env_RustyMetal", (0.3, 0.17, 0.1, 1), 0.8, 0.5)
WOOD = ("MI_Env_WoodPlanks", (0.3, 0.22, 0.15, 1), 0.85, 0.0)
STONE = ("MI_Old_Stone", (0.45, 0.42, 0.37, 1), 0.85, 0.0)
GUNMETAL = ("MI_MG_Gunmetal", (0.05, 0.05, 0.055, 1), 0.45, 0.8)
OLIVE = ("MI_Army_Olive", (0.13, 0.15, 0.1, 1), 0.7, 0.15)
CANVAS = ("MI_Army_Canvas", (0.2, 0.2, 0.14, 1), 0.95, 0.0)
GLASS = ("MI_Veh_GlassOpaque", (0.01, 0.015, 0.02, 1), 0.05, 0.0)
RUBBER = ("MI_Veh_Rubber", (0.02, 0.02, 0.02, 1), 0.9, 0.0)
JET_GREY = ("MI_Jet_Grey", (0.32, 0.34, 0.36, 1), 0.45, 0.4)
WHITE = ("MI_Boat_HullWhite", (0.6, 0.6, 0.57, 1), 0.6, 0.0)


def save(k, out):
    k.build()
    print(f"[m04] {k.name}: {k.export(os.path.join(out, k.name + '.fbx'))} tris")


def beam(k, name, a, b, w, mat):
    """Barra recta de sección cuadrada w entre dos puntos (cm)."""
    ax, ay, az = a
    bx, by, bz = b
    L = math.dist(a, b)
    obj = k.box(name, 0, L, -w / 2, w / 2, -w / 2, w / 2, mat, bevel=0.0)
    d = (bx - ax, by - ay, bz - az)
    yaw = math.atan2(d[1], d[0])
    pitch = math.atan2(d[2], math.hypot(d[0], d[1]))
    k.transform(obj, Matrix.Translation(a) @ Matrix.Rotation(yaw, 4, 'Z') @ Matrix.Rotation(-pitch, 4, 'Y'))
    return obj


def span(out):
    k = Kit("SM_RailBridge_Span", [STEEL, RUST, WOOD])
    L, W, H = 3000.0, 700.0, 600.0
    k.box("Deck", -L / 2, L / 2, -W / 2 + 40, W / 2 - 40, -60, -20, 0, bevel=0.0)
    k.box("Walk", -L / 2, L / 2, -W / 2 + 40, W / 2 - 40, -20, 0, 2, bevel=0.0)        # tablones de servicio
    for i in range(int(L / 60)):
        x = -L / 2 + 30 + i * 60
        k.box(f"Sleeper{i}", x - 12, x + 12, -160, 160, 0, 14, 2, bevel=0.0)
    for y in (-72, 72):
        k.box(f"Rail{y}", -L / 2, L / 2, y - 4, y + 4, 14, 30, 0, bevel=0.0)
    # Dos vigas de celosía (Warren con montantes) a los lados
    panels = 10
    for s in (-1, 1):
        y = s * (W / 2 - 20)
        beam(k, f"Bottom{s}", (-L / 2, y, -40), (L / 2, y, -40), 40, 0)
        beam(k, f"Top{s}", (-L / 2 + 150, y, H), (L / 2 - 150, y, H), 40, 0)
        beam(k, f"EndA{s}", (-L / 2, y, -40), (-L / 2 + 150, y, H), 40, 0)
        beam(k, f"EndB{s}", (L / 2, y, -40), (L / 2 - 150, y, H), 40, 0)
        for p in range(panels):
            x0 = -L / 2 + 150 + p * (L - 300) / panels
            x1 = x0 + (L - 300) / panels
            beam(k, f"Post{s}{p}", (x0, y, -40), (x0, y, H), 24, 0)
            beam(k, f"Diag{s}{p}", (x0, y, -40) if p % 2 == 0 else (x0, y, H), (x1, y, H) if p % 2 == 0 else (x1, y, -40), 20, 0)
        # Barandilla del paso de servicio
        k.box(f"Handrail{s}", -L / 2, L / 2, y - s * 25 - 3, y - s * 25 + 3, 95, 101, 1, bevel=0.0)
    # Arriostramiento superior entre las dos vigas
    for p in range(panels + 1):
        x = -L / 2 + 150 + p * (L - 300) / panels
        beam(k, f"Cross{p}", (x, -(W / 2 - 20), H), (x, W / 2 - 20, H), 20, 0)
    k.collision_box(-L / 2, L / 2, -W / 2, W / 2, -60, 0)
    for s in (-1, 1):
        # Solo hasta la barandilla: la celosía es calada; un muro de colisión hasta arriba tapaba la vista y las balas
        # a todo lo que hubiera en el puente desde cualquier ángulo (aliados, jugador en la trinchera)
        k.collision_box(-L / 2, L / 2, s * (W / 2 - 20) - 25, s * (W / 2 - 20) + 25, 0, 110)
    save(k, out)


def pier(out):
    k = Kit("SM_RailBridge_Pier", [STONE, RUST])
    k.box("Shaft", -200, 200, -450, 450, -1100, -60, 0, bevel=0.004)
    k.box("Cap", -230, 230, -480, 480, -100, -60, 0, bevel=0.006)
    for s in (-1, 1):
        k.prism(f"Cutwater{s}", [(-200, -1100), (200, -1100), (0, -1100)], 'z', -1100, -400, 0, bevel=0.0)
        cw = k.prism(f"Nose{s}", [(-200, 0), (200, 0), (0, s * 260)], 'z', -1100, -300, 0, bevel=0.004)
        k.transform(cw, Matrix.Translation((0, s * 450, 0)))
    k.collision_box(-230, 230, -480, 480, -1100, -60)
    save(k, out)


def mg_tripod(out):
    k = Kit("SM_MG_Tripod", [GUNMETAL, OLIVE])
    k.lathe("Pintle", [(0, 6), (110, 5), (118, 7)], (0, 0, 0), (0, 0, 1), seg=12, mat=0)
    for i, a in enumerate((0, 135, 225)):
        r = math.radians(a)
        beam(k, f"Leg{i}", (0, 0, 70), (95 * math.cos(r), 95 * math.sin(r), 0), 6, 0)
    k.box("AmmoCan", 30, 70, 20, 45, 60, 92, 1, bevel=0.004)
    k.collision_box(-40, 40, -40, 40, 0, 110)
    save(k, out)


def mg_gun(out):
    k = Kit("SM_MG_Gun", [GUNMETAL, OLIVE])
    k.box("Receiver", -70, 40, -10, 10, -9, 13, 0, bevel=0.006)
    k.box("TopCover", -60, 30, -9, 9, 13, 17, 0, bevel=0.004)
    k.lathe("Jacket", [(0, 7), (75, 7)], (40, 0, 3), (1, 0, 0), seg=14, mat=0)
    for i in range(10):
        x = 46 + i * 7
        k.lathe(f"Cool{i}", [(0, 7.6), (3, 7.6)], (x, 0, 3), (1, 0, 0), seg=14, mat=0)
    k.lathe("Barrel", [(0, 3.2), (32, 3.2), (34, 4.5), (36, 4.5)], (115, 0, 3), (1, 0, 0), seg=10, mat=0)
    for s in (-1, 1):
        beam(k, f"Spade{s}", (-70, s * 8, 0), (-92, s * 10, -2), 4, 0)
        k.lathe(f"Grip{s}", [(0, 2), (12, 2)], (-94, s * 10, -6), (0, 0, 1), seg=8, mat=1)
    k.box("Feed", -40, 0, -18, -10, -6, 6, 1, bevel=0.003)
    k.box("Sight", 20, 28, -2, 2, 13, 24, 0, bevel=0.0)
    k.box("Shield", 30, 34, -45, 45, -34, 24, 1, bevel=0.004)   # el borde queda por debajo de los ojos del tirador
    save(k, out)


def designator(out):
    k = Kit("SM_Designator", [OLIVE, GUNMETAL, GLASS])
    for i, a in enumerate((0, 120, 240)):
        r = math.radians(a)
        beam(k, f"Leg{i}", (0, 0, 120), (60 * math.cos(r), 60 * math.sin(r), 0), 4, 1)
    k.box("Laser", -25, 30, -9, 9, 120, 140, 0, bevel=0.006)
    k.lathe("Lens", [(0, 6), (4, 6)], (30, 0, 130), (1, 0, 0), seg=14, mat=2)
    k.box("Optic", -20, 15, -6, 6, 140, 150, 1, bevel=0.003)
    k.box("Radio", -40, -10, 20, 40, 0, 35, 0, bevel=0.004)
    k.collision_box(-30, 35, -30, 30, 0, 150)
    save(k, out)


def jet(out):
    k = Kit("SM_Jet", [JET_GREY, GLASS, GUNMETAL])
    k.lathe("Fuselage", [(-750, 40), (-650, 75), (-200, 85), (300, 80), (550, 55), (700, 20), (760, 0)], (0, 0, 0), (1, 0, 0), seg=20, mat=0)
    k.lathe("Canopy", [(250, 0), (300, 35), (430, 38), (520, 12)], (0, 0, 55), (1, 0, 0), seg=16, mat=1)
    k.lathe("Nozzle", [(0, 42), (60, 38)], (-750, 0, 0), (-1, 0, 0), seg=16, mat=2)
    for s in (-1, 1):
        k.prism(f"Wing{s}", [(250, s * 70), (-350, s * 560), (-470, s * 560), (-350, s * 70)], 'z', -8, 8, 0, bevel=0.002)
        k.prism(f"Stab{s}", [(-500, s * 60), (-700, s * 260), (-760, s * 260), (-700, s * 60)], 'z', -5, 5, 0, bevel=0.002)
        fin = k.prism(f"Fin{s}", [(-480, 40), (-700, 330), (-760, 330), (-740, 40)], 'y', -4, 4, 0, bevel=0.002)
        k.transform(fin, Matrix.Translation((0, s * 60, 30)) @ Matrix.Rotation(math.radians(s * 15), 4, 'X'))
        k.lathe(f"Tank{s}", [(0, 0), (40, 22), (300, 22), (340, 0)], (-300, s * 320, -40), (1, 0, 0), seg=12, mat=0)
    save(k, out)


def truck(out):
    k = Kit("SM_Truck_Army", [OLIVE, CANVAS, RUBBER, GLASS, GUNMETAL])
    k.box("Chassis", -380, 360, -110, 110, 60, 100, 4, bevel=0.004)
    k.box("Cab", 160, 360, -120, 120, 100, 290, 0, bevel=0.012)
    k.box("Hood", 280, 400, -100, 100, 100, 200, 0, bevel=0.01)
    k.box("Windshield", 268, 274, -100, 100, 205, 280, 3, bevel=0.0)
    k.box("Bed", -380, 150, -125, 125, 100, 160, 0, bevel=0.006)
    k.rounded_box("Canvas", -115, 0, 260, 530, 250, 200, 25, mat=1)
    for i, x in enumerate((-280, -150, 270)):
        for s in (-1, 1):
            k.lathe(f"Wheel{i}{s}", [(-20, 30), (-22, 50), (22, 50), (20, 30)], (x, s * 110, 52), (0, 1, 0), seg=18, closed=True, mat=2)
    k.collision_box(-380, 400, -125, 125, 0, 360)
    save(k, out)


def sat_dish(out):
    k = Kit("SM_SatDish", [WHITE, GUNMETAL, OLIVE])
    dish = k.lathe("Dish", [(0, 0), (10, 40), (25, 75), (40, 95), (42, 95)], (0, 0, 0), (1, 0, 0), seg=24, mat=0)
    k.transform(dish, Matrix.Translation((0, 0, 170)) @ Matrix.Rotation(math.radians(-35), 4, 'Y'))
    k.lathe("Feed", [(0, 2), (80, 2)], (0, 0, 170), (0.8, 0, 0.6), seg=6, mat=1)
    for i, a in enumerate((0, 120, 240)):
        r = math.radians(a)
        beam(k, f"Leg{i}", (0, 0, 160), (90 * math.cos(r), 90 * math.sin(r), 0), 5, 1)
    k.box("Case", -150, -60, -40, 40, 0, 60, 2, bevel=0.006)
    k.collision_box(-100, 100, -100, 100, 0, 250)
    save(k, out)


def main():
    argv = sys.argv[sys.argv.index("--") + 1:] if "--" in sys.argv else []
    base = os.path.abspath(argv[0] if argv else os.path.join(os.path.dirname(__file__), "..", "export"))
    bridge, mg = os.path.join(base, "Bridge"), os.path.join(base, "MountedGun")
    os.makedirs(bridge, exist_ok=True)
    os.makedirs(mg, exist_ok=True)
    for fn, out in ((span, bridge), (pier, bridge), (mg_tripod, mg), (mg_gun, mg), (designator, bridge), (jet, bridge),
                    (truck, bridge), (sat_dish, bridge)):
        bl_lib.reset_scene()
        fn(out)


if __name__ == "__main__":
    main()
