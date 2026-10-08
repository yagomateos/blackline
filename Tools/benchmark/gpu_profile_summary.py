"""Resume el volcado de ProfileGPU de un log de Unreal: pases con tiempo inclusivo >= umbral, hasta cierta profundidad.

Uso: python -I gpu_profile_summary.py <log> [umbral_ms=0.3] [profundidad=4]
"""
import re
import sys

path = sys.argv[1]
thr = float(sys.argv[2]) if len(sys.argv) > 2 else 0.3
max_depth = int(sys.argv[3]) if len(sys.argv) > 3 else 4
row = re.compile(r"┃.*┃.*│\s*([\d.]+) ms ┃(\s*)(\S.*?)\s*┃?\s*$")
with open(path, encoding="utf-8", errors="replace") as f:
    lines = f.read().splitlines()
start = next((i for i, l in enumerate(lines) if "GPU Profile for Frame" in l), None)
if start is None:
    sys.exit("Sin perfil de GPU en el log")
print(lines[start].split("Display: ")[-1])
for l in lines[start:start + 3000]:
    if "Frame Time" in l:
        print(l.split("Display: ")[-1].strip())
    m = row.search(l)
    if not m:
        continue
    ms, indent, name = float(m.group(1)), len(m.group(2)), m.group(3)
    depth = (indent - 1) // 3
    if ms >= thr and depth <= max_depth:
        print(f"{'  ' * depth}{ms:7.3f} ms  {name}")
