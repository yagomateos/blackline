"""Texturas de detalle tileables para el material maestro de armas (numpy, se ejecuta con el Python de Blender).

Uso:  blender -b --factory-startup -P gen_weapon_detail.py -- <carpeta_salida>
Salida (PNG):
  T_Weapon_Micro_N.png  512x512  normal DirectX de grano fino (acabado anodizado/cerakote, ~3 cm por repetición)
  T_Weapon_Smudge.png   1024x1024 R = manchas de rugosidad de baja frecuencia (grasa, huellas, roce)
                                  G = moteado medio (polímero texturizado), B = arañazos finos
Todo se genera filtrando ruido en frecuencia (FFT), así las texturas repiten sin costuras.
"""
import math
import os
import sys

import bpy
import numpy as np

rng = np.random.default_rng(7)


def band_noise(size, f_lo, f_hi, beta=1.0):
    """Ruido tileable con espectro ~1/f^beta limitado a la banda [f_lo, f_hi] (ciclos por textura)."""
    fx = np.fft.fftfreq(size) * size
    f = np.sqrt(fx[:, None] ** 2 + fx[None, :] ** 2)
    f[0, 0] = 1.0
    amp = (f ** -beta) * np.exp(-(f / f_hi) ** 2) * (1.0 - np.exp(-(f / max(f_lo, 1e-3)) ** 2))
    amp[0, 0] = 0.0
    spec = np.fft.fft2(rng.standard_normal((size, size))) * amp
    n = np.real(np.fft.ifft2(spec))
    n -= n.mean()
    return n / (n.std() + 1e-9)


def norm01(a, lo=-2.5, hi=2.5):
    return np.clip((a - lo) / (hi - lo), 0.0, 1.0)


def scratches(size, count):
    """Arañazos finos: segmentos cortos con orientación aleatoria (envolviendo los bordes)."""
    img = np.zeros((size, size), np.float32)
    for _ in range(count):
        x, y = rng.uniform(0, size, 2)
        ang = rng.uniform(0, math.pi)
        length = rng.uniform(size * 0.01, size * 0.06)
        strength = rng.uniform(0.3, 1.0)
        steps = int(length * 2)
        for s in range(steps):
            t = s / max(steps - 1, 1)
            px = int(x + math.cos(ang) * length * t) % size
            py = int(y + math.sin(ang) * length * t) % size
            img[py, px] = max(img[py, px], strength * (1.0 - abs(t - 0.5) * 1.2))
    return img


def save(path, rgb):
    h, w, _ = rgb.shape
    rgba = np.concatenate([rgb, np.ones((h, w, 1), np.float32)], axis=-1)
    img = bpy.data.images.new(os.path.basename(path), w, h, alpha=False)
    img.pixels.foreach_set(rgba[::-1].astype(np.float32).ravel())  # Blender guarda de abajo arriba
    img.filepath_raw = path
    img.file_format = 'PNG'
    img.save()
    print(f"[weapon_detail] {path}")


def micro_normal(size=512, strength=0.45):
    h = 0.7 * band_noise(size, 16, 70, 0.8) + 0.3 * band_noise(size, 50, 140, 0.4)
    dx = (np.roll(h, -1, axis=1) - np.roll(h, 1, axis=1)) * 0.5
    dy = (np.roll(h, -1, axis=0) - np.roll(h, 1, axis=0)) * 0.5
    nx, ny = -dx * strength / 10.0, dy * strength / 10.0  # DirectX: Y hacia abajo
    nz = np.ones_like(nx)
    ln = np.sqrt(nx * nx + ny * ny + nz * nz)
    return np.stack([nx / ln * 0.5 + 0.5, ny / ln * 0.5 + 0.5, nz / ln * 0.5 + 0.5], axis=-1)


def smudge(size=1024):
    low = band_noise(size, 1.5, 10, 1.6)
    blobs = np.clip(band_noise(size, 4, 24, 1.0) - 0.8, 0, None) * 1.4  # manchas sueltas (grasa/huellas)
    r = norm01(low * 0.8 + blobs, -2.2, 2.6)
    g = norm01(band_noise(size, 60, 220, 0.3), -2.5, 2.5)
    b = np.clip(scratches(size, 260), 0, 1)
    return np.stack([r, g, b], axis=-1)


def main():
    argv = sys.argv[sys.argv.index("--") + 1:] if "--" in sys.argv else []
    out = os.path.abspath(argv[0] if argv else ".")
    os.makedirs(out, exist_ok=True)
    save(os.path.join(out, "T_Weapon_Micro_N.png"), micro_normal())
    save(os.path.join(out, "T_Weapon_Smudge.png"), smudge())


main()
