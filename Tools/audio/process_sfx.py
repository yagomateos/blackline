"""Prepara los sonidos del juego a partir de grabaciones reales CC0 + síntesis donde no hay grabación.

Fuentes (CC0, ver Docs/Creditos_Assets.md):
  - The Free Firearm Sound Library (opengameart.org): AR-15 (5.56) cercano y medio, AK-47 ráfagas.
  - "Gun reload sounds" (SpringySpringo) y "equipment clicks III" (LFA) de opengameart.org.
Síntesis (granular / modal): impactos por material, casquillos por superficie, foley y ambiente.

Uso:  python -I process_sfx.py <carpeta_fuentes> <carpeta_salida>
  carpeta_fuentes debe contener "Prepared SFX Library/" y los .wav de opengameart.
Salida (48 kHz, 16 bits): Weapons/AR7, Impacts/<Material>, Casings/<Superficie>, Foley, Ambience.
"""
import math
import os
import random
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import audiolib as A

SR = A.SR


def mk(path):
    os.makedirs(os.path.dirname(path), exist_ok=True)
    return path


def load(path):
    sr, ch = A.read_wav(path)
    return A.resample(ch, sr, SR)


def seconds(s):
    return int(s * SR)


def mix_into(dst, src, offset=0, gain=1.0):
    if len(dst) < offset + len(src):
        dst.extend([0.0] * (offset + len(src) - len(dst)))
    for i, s in enumerate(src):
        dst[offset + i] += s * gain
    return dst


def pitch(ch, factor):
    """Cambio de tono por remuestreo (también cambia la duración, como un 'pitch' de motor)."""
    n = int(len(ch) / factor)
    out = []
    for i in range(n):
        x = i * factor
        j = int(x)
        f = x - j
        a = ch[j] if j < len(ch) else 0.0
        b = ch[j + 1] if j + 1 < len(ch) else a
        out.append(a + (b - a) * f)
    return out


# ---------------------------------------------------------------------------
# Armas: capas reales
# ---------------------------------------------------------------------------

def weapons(src, out):
    lib = os.path.join(src, "Prepared SFX Library")
    near = load(os.path.join(lib, "AR-15", "D_32P.wav"))        # AR-15 cercano, 2 disparos
    mid = load(os.path.join(lib, "AR-15", "D_24P.wav"))         # AR-15 medio, 2 series de 5
    ak = load(os.path.join(lib, "AK-47", "C_27P.wav"))          # AK-47 cercano, ráfagas
    near_m = A.to_mono(near)[0]
    ons_near = A.find_onsets(near_m, SR, -20, 0.5)[:2]
    ons_ak = A.find_onsets(A.to_mono(ak)[0], SR, -20, 0.06)
    ons_mid = A.find_onsets(A.to_mono(mid)[0], SR, -20, 0.05)
    rng = random.Random(7)

    # Capa cercana ("punch" + primeras reflexiones, 0,32 s), estéreo para el jugador.
    # Variaciones: los 2 disparos reales del AR-15, con un toque de cuerpo de un disparo de AK distinto
    # y ligeras variaciones de tono -> 12 variaciones que no suenan a "metralleta de la misma muestra".
    v = 0
    for k in range(12):
        base_on = ons_near[k % len(ons_near)]
        pre = seconds(0.004)
        close = A.slice_(near, base_on - pre, base_on + seconds(0.32))
        akon = ons_ak[(k * 3 + 1) % len(ons_ak)]
        body = A.slice_(ak, akon - pre, akon + seconds(0.08))
        f = 1.0 + rng.uniform(-0.035, 0.035)
        layered = []
        for c in range(2):
            ch = close[c][:]
            mix_into(ch, A.lowpass(body[c], SR, 2500), 0, 0.28)
            layered.append(pitch(ch, f))
        A.fade(layered, SR, 0.3, 60)
        v += 1
        A.write_wav(mk(os.path.join(out, "Weapons", "AR7", f"SW_AR7_Fire_Close_{v:02d}.wav")), layered, peak_db=-0.5)

    # Cola (eco exterior real del AR-15): desde 0,12 s tras el disparo hasta ~3,2 s, con entrada suave
    for k, on in enumerate(ons_near):
        tail = A.slice_(near, on + seconds(0.10), on + seconds(3.2))
        n = len(tail[0])
        for c in range(2):
            for i in range(min(seconds(0.06), n)):
                tail[c][i] *= i / seconds(0.06)
        tail = A.trim_tail(tail, SR, -55)
        A.fade(tail, SR, 0, 400)
        A.write_wav(mk(os.path.join(out, "Weapons", "AR7", f"SW_AR7_Tail_{k + 1:02d}.wav")), tail, peak_db=-3.0)

    # Lejano (para la IA / disparos lejanos): grabación a distancia media, mono, 1,2 s
    for k, on in enumerate(ons_mid[:10]):
        d = A.to_mono(A.slice_(mid, on - seconds(0.003), on + seconds(1.2)))
        A.fade(d, SR, 0.3, 250)
        A.write_wav(mk(os.path.join(out, "Weapons", "AR7", f"SW_AR7_Fire_Distant_{k + 1:02d}.wav")), d, peak_db=-1.0)

    # Cercano mono (IA a corta distancia, espacializado)
    for k in range(4):
        on = ons_near[k % len(ons_near)]
        m = A.to_mono(A.slice_(near, on - seconds(0.004), on + seconds(0.5)))
        m = [pitch(m[0], 1.0 + rng.uniform(-0.03, 0.03))]
        A.fade(m, SR, 0.3, 120)
        A.write_wav(mk(os.path.join(out, "Weapons", "AR7", f"SW_AR7_Fire_Close3D_{k + 1:02d}.wav")), m, peak_db=-0.5)


def reload_sounds(src, out):
    rel = load(os.path.join(src, "assaultriflereload1_0.wav"))
    clicks = load(os.path.join(src, "equipment_clicks3.wav"))
    cm = A.to_mono(clicks)[0]
    ons = A.find_onsets(cm, SR, -20, 0.06)

    def cut(chs, a, b, name, fade_out=40, peak=-2.0):
        s = A.to_mono(A.slice_(chs, seconds(a), seconds(b)))
        s = A.trim_tail(s, SR, -50)
        A.fade(s, SR, 0.5, fade_out)
        A.write_wav(mk(os.path.join(out, "Weapons", "AR7", name)), s, peak_db=peak)

    # Fusil (airsoft, CC0): suelta y saca el cargador / lo inserta
    cut(rel, 0.20, 0.80, "SW_AR7_MagOut.wav", 80)
    cut(rel, 1.00, 1.50, "SW_AR7_MagIn.wav", 60)
    # Cerrojo de un rifle real (equipment clicks III): tirar y soltar
    t0, t1 = ons[0] / SR, ons[1] / SR
    cut(clicks, t0 - 0.01, t1 - 0.01, "SW_AR7_BoltPull.wav", 30)
    cut(clicks, t1 - 0.01, t1 + 0.30, "SW_AR7_BoltRelease.wav", 60)
    # Gatillo en vacío: clic seco corto
    t = ons[5] / SR
    cut(clicks, t - 0.005, t + 0.09, "SW_AR7_DryFire.wav", 20, -6.0)


# ---------------------------------------------------------------------------
# Síntesis: impactos por material
# ---------------------------------------------------------------------------

def noise(n, rng):
    return [rng.uniform(-1, 1) for _ in range(n)]


def burst(rng, dur, lo, hi, decay, attack=0.0005):
    n = seconds(dur)
    s = A.lowpass(A.highpass(noise(n, rng), SR, lo), SR, hi)
    return [x * min(1.0, (i / SR) / attack) * math.exp(-(i / SR) / decay) for i, x in enumerate(s)]


def tone(freq, dur, decay, phase=0.0, sweep=1.0):
    out, ph = [], phase
    n = seconds(dur)
    for i in range(n):
        t = i / SR
        f = freq * (sweep ** (t / max(dur, 1e-6)))
        ph += 2 * math.pi * f / SR
        out.append(math.sin(ph) * math.exp(-t / decay))
    return out


def grains(rng, total, count, lo, hi, gdur=(0.002, 0.008), density_decay=0.15, amp=0.5):
    out = [0.0] * seconds(total)
    for _ in range(count):
        t = rng.expovariate(1.0 / density_decay)
        if t >= total - 0.01:
            continue
        g = burst(rng, rng.uniform(*gdur), lo, hi, rng.uniform(gdur[0], gdur[1]) * 0.5)
        mix_into(out, g, seconds(t), amp * math.exp(-t / (density_decay * 2)) * rng.uniform(0.4, 1.0))
    return out


def crack(rng, bright=3000):
    return burst(rng, 0.012, bright, 16000, 0.0018, 0.0001)


def impact(material, rng):
    if material == "Concrete":
        s = crack(rng, 2500)
        mix_into(s, tone(rng.uniform(95, 140), 0.12, 0.035), 0, 0.55)
        mix_into(s, burst(rng, 0.15, 600, 4000, 0.03), 0, 0.5)
        mix_into(s, grains(rng, 0.6, rng.randint(35, 60), 1800, 9000, (0.002, 0.007), 0.12, 0.35), seconds(0.01))
    elif material == "Metal":
        s = crack(rng, 3500)
        f0 = rng.uniform(850, 1500)
        for k, (r, dec, g) in enumerate(((1.0, 0.35, 0.5), (2.32, 0.22, 0.35), (4.25, 0.12, 0.25), (6.63, 0.07, 0.2))):
            mix_into(s, tone(f0 * r * rng.uniform(0.98, 1.02), 0.8, dec, rng.uniform(0, 6)), 0, g)
        mix_into(s, burst(rng, 0.05, 2000, 12000, 0.008), 0, 0.6)
        if rng.random() < 0.45:  # rebote (ricochet)
            mix_into(s, [x * math.sin(math.pi * i / seconds(0.32)) for i, x in enumerate(tone(rng.uniform(3200, 4200), 0.32, 1.0, 0, 0.4))], seconds(0.02), 0.22)
    elif material == "Wood":
        s = crack(rng, 1500)
        mix_into(s, burst(rng, 0.08, 250, 1400, 0.022), 0, 1.0)
        mix_into(s, tone(rng.uniform(380, 650), 0.12, 0.03), 0, 0.5)
        mix_into(s, grains(rng, 0.3, rng.randint(12, 22), 2500, 7000, (0.002, 0.005), 0.05, 0.25), seconds(0.005))
    elif material == "Glass":
        s = crack(rng, 4000)
        out = [0.0] * seconds(1.0)
        for _ in range(rng.randint(70, 130)):
            t = rng.expovariate(1 / 0.12)
            if t > 0.9:
                continue
            mix_into(out, tone(rng.uniform(2800, 9500), 0.08, rng.uniform(0.006, 0.03), rng.uniform(0, 6)), seconds(t), rng.uniform(0.05, 0.25) * math.exp(-t / 0.35))
        mix_into(s, out, seconds(0.004))
        mix_into(s, burst(rng, 0.2, 3000, 14000, 0.05), 0, 0.5)
    elif material == "Dirt":
        s = burst(rng, 0.12, 80, 700, 0.03)
        mix_into(s, burst(rng, 0.35, 1500, 6000, 0.09, 0.004), 0, 0.45)
        mix_into(s, grains(rng, 0.4, 25, 1500, 5000, (0.004, 0.012), 0.1, 0.2), seconds(0.02))
    else:
        raise ValueError(material)
    return s


def casing(surface, rng):
    out = [0.0] * seconds(0.6)
    bounces = [(0.0, 1.0), (rng.uniform(0.07, 0.11), 0.5), (rng.uniform(0.15, 0.2), 0.25), (rng.uniform(0.22, 0.28), 0.12)]
    for t, g in bounces:
        hit = [0.0] * seconds(0.4)
        if surface in ("Concrete", "Metal"):
            for k, f in enumerate((3900, 6150, 8800, 11200)):
                mix_into(hit, tone(f * rng.uniform(0.96, 1.04), 0.4, (0.14 if surface == "Metal" else 0.09) / (k + 1), rng.uniform(0, 6)), 0, 0.45 / (k + 1))
            mix_into(hit, burst(rng, 0.01, 3000, 14000, 0.002), 0, 0.6)
        elif surface == "Wood":
            mix_into(hit, burst(rng, 0.05, 600, 3500, 0.012), 0, 1.0)
            mix_into(hit, tone(rng.uniform(3500, 4500), 0.1, 0.02), 0, 0.2)
        else:  # tierra: casi solo un golpe sordo
            mix_into(hit, burst(rng, 0.05, 150, 1500, 0.015), 0, 1.0)
        mix_into(out, hit, seconds(t), g)
    return out


# ---------------------------------------------------------------------------
# Foley y ambiente
# ---------------------------------------------------------------------------

def cloth(rng, dur=0.45):
    """Roce de ropa/equipo: ruido filtrado con modulación lenta."""
    n = seconds(dur)
    base = A.lowpass(A.highpass(noise(n, rng), SR, 900), SR, 5000)
    env_rate = rng.uniform(9, 16)
    return [x * (0.5 + 0.5 * math.sin(2 * math.pi * env_rate * i / SR + rng.uniform(0, 6))) ** 2 * math.sin(math.pi * i / n) for i, x in enumerate(base)]


def footstep(surface, rng, run=False):
    """Bota militar: golpe de talón + apoyo de puntera/roce, por superficie."""
    out = [0.0] * seconds(0.35)
    heel = burst(rng, 0.05, 60, 900, 0.012)
    mix_into(heel, tone(rng.uniform(70, 110), 0.06, 0.015), 0, 0.5)
    toe_t = rng.uniform(0.05, 0.08) * (0.7 if run else 1.0)
    if surface == "Concrete":
        mix_into(out, heel, 0, 1.0)
        mix_into(out, burst(rng, 0.02, 1500, 7000, 0.004), 0, 0.35)          # clic de la suela
        mix_into(out, burst(rng, 0.09, 600, 4500, 0.025, 0.01), seconds(toe_t), 0.45)  # roce
        mix_into(out, grains(rng, 0.2, 6, 3000, 8000, (0.001, 0.003), 0.05, 0.12), seconds(0.01))  # arenilla
    elif surface == "Dirt":
        mix_into(out, A.lowpass(heel, SR, 500), 0, 0.8)
        mix_into(out, grains(rng, 0.25, rng.randint(25, 40), 1200, 6000, (0.002, 0.006), 0.06, 0.35), 0, 1.0)
    elif surface == "Metal":
        mix_into(out, heel, 0, 0.8)
        f0 = rng.uniform(280, 420)
        for r, d, g in ((1.0, 0.12, 0.35), (2.7, 0.06, 0.2), (5.1, 0.03, 0.12)):
            mix_into(out, tone(f0 * r, 0.25, d, rng.uniform(0, 6)), 0, g)
        mix_into(out, burst(rng, 0.06, 800, 5000, 0.015), seconds(toe_t), 0.35)
    elif surface == "Wood":
        mix_into(out, heel, 0, 0.9)
        mix_into(out, tone(rng.uniform(170, 240), 0.12, 0.04), 0, 0.45)
        mix_into(out, burst(rng, 0.06, 400, 3000, 0.02), seconds(toe_t), 0.4)
    return out


def ambience(src, rng, dur=40.0):
    """Ambiente de ciudad en guerra (bucle de 40 s): viento, rumor lejano de la ciudad y tiroteos lejanos."""
    n = seconds(dur)
    # Viento: ruido marrón filtrado con ráfagas
    wind, b = [], 0.0
    gust_phase = rng.uniform(0, 6)
    for i in range(n):
        b = 0.995 * b + 0.05 * rng.uniform(-1, 1)
        t = i / SR
        g = 0.55 + 0.45 * math.sin(2 * math.pi * t / 9.0 + gust_phase) * math.sin(2 * math.pi * t / 23.0)
        wind.append(b * g)
    wind = A.lowpass(A.highpass(wind, SR, 60), SR, 1200)
    # Rumor grave de ciudad (tráfico/maquinaria lejana)
    rumble = A.lowpass(noise(n, rng), SR, 140)
    L = [w * 0.9 + r * 0.6 for w, r in zip(wind, rumble)]
    R = L[:]
    # Tiroteos lejanos: ráfagas reales (AK-47 y PPSh a distancia media), muy filtradas
    lib = os.path.join(src, "Prepared SFX Library")
    far_src = [load(os.path.join(lib, "AK-47", "C_36P.wav")), load(os.path.join(lib, "PPSh", "P_22P.wav"))]
    t = 3.0
    while t < dur - 4:
        srcch = rng.choice(far_src)
        m = A.to_mono(srcch)[0]
        start = rng.randint(0, max(1, len(m) - seconds(3.0)))
        seg = A.lowpass(A.lowpass(m[start:start + seconds(2.5)], SR, 900), SR, 900)
        seg = [x * math.sin(math.pi * i / len(seg)) ** 0.3 for i, x in enumerate(seg)]
        pan = rng.uniform(0.2, 0.8)
        gain = rng.uniform(0.05, 0.14)
        mix_into(L, seg, seconds(t), gain * (1 - pan))
        mix_into(R, seg, seconds(t), gain * pan)
        t += rng.uniform(4.0, 9.0)
    # Bucle sin costura: fundido cruzado del final con el principio
    xf = seconds(2.0)
    for ch in (L, R):
        for i in range(xf):
            a = i / xf
            ch[i] = ch[i] * a + ch[n - xf + i] * (1 - a)
        del ch[n - xf:]
    return [L, R]


def main():
    src, out = sys.argv[1], sys.argv[2]
    weapons(src, out)
    reload_sounds(src, out)
    for material, count in (("Concrete", 6), ("Metal", 6), ("Wood", 5), ("Glass", 4), ("Dirt", 5)):
        for k in range(count):
            rng = random.Random(hash(material) % 997 + k * 31)
            s = impact(material, rng)
            A.write_wav(mk(os.path.join(out, "Impacts", material, f"SW_Impact_{material}_{k + 1:02d}.wav")), [A.trim_tail([s], SR, -55)[0]], peak_db=-1.0)
    for surface in ("Concrete", "Metal", "Wood", "Dirt"):
        for k in range(3):
            rng = random.Random(hash(surface) % 991 + k * 17)
            A.write_wav(mk(os.path.join(out, "Casings", surface, f"SW_Casing_{surface}_{k + 1:02d}.wav")), [casing(surface, rng)], peak_db=-1.0)
    for k in range(4):
        A.write_wav(mk(os.path.join(out, "Foley", f"SW_Foley_Gear_{k + 1:02d}.wav")), [cloth(random.Random(500 + k))], peak_db=-3.0)
    for surface in ("Concrete", "Dirt", "Metal", "Wood"):
        for k in range(8):
            rng = random.Random(hash(surface) % 983 + k * 13)
            A.write_wav(mk(os.path.join(out, "Footsteps", surface, f"SW_Step_{surface}_{k + 1:02d}.wav")), [footstep(surface, rng, k >= 4)], peak_db=-1.0)
    A.write_wav(mk(os.path.join(out, "Ambience", "SW_Amb_WarCity_Loop.wav")), ambience(src, random.Random(42)), peak_db=-6.0)
    print("OK")


if __name__ == "__main__":
    main()
