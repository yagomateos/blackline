"""Sonidos de la misión 4 "Fuego cruzado". Síntesis propia.

Uso:  python -I gen_m04_sfx.py <carpeta_salida SFX>
Salida:
  World/SW_Mortar_Whistle   silbido del proyectil de mortero que cae (1,4 s, baja de tono y crece)
  World/SW_MG_Overheat      chisporroteo del cañón al rojo y el cerrojo que se bloquea
  Vehicles/SW_Jet_Flyby     pasada de dos cazas a baja altura (rugido con Doppler y estela, 6 s)
"""
import math
import os
import random
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import audiolib as A
from process_sfx import SR, burst, grains, mix_into, mk, noise, seconds, tone

TAU = 2 * math.pi


def mortar_whistle(rng):
    n = seconds(1.4)
    out, ph = [], 0.0
    hiss = A.biquad(noise(n, rng), SR, "bp", 2500, 1.5)
    for i in range(n):
        t = i / n
        f = 1900 - 900 * t ** 1.4
        ph += TAU * f / SR
        env = (0.15 + 0.85 * t ** 2)
        out.append((math.sin(ph) * 0.6 + hiss[i] * 0.4) * env)
    return A.lowpass(out, SR, 5000)


def mg_overheat(rng):
    out = [0.0] * seconds(1.6)
    mix_into(out, burst(rng, 0.04, 1200, 7000, 0.01), 0, 0.6)          # cerrojo que no cierra
    mix_into(out, tone(1500, 0.08, 0.03), 0, 0.15)
    sizzle = A.biquad(noise(seconds(1.5), rng), SR, "hp", 3500, 0.7)
    mix_into(out, [x * 0.25 * math.exp(-i / (0.7 * SR)) for i, x in enumerate(sizzle)], seconds(0.05))
    mix_into(out, grains(rng, 1.4, 50, 2000, 9000, (0.001, 0.003), 0.4, 0.2), seconds(0.1))
    return out


def jet_flyby(rng):
    n = seconds(6.0)
    out = [0.0] * n
    roar = A.lowpass(noise(n, rng), SR, 900)
    tear = A.biquad(noise(n, rng), SR, "bp", 3500, 0.8)
    for jet, t0 in ((0, 2.6), (1, 3.1)):
        ph = rng.uniform(0, TAU)
        for i in range(n):
            t = i / SR
            d = t - t0
            amp = 1.0 / (1.0 + (d / 0.55) ** 2)                       # pasa por encima en t0
            f = 180 * (1.25 if d < 0 else 0.8) + 40 * math.tanh(-d * 3)   # Doppler: agudo al llegar, grave al irse
            ph += TAU * f / SR
            out[i] += (roar[i] * 1.2 + tear[i] * (0.7 if d < 0 else 0.35) + math.sin(ph) * 0.25) * amp * (0.9 if jet else 1.0)
    return A.saturate(A.compress(out, SR, -14, 3, 5, 200, 0), 1.3)


def main():
    root = sys.argv[1]
    w = lambda rel, sig, peak: A.write_wav(mk(os.path.join(root, rel)), [sig], peak_db=peak)
    w("World/SW_Mortar_Whistle.wav", mortar_whistle(random.Random(401)), -3.0)
    w("World/SW_MG_Overheat.wav", mg_overheat(random.Random(402)), -4.0)
    w("Vehicles/SW_Jet_Flyby.wav", jet_flyby(random.Random(403)), -0.5)
    print("OK")


if __name__ == "__main__":
    main()
