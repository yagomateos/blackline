"""Hojas de contactos de los fotogramas volcados, para elegir los planos del promo.

Uso: python sheets.py <carpeta_fotogramas>/M2 <salida_prefijo> [paso=15]
Genera <prefijo>_NN.png (8x6 miniaturas, cada una con su número de fotograma y la fase de la prueba).
"""
import os
import re
import sys

from PIL import Image, ImageDraw, ImageFont

src, prefix = sys.argv[1], sys.argv[2]
step = int(sys.argv[3]) if len(sys.argv) > 3 else 15
files = sorted(f for f in os.listdir(src) if f.startswith("MovieFrame"))
count = len(files)

# fases de la prueba -> fotograma de película
steps = []
log = os.path.join(src, "game.log")
if os.path.exists(log):
    offset, last, wrap = None, 0, 0
    for line in open(log, encoding="utf-8", errors="ignore"):
        m = re.search(r"\]\[\s*(\d+)\]", line)
        if not m:
            continue
        fr = int(m.group(1))  # el log muestra el contador de fotogramas módulo 1000
        if fr < last - 500:
            wrap += 1000
        last = fr
        fr += wrap
        s = re.search(r'Screenshot "MovieFrame(\d+)"', line)
        if s and offset is None:
            offset = int(s.group(1)) - fr
        s = re.search(r"\[BLTest\] -> (.+)$", line)
        if s:
            steps.append((fr, s.group(1).strip()))
    steps = [(fr + (offset or 0), n) for fr, n in steps]


def phase(i):
    p = ""
    for fr, n in steps:
        if fr <= i:
            p = n
    return p


font = ImageFont.truetype("C:/Windows/Fonts/consola.ttf", 13)
TW, TH, C, R = 200, 112, 8, 6
idx = list(range(0, count, step))
for page in range(0, len(idx), C * R):
    sheet = Image.new("RGB", (C * TW, R * (TH + 16)), (20, 20, 20))
    d = ImageDraw.Draw(sheet)
    for k, i in enumerate(idx[page:page + C * R]):
        im = Image.open(os.path.join(src, "MovieFrame%05d.png" % i)).convert("RGB").resize((TW, TH))
        x, y = (k % C) * TW, (k // C) * (TH + 16)
        sheet.paste(im, (x, y))
        d.text((x + 3, y + TH + 1), f"{i} {phase(i)[:18]}", fill=(255, 220, 120), font=font)
    sheet.save(f"{prefix}_{page // (C * R):02d}.png")
print(count, "fotogramas;", len(steps), "fases:", steps)
