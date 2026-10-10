# Misión 2 — "MANIFIESTO" (diseño)

> Estado: diseño (2026-10-09). Sale de la campaña de `Docs/Lore.md` (§5): *"El disco apunta a un almacén de la
> refinería. Infiltración nocturna para fotografiar el contenido de los contenedores... y lo que hay dentro no es
> munición."* Reutiliza los sistemas de la misión 1 (IA, escuadras, oleadas, humo, granadas, radio con voz,
> checkpoints, objetivos, vehículos guionizados, fachadas, FX, audio) y añade lo que la hace distinta: **noche,
> sigilo de verdad, alarma, dron de Corvane y un enemigo nuevo**.

## Resumen
Tres noches después de "Amanecer roto". El disco de Varek tiene los manifiestos de carga del lado este: cuatro
contenedores marcados a lápiz ("pesa demasiado", "esto no es grano") se descargaron en el **almacén 7 de la
refinería de Kessra**. Sable 2-1 entra solo en lancha por el canal de refrigeración, a las 03:40, para
fotografiarlos sin que nadie sepa que ha estado allí. Dentro no hay munición: hay **municiones merodeadoras (drones
suicidas) de un lote de pruebas de Corvane**, con etiquetas de evaluación en inglés. Al salir salta la alarma; la
Columna cierra la refinería, un dron de reconocimiento de Corvane le busca desde el aire y la lancha tiene que
volver a por él bajo el fuego.

- **Duración objetivo:** 12–15 min. **Hora:** 03:40, noche cerrada, llovizna, cielo con el resplandor naranja de la
  antorcha de la refinería. **Equipo:** AR-7 + 2 granadas (las mismas de la misión 1).
- **Tono:** tenso y silencioso la primera mitad (radio en susurros, sin música); la segunda, huida a contrarreloj.

## Recorrido (≈ 260 m, de oeste a este y vuelta al canal)
```
 N   [ ANTORCHA ]      [ TORRES DE DESTILACIÓN ]        [ PUERTA PRINCIPAL ]  <- refuerzos (alarma)
     ··· tuberías elevadas (rack) ··········································
  [CANAL]→ embarcadero → [ PARQUE DE TANQUES ] → [ PATIO DE CARGA ] → [ ALMACÉN 7 ]
  ^ inserción           (6 depósitos, pasarelas)  (grúa, camiones)      (contenedores, pasarela alta)
  ^ extracción  ←—— [ ZANJA DE TUBERÍAS ] ←—— [ ZONA DE PROCESO (fuego) ] ←—— salida trasera
 S
```

## Fases
| # | Fase | Contenido de juego | Sistemas que reutiliza / nuevos |
|---|---|---|---|
| 1 | **Inserción** | Lancha neumática apagada junto al embarcadero del canal. Briefing en susurros (TORRE + Varek por radio: el número del contenedor). Valla cortada. | Radio con voz, punto de inicio, checkpoint. **Nuevo:** lancha (malla), radio "susurro" (voz más baja), noche. |
| 2 | **Parque de tanques** | Sigilo: 2 patrullas con **linterna** entre depósitos de 14 m y pasarelas; un centinela en la pasarela alta. Sombras reales: la linterna y las farolas de sodio delatan, la oscuridad no. Se puede pasar sin disparar. | Patrulla, consciencia gradual, coberturas. **Nuevo:** linterna en el miliciano (foco que barre), depósitos y tuberías. |
| 3 | **Patio de carga** | Camiones y contenedores apilados, una grúa de puerto parada. Un control con 3 milicianos junto a un brasero. Rodeo por debajo del rack de tuberías o por los contenedores. | Mismas mecánicas que el patio de la misión 1 con otra disposición. |
| 4 | **Almacén 7** | Interior grande y oscuro (fluorescentes rotos, uno parpadea). Pasillos de contenedores. **Fotografiar 3 contenedores** (F mantenido: obturador + destello en el HUD). El tercero está abierto: cajas de drones con etiquetas "CORVANE — EVALUATION LOT 4 — NOT FOR EXPORT". TORRE se queda callado y luego: *"Eso no es munición, Sable. Es un banco de pruebas."* | Objetivos Interact en serie. **Nuevo:** "fotografiar" (efecto de cámara), cajas y drones (mallas), luz que parpadea. |
| 5 | **Alarma** | Al hacer la última foto alguien encuentra la valla cortada: sirena, se encienden los focos de la refinería, entran refuerzos por la puerta principal. En la pasarela del almacén aparece el **tirador de supresión** (LMG): ráfagas largas que obligan a moverse de cobertura en cobertura. | Oleadas (Defend), humo. **Nuevo:** sistema de **alarma** (focos, sirena, refuerzos), **tirador de supresión**. |
| 6 | **Dron** | Saliendo por la puerta trasera, un **dron de reconocimiento de Corvane** barre la zona de proceso con su foco; si te ve, marca tu posición a todas las escuadras (alerta inmediata). Se puede derribar (6–8 impactos) o esquivar bajo el rack. | **Nuevo:** `ABLDrone` (vuelo por puntos, foco, zumbido, detección, derribo con chispas y caída). |
| 7 | **Zona de proceso** | Un miliciano revienta una tubería de gas: **llamarada** que corta la ruta principal (explosión + muro de fuego). Rodeo obligatorio por la **zanja de tuberías** (pasaje bajo, agachado). | Fuego (`ABLSmokeEmitter`), explosión (`SpawnExplosion`), sacudida. **Nuevo:** evento "ruta cortada" (actor que se activa). |
| 8 | **Extracción** | Vuelta al embarcadero: la lancha viene a recogerte bajo el fuego. Defensa corta (2 oleadas + el dron si sigue vivo), "Subir a la lancha" y **pilotarla** (W/S, A/D) hasta la bocana de la dársena → resumen. | Defend + Interact + Reach pilotando. `ABLBoat`: llega por el canal y atraca pegada al embarcadero; luego la pilota el jugador (choca con muelles y límites). (2026-10-10: antes el punto de subida quedaba fuera de alcance → bloqueo.) |

## Enemigos
| Tipo | Dónde | Comportamiento |
|---|---|---|
| Miliciano Vesk (existente) | Todas las fases (≈ 16 en el nivel + refuerzos) | El mismo; de noche, la mitad lleva **linterna** (se ven venir, y ven más lejos donde apuntan). |
| **Tirador de supresión** (nuevo, del diseño aprobado) | Fase 5 (pasarela del almacén), fase 8 (embarcadero) | Ametralladora: ráfagas largas de 8–15 disparos con más dispersión, no flanquea ni cambia de cobertura, **supresión** (si te tiene localizado sigue tirando a tu última posición aunque no te vea). Más vida (casco y chaleco pesado). |
| **Dron de reconocimiento** (nuevo, Corvane) | Fases 6 y 8 | No dispara. Vuela por una ruta, foco cónico; si te ve 1 s seguido, alerta a todas las escuadras con tu posición. Derribable. Primer rastro físico de Corvane en el campo de batalla (prepara la misión 3). |

## Radio (guion breve; voces provisionales TTS como en la misión 1)
- TORRE (susurro de radio, más bajo): *"Sable, Torre. Almacén 7, al fondo del patio de carga. Fotos y fuera. Nadie
  puede saber que has estado ahí."*
- VAREK (por radio desde la base): *"Contenedores KSR 4471, 4472 y el 0918. El 0918 es el que pesaba demasiado."*
- Al ver las cajas: TORRE, silencio, *"...Eso no es munición, Sable. Es un banco de pruebas."*
- Alarma: TORRE *"Han encontrado la valla. Sal por detrás, la lancha vuelve a por ti."*
- Dron: SABLE 2-1 *"Torre, dron en el aire. No es de la Columna."* — TORRE *"Entonces ya sabemos de quién es."*
- Final: TORRE *"Fotos recibidas. Esta vez no van a poder decir que no estaban."*

## Cambios técnicos previstos (sin rehacer nada de lo que funciona)
- **Nivel:** `Tools/UnrealPython/build_m02_refineria.py` → `/Game/Maps/M02/L_M02_Manifiesto`. Los helpers comunes
  (caja, prop, edificio de fachadas, cobertura, checkpoint, radio, objetivo, oleada...) se sacan a un módulo nuevo
  `bl_levelkit.py`; **el script de la misión 1 no se toca**.
- **Noche:** iluminación propia en el script (sin sol; cielo oscuro, luna tenue, niebla con el resplandor naranja de
  la antorcha, farolas de sodio y focos), llovizna ligera con el sistema de sprites (sin Niagara).
- **C++ nuevo:** `ABLDrone`, `ABLBoat`, `ABLAlarmLight` (focos que se encienden con la alarma), linterna en
  `ABLEnemyCharacter` (`bFlashlight`), arquetipo **Gunner** en `ABLEnemyCharacter` (parámetros de disparo y vida),
  **alarma** en `ABLMissionDirector` (`TriggerAlarm`: sirena, `ActivateTags`, oleada de refuerzo), efecto de
  **foto** en el HUD al usar interactuables con `bPhoto`.
- **Arte (Blender por script):** depósito de almacenamiento, tramos de rack de tuberías con soportes, antorcha,
  torre de destilación, nave industrial de chapa, caja de drones + dron merodeador, dron cuadricóptero, lancha
  neumática semirrígida, torre de focos, valla con alambre.
- **Audio (síntesis):** ambiente nocturno de refinería (zumbido, vapor, antorcha), sirena de alarma en bucle,
  zumbido del dron, motor fueraborda, obturador de cámara, ráfaga de ametralladora (a partir de las ráfagas AK
  existentes), frases nuevas.
- **Menú:** la misión 2 pasa a "disponible" en Selección de misión, con sus puntos de inicio.
- **Prueba:** `Mission2` (una sola, cuando se permita abrir Unreal).
