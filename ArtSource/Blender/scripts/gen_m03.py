"""Assets de la misión 3 "Ría" (casco viejo y muelles al amanecer). Diseño propio, unidades de Unreal (cm).

Uso:  blender -b --factory-startup -P gen_m03.py -- <carpeta_salida>
Salida (carpeta OldTown salvo el equipo):
  SM_Corvane_Helmet  casco balístico de corte alto con raíles, soporte de visión nocturna y visor (espacio de la malla
                     del Mannequin, como SM_Militia_Helmet: cabeza en (0, 1.5, 168))
  SM_Corvane_Vest    portaplacas de placas gruesas, negro (espacio de la malla, pecho en z 141)
  SM_Door_Wood       hoja de puerta de 100 x 210 con cuarterones y pomo; pivote en la bisagra, la hoja va hacia +Y
  SM_Door_Frame      marco para un hueco de 110 x 225 (mismo pivote que la hoja)
  SM_BreachCharge    carga de brecha en tira con detonador (sobresale hacia +X)
  SM_BellTower       campanario de piedra 600 x 600 x 2900 con campanario abierto (arcos), campanas y tejado
  SM_StoneBridge     tramo de puente de piedra de tres arcos (2400 x 700, tablero a z 0)
  SM_Arcade          tramo de soportal de 400 (dos pilares, arco, techo) que se adosa a una fachada (fondo hacia +Y)
  SM_FishingBoat     barca de pesca de 900 (casco, puente, mástil, redes, neumáticos)
  SM_FishCrates      pila de cajas de pescado de plástico
  SM_Ship_Cargo      el "barco sin bandera": mercante de 90 m, casco gris sin nombre, superestructura a popa, dos grúas
"""
import math
import os
import sys

sys.path.append(os.path.dirname(os.path.abspath(__file__)))
from mathutils import Matrix

import bl_lib
from bl_kit import Kit

BLACK = ("MI_Corvane_Gear", (0.03, 0.03, 0.032, 1), 0.7, 0.05)
WEB = ("MI_Gear_Webbing", (0.12, 0.12, 0.1, 1), 0.9, 0.0)
GLASS = ("MI_Veh_GlassOpaque", (0.01, 0.015, 0.02, 1), 0.05, 0.0)
PLASTIC = ("MI_Obj_Plastic", (0.03, 0.03, 0.03, 1), 0.5, 0.0)
WOOD = ("MI_Env_WoodPlanks", (0.35, 0.25, 0.16, 1), 0.8, 0.0)
PAINT_DOOR = ("MI_Old_DoorPaint", (0.12, 0.2, 0.18, 1), 0.7, 0.0)
METAL = ("MI_Env_MetalPlate", (0.3, 0.3, 0.3, 1), 0.5, 0.8)
STONE = ("MI_Old_Stone", (0.45, 0.42, 0.37, 1), 0.85, 0.0)
TILE = ("MI_Old_RoofTile", (0.38, 0.17, 0.1, 1), 0.75, 0.0)
BRONZE = ("MI_Old_Bronze", (0.3, 0.22, 0.12, 1), 0.4, 0.9)
HULL_RED = ("MI_Boat_HullRed", (0.32, 0.06, 0.04, 1), 0.6, 0.0)
HULL_WHITE = ("MI_Boat_HullWhite", (0.6, 0.6, 0.57, 1), 0.6, 0.0)
NET = ("MI_Boat_Net", (0.1, 0.18, 0.14, 1), 0.95, 0.0)
RUBBER = ("MI_Veh_Rubber", (0.02, 0.02, 0.02, 1), 0.9, 0.0)
CRATE_BLUE = ("MI_Old_CrateBlue", (0.05, 0.15, 0.32, 1), 0.55, 0.0)
SHIP_GREY = ("MI_Ship_Grey", (0.2, 0.21, 0.22, 1), 0.55, 0.3)
SHIP_RUST = ("MI_Env_RustyMetal", (0.3, 0.17, 0.1, 1), 0.8, 0.5)
SHIP_DECK = ("MI_Ship_Deck", (0.14, 0.12, 0.1, 1), 0.8, 0.1)


def save(k, out):
    k.build()
    print(f"[m03] {k.name}: {k.export(os.path.join(out, k.name + '.fbx'))} tris")


# ---------------------------------------------------------------------------
# Equipo de Corvane (espacio de la malla del Mannequin)
# ---------------------------------------------------------------------------
def corvane_helmet(out):
    k = Kit("SM_Corvane_Helmet", [BLACK, WEB, GLASS, PLASTIC])
    # Corte alto: la cúpula no baja por las orejas
    prof = [(0.0, 13.6), (1.5, 13.4), (4.5, 12.6), (7.5, 11.0), (10.0, 8.6), (12.3, 5.4), (13.6, 2.4), (14.0, 0.4)]
    shell = k.lathe("Shell", prof, (0, 1.5, 167.5), (0, 0, 1), seg=28, mat=0)
    k.transform(shell, Matrix.Translation((0, 1.5, 167.5)) @ Matrix.Rotation(math.radians(-10), 4, 'X') @ Matrix.Translation((0, -1.5, -167.5)))
    for s in (-1, 1):
        k.box(f"Rail{s}", s * 12.6 - 0.9, s * 12.6 + 0.9, -4, 8, 169, 172.5, 3, bevel=0.002)
        k.box(f"Pad{s}", s * 11.8 - 1.2, s * 11.8 + 1.2, -1, 5, 160, 167, 1, bevel=0.003)   # cascos de oído
    k.box("Shroud", -2.5, 2.5, 12.2, 14.4, 175, 180, 3, bevel=0.003)
    k.box("NVGMount", -1.8, 1.8, 13.8, 17.0, 172, 176, 3, bevel=0.003)
    # Visor oscuro que tapa los ojos (balístico, abatible)
    visor = k.prism("Visor", [(-10.5, 159.5), (10.5, 159.5), (9.5, 168.5), (-9.5, 168.5)], 'y', 13.0, 14.2, 2, bevel=0.004)
    k.transform(visor, Matrix.Translation((0, 13.6, 164)) @ Matrix.Rotation(math.radians(8), 4, 'X') @ Matrix.Translation((0, -13.6, -164)))
    save(k, out)


def corvane_vest(out):
    k = Kit("SM_Corvane_Vest", [BLACK, WEB, PLASTIC])
    for name, y0, y1 in (("Front", 11.5, 17.5), ("Back", -18.0, -12.5)):
        k.rounded_box(f"{name}Plate", 0, (y0 + y1) / 2, 128, 26, y1 - y0, 34, radius=2.0, mat=0)
    for s in (-1, 1):
        k.rounded_box(f"Cummer{s}", s * 17.8, -0.5, 119, 4.0, 27, 16, radius=1.4, mat=1)
        k.prism(f"Strap{s}", [(12.5, 145), (13.5, 149.5), (2, 152.5), (-6, 152.5), (-12.5, 149), (-12.5, 145)], 'x', s * 9 - 2.4, s * 9 + 2.4, 1, bevel=0.006)
    for i, x in enumerate((-8.0, 0.0, 8.0)):
        k.rounded_box(f"Mag{i}", x, 19.8, 119, 7.2, 4.5, 14, radius=1.0, mat=1)
    k.rounded_box("Radio", -18.5, 4, 131, 4.5, 7, 14, radius=0.8, mat=2)
    k.lathe("Antenna", [(0, 0.45), (30, 0.3)], (-19, 2, 139), (-0.05, -0.15, 1), seg=6, mat=2)
    k.rounded_box("Tourniquet", 9, 19.6, 138, 9, 3, 4, radius=1.0, mat=2)
    save(k, out)


# ---------------------------------------------------------------------------
# CQB: puerta, marco y carga
# ---------------------------------------------------------------------------
def door(out):
    k = Kit("SM_Door_Wood", [PAINT_DOOR, METAL, WOOD])
    k.box("Leaf", -3, 3, 0, 100, 0, 210, 0, bevel=0.006)
    for s in (-1, 1):
        x = s * 3.4
        for i, (z0, z1) in enumerate(((20, 95), (110, 190))):
            k.box(f"Panel{s}{i}", x - 0.5, x + 0.5, 14, 86, z0, z1, 0, bevel=0.004)
        k.lathe(f"Knob{s}", [(0, 3), (4, 3.2), (6, 2.2)], (s * 3, 88, 100), (s, 0, 0), seg=12, mat=1)
        k.box(f"Plate{s}", s * 3 - 0.3, s * 3 + 0.3, 85, 91, 92, 112, 1, bevel=0.0)
    for z in (25, 185):
        k.box(f"Hinge{z}", -3.6, 3.6, -1.5, 3, z, z + 12, 1, bevel=0.0)
    k.collision_box(-3, 3, 0, 100, 0, 210)
    save(k, out)


def door_frame(out):
    k = Kit("SM_Door_Frame", [WOOD])
    for y0, y1 in ((-10, 0), (100, 110)):
        k.box(f"Jamb{y0}", -9, 9, y0, y1, 0, 225, 0, bevel=0.004)
    k.box("Head", -9, 9, -10, 110, 212, 225, 0, bevel=0.004)
    k.box("Sill", -10, 10, -10, 110, 0, 2, 0, bevel=0.0)
    for y0, y1 in ((-10, 0), (100, 110)):
        k.collision_box(-9, 9, y0, y1, 0, 225)
    k.collision_box(-9, 9, -10, 110, 212, 225)
    save(k, out)


def breach_charge(out):
    k = Kit("SM_BreachCharge", [PLASTIC, METAL])
    k.box("Strip", 0, 2.5, -22, 22, -3.5, 3.5, 0, bevel=0.003)
    k.box("Det", 0, 6, -5, 5, -6, 6, 0, bevel=0.004)
    k.box("Led", 6, 6.6, -1, 1, 3, 5, 1, bevel=0.0)
    k.lathe("Wire", [(0, 0.4), (30, 0.4)], (3, 4, 0), (0, 0.3, -1), seg=6, mat=0)
    save(k, out)


# ---------------------------------------------------------------------------
# Casco viejo
# ---------------------------------------------------------------------------
def arch_profile(x0, x1, spring, rise, seg=10):
    """Puntos de un arco de medio punto (de x1 a x0 por arriba), para restar del perfil de un muro."""
    cx, r = (x0 + x1) / 2, (x1 - x0) / 2
    return [(cx + r * math.cos(math.pi * i / seg), spring + rise / r * r * math.sin(math.pi * i / seg)) for i in range(seg + 1)]


def bell_tower(out):
    k = Kit("SM_BellTower", [STONE, TILE, BRONZE, WOOD])
    S, H, B0, B1 = 300.0, 2900.0, 1950.0, 2450.0
    k.box("Shaft", -S, S, -S, S, 0, B0, 0, bevel=0.004)
    for z in (700, 1400):
        k.box(f"Cornice{z}", -S - 12, S + 12, -S - 12, S + 12, z, z + 25, 0, bevel=0.006)
    def corner(sv):
        return (S - 30, S + 30) if sv > 0 else (-S - 30, -S + 30)
    for sx in (-1, 1):
        for sy in (-1, 1):
            (x0, x1), (y0, y1) = corner(sx), corner(sy)
            k.box(f"Pilaster{sx}{sy}", x0, x1, y0, y1, 0, B1 + 30, 0, bevel=0.004)
    # Campanario: un arco por cara (los muros dejan el hueco del arco)
    for face in range(4):
        wall = k.prism(f"Belfry{face}", [(-S, B0)] + [(-130, B0)] + [(-130, B0 + 330)] + arch_profile(-130, 130, B0 + 330, 130)[::-1][1:-1]
                       + [(130, B0 + 330), (130, B0), (S, B0), (S, B1), (-S, B1)], 'y', -S, -S + 50, 0, bevel=0.003)
        k.transform(wall, Matrix.Rotation(math.radians(90 * face), 4, 'Z'))
    k.box("BelfryFloor", -S + 50, S - 50, -S + 50, S - 50, B0 - 30, B0, 3, bevel=0.0)
    k.box("Parapet", -S - 15, S + 15, -S - 15, S + 15, B1, B1 + 40, 0, bevel=0.006)
    k.lathe("Roof", [(B1 + 40, S * 1.45), (H, 0)], (0, 0, 0), (0, 0, 1), seg=4, mat=1)
    k.transform(k.parts[-1], Matrix.Rotation(math.radians(45), 4, 'Z'))
    k.lathe("Cross", [(H, 3), (H + 160, 3)], (0, 0, 0), (0, 0, 1), seg=6, mat=2)
    k.box("CrossBar", -40, 40, -3, 3, H + 100, H + 106, 2, bevel=0.0)
    # Campanas colgadas del yugo
    k.box("Yoke", -200, 200, -12, 12, B0 + 380, B0 + 400, 3, bevel=0.004)
    for i, x in enumerate((-90, 90)):
        k.lathe(f"Bell{i}", [(0, 6), (8, 30), (40, 34), (70, 46), (78, 50), (80, 0)], (x, 0, B0 + 380), (0, 0, -1), seg=24, mat=2)
    k.collision_box(-S - 30, S + 30, -S - 30, S + 30, 0, B0)
    k.collision_box(-S, S, -S, S, B0 - 30, B0)
    for sx in (-1, 1):
        for sy in (-1, 1):
            (x0, x1), (y0, y1) = corner(sx), corner(sy)
            k.collision_box(x0, x1, y0, y1, B0, B1 + 40)
    k.collision_box(-S - 15, S + 15, -S - 15, S + 15, B1, B1 + 40)
    save(k, out)


def stone_bridge(out):
    k = Kit("SM_StoneBridge", [STONE])
    L, W, D = 2400.0, 700.0, 900.0
    # Alzado con tres arcos (perfil a lo largo de X, se extruye en el ancho)
    prof = [(-L / 2, 0), (-L / 2, -D)]
    for cx in (-800, 0, 800):
        prof += [(cx - 330, -D)] + [(x, z) for x, z in arch_profile(cx - 330, cx + 330, -D + 300, 330)[::-1]] + [(cx + 330, -D)]
    prof += [(L / 2, -D), (L / 2, 0)]
    k.prism("Spans", prof, 'y', -W / 2, W / 2, 0, bevel=0.003)
    k.box("Deck", -L / 2, L / 2, -W / 2, W / 2, -30, 0, 0, bevel=0.0)
    for s in (-1, 1):
        k.box(f"Parapet{s}", -L / 2, L / 2, s * W / 2 - (40 if s > 0 else 0), s * W / 2 + (40 if s < 0 else 0), 0, 95, 0, bevel=0.008)
        for cx in (-400, 400):
            k.box(f"Cutwater{s}{cx}", cx - 70, cx + 70, s * W / 2 - (0 if s > 0 else 120), s * W / 2 + (120 if s > 0 else 0), -D, -100, 0, bevel=0.01)
    k.collision_box(-L / 2, L / 2, -W / 2, W / 2, -60, 0)
    for s in (-1, 1):
        k.collision_box(-L / 2, L / 2, s * W / 2 - (40 if s > 0 else 0), s * W / 2 + (40 if s < 0 else 0), 0, 95)
    save(k, out)


def arcade(out):
    k = Kit("SM_Arcade", [STONE, TILE])
    L, D, H = 400.0, 300.0, 420.0
    for x in (0, L):
        k.box(f"Pillar{x}", x - 30, x + 30, -30, 30, 0, H, 0, bevel=0.006)
        k.box(f"Base{x}", x - 38, x + 38, -38, 38, 0, 30, 0, bevel=0.004)
    front = [(0, 300)] + arch_profile(30, L - 30, 300, 80)[::-1] + [(L, 300), (L, H), (0, H)]
    k.prism("ArchFront", front, 'y', -30, 30, 0, bevel=0.003)
    k.box("Ceiling", 0, L, -30, D, H - 20, H, 0, bevel=0.0)
    k.box("Roof", -10, L + 10, -60, D, H, H + 25, 1, bevel=0.004)
    for x in (0, L):
        k.collision_box(x - 38, x + 38, -38, 38, 0, H)
    k.collision_box(0, L, -60, D, H - 20, H + 25)
    save(k, out)


# ---------------------------------------------------------------------------
# Muelles
# ---------------------------------------------------------------------------
def fishing_boat(out):
    k = Kit("SM_FishingBoat", [HULL_RED, HULL_WHITE, WOOD, NET, RUBBER, GLASS])
    L, Wd = 900.0, 280.0
    plan = [(-450, -120), (250, -140), (380, -90), (450, 0), (380, 90), (250, 140), (-450, 120)]
    k.prism("HullLow", plan, 'z', -90, 10, 0, bevel=0.01)
    k.prism("HullHigh", plan, 'z', 10, 70, 1, bevel=0.01)
    k.prism("Deck", [(x * 0.97, y * 0.92) for x, y in plan], 'z', 60, 66, 2, bevel=0.0)
    k.box("Wheelhouse", -260, -40, -90, 90, 66, 300, 1, bevel=0.01)
    k.box("Windows", -45, -38, -80, 80, 220, 280, 5, bevel=0.0)
    k.box("CabinRoof", -275, -25, -100, 100, 300, 312, 2, bevel=0.004)
    k.lathe("Mast", [(0, 6), (520, 4)], (60, 0, 66), (0, 0, 1), seg=8, mat=2)
    k.box("Boom", 60, 340, -3, 3, 400, 406, 2, bevel=0.0)
    k.rounded_box("Nets", 220, 0, 100, 160, 150, 60, 25, mat=3)
    for i, x in enumerate((-300, -100, 100, 300)):
        for s in (-1, 1):
            t = k.lathe(f"Tire{s}{i}", [(-12, 26), (12, 26), (12, 14), (-12, 14)], (x, s * 146, 30), (0, 1, 0), seg=14, closed=True, mat=4)
    k.collision_box(-450, 450, -140, 140, -90, 66)
    k.collision_box(-260, -40, -90, 90, 66, 312)
    save(k, out)


def fish_crates(out):
    k = Kit("SM_FishCrates", [CRATE_BLUE, NET])
    for i, (x, y, z, yaw) in enumerate(((0, 0, 0, 0), (0, 0, 32, 3), (0, 0, 64, -2), (75, 5, 0, 8), (75, 5, 32, 4), (20, 62, 0, -6))):
        c = k.box(f"Crate{i}", -35, 35, -25, 25, 0, 30, 0, bevel=0.01)
        k.transform(c, Matrix.Translation((x, y, z)) @ Matrix.Rotation(math.radians(yaw), 4, 'Z'))
    k.rounded_box("NetPile", 60, 70, 15, 70, 60, 30, 12, mat=1)
    k.collision_box(-40, 115, -30, 95, 0, 94)
    save(k, out)


def cargo_ship(out):
    k = Kit("SM_Ship_Cargo", [SHIP_GREY, SHIP_RUST, SHIP_DECK, GLASS, HULL_WHITE])
    L, B = 9000.0, 1500.0
    plan = [(-4500, -700), (-4500, 700), (3200, 750), (4100, 500), (4500, 0), (4100, -500), (3200, -750)]
    k.prism("HullBottom", plan, 'z', -650, -200, 1, bevel=0.002)      # obra viva (óxido bajo la línea de agua)
    k.prism("Hull", plan, 'z', -200, 900, 0, bevel=0.002)
    k.prism("Deck", [(x * 0.995, y * 0.98) for x, y in plan], 'z', 900, 915, 2, bevel=0.0)
    k.prism("Bulwark", [(3200, 750), (4100, 500), (4500, 0), (4100, -500), (3200, -750), (3200, -700), (4050, -470), (4420, 0), (4050, 470), (3200, 700)], 'z', 915, 1060, 0, bevel=0.002)
    # Escotillas de bodega
    for i, x in enumerate((-1500, 0, 1500, 2800)):
        k.box(f"Hatch{i}", x - 550, x + 550, -560, 560, 915, 1080, 1 if i % 2 else 0, bevel=0.003)
    # Superestructura a popa: cuatro cubiertas, puente con ventanas, alerones y chimenea
    for j, (h0, h1, w) in enumerate(((915, 1250, 650), (1250, 1580, 620), (1580, 1910, 590), (1910, 2240, 640))):
        k.box(f"Deckhouse{j}", -4300, -3200, -w, w, h0, h1, 4 if j < 3 else 0, bevel=0.002)
    k.box("BridgeWindows", -3205, -3190, -600, 600, 2040, 2200, 3, bevel=0.0)
    for s in (-1, 1):
        k.box(f"Wing{s}", -3500, -3200, s * 640 - (0 if s > 0 else 220), s * 640 + (220 if s > 0 else 0), 2160, 2240, 0, bevel=0.002)
    k.box("Funnel", -4200, -3700, -180, 180, 2240, 2850, 0, bevel=0.01)
    k.box("FunnelTop", -4210, -3690, -190, 190, 2850, 2890, 1, bevel=0.004)
    k.lathe("Mast", [(0, 12), (900, 8)], (-3500, 0, 2240), (0, 0, 1), seg=8, mat=0)
    # Dos grúas de cubierta
    for i, x in enumerate((-750, 2150)):
        k.box(f"CranePost{i}", x - 90, x + 90, -90, 90, 1080, 1900, 0, bevel=0.004)
        jib = k.box(f"CraneJib{i}", x, x + 1800, -30, 30, 1850, 1920, 0, bevel=0.003)
        k.transform(jib, Matrix.Translation((x, 0, 1885)) @ Matrix.Rotation(math.radians(-18), 4, 'Y') @ Matrix.Translation((-x, 0, -1885)))
    # Amarras de proa (bolardos) y escobén: ahí va la baliza
    for s in (-1, 1):
        k.lathe(f"Bitt{s}", [(0, 22), (60, 22), (66, 30)], (3600, s * 400, 915), (0, 0, 1), seg=12, mat=1)
        k.lathe(f"Hawse{s}", [(0, 40), (30, 34)], (3900, s * 600, 700), (0.3, s, 0), seg=14, mat=1)
    k.collision_box(-4500, 3200, -750, 750, -650, 915)
    k.collision_box(3200, 4500, -500, 500, -650, 915)
    k.collision_box(-4300, -3200, -650, 650, 915, 2240)
    save(k, out)


def main():
    argv = sys.argv[sys.argv.index("--") + 1:] if "--" in sys.argv else []
    base = os.path.abspath(argv[0] if argv else os.path.join(os.path.dirname(__file__), "..", "export"))
    gear = os.path.join(base, "Gear")
    town = os.path.join(base, "OldTown")
    os.makedirs(gear, exist_ok=True)
    os.makedirs(town, exist_ok=True)
    for fn, out in ((corvane_helmet, gear), (corvane_vest, gear), (door, town), (door_frame, town), (breach_charge, town),
                    (bell_tower, town), (stone_bridge, town), (arcade, town), (fishing_boat, town), (fish_crates, town),
                    (cargo_ship, town)):
        bl_lib.reset_scene()
        fn(out)


if __name__ == "__main__":
    main()
