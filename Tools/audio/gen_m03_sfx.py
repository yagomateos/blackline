"""Sonidos de la misión 3 "Ría" (casco viejo y muelles al amanecer). Síntesis propia.

Uso:  python -I gen_m03_sfx.py <carpeta_salida SFX>
Salida:
  World/SW_Door_Open            puerta de madera que se abre (picaporte + bisagra que cruje)
  World/SW_Door_Kick            patada: golpe seco, cerradura que salta, astillas, la hoja contra la pared
  World/SW_Breach_Beep          pitido corto del detonador de la carga
  Ambience/Distant/SW_Bells_Toll   campanas tocando las 7:30 (dos campanas, síntesis modal, cola larga)
  Vehicles/SW_Ship_Horn         sirena de barco (tifón grave de dos tonos, 4 s, eco en la ría)
  Ambience/Zones/SW_AmbZ_Gulls_Loop gaviotas lejanas y agua contra los pilotes
"""
import math
import os
import random
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import audiolib as A
from process_sfx import SR, burst, grains, mix_into, mk, noise, seconds, tone
from gen_world_audio import echo, loopify, smooth_noise

TAU = 2 * math.pi


def modal(freqs, decays, amps, dur, rng):
    out = [0.0] * seconds(dur)
    for f, d, a in zip(freqs, decays, amps):
        ph = rng.uniform(0, TAU)
        for i in range(len(out)):
            t = i / SR
            out[i] += math.sin(ph + TAU * f * t) * a * math.exp(-t / d)
    return out


def bell(rng, f0):
    # Parciales de campana de iglesia (hum, prime, tercera menor, quinta, nominal...)
    ratios = (0.5, 1.0, 1.183, 1.506, 2.0, 2.514, 2.662, 3.011, 4.166)
    decays = (9.0, 6.0, 4.5, 3.5, 3.0, 2.2, 2.0, 1.6, 1.1)
    amps = (0.35, 0.5, 0.4, 0.2, 0.45, 0.18, 0.12, 0.1, 0.08)
    s = modal([f0 * r * rng.uniform(0.997, 1.003) for r in ratios], decays, amps, 9.0, rng)
    mix_into(s, burst(rng, 0.02, 800, 6000, 0.004), 0, 0.5)   # golpe del badajo
    return s


def bells(rng):
    out = [0.0] * seconds(16.0)
    seq = [(0.0, 0), (1.6, 1), (3.2, 0), (4.8, 1), (6.4, 0), (8.0, 1), (9.6, 0)]
    big, small = bell(rng, 196.0), bell(rng, 262.0)
    for t, which in seq:
        mix_into(out, big if which == 0 else small, seconds(t), 0.5 if which == 0 else 0.38)
    return echo(out, ((0.25, 0.3, 2000), (0.6, 0.18, 1400)))


def ship_horn(rng):
    n = seconds(4.0)
    out = []
    for i in range(n):
        t = i / SR
        env = min(1.0, t / 0.25) * (1.0 if t < 3.3 else max(0.0, 1 - (t - 3.3) / 0.7))
        s = 0.0
        for f, a in ((110.0, 1.0), (138.6, 0.7)):
            for h in range(1, 8):
                s += math.sin(TAU * f * h * t + 0.4 * math.sin(TAU * 5.0 * t)) * a / (h ** 1.1)
        out.append(s * env * 0.18)
    out = A.saturate(out, 1.4)
    return echo(out, ((0.7, 0.35, 900), (1.6, 0.22, 700), (2.8, 0.12, 500)))


def door_open(rng):
    out = [0.0] * seconds(1.3)
    mix_into(out, burst(rng, 0.03, 1200, 7000, 0.006), 0, 0.6)          # picaporte
    mix_into(out, tone(2100, 0.06, 0.02), 0, 0.15)
    # Bisagra: tono que sube y baja con rozamiento (grano)
    n = seconds(0.9)
    ph = 0.0
    creak = []
    for i in range(n):
        t = i / SR
        f = 420 + 260 * math.sin(math.pi * t / 0.9) + 40 * math.sin(TAU * 13 * t)
        ph += TAU * f / SR
        creak.append((math.sin(ph) + 0.5 * math.sin(2.02 * ph)) * math.sin(math.pi * t / 0.9) * (0.6 + 0.4 * rng.random()))
    mix_into(out, A.biquad(creak, SR, "bp", 900, 1.2), seconds(0.12), 0.35)
    return out


def door_kick(rng):
    out = [0.0] * seconds(1.5)
    mix_into(out, A.lowpass(noise(seconds(0.2), rng), SR, 400), 0, 1.4)    # golpe de la bota contra la hoja
    mix_into(out, [x * math.exp(-i / (0.05 * SR)) for i, x in enumerate(tone(85, 0.3, 0.12))], 0, 1.0)
    mix_into(out, burst(rng, 0.05, 1500, 9000, 0.01), seconds(0.02), 0.7)     # la cerradura salta
    mix_into(out, grains(rng, 0.6, 40, 1200, 6000, (0.002, 0.006), 0.12, 0.35), seconds(0.03))   # astillas
    mix_into(out, A.lowpass(noise(seconds(0.25), rng), SR, 600), seconds(0.38), 1.0)   # contra la pared
    return A.saturate(A.compress(out, SR, -10, 4, 1, 80, 0), 1.3)


def breach_beep(rng):
    b = tone(3400, 0.07, 0.04)
    return [x * min(1.0, i / (0.002 * SR)) for i, x in enumerate(b)]


def gulls(rng, dur=30.0):
    n = seconds(dur + 2.0)
    out = [0.0] * n
    water = A.lowpass(noise(n, rng), SR, 700)
    lap = smooth_noise(n, rng, 0.9)
    for i in range(n):
        out[i] = water[i] * (0.1 + 0.4 * lap[i] ** 2)
    for _ in range(int(dur / 2.5)):
        t0 = rng.randrange(n - seconds(1.5))
        calls = rng.randint(1, 3)
        for c in range(calls):
            dur_c = rng.uniform(0.25, 0.45)
            m = seconds(dur_c)
            f0 = rng.uniform(1300, 1900)
            ph = 0.0
            cry = []
            for i in range(m):
                t = i / m
                f = f0 * (1.25 - 0.45 * t) + 120 * math.sin(TAU * 22 * t * dur_c)
                ph += TAU * f / SR
                env = math.sin(math.pi * t) ** 0.6
                cry.append((math.sin(ph) + 0.4 * math.sin(2 * ph) + 0.2 * math.sin(3 * ph)) * env)
            cry = A.biquad(cry, SR, "bp", 2200, 0.8)
            mix_into(out, cry, t0 + seconds(c * (dur_c + 0.08)), rng.uniform(0.05, 0.16))
    return loopify([out], 2.0)[0]


def main():
    root = sys.argv[1]
    w = lambda rel, sig, peak: A.write_wav(mk(os.path.join(root, rel)), [sig], peak_db=peak)
    w("World/SW_Door_Open.wav", door_open(random.Random(301)), -4.0)
    w("World/SW_Door_Kick.wav", door_kick(random.Random(302)), -1.0)
    w("World/SW_Breach_Beep.wav", breach_beep(random.Random(303)), -6.0)
    w("Ambience/Distant/SW_Bells_Toll.wav", bells(random.Random(304)), -1.0)
    w("Vehicles/SW_Ship_Horn.wav", ship_horn(random.Random(305)), -0.5)
    w("Ambience/Zones/SW_AmbZ_Gulls_Loop.wav", gulls(random.Random(306)), -4.0)
    print("OK")


if __name__ == "__main__":
    main()
