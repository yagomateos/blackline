"""Tema del menú principal (Bloque 10): pieza original de estilo militar épico, en bucle. Síntesis propia.

Re menor, 72 ppm, 16 compases (53,3 s) en cuatro secciones:
  1-4   motivo de piano sobre un colchón de cuerdas graves y coro lejano
  5-8   entra el ostinato de cuerdas y los taikos
  9-12  tutti: metales con el tema, coro abierto, percusión completa
  13-16 clímax y descenso hacia el principio (el final enlaza con el compás 1)
Coro: síntesis por formantes ("ah"). Reverb de sala grande (Schroeder: 4 peines + 2 pasatodos por canal).

Uso:  python -I gen_music_menu.py <carpeta_salida SFX>      -> Music/SW_Mus_Menu_Loop.wav (48 kHz estéreo)
"""
import math
import os
import random
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import audiolib as A
from process_sfx import SR, burst, mix_into, mk, seconds, tone
from gen_world_audio import NOTE, saw_voice, taiko

TAU = 2 * math.pi
BPM = 72.0
BEAT = 60.0 / BPM
BAR = 4 * BEAT
BARS = 16

# Notas que faltan en NOTE
NOTE.update({"D5": 587.33, "C5": 523.25, "Bb4": 466.16, "A4": 440.0, "G4": 392.0, "F4": 349.23, "E4": 329.63,
             "Cs4": 277.18, "C4": 261.63, "Bb3": 233.08, "D2": 73.42, "A2": 110.0, "G1": 49.0, "F1": 43.65})


def piano(freq, dur, rng):
    """Piano sencillo: armónicos con caída distinta, ataque de martillo y ligera desafinación."""
    n = seconds(dur)
    out = [0.0] * n
    for h, g, d in ((1, 1.0, 1.4), (2, 0.45, 0.9), (3, 0.22, 0.6), (4, 0.12, 0.4), (5, 0.06, 0.3)):
        f = freq * h * (1 + 0.0004 * h * h)
        ph = rng.uniform(0, TAU)
        for i in range(n):
            t = i / SR
            out[i] += g * math.sin(TAU * f * t + ph) * math.exp(-t / d)
    mix_into(out, burst(rng, 0.02, 1500, 6000, 0.004), 0, 0.08)
    return out


def choir(freq, dur, rng, attack=1.2, release=1.5):
    """Coro "ah": tres voces desafinadas con vibrato; cada armónico pesa según los formantes de la /a/."""
    formants = ((750, 90, 1.0), (1150, 110, 0.5), (2600, 160, 0.25))
    n = seconds(dur)
    out = [0.0] * n
    nh = int(4000 / freq)
    for v in range(3):
        f0 = freq * (2 ** (rng.uniform(-9, 9) / 1200))
        vib_ph = rng.uniform(0, TAU)
        amps = []
        for h in range(1, nh + 1):
            fh = f0 * h
            a = sum(g * math.exp(-((fh - fc) / bw) ** 2 * 0.5) for fc, bw, g in formants) + 0.02 / h
            amps.append(a)
        phs = [rng.uniform(0, TAU) for _ in amps]
        for i in range(0, n):
            t = i / SR
            vib = 1 + 0.004 * math.sin(TAU * 5.2 * t + vib_ph)
            s = 0.0
            for h, (a, p) in enumerate(zip(amps, phs), start=1):
                s += a * math.sin(TAU * f0 * h * vib * t + p)
            out[i] += s
    a_n, r_n = seconds(attack), seconds(release)
    for i in range(n):
        out[i] *= min(1.0, i / a_n) * min(1.0, (n - i) / r_n) / 3
    return out


def brass(freq, dur, rng, swell=0.6):
    """Metal (trompas): diente de sierra con el filtro abriéndose al entrar la nota."""
    n = seconds(dur)
    raw = saw_voice(freq, dur, rng, (-5, 0, 6), 6000, 0.08, 0.5)
    out, y = [], 0.0
    for i, x in enumerate(raw):
        t = i / SR
        cutoff = 300 + 2200 * min(1.0, t / swell) * (0.85 + 0.15 * math.sin(TAU * 4.8 * t))
        a = 1 - math.exp(-TAU * cutoff / SR)
        y += a * (x - y)
        out.append(y)
    return out


def reverb(ch, mix=0.28, seed=0):
    """Schroeder: 4 peines en paralelo + 2 pasatodos (cola de ~3 s)."""
    rng = random.Random(seed)
    delays = [int(SR * d * rng.uniform(0.97, 1.03)) for d in (0.0297, 0.0371, 0.0411, 0.0437)]
    fb = 0.86
    wet = [0.0] * len(ch)
    for d in delays:
        buf = [0.0] * d
        lp = 0.0
        k = 0
        for i, x in enumerate(ch):
            y = buf[k]
            lp = lp + 0.35 * (y - lp)                 # amortiguación de agudos en la cola
            buf[k] = x + lp * fb
            k = (k + 1) % d
            wet[i] += y * 0.25
    for d, g in ((int(SR * 0.005), 0.7), (int(SR * 0.0017), 0.7)):
        buf = [0.0] * d
        k = 0
        for i, x in enumerate(wet):
            b = buf[k]
            y = -g * x + b
            buf[k] = x + g * y
            k = (k + 1) % d
            wet[i] = y
    return [x * (1 - mix) + w * mix * 2.2 for x, w in zip(ch, wet)]


def main():
    out_dir = sys.argv[1]
    rng = random.Random(2031)
    total = BARS * BAR
    n = seconds(total + 4.0)
    L, R = [0.0] * n, [0.0] * n

    def put(sig, t, g, pan=0.5):
        mix_into(L, sig, seconds(t), g * math.cos(pan * math.pi / 2) * 1.41)
        mix_into(R, sig, seconds(t), g * math.sin(pan * math.pi / 2) * 1.41)

    # Armonía (un acorde por compás): i - VI - iv - V  |  i - VI - III - V ...
    prog = [("D2", ["D3", "F3", "A3"]), ("Bb1", ["Bb2", "D3", "F3"]), ("G1", ["G2", "Bb2", "D3"]), ("A1", ["A2", "Cs4", "E3"]),
            ("D2", ["D3", "F3", "A3"]), ("Bb1", ["Bb2", "D3", "F3"]), ("F1", ["F3", "A3", "C4"]), ("A1", ["A2", "Cs4", "E3"])]
    # Motivo (piano / metales): re-fa-mi-la · re-fa-sol-la (corcheas con puntillo)
    motif = [("D4", 0, 1.5), ("F4", 1.5, 0.5), ("E4", 2, 1.0), ("A3", 3, 1.0), ("D4", 4, 1.5), ("F4", 5.5, 0.5), ("G4", 6, 1.0), ("A4", 7, 1.0)]

    for b in range(BARS):
        t0 = b * BAR
        root, chord = prog[b % 8]
        sec = b // 4
        # Colchón de cuerdas graves (siempre)
        for k, nn in enumerate(chord):
            put(saw_voice(NOTE[nn], BAR + 0.8, rng, (-10, 0, 9), 700 + 400 * sec, 0.9, 0.7), t0, 0.12 + 0.03 * sec, 0.3 + 0.2 * k)
        put(tone(NOTE[root] / 2, BAR + 0.5, 99), t0, 0.22, 0.5)
        # Coro: lejano al principio, abierto en el tutti
        if sec in (0, 2, 3):
            for k, nn in enumerate(chord[:2]):
                put(choir(NOTE[nn] * 2, BAR + 1.0, rng), t0, (0.05 if sec == 0 else 0.11), 0.25 + 0.5 * k)
        # Ostinato de cuerdas (semicorcheas, acento 3-3-2)
        if sec >= 1:
            for s in range(16):
                acc = s % 8 in (0, 3, 6)
                v = saw_voice(NOTE[root] * (2 if s % 16 >= 12 and sec == 3 else 1), BEAT / 4 * 0.85, rng, (-6, 6), 500 if acc else 330, 0.003, 0.05)
                put(v, t0 + s * BEAT / 4, 0.42 if acc else 0.26, 0.55)
        # Percusión
        if sec >= 1:
            for pos, g in ((0, 1.0), (2.5, 0.55), (3, 0.7)) if sec == 1 else ((0, 1.0), (1.5, 0.6), (2, 0.85), (3, 0.6), (3.5, 0.5)):
                put(taiko(rng, rng.uniform(0.95, 1.03)), t0 + pos * BEAT, 0.8 * g, rng.uniform(0.4, 0.6))
        if b in (7, 11):     # redoble antes de cada sección
            for k in range(8):
                put(taiko(rng, 1.3), t0 + (2 + k * 0.25) * BEAT, 0.25 + 0.06 * k, 0.3 + 0.05 * k)
        # Motivo: piano en la intro y en el descenso, metales en el tutti (dos compases por frase)
        if b % 2 == 0:
            half = motif[:4] if (b // 2) % 2 == 0 else motif[4:]
            base = 0 if (b // 2) % 2 == 0 else 4
            for nn, pos, dur in half:
                tt = t0 + (pos - base) * BEAT
                if sec in (0, 3):
                    put(piano(NOTE[nn], 2.5, rng), tt, 0.32, 0.62)
                if sec in (2, 3):
                    put(brass(NOTE[nn] / 2, dur * BEAT + 0.4, rng), tt, 0.3, 0.45)
                    put(brass(NOTE[nn] / 4, dur * BEAT + 0.4, rng), tt, 0.18, 0.5)

    # Bucle: la cola que pasa del final vuelve al principio; luego reverb y otra vez
    end = seconds(total)
    for ch in (L, R):
        for i in range(end, len(ch)):
            ch[i - end] += ch[i]
        del ch[end:]
    L = reverb(L, seed=1)
    R = reverb(R, seed=2)
    L = A.biquad(L, SR, "hp", 30, 0.7)
    R = A.biquad(R, SR, "hp", 30, 0.7)
    path = mk(os.path.join(out_dir, "Music", "SW_Mus_Menu_Loop.wav"))
    A.write_wav(path, [L, R], peak_db=-2.0)
    print("OK", path)


if __name__ == "__main__":
    main()
