# Misión 4 — "FUEGO CRUZADO" (diseño)

> Estado: diseño (2026-10-09). Campaña (`Docs/Lore.md` §5): *"La Columna lanza una ofensiva para cruzar la línea
> negra. Defensa de un puente con el ejército de Varania mientras TORRE intenta sacar las pruebas del país."*
> Es la misión de **escala**: aliados, morteros, ametralladora montada, apoyo aéreo y la voladura del puente.

## Resumen
Con la baliza en el barco y las fotos de Corvane, TORRE tiene por fin pruebas; falta sacarlas: un enlace por satélite
desde el puesto del ejército en el **puente del ferrocarril**, el único paso sobre el río que la línea negra no ha
cortado. A media tarde la Columna ataca con todo para cruzar. Sable 2-1 llega al puesto con una compañía del ejército de
Varania al mando del **teniente Ilić**, defiende la cabeza de puente mientras sube la transmisión, frena la carga con la
ametralladora del búnker, marca un blindado para la aviación y, cuando ya no se puede aguantar, **vuela el primer tramo
del puente**.

- **Duración objetivo:** 15 min. **Hora:** 16:40, día gris con humo, columnas de humo y polvo de los morteros.
  **Equipo:** AR-7 + 2 granadas. Aliados: 6 soldados del ejército en sus puestos.
- **Tono:** batalla abierta, ruido constante, radio cargada; la transmisión (%) en el objetivo marca el ritmo.

## Recorrido
```
  OESTE (gobierno)                                RÍO                                ESTE (Columna)
  [camiones]→[trincheras + puesto de mando]→[búnker MG]=====tramo 1=====pila=====tramo 2=====pila=====tramo 3=====[ruinas]
       retirada            enlace satélite        cabeza de puente      (se vuela)                       oleadas, BTR
```

## Fases
| # | Objetivo | Contenido | Sistemas (nuevo en negrita) |
|---|---|---|---|
| 1 | Llega al puesto del puente | Desde los camiones por las trincheras bajo fuego de **mortero** (silbido y explosión, se ve dónde cae). | Reach. **`ABLMortarBarrage`** |
| 2 | Defiende la cabeza de puente (transmisión %) | 3 oleadas cruzan el puente con humo; los **soldados aliados** disparan desde sus puestos. | Defend. **`ABLAllyController`**, **progreso en el texto del objetivo** |
| 3 | Toma la ametralladora del búnker | El tirador del ejército cae: F para montarla. | Interact. **`ABLMountedGun`** (montar/desmontar, recalentamiento, retroceso) |
| 4 | Detén la carga sobre el puente | Dos oleadas grandes por el puente: con la ametralladora. | Defend |
| 5 | Marca el blindado para la aviación | Un BTR entra por el puente disparando al búnker; el designador láser del observador (F mantenido 3 s) y, a los pocos segundos, **pasan dos cazas**: el BTR revienta. | Interact. **`ABLStrikeDesignator`**, BTR destruible, caza (`ABLScriptedMover`) |
| 6 | Coloca las cargas en el primer tramo (0/2) | Salir al puente bajo fuego para poner dos cargas en las vigas. | InteractAll |
| 7 | Vuela el puente | Detonador del búnker: el tramo se parte y cae al río (física). | Interact. **`ABLScriptedMover` con caída física** |
| 8 | Aguanta hasta que termine la transmisión | Los que ya cruzaron por el vado del sur atacan el flanco; la transmisión llega al 100 %. | Defend |
| 9 | Retírate a los camiones | Cierre: las pruebas han salido del país. | Reach |

## Personajes y enemigos
- **Teniente Ilić** (radio, ejército de Varania): directo, cansado, agradece la ayuda y no se fía de los "asesores".
- **Soldados del ejército** (aliados): mismo cuerpo, uniforme gris verdoso y casco; no se mueven de su puesto,
  disparan ráfagas cortas a los milicianos que ven. Pueden caer.
- **Columna**: milicianos con humo, un tirador de supresión en el tramo central, BTR. Sin Corvane (la ofensiva es suya).

## Técnica
- **C++:** `Weapons/BLMountedGun` (interactuable: el jugador se sienta, el arma sigue su mirada dentro del arco,
  ráfagas con recalentamiento), `ABLCharacter` montado (sin moverse, mirada limitada, gatillo al arma, F para bajarse,
  el arma propia oculta), `AI/BLAllyController`, papel `Ally` en `ABLEnemyCharacter`, `Mission/BLMortarBarrage`,
  `Mission/BLStrikeDesignator`, `ABLBTR::DestroyVehicle`, `ABLScriptedMover::bPhysicsFall`, `FBLObjective::ProgressLabel`.
- **Arte:** puente de celosía de ferrocarril (tramos de 30 m y pilas), ametralladora montada (trípode y arma),
  designador láser, caza, camión militar, búnker de sacos, puesto de mando con antena de satélite.
- **Audio:** silbido de mortero, paso de los cazas, ametralladora pesada, frases del teniente y de TORRE.
- **Nivel:** `build_m04_puente.py` → `/Game/Maps/M04/L_M04_FuegoCruzado`. Prueba `Mission4`.
