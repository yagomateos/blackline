# BLACKLINE — Memoria resumida

> Conocimiento duradero en una página. El documento completo es `MEMORIA.md`; el estado y las pruebas, `BLACKLINE_MASTER_STATUS.md`.

## Arquitectura
- UE 5.8, todo en C++ (`Source/Blackline/<AI|Animation|Combat|Debug|Environment|Mission|Player|UI|Weapons>`), sin Blueprints de juego. UI con Slate (`UI/SBL*`) y HUD con Canvas (`UI/BLHUD`).
- Mapas generados por script (`Tools/UnrealPython/build_m0X_*.py` + `bl_levelkit.py`); assets importados por script (`setup_*_assets.py`); armas modeladas en Blender por script (`ArtSource/Blender/scripts`); audio generado o procesado por script (`Tools/audio`).
- Datos de arma: `UBLWeaponData` (`DA_AR7`, `DA_P17`, `DA_SG12`). Disparo: `UBLWeaponComponent` + `BLDamage::WeaponTrace` (atraviesa triggers y muros invisibles).
- Misión: `ABLMissionDirector` (objetivos, radio, resumen). Campaña: `BLCampaign` / `UBLCampaignSave` (`Saved/SaveGames/BLCampaign.sav`); el orden y los mapas de las misiones salen de `BLMenuData::Missions`.

## Cómo probar
- `bash Tools/run_test.sh <Prueba> <Mapa> [--nobuild]`. Weapons, Pistol, Shotgun, Combat y Movement van en `/Game/Maps/Dev/L_Dev_Movement`; AI, Disparo, Level, Views y Mission en `L_M01`; MissionN en su mapa; Menu en `L_MainMenu`.
- Opciones útiles: `BL_ARGS="-BLStart=<Fase>"`, `-BLPressContinue` (pulsa F en el resumen), `-BLSaveProgress` (deja que la prueba guarde la campaña).
- Build empaquetada: RunUAT BuildCookRun → `../BlacklineBuild/Windows`; pruebas en ella con `Tools/run_packaged_tests.sh`.

## Decisiones
- Las pruebas automáticas no escriben el progreso del jugador.
- F en el resumen = siguiente misión; reiniciar va por el menú de pausa.
- La recarga pedida durante el bombeo de la escopeta se encola.
- Los vídeos de `Docs/video/` no van a git (pesan cientos de MB).

- Cualquier material creado por script necesita los usos de Nanite e instancias: ejecutar `fix_material_usage.py` y buscar "missing usage flag" en el log de la build (si falta, en la build se ve el material por defecto y bajan los fps).
- Dificultad: `UBLUserSettings::Difficulty` (0/1/2 → daño recibido 35/55/85 %); las pruebas juegan siempre en Veterano.

## Siguiente tarea exacta
Prueba manual del usuario (sensación, dificultad, encuadres) y recursos externos (voces, brazos/cuerpos, audio grabado). Las pruebas automáticas ya excluyen los frames de captura al medir el peor frame y anotan los picos de más de 100 ms.
