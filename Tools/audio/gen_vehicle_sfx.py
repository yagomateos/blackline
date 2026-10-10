"""Sonidos de los vehículos (Bloque 11, fases 8-9). Síntesis propia.

Uso:  python -I gen_vehicle_sfx.py <carpeta_salida SFX>
Salida (Vehicles/):
  SW_BTR_Engine_Loop      diésel V8 al ralentí-medio: pulsos de combustión, turbo, traqueteo de la carrocería
  SW_BTR_Cannon_XX        cañón de 30 mm: estampido seco, golpe grave y eco de las fachadas
  SW_Heli_Rotor_Loop      rotor de 4 palas (golpe de pala ~17 Hz), turbina y viento
  SW_Heli_DoorGun_XX      ráfaga de ametralladora de puerta (8-10 disparos)
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


def btr_engine(rng, dur=8.0):
    n = seconds(dur + 1.5)
    out = [0.0] * n
    rpm_mod = smooth_noise(n, rng, 0.7)
    # Combustión: tren de pulsos graves (V8 a ~1100 rpm -> ~73 Hz de encendidos) con jitter
    ph = 0.0
    pulse = A.lowpass(noise(seconds(0.02), rng), SR, 400)
    t_next = 0.0
    i = 0
    while i < n:
        f = 70.0 + 8.0 * rpm_mod[i]
        g = 0.7 + 0.3 * rng.random()
        mix_into(out, [x * g * math.exp(-k / (0.006 * SR)) for k, x in enumerate(pulse)], i)
        i += int(SR / f * rng.uniform(0.96, 1.04))
    out = out[:n]
    out = A.lowpass(out, SR, 450)
    # Armónicos del motor (cuerpo constante)
    for h, g in ((36.5, 0.5), (73.0, 0.35), (146.0, 0.12)):
        ph = rng.uniform(0, TAU)
        for k in range(n):
            ph += TAU * h * (1 + 0.06 * rpm_mod[k]) / SR
            out[k] += math.sin(ph) * g
    # Turbo: silbido fino que sube y baja
    whine = A.biquad(noise(n, rng), SR, "bp", 3200, 12.0)
    for k in range(n):
        out[k] += whine[k] * (0.25 + 0.2 * rpm_mod[k])
    # Traqueteo de la carrocería y las orugas de las ruedas sobre el asfalto roto
    for _ in range(int(25 * dur)):
        g = burst(rng, rng.uniform(0.004, 0.012), 400, 3500, 0.003)
        mix_into(out, g, rng.randrange(n - seconds(0.02)), 0.18 * rng.uniform(0.3, 1.0))
    road = A.lowpass(noise(n, rng), SR, 180)
    for k in range(n):
        out[k] += road[k] * 0.6
    s = loopify([out], 1.5)[0]
    return A.saturate(A.compress(s, SR, -16, 3, 5, 120, 0), 1.3)


def btr_cannon(rng):
    n = seconds(3.5)
    out = [0.0] * n
    # Boca: onda N muy corta + estampido de banda ancha
    for i in range(int(0.003 * SR)):
        out[i] += 1.0 - 2.0 * i / (0.003 * SR)
    mix_into(out, burst(rng, 0.08, 150, 14000, 0.018, 0.0001), 0, 1.0)
    # Golpe de presión grave
    body = A.lowpass(noise(seconds(1.5), rng), SR, 220)
    mix_into(out, [x * math.exp(-(i / SR) / 0.22) * 3.0 for i, x in enumerate(body)], seconds(0.001))
    mix_into(out, tone(rng.uniform(55, 65), 0.6, 0.18, 0, 0.7), 0, 0.9)
    # Mecánica del cierre (clac metálico del cañón al reciclar)
    mix_into(out, tone(1900, 0.08, 0.02), seconds(0.16), 0.15)
    mix_into(out, burst(rng, 0.03, 1500, 7000, 0.006), seconds(0.16), 0.2)
    # Eco de las fachadas de la calle
    s = echo(out, ((0.09, 0.4, 1200), (0.21, 0.3, 900), (0.38, 0.2, 700), (0.7, 0.12, 500)))
    tail = A.lowpass(noise(seconds(3.0), rng), SR, 600)
    mix_into(s, [x * min(1.0, (i / SR) / 0.05) * math.exp(-(i / SR) / 0.8) * 0.35 for i, x in enumerate(tail)], seconds(0.05))
    return A.saturate(A.compress(s, SR, -12, 4, 1, 100, 0), 1.5)


def heli_rotor(rng, dur=8.0):
    n = seconds(dur + 1.5)
    out = [0.0] * n
    mod = smooth_noise(n, rng, 0.4)
    # Golpe de pala (blade slap): 4 palas x 4.3 rev/s ~ 17 Hz
    slap = A.biquad(noise(seconds(0.04), rng), SR, "bp", 160, 0.7)
    t = 0.0
    while t < dur + 1.4:
        g = 0.75 + 0.25 * math.sin(TAU * t * 0.9) + 0.1 * rng.random()
        k = seconds(t)
        mix_into(out, [x * g * math.exp(-j / (0.012 * SR)) for j, x in enumerate(slap)], k)
        t += 1.0 / 17.2 * rng.uniform(0.985, 1.015)
    out = out[:n]
    # Fundamental grave del rotor
    ph = 0.0
    for k in range(n):
        ph += TAU * 17.2 / SR
        out[k] += math.sin(ph) * 0.4 + math.sin(2 * ph) * 0.2
    # Turbina: tono agudo inestable + soplo
    tph = 0.0
    hiss = A.biquad(noise(n, rng), SR, "bp", 5200, 2.0)
    for k in range(n):
        tph += TAU * (2350 + 25 * mod[k]) / SR
        out[k] += math.sin(tph) * 0.035 + hiss[k] * 0.12
    # Viento del rotor (downwash)
    wind = A.lowpass(noise(n, rng), SR, 500)
    for k in range(n):
        out[k] += wind[k] * (0.6 + 0.3 * mod[k])
    s = loopify([out], 1.5)[0]
    return A.compress(s, SR, -16, 3, 5, 150, 0)


def door_gun(rng):
    rounds = rng.randint(8, 10)
    period = 60.0 / 750.0
    out = [0.0] * seconds(rounds * period + 1.5)
    for r in range(rounds):
        k = seconds(r * period * rng.uniform(0.97, 1.03))
        shot = burst(rng, 0.05, 200, 11000, 0.012, 0.0002)
        body = [x * math.exp(-(i / SR) / 0.06) * 1.6 for i, x in enumerate(A.lowpass(noise(seconds(0.25), rng), SR, 350))]
        mix_into(out, shot, k, rng.uniform(0.85, 1.0))
        mix_into(out, body, k)
    s = echo(out, ((0.12, 0.3, 1000), (0.3, 0.18, 700)))
    return A.saturate(A.compress(s, SR, -14, 4, 1, 80, 0), 1.3)


def main():
    out = os.path.join(sys.argv[1], "Vehicles")
    A.write_wav(mk(os.path.join(out, "SW_BTR_Engine_Loop.wav")), [btr_engine(random.Random(90))], peak_db=-2.0)
    for k in range(3):
        A.write_wav(mk(os.path.join(out, f"SW_BTR_Cannon_{k + 1:02d}.wav")), [btr_cannon(random.Random(91 + k))], peak_db=-0.5)
    A.write_wav(mk(os.path.join(out, "SW_Heli_Rotor_Loop.wav")), [heli_rotor(random.Random(95))], peak_db=-2.0)
    for k in range(3):
        A.write_wav(mk(os.path.join(out, f"SW_Heli_DoorGun_{k + 1:02d}.wav")), [door_gun(random.Random(96 + k))], peak_db=-1.0)
    print("OK")


if __name__ == "__main__":
    main()
