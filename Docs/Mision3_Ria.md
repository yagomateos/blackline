# Misión 3 — "RÍA" (diseño)

> Estado: diseño (2026-10-09). Sale de la campaña de `Docs/Lore.md` (§5): *"Varek reconoce en las fotos el «barco sin
> bandera». BLACKLINE lo busca entre los muelles del casco viejo, casa por casa junto a la ría; primeros operadores de
> Corvane."* Reutiliza todo lo de las misiones 1 y 2 y añade **CQB con puertas y cargas de brecha, operadores de
> Corvane, un tirador con láser en el campanario y un barco que zarpa**.

## Resumen
Una semana después. En las fotos del almacén 7 aparece de fondo un mercante de casco gris sin bandera ni nombre; Varek
lo reconoce: el *barco sin bandera* de sus notas, que fondea en la ría del casco viejo cuando nadie mira. El práctico del
puerto viejo, que lo guía de noche, vive en las casas de la ribera y guarda un cuaderno con sus entradas y salidas.
Sable 2-1 entra al amanecer por el puente viejo, con niebla en la ría, para conseguir el cuaderno y **colocar una baliza
GPS en el casco** antes de que zarpe. En los muelles le esperan, por primera vez, **operadores de Corvane**.

- **Duración objetivo:** 15 min. **Hora:** 07:20, amanecer con niebla espesa sobre el agua que se levanta poco a poco,
  calles mojadas, campanas. **Equipo:** AR-7 + 2 granadas M-6 + **2 cargas de brecha**.
- **Tono:** tensión de interiores (CQB lento, puertas, silencio), luego el muelle a campo abierto y la huida.

## Recorrido (≈ 230 m, sur → norte siguiendo la ría)
```
                         [ MUELLE EXTERIOR · BARCO SIN BANDERA ]  (fase 6-7)
   ría ~~~~~~~~~~~~      [ LONJA ] -> [ ALMACENES ] -> furgón (fase 8)
   ~~~~~~ [ MUELLE DE PESCADORES ] (fase 5)
   ~~~~~~ [ PLAZA + CAMPANARIO ] (fase 4)
   ~~~~~~ [ RIBERA: 3 CASAS ] (fase 3)      calles del casco viejo, soportales (fase 2)
   ~~~~~~ [ PUENTE VIEJO ] <- inserción (fase 1)
```

## Fases
| # | Fase | Contenido de juego | Sistemas que reutiliza / nuevos |
|---|---|---|---|
| 1 | **Puente viejo** | Cruce a pie del puente de piedra en la niebla; dos centinelas al otro lado (sigilo o combate). Briefing: TORRE + Varek. | Radio, IA, coberturas. **Nuevo:** niebla de amanecer densa. |
| 2 | **Casco viejo** | Calles estrechas con soportales, escaleras entre niveles, un control de la Columna en la plaza pequeña. | Fachadas (`ABLBuilding`), combate en calle. |
| 3 | **Casa a casa** | Tres casas de la ribera; puertas que se abren con F (empujando) o **de una patada** (sprint + F). En la tercera, la puerta está atrancada: **carga de brecha** (colocar, 3 s, estalla, los de dentro quedan **aturdidos** 3 s). El **cuaderno del práctico** está en la buhardilla. | Objetivos ClearSquad + Interact. **Nuevo:** `ABLDoor` (bisagra, empuje, patada, brecha), aturdimiento de la IA. |
| 4 | **Campanario** | La plaza grande está cubierta por un **tirador de Corvane** en el campanario: **láser rojo visible**, disparos lentos y casi letales. Se cruza de cobertura en cobertura o se le flanquea por los tejados de la lonja y la casa del cura (ventana a su altura). Las campanas tocan las 7:30. | Objetivo ClearSquad. **Nuevo:** papel **Sniper** (láser, apunta lento, dispara fuerte), campanas. |
| 5 | **Muelle de pescadores** | La niebla se abre: el barco aparece amarrado en el muelle exterior. Barcas, redes, cajas de pescado; milicianos y el primer **operador de Corvane** dirigiendo. | **Nuevo:** papel **Operator** (blindado, granadas de fragmentación, flanquea agresivo, mejor puntería). |
| 6 | **Operadores** | Cuatro operadores defienden la pasarela del barco; usan granadas para sacarte de la cobertura. | ClearSquad con operadores. |
| 7 | **La baliza** | Colocar la baliza en el casco (F mantenido 4 s) bajo el agua de la amarra. Suena la sirena del barco y **zarpa** (se aparta del muelle y se pierde en la niebla); los operadores que quedan y refuerzos por la lonja. | Interact + Defend. **Nuevo:** `ABLScriptedMover` (el barco se mueve por un recorrido), sirena de barco. |
| 8 | **Salida** | Huida por la lonja y los almacenes hasta el furgón de BLACKLINE. | Reach + radio de cierre. |

## Enemigos nuevos
| Tipo | Comportamiento |
|---|---|
| **Operador de Corvane** (`Role = Operator`) | 260 de vida y placa (el torso recibe la mitad de daño; la cabeza, entero), ropa negra sin insignias, casco con visor. Puntería mejor (error mínimo 60 %), turno de disparo siempre, flanquea con más frecuencia y **lanza granadas de fragmentación** cuando el jugador lleva 3 s a cubierto a 8–25 m. Barks en inglés cortado ("Contact front!", "Frag out!"). |
| **Tirador del campanario** (`Role = Sniper`) | No se mueve. Láser rojo del arma al punto donde apunta (lo ve el jugador y avisa). Tarda 1,8 s en fijar; dispara uno a uno con mucho daño (80); si pierde al jugador, mantiene el láser en su última posición. |
| Miliciano (existente) | Puentes, calles, casas. |

## Radio (guion breve)
- TORRE: *"Sable, Torre. El práctico vive en las casas de la ribera. Su cuaderno nos dice cuándo entra y sale el barco."*
- VAREK: *"Casco gris, sin nombre, sin bandera. Siempre en el muelle exterior, siempre con niebla."*
- Brecha: SABLE *"Puerta atrancada. Coloco carga."*
- Campanario: SABLE *"¡Tirador en el campanario! Láser rojo."* — TORRE *"Eso no es un miliciano."*
- Muelle: SABLE *"Torre, veo el barco."* — TORRE *"Baliza en el casco, junto a la amarra de proa."*
- Operadores: TORRE *"Esos son de Corvane. Ya no se esconden."*
- Zarpa: TORRE *"Se mueve. Y la señal es buena: lo tenemos."*
- Final: TORRE *"Buen trabajo, Sable. Ahora sabemos adónde va."*

## Cambios técnicos previstos
- **C++:** `Mission/BLDoor` (puerta con bisagra: F abre empujando hacia el otro lado, patada si se llega corriendo,
  `bBarred` necesita carga de brecha; al reventar aturde a los enemigos de la habitación), `ABLAIController::Stun`,
  papeles `Operator` y `Sniper` en `ABLEnemyCharacter`/`ABLAIController` (granadas de la IA con
  `SuggestProjectileVelocity`, láser con una malla escalada y material emisivo), `ABLScriptedMover` (mueve un actor
  por un recorrido al activarse), cargas de brecha en el jugador (contador en el HUD como las granadas).
- **Arte (Blender):** casco del mercante (90 m, superestructura, grúas de cubierta), campanario de piedra, puente de
  arcos de piedra, barcas de pesca, cajas de pescado y redes, puerta de madera con marco, carga de brecha, equipo de
  operador (casco con visor, chaleco de placas).
- **Audio:** campanas (síntesis modal), sirena de barco, patada y astillado de puerta, pitido de la carga, gaviotas,
  agua contra el casco, frases nuevas (incl. barks de operador).
- **Nivel:** `Tools/UnrealPython/build_m03_ria.py` → `/Game/Maps/M03/L_M03_Ria` con `bl_levelkit.py`; amanecer con
  niebla densa (`dawn_fog_lighting`). Prueba `Mission3`.
