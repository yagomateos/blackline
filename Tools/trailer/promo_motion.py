"""Promo de BLACKLINE en motion graphics (~30 s): textos que golpean a ritmo + planos de acción.

Reutiliza la gradación/efectos de build_trailer.py y el audio de trailer_audio.py.
Uso:  python promo_motion.py <carpeta_fotogramas> <salida.mp4>
"""
import os
import subprocess
import sys

import imageio_ffmpeg
import numpy as np
from PIL import Image, ImageDraw, ImageFilter, ImageFont

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import build_trailer as B  # noqa: E402
import trailer_audio as TA  # noqa: E402

W, H, FPS, BAR, AMBER = B.W, B.H, B.FPS, B.BAR, B.AMBER
AR7, BOOM, ROTOR = B.AR7, B.BOOM, B.ROTOR
burst = B.burst
FLESH = ["Impacts/Flesh/" + f for f in sorted(os.listdir(os.path.join(TA.SFX, "Impacts", "Flesh")))]


def C(tag, f0, dur, word=None, sfx=(), speed=1.0, rapid=False):
    return ("clip", dur, dict(tag=tag, f0=f0, speed=speed, word=word, sfx=list(sfx), rapid=rapid))


def T(dur, big, small=None, style="slam"):
    return ("card", dur, dict(big=big, small=small, style=style))


SEQ = []          # se rellena abajo (ver main)
LOGO_BG = ("M5", 2790)


# ---------------------------------------------------------------------------------------------
def render(frames_root, out_mp4, seq):
    fr = B.Frames(frames_root)
    timeline, events = [], []
    t = 0.0
    first_clip = True
    rapid_start = None
    for kind, dur, d in seq:
        timeline.append((kind, t, dur, d))
        if kind == "card":
            events.append((t, "hit" if d["style"] != "soft" else "taiko", 0.6))
        elif kind == "clip":
            if first_clip:
                events.append((t, "braam", None))
                events.append((t, "music_on", None))
                first_clip = False
            else:
                events.append((t, "whoosh", None))
            for st, path, g in d["sfx"]:
                events.append((t + st, "sfx", (path, g)))
            if d.get("rapid") and rapid_start is None:
                rapid_start = t
        elif kind == "black":
            events.append((t, "silence", dur))
        elif kind == "logo":
            events.append((t, "boom", None))
            events.append((t, "braam", None))
        t += dur
    total = t

    # ----------------------------- audio -----------------------------
    mix = TA.Mix(total)
    music_on = next(e[0] for e in events if e[1] == "music_on")
    silence = next(e for e in events if e[1] == "silence")
    mix.add(TA.drone(music_on + 0.6), 0.0, 1.0)
    mix.add(TA.load("Radio/SW_Radio_In.wav"), 0.05, 0.5)
    o = TA.ostinato(silence[0] - music_on + 0.3)
    mix.add(o, music_on, 1.0)
    k = 0
    while music_on + k * 0.5 < silence[0]:
        mix.add(TA.taiko(0.6, 62 if k % 4 == 0 else 85, 0.6 if k % 2 == 0 else 0.35), music_on + k * 0.5, 1.0)
        k += 1
    if rapid_start:
        mix.add(TA.riser(silence[0] - rapid_start), rapid_start, 0.9)
    for (et, kind, extra) in events:
        if kind == "braam":
            mix.add(TA.braam(), et, 1.0)
        elif kind == "boom":
            mix.add(TA.boom(3.0, gain=1.2), et, 1.0)
        elif kind == "hit":
            mix.add(TA.hit(), et, 0.9)
            mix.add(TA.whoosh(0.4, True), et - 0.35, 0.5)
        elif kind == "taiko":
            mix.add(TA.taiko(0.6, 75, extra), et, 1.0)
        elif kind == "whoosh":
            mix.add(TA.whoosh(0.35), et - 0.15, 0.45)
        elif kind == "sfx":
            path, g = extra
            mix.add(TA.load(path), et, g)
    mix.duck(silence[0], silence[0] + silence[2], 0.0)
    wav = os.path.splitext(out_mp4)[0] + ".wav"
    mix.write(wav, total)

    # ----------------------------- vídeo -----------------------------
    f_big = B.load_font(150)
    f_mid = B.load_font(30)
    f_word = B.load_font(96)
    f_logo = B.load_font(118)
    f_tag = B.load_font(26)
    f_small = B.load_font(16, bold=False)

    ff = imageio_ffmpeg.get_ffmpeg_exe()
    cmd = [ff, "-y", "-loglevel", "error", "-f", "rawvideo", "-pix_fmt", "rgb24", "-s", f"{W}x{H}",
           "-r", str(FPS), "-i", "-", "-i", wav, "-c:v", "libx264", "-preset", "medium", "-crf", "18",
           "-vf", "scale=1920:1080:flags=lanczos", "-pix_fmt", "yuv420p", "-c:a", "aac", "-b:a", "256k", "-shortest", "-movflags", "+faststart", out_mp4]
    proc = subprocess.Popen(cmd, stdin=subprocess.PIPE)
    rng = np.random.default_rng(11)
    last_clip_img = np.zeros((H, W, 3), np.float32)
    logo_bg = None
    nframes = int(round(total * FPS))

    def text_layer(txt, font, spacing, scale, alpha, color=(245, 245, 245)):
        d0 = ImageDraw.Draw(Image.new("RGBA", (1, 1)))
        w = B.spaced_width(d0, txt, font, spacing)
        tl = Image.new("RGBA", (int(w) + 40, font.size + 50), (0, 0, 0, 0))
        B.spaced(ImageDraw.Draw(tl), (20, 10), txt, font, color + (255,), spacing)
        if abs(scale - 1) > 0.01:
            tl = tl.resize((max(1, int(tl.width * scale)), max(1, int(tl.height * scale))), Image.BICUBIC)
        if alpha < 1:
            tl.putalpha(tl.getchannel("A").point(lambda v: int(v * alpha)))
        return tl

    f_mono = ImageFont.truetype("C:/Windows/Fonts/consola.ttf", 15)
    LABEL = {"M1": "OP-01 KESSRA ESTE", "M2": "OP-02 REFINERÍA", "M3": "OP-03 CASCO VIEJO", "M4": "OP-04 PUENTE",
             "M5": "OP-05 TERMINAL", "WP": "CAMPO DE TIRO", "AI": "CONTACTO HOSTIL"}

    def hud(dr, tt, d):
        c = (235, 235, 235, 170)
        m, L = 40, 34
        for (x, y, sx, sy) in ((m, BAR + 14, 1, 1), (W - m, BAR + 14, -1, 1), (m, H - BAR - 14, 1, -1), (W - m, H - BAR - 14, -1, -1)):
            dr.line([(x, y), (x + sx * L, y)], fill=c, width=2)
            dr.line([(x, y), (x, y + sy * L)], fill=c, width=2)
        dr.text((m + 14, BAR + 22), f"SABLE 2-1  //  {LABEL.get(d['tag'], '')}", font=f_mono, fill=c)
        if int(tt * 2) % 2 == 0:
            dr.ellipse([W - m - 128, BAR + 25, W - m - 118, BAR + 35], fill=(220, 40, 40, 230))
        dr.text((W - m - 110, BAR + 22), "REC %02d:%02d:%02d" % (int(tt) // 60, int(tt) % 60, int(tt * 30) % 30), font=f_mono, fill=c)
        dr.text((W - m - 230, H - BAR - 40), "41°37'N 19°08'E  ALT 012", font=f_mono, fill=c)

    for n in range(nframes):
        tt = n / FPS
        kind, st, dur, d = next(s for s in timeline if s[1] <= tt + 1e-6 < s[1] + s[2] + 1e-6)
        lt = tt - st
        layer = Image.new("RGBA", (W, H), (0, 0, 0, 0))
        draw = ImageDraw.Draw(layer)

        if kind == "clip":
            fidx = d["f0"] + int(lt * FPS * d["speed"])
            img = B.grade(fr.get(d["tag"], fidx))
            punch = 0.12 * max(0, 1 - lt / 0.2) ** 2
            zoom = 1.03 + 0.05 * (lt / dur) + punch
            sh = 10 * max(0, 1 - lt / 0.3) + (3 if d["sfx"] else 0)
            img = B.transform(img, zoom, rng.uniform(-sh, sh), rng.uniform(-sh, sh))
            img = B.aberration(img, int(7 * max(0, 1 - lt / 0.12)))
            img = img + 0.6 * max(0, 1 - lt / 0.09)
            if lt < 0.1:  # glitch: bandas desplazadas al cortar
                for _ in range(5):
                    y0 = int(rng.uniform(BAR, H - BAR - 40)); hh = int(rng.uniform(8, 40))
                    img[y0:y0 + hh] = np.roll(img[y0:y0 + hh], int(rng.uniform(-60, 60)), axis=1)
            last_clip_img = img
            hud(draw, tt, d)
            if d["word"]:
                k = min(1, lt / 0.12)
                tl = text_layer(d["word"], f_word, 10, 1 + 0.5 * (1 - k) ** 2, min(1, lt / 0.08) * min(1, (dur - lt) / 0.08))
                layer.alpha_composite(tl, (int(70 - 30 * (1 - k)), int(H - BAR - tl.height - 20)))
                draw.rectangle([70, H - BAR - 22, 70 + int(260 * k), H - BAR - 16], fill=AMBER + (255,))
            img = B.over(np.clip(img, 0, 1), B.to_np(layer))
            img = B.finish(img)

        elif kind == "card":
            # fondo: último plano muy oscuro y desenfocado + franja ámbar que barre
            bg = Image.fromarray((np.clip(last_clip_img, 0, 1) * 255).astype(np.uint8)).resize((160, 90)).resize((W, H), Image.BILINEAR)
            img = np.asarray(bg, dtype=np.float32) / 255 * 0.22
            wipe = min(1, lt / 0.14)
            x1 = int(-W * 0.3 + W * 1.6 * wipe)
            draw.polygon([(x1 - 520, H / 2 + 120), (x1 - 380, H / 2 + 120), (x1 - 300, H / 2 - 120), (x1 - 440, H / 2 - 120)],
                         fill=AMBER + (int(200 * (1 - wipe * 0.6)),))
            k = min(1, lt / 0.1)
            out_k = min(1, (dur - lt) / 0.07)
            if d["style"] == "slide":
                tl = text_layer(d["big"], f_big, 14, 1.0, out_k)
                x = int((W - tl.width) / 2 + 220 * (1 - k) ** 3 - 30 * lt)
            else:
                tl = text_layer(d["big"], f_big, 14, 1 + 0.6 * (1 - k) ** 2, k * out_k)
                x = int((W - tl.width) / 2)
            shk = 12 * max(0, 1 - lt / 0.25)
            y = int(H / 2 - tl.height / 2 - 20)
            layer.alpha_composite(tl, (max(0, x + int(rng.uniform(-shk, shk))), max(0, y + int(rng.uniform(-shk, shk)))))
            # la "línea negra": barra que cruza el texto
            draw.rectangle([0, H / 2 - 12, int(W * min(1, lt / 0.2)), H / 2 - 4], fill=(0, 0, 0, 255))
            if d["small"] and lt > 0.08:
                ts = text_layer(d["small"], f_mid, 8, 1.0, min(1, (lt - 0.08) / 0.1) * out_k, AMBER)
                layer.alpha_composite(ts, (int((W - ts.width) / 2), int(H / 2 + 70)))
            img = img + 0.5 * max(0, 1 - lt / 0.06)
            img = B.over(np.clip(img, 0, 1), B.to_np(layer))
            img = B.aberration(img, int(5 * max(0, 1 - lt / 0.1)))
            img = B.finish(img)

        elif kind == "black":
            img = B.finish(np.zeros((H, W, 3), np.float32))

        elif kind == "logo":
            if logo_bg is None:
                bg = Image.fromarray((B.grade(fr.get(*LOGO_BG)) * 255).astype(np.uint8)).filter(ImageFilter.GaussianBlur(6))
                logo_bg = np.asarray(bg, dtype=np.float32) / 255 * 0.32
            img = B.transform(logo_bg.copy(), 1.0 + 0.04 * lt / dur, 0, 0)
            k = min(1, lt / 0.16)
            size_k = 1 + 0.35 * (1 - k) ** 2
            tl = text_layer("BLACKLINE", f_logo, 18, 1.0, 1.0)
            ImageDraw.Draw(tl).rectangle([0, 82, tl.width, 92], fill=(0, 0, 0, 255))
            tl = tl.resize((int(tl.width * size_k), int(tl.height * size_k)), Image.BICUBIC)
            tl.putalpha(tl.getchannel("A").point(lambda v: int(v * k)))
            shk = 10 * max(0, 1 - lt / 0.45)
            layer.alpha_composite(tl, (max(0, int((W - tl.width) / 2 + rng.uniform(-shk, shk))),
                                       max(0, int(H / 2 - 95 * size_k + rng.uniform(-shk, shk)))))
            if lt > 0.6:
                ts = text_layer("CRUZA LA LÍNEA", f_tag, 10, 1.0, min(1, (lt - 0.6) / 0.3), AMBER)
                layer.alpha_composite(ts, (int((W - ts.width) / 2), int(H / 2 + 52)))
            if lt > 1.1:
                ts = text_layer("5 MISIONES  ·  CAMPAÑA PARA UN JUGADOR  ·  UNREAL ENGINE 5", f_small, 3, 1.0,
                                0.8 * min(1, (lt - 1.1) / 0.4), (190, 190, 190))
                layer.alpha_composite(ts, (int((W - ts.width) / 2), H - BAR - 56))
            img = img + 0.9 * max(0, 1 - lt / 0.1)
            img = B.aberration(np.clip(img, 0, 1), int(8 * max(0, 1 - lt / 0.25)))
            img = B.over(img, B.to_np(layer))
            img = B.finish(img * min(1, (dur - lt) / 0.35))

        proc.stdin.write((np.clip(img, 0, 1) * 255).astype(np.uint8).tobytes())
        if n % 120 == 0:
            print(f"  {n}/{nframes}", flush=True)
    proc.stdin.close()
    proc.wait()
    os.remove(wav)
    print(f"OK {out_mp4} ({total:.1f} s)")


if __name__ == "__main__":
    import promo_seq
    render(sys.argv[1], sys.argv[2], promo_seq.SEQ)
