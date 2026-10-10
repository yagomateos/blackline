"""Hoja de contactos de capturas (para revisar ajustes visuales de un vistazo). Usa el Python de Blender (numpy).

Uso: blender -b --factory-startup -P Tools/contact_sheet.py -- <salida.png> <columnas> <ancho_celda> <img1> <img2> ...
"""
import sys

import bpy
import numpy as np

argv = sys.argv[sys.argv.index("--") + 1:]
out, cols, cell_w = argv[0], int(argv[1]), int(argv[2])
paths = argv[3:]
cells = []
for p in paths:
    img = bpy.data.images.load(p)
    w, h = img.size
    px = np.array(img.pixels[:], dtype=np.float32).reshape(h, w, 4)
    step = max(1, w // cell_w)
    cells.append(px[::step, ::step][: (h // step), : cell_w])
ch, cw = min(c.shape[0] for c in cells), min(c.shape[1] for c in cells)
rows = (len(cells) + cols - 1) // cols
sheet = np.ones((rows * ch, cols * cw, 4), dtype=np.float32)
for i, c in enumerate(cells):
    r, k = divmod(i, cols)
    # pixels de Blender: fila 0 abajo; la primera imagen va arriba a la izquierda
    y0 = (rows - 1 - r) * ch
    sheet[y0:y0 + ch, k * cw:(k + 1) * cw] = c[:ch, :cw]
res = bpy.data.images.new("sheet", cols * cw, rows * ch, alpha=False)
res.pixels.foreach_set(sheet.ravel())
res.filepath_raw = out
res.file_format = 'PNG'
res.save()
print(f"Hoja: {out} ({len(cells)} imágenes)")
