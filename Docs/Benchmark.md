# Benchmark de rendimiento

Mapa: `/Game/Maps/Benchmark/L_Benchmark` · 1920x1080 ventana · 1500 frames (500 de calentamiento descartados)
Hardware: GTX 1660 Super 6 GB · Ryzen 5 3600 · 16 GB RAM · build editor `-game` (≈Development)

| Config | Descripción | FPS medio | Frame ms (avg / p99) | GPU ms (avg / p95) | Game ms | Render ms | VRAM máx MB |
|---|---|---|---|---|---|---|---|
| `lumen_vsm_100` | Lumen GI+reflejos, Virtual Shadow Maps, 1080p nativo | 55 | 18.2 / 19.8 | 17.0 / 17.8 | 2.3 | 18.2 | 2578 |
| `lumen_vsm_75` | Lumen + VSM, TSR al 75% (810p interno) | 71 | 14.1 / 15.5 | 13.0 / 13.5 | 2.1 | 14.1 | 2497 |
| `lumen_vsm_67` | Lumen + VSM, TSR al 67% (720p interno, preset 'Rendimiento') | 77 | 12.9 / 14.4 | 11.8 / 12.4 | 2.2 | 12.9 | 2470 |
| `lumen_csm_75` | Lumen + shadow maps clásicos, TSR 75% | 63 | 16.0 / 17.4 | 14.8 / 15.5 | 2.2 | 16.0 | 2333 |
| `nolumen_vsm_75` | Sin Lumen (≈coste de iluminación baked), SSR, VSM, TSR 75% | 84 | 11.9 / 13.6 | 10.8 / 11.5 | 2.1 | 11.9 | 2478 |
| `nolumen_csm_75` | Sin Lumen (≈coste de iluminación baked), SSR, shadow maps, TSR 75% | 73 | 13.7 / 15.3 | 12.6 / 13.4 | 2.1 | 13.7 | 2349 |

Columnas CSV usadas: {'frame': 'FrameTime', 'game': 'GameThreadTime', 'render': 'RenderThreadTime', 'gpu': 'GPUTime', 'vram': 'GPUMem/LocalUsedMB'}
![Vista del benchmark](img/benchmark_view.png)

## Análisis (2026-10-07)

- **Lumen cuesta ~2,2 ms de GPU** (`lumen_vsm_75` 13,0 ms vs `nolumen_vsm_75` 10,8 ms). Asumible.
- **Con Nanite, VSM es más barato que shadow maps clásicos** (~1,8 ms menos): los CSM re-renderizan toda la geometría en cada cascada; VSM cachea páginas y aprovecha Nanite.
- **TSR 75% vs nativo ahorra ~4 ms**. Calidad visual con TSR a 810p interno es buena en 1080p.
- **VRAM ~2,5 GB** con texturas de prueba: deja ~2 GB de margen para texturas reales, personajes y VFX.
- CPU (game thread 2,2 ms) sobrada; el cuello de botella es la GPU.

## Decisión

| Ajuste | Valor por defecto ("Alto") |
|---|---|
| Iluminación global | **Lumen** (GI + reflejos, calidad High) |
| Sombras | **Virtual Shadow Maps** |
| Geometría de entorno | **Nanite** |
| Anti-aliasing / escalado | **TSR al 75%** |
| Preset "Rendimiento" | Lumen + VSM + TSR 67% (77 fps) |
| Preset "Bajo" | Sin Lumen (SkyLight + AO) + VSM + TSR 67% |

**Margen:** escena de prueba a 13 ms GPU → quedan ~3,6 ms hasta 16,6 ms (60 fps) para personajes, armas,
Niagara, decals, translucidez y post-proceso. Es justo: hay que re-medir en el Bloque 8 (arte) con
contenido real, y vigilar especialmente translucidez (humo) y número de luces con sombra.

## Cómo repetirlo
```
"C:/Program Files/Epic Games/UE_5.8/Engine/Binaries/ThirdParty/Python3/Win64/python.exe" Tools/benchmark/run_benchmark.py [configs...]
```
Notas: en builds de editor `-csvExitOnCompletion` no cierra el juego (el script lo cierra al detectar el CSV);
los CSV se escriben en `%LOCALAPPDATA%/UnrealEngine/5.8/Saved/Profiling/CSV`.
