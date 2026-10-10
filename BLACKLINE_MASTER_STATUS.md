# BLACKLINE — Estado maestro

> Estado verificado del proyecto, matriz de pruebas y trabajo pendiente por gravedad.
> El detalle técnico, el diseño aprobado y el historial están en `MEMORIA.md` (fuente principal).
> Última actualización: 2026-10-10.

## 1. Resumen

| Área | Estado |
|---|---|
| Motor / proyecto | Unreal Engine 5.8, C++ (módulo `Blackline`), Win64. Objetivo de hardware: GTX 1660 Super 6 GB, 16 GB RAM, 1080p 60 fps (mín. 45). |
| Compilación | ✅ Editor Win64 Development sin errores ni avisos (2026-10-10). |
| Campaña | ✅ 5 misiones jugables de principio a fin (pruebas automáticas en verde). |
| Bucle principal | ✅ Menú → misión → resumen → **siguiente misión** → … → fin de campaña → menú. Progreso guardado y **Continuar** (nuevo, 2026-10-10). |
| Armas | ✅ AR-7 (fusil), P-17 (pistola), **SG-12 (escopeta)**, granadas, ametralladora montada, armas del suelo. |
| IA | ✅ Milicianos y operadores: patrulla, oído/vista, coberturas, flanqueo, persecución, granadas, bajas con ragdoll. |
| Pantallas | ✅ Menú principal, selección de misión con punto de inicio, armamento, inteligencia/lore, opciones (gráficos/audio/controles), pausa, muerte, misión fallida, misión completada, fin de campaña. |
| Build empaquetada | ✅ Win64 Development (2026-10-10), cocinado sin errores ni avisos; todas las misiones pasan en ella. |
| Vídeo promocional | ✅ `Docs/video/BLACKLINE_Promo_v2.mp4` (30 s, 1080p; fuera de git). |

## 2. Funcionalidad implementada (resumen)

- **Jugador:** andar, correr, agacharse, saltar, mantle, asomarse (lean), ADS, retroceso, recargas táctica/en vacío, cambio de arma (1/2/3, rueda, Y), interacción mantenida (F), salud con regeneración por segmentos, muerte y reaparición en checkpoint.
- **Combate:** daño por hueso (cabeza ×2,5, piernas ×0,75), hitmarkers, salpicaduras, impactos por superficie (hormigón, metal, madera, cristal, tierra, carne), balas que pasan cerca, reacción física parcial, dificultad provisional (el jugador recibe el 55 %).
- **Misiones:** director con objetivos encadenados, checkpoints, radio con subtítulos, alarma, eventos (BTR, helicóptero, morteros, voladura del puente, contrarreloj, dron, gas, lancha), condiciones de fallo (M5: matar al objetivo).
- **Audio:** capas de disparo (cercano/lejano/cola), ambiente por zona, música de combate, acústica, voces (provisionales).
- **Arte:** fachadas modulares, atrezo, materiales PBR de Poly Haven, humo/fuego, iluminación por misión, preajuste gráfico medido.

## 3. Matriz de pruebas (resultados reales)

Pruebas automáticas (`Tools/run_test.sh <Prueba> <Mapa>`): el bot ejecuta los pasos, comprueba el estado del juego y guarda capturas en `Saved/BLTest`. Editor, 1280×720.

| ID | Área | Mapa | Procedimiento (resumen) | Resultado | Fecha | Evidencia |
|---|---|---|---|---|---|---|
| T-MOV | Movimiento | L_Dev_Movement | sprint, salto, agachado, mantle, lean, cámara | ✅ 17/17 (130 fps) | 2026-10-10 | Movement_results.txt |
| T-WPN | AR-7 | L_Dev_Movement | ráfaga, cadencia, retroceso, recargas, cerrojo, ADS, 5 superficies | ✅ 16/16 (116 fps) | 2026-10-10 | Weapons_results.txt |
| T-PST | P-17 y cambio de arma | L_Dev_Movement | 17 pasos (semiauto, ADS, recargas, cambio a mitad de recarga) | ✅ 17/17 (113 fps) | 2026-10-10 | Pistol_results.txt |
| T-SGN | SG-12 y armas del suelo | L_Dev_Movement | recoger/cambiar, perdigones, bombeo, recarga cartucho a cartucho, interrumpir, vaciar, munición, devolver | ✅ 14/14 (121 fps) | 2026-10-10 | Shotgun_results.txt |
| T-CMB | Daño, muerte, checkpoint | L_Dev_Movement | torso/cabeza/pierna, sangre, baja, daño al jugador, regeneración, muerte, reaparición | ✅ 12/12 (125 fps) | 2026-10-10 | Combat_results.txt |
| T-AI | IA | L_M01 | patrulla, oído, sigilo, detección, coberturas, disparo, recarga, flanqueo, persecución, bajas | ✅ 12/12 (88 fps) | 2026-10-10 | AI_results.txt |
| T-FIRE | Balas reales contra enemigos | L_M01 | 16 enemigos a 7–12 m con ADS: trazado → daño → hitmarker | ✅ 19/19 | 2026-10-10 | Disparo_results.txt |
| T-MENU | Menú y pausa | L_MainMenu | páginas, navegación, despliegue a misión, pausa | ✅ 8/8 + despliegue + pausa | 2026-10-10 | Menu_*.png |
| T-M1 | Misión 1 completa | L_M01 | 9 fases hasta el resumen | ✅ 20/20 | 2026-10-10 | Mission_results.txt |
| T-M2 | Misión 2 | L_M02 | 17 pasos hasta la lancha | ✅ 17/17 | 2026-10-10 | Mission2_results.txt |
| T-M3 | Misión 3 + siguiente misión | L_M03 | 17 pasos; F en el resumen carga M04 | ✅ 17/17, carga M04 | 2026-10-10 | Mission3_game.log |
| T-M4 | Misión 4 | L_M04 | 13 pasos hasta la retirada | ✅ 13/13 | 2026-10-10 | Mission4_results.txt |
| T-M5 | Misión 5 (final) | L_M05 | 13 pasos hasta fin de campaña | ✅ 13/13 | 2026-10-10 | Mission5_results.txt |
| T-M5F | Fallo de misión | L_M05 (Azotea) | matar al objetivo → misión fallida | ✅ 3/3 | 2026-10-09 | Mission5Fallo_results.txt |
| T-CAMP | Guardado de campaña | L_M03 + menú | completar → .sav escrito → menú "CONTINUAR · MISIÓN 04", 03 "COMPLETADA" | ✅ | 2026-10-10 | Menu_Principal.png, Menu_Misiones.png |
| T-AUD | Audio | L_M01 | 12 pasos | ✅ 12/12 | 2026-10-09 | Audio_results.txt |
| T-LVL | Recorrido M1 | L_M01 | 245 m a pie por los 4 checkpoints | ✅ 8/8 | 2026-10-09 | Level_results.txt |
| T-VIEW | Vistas y rendimiento | L_M01 | 19 vistas con fps | ✅ 19/19 (104 fps) | 2026-10-09 | Views_results.txt |
| T-GREN | Granadas (jugador e IA) | — | 6 + 2 pasos | ✅ | 2026-10-09 | Grenade*_results.txt |
| T-PKG-MENU | Build: menú | L_MainMenu | menú, despliegue, pausa | ✅ 8/8 + despliegue + pausa | 2026-10-10 | BlacklineBuild/.../Saved/BLTest |
| T-PKG-M1 | Build: misión 1 | L_M01 | 20 pasos | ✅ 20/20 · **87 fps** (720p) | 2026-10-10 | idem |
| T-PKG-M2 | Build: misión 2 | L_M02 | 17 pasos | ✅ 17/17 · 82 fps | 2026-10-10 | idem |
| T-PKG-M3 | Build: misión 3 | L_M03 | 17 pasos | ✅ 17/17 · 88 fps | 2026-10-10 | idem |
| T-PKG-M4 | Build: misión 4 | L_M04 | 13 pasos | ✅ 13/13 · 89 fps | 2026-10-10 | idem |
| T-PKG-M5 | Build: misión 5 | L_M05 | 13 pasos | ✅ 13/13 · **99 fps** | 2026-10-10 | idem |
| T-PKG-M5F | Build: fallo M5 | L_M05 | 3 pasos | ✅ 3/3 | 2026-10-10 | idem |
| T-PKG-AI | Build: IA | L_M01 | 12 pasos | ✅ 12/12 · 89 fps | 2026-10-10 | idem |
| T-PKG-MAT | Build: materiales | todas | log sin "missing usage flag" | ✅ 0 avisos (antes: 30 materiales con el material por defecto) | 2026-10-10 | Blackline.log |
| T-ASPECT | Relaciones de aspecto | menú + M05 | menú, selección de misión y HUD de misión fallida a 1720×720 (21:9) y 960×720 (4:3) | ✅ legibles, sin solapes (solo se cortan etiquetas del mapa decorativo) | 2026-10-10 | Saved/BLTest/aspect/aspect_sheet.png |
| T-PERF1080 | Rendimiento 1080p en la build | L_M01 / L_M04 | AI (12 enemigos) y misión 4 completa a 1920×1080 | ✅ 70 fps / 73 fps de media | 2026-10-10 | BlacklineBuild/.../AI_results.txt |
| T-HITCH | Tirones reales | L_M01, L_M04 | peor frame excluyendo los frames de captura de la prueba | ✅ M1 36,7 ms, M4 33,7 ms, AI 33,5 ms (ningún frame > 100 ms) | 2026-10-10 | Mission_results.txt |
| T-DIFF | Dificultad | L_MainMenu | elegir Élite → guardado en GameUserSettings.ini | ✅ "dificultad 2 (ÉLITE, daño 85 %)" | 2026-10-10 | Menu_results.txt |

Notas de método:
- Mission 1–5 del 2026-10-10 se pasaron con paso de tiempo fijo a 30 fps (grabación del vídeo); son válidas para la lógica, no para medir fps.
- Weapons, Combat, Pistol, Shotgun y Movement van en `L_Dev_Movement`; en el mapa de la misión fallan porque allí no están los blancos. Ese mapa de desarrollo no se cocina, así que esas pruebas solo se pasan en el editor (no es un defecto del juego).

**No verificado / BLOQUEADO** (requiere a una persona o herramientas que no hay):
- Sensación de juego, dificultad y ritmo jugando a mano: **NO VERIFICADO** (pendiente de prueba del usuario).
- Mando (gamepad): **NO VERIFICADO**.
- Sesiones largas / fugas de memoria: **NO VERIFICADO**.

## 4. Defectos conocidos (por gravedad)

| ID | Gravedad | Defecto | Estado |
|---|---|---|---|
| D-01 | P2 | Voces sintetizadas (Windows TTS): suenan robóticas y su licencia no cubre la distribución. | Abierto: hacen falta actores de voz (guion en `Tools/audio/voice_lines.tsv`). |
| D-02 | P2 | Brazos del jugador y cuerpo del miliciano = Mannequin de Epic (tintado, con equipo encima). | Abierto: no se puede producir un humano realista por script; candidatos Fab/Mixamo con licencia válida. |
| D-03 | P3 | "Pico de 150–400 ms" en las pruebas. | ✅ Diagnosticado 2026-10-10: eran los frames en que la prueba hace capturas (lectura de la GPU). Sin ellos, el peor frame es de 34–37 ms. Queda por mirar solo la primera partida en una build recién instalada (compilación de PSO). |
| D-04 | P3 | Música y varios efectos sintetizados (impactos, casquillos, pasos). | Abierto: sustituir por grabaciones. |
| D-05 | P3 | Arte pendiente: nave del puerto y límites como cajas texturizadas, sin interiores, sin decals ni cristales rompibles. | Abierto. |
| D-06 | P3 | IA sin animaciones propias de agacharse/muerte (procedural + ragdoll); no salta obstáculos (faltan NavLinks). | Abierto. |
| D-07 | P3 | Sin menú de dificultad (55 % de daño fijo). | ✅ Arreglado 2026-10-10: Opciones → Controles → DIFICULTAD (Recluta 35 %, Veterano 55 %, Élite 85 %). |
| D-08 | P3 | Los enemigos muertos no reaparecen al volver a un checkpoint (no se restaura el mundo). | Decisión de diseño; documentado. |

Arreglados en esta sesión (2026-10-10):
- **30 materiales de entorno sin los usos de Nanite/instancias** (P1 visual y de rendimiento): en la build empaquetada fachadas, contenedores, hormigón, madera, sacos terreros, etc. usaban el material por defecto, y la misión 1 y la 5 bajaban a 51-53 fps. Causa: el maestro nuevo `M_Env_Triplanar_AT` se creó sin esos usos. Arreglo: `fix_material_usage.py`. Ahora 0 avisos; M1 87 fps y M5 99 fps.
- **Recarga ignorada durante el bombeo** (P2, manejo): pulsar R justo después de disparar la escopeta no hacía nada. Ahora queda en cola.
- **Sin continuidad de campaña** (P1, bucle principal): al completar una misión solo se podía reiniciar. Ahora hay siguiente misión, guardado y Continuar.
- Pruebas que daban falsos fallos: Shotgun/Gastar3 (duración), Disparo (línea de vista por otro canal), Weapons/ImpactoDirt (puntería).

### Bugs del informe del usuario (2026-10-10, segunda tanda)

| # | Gravedad | Bug | Causa raíz | Arreglo | Verificación (prueba real) |
|---|---|---|---|---|---|
| 1 | P1 | La lancha (M2) no deja completar la fase | Atracaba a y 520 y el punto de "Subir" quedaba a ~250 cm de la cámara desde el embarcadero (alcance 210, muro invisible en el borde): imposible subir. La prueba automática llamaba a `Use()` y no lo veía. No había pilotaje. | Atraque pegado al embarcadero y punto de subida en su borde; objetivo nuevo "Sal de la dársena con la lancha": el jugador la pilota (W/S, A/D; choca con muelles y límites; vuelve al timón si muere); HUD de nudos. `ABLBoat`, `BLCharacterMount`, `patch_m02_boat_drive.py`. | Mission2 **18/18**: sube con la detección real (mirar + F) y pilota hasta la bocana (54 km/h). |
| 2 | P2 | El barco de M3 parece de papel | Casco = prismas de paredes verticales con un color plano sin textura (`MI_Ship_Grey` sobre `M_Prop_Simple`). | Casco con forma (pantoque y abanico), tracas, banda roja de flotación, cinta, barandillas con candeleros, ojos de buey, brazolas con refuerzos, grúas cilíndricas, anclas, marcas de calado (`gen_m03.py`, 7.170 tris); material PBR por UV con chapa y óxido de Poly Haven (`M_Ship_UV`, `update_ship.py`; el triplanar de mundo "nadaba" al moverse). | Capturas Mission3_Baliza/Zarpa: chapa con relieve, tracas con sombra, barandilla; Mission3 **17/17**. |
| 3 | P1 | El fusil sigue disparando tras soltar la ametralladora | No se reproducía por código con una sola causa; el estado de disparo dependía del orden de eventos y del gatillo apretado al montar/desmontar. La ametralladora usa el mismo sonido que el AR-7 (se confunden). | Gestión explícita: al montar y al desmontar se sueltan los gatillos de las dos armas y, si el botón estaba apretado, la pulsación no cuenta hasta soltarlo (`bFireNeedsRelease`); recarga cancelada al montar. | MountedGun: bajarse con el gatillo apretado → ametralladora 0 disparos después y fusil 0; soltar → 0; apretar de nuevo → 11 disparos; 3 ciclos rápidos sin disparos fantasma. |
| 4 | P2 | No se puede elegir armamento | No existía (el menú "Armamento" solo informaba; el inventario era fijo AR-7 + P-17). | Pantalla EQUIPAMIENTO tras JUGAR/CONTINUAR o DESPLEGAR: principal AR-7 / SG-12, secundaria P-17, granadas, estadísticas, munición real del asset, COMENZAR MISIÓN. Se guarda en las opciones y `ApplyLoadout()` la pone en el inventario real antes de crearlo (sin duplicados al reiniciar o reaparecer). Una misión puede imponer arma (`ForcedPrimary`). | Menu: Equipamiento (SG-12) → despliegue con **DA_SG12 en la mano y 2 armas**. Captura Menu_Equipamiento. |
| 5 | P1 | Las balas no hacen daño a enemigos lejanos | Caída de daño del fusil desde 30 m hasta el 65 % a 80 m (7 impactos al torso) y alcance de 150 m. | Daño entero hasta 60 m, 80 % desde 150 m, alcance 300 m (pistola y escopeta conservan los suyos). | Distancias: AR-7 al torso 4 impactos a 20/50/100 m y 5 a 140 m; todos los impactos registrados = daño real. |
| 6 | P1 | Muero demasiado rápido y no veo el daño | IA: 3 tiradores a la vez, puntería que convergía a 28 cm, ráfagas de 3-7 → con 3 enemigos a 20 m se moría **0,3 s** después del primer impacto. Feedback: la viñeta/tinte/latido de salud baja **nunca funcionaron** (`SmoothStep(0.6, 0.15)` devuelve 0); arcos pequeños junto a la mira. | IA: 2 tiradores a la vez, error mínimo 34 cm + 1,5 cm/m, convergencia más lenta, ráfagas de 2-5. HUD: viñeta roja en los bordes (más fuerte del lado del tirador), arcos de dirección grandes con punta, barra de salud por segmentos, salud baja con latido; destello más duradero. | Aguante (Veterano): 1×20 m 3,9-6,6 s; 3×20 m 2,4-5,6 s; 3×40 m 2,7-4,7 s; 2×30 m 2,3 s desde el primer impacto. Captura Aguante_Feedback. |
| 7 | P1 | La ametralladora apunta demasiado arriba y falla a distancia | Ojos 47 cm por encima del cañón y bala desde los ojos: el cañón no apuntaba a la mira; alcance 150 m y dispersión 0,7-1,4°; límites -14/+22° mayores que la tronera. | El tirador se agacha con los ojos en la línea de mira; el cañón apunta al punto de la mira y la bala sale de la boca hacia él; alcance 300 m, dispersión 0,3-0,6°; arco -7/+8° (lo que deja ver la tronera). | MountedGun: cañón a 0,04-0,26° del punto apuntado (3 alturas); ráfaga con error medio 0,18° (14 cm a 45 m). |
| 8 | P1 | Un tiro en la cabeza no mata | 28 × 2,5 = 70, y los enemigos reciben el 80 % (operadores 65 %, 140 de vida): 56 de 100. | `bHeadshotKills`: un impacto del jugador en la cabeza de un enemigo con arma de una sola bala es letal (no afecta al jugador ni a la escopeta). | Distancias: AR-7 a 20/50/100/140 m, quieto y en movimiento, y P-17 a 20/50 m: **el primer impacto en la cabeza mata** (13/13). Combat: cabeza → muerto. |
| 9 | P1 | (encontrado en la regresión) Balas desviadas tras un tirón | El muelle del retroceso/temblor de cámara (`FBLSpringVector`) hacía como mucho 8 subpasos: con un frame largo (carga, shaders, captura) el paso era de 50 ms, el integrador se volvía inestable y la cámara giraba hasta 80° un frame; las balas salen de la cámara visible y se iban al cielo (2 de cada ráfaga en Weapons, de forma intermitente). | Subpasos según la frecuencia del muelle (h ≤ 0,5/ω) y como mucho 0,1 s simulados por frame. | Weapons **16/16 tres veces seguidas** (antes 14-15/16 intermitente), 0 desviaciones de cámara registradas; Pistol 17, Shotgun 14, Movement 17. |
| 10 | P1 | No se puede subir al helicóptero de extracción (M1) | La colisión del fuselaje era una caja maciza sobre toda la cabina (los huecos de las puertas solo eran visuales) y el interior casi negro parecía cerrado; la prueba llamaba a `Use()` directamente. | Colisión en carcasa con los huecos libres, peldaño, puertas correderas abiertas, interior gris verdoso con luz; "Subir" en el hueco de la puerta; al subir, el jugador queda dentro de la cabina mirando afuera (`EnterVehicleSeat`). | Mission **20/20** subiendo con la interacción real; capturas Mission_HeliPuerta / Mission_HeliDentro. |
| 11 | P2 | Hélices del helicóptero poco reales | Aspas = tablones planos de cuerda constante a ~3 vueltas/s (una cruz girando despacio). | Aspas con perfil, torsión y punta en flecha; cabeza con horquillas, bielas y plato; 300 rpm reales con disco de desenfoque cónico (estelas suaves) a régimen de vuelo y palas nítidas solo al arrancar o pararse (rotor de cola igual). | Prueba Rotor 4/4 (disco visible en vuelo y oculto frenando); capturas Rotor_*. |
| 12 | P1 | (build) La SG-12 y las chispas faltaban en la build | Se cargan por ruta en tiempo de ejecución y no se cocinaban; el latón de los cartuchos (`M_Brass`) sin uso de malla esquelética. | `DirectoriesToAlwaysCook` (FX, SG12, materiales físicos) y `fix_skeletal_usage.py`. | Build: Menu despliega con la SG-12; 0 "Failed to find object". |

## 5. Rendimiento (medido)

- Build empaquetada, 1280×720 (2026-10-10): misiones 82–99 fps de media; peor frame 165–196 ms (picos de carga).
- Editor, 1280×720: 88–130 fps según la prueba.
- 1080p preajuste Alto (editor): 62–77 fps sin enemigos, ~71 fps en combate (Bloque 9). Pendiente: medirlo en la build empaquetada actual.
- Peor frame real: 34–37 ms (editor, misiones completas); los picos de 150–400 ms eran las capturas de las pruebas (D-03).

## 6. Trabajo pendiente (orden de prioridad)

1. Primera ejecución de una build recién instalada: comprobar la compilación de PSO (D-03).
2. Tras cualquier material nuevo creado por script: ejecutar `fix_material_usage.py` y comprobar el log de la build (regla añadida a MEMORIA).
3. Publicar en GitHub con `Tools/publish_public.sh` (añadir a EXCLUDE los assets de terceros nuevos si los hay).
4. Prueba manual del usuario (sensación, dificultad, encuadres) y menú de dificultad (D-07).
5. Contenido que requiere recursos externos: voces (D-01), brazos/cuerpos (D-02), audio grabado (D-04).
