# Créditos y licencias de recursos externos

Todos los recursos externos usados son **CC0 (dominio público)** salvo que se indique otra cosa.
No requieren atribución, pero se registran aquí por trazabilidad. Los originales descargados se guardan
fuera del repositorio (`../BlacklineDownloads/`); en el repo solo están los derivados procesados.

## Sonido
| Recurso | Fuente | Licencia | Uso |
|---|---|---|---|
| The Free Firearm Sound Library (AR-15 cercano/medio, AK-47 y PPSh ráfagas) | https://opengameart.org/content/the-free-firearm-sound-library | CC0 | Disparo del AR-7 (capas cercana, cola, lejana), tiroteos lejanos del ambiente |
| Gun reload sounds (SpringySpringo) | https://opengameart.org/content/gun-reload-sounds | CC0 | Sacar/meter cargador |
| equipment clicks III (LFA) | https://opengameart.org/content/equipment-clicks-iii | CC0 | Cerrojo, gatillo en vacío |

Procesado: `Tools/audio/process_sfx.py` (recorte por transitorios, capas, remuestreo a 48 kHz).
Impactos por material, casquillos, pasos, foley y viento/rumor del ambiente: **síntesis propia** (mismo script).

**Bloque 7:**
- Combate lejano (`Ambience/Distant/SW_Dist_Burst_*`): ráfagas de AK-47 y PPSh de la misma Free Firearm Sound Library (CC0), filtradas y con ecos (`Tools/audio/gen_world_audio.py`).
- Balas que pasan cerca, ambientes por zona (agua, farolas, viento, sala), explosiones y sirena lejanas, música de combate y golpes musicales: **síntesis propia** (mismo script).
- **Voces (PROVISIONALES)**: radio y barks generados con la síntesis de voz de Windows (voz es-ES "Pablo") y procesados (`Tools/audio/gen_voices.py`). Sirven para el prototipo; **sustituir por actores de voz** antes de publicar nada (las condiciones de uso de las voces de Windows no están pensadas para distribuirlas en un juego).

## Texturas
| Recurso | Fuente | Licencia |
|---|---|---|
| concrete_wall_008, concrete_floor_worn_001, plastered_wall_04, asphalt_02, metal_plate_02, rusty_metal_02, old_planks_02, dirt (2K: color, normal DX, ARM) | https://polyhaven.com | CC0 |

Texturas de efectos (humo, decals de impacto) y máscaras del AR-7: generadas por script (`Tools/textures/gen_fx_textures.py`, horneado en `gen_ar7.py`).

## Modelos y animación
- Mannequin, animaciones de fusil y packs de plantilla de **Epic Games** (licencia de Unreal Engine; solo para uso en UE).
- AR-7 y mallas de efectos: modelados por script en Blender (`ArtSource/Blender/scripts`).
