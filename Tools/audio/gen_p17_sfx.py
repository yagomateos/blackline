"""Sonidos de la pistola P-17 a partir de grabaciones reales CC0 (Free Firearm Sound Library) + síntesis.

Fuentes (ver Docs/Creditos_Assets.md):
  - Walther PPQ 9 mm: X_39P (cercano), X_31P (media distancia). Cuerpo extra: 1911 .45 cercano (A_42P).
  - Recarga y cerrojo: las mismas grabaciones CC0 que el AR-7 (assaultriflereload1_0, equipment_clicks3),
    más agudas y cortas (piezas más pequeñas).
  - Cargador que cae al suelo: síntesis (golpe sordo de polímero + chapa de acero).

Uso:  python -I gen_p17_sfx.py <carpeta_fuentes> <carpeta_salida>   (las mismas carpetas que process_sfx.py)
Salida: <salida>/Weapons/P17/SW_P17_*.wav (48 kHz, 16 bits).
"""
import math
import os
import random
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import audiolib as A
from process_sfx import SR, burst, load, mix_into, mk, pitch, seconds, tone


def write(out, name, chans, peak_db):
    A.write_wav(mk(os.path.join(out, "Weapons", "P17", name)), chans, peak_db=peak_db)


def fire(src, out):
    lib = os.path.join(src, "Prepared SFX Library")
    near = load(os.path.join(lib, "Walther PPQ", "X_39P.wav"))
    mid = load(os.path.join(lib, "Walther PPQ", "X_31P.wav"))
    body = load(os.path.join(lib, "1911", "A_42P.wav"))
    ons_near = A.find_onsets(A.to_mono(near)[0], SR, -20, 0.4)
    ons_mid = A.find_onsets(A.to_mono(mid)[0], SR, -20, 0.3)
    ons_body = A.find_onsets(A.to_mono(body)[0], SR, -20, 0.4)
    print(f"PPQ cercano: {len(ons_near)} disparos, medio: {len(ons_mid)}, 1911: {len(ons_body)}")
    rng = random.Random(17)
    pre = seconds(0.004)

    # Cercano (jugador, estéreo): chasquido del 9 mm con algo del cuerpo grave del .45, 8 variaciones
    for k in range(8):
        on = ons_near[k % len(ons_near)]
        close = A.slice_(near, on - pre, on + seconds(0.28))
        bon = ons_body[k % len(ons_body)]
        b = A.slice_(body, bon - pre, bon + seconds(0.07))
        f = 1.0 + rng.uniform(-0.03, 0.03)
        layered = []
        for c in range(2):
            ch = close[c][:]
            mix_into(ch, A.lowpass(b[c], SR, 1800), 0, 0.22)
            layered.append(pitch(ch, f))
        A.fade(layered, SR, 0.3, 50)
        write(out, f"SW_P17_Fire_Close_{k + 1:02d}.wav", layered, -0.5)

    # Cola (eco real del exterior)
    for k, on in enumerate(ons_near[:2]):
        tail = A.slice_(near, on + seconds(0.09), on + seconds(2.6))
        for c in range(2):
            for i in range(min(seconds(0.05), len(tail[c]))):
                tail[c][i] *= i / seconds(0.05)
        tail = A.trim_tail(tail, SR, -55)
        A.fade(tail, SR, 0, 350)
        write(out, f"SW_P17_Tail_{k + 1:02d}.wav", tail, -4.0)

    # Lejano (IA a distancia) y cercano mono espacializado
    for k, on in enumerate(ons_mid[:6]):
        d = A.to_mono(A.slice_(mid, on - seconds(0.003), on + seconds(1.0)))
        A.fade(d, SR, 0.3, 220)
        write(out, f"SW_P17_Fire_Distant_{k + 1:02d}.wav", d, -1.5)
    for k in range(4):
        on = ons_near[k % len(ons_near)]
        m = A.to_mono(A.slice_(near, on - pre, on + seconds(0.45)))
        m = [pitch(m[0], 1.0 + rng.uniform(-0.03, 0.03))]
        A.fade(m, SR, 0.3, 110)
        write(out, f"SW_P17_Fire_Close3D_{k + 1:02d}.wav", m, -0.5)


def mechanics(src, out):
    rel = load(os.path.join(src, "assaultriflereload1_0.wav"))
    clicks = load(os.path.join(src, "equipment_clicks3.wav"))
    ons = A.find_onsets(A.to_mono(clicks)[0], SR, -20, 0.06)

    def cut(chs, a, b, name, factor, fade_out=40, peak=-2.0):
        s = A.to_mono(A.slice_(chs, seconds(a), seconds(b)))
        s = [pitch(s[0], factor)]
        s = A.trim_tail(s, SR, -50)
        A.fade(s, SR, 0.5, fade_out)
        write(out, name, s, peak)

    # Botón y cargador que sale / entra (más pequeño y ligero que el del fusil)
    cut(rel, 0.20, 0.62, "SW_P17_MagOut.wav", 1.35, 60)
    cut(rel, 1.00, 1.42, "SW_P17_MagIn.wav", 1.3, 50)
    # Corredera: tirar y soltar (golpe seco de acero)
    t0, t1 = ons[0] / SR, ons[1] / SR
    cut(clicks, t0 - 0.01, t1 - 0.01, "SW_P17_SlidePull.wav", 1.4, 30)
    cut(clicks, t1 - 0.01, t1 + 0.25, "SW_P17_SlideRelease.wav", 1.25, 50)
    # Percutor en vacío
    t = ons[5] / SR
    cut(clicks, t - 0.005, t + 0.07, "SW_P17_DryFire.wav", 1.25, 15, -7.0)


def mag_drop(rng):
    """Cargador vacío contra el suelo: golpe de la base de polímero, chapa de acero y un rebote."""
    out = [0.0] * seconds(0.7)
    for t, g in ((0.0, 1.0), (rng.uniform(0.11, 0.15), 0.45), (rng.uniform(0.2, 0.26), 0.18)):
        hit = burst(rng, 0.06, 120, 1800, 0.012)
        for k, f in enumerate((1750, 2900, 4650, 6900)):
            mix_into(hit, tone(f * rng.uniform(0.95, 1.05), 0.3, 0.05 / (k + 1), rng.uniform(0, 6)), 0, 0.3 / (k + 1))
        mix_into(hit, burst(rng, 0.012, 2500, 12000, 0.003), 0, 0.35)
        mix_into(out, hit, seconds(t), g)
    return [A.trim_tail([out], SR, -55)[0]]


def main():
    src, out = sys.argv[1], sys.argv[2]
    fire(src, out)
    mechanics(src, out)
    for k in range(2):
        write(out, f"SW_P17_MagDrop_{k + 1:02d}.wav", mag_drop(random.Random(170 + k)), -2.0)
    print("OK")


if __name__ == "__main__":
    main()
