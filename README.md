# BLACKLINE

FPS táctico militar para PC hecho con **Unreal Engine 5.8** y **Blender 5.2**, pensado para funcionar bien en una
**GTX 1660 Super (6 GB)**. Vertical slice jugable: misión 1 **"Amanecer roto"** (inserción, patio del puerto,
control de carretera, combate en la calle principal y recuperación del disco duro de un informante).

> Kessra, 2031. Una ciudad portuaria partida por la "línea negra". El Sargento Adrián Roca ("Sable 2-1") entra
> al amanecer en territorio de la Columna Vesk para recuperar lo que su informante no llegó a entregar.

Casi todo el contenido propio se **genera por script**: modelos en Blender (AR-7, fachadas modulares, coberturas,
vehículos, atrezo), materiales y niveles con Python del editor, sonido procesado/sintetizado con Python, IA,
animación procedural, HUD y pruebas en C++. Ver [`MEMORIA.md`](MEMORIA.md) (diseño, arquitectura, decisiones y estado).

## Qué hay
- Personaje FPS con cámara procedural (bob por pasos, sway, inclinación, mantle, lean, ADS calibrado).
- AR-7 "Halcón": retroceso, dispersión, recarga procedural con IK de ambas manos, fogonazo, casquillos, impactos por material.
- IA del miliciano: patrulla, consciencia gradual, oído, coberturas, turnos de disparo, flanqueo, persecución, barks.
- Salud por segmentos, daño por zonas, ragdoll, checkpoints, HUD "terminal táctico", radio con subtítulos.
- Audio: disparos por capas, balas que pasan cerca, música de combate, ambiente y acústica por zona.
- Arte: edificios de fachadas modulares (instancias Nanite), atrezo de calle, humo y fuego, Lumen + VSM.
- Pruebas automáticas: `bash Tools/run_test.sh <Movement|Weapons|Combat|Level|AI|Mission|Audio|Views> <mapa>`.

## Assets de terceros (NO incluidos en este repositorio)
Este repositorio público **no incluye** contenido que no se puede redistribuir en abierto:

1. **Packs de Epic de las plantillas de UE 5.8** (Mannequin, animaciones de fusil, armas de ejemplo, LevelPrototyping,
   Input de la plantilla). Son gratuitos para cualquier usuario de Unreal: abre el proyecto y en el *Content Drawer*
   usa **Add → Add Feature or Content Pack** y añade **First Person** y **Third Person**. Deben quedar en
   `Content/Characters/Mannequins`, `Content/Weapons/{Rifle,Pistol,GrenadeLauncher,Shared}`, `Content/LevelPrototyping`
   y `Content/Input`. Después ejecuta `Tools/UnrealPython/create_input_assets.py`.
2. **Voces provisionales** (síntesis de voz de Windows): regenéralas en Windows con
   `python -I Tools/audio/gen_voices.py Tools/audio/voice_lines.tsv ArtSource/Audio/SFX` y
   `Tools/UnrealPython/setup_audio_world.py`.

Texturas (Poly Haven) y grabaciones de armas (Free Firearm Sound Library) son **CC0**: ver [`Docs/Creditos_Assets.md`](Docs/Creditos_Assets.md).

## Requisitos
- Unreal Engine 5.8, Visual Studio 2022 (MSVC 14.44+), Windows SDK 10.0.22621.
- Blender 5.2 (solo para regenerar modelos), Git LFS.

## Jugar
- `Tools/jugar.bat`: misión 1. `Tools/jugar_fase.bat Fase3`: empezar en una fase. `Tools/jugar_pruebas.bat`: mapa de pruebas.
- Controles: WASD, ratón, Shift sprint, C agacharse, Espacio saltar/encaramarse, Q/E inclinarse, clic dcho. apuntar,
  clic izq. disparar, R recargar, F interactuar.

## Estado
Vertical slice (Bloques 0–8) terminado; en curso el Bloque 9 (test y optimización) y el menú principal.
Más en `MEMORIA.md`.
