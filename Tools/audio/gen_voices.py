"""Voces del Bloque 7 (PROVISIONALES): radio de la misión y barks de la IA con la síntesis de voz de Windows.

Las voces neuronales de Windows (es-ES "Pablo") dan una base inteligible; aquí se procesan para que encajen:
  - radio: banda de radio táctica (400-3000 Hz), saturación, compresión fuerte, soplido de fondo y
    ráfaga de ruido al soltar el PTT. Se oyen en 2D con subtítulos.
  - bark: voz "gritada" (compresión, saturación, realce de presencia) y tres voces distintas para los
    milicianos (tono y velocidad). Se oyen en 3D desde la cabeza del enemigo.
Sustituir por actores de voz reales cuando sea posible (ver MEMORIA.md).

Uso:  python -I gen_voices.py <voice_lines.tsv> <carpeta_salida SFX>
Salida: Voice/Radio/VO_<id>.wav · Voice/Barks/VO_Bark_<Categoría>_V<v>_<n>.wav (48 kHz, mono)
"""
import math
import os
import random
import subprocess
import sys
import tempfile

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import audiolib as A

SR = A.SR
HERE = os.path.dirname(os.path.abspath(__file__))

# Voces de los milicianos: (voz de Windows, tono, velocidad)
BARK_VOICES = [("Pablo", "-20%", "1.12"), ("Pablo", "-6%", "1.2"), ("Pablo", "+9%", "1.1")]


def read_lines(tsv):
    out = []
    with open(tsv, encoding="utf-8") as f:
        for line in f:
            line = line.rstrip("\n").rstrip("\r")
            if not line.strip() or line.startswith("#"):
                continue
            c = line.split("\t")
            out.append({"id": c[0], "kind": c[1], "who": c[2], "voice": c[3], "pitch": c[4], "rate": c[5], "text": c[6]})
    return out


def tts(jobs, raw_dir):
    """jobs: [(nombre, voz, tono, velocidad, texto)] -> WAV secos en raw_dir (vía tts_lines.ps1)."""
    job_tsv = os.path.join(raw_dir, "jobs.tsv")
    with open(job_tsv, "w", encoding="utf-8") as f:
        for j in jobs:
            f.write("\t".join(j) + "\n")
    subprocess.run(["powershell", "-NoProfile", "-ExecutionPolicy", "Bypass", "-File", os.path.join(HERE, "tts_lines.ps1"), job_tsv, raw_dir],
                   check=True, stdout=subprocess.DEVNULL)


def load_mono(path):
    sr, ch = A.read_wav(path)
    m = A.to_mono(ch)
    m = A.resample(m, sr, SR)[0]
    # Quitar silencios de la síntesis al principio y al final
    peak = max(abs(x) for x in m) or 1e-9
    thr = peak * 0.02
    a = next((i for i, x in enumerate(m) if abs(x) > thr), 0)
    b = len(m) - next((i for i, x in enumerate(reversed(m)) if abs(x) > thr), 0)
    return m[max(0, a - int(0.02 * SR)):min(len(m), b + int(0.08 * SR))]


def radio(v, rng):
    s = A.biquad(A.biquad(v, SR, "hp", 420, 0.8), SR, "hp", 380, 0.7)
    s = A.biquad(A.biquad(s, SR, "lp", 2900, 0.9), SR, "lp", 3200, 0.7)
    s = A.biquad(s, SR, "peak", 1700, 1.2, 5.0)                                   # nasalidad de altavoz pequeño
    s = A.compress(s, SR, -26, 6, 2, 60, 0)
    peak = max(abs(x) for x in s) or 1e-9
    s = A.saturate([x / peak for x in s], 2.4)
    # Soplido continuo bajo la voz + ráfaga al soltar el PTT ("kssh")
    pre, post = int(0.06 * SR), int(0.22 * SR)
    out = [0.0] * pre + s + [0.0] * post
    hiss = A.biquad([rng.uniform(-1, 1) for _ in range(len(out))], SR, "bp", 2200, 0.6)
    for i in range(len(out)):
        out[i] += hiss[i] * 0.035
    burst_n = int(0.16 * SR)
    burst = A.biquad([rng.uniform(-1, 1) for _ in range(burst_n)], SR, "bp", 2500, 0.5)
    start = pre + len(s) + int(0.02 * SR)
    for i in range(burst_n):
        if start + i < len(out):
            out[start + i] += burst[i] * 0.5 * math.exp(-i / (0.05 * SR))
    return A.fade([out], SR, 3, 20)[0]


def bark(v, rng):
    s = A.biquad(v, SR, "hp", 140, 0.7)
    s = A.biquad(s, SR, "peak", 2600, 1.0, 6.0)                                   # presencia: voz forzada
    s = A.biquad(s, SR, "peak", 300, 1.0, -3.0)
    s = A.compress(s, SR, -32, 6, 1.5, 70, 0)
    peak = max(abs(x) for x in s) or 1e-9
    s = A.saturate([x / peak for x in s], 2.6)
    s = A.biquad(s, SR, "lp", 7500, 0.7)
    return A.fade([s + [0.0] * int(0.05 * SR)], SR, 2, 30)[0]


def main():
    tsv, out = sys.argv[1], sys.argv[2]
    lines = read_lines(tsv)
    raw_dir = tempfile.mkdtemp(prefix="bl_vo_")
    jobs = []
    for L in lines:
        if L["kind"] == "radio":
            jobs.append((L["id"], L["voice"], L["pitch"], L["rate"], L["text"]))
        else:
            for v, (voice, pitch, rate) in enumerate(BARK_VOICES):
                jobs.append((f"{L['id']}__V{v + 1}", voice, pitch, rate, L["text"]))
    tts(jobs, raw_dir)
    count = 0
    for L in lines:
        rng = random.Random(L["id"])
        if L["kind"] == "radio":
            s = radio(load_mono(os.path.join(raw_dir, L["id"] + ".wav")), rng)
            path = os.path.join(out, "Voice", "Radio", f"VO_{L['id']}.wav")
            os.makedirs(os.path.dirname(path), exist_ok=True)
            A.write_wav(path, [s], peak_db=-2.0)
            count += 1
        else:
            cat, n = L["id"].rsplit("_", 1)
            for v in range(len(BARK_VOICES)):
                s = bark(load_mono(os.path.join(raw_dir, f"{L['id']}__V{v + 1}.wav")), rng)
                path = os.path.join(out, "Voice", "Barks", f"VO_Bark_{cat}_V{v + 1}_{n}.wav")
                os.makedirs(os.path.dirname(path), exist_ok=True)
                A.write_wav(path, [s], peak_db=-1.0)
                count += 1
    print(f"OK: {count} voces")


if __name__ == "__main__":
    main()
