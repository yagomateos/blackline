"""Normal de agua del muelle (Bloque 11, fase 9): olas pequeñas, sin costuras (suma de senos con vectores enteros).

Uso:  python -I gen_water_texture.py <carpeta_salida>
Salida: T_Water_N.png 512x512 RGB normal (DirectX).
"""
import math
import os
import random
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from gen_fx_textures import write_png


def main():
    out = sys.argv[1]
    os.makedirs(out, exist_ok=True)
    size = 512
    rng = random.Random(7)
    # Olas: (kx, ky enteros -> tileable, amplitud, fase); el espectro cae con la frecuencia
    waves = []
    for _ in range(26):
        while True:
            kx, ky = rng.randint(-9, 9), rng.randint(-9, 9)
            if kx or ky:
                break
        k = math.hypot(kx, ky)
        waves.append((kx, ky, 1.0 / (k ** 1.4), rng.uniform(0, 2 * math.pi)))
    tau = 2 * math.pi
    px = [0] * (size * size * 3)
    strength = 0.04
    for y in range(size):
        v = y / size
        for x in range(size):
            u = x / size
            dx = dy = 0.0
            for kx, ky, a, ph in waves:
                c = math.cos(tau * (kx * u + ky * v) + ph) * a * tau
                dx += c * kx
                dy += c * ky
            nx, ny, nz = -dx * strength, dy * strength, 1.0     # DirectX: Y invertida
            l = math.sqrt(nx * nx + ny * ny + nz * nz)
            i = (y * size + x) * 3
            px[i:i + 3] = [int((nx / l * 0.5 + 0.5) * 255), int((ny / l * 0.5 + 0.5) * 255), int((nz / l * 0.5 + 0.5) * 255)]
    write_png(os.path.join(out, "T_Water_N.png"), size, size, px, 3)
    print("OK")


if __name__ == "__main__":
    main()
