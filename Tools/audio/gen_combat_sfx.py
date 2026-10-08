"""Sonidos de combate sintetizados (Bloque 3): impactos en carne, hitmarker, daño recibido y latido.

Uso:  python -I gen_combat_sfx.py <carpeta_salida>      (normalmente ArtSource/Audio/SFX)
Salida (48 kHz, 16 bits):
  Impacts/Flesh/SW_Impact_Flesh_XX.wav   golpe sordo y húmedo de bala en el cuerpo (3D)
  UI/SW_UI_Hitmarker.wav, SW_UI_Hitmarker_Kill.wav   confirmación de impacto (2D, seco y corto)
  Player/SW_Player_Hurt_XX.wav           impacto recibido: golpe grave en el pecho + roce de equipo (2D)
  Player/SW_Player_Heartbeat_Loop.wav    latido a 72 ppm para salud baja (bucle)
  Radio/SW_Radio_In.wav, SW_Radio_Out.wav  apertura/cierre de radio (Bloque 6)
  UI/SW_UI_Objective.wav, SW_UI_Pickup.wav  objetivo nuevo, recoger objeto (Bloque 6)
Reutiliza los generadores de process_sfx.py (ruido filtrado, tonos con caída, granos).
"""
import math
import os
import random
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import audiolib as A
from process_sfx import SR, burst, cloth, mix_into, mk, seconds, tone


def flesh(rng):
    """Bala en el cuerpo: golpe grave amortiguado + chasquido húmedo corto + tela."""
    s = burst(rng, 0.09, 60, 900, 0.022)
    mix_into(s, tone(rng.uniform(65, 95), 0.12, 0.03, 0, 0.7), 0, 0.8)
    wet = burst(rng, 0.06, 700, 3200, 0.012, 0.002)
    # modulación rápida = textura "húmeda"
    am = rng.uniform(140, 220)
    wet = [x * (0.6 + 0.4 * math.sin(2 * math.pi * am * i / SR)) for i, x in enumerate(wet)]
    mix_into(s, wet, seconds(0.004), 0.45)
    mix_into(s, burst(rng, 0.008, 2500, 9000, 0.0015, 0.0001), 0, 0.25)   # chasquido de tela/chaleco
    return s


def hitmarker(kill=False):
    """Clic metálico seco; la baja suena más grave y doble."""
    rng = random.Random(7 if kill else 3)
    s = [0.0] * seconds(0.18 if kill else 0.08)
    freqs = (1700, 2550) if kill else (2900, 4300)
    for f in freqs:
        mix_into(s, tone(f, 0.06, 0.012), 0, 0.5)
    mix_into(s, burst(rng, 0.006, 3000, 12000, 0.001, 0.0001), 0, 0.5)
    if kill:
        mix_into(s, tone(140, 0.12, 0.04, 0, 0.6), 0, 0.6)
        for f in freqs:
            mix_into(s, tone(f * 0.94, 0.06, 0.012), seconds(0.055), 0.4)
    return s


def hurt(rng):
    """Daño recibido (primera persona): golpe en el pecho muy grave + crujido de equipo."""
    s = burst(rng, 0.25, 30, 400, 0.06)
    mix_into(s, tone(rng.uniform(45, 60), 0.3, 0.08, 0, 0.6), 0, 1.0)
    mix_into(s, cloth(rng, 0.3), seconds(0.01), 0.35)
    mix_into(s, burst(rng, 0.03, 1200, 6000, 0.006), 0, 0.2)
    return s


def heartbeat(bpm=72.0, beats=4):
    """Latido 'lub-dub' muy grave para salud baja, en bucle exacto."""
    period = 60.0 / bpm
    out = [0.0] * seconds(period * beats)
    for b in range(beats):
        t0 = seconds(b * period)
        mix_into(out, tone(52, 0.18, 0.05, 0, 0.8), t0, 1.0)
        mix_into(out, tone(44, 0.2, 0.06, 0, 0.8), t0 + seconds(0.16), 0.7)
    return A.lowpass(out, SR, 180)


def radio_click(rng, opening=True):
    """Apertura/cierre de radio: clic del pulsador + ráfaga de estática filtrada (banda de radio 300-3400 Hz)."""
    dur = 0.16 if opening else 0.22
    static = burst(rng, dur, 300, 3400, 0.05 if opening else 0.08, 0.002)
    s = [0.0] * seconds(dur + 0.05)
    mix_into(s, burst(rng, 0.006, 1500, 9000, 0.0012, 0.0001), 0, 0.9)          # clic
    mix_into(s, static, seconds(0.01), 0.55)
    if not opening:
        mix_into(s, tone(1250, 0.09, 0.05), seconds(0.02), 0.25)                 # "roger beep" corto
    return s


def objective_tone():
    """Aviso de objetivo nuevo: dos notas limpias y graves, discretas (interfaz militar, no arcade)."""
    s = [0.0] * seconds(0.45)
    mix_into(s, tone(660, 0.25, 0.09), 0, 0.5)
    mix_into(s, tone(880, 0.3, 0.12), seconds(0.11), 0.45)
    return A.lowpass(s, SR, 5000)


def pickup(rng):
    """Recoger un objeto pequeño: roce de tela + clic de plástico/metal."""
    s = cloth(rng, 0.35)
    mix_into(s, burst(rng, 0.02, 1800, 8000, 0.004), seconds(0.18), 0.8)
    mix_into(s, tone(2400, 0.05, 0.01), seconds(0.18), 0.25)
    return s


def main():
    out = sys.argv[1]
    A.write_wav(mk(os.path.join(out, "Radio", "SW_Radio_In.wav")), [radio_click(random.Random(31), True)], peak_db=-3.0)
    A.write_wav(mk(os.path.join(out, "Radio", "SW_Radio_Out.wav")), [radio_click(random.Random(32), False)], peak_db=-3.0)
    A.write_wav(mk(os.path.join(out, "UI", "SW_UI_Objective.wav")), [objective_tone()], peak_db=-4.0)
    A.write_wav(mk(os.path.join(out, "UI", "SW_UI_Pickup.wav")), [pickup(random.Random(33))], peak_db=-3.0)
    for k in range(5):
        A.write_wav(mk(os.path.join(out, "Impacts", "Flesh", f"SW_Impact_Flesh_{k + 1:02d}.wav")),
                    [A.trim_tail([flesh(random.Random(900 + k * 7))], SR, -55)[0]], peak_db=-1.0)
    A.write_wav(mk(os.path.join(out, "UI", "SW_UI_Hitmarker.wav")), [hitmarker(False)], peak_db=-3.0)
    A.write_wav(mk(os.path.join(out, "UI", "SW_UI_Hitmarker_Kill.wav")), [hitmarker(True)], peak_db=-3.0)
    for k in range(3):
        A.write_wav(mk(os.path.join(out, "Player", f"SW_Player_Hurt_{k + 1:02d}.wav")), [hurt(random.Random(950 + k))], peak_db=-1.0)
    A.write_wav(mk(os.path.join(out, "Player", "SW_Player_Heartbeat_Loop.wav")), [heartbeat()], peak_db=-2.0)
    print("OK")


if __name__ == "__main__":
    main()
