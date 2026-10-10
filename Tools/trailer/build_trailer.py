"""Monta el promo de BLACKLINE (~30 s, 1280x720 30 fps, H.264 + AAC).

Fuente: fotogramas volcados del juego con -dumpmovie mientras el bot juega cada misión
(ver Tools/trailer/README.md). Efectos: franjas de cine, gradación, empujes de cámara, temblor en golpes,
destellos, aberración cromática en los cortes, grano y viñeta. Títulos con PIL. Audio: trailer_audio.py.

Uso:  python build_trailer.py <carpeta_fotogramas> <salida.mp4>
      (carpeta_fotogramas contiene M1..M5 con MovieFrameNNNNN.png)
"""
import os
import subprocess
import sys

import imageio_ffmpeg
import numpy as np
from PIL import Image, ImageDraw, ImageFilter, ImageFont

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import trailer_audio as TA  # noqa: E402

W, H, FPS = 1280, 720, 30
BAR = 84  # franjas 2.39:1 aprox.
FONT = "C:/Windows/Fonts/bahnschrift.ttf"
AMBER = (232, 170, 60)

# ---------------------------------------------------------------------------------------------
# Montaje. Cada misión: título + fases (clip = misión, fotograma inicial, duración s, velocidad, fase, sfx).
# sfx: lista de (segundo dentro del clip, archivo relativo a ArtSource/Audio/SFX, ganancia)
# ---------------------------------------------------------------------------------------------
AR7 = ["Weapons/AR7/SW_AR7_Fire_Close_%02d.wav" % i for i in range(1, 13)]
BOOM = ["Weapons/Grenade/SW_Grenade_Explosion_%02d.wav" % i for i in range(1, 4)]
ROTOR = "Vehicles/SW_Heli_Rotor_Loop.wav"


def burst(t0, n, gap=0.09, g=0.55):
    return [(t0 + i * gap, AR7[i % len(AR7)], g) for i in range(n)]


INTRO = 2.0
MISSIONS = [
    ("01", "AMANECER ROTO", [
        ("M1", 150, 1.6, 1.0, "INFILTRACIÓN", []),
        ("M1", 505, 1.2, 1.0, "RESCATE", []),
        ("M1", 3990, 1.2, 1.0, "EXTRACCIÓN", [(0.0, ROTOR, 0.5)]),
    ]),
    ("02", "MANIFIESTO", [
        ("M2", 120, 1.6, 1.0, "INFILTRACIÓN", []),
        ("M2", 420, 1.2, 1.0, "ALARMA", [(0.0, "Vehicles/SW_Drone_Alert.wav", 0.5)]),
        ("M2", 1830, 1.2, 1.0, "HUIDA", burst(0.15, 6)),
    ]),
    ("03", "RÍA", [
        ("M3", 100, 1.6, 1.0, "CASCO VIEJO", burst(0.6, 3, 0.12, 0.45)),
        ("M3", 355, 1.2, 1.0, "BRECHA", [(0.1, BOOM[0], 0.8)]),
        ("M3", 510, 1.2, 1.0, "TIRADOR", [(0.2, "Weapons/Bullet/SW_Bullet_Crack_01.wav", 0.8)]),
    ]),
    ("04", "FUEGO CRUZADO", [
        ("M4", 120, 1.6, 1.0, "EL PUENTE", [(0.3, "Vehicles/SW_Jet_Flyby.wav", 0.6)]),
        ("M4", 1820, 1.2, 1.0, "AMETRALLADORA", burst(0.0, 11, 0.1, 0.5)),
        ("M4", 3960, 1.2, 1.0, "EL VADO", [(0.25, BOOM[1], 0.8)]),
    ]),
    ("05", "LÍNEA NEGRA", [
        ("M5", 120, 1.6, 1.0, "EL PUERTO", burst(0.5, 4, 0.11, 0.45)),
        ("M5", 2575, 1.2, 1.0, "LA AZOTEA", [(0.0, ROTOR, 0.6)]),
        ("M5", 2700, 1.2, 1.0, "DETENCIÓN", [(0.1, BOOM[2], 0.8)]),
    ]),
]
MONTAGE = [(tag, f, d, 1.0) for tag, f, d in [   # (misión, fotograma, duración, velocidad)
    ("M2", 3130, 0.42), ("M1", 800, 0.40), ("M3", 1000, 0.38), ("M4", 1940, 0.36), ("M5", 150, 0.34),
    ("M1", 4860, 0.32), ("M3", 2060, 0.30), ("M4", 3020, 0.28), ("M2", 1850, 0.26), ("M5", 2720, 0.24),
    ("M1", 4000, 0.22), ("M4", 4020, 0.2)]]
LOGO_BG = ("M5", 2790)
SILENCE = 0.45
LOGO = 3.5


def load_font(size, bold=True):
    f = ImageFont.truetype(FONT, size)
    try:
        f.set_variation_by_name("Bold SemiCondensed" if bold else "SemiLight SemiCondensed")
    except Exception:
        pass
    return f


def spaced(draw, xy, text, font, fill, spacing):
    x, y = xy
    for ch in text:
        draw.text((x, y), ch, font=font, fill=fill)
        x += draw.textlength(ch, font=font) + spacing
    return x


def spaced_width(draw, text, font, spacing):
    return sum(draw.textlength(c, font=font) for c in text) + spacing * (len(text) - 1)


# ---------------------------------------------------------------------------------------------
class Frames:
    def __init__(self, root):
        self.root = root

    def get(self, tag, idx):
        p = os.path.join(self.root, tag, "MovieFrame%05d.png" % idx)
        while not os.path.exists(p) and idx > 0:
            idx -= 1
            p = os.path.join(self.root, tag, "MovieFrame%05d.png" % idx)
        return np.asarray(Image.open(p).convert("RGB"), dtype=np.float32) / 255


def grade(img):
    # contraste en S, algo menos de saturación, sombras frías y altas cálidas
    img = np.clip(img * 1.12, 0, 1)  # las misiones nocturnas salen muy oscuras
    l = img.mean(axis=2, keepdims=True)
    img = l + (img - l) * 0.82
    img = np.clip((img - 0.5) * 1.18 + 0.5, 0, 1)
    img = img * img * (3 - 2 * img) * 0.35 + img * 0.65
    sh = (1 - l) ** 2
    hi = l ** 2
    img = img + sh * np.array([-0.02, 0.005, 0.03]) + hi * np.array([0.04, 0.015, -0.03])
    return np.clip(img, 0, 1)


def transform(img, zoom, dx, dy):
    if zoom <= 1.0005 and abs(dx) < 0.5 and abs(dy) < 0.5:
        return img
    cw, ch = W / zoom, H / zoom
    x0 = (W - cw) / 2 + dx
    y0 = (H - ch) / 2 + dy
    x0 = min(max(x0, 0), W - cw)
    y0 = min(max(y0, 0), H - ch)
    pil = Image.fromarray((img * 255).astype(np.uint8))
    pil = pil.resize((W, H), Image.BILINEAR, box=(x0, y0, x0 + cw, y0 + ch))
    return np.asarray(pil, dtype=np.float32) / 255


def aberration(img, px):
    if px < 1:
        return img
    out = img.copy()
    out[:, px:, 0] = img[:, :-px, 0]
    out[:, :-px, 2] = img[:, px:, 2]
    return out


_vy, _vx = np.mgrid[0:H, 0:W]
VIGNETTE = (1 - 0.55 * (((_vx - W / 2) / (W / 2)) ** 2 + ((_vy - H / 2) / (H / 2)) ** 2) ** 1.4 * 0.5)[..., None]
GRAIN_RNG = np.random.default_rng(3)


def finish(img, bars=True):
    img = img * VIGNETTE
    img = img + GRAIN_RNG.normal(0, 0.018, (H, W, 1))
    if bars:
        img[:BAR] = 0
        img[H - BAR:] = 0
    return np.clip(img, 0, 1)


def over(img, rgba):
    a = rgba[..., 3:4]
    return img * (1 - a) + rgba[..., :3] * a


def overlay_layer():
    return Image.new("RGBA", (W, H), (0, 0, 0, 0))


def to_np(layer):
    return np.asarray(layer, dtype=np.float32) / 255


# ---------------------------------------------------------------------------------------------
def build(frames_root, out_mp4):
    fr = Frames(frames_root)
    events = []      # audio: (t, tipo, extra)
    timeline = []    # vídeo: funciones por fotograma

    t = 0.0
    # --- Frío: textos sobre negro -------------------------------------------------------------
    timeline.append(("intro", t, INTRO, None))
    events += [(0.0, "drone", INTRO + 0.8), (0.05, "sfx", ("Radio/SW_Radio_In.wav", 0.5)),
               (0.25, "taiko", 0.35), (1.1, "taiko", 0.45), (INTRO - 0.55, "whoosh_rev", 0.6)]
    t += INTRO
    events.append((t, "braam", None))
    music_start = t

    # --- Misiones ------------------------------------------------------------------------------
    for mi, (num, name, clips) in enumerate(MISSIONS):
        block_start = t
        block_len = sum(c[2] for c in clips)
        for ci, (tag, f0, dur, speed, phase, sfx) in enumerate(clips):
            timeline.append(("clip", t, dur, dict(tag=tag, f0=f0, speed=speed, phase=phase,
                                                  title=(num, name, block_start, block_len),
                                                  hard=(ci == 0))))
            events.append((t, "hit" if ci == 0 else "whoosh", None))
            for (st, path, g) in sfx:
                events.append((t + st, "sfx", (path, g)))
            t += dur

    # --- Montaje rápido -----------------------------------------------------------------------
    montage_start = t
    for (tag, f0, dur, speed) in MONTAGE:
        timeline.append(("clip", t, dur, dict(tag=tag, f0=f0, speed=speed, phase=None, title=None, hard=True)))
        events.append((t, "taiko", 0.55))
        t += dur
    events.append((montage_start, "riser", t - montage_start))
    music_end = t
    events.append((music_start, "ostinato", montage_start - music_start))

    # --- Silencio y logo ----------------------------------------------------------------------
    timeline.append(("black", t, SILENCE, None))
    t += SILENCE
    events.append((t, "boom", None))
    events.append((t, "braam", None))
    timeline.append(("logo", t, LOGO, None))
    t += LOGO
    total = t

    # =========================== audio ===========================
    mix = TA.Mix(total)
    for (et, kind, extra) in events:
        if kind == "drone":
            mix.add(TA.drone(extra), et, 1.0)
        elif kind == "braam":
            mix.add(TA.braam(), et, 1.0)
        elif kind == "boom":
            mix.add(TA.boom(3.0, gain=1.2), et, 1.0)
        elif kind == "hit":
            mix.add(TA.hit(), et, 0.9)
            mix.add(TA.whoosh(0.5, True), et - 0.45, 0.6)
        elif kind == "whoosh":
            mix.add(TA.whoosh(0.45), et - 0.2, 0.55)
            mix.add(TA.taiko(0.5, 70, 0.5), et, 1.0)
        elif kind == "whoosh_rev":
            mix.add(TA.whoosh(extra, True), et, 0.8)
        elif kind == "taiko":
            mix.add(TA.taiko(0.6, 75, extra), et, 1.0)
        elif kind == "riser":
            mix.add(TA.riser(extra), et, 0.9)
        elif kind == "ostinato":
            o = TA.ostinato(extra + 0.3)
            mix.add(o, et, 1.0)
            # taikos en negras
            k = 0
            while k * 0.5 < extra:
                mix.add(TA.taiko(0.6, 62 if k % 4 == 0 else 85, 0.6 if k % 2 == 0 else 0.35), et + k * 0.5, 1.0)
                k += 1
        elif kind == "sfx":
            path, g = extra
            mix.add(TA.load(path), et, g)
    mix.duck(music_end, music_end + SILENCE, 0.0)
    wav = os.path.splitext(out_mp4)[0] + ".wav"
    mix.write(wav, total)

    # =========================== vídeo ===========================
    f_title_small = load_font(20)
    f_title_big = load_font(54)
    f_phase = load_font(18, bold=False)
    f_intro = load_font(34)
    f_logo = load_font(118)
    f_tag = load_font(26)
    f_small = load_font(16, bold=False)

    ff = imageio_ffmpeg.get_ffmpeg_exe()
    cmd = [ff, "-y", "-loglevel", "error",
           "-f", "rawvideo", "-pix_fmt", "rgb24", "-s", f"{W}x{H}", "-r", str(FPS), "-i", "-",
           "-i", wav, "-c:v", "libx264", "-preset", "slow", "-crf", "17", "-pix_fmt", "yuv420p",
           "-c:a", "aac", "-b:a", "256k", "-shortest", "-movflags", "+faststart", out_mp4]
    proc = subprocess.Popen(cmd, stdin=subprocess.PIPE)

    nframes = int(round(total * FPS))
    logo_bg = None
    for n in range(nframes):
        tt = n / FPS
        seg = next(s for s in timeline if s[1] <= tt + 1e-6 < s[1] + s[2] + 1e-6)
        kind, st, dur, d = seg
        lt = tt - st
        layer = overlay_layer()
        draw = ImageDraw.Draw(layer)

        if kind == "intro":
            img = np.zeros((H, W, 3), np.float32)
            lines = [(0.15, 1.0, "KESSRA · 2031"), (1.0, INTRO, "UNA CIUDAD PARTIDA EN DOS")]
            for a, b, txt in lines:
                if a <= lt < b:
                    k = min(1, (lt - a) / 0.12) * min(1, (b - lt) / 0.1)
                    w = spaced_width(draw, txt, f_intro, 6)
                    spaced(draw, ((W - w) / 2, H / 2 - 22), txt, f_intro, (235, 235, 235, int(255 * k)), 6)
            img = over(img, to_np(layer))
            img = finish(img)

        elif kind == "black":
            img = finish(np.zeros((H, W, 3), np.float32))

        elif kind == "clip":
            fidx = d["f0"] + int(lt * FPS * d["speed"])
            img = grade(fr.get(d["tag"], fidx))
            # empuje lento + golpe de zoom al entrar
            punch = 0.10 * max(0, 1 - lt / 0.22) ** 2 if d["hard"] else 0.05 * max(0, 1 - lt / 0.18) ** 2
            zoom = 1.02 + 0.035 * (lt / dur) + punch
            sh = 9 * max(0, 1 - lt / 0.35) if d["hard"] else 4 * max(0, 1 - lt / 0.2)
            dx = GRAIN_RNG.uniform(-sh, sh)
            dy = GRAIN_RNG.uniform(-sh, sh)
            img = transform(img, zoom, dx, dy)
            img = aberration(img, int(6 * max(0, 1 - lt / 0.12)))
            # destello al cortar
            fl = (0.75 if d["hard"] else 0.3) * max(0, 1 - lt / (0.12 if d["hard"] else 0.07))
            img = img + fl
            # títulos de misión y fase
            if d["title"]:
                num, name, bs, bl = d["title"]
                bt = tt - bs
                slide = min(1, bt / 0.18)
                slide = 1 - (1 - slide) ** 3
                fade = min(1, (bs + bl - tt) / 0.12)
                a = int(255 * fade)
                x = 64 - 40 * (1 - slide)
                y = H - BAR - 128
                draw.rectangle([x, y + 4, x + 4, y + 92], fill=AMBER + (a,))
                spaced(draw, (x + 18, y), f"MISIÓN {num}", f_title_small, AMBER + (a,), 4)
                spaced(draw, (x + 16, y + 22), name, f_title_big, (245, 245, 245, a), 3)
                if d["phase"]:
                    pa = int(a * min(1, lt / 0.15))
                    spaced(draw, (x + 18, y + 86), d["phase"], f_phase, (200, 200, 200, pa), 3)
            img = over(np.clip(img, 0, 1), to_np(layer))
            img = finish(img)

        elif kind == "logo":
            if logo_bg is None:
                tag, f = LOGO_BG
                bg = Image.fromarray((grade(fr.get(tag, f)) * 255).astype(np.uint8)).filter(ImageFilter.GaussianBlur(6))
                logo_bg = np.asarray(bg, dtype=np.float32) / 255 * 0.32
            img = logo_bg.copy()
            img = transform(img, 1.0 + 0.04 * lt / LOGO, 0, 0)
            # logo: entra grande y se asienta con temblor
            k = min(1, lt / 0.16)
            size_k = 1 + 0.35 * (1 - k) ** 2
            txt = "BLACKLINE"
            sp = 18
            w = spaced_width(draw, txt, f_logo, sp)
            tl = Image.new("RGBA", (int(w) + 40, 170), (0, 0, 0, 0))
            spaced(ImageDraw.Draw(tl), (20, 10), txt, f_logo, (245, 245, 245, 255), sp)
            # línea negra que corta el logo
            ImageDraw.Draw(tl).rectangle([0, 82, tl.width, 92], fill=(0, 0, 0, 255))
            tl = tl.resize((int(tl.width * size_k), int(tl.height * size_k)), Image.BICUBIC)
            shk = 10 * max(0, 1 - lt / 0.45)
            ox = int((W - tl.width) / 2 + GRAIN_RNG.uniform(-shk, shk))
            oy = int(H / 2 - 95 * size_k + GRAIN_RNG.uniform(-shk, shk))
            tl.putalpha(tl.getchannel("A").point(lambda v: int(v * k)))
            layer.alpha_composite(tl, (max(0, ox), max(0, oy)))
            if lt > 0.7:
                a = int(255 * min(1, (lt - 0.7) / 0.3))
                tg = "CRUZA LA LÍNEA"
                w2 = spaced_width(draw, tg, f_tag, 10)
                spaced(draw, ((W - w2) / 2, H / 2 + 62), tg, f_tag, AMBER + (a,), 10)
            if lt > 1.3:
                a = int(200 * min(1, (lt - 1.3) / 0.4))
                s2 = "5 MISIONES  ·  CAMPAÑA PARA UN JUGADOR  ·  UNREAL ENGINE 5"
                w3 = spaced_width(draw, s2, f_small, 3)
                spaced(draw, ((W - w3) / 2, H - BAR - 46), s2, f_small, (190, 190, 190, a), 3)
            img = img + 0.9 * max(0, 1 - lt / 0.1)
            img = aberration(np.clip(img, 0, 1), int(8 * max(0, 1 - lt / 0.25)))
            img = over(img, to_np(layer))
            fade_out = min(1, (LOGO - lt) / 0.35)
            img = finish(img * fade_out)

        proc.stdin.write((img * 255).astype(np.uint8).tobytes())
        if n % 60 == 0:
            print(f"  {n}/{nframes}", flush=True)
    proc.stdin.close()
    proc.wait()
    print(f"OK {out_mp4} ({total:.1f} s)")


if __name__ == "__main__":
    build(sys.argv[1], sys.argv[2])
