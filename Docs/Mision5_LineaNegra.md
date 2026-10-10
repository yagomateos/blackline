# Misión 5 — "LÍNEA NEGRA" (diseño)

> Estado: diseño (2026-10-09). Final de la campaña (`Docs/Lore.md` §5): *"Asalto final al puerto para capturar a «el
> inglés» y los registros de Corvane antes de que lo destruyan todo."* Primera misión con **condiciones de fallo**
> (tiempo y objetivo vivo), **compañeros de BLACKLINE** que te siguen y un **final** con captura.

## Resumen
Con las pruebas fuera del país, la coalición por fin autoriza a BLACKLINE a actuar. Corvane lo sabe: está recogiendo
en la **capitanía del puerto**, su cuartel en Kessra, y quemando lo que no puede llevarse. Al amanecer, Sable 2-1 entra
en la terminal de contenedores con **Sable 2-2 y 2-3**. Hay que llegar a la sala de servidores **antes de que la quemen**,
descargar los registros bajo fuego y atrapar **vivo** a "el inglés", que huye a la azotea donde le espera un helicóptero.

- **Duración objetivo:** 15–18 min. **Hora:** 06:30, amanecer frío (como la misión 1: la campaña se cierra donde empezó).
  **Equipo:** AR-7, 3 granadas, 1 carga de brecha. **Compañeros:** 2 (siguen al jugador y combaten).

## Fases
| # | Objetivo | Contenido | Sistemas (nuevo en negrita) |
|---|---|---|---|
| 1 | Entra en la terminal con tu equipo | Desembarco entre contenedores; Sable 2-2 y 2-3 te siguen. | Reach. **Aliados que siguen** (`bFollowPlayer`) |
| 2 | Despeja la entrada de la terminal | Milicianos y dos operadores entre grúas pórtico y contenedores apilados. | ClearSquad |
| 3 | Llega a la sala de servidores **(2:00)** | La capitanía: vestíbulo, escalera, primera planta. Si se acaba el tiempo, Corvane quema los servidores: **misión fallida** (se reinicia en la fase). | Reach. **Límite de tiempo + fallo** |
| 4 | Desactiva las cargas de termita (0/2) | Dos cargas en los armarios de servidores. | InteractAll |
| 5 | Descarga los registros (**descarga %**) | Defensa de la sala con los compañeros: operadores por la escalera y el patio. | Defend con progreso |
| 6 | Sube a la azotea: "el inglés" escapa | Llega el helicóptero de Corvane al helipuerto; "el inglés" corre hacia él. **Si muere, misión fallida.** | Reach (altura). **Objetivo vivo** (`VIP`) |
| 7 | Inutiliza el helicóptero de Corvane | Dispararle hasta que humea y se posa; el inglés se rinde. | **Objetivo `Destroy`**, helicóptero dañable |
| 8 | Detén a "el inglés" | F mantenido junto a él. | Interact |
| 9 | Extracción en la azotea | El helicóptero de la coalición aterriza. Fin de la campaña. | Interact (helicóptero de la misión 1) |

## Técnica
- **Director:** `FBLObjective::TimeLimit` + `FailText`, `FailMission` (pantalla de misión fallida y recarga del nivel
  en la última fase alcanzada), VIP (actores con la etiqueta `BLVIP`: si muere, fallo), objetivo `Destroy`
  (`IBLDestructibleTarget`), `bCampaignFinale` (la pantalla final dice "FIN DE LA CAMPAÑA").
- **IA:** `bFollowPlayer` en `ABLAllyController` (va detrás del jugador y dispara mientras), papel **VIP** con
  `ABLVIPController` (no dispara; al activarse corre por su ruta hasta el helipuerto; se rinde cuando cae el
  helicóptero). `ABLEnemyCharacter` reenvía `OnMissionActivate` a su controlador.
- **Helicóptero:** dañable (`bDamageable`, vida), humo y aterrizaje forzoso al inutilizarlo, material de casco propio.
- **Nivel:** `build_m05_terminal.py` → `/Game/Maps/M05/L_M05_LineaNegra`: terminal de contenedores, grúas, capitanía
  de 3 plantas con interiores y helipuerto en la azotea.
