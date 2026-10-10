"""Sonidos de la granada M-6 (Bloque 11). Síntesis propia.

Uso:  python -I gen_grenade_sfx.py <carpeta_salida SFX>
Salida: Weapons/Grenade/SW_Grenade_Explosion_XX (3D, cercana), SW_Grenade_Pin, SW_Grenade_Throw, SW_Grenade_Bounce_XX
"""
import math
import os
import random
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import audiolib as A
from process_sfx import SR, burst, grains, mix_into, mk, noise, seconds, tone


def explosion(rng):
    """Detonación a < 20 m: frente de presión (chasquido + golpe), cuerpo grave, lluvia de cascotes y cola de la ciudad."""
    n = seconds(4.5)
    out = [0.0] * n
    # Frente de choque: onda N corta y muy fuerte + ruido de banda ancha
    for i in range(int(0.004 * SR)):
        out[i] += 1.0 - 2.0 * i / (0.004 * SR)
    mix_into(out, burst(rng, 0.06, 200, 12000, 0.012, 0.0002), 0, 0.9)
    # Cuerpo: ruido grave con caída larga + tono de "pecho"
    body = A.lowpass(A.lowpass(noise(seconds(2.5), rng), SR, 260), SR, 300)
    mix_into(out, [x * math.exp(-(i / SR) / 0.45) * 3.0 for i, x in enumerate(body)], seconds(0.002))
    mix_into(out, tone(rng.uniform(42, 55), 1.2, 0.35, 0, 0.6), 0, 0.9)
    # Medios rasgados (el aire y los cristales)
    mid = A.biquad(noise(seconds(1.2), rng), SR, "bp", 1500, 0.6)
    mix_into(out, [x * math.exp(-(i / SR) / 0.12) for i, x in enumerate(mid)], 0, 0.6)
    # Cascotes y metralla cayendo
    mix_into(out, grains(rng, 2.5, 90, 1500, 9000, (0.002, 0.008), 0.6, 0.25), seconds(0.25))
    # Eco de las fachadas
    tail = A.lowpass(noise(seconds(4.0), rng), SR, 700)
    mix_into(out, [x * (min(1.0, (i / SR) / 0.08)) * math.exp(-(i / SR) / 1.1) * 0.5 for i, x in enumerate(tail)], seconds(0.06))
    for d, g in ((0.11, 0.35), (0.24, 0.22), (0.41, 0.14)):
        mix_into(out, A.lowpass(out[:seconds(0.3)], SR, 900), seconds(d), g)
    s = A.compress(out, SR, -14, 4, 1, 120, 0)
    return A.saturate(s, 1.4)


def pin(rng):
    s = tone(4200, 0.08, 0.02)
    mix_into(s, burst(rng, 0.03, 3000, 10000, 0.006), 0, 0.5)
    mix_into(s, tone(2900, 0.15, 0.05), seconds(0.12), 0.6)     # la palanca salta ("ping")
    mix_into(s, tone(6100, 0.1, 0.03), seconds(0.12), 0.3)
    return s


def throw(rng):
    n = seconds(0.4)
    w = A.biquad(noise(n, rng), SR, "bp", 900, 0.8)
    return [x * math.sin(math.pi * i / n) ** 2 for i, x in enumerate(w)]


def bounce(rng):
    s = burst(rng, 0.03, 300, 3000, 0.008)
    for f, g, d in ((1800, 0.4, 0.05), (3100, 0.25, 0.03), (520, 0.5, 0.04)):
        mix_into(s, tone(f * rng.uniform(0.95, 1.05), 0.15, d), 0, g)
    return s


def main():
    out = sys.argv[1]
    for k in range(3):
        A.write_wav(mk(os.path.join(out, "Weapons", "Grenade", f"SW_Grenade_Explosion_{k + 1:02d}.wav")), [explosion(random.Random(60 + k))], peak_db=-0.5)
    for k in range(3):
        A.write_wav(mk(os.path.join(out, "Weapons", "Grenade", f"SW_Grenade_Bounce_{k + 1:02d}.wav")), [bounce(random.Random(70 + k))], peak_db=-3.0)
    A.write_wav(mk(os.path.join(out, "Weapons", "Grenade", "SW_Grenade_Pin.wav")), [pin(random.Random(80))], peak_db=-4.0)
    A.write_wav(mk(os.path.join(out, "Weapons", "Grenade", "SW_Grenade_Throw.wav")), [throw(random.Random(81))], peak_db=-6.0)
    print("OK")


if __name__ == "__main__":
    main()
