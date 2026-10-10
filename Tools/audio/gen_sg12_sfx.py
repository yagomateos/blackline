"""Sonidos de la escopeta SG-12 "Mastín" a partir de grabaciones reales CC0 + síntesis.

Fuentes (ver Docs/Creditos_Assets.md):
  - Free Firearm Sound Library: Benelli Nova (O_21P cercano, O_17P medio), Winchester Model 12 (K_22P cercano,
    K_17P medio) y Charles Daly (H_21P cercano, H_16P medio): las tres de corredera, calibre 12.
  - Bombeo: shotguncock.wav (SpringySpringo, "gun reload sounds", CC0), partido en el tirón atrás y el empuje adelante.
  - Cartucho al depósito y gatillo en vacío: equipment_clicks3 (LFA, CC0) más grave + roce sintetizado.

Uso:  python -I gen_sg12_sfx.py <carpeta_fuentes> <carpeta_salida>   (las mismas carpetas que process_sfx.py)
Salida: <salida>/Weapons/SG12/SW_SG12_*.wav (48 kHz, 16 bits).
"""
import os
import random
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import audiolib as A
from process_sfx import SR, burst, load, mix_into, mk, pitch, seconds


def write(out, name, chans, peak_db):
    A.write_wav(mk(os.path.join(out, "Weapons", "SG12", name)), chans, peak_db=peak_db)


def shots(path, threshold=-20):
    chans = load(path)
    return chans, A.find_onsets(A.to_mono(chans)[0], SR, threshold, 0.4)


def fire(src, out):
    lib = os.path.join(src, "Prepared SFX Library")
    near = [shots(os.path.join(lib, d, f)) for d, f in (("Nova", "O_21P.wav"), ("Model 12", "K_22P.wav"), ("CD", "H_21P.wav"))]
    mid = [shots(os.path.join(lib, d, f)) for d, f in (("Nova", "O_17P.wav"), ("Model 12", "K_17P.wav"), ("CD", "H_16P.wav"))]
    rng = random.Random(12)
    pre = seconds(0.004)

    # Cercano (jugador, estéreo): el estampido de una escopeta y el grave de otra; 8 variaciones
    k = 0
    for main in range(3):
        for body in range(3):
            if main == body or k >= 8:
                continue
            for shot in range(2 if k < 6 else 1):
                if k >= 8:
                    break
                chans, ons = near[main]
                bchans, bons = near[body]
                on, bon = ons[shot % len(ons)], bons[(shot + 1) % len(bons)]
                close = A.slice_(chans, on - pre, on + seconds(0.5))
                b = A.slice_(bchans, bon - pre, bon + seconds(0.12))
                f = 1.0 + rng.uniform(-0.025, 0.025)
                layered = []
                for c in range(2):
                    ch = close[c][:]
                    mix_into(ch, A.lowpass(b[c], SR, 900), 0, 0.35)
                    layered.append(pitch(ch, f))
                A.fade(layered, SR, 0.3, 80)
                write(out, f"SW_SG12_Fire_Close_{k + 1:02d}.wav", layered, -0.3)
                k += 1

    # Cola: eco real (la escopeta retumba más que el fusil)
    for k, (chans, ons) in enumerate(near[:2]):
        tail = A.slice_(chans, ons[0] + seconds(0.12), ons[0] + seconds(2.9))
        for c in range(2):
            for i in range(min(seconds(0.06), len(tail[c]))):
                tail[c][i] *= i / seconds(0.06)
        tail = A.trim_tail(tail, SR, -55)
        A.fade(tail, SR, 0, 400)
        write(out, f"SW_SG12_Tail_{k + 1:02d}.wav", tail, -3.0)

    # Lejano (IA a distancia) y cercano mono espacializado
    k = 0
    for chans, ons in mid:
        for on in ons[:2]:
            d = A.to_mono(A.slice_(chans, on - seconds(0.003), on + seconds(1.2)))
            A.fade(d, SR, 0.3, 260)
            write(out, f"SW_SG12_Fire_Distant_{k + 1:02d}.wav", d, -1.0)
            k += 1
    for k in range(4):
        chans, ons = near[k % 3]
        m = A.to_mono(A.slice_(chans, ons[k // 3] - pre, ons[k // 3] + seconds(0.6)))
        m = [pitch(m[0], 1.0 + rng.uniform(-0.03, 0.03))]
        A.fade(m, SR, 0.3, 140)
        write(out, f"SW_SG12_Fire_Close3D_{k + 1:02d}.wav", m, -0.3)


def mechanics(src, out):
    def cut(chs, a, b, name, factor=1.0, fade_out=40, peak=-2.0, extra=None):
        s = A.to_mono(A.slice_(chs, seconds(a), seconds(b)))
        s = [pitch(s[0], factor)]
        if extra:
            mix_into(s[0], extra, 0, 1.0)
        s = A.trim_tail(s, SR, -55)
        A.fade(s, SR, 0.5, fade_out)
        write(out, name, s, peak)

    # Bombeo real: el tirón atrás (0,04-0,185 s) y el empuje adelante que cierra (0,185 s hasta el final)
    cock = load(os.path.join(src, "shotguncock_0.wav"))
    cut(cock, 0.035, 0.19, "SW_SG12_PumpBack.wav", 1.0, 25, -1.5)
    cut(cock, 0.185, 0.47, "SW_SG12_PumpForward.wav", 1.0, 80, -1.0)
    # Variante algo más grave para la recarga en vacío (se cierra con más decisión)
    cut(cock, 0.185, 0.47, "SW_SG12_PumpForwardHard.wav", 0.93, 90, -0.5)

    # Cartucho al depósito: clic del elevador + roce de la vaina de plástico contra el muelle
    clicks = load(os.path.join(src, "equipment_clicks3.wav"))
    ons = A.find_onsets(A.to_mono(clicks)[0], SR, -20, 0.06)
    for k in range(3):
        rng = random.Random(120 + k)
        t = ons[(2 + k * 2) % len(ons)] / SR
        rub = burst(rng, 0.09, 600, 5000, 0.04)
        rub = [x * 0.25 for x in rub]
        cut(clicks, t - 0.01, t + 0.12, f"SW_SG12_ShellIn_{k + 1:02d}.wav", 0.82 + 0.04 * k, 40, -4.0, rub)
    # Percutor en vacío (más grave que el de la pistola) y manipulación al sacar el cartucho
    t = ons[5] / SR
    cut(clicks, t - 0.005, t + 0.08, "SW_SG12_DryFire.wav", 0.9, 15, -6.0)


def main():
    src, out = sys.argv[1], sys.argv[2]
    fire(src, out)
    mechanics(src, out)
    print("OK")


if __name__ == "__main__":
    main()
