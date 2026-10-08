"""Sonidos del menú (Bloque 10): moverse, aceptar, volver, ajustar un valor. Síntesis propia.

Uso:  python -I gen_ui_sfx.py <carpeta_salida SFX>   -> UI/SW_UI_Menu{Move,Select,Back,Tick}.wav
"""
import math
import os
import random
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import audiolib as A
from process_sfx import SR, burst, mix_into, mk, seconds, tone


def move(rng):
    s = tone(1850, 0.05, 0.012)
    mix_into(s, burst(rng, 0.02, 3000, 9000, 0.003), 0, 0.25)
    return s


def select(rng):
    """Confirmación: golpe grave + dos tonos ascendentes de terminal + cola metálica."""
    s = [0.0] * seconds(0.7)
    mix_into(s, tone(70, 0.5, 0.12, 0, 0.6), 0, 0.8)
    mix_into(s, burst(rng, 0.1, 80, 1500, 0.03), 0, 0.5)
    mix_into(s, tone(880, 0.12, 0.05), seconds(0.02), 0.35)
    mix_into(s, tone(1320, 0.18, 0.07), seconds(0.09), 0.3)
    mix_into(s, A.biquad(burst(rng, 0.5, 2000, 8000, 0.15, 0.01), SR, "bp", 3500, 2.0), seconds(0.02), 0.15)
    return s


def back(rng):
    s = tone(1100, 0.1, 0.04, 0, 0.7)
    mix_into(s, tone(700, 0.12, 0.05), seconds(0.05), 0.6)
    return s


def tick(rng):
    return tone(2600, 0.025, 0.005)


def main():
    out = sys.argv[1]
    rng = random.Random(77)
    for name, fn, peak in (("Move", move, -9.0), ("Select", select, -4.0), ("Back", back, -8.0), ("Tick", tick, -12.0)):
        A.write_wav(mk(os.path.join(out, "UI", f"SW_UI_Menu{name}.wav")), [fn(rng)], peak_db=peak)
    print("OK")


if __name__ == "__main__":
    main()
