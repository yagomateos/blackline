"""Benchmark de rendimiento de BLACKLINE.

Lanza el juego (-game) en L_Benchmark con varias configuraciones de render, captura un CSV
por configuración con el CSV Profiler de Unreal y resume los resultados en Docs/Benchmark.md.

Uso (con el Python que trae Unreal):
  "C:/Program Files/Epic Games/UE_5.8/Engine/Binaries/ThirdParty/Python3/Win64/python.exe" \
      Tools/benchmark/run_benchmark.py [config1 config2 ...]
"""
import csv
csv.field_size_limit(2**31 - 1)  # la fila final de metadatos del CSV de Unreal es muy larga
import glob
import os
import statistics
import subprocess
import sys
import time

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
EDITOR = r"C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe"
UPROJECT = os.path.join(ROOT, "Blackline.uproject")
# En -game con motor instalado, el CSV se escribe en el Saved del usuario, no en el del proyecto
CSV_DIR = os.path.join(os.environ["LOCALAPPDATA"], "UnrealEngine", "5.8", "Saved", "Profiling", "CSV")
MAP = "/Game/Maps/Benchmark/L_Benchmark"

CAPTURE_FRAMES = 1500
WARMUP_FRAMES = 500  # se descartan (streaming, PSO, auto-exposición)

BASE = ("r.SetRes 1920x1080w, r.VSync 0, t.MaxFPS 0, sg.ViewDistanceQuality 2, sg.ShadowQuality 2, "
        "sg.GlobalIlluminationQuality 2, sg.ReflectionQuality 2, sg.PostProcessQuality 2, "
        "sg.TextureQuality 2, sg.EffectsQuality 2, sg.ShadingQuality 2")

CONFIGS = {
    "warmup":             (BASE, "Compila shaders/PSO; no se reporta"),
    "lumen_vsm_100":      (BASE + ", r.ScreenPercentage 100",
                           "Lumen GI+reflejos, Virtual Shadow Maps, 1080p nativo"),
    "lumen_vsm_75":       (BASE + ", r.ScreenPercentage 75",
                           "Lumen + VSM, TSR al 75% (810p interno)"),
    "lumen_csm_75":       (BASE + ", r.ScreenPercentage 75, r.Shadow.Virtual.Enable 0",
                           "Lumen + shadow maps clásicos, TSR 75%"),
    "nolumen_csm_75":     (BASE + ", r.ScreenPercentage 75, r.Shadow.Virtual.Enable 0, "
                                  "r.DynamicGlobalIlluminationMethod 0, r.ReflectionMethod 2",
                           "Sin Lumen (≈coste de iluminación baked), SSR, shadow maps, TSR 75%"),
    "nolumen_vsm_75":     (BASE + ", r.ScreenPercentage 75, "
                                  "r.DynamicGlobalIlluminationMethod 0, r.ReflectionMethod 2",
                           "Sin Lumen (≈coste de iluminación baked), SSR, VSM, TSR 75%"),
    "lumen_vsm_67":       (BASE + ", r.ScreenPercentage 67",
                           "Lumen + VSM, TSR al 67% (720p interno, preset 'Rendimiento')"),
}


def run_config(name, timeout_s=1800):
    """Lanza el juego y espera a que aparezca el CSV. -csvExitOnCompletion no cierra el proceso
    en builds de editor, así que lo cerramos nosotros cuando el CSV está escrito."""
    cmds, _ = CONFIGS[name]
    before = set(glob.glob(os.path.join(CSV_DIR, "*.csv")))
    args = [EDITOR, UPROJECT, MAP, "-game", "-windowed", "-ResX=1920", "-ResY=1080",
            "-nosplash", "-NoSound", "-unattended",
            f"-csvCaptureFrames={CAPTURE_FRAMES}", "-csvExitOnCompletion",
            f"-ExecCmds={cmds}"]
    print(f"\n=== {name}: {cmds}")
    t0 = time.time()
    proc = subprocess.Popen(args)
    found = None
    while time.time() - t0 < timeout_s:
        new = set(glob.glob(os.path.join(CSV_DIR, "*.csv"))) - before
        if new:
            # El CSV se crea vacío al empezar la captura; se da por terminado cuando
            # tiene datos y deja de crecer durante 6 s.
            cand = max(new, key=os.path.getmtime)
            size = os.path.getsize(cand)
            if size > 0:
                time.sleep(6)
                if os.path.getsize(cand) == size:
                    found = cand
                    break
        if proc.poll() is not None:
            break
        time.sleep(3)
    if proc.poll() is None:
        proc.kill()
        proc.wait()
    print(f"   terminado en {time.time() - t0:.0f}s -> {found}")
    return found


def pick(header, *candidates):
    for c in candidates:
        if c in header:
            return header.index(c)
    for c in candidates:
        for i, h in enumerate(header):
            if c.lower() in h.lower():
                return i
    return None


def analyze(path):
    with open(path, newline="", encoding="utf-8", errors="ignore") as f:
        rows = list(csv.reader(f))
    header = rows[0]
    # Las filas de datos llevan una coma final (una columna más); se excluyen la cabecera repetida y los metadatos
    data = [r for r in rows[1:] if len(r) >= len(header) and r[1] != header[1] and not r[0].startswith("[")]
    data = data[WARMUP_FRAMES:]
    idx = {
        "frame": pick(header, "FrameTime"),
        "game": pick(header, "GameThreadTime"),
        "render": pick(header, "RenderThreadTime"),
        "gpu": pick(header, "GPUTime", "GPU/Total", "GPU"),
        "vram": pick(header, "GPUMem/LocalUsedMB"),
    }

    def col(i):
        out = []
        for r in data:
            try:
                out.append(float(r[i]))
            except (ValueError, IndexError):
                pass
        return out

    res = {"frames": len(data), "columns": {k: (header[v] if v is not None else None) for k, v in idx.items()}}
    for k, i in idx.items():
        if i is None:
            continue
        v = col(i)
        if v:
            v_sorted = sorted(v)
            res[k] = {"avg": statistics.mean(v), "p95": v_sorted[int(len(v) * 0.95) - 1],
                      "p99": v_sorted[int(len(v) * 0.99) - 1], "max": v_sorted[-1]}
    return res


def main():
    sys.stdout.reconfigure(encoding="utf-8")
    names = sys.argv[1:] or list(CONFIGS)
    results = {}
    for n in names:
        path = run_config(n)
        if path and n != "warmup":
            results[n] = analyze(path)
            r = results[n]
            if "frame" in r:
                print(f"   frames={r['frames']} avg={r['frame']['avg']:.2f}ms "
                      f"({1000 / r['frame']['avg']:.0f} fps) p99={r['frame']['p99']:.2f}ms")
    if not results:
        return
    lines = ["# Benchmark de rendimiento", "",
             f"Mapa: `{MAP}` · 1920x1080 ventana · {CAPTURE_FRAMES} frames ({WARMUP_FRAMES} de calentamiento descartados)",
             "Hardware: GTX 1660 Super 6 GB · Ryzen 5 3600 · 16 GB RAM · build editor `-game` (≈Development)", "",
             "| Config | Descripción | FPS medio | Frame ms (avg / p99) | GPU ms (avg / p95) | Game ms | Render ms | VRAM máx MB |",
             "|---|---|---|---|---|---|---|---|"]
    for n, r in results.items():
        def f(k, a="avg", b="p99"):
            return f"{r[k][a]:.1f} / {r[k][b]:.1f}" if k in r else "n/d"
        fps = f"{1000 / r['frame']['avg']:.0f}" if "frame" in r else "n/d"
        g = f"{r['game']['avg']:.1f}" if "game" in r else "n/d"
        rt = f"{r['render']['avg']:.1f}" if "render" in r else "n/d"
        vr = f"{r['vram']['max']:.0f}" if "vram" in r else "n/d"
        lines.append(f"| `{n}` | {CONFIGS[n][1]} | {fps} | {f('frame')} | {f('gpu', 'avg', 'p95')} | {g} | {rt} | {vr} |")
    lines += ["", "Columnas CSV usadas: " + str(next(iter(results.values()))["columns"]), ""]
    out = os.path.join(ROOT, "Docs", "Benchmark.md")
    with open(out, "w", encoding="utf-8") as fh:
        fh.write("\n".join(lines))
    print("\n".join(lines))


if __name__ == "__main__":
    main()
