"""Audio del mundo (Bloque 7): balas que pasan cerca, ambientes por zona, combate lejano y música.

Uso:  python -I gen_world_audio.py <carpeta_fuentes> <carpeta_salida SFX>
  carpeta_fuentes = la de process_sfx.py (contiene "Prepared SFX Library/", grabaciones CC0).
Salida (48 kHz, 16 bits):
  Weapons/Bullet/SW_Bullet_Crack_XX   chasquido supersónico (bala que pasa a < 3 m) + reflejo en el suelo
  Weapons/Bullet/SW_Bullet_Whiz_XX    silbido de bala con efecto Doppler
  Ambience/Zones/SW_AmbZ_*_Loop       bucles mono para emisores 3D: agua del muelle, fuego, zumbido de farola,
                                      viento en callejón, tono de sala
  Ambience/Distant/SW_Dist_*          sueltos para el "combate lejano": ráfagas reales filtradas con eco,
                                      explosiones, sirena; chirridos metálicos de contenedor
  Music/SW_Mus_Combat_Loop            bucle de combate (100 ppm, 8 compases, re menor), estéreo
  Music/SW_Mus_Stinger_Contact        golpe al empezar el combate
  Music/SW_Mus_Stinger_Complete       cierre de misión
"""
import math
import os
import random
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import audiolib as A
from process_sfx import SR, burst, grains, load, mix_into, mk, noise, seconds, tone

TAU = 2 * math.pi


def loopify(chans, xf_s=1.5):
    """Bucle sin costura: fundido cruzado del final con el principio."""
    xf = seconds(xf_s)
    out = []
    for ch in chans:
        n = len(ch)
        c = ch[:n - xf]
        for i in range(xf):
            a = i / xf
            c[i] = ch[i] * a + ch[n - xf + i] * (1 - a)
        out.append(c)
    return out


def echo(ch, taps):
    """taps: [(retardo_s, ganancia, corte_lp)] -> reflexiones (fachadas lejanas)."""
    out = ch[:] + [0.0] * seconds(max(t for t, _, _ in taps) + 0.1)
    for t, g, lp in taps:
        mix_into(out, A.lowpass(ch, SR, lp), seconds(t), g)
    return out


def smooth_noise(n, rng, rate_hz):
    """Ruido de baja frecuencia (para modular): interpolación de valores aleatorios."""
    step = max(1, int(SR / rate_hz))
    pts = [rng.uniform(0, 1) for _ in range(n // step + 3)]
    out = []
    for i in range(n):
        k, f = divmod(i, step)
        f /= step
        f = f * f * (3 - 2 * f)
        out.append(pts[k] * (1 - f) + pts[k + 1] * f)
    return out


# ---------------------------------------------------------------------------
# Balas
# ---------------------------------------------------------------------------

def bullet_crack(rng):
    """Onda N de la bala supersónica: subida instantánea, rampa, segunda subida; luego reflejo del suelo y cola."""
    n_len = rng.uniform(0.00035, 0.0005)
    n = seconds(0.35)
    out = [0.0] * n
    nw = int(n_len * SR) or 1
    for i in range(nw + 1):
        out[i + 10] = 1.0 - 2.0 * i / nw
    s = A.biquad(out, SR, "hp", 600, 0.7)
    mix_into(s, burst(rng, 0.02, 2500, 15000, 0.003, 0.0001), 10, 0.5)       # "chasquido" de aire
    ref = s[:seconds(0.02)]
    mix_into(s, A.lowpass(ref, SR, 5000), seconds(rng.uniform(0.006, 0.012)), 0.45)  # reflejo en el suelo
    tail = burst(rng, 0.3, 800, 6000, 0.07, 0.004)                             # eco corto del entorno
    mix_into(s, tail, seconds(0.01), 0.12)
    return s


def bullet_whiz(rng):
    """Silbido: ruido de banda estrecha con barrido Doppler descendente y forma de campana."""
    dur = rng.uniform(0.28, 0.4)
    n = seconds(dur)
    src = noise(n, rng)
    f0, f1 = rng.uniform(2600, 3400), rng.uniform(900, 1300)
    out, z1, z2 = [], 0.0, 0.0
    # Resonador de 2 polos con frecuencia variable
    for i, x in enumerate(src):
        t = i / n
        f = f0 * (f1 / f0) ** (1 / (1 + math.exp(-(t - 0.45) * 14)))
        r = 0.985
        c = 2 * r * math.cos(TAU * f / SR)
        y = x * (1 - r) + c * z1 - r * r * z2
        z2, z1 = z1, y
        env = math.exp(-((t - 0.42) / 0.16) ** 2)
        out.append(y * env)
    return out


# ---------------------------------------------------------------------------
# Ambientes por zona (mono, para emisores 3D)
# ---------------------------------------------------------------------------

def amb_water(rng, dur=24.0):
    """Agua contra el muelle: chapoteos graves irregulares + rumor de oleaje."""
    n = seconds(dur)
    swell = smooth_noise(n, rng, 0.25)
    base = A.lowpass(A.lowpass(noise(n, rng), SR, 500), SR, 700)
    out = [b * (0.25 + 0.75 * s) * 0.6 for b, s in zip(base, swell)]
    t = 0.2
    while t < dur - 1.2:
        lap = A.biquad(burst(rng, rng.uniform(0.4, 0.9), 150, 1600, rng.uniform(0.12, 0.3), 0.04), SR, "peak", 450, 1.0, 6)
        mix_into(out, lap, seconds(t), rng.uniform(0.4, 1.0))
        # gotas/burbujas
        for _ in range(rng.randint(2, 6)):
            mix_into(out, tone(rng.uniform(700, 1800), 0.05, 0.012, 0, rng.uniform(1.2, 1.8)), seconds(t + rng.uniform(0.05, 0.5)), rng.uniform(0.05, 0.15))
        t += rng.uniform(0.6, 2.2)
    return loopify([out], 2.0)[0]


def amb_fire(rng, dur=20.0):
    """Coche ardiendo: rugido grave + chisporroteo + chasquidos."""
    n = seconds(dur)
    roar_mod = smooth_noise(n, rng, 3.0)
    roar = A.lowpass(A.lowpass(noise(n, rng), SR, 250), SR, 400)
    out = [r * (0.5 + 0.5 * m) * 1.4 for r, m in zip(roar, roar_mod)]
    mix_into(out, grains(rng, dur, int(dur * 70), 1500, 9000, (0.001, 0.004), dur / 2.5, 0.35))
    t = 0.0
    while t < dur - 0.3:
        mix_into(out, burst(rng, 0.03, 800, 7000, 0.006, 0.0002), seconds(t), rng.uniform(0.3, 0.9))
        t += rng.expovariate(1.5)
    return loopify([out], 1.5)[0]


def amb_hum(rng, dur=10.0):
    """Farola de sodio: zumbido de 100 Hz con armónicos y algo de chisporroteo de la reactancia."""
    n = seconds(dur)
    wob = smooth_noise(n, rng, 0.5)
    out = []
    for i in range(n):
        t = i / SR
        s = 0.0
        for h, g in ((1, 1.0), (2, 0.5), (3, 0.35), (5, 0.18), (7, 0.08)):
            s += g * math.sin(TAU * 100 * h * t + h * 0.7)
        out.append(s * (0.85 + 0.15 * wob[i]))
    out = A.biquad(out, SR, "hp", 80, 0.7)
    mix_into(out, grains(rng, dur, int(dur * 6), 3000, 9000, (0.001, 0.002), dur / 2, 0.08))
    return loopify([out], 1.0)[0]


def amb_wind_alley(rng, dur=24.0):
    """Viento encañonado en un callejón: soplo con ráfagas y silbido resonante que cambia de tono."""
    n = seconds(dur)
    gust = smooth_noise(n, rng, 0.18)
    base = A.biquad(A.lowpass(noise(n, rng), SR, 1800), SR, "hp", 120, 0.7)
    src = noise(n, rng)
    whistle, z1, z2 = [], 0.0, 0.0
    pitch_mod = smooth_noise(n, rng, 0.3)
    for i, x in enumerate(src):
        f = 520 + 260 * pitch_mod[i]
        r = 0.9975
        c = 2 * r * math.cos(TAU * f / SR)
        y = x * (1 - r) + c * z1 - r * r * z2
        z2, z1 = z1, y
        whistle.append(y)
    out = [(b * 0.8 + w * 1.6 * g * g) * (0.3 + 0.7 * g) for b, w, g in zip(base, whistle, gust)]
    return loopify([out], 2.5)[0]


def amb_room(rng, dur=20.0):
    """Tono de sala del local: aire grave, nevera/zumbido lejano y algún goteo."""
    n = seconds(dur)
    out = [x * 0.5 for x in A.lowpass(A.lowpass(noise(n, rng), SR, 180), SR, 260)]
    for i in range(n):
        out[i] += 0.04 * math.sin(TAU * 50 * i / SR) + 0.02 * math.sin(TAU * 150 * i / SR)
    t = rng.uniform(0.5, 2.0)
    while t < dur - 0.5:
        mix_into(out, tone(rng.uniform(1400, 2200), 0.12, 0.02, 0, 1.5), seconds(t), 0.12)
        t += rng.uniform(2.0, 5.0)
    return loopify([out], 1.5)[0]


# ---------------------------------------------------------------------------
# Combate lejano y sueltos
# ---------------------------------------------------------------------------

def far_burst(m, rng):
    """Ráfaga real a 300-800 m: corte de una grabación, sin agudos, con ecos de la ciudad."""
    start = rng.randint(0, max(1, len(m) - seconds(2.5)))
    seg = m[start:start + seconds(rng.uniform(1.0, 2.2))]
    seg = A.lowpass(A.lowpass(A.lowpass(seg, SR, 1100), SR, 1100), SR, 1400)
    seg = [x * math.sin(math.pi * i / len(seg)) ** 0.25 for i, x in enumerate(seg)]
    return echo(seg, [(rng.uniform(0.18, 0.3), 0.45, 700), (rng.uniform(0.45, 0.7), 0.3, 500), (rng.uniform(0.9, 1.3), 0.18, 400)])


def far_explosion(rng):
    """Explosión lejana: golpe grave (sin agudos por la distancia) + retumbo largo + ecos."""
    n = seconds(5.0)
    boom = A.lowpass(A.lowpass(noise(n, rng), SR, 120), SR, 160)
    out = [x * (min(1.0, (i / SR) / 0.008)) * math.exp(-(i / SR) / 0.9) * 2.0 for i, x in enumerate(boom)]
    mix_into(out, tone(rng.uniform(32, 45), 1.5, 0.45, 0, 0.7), 0, 0.6)
    rumble = A.lowpass(noise(n, rng), SR, 300)
    mix_into(out, [x * math.exp(-(i / SR) / 1.8) * 0.35 for i, x in enumerate(rumble)], seconds(0.15))
    out = A.biquad(A.biquad(out, SR, "lp", 450, 0.7), SR, "lp", 600, 0.7)
    return echo(out, [(rng.uniform(0.3, 0.5), 0.4, 200), (rng.uniform(0.8, 1.2), 0.25, 150)])


def far_siren(rng, dur=18.0):
    """Sirena antiaérea lejana: subida, meseta ondulante y bajada, muy filtrada con eco."""
    n = seconds(dur)
    out, ph = [], 0.0
    for i in range(n):
        t = i / SR
        f = 260 + 360 * min(1.0, t / 4.0) * min(1.0, (dur - t) / 5.0) + 15 * math.sin(TAU * 0.25 * t)
        ph += TAU * f / SR
        s = math.sin(ph) + 0.4 * math.sin(2 * ph) + 0.2 * math.sin(3 * ph)
        out.append(s * min(1.0, t / 1.5) * min(1.0, (dur - t) / 2.0))
    out = A.lowpass(A.lowpass(out, SR, 900), SR, 1200)
    return echo(out, [(0.35, 0.4, 700), (0.8, 0.25, 500)])


def metal_creak(rng):
    """Chirrido de contenedor/chapa: fricción (stick-slip) que excita resonancias metálicas."""
    dur = rng.uniform(1.0, 2.0)
    n = seconds(dur)
    exc = [0.0] * n
    t, rate = 0.0, rng.uniform(25, 60)
    while t < dur - 0.05:
        exc[int(t * SR)] = rng.uniform(0.5, 1.0) * math.sin(math.pi * t / dur)
        t += 1.0 / (rate * rng.uniform(0.7, 1.3))
        rate *= rng.uniform(0.99, 1.012)
    out = [0.0] * n
    f0 = rng.uniform(180, 320)
    for r, g, q in ((1.0, 1.0, 30), (2.13, 0.6, 40), (3.4, 0.4, 50), (5.2, 0.25, 60)):
        mix_into(out, A.biquad(exc, SR, "bp", f0 * r, q), 0, g * 6)
    return out


# ---------------------------------------------------------------------------
# Música (síntesis propia): "pulso táctico" en re menor, 100 ppm
# ---------------------------------------------------------------------------

BPM = 100.0
BEAT = 60.0 / BPM
NOTE = {"D1": 36.71, "A1": 55.0, "Bb1": 58.27, "C2": 65.41, "D2": 73.42, "E2": 82.41, "F2": 87.31, "G2": 98.0,
        "A2": 110.0, "Bb2": 116.54, "C3": 130.81, "D3": 146.83, "E3": 164.81, "F3": 174.61, "G3": 196.0, "A3": 220.0,
        "Bb3": 233.08, "C4": 261.63, "D4": 293.66, "E4": 329.63, "F4": 349.23, "G4": 392.0, "A4": 440.0, "Cs4": 277.18}


def saw_voice(freq, dur, rng, detune=(0.0,), cutoff=1200.0, attack=0.01, release=0.08, sustain=1.0):
    """Diente de sierra con varias voces desafinadas, filtro y envolvente (cuerdas/sinte)."""
    n = seconds(dur)
    out = [0.0] * n
    for d in detune:
        f = freq * (2 ** (d / 1200))
        ph = rng.uniform(0, 1)
        inc = f / SR
        for i in range(n):
            ph += inc
            if ph >= 1.0:
                ph -= 1.0
            out[i] += 2 * ph - 1
    out = A.biquad(A.biquad(out, SR, "lp", cutoff, 0.9), SR, "lp", cutoff * 1.3, 0.7)
    a, r = seconds(attack), seconds(release)
    g = 1.0 / len(detune)
    for i in range(n):
        e = min(1.0, i / a if a else 1.0) * (min(1.0, (n - i) / r) if r else 1.0) * sustain
        out[i] *= e * g
    return out


def taiko(rng, pitch=1.0):
    n = seconds(1.2)
    out, ph = [], 0.0
    for i in range(n):
        t = i / SR
        f = (55 + 70 * math.exp(-t / 0.03)) * pitch
        ph += TAU * f / SR
        out.append(math.sin(ph) * math.exp(-t / 0.28))
    mix_into(out, burst(rng, 0.08, 120, 2500, 0.02, 0.0005), 0, 0.5)
    return out


def tick(rng):
    return burst(rng, 0.04, 5000, 14000, 0.006, 0.0002)


def rim(rng):
    s = burst(rng, 0.08, 900, 6000, 0.015, 0.0002)
    mix_into(s, tone(820, 0.05, 0.01), 0, 0.3)
    return s


def music_combat(rng):
    bars = 8
    bar = BEAT * 4
    total = bars * bar
    n = seconds(total + 2.0)
    L, R = [0.0] * n, [0.0] * n

    def put(sig, t, g, pan=0.5):
        mix_into(L, sig, seconds(t), g * math.cos(pan * math.pi / 2) * 1.41)
        mix_into(R, sig, seconds(t), g * math.sin(pan * math.pi / 2) * 1.41)

    # Armonía (2 compases por acorde): Dm - Bb - Gm - A
    chords = [("D2", ["D3", "F3", "A3"]), ("Bb1", ["Bb2", "D3", "F3"]), ("G2", ["G2", "Bb2", "D3"]), ("A1", ["A2", "Cs4", "E3"])]
    for c, (root, pad) in enumerate(chords):
        t0 = c * 2 * bar
        # Pad de cuerdas: entra suave, sin ataque
        for k, nn in enumerate(pad):
            put(saw_voice(NOTE[nn], 2 * bar + 0.6, rng, (-9, 0, 8), 900, 0.9, 0.6, 1.0), t0, 0.16, 0.25 + 0.25 * k)
        # Bajo ostinato en semicorcheas con acento (patrón 3-3-2)
        for s in range(32):
            accent = s % 8 in (0, 3, 6)
            note = NOTE[root] * (2 if s % 16 in (14, 15) else 1)
            v = saw_voice(note, BEAT / 4 * 0.9, rng, (-6, 6), 380 if accent else 260, 0.002, 0.04)
            put(v, t0 + s * BEAT / 4, 0.55 if accent else 0.35, 0.5)
        # Drone grave
        put([x * 0.8 for x in tone(NOTE["D1"], 2 * bar + 0.5, 99, 0)], t0, 0.25, 0.5)
    # Percusión
    for b in range(bars):
        t0 = b * bar
        for beat_pos, g in ((0, 1.0), (1.5, 0.6), (2, 0.85), (3.5, 0.5) if b % 2 else (3, 0.7)):
            put(taiko(rng, rng.uniform(0.97, 1.03)), t0 + beat_pos * BEAT, 0.9 * g, rng.uniform(0.4, 0.6))
        if b % 4 == 3:  # redoble de cierre de frase
            for k in range(4):
                put(taiko(rng, 1.25), t0 + (3 + k * 0.25) * BEAT, 0.35 + 0.1 * k, 0.3 + 0.1 * k)
        for s in range(16):
            put(tick(rng), t0 + s * BEAT / 4, 0.12 if s % 2 else 0.2, 0.7)
        for beat_pos in (1, 3):
            put(rim(rng), t0 + beat_pos * BEAT, 0.25, 0.35)
    # Bucle: lo que sobresale tras el último compás vuelve al principio
    end = seconds(total)
    for ch in (L, R):
        for i in range(end, len(ch)):
            ch[i - end] += ch[i]
        del ch[end:]
    return [L, R]


def stinger_contact(rng):
    n = seconds(4.0)
    L, R = [0.0] * n, [0.0] * n
    for ch in (L, R):
        mix_into(ch, taiko(rng, 0.8), 0, 1.0)
        mix_into(ch, taiko(rng, 0.8), seconds(0.12), 0.6)
    swell = saw_voice(NOTE["D2"], 3.5, rng, (-12, 0, 11), 500, 1.6, 1.2)
    mix_into(swell, saw_voice(NOTE["A2"], 3.5, rng, (-10, 9), 600, 1.6, 1.2), 0, 0.7)
    mix_into(L, swell, 0, 0.5)
    mix_into(R, swell[::1], seconds(0.01), 0.5)
    return [L, R]


def stinger_complete(rng):
    n = seconds(7.0)
    L, R = [0.0] * n, [0.0] * n
    seq = [(0.0, ["D3", "F3", "A3"]), (1.8, ["Bb2", "D3", "F3"]), (3.6, ["D3", "F3", "A3", "D4"])]
    for t, notes in seq:
        for k, nn in enumerate(notes):
            v = saw_voice(NOTE[nn], 3.2, rng, (-8, 0, 7), 1100, 0.5, 1.5)
            mix_into(L, v, seconds(t), 0.25 * (1 - k * 0.15))
            mix_into(R, v, seconds(t + 0.007), 0.25 * (0.6 + k * 0.15))
    for ch in (L, R):
        mix_into(ch, taiko(rng, 0.85), seconds(3.6), 0.6)
        mix_into(ch, tone(NOTE["D1"], 3.4, 1.2), seconds(3.6), 0.4)
    return [L, R]


# ---------------------------------------------------------------------------

def main():
    src, out = sys.argv[1], sys.argv[2]

    def w(rel, chans, peak=-1.0):
        A.write_wav(mk(os.path.join(out, rel)), chans, peak_db=peak)

    for k in range(6):
        w(f"Weapons/Bullet/SW_Bullet_Crack_{k + 1:02d}.wav", [bullet_crack(random.Random(700 + k))], -1.0)
    for k in range(4):
        w(f"Weapons/Bullet/SW_Bullet_Whiz_{k + 1:02d}.wav", [bullet_whiz(random.Random(720 + k))], -3.0)
    print("balas")

    w("Ambience/Zones/SW_AmbZ_Water_Loop.wav", [amb_water(random.Random(11))], -6.0)
    w("Ambience/Zones/SW_AmbZ_Fire_Loop.wav", [amb_fire(random.Random(12))], -6.0)
    w("Ambience/Zones/SW_AmbZ_Hum_Loop.wav", [amb_hum(random.Random(13))], -10.0)
    w("Ambience/Zones/SW_AmbZ_WindAlley_Loop.wav", [amb_wind_alley(random.Random(14))], -6.0)
    w("Ambience/Zones/SW_AmbZ_Room_Loop.wav", [amb_room(random.Random(15))], -10.0)
    print("ambientes")

    lib = os.path.join(src, "Prepared SFX Library")
    recs = [A.to_mono(load(os.path.join(lib, d, f)))[0] for d, f in
            (("AK-47", "C_36P.wav"), ("AK-47", "C_31P.wav"), ("PPSh", "P_22P.wav"), ("AK-47", "C_29P.wav"))]
    for k in range(8):
        rng = random.Random(800 + k)
        w(f"Ambience/Distant/SW_Dist_Burst_{k + 1:02d}.wav", [far_burst(recs[k % len(recs)], rng)], -3.0)
    for k in range(4):
        w(f"Ambience/Distant/SW_Dist_Explosion_{k + 1:02d}.wav", [far_explosion(random.Random(820 + k))], -1.0)
    w("Ambience/Distant/SW_Dist_Siren_01.wav", [far_siren(random.Random(830))], -4.0)
    for k in range(4):
        w(f"Ambience/Distant/SW_Creak_Metal_{k + 1:02d}.wav", [metal_creak(random.Random(840 + k))], -3.0)
    print("lejanos")

    w("Music/SW_Mus_Combat_Loop.wav", music_combat(random.Random(900)), -3.0)
    w("Music/SW_Mus_Stinger_Contact.wav", stinger_contact(random.Random(901)), -2.0)
    w("Music/SW_Mus_Stinger_Complete.wav", stinger_complete(random.Random(902)), -3.0)
    print("OK")


if __name__ == "__main__":
    main()
