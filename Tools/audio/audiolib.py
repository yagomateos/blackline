"""Utilidades de audio sin dependencias (solo stdlib) para preparar sonidos del juego.

Lectura/escritura WAV PCM 16/24/32 bits, mono/estéreo, detección de transitorios,
recorte, fundidos, normalización, filtros simples y remuestreo lineal.
Las muestras se manejan como listas de canales: [[L...], [R...]] en float -1..1.
"""
import math
import struct
import wave

SR = 48000


def read_wav(path):
    """Devuelve (sample_rate, canales[list[list[float]]])."""
    with open(path, "rb") as f:
        data = f.read()
    # Lector RIFF propio: tolera chunks extra (bext, LIST...) y WAVE_FORMAT_EXTENSIBLE
    assert data[:4] == b"RIFF" and data[8:12] == b"WAVE", path
    pos, fmt, raw = 12, None, None
    while pos + 8 <= len(data):
        cid, size = data[pos:pos + 4], struct.unpack("<I", data[pos + 4:pos + 8])[0]
        body = data[pos + 8:pos + 8 + size]
        if cid == b"fmt ":
            fmt = struct.unpack("<HHIIHH", body[:16])
        elif cid == b"data":
            raw = body
        pos += 8 + size + (size & 1)
    tag, nch, sr, _, block, bits = fmt
    n = len(raw) // block
    chans = [[0.0] * n for _ in range(nch)]
    width = bits // 8
    if tag == 3 or (tag == 0xFFFE and bits == 32 and False):
        fmtc = "<" + "f" * (n * nch)
        vals = struct.unpack(fmtc, raw[:n * block])
        for i in range(n):
            for c in range(nch):
                chans[c][i] = vals[i * nch + c]
        return sr, chans
    if width == 2:
        vals = struct.unpack("<" + "h" * (n * nch), raw[:n * block])
        scale = 1 / 32768.0
        for i in range(n):
            for c in range(nch):
                chans[c][i] = vals[i * nch + c] * scale
    elif width == 3:
        scale = 1 / 8388608.0
        for i in range(n):
            base = i * block
            for c in range(nch):
                o = base + c * 3
                v = raw[o] | (raw[o + 1] << 8) | (raw[o + 2] << 16)
                if v & 0x800000:
                    v -= 0x1000000
                chans[c][i] = v * scale
    elif width == 4:
        vals = struct.unpack("<" + "i" * (n * nch), raw[:n * block])
        scale = 1 / 2147483648.0
        for i in range(n):
            for c in range(nch):
                chans[c][i] = vals[i * nch + c] * scale
    else:
        raise ValueError(f"{path}: {bits} bits no soportado")
    return sr, chans


def write_wav(path, chans, sr=SR, peak_db=-1.0):
    """Escribe PCM 16 bits normalizando al pico indicado (dBFS)."""
    peak = max(1e-9, max(abs(s) for ch in chans for s in ch))
    gain = 10 ** (peak_db / 20.0) / peak
    n = len(chans[0])
    nch = len(chans)
    frames = bytearray()
    for i in range(n):
        for c in range(nch):
            v = int(max(-1.0, min(1.0, chans[c][i] * gain)) * 32767)
            frames += struct.pack("<h", v)
    with wave.open(path, "wb") as w:
        w.setnchannels(nch)
        w.setsampwidth(2)
        w.setframerate(sr)
        w.writeframes(bytes(frames))


def to_mono(chans):
    if len(chans) == 1:
        return [chans[0][:]]
    n = len(chans[0])
    return [[sum(ch[i] for ch in chans) / len(chans) for i in range(n)]]


def resample(chans, sr_in, sr_out=SR):
    if sr_in == sr_out:
        return chans
    ratio = sr_in / sr_out
    n_out = int(len(chans[0]) / ratio)
    out = []
    for ch in chans:
        o = []
        for i in range(n_out):
            x = i * ratio
            j = int(x)
            f = x - j
            a = ch[j]
            b = ch[j + 1] if j + 1 < len(ch) else a
            o.append(a + (b - a) * f)
        out.append(o)
    return out


def envelope(ch, sr, win_ms=2.0):
    """Envolvente de pico por ventanas (lista de (indice_inicio, pico))."""
    w = max(1, int(sr * win_ms / 1000))
    return [(i, max(abs(s) for s in ch[i:i + w])) for i in range(0, len(ch), w)]


def find_onsets(ch, sr, threshold_db=-18.0, min_gap_s=0.25):
    """Transitorios: subidas por encima del umbral relativo al pico global, separadas min_gap_s."""
    env = envelope(ch, sr)
    peak = max(p for _, p in env) or 1e-9
    thr = peak * 10 ** (threshold_db / 20.0)
    onsets, last = [], -1e9
    prev = 0.0
    for i, p in env:
        if p >= thr and prev < thr and (i - last) / sr >= min_gap_s:
            onsets.append(i)
            last = i
        prev = p
    return onsets


def slice_(chans, start, end):
    return [ch[max(0, start):end] for ch in chans]


def fade(chans, sr, fade_in_ms=1.0, fade_out_ms=30.0):
    n = len(chans[0])
    fi, fo = int(sr * fade_in_ms / 1000), int(sr * fade_out_ms / 1000)
    for ch in chans:
        for i in range(min(fi, n)):
            ch[i] *= i / fi
        for i in range(min(fo, n)):
            ch[n - 1 - i] *= i / fo
    return chans


def trim_tail(chans, sr, floor_db=-50.0, min_s=0.05):
    """Recorta el silencio final por debajo de floor_db respecto al pico."""
    peak = max(1e-9, max(abs(s) for ch in chans for s in ch))
    thr = peak * 10 ** (floor_db / 20.0)
    n = len(chans[0])
    end = n
    while end > int(min_s * sr) and max(abs(ch[end - 1]) for ch in chans) < thr:
        end -= 1
    return [ch[:end] for ch in chans]


def lowpass(ch, sr, cutoff):
    a = 1.0 - math.exp(-2.0 * math.pi * cutoff / sr)
    y, out = 0.0, []
    for s in ch:
        y += a * (s - y)
        out.append(y)
    return out


def highpass(ch, sr, cutoff):
    lp = lowpass(ch, sr, cutoff)
    return [s - l for s, l in zip(ch, lp)]


def biquad(ch, sr, kind, freq, q=0.707, gain_db=0.0):
    """Filtro biquad RBJ: kind = 'lp' | 'hp' | 'bp' | 'peak'. Más selectivo que los de un polo."""
    w = 2 * math.pi * freq / sr
    cw, sw = math.cos(w), math.sin(w)
    alpha = sw / (2 * q)
    if kind == "lp":
        b0, b1, b2 = (1 - cw) / 2, 1 - cw, (1 - cw) / 2
        a0, a1, a2 = 1 + alpha, -2 * cw, 1 - alpha
    elif kind == "hp":
        b0, b1, b2 = (1 + cw) / 2, -(1 + cw), (1 + cw) / 2
        a0, a1, a2 = 1 + alpha, -2 * cw, 1 - alpha
    elif kind == "bp":
        b0, b1, b2 = alpha, 0.0, -alpha
        a0, a1, a2 = 1 + alpha, -2 * cw, 1 - alpha
    elif kind == "peak":
        A_ = 10 ** (gain_db / 40)
        b0, b1, b2 = 1 + alpha * A_, -2 * cw, 1 - alpha * A_
        a0, a1, a2 = 1 + alpha / A_, -2 * cw, 1 - alpha / A_
    else:
        raise ValueError(kind)
    b0, b1, b2, a1, a2 = b0 / a0, b1 / a0, b2 / a0, a1 / a0, a2 / a0
    x1 = x2 = y1 = y2 = 0.0
    out = []
    for x in ch:
        y = b0 * x + b1 * x1 + b2 * x2 - a1 * y1 - a2 * y2
        x2, x1, y2, y1 = x1, x, y1, y
        out.append(y)
    return out


def compress(ch, sr, threshold_db=-18.0, ratio=4.0, attack_ms=3.0, release_ms=80.0, makeup_db=0.0):
    """Compresor de pico simple (seguidor de envolvente)."""
    thr = 10 ** (threshold_db / 20)
    ga = math.exp(-1.0 / (sr * attack_ms / 1000))
    gr = math.exp(-1.0 / (sr * release_ms / 1000))
    makeup = 10 ** (makeup_db / 20)
    env, out = 0.0, []
    for x in ch:
        a = abs(x)
        env = ga * env + (1 - ga) * a if a > env else gr * env + (1 - gr) * a
        g = 1.0 if env <= thr else (thr * (env / thr) ** (1 / ratio)) / env
        out.append(x * g * makeup)
    return out


def saturate(ch, drive=2.0):
    """Saturación suave (tanh) normalizada."""
    k = math.tanh(drive)
    return [math.tanh(x * drive) / k for x in ch]


def stats(chans, sr):
    """Duración, pico dBFS, RMS dBFS y tasa de cruces por cero (brillo aproximado)."""
    ch = chans[0]
    n = len(ch)
    peak = max(abs(s) for s in ch) or 1e-9
    rms = math.sqrt(sum(s * s for s in ch) / max(n, 1)) or 1e-9
    zc = sum(1 for i in range(1, n) if (ch[i - 1] < 0) != (ch[i] < 0))
    return {"dur": n / sr, "peak_db": 20 * math.log10(peak), "rms_db": 20 * math.log10(rms), "zcr_hz": zc / 2 / max(n / sr, 1e-9)}
