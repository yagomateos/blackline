"""Texturas procedurales de efectos (sin dependencias): atlas de humo/polvo y decals de impacto por material.

Uso:  python -I gen_fx_textures.py <carpeta_salida>
Salida (PNG):
  T_FX_SmokeAtlas.png      512x512 RGBA, 2x2 bocanadas (blanco, alfa = densidad)
  T_Decal_<Mat>_BC.png     256x256 RGBA (color + opacidad)
  T_Decal_<Mat>_N.png      256x256 RGB normal (DirectX, a partir de un mapa de alturas)
  Mat = Concrete, Metal, Wood, Glass, Dirt; además T_Decal_Blood_* (salpicadura de sangre)
"""
import math
import os
import random
import struct
import sys
import zlib


def write_png(path, w, h, pixels, channels):
    """pixels: lista plana de enteros 0..255 (fila a fila)."""
    raw = bytearray()
    stride = w * channels
    for y in range(h):
        raw.append(0)
        raw += bytes(pixels[y * stride:(y + 1) * stride])
    color_type = 6 if channels == 4 else 2

    def chunk(tag, data):
        return struct.pack(">I", len(data)) + tag + data + struct.pack(">I", zlib.crc32(tag + data) & 0xFFFFFFFF)

    png = b"\x89PNG\r\n\x1a\n" + chunk(b"IHDR", struct.pack(">IIBBBBB", w, h, 8, color_type, 0, 0, 0))
    png += chunk(b"IDAT", zlib.compress(bytes(raw), 9)) + chunk(b"IEND", b"")
    with open(path, "wb") as f:
        f.write(png)


class ValueNoise:
    def __init__(self, seed, size=256):
        rng = random.Random(seed)
        self.size = size
        self.v = [rng.random() for _ in range(size * size)]

    def at(self, x, y):
        s = self.size
        xi, yi = int(math.floor(x)), int(math.floor(y))
        fx, fy = x - xi, y - yi
        fx, fy = fx * fx * (3 - 2 * fx), fy * fy * (3 - 2 * fy)

        def g(i, j):
            return self.v[(j % s) * s + (i % s)]
        a = g(xi, yi) + (g(xi + 1, yi) - g(xi, yi)) * fx
        b = g(xi, yi + 1) + (g(xi + 1, yi + 1) - g(xi, yi + 1)) * fx
        return a + (b - a) * fy

    def fbm(self, x, y, octaves=5, lac=2.0, gain=0.5):
        amp, freq, tot, norm = 1.0, 1.0, 0.0, 0.0
        for _ in range(octaves):
            tot += amp * self.at(x * freq, y * freq)
            norm += amp
            amp *= gain
            freq *= lac
        return tot / norm


def clamp01(v):
    return 0.0 if v < 0 else 1.0 if v > 1 else v


def smoothstep(a, b, x):
    t = clamp01((x - a) / (b - a))
    return t * t * (3 - 2 * t)


def smoke_atlas(path):
    N, cell = 512, 256
    px = [0] * (N * N * 4)
    for q in range(4):
        noise = ValueNoise(100 + q)
        ox, oy = (q % 2) * cell, (q // 2) * cell
        for y in range(cell):
            for x in range(cell):
                u, v = (x + 0.5) / cell * 2 - 1, (y + 0.5) / cell * 2 - 1
                r = math.sqrt(u * u + v * v)
                n = noise.fbm(x / 28.0, y / 28.0, 5)
                # forma de bocanada irregular: radio deformado por ruido
                edge = 0.86 + (n - 0.5) * 0.45
                a = smoothstep(edge, edge - 0.6, r) * (0.55 + 0.45 * noise.fbm(x / 12.0 + 50, y / 12.0, 4))
                i = ((oy + y) * N + ox + x) * 4
                shade = int(200 + 55 * n)
                px[i:i + 4] = [shade, shade, shade, int(255 * clamp01(a * 1.25))]
    write_png(path, N, N, px, 4)


def height_to_normal(h, size, strength):
    px = [0] * (size * size * 3)
    for y in range(size):
        for x in range(size):
            dx = h[y * size + min(x + 1, size - 1)] - h[y * size + max(x - 1, 0)]
            dy = h[min(y + 1, size - 1) * size + x] - h[max(y - 1, 0) * size + x]
            nx, ny, nz = -dx * strength, dy * strength, 1.0  # DirectX: Y invertida
            l = math.sqrt(nx * nx + ny * ny + nz * nz)
            i = (y * size + x) * 3
            px[i:i + 3] = [int((nx / l * 0.5 + 0.5) * 255), int((ny / l * 0.5 + 0.5) * 255), int((nz / l * 0.5 + 0.5) * 255)]
    return px


def decal(material, out_dir, seed=1):
    S = 256
    rng = random.Random(seed)
    noise = ValueNoise(seed * 7)
    rgba = [0] * (S * S * 4)
    height = [0.0] * (S * S)
    # parámetros por material
    cracks = [rng.uniform(0, 2 * math.pi) for _ in range({"Concrete": 7, "Glass": 12, "Wood": 3}.get(material, 0))]
    for y in range(S):
        for x in range(S):
            u, v = (x + 0.5) / S * 2 - 1, (y + 0.5) / S * 2 - 1
            if material == "Wood":
                u *= 0.75   # agujero algo alargado en la veta
            r = math.sqrt(u * u + v * v)
            ang = math.atan2(v, u)
            n = noise.fbm(x / 16.0, y / 16.0, 4)
            hole = 0.11 + (n - 0.5) * 0.05
            col, a, h = (0, 0, 0), 0.0, 0.0
            if material in ("Concrete", "Dirt", "Wood"):
                ring = (0.42 if material == "Concrete" else 0.3 if material == "Dirt" else 0.32) + (n - 0.5) * 0.25
                if r < hole:
                    col, a, h = (12, 11, 10), 1.0, -1.0 + r / hole * 0.3
                elif r < ring:
                    t = (r - hole) / (ring - hole)
                    base = {"Concrete": (150, 145, 135), "Dirt": (60, 45, 32), "Wood": (170, 125, 80)}[material]
                    shade = 0.75 + 0.35 * noise.fbm(x / 6.0 + 9, y / 6.0, 3)
                    col = tuple(int(c * shade) for c in base)
                    a = (1 - t) ** 0.6 * (0.7 + 0.3 * n)
                    h = -0.6 * (1 - t) + 0.25 * (noise.fbm(x / 5.0, y / 5.0, 3) - 0.5)
                # hollín/polvo alrededor
                soot = smoothstep(0.95, 0.3, r) * 0.35 * n
                if a < soot and material != "Wood":
                    col, a = (40, 38, 35), soot
            elif material == "Metal":
                if r < 0.09:
                    col, a, h = (8, 8, 9), 1.0, -1.0
                elif r < 0.2:
                    t = (r - 0.09) / 0.11
                    col, a, h = (200, 200, 205), 1.0 - t * 0.6, 0.6 * (1 - t)   # borde brillante doblado
                elif r < 0.5:
                    t = (r - 0.2) / 0.3
                    col, a, h = (110, 105, 100), (1 - t) * 0.5, -0.2 * (1 - t)  # pintura saltada
            elif material == "Glass":
                if r < 0.06:
                    col, a, h = (230, 235, 240), 0.95, -0.5
                elif r < 0.16:
                    col, a = (240, 245, 250), 0.55 * (1 - (r - 0.06) / 0.1) + 0.2
                # anillos concéntricos de fractura
                for rr in (0.32, 0.55):
                    d = abs(r - rr * (0.9 + 0.2 * n))
                    if d < 0.008:
                        col, a = (245, 248, 252), max(a, 0.8)
            # grietas radiales
            for c in cracks:
                da = abs((ang - c + math.pi) % (2 * math.pi) - math.pi)
                width = 0.012 if material == "Glass" else 0.02
                length = 0.95 if material == "Glass" else 0.65
                wiggle = (noise.fbm(r * 20, c * 10, 3) - 0.5) * 0.08
                if da * r < width * (1 - r / length) + abs(wiggle) * 0.1 and hole < r < length:
                    if material == "Glass":
                        col, a = (245, 248, 252), max(a, 0.85)
                    else:
                        col, a, h = (20, 18, 16), max(a, 0.9 * (1 - r / length)), -0.5
            # borde suave: nada fuera del círculo unidad
            a *= smoothstep(1.0, 0.85, r)
            i = (y * S + x) * 4
            rgba[i:i + 4] = [col[0], col[1], col[2], int(255 * clamp01(a))]
            height[y * S + x] = h
    write_png(os.path.join(out_dir, f"T_Decal_{material}_BC.png"), S, S, rgba, 4)
    write_png(os.path.join(out_dir, f"T_Decal_{material}_N.png"), S, S, height_to_normal(height, S, 6.0), 3)


def blood(out_dir, seed=11):
    """Salpicadura de sangre (pared/suelo detrás del impacto): mancha irregular, gotas y regueros en abanico."""
    S = 256
    rng = random.Random(seed)
    noise = ValueNoise(seed * 5)
    drops = []
    for _ in range(46):
        ang = rng.gauss(0.0, 0.7)                      # la mayoría en la dirección del disparo (+u)
        dist = rng.uniform(0.2, 0.9) ** 0.8
        drops.append((math.cos(ang) * dist, math.sin(ang) * dist, rng.uniform(0.012, 0.05) * (1.2 - dist), ang))
    rgba = [0] * (S * S * 4)
    height = [0.0] * (S * S)
    for y in range(S):
        for x in range(S):
            u, v = (x + 0.5) / S * 2 - 1, (y + 0.5) / S * 2 - 1
            n = noise.fbm(x / 14.0, y / 14.0, 4)
            r = math.sqrt(u * u + v * v)
            a = smoothstep(0.26 + (n - 0.5) * 0.22, 0.2 + (n - 0.5) * 0.22, r)   # mancha central
            for du, dv, rad, ang in drops:
                # gota alargada en su dirección (estela)
                ca, sa = math.cos(ang), math.sin(ang)
                pu, pv = (u - du) * ca + (v - dv) * sa, -(u - du) * sa + (v - dv) * ca
                d = math.sqrt((pu / (2.2 if pu < 0 else 1.0)) ** 2 + pv * pv)
                a = max(a, smoothstep(rad, rad * 0.6, d))
            a *= smoothstep(1.0, 0.85, r)
            dark = 0.55 + 0.45 * noise.fbm(x / 5.0 + 3, y / 5.0, 3)
            col = (int(92 * dark), int(8 * dark), int(7 * dark))
            i = (y * S + x) * 4
            rgba[i:i + 4] = [col[0], col[1], col[2], int(255 * clamp01(a * 0.95))]
            height[y * S + x] = a * 0.35
    write_png(os.path.join(out_dir, "T_Decal_Blood_BC.png"), S, S, rgba, 4)
    write_png(os.path.join(out_dir, "T_Decal_Blood_N.png"), S, S, height_to_normal(height, S, 3.0), 3)


if __name__ == "__main__":
    out = sys.argv[1]
    os.makedirs(out, exist_ok=True)
    smoke_atlas(os.path.join(out, "T_FX_SmokeAtlas.png"))
    for k, m in enumerate(("Concrete", "Metal", "Wood", "Glass", "Dirt")):
        decal(m, out, seed=k + 3)
    blood(out)
    print("OK")
