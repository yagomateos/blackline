"""Sonidos de la misión 2 "Manifiesto" (refinería de noche). Síntesis propia.

Uso:  python -I gen_m02_sfx.py <carpeta_salida SFX>
Salida:
  Vehicles/SW_Drone_Buzz_Loop       zumbido de 4 rotores (batido entre hélices, armónicos agudos)
  Vehicles/SW_Drone_Alert           pitido doble del dron al marcar al jugador
  Vehicles/SW_Boat_Outboard_Loop    fueraborda de dos tiempos al ralentí-medio con chapoteo
  Ambience/Zones/SW_AmbZ_Siren_Loop sirena de alarma industrial (sube y baja), 3D
  Ambience/Zones/SW_AmbZ_Refinery_Loop  zumbido grave de la planta, vapor y golpes metálicos lejanos
  Ambience/Zones/SW_AmbZ_Flare_Loop rugido de la antorcha (combustión, turbulencia)
  UI/SW_UI_Shutter                  obturador de cámara réflex (espejo + cortinilla)
"""
import math
import os
import random
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import audiolib as A
from process_sfx import SR, burst, mix_into, mk, noise, seconds, tone
from gen_world_audio import loopify, smooth_noise

TAU = 2 * math.pi


def drone_buzz(rng, dur=6.0):
    n = seconds(dur + 1.0)
    out = [0.0] * n
    mod = smooth_noise(n, rng, 0.8)
    # 4 rotores a ~180-195 Hz de paso de pala: el batido entre ellos da el "zumbido" característico
    for f0 in (182.0, 186.5, 191.0, 194.0):
        ph = rng.uniform(0, TAU)
        for i in range(n):
            ph += TAU * f0 * (1 + 0.015 * mod[i]) / SR
            s = math.sin(ph)
            out[i] += (0.5 * s + 0.25 * math.sin(2 * ph) + 0.15 * math.sin(3 * ph) + 0.08 * (1 if s > 0 else -1)) * 0.25
    # Aire de las hélices
    air = A.biquad(noise(n, rng), SR, "bp", 1800, 0.7)
    for i in range(n):
        out[i] += air[i] * 0.12
    return loopify([A.compress(out, SR, -16, 3, 5, 120, 0)], 1.0)[0]


def drone_alert(rng):
    out = [0.0] * seconds(0.6)
    for t0 in (0.0, 0.2):
        b = tone(2350, 0.12, 0.08)
        mix_into(out, [x * min(1.0, i / (0.004 * SR)) for i, x in enumerate(b)], seconds(t0), 0.6)
        mix_into(out, tone(4700, 0.12, 0.05), seconds(t0), 0.15)
    return out


def outboard(rng, dur=6.0):
    n = seconds(dur + 1.5)
    out = [0.0] * n
    pulse = A.lowpass(noise(seconds(0.015), rng), SR, 900)
    i = 0
    while i < n:
        g = 0.6 + 0.4 * rng.random()
        mix_into(out, [x * g * math.exp(-k / (0.004 * SR)) for k, x in enumerate(pulse)], i)
        i += int(SR / (95.0 * rng.uniform(0.97, 1.03)))
    out = A.saturate(A.lowpass(out[:n], SR, 1400), 1.6)
    # Chapoteo del casco contra el agua
    water = A.lowpass(noise(n, rng), SR, 600)
    slap = smooth_noise(n, rng, 1.6)
    for k in range(n):
        out[k] += water[k] * (0.15 + 0.35 * slap[k] ** 3)
    return loopify([A.compress(out, SR, -16, 3, 5, 120, 0)], 1.5)[0]


def siren(rng, dur=8.0):
    # Sirena de motor: sube de 380 a 760 Hz en 2 s y baja, con armónicos impares (sonido "de bocina")
    n = seconds(dur + 1.0)
    out = []
    ph = 0.0
    for i in range(n):
        t = i / SR
        c = (t % 4.0) / 4.0
        f = 380 + 380 * (math.sin(math.pi * c) ** 0.8)
        ph += TAU * f / SR
        s = math.sin(ph) + 0.33 * math.sin(3 * ph) + 0.2 * math.sin(5 * ph)
        out.append(s * 0.4)
    out = A.biquad(out, SR, "peak", 1200, 1.0, 4.0)
    # Eco de la planta (grandes superficies metálicas)
    late = A.lowpass(out, SR, 1500)
    for d, g in ((0.18, 0.35), (0.42, 0.22), (0.8, 0.12)):
        mix_into(out, late, seconds(d), g)
    return loopify([out[:n]], 1.0)[0]


def refinery(rng, dur=30.0):
    n = seconds(dur + 2.0)
    out = [0.0] * n
    mod = smooth_noise(n, rng, 0.15)
    # Zumbido eléctrico de 50 Hz y sus armónicos + rumor de bombas
    ph = 0.0
    for i in range(n):
        ph += TAU * 50 / SR
        out[i] = (0.25 * math.sin(ph) + 0.12 * math.sin(2 * ph) + 0.06 * math.sin(3 * ph)) * (0.8 + 0.2 * mod[i])
    rumble = A.lowpass(noise(n, rng), SR, 120)
    hiss = A.biquad(noise(n, rng), SR, "hp", 3000, 0.7)
    hmod = smooth_noise(n, rng, 0.08)
    for i in range(n):
        out[i] += rumble[i] * 0.9 + hiss[i] * 0.05 * hmod[i] ** 2
    # Golpes metálicos lejanos (dilatación de tuberías) y escapes de vapor
    for _ in range(int(dur / 6)):
        t = rng.randrange(n - seconds(2))
        clank = [x * 0.35 for x in tone(rng.uniform(300, 700), 1.2, 0.35)]
        mix_into(out, A.lowpass(clank, SR, 1500), t)
    for _ in range(int(dur / 10)):
        t = rng.randrange(n - seconds(3))
        steam = A.biquad(noise(seconds(2.5), rng), SR, "bp", 4000, 0.5)
        mix_into(out, [x * math.sin(math.pi * k / len(steam)) * 0.25 for k, x in enumerate(steam)], t)
    return loopify([out], 2.0)[0]


def flare(rng, dur=12.0):
    n = seconds(dur + 1.5)
    roar = A.lowpass(noise(n, rng), SR, 250)
    mid = A.biquad(noise(n, rng), SR, "bp", 700, 0.6)
    mod = smooth_noise(n, rng, 2.0)
    out = [roar[i] * 1.6 * (0.7 + 0.5 * mod[i]) + mid[i] * 0.25 * mod[i] for i in range(n)]
    return loopify([A.compress(out, SR, -18, 3, 10, 200, 0)], 1.5)[0]


def shutter(rng):
    out = [0.0] * seconds(0.25)
    mix_into(out, burst(rng, 0.02, 1500, 9000, 0.004), 0, 0.8)           # espejo que sube
    mix_into(out, burst(rng, 0.015, 2500, 12000, 0.003), seconds(0.045), 0.6)   # cortinilla
    mix_into(out, burst(rng, 0.025, 800, 6000, 0.006), seconds(0.09), 0.7)      # espejo que baja
    mix_into(out, tone(3200, 0.04, 0.01), seconds(0.09), 0.1)
    return out


def main():
    root = sys.argv[1]
    w = lambda rel, sig, peak: A.write_wav(mk(os.path.join(root, rel)), [sig], peak_db=peak)
    w("Vehicles/SW_Drone_Buzz_Loop.wav", drone_buzz(random.Random(201)), -2.0)
    w("Vehicles/SW_Drone_Alert.wav", drone_alert(random.Random(202)), -3.0)
    w("Vehicles/SW_Boat_Outboard_Loop.wav", outboard(random.Random(203)), -2.0)
    w("Ambience/Zones/SW_AmbZ_Siren_Loop.wav", siren(random.Random(204)), -1.0)
    w("Ambience/Zones/SW_AmbZ_Refinery_Loop.wav", refinery(random.Random(205)), -3.0)
    w("Ambience/Zones/SW_AmbZ_Flare_Loop.wav", flare(random.Random(206)), -2.0)
    w("UI/SW_UI_Shutter.wav", shutter(random.Random(207)), -3.0)
    print("OK")


if __name__ == "__main__":
    main()
