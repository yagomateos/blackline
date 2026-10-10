"""Banda sonora del promo de BLACKLINE (~30 s): síntesis propia + sonidos del juego (ArtSource/Audio/SFX).

Estructura (los tiempos vienen del montaje, ver build_trailer.py):
  frío      negro, dron grave, radio, latido
  golpe     BRAAM al primer plano
  misiones  ostinato a 120 ppm + taikos, golpe en cada misión, disparos/explosiones del juego
  subida    riser con redobles que aceleran
  silencio  medio segundo vacío
  logo      impacto final + cola
"""
import os
import wave

import numpy as np
from scipy.signal import lfilter

SR = 48000
ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
SFX = os.path.join(ROOT, "ArtSource", "Audio", "SFX")
rng = np.random.default_rng(7)


def load(rel):
    """WAV del juego -> estéreo float (2, n) a 48 kHz."""
    with wave.open(os.path.join(SFX, rel), "rb") as w:
        nch, width, sr, n = w.getnchannels(), w.getsampwidth(), w.getframerate(), w.getnframes()
        raw = w.readframes(n)
    if width == 2:
        a = np.frombuffer(raw, "<i2").astype(np.float32) / 32768
    elif width == 3:
        b = np.frombuffer(raw, np.uint8).reshape(-1, 3).astype(np.int32)
        v = b[:, 0] | (b[:, 1] << 8) | (b[:, 2] << 16)
        v[v >= 1 << 23] -= 1 << 24
        a = v.astype(np.float32) / (1 << 23)
    else:
        a = np.frombuffer(raw, "<i4").astype(np.float32) / 2 ** 31
    a = a.reshape(-1, nch).T
    if nch == 1:
        a = np.vstack([a, a])
    if sr != SR:
        t = np.arange(int(a.shape[1] * SR / sr)) * sr / SR
        a = np.vstack([np.interp(t, np.arange(a.shape[1]), c) for c in a])
    return a[:2]


def env(n, a, d, curve=4.0):
    """Ataque lineal a muestras, caída exponencial d (s)."""
    t = np.arange(n) / SR
    e = np.exp(-t / max(d, 1e-3) * (curve / 4))
    att = max(1, int(a * SR))
    e[:att] *= np.linspace(0, 1, att)
    return e


def lowpass(x, fc):
    """Paso bajo de un polo (aplicado dos veces)."""
    k = np.exp(-2 * np.pi * fc / SR)
    for _ in range(2):
        x = lfilter([1 - k], [1, -k], x)
    return x


def saw(freq, n, detune=0.0):
    t = np.arange(n) / SR
    f = freq * (1 + detune)
    return 2 * ((t * f) % 1.0) - 1


def stereo(m, width=0.0):
    return np.vstack([m * (1 - width), m * (1 + width)]) if width else np.vstack([m, m])


def braam(dur=3.2):
    n = int(dur * SR)
    out = np.zeros(n)
    for f in (36.71, 73.42, 110.0, 146.83, 55.0):
        for dt in (-0.006, 0.0, 0.007):
            out += saw(f, n, dt) * (1.0 if f < 80 else 0.55)
    out = lowpass(out, 900) * env(n, 0.02, 1.4)
    out = np.tanh(out * 0.9)
    sub = np.sin(2 * np.pi * np.cumsum(np.linspace(70, 32, n)) / SR) * env(n, 0.005, 1.0) * 1.2
    return stereo(out * 0.5 + sub * 0.6, 0.05)


def boom(dur=2.5, f0=90, f1=28, gain=1.0):
    n = int(dur * SR)
    sweep = np.sin(2 * np.pi * np.cumsum(np.geomspace(f0, f1, n)) / SR) * env(n, 0.002, 0.7)
    nz = lowpass(rng.standard_normal(n), 1800) * env(n, 0.001, 0.12) * 0.8
    return stereo(np.tanh((sweep * 1.4 + nz) * gain))


def hit(dur=1.2):
    """Golpe corto de corte: taiko grave + chasquido."""
    n = int(dur * SR)
    body = np.sin(2 * np.pi * np.cumsum(np.geomspace(140, 45, n)) / SR) * env(n, 0.001, 0.35)
    snap = lowpass(rng.standard_normal(n), 5000) * env(n, 0.0005, 0.03) * 0.6
    return stereo(np.tanh((body + snap) * 1.3) * 0.9)


def taiko(dur=0.6, f=80, g=0.7):
    n = int(dur * SR)
    b = np.sin(2 * np.pi * np.cumsum(np.geomspace(f * 1.8, f, n)) / SR) * env(n, 0.001, 0.18)
    s = lowpass(rng.standard_normal(n), 2500) * env(n, 0.0005, 0.02) * 0.3
    return stereo((b + s) * g)


def whoosh(dur=0.8, rev=False):
    n = int(dur * SR)
    x = rng.standard_normal(n)
    sh = np.sin(np.linspace(0, np.pi, n)) ** 2
    x = lowpass(x, 3500) * sh
    if rev:
        x = x * np.linspace(0.2, 1, n)
    return np.vstack([x * np.linspace(1, 0.3, n), x * np.linspace(0.3, 1, n)]) * 0.5


def riser(dur):
    n = int(dur * SR)
    t = np.arange(n) / SR
    up = np.linspace(0, 1, n)
    nz = lowpass(rng.standard_normal(n), 6000) * up ** 2 * 0.5
    tone = sum(np.sin(2 * np.pi * np.cumsum(np.geomspace(f, f * 4, n)) / SR) for f in (110, 165, 220.5)) / 3
    return stereo((nz + tone * up ** 1.5 * 0.35) * 0.8, 0.1)


def drone(dur):
    n = int(dur * SR)
    t = np.arange(n) / SR
    x = (saw(36.71, n) + saw(36.9, n) + 0.4 * saw(73.6, n))
    x = lowpass(x, 220) * (0.6 + 0.4 * np.sin(2 * np.pi * 0.7 * t))
    fade = np.minimum(1, t / (dur * 0.7))
    return stereo(x * fade * 0.35, 0.08)


def ostinato(dur, bpm=120.0):
    """Cuerdas graves en semicorcheas (Re menor) con filtro que se abre."""
    n = int(dur * SR)
    step = 60 / bpm / 4
    notes = [73.42, 73.42, 87.31, 73.42, 73.42, 110.0, 98.0, 87.31]
    out = np.zeros(n)
    k = int(step * SR)
    for i in range(int(dur / step)):
        f = notes[i % len(notes)]
        seg = (saw(f, k, -0.004) + saw(f, k, 0.004) + 0.5 * saw(2 * f, k)) * env(k, 0.004, 0.09)
        a = i * k
        out[a:a + k] += seg[:max(0, min(k, n - a))]
    cut = np.linspace(500, 2600, n)
    # filtro de apertura progresiva: mezcla de dos pasos bajo
    lo, hi = lowpass(out, 500), lowpass(out, 2600)
    w = (cut - 500) / 2100
    return stereo((lo * (1 - w) + hi * w) * 0.22, 0.12)


class Mix:
    def __init__(self, dur):
        self.buf = np.zeros((2, int(dur * SR) + SR * 4))

    def add(self, sig, t, gain=1.0):
        a = int(t * SR)
        if a >= self.buf.shape[1]:
            return
        b = min(self.buf.shape[1], a + sig.shape[1])
        self.buf[:, a:b] += sig[:, :b - a] * gain

    def duck(self, t0, t1, g):
        self.buf[:, int(t0 * SR):int(t1 * SR)] *= g

    def write(self, path, dur):
        x = self.buf[:, :int(dur * SR)]
        # compresor/limitador suave
        x = np.tanh(x * 1.15) * 0.97
        x /= max(1e-6, np.abs(x).max()) / 0.95
        n = x.shape[1]
        fo = int(0.15 * SR)
        x[:, n - fo:] *= np.linspace(1, 0, fo)
        pcm = (x.T * 32767).astype("<i2")
        with wave.open(path, "wb") as w:
            w.setnchannels(2)
            w.setsampwidth(2)
            w.setframerate(SR)
            w.writeframes(pcm.tobytes())
