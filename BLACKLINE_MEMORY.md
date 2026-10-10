# BLACKLINE — Memoria resumida

> Conocimiento duradero en una página. El documento completo es `MEMORIA.md`; el estado y las pruebas, `BLACKLINE_MASTER_STATUS.md`.

## Arquitectura
- UE 5.8, todo en C++ (`Source/Blackline/<AI|Animation|Combat|Debug|Environment|Mission|Player|UI|Vehicles|Weapons>`), sin Blueprints de juego. UI con Slate (`UI/SBL*`) y HUD con Canvas (`UI/BLHUD`).
- Mapas generados por script (`Tools/UnrealPython/build_m0X_*.py` + `bl_levelkit.py`) y parches puntuales (`patch_*.py`); assets importados por script (`setup_*_assets.py`, `update_ship.py`); modelos en Blender por script (`ArtSource/Blender/scripts`; `BL_ONLY=<función>` genera solo una pieza); audio por script (`Tools/audio`).
- Armas: `UBLWeaponData` (`DA_AR7`, `DA_P17`, `DA_SG12`) + `UBLWeaponComponent` + `BLDamage::WeaponTrace`. Ametralladora fija: `ABLMountedGun`. Lancha pilotable: `ABLBoat` (`ABLCharacter::BoardBoat`).
- Misión: `ABLMissionDirector`. Campaña: `BLCampaign` / `UBLCampaignSave`. Orden, mapas y arma impuesta por misión: `BLMenuData::Missions`. Equipamiento: `BLMenuData::LoadoutPrimaries` + `UBLUserSettings::LoadoutPrimary` → `ABLCharacter::ApplyLoadout()`.

## Cómo probar
- `bash Tools/run_test.sh <Prueba> <Mapa> [--nobuild]`. En `/Game/Maps/Dev/L_Dev_Movement`: Weapons, Pistol, Shotgun, Combat, Movement, Aguante. En `L_M01`: AI, Disparo, Level, Views, Mission. En `L_M04`: Mission4, MountedGun, Distancias. MissionN en su mapa; Menu en `L_MainMenu`.
- Opciones: `BL_ARGS="-BLStart=<Fase>"`, `-BLPressContinue`, `-BLSaveProgress`, `-BLLoadout=SG12` (arma principal en una prueba).
- Build empaquetada: RunUAT BuildCookRun → `../BlacklineBuild/Windows`; pruebas con `Tools/run_packaged_tests.sh`.

## Decisiones
- Las pruebas automáticas no escriben el progreso del jugador y juegan en Veterano con el AR-7 (salvo -BLLoadout; la de Menu prueba la elección).
- F en el resumen = siguiente misión; reiniciar va por el menú de pausa.
- Recarga pedida durante el bombeo: se encola. Al montar/desmontar con el gatillo apretado hay que soltarlo (`bFireNeedsRelease`).
- Disparo del jugador a la cabeza de un enemigo con arma de una bala: letal (`bHeadshotKills`). Fusil: daño entero hasta 60 m, 80 % desde 150 m, alcance 300 m.
- IA (medido con "Aguante"): como mucho 2 disparando a la vez, error mínimo 34 cm + 1,5 cm/m, ráfagas de 2-5.
- `FMath::SmoothStep(A, B, x)` exige A < B: al revés devuelve 0 (así estuvo roto el efecto de salud baja).
- Muelles (`FBLSpringVector`): subpasos según la frecuencia; con pasos fijos un frame largo los hacía explotar (cámara girada y balas desviadas).
- Cualquier material creado por script necesita usos de Nanite e instancias: `fix_material_usage.py` y buscar "missing usage flag" en el log de la build.
- Objetos que se mueven (barco): materiales por UV (`M_Ship_UV`), no el triplanar de mundo (la textura "nada").
- Dificultad: `UBLUserSettings::Difficulty` (0/1/2 → daño recibido 35/55/85 %).
- Los vídeos de `Docs/video/` no van a git.

## Bugs del 2026-10-10 (informe del usuario) — detalle en BLACKLINE_MASTER_STATUS.md §4
1. Lancha bloqueante · 2. Barco de cartón · 3. Fusil disparando tras la ametralladora · 4. Sin selección de armamento ·
5. Sin daño a distancia · 6. Muerte rápida sin indicadores · 7. Ametralladora mal apuntada · 8. Cabeza sin muerte en el acto.

## Siguiente tarea exacta
Prueba manual del usuario de los 8 arreglos (sobre todo el equilibrio de daño y el pilotaje de la lancha); vista previa 3D del arma en EQUIPAMIENTO.
