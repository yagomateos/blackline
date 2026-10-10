"""Localiza fogonazos en los fotogramas volcados: picos de brillo en la zona de la boca del arma.

Uso: python find_shots.py <carpeta_fotogramas>  -> imprime, por misión, tramos de 1 s con más disparos.
"""
import json
import os
import sys
from multiprocessing import Pool

import numpy as np
from PIL import Image

ROOT = sys.argv[1]


def score(path):
    im = Image.open(path)
    im.draft("RGB", (320, 180))
    a = np.asarray(im.convert("L").crop((520, 260, 800, 480)).reduce(4), dtype=np.float32)
    return float(np.percentile(a, 99)), float(a.mean())


def main():
    out = {}
    for tag in sorted(os.listdir(ROOT)):
        d = os.path.join(ROOT, tag)
        files = sorted(f for f in os.listdir(d) if f.startswith("MovieFrame"))
        with Pool(8) as p:
            s = np.array(p.map(score, [os.path.join(d, f) for f in files], chunksize=64))
        mean = s[:, 1]
        base = np.array([np.median(mean[max(0, i - 15):i + 15]) for i in range(len(mean))])
        spike = mean - base
        flashes = np.where(spike > 6)[0]
        # tramos de 36 fotogramas con más fogonazos (sin solaparse)
        cnt = np.convolve((spike > 6).astype(int), np.ones(36, int), "valid")
        picks = []
        for i in np.argsort(-cnt):
            if cnt[i] < 3 or len(picks) >= 8:
                break
            if all(abs(i - j) > 60 for j, _ in picks):
                picks.append((int(i), int(cnt[i])))
        out[tag] = picks
        print(tag, len(files), "fogonazos:", len(flashes), "mejores tramos:", picks, flush=True)
    json.dump(out, open(os.path.join(ROOT, "shots.json"), "w"))


if __name__ == "__main__":
    main()
