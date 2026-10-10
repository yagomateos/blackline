# BLACKLINE — Memoria del proyecto

> Documento vivo. Se actualiza tras cada cambio importante.
> Última actualización: 2026-10-09 · **Misión 1 completa (9 fases)** · **Campaña completa (5 misiones) compilada y verificada en Unreal** (2026-10-09: Mission2 17/17, Mission3 17/17, Mission4 13/13, Mission5 13/13, Mission5Fallo 3/3 + regresión Mission/AI/Menu OK) · 2026-10-08: **Menú principal, pausa, opciones y LORE** hechos (adelantados de la Fase 4 a petición del usuario) · repo público en GitHub · **Bloque 8 (pase de arte)** hecho: fachadas modulares, atrezo, humo/fuego, iluminación, equipo del miliciano y preajuste gráfico medido a 1080p · **Bloque 7 (audio base)** hecho: voces de radio y de la IA (provisionales), música de combate, ambiente por zona, acústica, balas que pasan cerca, pasos de la IA · Bloque 6: el vertical slice se juega de principio a fin.
> Requisitos originales del usuario (copia literal): `Docs/PROMPT_ORIGINAL.md`.

---

## 1. Estado actual

| Bloque | Estado |
|---|---|
| Diseño (Fase 1) | ✅ Aprobado 2026-10-07 |
| 0 — Preparación (proyecto, git, pipeline, benchmark) | ✅ Hecho |
| 1 — Personaje FPS + cámara | ✅ Hecho · 17/17 pruebas · ~107 fps · probado por el usuario 2026-10-08 (OK salvo recorte del arma en sprint, corregido) |
| 2 — Sistema de armas + AR-7 | ✅ Hecho + **pase de realismo** + **AR-7 v2** (2026-10-08) · Weapons 16/16 (94 fps) + Movement 17/17 (101 fps) · pendiente prueba del usuario |
| 3 — Salud/daño/muerte | ✅ Hecho (2026-10-08) · Combat 12/12 (101 fps) · **probado por el usuario: OK** |
| 4 — Nivel gris | ✅ Hecho (2026-10-08) · Level 8/8 · **probado por el usuario: OK** (gráficos en el Bloque 8) |
| 5 — IA miliciano | ✅ Hecho (2026-10-08) · AI 12/12 (estable en 3 ejecuciones, 85–87 fps con 12 enemigos) · pendiente prueba del usuario |
| 6 — Misión + HUD + checkpoint | ✅ Hecho (2026-10-08) · Mission 9/9 · pendiente prueba del usuario |
| 7 — Audio base | ✅ Hecho (2026-10-08) · Audio 12/12 (85 fps) + regresión Mission/AI/Weapons/Combat/Movement OK · pendiente prueba del usuario |
| 8 — Pase de arte | ✅ Hecho (2026-10-08) · Views 9/9 · **1080p: 62–77 fps** (preajuste Alto) · regresión AI/Mission/Level/Audio OK · pendiente prueba del usuario |
| 9 — Test + optimización (Fase 3) | ✅ Hecho (2026-10-08) · **todas las pruebas pasan en la build empaquetada** (Menu, Mission, Audio, AI, Views, Level, Movement, Weapons, Combat; 90–137 fps a 720p) · 6 fallos encontrados y arreglados (ver Bloque 9) |
| 10 — Menú, opciones, pausa y lore (adelantado de la Fase 4) | ✅ Hecho (2026-10-08) · Menu 10/10 (incl. despliegue y pausa) · regresión OK · pendiente prueba del usuario |
| 11 — Misión 1 completa (fases 5–9) | ✅ Hecho (2026-10-09) · Mission 20/20 (91 fps a 720p) · Views 19/19 · regresión del editor OK (Menu, Audio 12, AI 12, Level 8, Grenade 6, GrenadeAI 2, Movement 17, Weapons 16, Combat 12) · build empaquetada generada, **sus pruebas no se terminaron** (paradas a petición del usuario) |
| 12 — Misión 2 "Manifiesto" | ✅ Hecho (2026-10-09) · Mission2 **17/17** a la primera (92 fps a 720p) · pendiente prueba del usuario |
| 13 — Misión 3 "Ría" | ✅ Hecho (2026-10-09) · Mission3 **17/17** (95 fps) tras arreglar la escalera de las casas (NavMesh) y el aviso de la puerta al director · pendiente prueba del usuario |
| 14 — Misión 4 "Fuego cruzado" | ✅ Hecho (2026-10-09) · Mission4 **13/13** (97 fps) tras arreglar muros invisibles, colisión del puente, voladura, calor de la ametralladora y aliados · pendiente prueba del usuario |
| 15 — Misión 5 "Línea negra" (final) | ✅ Hecho (2026-10-09) · Mission5 **13/13** (102 fps) + Mission5Fallo **3/3** · pendiente prueba del usuario |
| 16 — Pistola P-17 + cambio de arma | ✅ Hecho (2026-10-09) · Pistol **17/17** (123 fps) · encuadre FP ajustado con capturas · regresión Weapons 16/16, Combat 12/12, Movement 17/17, Mission (M1) 20/20 (89 fps) · pendiente prueba del usuario |
| 17 — Escopeta SG-12 + armas del suelo | ✅ Hecho (2026-10-09/10) · Shotgun **14/14** (121 fps) · regresión Weapons 16, Pistol 17, Movement 17, Combat 12, AI 12, Disparo 19, Menu OK · recarga en cola durante el bombeo · pendiente prueba del usuario |
| 18 — Progreso de campaña | ✅ Hecho (2026-10-10) · guardado `BLCampaign.sav`, F en el resumen = siguiente misión, menú "CONTINUAR" y misiones completadas · verificado con Mission3 + `-BLPressContinue` (carga M04) y capturas del menú |

### Sesión 2026-10-10 (tarde) — 8 bugs del usuario
- Arreglados y verificados con pruebas en el juego: lancha (atraque fuera de alcance + pilotaje nuevo), barco de M3 (geometría y material PBR por UV), disparos fantasma tras la ametralladora, selección de armamento (EQUIPAMIENTO), daño a distancia, muerte rápida sin indicadores (IA reequilibrada con medición + HUD de daño; la viñeta de salud baja nunca había funcionado), puntería de la ametralladora, tiro en la cabeza letal. Y uno encontrado en la regresión: el muelle de cámara explotaba con frames largos (balas desviadas).
- Pruebas nuevas: MountedGun, Distancias, Aguante (L_M04 / L_Dev_Movement). Detalle, causas y resultados: `BLACKLINE_MASTER_STATUS.md` §4.

### Sesión 2026-10-10 — LEER PRIMERO
- Estado verificado, matriz de pruebas y pendientes por gravedad: **`BLACKLINE_MASTER_STATUS.md`**. Resumen de una página: `BLACKLINE_MEMORY.md`.
- Hecho hoy: Bloque 17 (SG-12, commit f072429) verificado y arreglado; Bloque 18 (campaña, commit ab23a7f); promo en vídeo.
- Build empaquetada del 2026-10-10: todas las misiones en verde (82–99 fps a 720p). Se encontró y arregló que 30 materiales de entorno salían con el material por defecto en la build (faltaban los usos de Nanite/instancias en `M_Env_Triplanar_AT`) → **regla: tras crear materiales por script, ejecutar `fix_material_usage.py` y buscar "missing usage flag" en el log de la build**.
- Dificultad en Opciones → Controles (Recluta/Veterano/Élite).
- **Siguiente tarea exacta:** medir 1080p con combate en la build y el pico del primer impacto (PSO); publicar en GitHub.

### Promo en vídeo (2026-10-10)
- **`Docs/video/BLACKLINE_Promo_v2.mp4`**: tráiler de 30 s a 1920×1080 en motion graphics (textos que golpean a ritmo, HUD táctico, glitches, destellos, zooms, montaje rápido, logo con "CRUZA LA LÍNEA"). v1 (`BLACKLINE_Promo.mp4`, 720p) recorría misión a misión; el usuario pidió menos fases y más acción.
- Cómo se hizo: las pruebas automáticas se grabaron con `BL_EXEC="showhud" BL_ARGS="-dumpmovie -UseFixedTimeStep -FPS=30"` (fotogramas sin HUD en `Saved/Screenshots/WindowsEditor`; el juego va ~5× más lento). Grabadas: Mission (M1), Mission2–5, Weapons y AI (parcial, hasta "Bajas"). Combat y Level no se llegaron a grabar. Los fotogramas (~25 GB) están en el scratchpad de la sesión, no en el repo.
- Montaje: `Tools/trailer/` → `promo_seq.py` (lista de planos), `promo_motion.py` (render con PIL+numpy → ffmpeg de `imageio-ffmpeg`), `trailer_audio.py` (música/golpes sintetizados + SFX de `ArtSource/Audio/SFX`), `sheets.py` (hojas de contactos con la fase de la prueba), `find_shots.py` (detecta fogonazos). Render: ~4,5 min.
- Limitación: el material viene de pruebas (el bot se teletransporta), no hay carreras ni saltos. Para un tráiler mejor haría falta un modo de grabación con cámaras de recorrido.
- Nota: Weapons dio 11/16 con paso de tiempo fijo (las pruebas de cadencia miden tiempo real); no es una regresión comprobada.

### Sesión 2026-10-09
- Misiones 2–5 verificadas en Unreal (Bloques 12–15). Después, a petición del usuario ("sigue con la P-17 y el cambio de arma"): **Bloque 16 — pistola P-17 y cambio de arma**, hecho y probado (ver Bloque 16 en Tareas y "Pruebas automáticas").
- **Última tarea:** P-17 completa (malla propia, sonido real CC0, recarga procedural con cargador que cae al suelo, corredera que queda abierta y se monta) + cambio de arma (1/2, rueda, Y). Resultado: Pistol 17/17 y regresión en verde.
- **Siguiente tarea exacta (por prioridad):** 1) que el usuario juegue (misiones 2–5 y la P-17: tecla 2) y anotar lo que diga — en particular el encuadre de la pistola en la recarga (el brocal queda en el borde inferior de la pantalla); 2) build empaquetada con las 5 misiones y la P-17, y pasar las pruebas en ella; 3) publicar en GitHub con `Tools/publish_public.sh`; 4) tercera arma (SG-12 "Mastín", escopeta) con el mismo método.
- La regla de "no abrir Unreal" ya no está en vigor; sigue la de no repetir pruebas que no toca (`blackline-test-cadence`).

---

## 2. Diseño aprobado

### Concepto
- **Nombre provisional:** BLACKLINE.
- **Ambientación:** 2031, Kessra, ciudad portuaria de la República de Varania (ficticia). Ciudad partida por la "línea negra": al este controla la **Columna Vesk** (milicia paramilitar) entrenada y armada por la contratista **Corvane Security**.
- **Protagonista:** Sargento **Adrián Roca**, indicativo **"Sable 2-1"**, del Grupo Operativo BLACKLINE.
- **Arco:** Corvane usa la guerra como banco de pruebas de armamento; BLACKLINE reúne pruebas misión a misión.
- **Tono:** realista, sobrio, operación militar moderna. Sin sci-fi.

### Misión 1 — "AMANECER ROTO" (~15–20 min, amanecer)
| # | Fase (diseño aprobado) | Estado (2026-10-08) |
|---|---|---|
| 1 | Inserción (furgón civil, briefing por radio) | ✅ Jugable (calle trasera, furgón, briefing con voz) |
| 2 | Infiltración (patrullas, sigilo opcional) | ✅ Jugable (callejones + patio de contenedores, 2 patrullas y centinela, consciencia gradual) |
| 3 | Contacto (control de carretera, alarma) | ✅ Jugable (control con 4 milicianos; los disparos alertan a la escuadra). Sin alarma sonora propia |
| 4 | Combate en calle (cobertura, flanqueos) | ✅ Jugable (calle principal, 5 milicianos, flanqueo por los callejones) |
| 5 | Asalto al bloque de viviendas (CQB piso a piso) | ✅ Jugable (2026-10-09): bloque C3 con portal, escalera de ida y vuelta, pasillos, 12 pisos con muebles, azotea; 6 milicianos dentro; luces interiores |
| 6 | Objetivo: rescatar a **Tomas Varek** y recuperar su disco duro | ✅ Jugable: disco en el local; Varek retenido en la última planta, se libera con F (mantener 2 s) y te sigue (`ABLVarek`) |
| 7 | Contraataque (defender posición, oleadas, humo) | ✅ Jugable: objetivo `Defend` con 3 oleadas (calle con humo, pasaje trasero, calle + callejón con humo), ≥ 75 s; humo que tapa la vista de la IA |
| 8 | Evento: un **BTR** dispara contra el edificio → derrumbe parcial → huida por tejados | ✅ Jugable (2026-10-09): `ABLBTR` entra por la calle, 3 cañonazos a la fachada de C3; al 2.º cae un paño de la 2.ª planta + peto + cornisa (física) y los escombros tapan la escalera; luego dispara cerca del jugador si lo ve. Azotea → pasarela de andamio → azotea de C4 → escalera de incendios de 3 tramos → muelle |
| 9 | Extracción en el muelle con cobertura de helicóptero | ✅ Jugable (2026-10-09): muelle (contenedores, nave, grúa pórtico, agua), zona de aterrizaje con humo verde, defensa de 2 oleadas (valla norte y callejón G) ≥ 45 s; `ABLHelicopter` llega del mar, estacionario con ametrallador de puerta, aterriza; "Subir al helicóptero" → radio de cierre → resumen |

### Misión 2 — "MANIFIESTO" (~12–15 min, noche) — diseño: `Docs/Mision2_Manifiesto.md`
Refinería de Kessra, 03:40. Infiltración nocturna en lancha para fotografiar tres contenedores del almacén 7; dentro hay municiones merodeadoras de un lote de pruebas de Corvane. Mapa `/Game/Maps/M02/L_M02_Manifiesto` (script `Tools/UnrealPython/build_m02_refineria.py`, ~265 × 70 m). **Estado: hecha y verificada con su prueba automática (2026-10-09); pendiente de que la juegue el usuario.**

| # | Objetivo (tipo) | Contenido / sistemas |
|---|---|---|
| 1 | Cruza la valla del recinto (Reach) | Embarcadero, lancha amarrada (`ABLBoat` atrezo), valla con hueco cortado, briefing TORRE + Varek |
| 2 | Atraviesa el parque de tanques sin que te vean (Reach) | 6 depósitos con muretes, avenida iluminada (`BLLit`), callejones oscuros, rack de tuberías; 2 patrullas con **linterna**; **noche** (`ABLNightSettings`) y **alarma si te descubren** |
| 3 | Rodea el patio de carga hasta el almacén 7 (Reach) | Contenedores, grúa pórtico, brasero con 3 guardias (su luz delata) |
| 4 | Fotografía los contenedores del manifiesto (**InteractAll**, 0/3) | Fotos con obturador y visor en el HUD, una frase por foto; cajas de drones + revelación |
| 5 | Resiste dentro del almacén (Defend, 3 oleadas, ≥ 40 s) | **Alarma** (focos `ABLAlarmLight`, sirenas), **tirador de supresión** en la pasarela, refuerzos con linterna y humo |
| 6 | Sal por la puerta trasera (Reach) | Aparece el **dron de Corvane** (`ABLDrone`: foco, detección, alerta, derribo) |
| 7 | Cruza la zona de proceso hasta el muelle (Reach) | **La tubería de gas revienta** (`ABLHazardEvent`) y corta la calle → rodeo por la **zanja de tuberías**; antorcha que ilumina la zona |
| 8 | Aguanta en el muelle hasta que llegue la lancha (Defend, 2 oleadas) | Lancha de extracción por la dársena (`ABLBoat`), otro tirador de supresión |
| 9 | Sube a la lancha (Interact) | Cierre con radio y resumen |

### Misión 3 — "RÍA" (~15 min, amanecer con niebla) — diseño: `Docs/Mision3_Ria.md`
Casco viejo de Kessra junto a la ría, 07:20. El cuaderno del práctico y una baliza GPS en el casco del "barco sin bandera". Mapa `/Game/Maps/M03/L_M03_Ria` (script `Tools/UnrealPython/build_m03_ria.py`, ~205 × 50 m). **Estado: hecha y verificada con su prueba automática (2026-10-09); pendiente de que la juegue el usuario.**

| # | Objetivo (tipo) | Contenido / sistemas |
|---|---|---|
| 1 | Cruza el puente viejo (Reach) | Puente de piedra de tres arcos sobre el canal, niebla de amanecer, dos centinelas |
| 2 | Atraviesa el casco viejo hasta la plaza pequeña (Reach) | Calle mayor con soportales, control con sacos y fuente; barricada de contenedores que obliga a bajar a la ribera |
| 3 | Despeja las casas de la ribera (ClearSquad) | Casas de 2 plantas + buhardilla con escaleras y **puertas `ABLDoor`** (F empuja; corriendo, patada que tira al de detrás; la IA las abre) |
| 4 | Entra en la casa del práctico (Interact) | Puerta atrancada: **carga de brecha** (3 s de pitidos, la hoja sale volando, los de dentro **aturdidos** 3,5 s) |
| 5 | Despeja la casa del práctico (ClearSquad) | Dos aturdidos abajo, uno en la buhardilla |
| 6 | Coge el cuaderno del práctico (Interact) | Buhardilla; radio: el barco está amarrado hoy |
| 7 | Neutraliza al tirador del campanario (ClearSquad) | **Tirador de Corvane** con **láser rojo**, tarda 1,8 s en fijar; campanas; soportales y lonja como cobertura desde arriba |
| 8 | Llega al muelle de pescadores (Reach) | Barcas, cajas, redes; un **operador** dirige a tres milicianos |
| 9 | Elimina a los operadores de Corvane del espigón (ClearSquad) | Cuatro **operadores** (placa, mejor puntería, sin barks, granadas si te escondes) |
| 10 | Coloca la baliza en el casco (Interact, 4 s) | Junto a la amarra de proa |
| 11 | Resiste en el espigón mientras el barco zarpa (Defend, 2 oleadas, ≥ 35 s) | El barco (`ABLScriptedMover`) toca la sirena y se aleja ría abajo; refuerzos con dos operadores |
| 12 | Llega al furgón (Reach) | Salida entre almacenes; cierre de TORRE |

### Misión 4 — "FUEGO CRUZADO" (~15 min, tarde gris) — diseño: `Docs/Mision4_FuegoCruzado.md`
Puente del ferrocarril sobre el río de la línea negra, 16:40. Defensa con el ejército de Varania mientras TORRE saca las pruebas por satélite. Mapa `/Game/Maps/M04/L_M04_FuegoCruzado` (`build_m04_puente.py`, ~210 × 80 m). **Estado: hecha y verificada con su prueba automática (2026-10-09); pendiente de que la juegue el usuario.**

| # | Objetivo (tipo) | Contenido / sistemas |
|---|---|---|
| 1 | Llega al puesto del puente por la trinchera (Reach) | **Mortero** (`ABLMortarBarrage`: silbido 1,4 s antes, explosión, nunca encima del jugador) |
| 2 | Defiende la cabeza de puente (Defend, 3 oleadas, **"enlace %"** en el texto) | 6 **soldados aliados** (`ABLAllyController`: no se mueven, buscan blanco, ráfagas y se agachan) |
| 3 | Toma la ametralladora del búnker (Interact) | **`ABLMountedGun`**: montar con F, mirada limitada al arco, calor y bloqueo, retroceso, F para bajarse; HUD propio |
| 4 | Detén la carga sobre el puente (Defend, 2 oleadas grandes) | Con la ametralladora por la tronera |
| 5 | Marca el blindado para la aviación (Interact, 3 s) | BTR por el puente; **designador** → **dos cazas** (`ABLScriptedMover`) y la pasada destruye el BTR (`DestroyVehicle`: arde) |
| 6 | Coloca las cargas en el primer tramo (InteractAll 0/2) | Mortero sobre el puente mientras tanto |
| 7 | Vuela el puente (Interact) | Detonador del búnker |
| 8 | Aguanta hasta que termine la transmisión (Defend, "transmisión %") | El tramo 1 explota y **cae al río con física**; los del vado atacan por el flanco |
| 9 | Retírate a los camiones (Reach) | Cierre del teniente Ilić |

### Misión 5 — "LÍNEA NEGRA" (~15–18 min, amanecer) — diseño: `Docs/Mision5_LineaNegra.md` — FINAL DE LA CAMPAÑA
Terminal de contenedores y capitanía del puerto (cuartel de Corvane), 06:30. Mapa `/Game/Maps/M05/L_M05_LineaNegra` (`build_m05_terminal.py`, ~160 × 100 m). **Estado: hecha y verificada con su prueba automática (2026-10-09); pendiente de que la juegue el usuario.**

| # | Objetivo (tipo) | Contenido / sistemas |
|---|---|---|
| 1 | Entra en la terminal con tu equipo (Reach) | **Sable 2-2 y 2-3** (`bFollowPlayer`: te siguen, reaparecen a tu lado si se quedan atrás, disparan) |
| 2 | Despeja la entrada de la terminal (ClearSquad) | Contenedores apilados, grúas STS, coches de transporte; milicianos y dos operadores |
| 3 | Llega a la sala de servidores (Reach, **2:00**) | **Contrarreloj con fallo** ("Han quemado los servidores"): cuenta atrás en el texto; al fallar se repite la fase |
| 4 | Desactiva las cargas de termita (InteractAll 0/2) | Sala de servidores con armarios y luces |
| 5 | Descarga los registros de Corvane (Defend, "descarga %") | Oleadas por la escalera y el patio, con operadores |
| 6 | Sube a la azotea: el inglés escapa (Reach, altura) | El inglés (`ABLVIP`) corre al helipuerto; llega el **helicóptero de Corvane** (negro). **Si el jugador le mata, misión fallida** (los tiros de otros no le hacen daño) |
| 7 | Inutiliza el helicóptero de Corvane (**Destroy**) | Helicóptero dañable: humea y aterrizaje forzoso; si tardas 45 s en el helipuerto, **escapa (fallo)** |
| 8 | Detén a «el inglés» (Interact) | Se rinde de rodillas |
| 9 | Sube al helicóptero de la coalición (Interact) | Halcón 1 aterriza en la azotea; pantalla **"FIN DE LA CAMPAÑA"** |

**Vertical slice = fases 1–4 + objetivo simple** (llegar a punto + recuperar objeto): esa parte está terminada. Las fases 5–9 son la **misión 1 completa** (ver Tareas pendientes, Bloque 11).
**Decidido (usuario, 2026-10-08): Varek se rescata en la misión 1** (fase 6), como en el diseño aprobado. La radio provisional del slice ("Ni rastro de Varek") se cambiará en el Bloque 11; el lore y el menú ya están ajustados (la misión 3 "Ría" pasa a ser la búsqueda del barco de Corvane).

### Arsenal
| Arma | Rol | Prioridad |
|---|---|---|
| AR-7 "Halcón" (fusil 5.56, ~750 RPM) | Principal, arma estrella | Slice |
| P-17 (pistola 9 mm, semiautomática, 17+1) | Secundaria, siempre en la funda (tecla 2) | ✅ Hecha (Bloque 16) |
| SG-12 "Mastín" (escopeta corredera) | CQB | Fase 4 |
| Granada M-6 (fragmentación) | Utilidad | Fase 4 |
| SMG-9 "Vesper" | CQB | Fase 4 (última) |

Cada arma = Data Asset (stats, curvas de retroceso, sonidos, VFX, animaciones).

### Enemigos
| Tipo | Rol | Cuándo |
|---|---|---|
| Miliciano Vesk | Infantería básica | Slice |
| Tirador de supresión (LMG) | Fija al jugador | Fase 4 |
| Operador Corvane | Élite, blindado, granadas | Fase 4 |
| BTR | Vehículo guionizado | Fase 4 |

### IA
- ~~StateTree~~ **Máquina de estados en C++** (cambio comunicado al usuario el 2026-10-08: StateTree/EQS no se pueden generar ni probar por script): Patrulla → Sospecha/Investigar → Combate (Cubrirse/Disparar/Reposicionar/Flanquear/Recargar) → Perseguir → Buscar. ~~EQS~~ → puntuación de coberturas en C++ sobre `ABLCoverPoint`.
- **AI Perception**: vista, oído (disparos, sprint, granadas), daño.
- **EQS** sobre **puntos de cobertura colocados a mano**.
- **Coordinador de escuadra** (subsistema C++): tokens de ataque (2–3 tiradores activos), roles flanqueo/supresión.
- **Barks** de voz ("¡Recargando!", "¡Granada!"...).
- Muerte: hit reaction → ragdoll con Physical Animation.

### Mecánicas
- Andar, sprint, agacharse, mantle (obstáculos bajos), salto limitado, deslizamiento corto (a validar).
- Inclinación Q/E; apoyo del arma en bordes (reduce retroceso). Sin sistema "pegarse a cobertura".
- Salud con regeneración parcial por segmentos; daño por zonas (cabeza/torso/extremidades).
- Checkpoints automáticos por fase de misión.
- Interacción contextual con un botón.

### Cámara FPS
Procedural y configurable: head bob ligado a pasos (suave), sway por inercia, inclinación al strafe, retroceso separado arma/cámara, sacudida por daño, FOV en ADS. First Person Rendering de UE5 (FOV propio del arma, sin clipping). Opción de accesibilidad para reducir movimiento.

### Dirección visual
Amanecer frío (gris azulado/cian) + acentos de sodio naranja. PBR con texturas CC0 / Fab. Niagara para muzzle flash, casquillos, impactos por material, polvo, humo, cristales. Destrucción limitada (decals, cristales, derrumbe pre-simulado en Blender). Sin Chaos destruction en tiempo real.

### Menú / HUD
- **Menú:** "terminal táctico de mando": mapa topográfico oscuro de Kessra con la línea negra, tipografía monoespaciada, acento ámbar. Misiones como puntos en el mapa. Entradas requeridas: Jugar, Selección de misión, Armamento/loadout, Opciones (Gráficos, Audio, Controles), Salir. Identidad propia, sin copiar la UI de CoD. **Hecho (Bloque 10)** + Inteligencia (expedientes del lore) y música propia (`SW_Mus_Menu_Loop`).
- **Lore:** `Docs/Lore.md` (mundo, facciones, personajes, cronología, campaña de 5 misiones). TORRE = Capitán Elias Marot.
- **HUD:** mínimo y contextual. Munición abajo-derecha (aparece al disparar/recargar), granadas, marcador de objetivo con distancia, indicador direccional de daño, hitmarker, prompt de interacción. Sin minimapa.

### Audio
MetaSounds: disparos por capas (mecánica + cuerpo + cola cercana/lejana según distancia), reverb por zona, variación aleatoria. Fuentes royalty-free (Sonniss GDC, CC0). Barks de IA y radio del escuadrón.
Cobertura requerida: disparos, recargas, pasos, impactos, explosiones, ambiente urbano, voces, comunicaciones y música solo cuando sea apropiada. El sonido es parte central del feedback de combate.

---

## 3. Arquitectura técnica

### Herramientas
- Unreal Engine **5.8** (`C:\Program Files\Epic Games\UE_5.8`)
- Blender **5.2.2 LTS**
- Visual Studio 2022 17.14 (MSVC 19.44.35229, Windows SDK 10.0.22621 y 10.0.26100)
- Git + Git LFS (repo local, rama `main`)

### Decisiones de tecnología
| Tecnología | Decisión | Motivo |
|---|---|---|
| C++ + Blueprints | Núcleo en C++, contenido en BP/Data Assets | Rendimiento, mantenibilidad |
| Enhanced Input | Sí | Estándar, remapeo |
| StateTree + Perception + EQS | Sí | IA legible y barata |
| GAS | **No** | Excesivo para FPS de un jugador |
| Nanite | Solo entorno estático | Funciona en GTX 1660S, ahorra LODs |
| Lumen | **Sí** (GI + reflejos, High) | Benchmark: cuesta ~2,2 ms. Ver `Docs/Benchmark.md`. Bloque 8: en el nivel real, Épico costaba +7 ms (TSR con historial a 4K + Lumen) → preajuste por defecto **Alto** en `DefaultGameUserSettings.ini` |
| Virtual Shadow Maps | **Sí** | Benchmark: con Nanite es ~1,8 ms más barato que CSM |
| Resolución interna | **TSR 75%** por defecto | Ahorra ~4 ms frente a nativo |
| Niagara | Sí, con presupuesto por escalabilidad | — |
| MetaSounds | Sí | — |
| CommonUI | Sí | Menús con mando/teclado |
| Control Rig | Ajustes (IK manos) | No animar desde cero |
| World Partition | **No** (misión 1) | Mapa pequeño |
| Chaos destruction | No (salvo cristales) | Coste |

### Estructura del repositorio
```
Blackline.uproject
Config/                 Ajustes del motor
Content/                Assets de Unreal (LFS)
Source/Blackline/       C++ (a crear en Bloque 1)
ArtSource/Blender/
  scripts/              Generadores de assets (Python de Blender)
  export/               FBX exportados (ignorado en git, se regenera)
Tools/
  UnrealPython/         Scripts de automatización del editor
  benchmark/            Benchmark de rendimiento
Docs/                   Resultados y documentación técnica
MEMORIA.md
```

### C++ implementado
| Clase | Archivo | Función |
|---|---|---|
| `ABLCharacter` | `Player/BLCharacter` | Movimiento: andar 420, sprint 660, agachado 210 cm/s, ADS ×0.6, salto ~78 cm, mantle 45–165 cm, lean Q/E con colisión |
| `UBLFirstPersonRigComponent` | `Player/BLFirstPersonRigComponent` | Cámara/arma procedural: bob por pasos, sway con muelles, inclinación strafe, aterrizaje, poses sprint/mantle, **calibración automática de ADS**, FOV, evento `OnFootstep`, `AddCameraKick`/`AddWeaponKick` (para retroceso/daño) |
| `FBLSpringVector`, `BLExpInterp` | `Player/BLSpring.h` | Muelles amortiguados e interpolación independiente del frame rate |
| `ABLPlayerController`, `ABLPlayerCameraManager` | `Player/BLPlayerController` | Contextos de input, límites de pitch ±80° |
| `ABLGameMode` | `Core/BLGameMode` | GameMode global por defecto |
| `UBLAutoTestComponent` | `Debug/BLAutoTestComponent` | Piloto automático de pruebas (`-BLTest=Movement` / `Weapons`). Ignora el ratón durante la prueba |
| `UBLWeaponData` | `Weapons/BLWeaponData.h` | Data Asset del arma: cadencia, daño y caída, munición, dispersión, retroceso (mira + visual), recarga, sockets, distancia ojo-mira, sonidos (listas de variaciones) y efectos |
| `IBLWeaponOwner` | `Weapons/BLWeaponOwner.h` | Interfaz del portador (jugador ahora, IA en el Bloque 5): vista de disparo, malla, bloqueo (sprint/mantle), eventos |
| `UBLWeaponComponent` | `Weapons/BLWeaponComponent` | Inventario, hitscan (`ApplyPointDamage`), cadencia por acumulador, semi/auto, recarga táctica/vacío con inserción de munición, recarga automática al apretar sin balas, dispersión (cadera/ADS/movimiento/aire/bloom), retroceso sobre la mira con recuperación parcial que respeta la compensación del jugador. CVar `bl.Weapon.DebugTraces` |
| `UBLWeaponFXComponent` | `Weapons/BLWeaponFXComponent` | Fogonazo (malla + luz), casquillos, decals de impacto (máx. 64), esquirlas, Niagara, sonidos 2D (jugador) / 3D (IA) |
| `UBLDebrisPoolComponent` | `Weapons/BLDebrisPoolComponent` | Partículas de malla baratas (casquillos, esquirlas) con ISM, un rebote y sonido al tocar el suelo |
| `UBLWeaponAnimInstance` | `Animation/BLWeaponAnimInstance` | Mecánica del arma en C++: el cerrojo cicla en cada disparo, queda abierto con el cargador vacío y se suelta al golpear la retenida (`BoltReleaseTime`); el gatillo se mueve al apretar. Lee el `UBLWeaponComponent` del dueño (sirve para la IA) |
| `UBLFirstPersonAnimInstance` | `Animation/BLFirstPersonAnimInstance` | Animación de brazos en C++ (sin Anim Blueprint): pose base + acción opcional (solo brazos) + **IK de dos huesos de ambas manos al arma**; la izquierda puede seguir un punto del arma (recarga) |
| `UBLSurfaceEffectsData` | `FX/BLSurfaceEffects` | Tabla por superficie (hormigón, metal, madera, cristal, tierra): sonidos de impacto/casquillo/pasos, decal, polvo, chorro, chispas, escombros. `ResolveSurface(Hit)`: material físico → override del componente → material de la instancia (UE no aplica el PhysMaterial de las instancias a la colisión simple) |
| `UBLFXSubsystem` | `FX/BLFXSubsystem` | Partículas propias baratas: 2 ISM (polvo iluminado con atlas 2x2 y suavizado por profundidad; chispas aditivas estiradas por velocidad), datos por instancia, presupuesto fijo (160 sprites, 128 chispas, 80 decals); escombros con pool de mallas |
| `UBLScriptLibrary` | `Debug/BLScriptLibrary` | Apoyo a los scripts de Python (asignar SurfaceType a materiales físicos) |
| `ABLDebugHUD` | `UI/BLDebugHUD` | HUD provisional: mira dinámica con la dispersión, munición, "RECARGANDO", **hitmarker** (rojo en bajas, mayor en cabeza), **indicadores direccionales de daño** (arcos rojos que siguen la dirección al girar) y aviso "PUNTO DE CONTROL" (el HUD real es del Bloque 6) |
| `BLDamage`, `EBLHitZone`, `ECC_BLWeapon` | `Combat/BLDamageTypes` | Zonas por hueso (cabeza/cuello, torso, extremidades). Canal de trazado **Weapon** (`ECC_GameTraceChannel1`): la cápsula de los personajes lo ignora y su malla (Physics Asset) lo bloquea → impacto por hueso |
| `UBLHealthComponent` | `Combat/BLHealthComponent` | Salud por **segmentos** (4 × 25) con regeneración parcial (tras 4 s, 20/s, hasta el tope del segmento). Recibe `ApplyPointDamage`/`ApplyRadialDamage`. Eventos `OnDamaged`/`OnDeath` con `FBLDamageInfo` (zona, hueso, dirección, origen). `DamageTakenMultiplier` (dificultad) |
| `UBLHitReactionComponent` | `Combat/BLHitReactionComponent` | Reacción **física** a los impactos (Physical Animation desde `spine_01` + impulso en el hueso, vuelve a la animación en 0,45 s) y **ragdoll** al morir. Sin Anim Blueprint; lo usará la IA |
| `ABLTargetDummy` | `Combat/BLTargetDummy` | Maniquí de prueba (salud + reacción + ragdoll), reaparece a los 5 s |
| `UBLCheckpointSubsystem`, `ABLCheckpointVolume` | `Mission/` | Checkpoint = punto de reaparición + **inventario/munición** guardados. El inicio es el primero; los volúmenes guardan al entrar |
| `ABLEnemyCharacter` | `AI/BLEnemyCharacter` | Miliciano: Mannequin con uniforme caqui (`MI_Militia_*`) y **equipo** (Bloque 8: chaleco portacargadores, casco en 2 de cada 3, brazalete negro de la Columna; piezas rígidas modeladas en el espacio de la malla y ajustadas al hueso con la inversa de su pose de referencia), AR-7 en la mano (mismo `UBLWeaponComponent`, munición de reserva infinita), salud sin regeneración, reacción física y ragdoll; el arma cae con físicas al morir. Agacharse = solo pose. Escuadra (`SquadId`) y ruta de patrulla (`PatrolPoints`) |
| `UBLEnemyAnimInstance` | `Animation/BLEnemyAnimInstance` | Animación en C++: locomoción de fusil en 8 direcciones (andar/trotar, fase común sin patinar), idle de apuntado, recarga en la parte superior, agacharse procedural (pelvis + IK de piernas), torso para apuntar en vertical, retroceso, IK de la mano izquierda al guardamanos |
| `ABLAIController` | `AI/BLAIController` | Percepción (vista 50 m / 70°, oído, daño) + **consciencia gradual** (más rápida cerca, de pie y corriendo). Estados: patrulla/centinela, sospecha, investigar, combate, búsqueda. Combate: cobertura → esconderse/asomarse (de pie tras baja, de lado tras alta) → ráfagas con **error de puntería** que baja mientras te ve, supresión al perderte, recarga a cubierto, reposicionarse si la cobertura ya no protege, flanqueo, persecución a la última posición. `bl.AI.Debug 1` muestra estado/consciencia/turno sobre cada enemigo. Barks con voz vía `UBLAudioSubsystem` (Bloque 7) |
| `ABLCoverPoint` | `AI/BLCoverPoint` | Punto de cobertura baja/alta; protección real comprobada con trazados contra la posición del jugador; posición de disparo (encima o asomándose hasta 1,9 m) |
| `UBLSquadSubsystem` | `AI/BLSquadSubsystem` | Turnos de ataque (máx. 3 disparando a la vez), alerta a la escuadra y a los cercanos (35 m), reservas de cobertura, flanqueo (con ≥ 3 en combate; reintenta a los 3 s si no hay hueco) |
| `ABLMissionDirector` | `Mission/BLMissionDirector` | Objetivos (llegar, eliminar escuadra, interactuar), radio **con voz** (`FBLRadioLine::Voice`: entra 0,12 s tras el clic y el subtítulo dura lo que la voz; atenúa ambiente/música) y subtítulos en cola (descarta instrucciones obsoletas si el jugador va más rápido), resumen final (tiempo, bajas, precisión, a la cabeza, caídas), reinicio |
| `FBLObjective`, `FBLRadioLine` | `Mission/BLMissionTypes.h` | Datos de objetivos y frases de radio (los configura `build_m01_greybox.py`) |
| `ABLInteractable` | `Mission/BLInteractable` | Objeto con "[F] <acción>" y tiempo de mantener; se recoge (desaparece) |
| `ABLCharacter` (interacción) | `Player/BLCharacterInteraction.cpp` | Enfoca el interactuable más centrado a < 2,1 m (sin apuntar exacto), mantener F; con la misión completada F reinicia |
| `ABLHUD` | `UI/BLHUD` | **HUD definitivo** (sustituye a `ABLDebugHUD`): objetivo arriba a la izquierda (resaltado al cambiar), marcador con distancia pegado al borde si sale de pantalla, munición contextual (aparece al usarla, se atenúa a los 4 s; siempre visible si queda poca), mira, hitmarker, indicadores de daño, aviso de interacción con barra, subtítulos con indicativo, "PUNTO DE CONTROL", pantalla de caída y resumen de misión. Estilo "terminal táctico": ámbar + DroidSansMono/Roboto |
| `UBLAudioSubsystem` | `Audio/BLAudioSubsystem` | **Director de audio (Bloque 7)**: barks de la IA (11 categorías `EBLBark` × 3 voces, sonidos por nombre `VO_Bark_<Cat>_V<v>_<n>`; hueco global 0,6 s y por categoría para que no se pisen; dolor y bajas sin esperar turno; en 3D pegados a la cabeza; "¡Hombre abajo!" lo dice el compañero vivo más cercano < 30 m), **música dinámica** (golpe + bucle al entrar en combate si hace > 25 s del anterior; se funde 5 s después de que nadie esté en combate; cierre al completar la misión), **combate lejano** (ráfagas/explosiones cada 5–14 s y una sirena cada 2,5–4 min, a 150–250 m en direcciones aleatorias), **balas que pasan a < 3,5 m** del jugador (chasquido supersónico + silbido Doppler) y **mezcla de radio** (`SMix_RadioDuck`: ambiente ×0,5, música ×0,55) |
| `ABLReverbZone` | `Audio/BLReverbZone` | Caja acústica: con la cámara dentro activa su reverb (prioridad sobre la del exterior) y escala el ambiente de ciudad (`ABLGameMode::SetAmbienceScale`). En vez de AudioVolume (brushes no generables por script) |
| `ABLBuilding` | `Environment/BLBuilding` | **Edificio de fachadas modulares (Bloque 8)**: núcleo macizo (colisión, navegación) 25 cm por dentro y fachadas con instancias del kit (`gen_facade_kit.py`): plantas de 320 cm, módulos de ~400 cm estirados a cada cara; columnas de ventanas coherentes de abajo arriba, persianas a distintas alturas, balcones, aires acondicionados, bajantes, imposta, cornisa con peto; planta baja con locales (persiana/escaparate), portales y paños ciegos (más locales en `StreetMask`). Caras ocultas fuera (`FaceMask`). `bNoGroundFloor` para plantas sobre un bajo hecho aparte. Todo por `Seed` |
| `ABLSmokeEmitter` | `Environment/BLSmokeEmitter` | Humo y fuego ambiental con sprites ISM (M_FX_Dust, mismos datos por instancia que los impactos): columnas de humo lejanas (36 sprites enormes que suben y deriva con el viento) y fuegos (`bFire`: llamas aditivas M_FX_Flame, luz que parpadea sin sombras, sonido de fuego). Coste fijo |
| `UBLUserSettings` | `UI/BLUserSettings` | Opciones del jugador en `GameUserSettings.ini`: sensibilidad, invertir Y, FOV, movimiento de cámara (accesibilidad), volúmenes (general + `SMix_User` sobre SC_Music/SFX/Voice/Ambience; los sonidos sin clase son SC_SFX por `DefaultSoundClassName`). Gráficos vía `UGameUserSettings`: preajustes Baja/Media/**Alta (recomendada)**/Épica, pantalla, resolución, escala TSR (corrige el 0 que guarda el editor), VSync, límite de FPS |
| `SBLMainMenu` + `ABLMenuPlayerController`/`ABLMenuGameMode` | `UI/` | **Menú principal en Slate (C++)** sobre `L_MainMenu` (vacío): mapa topográfico de Kessra generado por código (`SBLTopoMap`: curvas de nivel por marching squares, costa, ría, línea negra, misiones), páginas Jugar / Selección de misión (+ punto de inicio) / Armamento / Inteligencia / Opciones / Salir. Teclado, ratón y mando. Música con fundido; sonidos de navegación. Despliega con `?BLStart=<Fase>` |
| `SBLOptionsPanel`, `SBLPauseMenu` | `UI/` | Opciones (Gráficos/Audio/Controles, se aplican al momento, se guardan al volver) compartidas con la **pausa** (Esc / Start: continuar, volver al punto de control, reiniciar misión, opciones, salir al menú; el juego queda en pausa) |
| `ABLGrenade` | `Weapons/BLGrenade` | Granada M-6 (Bloque 11): física, rebotes, espoleta, daño radial con caída, impulso, efectos (`UBLFXSubsystem::SpawnExplosion`), sonido, sacudida; lista de granadas vivas para la IA (`ABLAIController::TickGrenadeEscape`: grito + huida). Jugador: `BLCharacterGrenade.cpp` |
| `ABLCharacter` (combate) | `Player/BLCharacterCombat.cpp` | Daño recibido: sacudida de cámara/arma según el lado, destello, indicador, sonido. Pantalla dañada (viñeta + tinte rojo según la salud, sumados a la gradación del nivel) y latido con salud baja. Muerte: suelta todo, la cámara cae al suelo rodando, el arma cae, fundido a negro y reaparición a los 4 s en el checkpoint. Hitmarker + sonido |

**Jerarquía del personaje (desde el Bloque 2):** Capsule → CameraRoot (altura de ojos suavizada + pitch + lean) → Camera (bob/impulsos) → { WeaponRoot → WeaponMesh ; FirstPersonMesh }. **El arma manda**: el rig coloca WeaponRoot delante de la cámara y las dos manos del Mannequin la siguen por IK; el cuerpo FP queda fijo a la cámara (así nunca entra en cuadro al mover el arma). La malla FP hace tick después del personaje (sin retraso de un frame). El cuerpo completo (`Mesh`) es invisible para el jugador pero proyecta sombra.

**Calibración ADS:** la mira del arma (posición del socket `Sight`, o `SightLocalOffset`) se coloca en el eje de la cámara a `AimEyeDistance` (dato del arma: AR-7 = 9 cm). Poses de cadera/sprint/mantle/equipar = desplazamientos respecto a esa pose pivotando sobre la mira. **Recarga = pose absoluta** del arma en espacio de cámara (`ReloadGripLocation` + `ReloadRotation`). Encima se suman bob, sway, strafe e impulsos.

**Pase de realismo (2026-10-08):**
- **Recarga v2**: 2,0 s táctica (deja 30+1 en recámara) / 2,5 s en vacío. Trayectoria de la mano con Catmull-Rom (sin paradas): se acerca por debajo, tira del cargador en su eje, portacargadores, nuevo cargador, lo encaja y le da un golpe; en vacío golpea la retenida. El arma gira/se inclina con cada acción, impulsos al sacar/meter/asentar/retenida, la cámara acompaña (`ReloadCameraTilt`), foley de manos.
- **Rig**: temblor de cámara de alta frecuencia + golpe de FOV por disparo, inercia del arma con la aceleración, respiración (figura de 8, menor en ADS), giro y caída del arma en la transición a ADS, caída al agacharse.
- **Fogonazo**: llama lateral + estrella frontal en HLSL (forma distinta cada fotograma), más pequeño y sin "bola".
- **Brazos**: uniforme verde oliva mate y guantes (`MI_Soldier_*`) en vez del Mannequin blanco.
- **Arma**: `M_Weapon_Master` con máscaras horneadas en Blender (AO, aristas por nodo Bevel, cavidades) → desgaste de bordes, suciedad, grunge de rugosidad y micro-normal; instancias por pieza.

**AR-7 v2 (2026-10-08):**
- **Modelo** (`gen_ar7.py`, 23,4k tris): perfiles reales — receptor superior con hombros, ventana de expulsión con cerrojo visible y tapa abierta, deflector, asistente de cierre, raíl Picatinny ranurado, guardamanos octogonal **hueco** con ranuras M-LOK (se ven cañón, bloque y tubo de gases), apagallamas de jaula, pistolete con dedo y cola de castor, guardamonte curvo, selector/retén/botón del cargador, culata con ventana y cantonera estriada, cargador curvo tipo PMAG con nervios, mira trasera **de anillo** y delantera con capucha cerrada y tritio. Mismos huesos/sockets que v1. Booleanas exactas + biseles aplicados al final + triangulado en Blender (n-gonos cóncavos). `SM_AR7_Mag` se extrae del grupo `magazine` (mismas UV).
- **Material** v2: texturas de detalle propias y tileables (`Tools/textures/gen_weapon_detail.py`: micro-normal de grano fino, manchas de rugosidad, moteado de polímero, arañazos) a **tamaño real** usando la escala de UV que escribe el horneado (`T_AR7_Masks.json`). Horneado con 160 muestras + desenfoque (sin ruido).
- **ADS**: plano de recorte cercano a 2 cm (`DefaultEngine.ini`) para que se vea la mira trasera; ojo a 13 cm de la apertura (`AimEyeDistance`). Las poses de cadera/sprint/mantle/equipar se compensan con `PoseReferenceAimDistance` (9 cm) para no cambiar al variar la distancia de cada arma.
- **LOD** automáticos (100/40/15 %, `LODSettings_Weapon`) para la IA y el arma en el suelo.
- **Sprint** v2: arma alzada, ladeada (-28°) y cruzada a la izquierda en la mitad inferior, visible sin tapar la vista.

**Recarga procedural en primera persona (v1, sustituida por la v2):** línea de tiempo en `ABLCharacter::UpdateProceduralWeaponActions`: el arma se inclina, la mano izquierda (IK) va al cargador → lo saca (se oculta el hueso `magazine` y aparece `SM_AR7_Mag` en la mano) → fuera de cuadro → lo mete en `ReloadAmmoInsertTime` → (vacío: monta la palanca de carga) → vuelve al guardamanos. Impulsos al sacar, meter y montar. Sonidos sincronizados por fracción en `DA_AR7`.

### Controles (teclado/ratón)
WASD mover · Ratón mirar · Shift sprint (mantener) · C / Ctrl agacharse (alternar) · Espacio saltar / encaramarse · Q/E inclinarse (mantener) · Clic dcho. apuntar · Clic izq. disparar · R recargar · F interactuar · G granada · 1 fusil / 2 pistola · rueda del ratón = siguiente arma. Mando: mapeado básico (Y = siguiente arma).

### Jugar
Doble clic en `Tools/jugar.bat` (juego en ventana, sin editor, **menú principal** `L_MainMenu`, que es también el mapa por defecto) o `Tools/jugar_pruebas.bat` (mapa de pruebas `L_Dev_Movement`). O abrir `Blackline.uproject` y pulsar Play en el editor. Empezar en una fase concreta: `Tools\jugar_fase.bat Fase3` (Fase1 inserción, Fase2 patio, Fase3 control, Fase4 calle, Objetivo local, Bloque, Azotea (blindado), Muelle (extracción); arg. `-BLStart=<Fase>`). Build empaquetada (Bloque 9): `..\BlacklineBuild\Windows\Blackline.exe`.

### Pruebas automáticas
`bash Tools/run_test.sh [Prueba] [Mapa] [--nobuild]` compila, lanza el juego con `-BLTest=<Prueba>` y muestra resultados. Resultados y capturas (desde el render) en `Saved/BLTest/`.
- **Movement** (mapa `L_Dev_Movement`): 17 pasos — asentar, andar, sprint, frenar, agacharse, andar agachado, levantarse, ADS, soltar ADS, lean izq/der, salto, mantle 100/150, muro 250 (sin mantle), escaleras, lean contra pared.
- **Mission** (mapa de la misión, ~4 min; `BL_TIMEOUT=600`): 20 pasos — briefing, los 14 objetivos en orden (fases 1–9: callejón, patio, control, local, disco con F, bloque, despejar, liberar a Varek, que te siga, contraataque con 3 oleadas y humo, blindado en posición con 3 disparos + derrumbe + escombros, pasarela, escalera de incendios, zona de aterrizaje, defensa con 3 oleadas y ráfagas del helicóptero, aterrizaje y subida), marcador, pantalla de misión completada. `BL_TIMEOUT=600 bash Tools/run_test.sh Mission /Game/Maps/M01/L_M01_AmanecerRoto`
- **Menu** (`bash Tools/run_test.sh Menu /Game/Maps/Menu/L_MainMenu`): recorre las páginas con capturas, cambia opciones y comprueba que se guardan en el .ini (y las restaura), despliega la misión 1 desde el patio (jugador en su sitio y objetivo 2) y abre la pausa (juego en pausa).
- **Views** (mapa de la misión, ~60 s): 19 vistas fijas (inserción, callejón, patio, control, calle, fachadas, local, retrato de un miliciano, coche ardiendo, bloque por fuera/portal/escalera/pasillo, Varek, azotea, pasarela, escalera de incendios, muelle, agua) con fps medios y capturas; enemigos retirados salvo el del retrato. A 1080p: `BL_RESX=1920 BL_RESY=1080 bash Tools/run_test.sh Views /Game/Maps/M01/L_M01_AmanecerRoto --nobuild`. Perfil de GPU de una vista: `BL_ARGS="-BLProfileView=Calle"` y luego `python -I Tools/benchmark/gpu_profile_summary.py Saved/BLTest/Views_game.log 0.3 6`.
- **Audio** (mapa de la misión, con `-NoSound`: comprueba la lógica y que los sonidos existen): 12 pasos — recursos (66 barks, 11/11 frases de radio con voz), voz del briefing + atenuación de la mezcla, emisores de ambiente (16) y zonas acústicas (8), reverb activa en el local / callejón / calle, pasos de la patrulla, contacto (bark + golpe + música), balas que pasan cerca, "¡Hombre abajo!", la música se va al acabar el combate, combate lejano. `bash Tools/run_test.sh Audio /Game/Maps/M01/L_M01_AmanecerRoto --nobuild`
- **AI** (mapa de la misión): 12 pasos — 12 enemigos con controlador en patrulla, las 4 patrullas recorren su ruta, disparos oídos a 30–40 m (investigan), no detecta por la espalda agachado, detecta de frente y alerta a su escuadra, coberturas (en cobertura y protegidos del jugador), disparan con como mucho 3 turnos, recargan, flanquean, persiguen al perderlo, captura de cerca, bajas (ragdoll, salen de la escuadra). `bash Tools/run_test.sh AI /Game/Maps/M01/L_M01_AmanecerRoto`
- **Level** (mapa `/Game/Maps/M01/L_M01_AmanecerRoto`): 8 pasos (retira a los enemigos) — ruta de navegación furgón → objetivo, **recorrido a pie** (sprint, siguiendo la ruta: 245 m sin atascarse, cruzando los 4 checkpoints en orden) y vistas de cada fase con fps medios (mínimo 45). `bash Tools/run_test.sh Level /Game/Maps/M01/L_M01_AmanecerRoto`
- **Combat** (`L_Dev_Movement`, NO el de la misión; estaciones "Dianas" y "Checkpoint"): 12 pasos — maniquíes presentes, disparo al torso (daño 28, reacción física, hitmarker, superficie Carne), cabeza (×2,5), pierna (×0,75), salpicaduras de sangre en la pared, baja (ragdoll + hitmarker de baja), reaparición del maniquí, checkpoint, daño al jugador (salud, indicador), regeneración hasta el tope del segmento (75), muerte (cámara al suelo) y reaparición en el checkpoint con la munición guardada.
- **Weapons** (`L_Dev_Movement`, NO el de la misión; estación "Tiro": muro a 10 m + cajas con física): 16 pasos — equipar, mano izquierda en el agarre (< 3 cm), ráfaga 1 s (cadencia, impactos, munición, retroceso), recuperación de la mira, recarga, sprint cortado al disparar, vaciar cargador + recarga automática + cerrojo abierto (hueso desplazado), recarga en vacío + cerrojo cerrado, dispersión en ADS, ráfaga en ADS, captura de impactos. Capturas de la recarga a 0,4 / 1,0 / 1,4 s.
- **Pistol** (`bash Tools/run_test.sh Pistol` en `L_Dev_Movement`, estación "Tiro"): 17 pasos — inventario (AR-7 + P-17 17/51), cambio AR-7 → P-17 (0,80 s), manos en el puño (< 3 cm), semiautomática (1 disparo con el gatillo apretado), cadencia pulsando (7 en 1,4 s) y retroceso, recuperación, ADS (mira a 34 cm en el eje de la cámara, 0,06 cm), disparo en ADS, recarga táctica 17+1 con el cargador en el suelo, vaciar (corredera abierta 2,4 cm) + recarga automática, recarga en vacío (corredera cerrada), cambio a mitad de recarga (conserva la munición), ráfaga del AR-7, rueda, arrepentirse a medio cambio, sprint, checkpoint con la pistola. Capturas de cadera, ADS, disparo y cada momento de las dos recargas. **17/17, 123 fps (2026-10-09).** `PistolHands` (ajuste visual, no regresión): giros candidatos de la mano izquierda; `-BLGripRot=P,Y,R` afina alrededor de uno. Hojas de contactos: `blender -b --factory-startup -P Tools/contact_sheet.py -- <salida.png> <columnas> <ancho> <capturas...>`.
- **Mission2** (`BL_TIMEOUT=600 bash Tools/run_test.sh Mission2 /Game/Maps/M02/L_M02_Manifiesto`): 17 pasos — briefing, noche (a oscuras/lejos/bajo farola), valla, tanques, almacén, fotos (una con F), alarma + focos, 3 oleadas con la ametralladora de supresión, dron derribado, gas que corta la calle, zanja, muelle, lancha, completada. **17/17, 92 fps (2026-10-09).**
- **Mission3** (`... Mission3 /Game/Maps/M03/L_M03_Ria`): 17 pasos — puente, casco, casas, brecha (puerta volada, 2 aturdidos), casa del práctico, cuaderno, láser y fijación del tirador, campanario, pescadores, operadores (blindaje, granadas), baliza, barco que zarpa (85 m), furgón. **17/17, 95 fps (2026-10-09).**
- **Mission4** (`... Mission4 /Game/Maps/M04/L_M04_FuegoCruzado`): 13 pasos — trinchera con morteros, defensa con % de enlace y aliados disparando (220 disparos), montar la ametralladora, carga sobre el puente (299 disparos, se recalienta), bajarse, cazas sobre el BTR, cargas, voladura (el tramo cae al río), vado, retirada. **13/13, 97 fps (2026-10-09).**
- **Mission5** (`... Mission5 /Game/Maps/M05/L_M05_LineaNegra`): 13 pasos — compañeros que siguen, terminal, contrarreloj con cuenta atrás, termita, descarga, azotea (el inglés y el helicóptero), helicóptero inutilizado, detención, extracción, fin de campaña. **13/13, 102 fps (2026-10-09).** **Mission5Fallo** (`BL_ARGS="-BLStart=Azotea" ... Mission5Fallo /Game/Maps/M05/L_M05_LineaNegra`): el fuego ajeno no le mata; si le mata el jugador, misión fallida. **3/3 (2026-10-09).**
- Regresión del 2026-10-09 tras los cambios compartidos: Mission 20/20 (91 fps), AI 12/12 (96 fps), Menu 8 + despliegue + pausa.
- Las estaciones del mapa se localizan por TargetPoints con tag `BLTest_Start_<Nombre>`.

### Estructura C++ prevista (plan inicial de la Fase 1; la real es la tabla "C++ implementado")
```
Source/Blackline/
  Player/   BLCharacter, BLPlayerController, BLCameraComponent
  Weapons/  BLWeaponBase, BLWeaponData, BLRecoilComponent, BLProjectile
  Combat/   BLHealthComponent, BLDamageTypes, BLImpactEffects
  AI/       BLAICharacter, BLAIController, BLSquadSubsystem, BLCoverPoint, tareas StateTree
  Mission/  BLMissionSubsystem, BLObjective, BLCheckpointSubsystem
  UI/       HUD, menús
  Core/     GameMode, GameInstance, SaveGame, Settings
```

### Presupuestos de rendimiento (GTX 1660 Super 6 GB)
- 1080p, objetivo **60 fps** (16,6 ms), mínimo 45.
- Escena de benchmark: 13,0 ms GPU (72 fps) → **margen de ~3,6 ms** para personajes, armas, VFX y post-proceso.
- VRAM < 4,5 GB.
- Texturas: 2K armas/personajes, 1K props, 512 props pequeños.
- ≤ 8 enemigos activos en combate, ≤ 12 vivos.
- Sombras dinámicas: sol + 1–2 luces clave.

---

## 3b. Nivel de la misión 1 — `L_M01_AmanecerRoto` (nivel gris)
Coordenadas en cm, +X = este (hacia donde se avanza), +Y = sur. Generado por `Tools/UnrealPython/build_m01_greybox.py`.
| Fase | Zona | Contenido de juego |
|---|---|---|
| 1 Inserción | Calle trasera (x 0–6000, 8 m de ancho) | Furgón con puertas abiertas; inicio junto a él. Calle cerrada al este (la manzana M1) |
| 2 Infiltración | Callejones A (x 28–33 m) y B (48–54 m, valla saltable de 110 cm) → patio del puerto (75 × 49 m) | Filas de contenedores (algunos apilados o abiertos) con varias rutas y puntos ciegos; nave al fondo; palés. Checkpoint "Infiltracion" en el callejón A |
| 3 Contacto | Puerta del patio → carretera N-S (14 m) | Control: jerseys en zigzag, sacos en esquina a ambos lados, caseta, barrera, coche calcinado y T-walls; farolas de sodio. Checkpoint "Contacto" en la puerta |
| 4 Calle | Calle principal E-O (x 104–195 m, 14 m) | Coches calcinados, jerseys, T-walls, contenedor y escombros alternando lados; posición de sacos enemiga al fondo; callejones C/D (norte) y E/F (sur) unidos por patios traseros = rutas de flanqueo. Checkpoint "Calle" en el cruce |
| Objetivo | Local al final de la calle (x 195–210 m) | Planta baja abierta con mostrador y mesa; `TargetPoint` "Objetivo_Disco". Checkpoint "Objetivo". Encima, 3 plantas de viviendas solo de fachada (`ABLBuilding` sin planta baja) |
| 5–9 | *Sin construir* | Bloque de viviendas con interiores y escaleras, posición a defender, BTR, derrumbe, tejados y muelle de extracción |

## 4. Pipeline de assets (Blender → Unreal)
- Unidades: metros en Blender, escala 1.0 → Unreal 1 m = 100 cm.
- Export FBX: `apply_scale_options=FBX_SCALE_UNITS`, forward `-Y`, up `Z`, smoothing `FACE`, sin leaf bones.
- Nombres: `SM_` estático, `SK_` skeletal, `T_` textura, `M_`/`MI_` material, `UCX_` colisión.
- Librería común: `ArtSource/Blender/scripts/bl_lib.py` (cajas, cilindros, bevel, export).
- Previsualización sin abrir Blender: `preview_render.py`.
- Ejecutar un generador: `blender -b --factory-startup -P <script.py> -- <salida.fbx> [args]`.

### Assets creados
| Asset | Script | Tris | Uso |
|---|---|---|---|
| SM_BM_Apartment_A | `gen_benchmark_building.py` | 53.684 | Benchmark (Nanite) |
| SK_AR7 (11 huesos) + SM_AR7_Mag | `gen_ar7.py` (`--bake` para las máscaras; copiar `T_AR7_Masks.png/.json` a `ArtSource/Textures/Weapons`) | 23.416 / 1.100 | Arma del jugador (v2). 89 cm, origen en el pistolete, +Y adelante. Material maestro con desgaste |
| SM_Casing_556, SM_AR7_MuzzleFlash, SM_ImpactChip | `gen_weapon_fx.py` | 124 / 6 / 20 | Casquillo, fogonazo, esquirla (sin Nanite) |
| SK_P17 (10 huesos) + SM_P17_Mag + SM_Casing_9mm | `gen_p17.py` (reutiliza las primitivas de `gen_ar7.py`; `--bake` → copiar `T_P17_Masks.png/.json` a `ArtSource/Textures/Weapons`) | 7.988 / 428 / 116 | Pistola del jugador: 21 cm, corredera (`slide`) que retrocede y queda abierta, cargador por el eje del puño, miras de muesca con punto de tritio. `setup_p17_assets.py` importa todo (sonidos de `Tools/audio/gen_p17_sfx.py`) y crea `DA_P17` |
| SM_Cover_Sandbag_Straight / _Corner, SM_Cover_Jersey, SM_Cover_TWall | `gen_level_props.py` (+ `bl_kit.py`) | 8,3k / 15k / 0,8k / 0,4k | Coberturas con medidas finales (sacos 115 cm, jersey 107, T-wall 370). Nanite, colisión UCX |
| SM_Container_20ft (+ _Open), SM_Veh_Van_Civil (+ _Open), SM_Veh_Sedan_Wreck | `gen_level_props.py` | 6,4k / 6–7k / 3k | Contenedor corrugado con puertas y barras; furgón (inserción, puertas traseras abiertas); coche calcinado sin cristales. Materiales triplanares (`setup_level_assets.py`) |
| SM_Obj_Laptop_Rugged, SM_Obj_HardDrive | `gen_level_props.py` | ~3k / ~1k | Objetivo de la misión (portátil y disco con LED emisivo) |
| SM_Fac_* (15 piezas: ventanas, balcón, persiana, locales, portal, cornisa, imposta, bajante, aire acondicionado, rótulo) | `gen_facade_kit.py` | 0,2–1,8k | Kit de fachadas modulares de `ABLBuilding` (Bloque 8). Nanite, UCX en los muros |
| SM_Prop_* (contenedor de basura, bidón, neumáticos, palé/pila, caja, poste, cable, 3 escombros, bolsas, bolardo) | `gen_street_props.py` | 0,1–7k | Atrezo de calle (Bloque 8). Nanite, UCX |
| SM_Militia_Vest / Helmet / Armband | `gen_militia_gear.py` | 3,9k / 0,7k / 0,1k | Equipo del miliciano en huesos del Mannequin (Bloque 8) |

**Audio del Bloque 7** (`Tools/audio/`): `voice_lines.tsv` = tabla única de frases (id, tipo radio/bark, quién, voz, tono, velocidad, texto); `gen_voices.py` las sintetiza con las voces de Windows (`tts_lines.ps1`, WinRT, voz es-ES "Pablo") y las procesa (radio: banda 400–3000 Hz, saturación, compresión, soplido y "kssh" al soltar; barks: voz forzada, 3 voces de milicianos con tono/velocidad distintos). `gen_world_audio.py <fuentes> ArtSource/Audio/SFX`: balas (onda N + reflejo; silbido Doppler), ambientes de zona en bucle (agua, fuego, farola, viento en callejón, sala), combate lejano (ráfagas reales CC0 filtradas con eco, explosiones, sirena), chirridos metálicos y música (pulso táctico en re menor a 100 ppm, 19,2 s, + golpes de contacto y de cierre). `setup_audio_world.py` (commandlet) importa esas carpetas, crea `SA_Voice/Bullet/Distant/AmbSmall/AmbLarge`, `SCon_Voice`, clases `SC_SFX/Voice/Music/Ambience`, `SMix_RadioDuck`, `RE_Alley`, `RE_Interior`. `build_m01_greybox.py` lee el texto de la radio de la tabla por id, asigna la voz y coloca emisores (agua al norte del patio, zumbido en las 6 farolas, viento en los 6 callejones, sala del local) y zonas acústicas (callejones, calle trasera, local). Los sonidos que se cargan por nombre están en `DirectoriesToAlwaysCook` (`DefaultGame.ini`).
**Arte del Bloque 8**: `gen_facade_kit.py` (Blender → `export/Facade`) + `setup_facade_assets.py` (mallas Nanite en `/Game/Environment/Facade`, `MI_Fac_Wall_<Color>` ×6, carpintería/piedra/metal triplanares, `M_Fac_Glass` con interior/cortina/luz por instancia, `M_Fac_Sign` descolorido). `gen_street_props.py` (→ `export/Props`, lo importa `setup_level_assets.py`): contenedor de basura, bidón, neumáticos, palés, caja, poste + cable con catenaria, 3 montones de escombro, bolsas, bolardo. `add_env_grime.py`: variación a gran escala, chorretones y suciedad a ras de suelo en `M_Env_Triplanar` (parámetro `GrimeStrength`). `setup_fx_env.py`: `M_FX_Flame`. `gen_militia_gear.py` + `setup_gear_assets.py`: equipo del miliciano con `M_Gear_Triplanar` (triplanar en espacio LOCAL: no "nada" al moverse). `inspect_refpose.py` vuelca la pose de referencia del Mannequin. En el nivel (`build_m01_greybox.py`): `finish_buildings()` (caras compartidas y calles), `dressing()`, `effects()`; iluminación con exposición local y más luz de cielo.
Orden de scripts de assets (idempotentes): `setup_audio_fx.py` (sonidos, texturas, materiales de efectos y entorno, PM_*, `DA_SurfaceEffects`) → `setup_weapon_assets.py` (AR-7, materiales del arma y del soldado, `DA_AR7`) → `build_dev_movement_map.py`. `make_reverb.py` crea `RE_UrbanOutdoor`. Nivel: `gen_level_props.py` (Blender) → `setup_level_assets.py` → `build_m01_greybox.py` (con `-ExecCmds="py ..."`: espera a que se genere el NavMesh y comprueba las rutas antes de guardar).
**Sonido real (CC0)**: AR-15 cercano/medio y ráfagas de AK/PPSh (Free Firearm Sound Library), recarga y cerrojo reales. Procesado con `Tools/audio/process_sfx.py` (+ `audiolib.py`): 12 variaciones de disparo cercano por capas, cola real que suena al acabar cada disparo/ráfaga, versión 3D y lejana para la IA. Impactos, casquillos y pasos por superficie, foley y ambiente (viento, rumor, tiroteos lejanos reales filtrados): síntesis. Atenuación con absorción del aire, envío a reverb, oclusión en impactos, límites de voces. Ver `Docs/Creditos_Assets.md`.
**Texturas reales (CC0)**: 8 juegos 2K de Poly Haven en `ArtSource/Textures/PolyHaven`, material de entorno triplanar `M_Env_Triplanar` (color, normal con mezcla "whiteout" y ARM) + instancias con superficie física.
Galería de tiro: dianas de acero, tablones, cristal, tierra y hormigón (estación "Materiales").

### Assets externos aprobados
Mannequin y Game Animation Sample de Epic, Mixamo, texturas CC0 (Poly Haven, ambientCG), Fab (según licencia), sonidos royalty-free (Sonniss GDC, CC0).

---

## 5. Problemas conocidos
### Abiertos
- **Voces PROVISIONALES** (síntesis de voz de Windows, una sola voz masculina con tres tonos): suenan algo robóticas, sobre todo los gritos. Sustituir por actores de voz antes de publicar (las condiciones de uso de las voces de Windows no cubren distribuirlas). La tabla `voice_lines.tsv` sirve de guion para grabarlas.
- **Música sintetizada** (pulso + cuerdas de diente de sierra + percusión): funcional pero sencilla; sustituir por una pista compuesta/licenciada cuando se pueda. No hay música fuera del combate (decisión: solo cuando sea apropiada).
- Los chirridos de contenedor están generados pero sin colocar (el fuego ya suena en los fuegos del Bloque 8).
- **MetaSounds no se usa**: las capas del disparo (cercana/lejana con mezcla por distancia, cola) se resuelven en C++ con SoundWaves (los MetaSounds no se pueden generar y probar por script con fiabilidad). Pendiente de valorar en la Fase 4.
- **Misión**: solo fases 1–4 + disco (ver la tabla de la misión 1). Al completar, F reinicia el nivel (o Esc → salir al menú). Los enemigos muertos siguen muertos tras reaparecer en un checkpoint (no se restaura el mundo). Sin guardado de progreso (el menú no tiene "Continuar").
- **IA**: sin animaciones de agacharse ni de muerte reales (agacharse procedural, muerte = ragdoll); sin gemido al morir (solo golpe del cuerpo contra el suelo); pasos por distancia recorrida, no sincronizados con los pies de la animación; sin granadas (Operador Corvane, Fase 4); la IA no salta obstáculos (NavLinks pendientes). Primer impacto/ragdoll de la partida: pico de ~400 ms en la build de editor.
- Dificultad provisional: el jugador recibe el 55 % del daño (`DamageTakenMultiplier`); menú de dificultad en la Fase 4.
- **Arte (Bloque 8) pendiente**: la nave del puerto, los muros del patio y los límites del nivel siguen siendo cajas con textura; los edificios no tienen interiores (ventanas opacas con "interior" falso); sin cristales que se rompan, sin decals de pintadas/carteles/charcos; el cuerpo del miliciano y los brazos del jugador siguen siendo el Mannequin (con equipo encima). El fuego no daña al jugador.
- **Rendimiento a 1080p** (build de editor): 62–77 fps sin enemigos en el preajuste Alto; con combate y 12 enemigos hay que medirlo en el Bloque 9 (y en build empaquetada, que suele ir algo mejor).
- La valla del callejón B (110 cm) no está en el NavMesh: la IA necesitará NavLinks para saltar obstáculos.
- **Dianas = Mannequin blanco de Epic** (provisional hasta el miliciano del Bloque 5). Sin animaciones de reacción, solo física. Sangre sin marca sobre el cuerpo (los decals no se proyectan en personajes).
- Primer ragdoll/salpicadura de la partida: pico de ~400 ms en la build de editor; en la build empaquetada el peor frame baja a ~150–200 ms (sobre todo al empezar): falta precachear PSO.
- Sonidos de combate **sintetizados** (`Tools/audio/gen_combat_sfx.py`): impacto en carne, hitmarker, daño recibido, latido. Sustituir por grabaciones si se consiguen (no se encontraron CC0 buenas en el Bloque 7).
- **AR-7**: detalles finos pendientes (marcajes, grabados, moleteado). La palanca de carga no se anima (la recarga en vacío usa la retenida).
- **Impactos, casquillos y pasos sintetizados** (no hay grabaciones CC0 buenas): sustituir por grabaciones reales en el Bloque 7 si se consiguen (p. ej. Sonniss GDC).
- **Brazos**: siguen siendo el Mannequin (tintado). Brazos de soldado propios: pendiente (un brazo humano realista no es viable por script; candidatos: Fab/Mixamo con licencia válida).
- **Pose de recarga encuadrada "a ojo"** (`ReloadGripLocation`/`ReloadRotation` en `FirstPersonRig`): el AR-7 queda grande en pantalla; pulir junto al pase de arte.
- **Animación FP de base**: el idle de Epic solo da hombros/codos; todo lo demás es procedural. Sin animación de inspección ni de cambio de arma (Fase 4).
- **Pose de sprint procedural** (v2, 2026-10-08): mejorará con animación real. Regla: ninguna pose debe acercar el arma a < 10 cm de la cámara (plano de recorte cercano), o se ve cortada (corregido 2026-10-08 tras la prueba del usuario).
- **Cuerpo para el mundo/sombra** sin locomoción: reproduce el idle en bucle (solo afecta a la sombra del jugador).
- **Mantle a agachado no soportado**: si encima del obstáculo no cabe de pie, no se permite el mantle.
- Deslizamiento (slide) no implementado: pendiente de decidir si aporta (diseño: "a validar").
- Picos de ~170 ms en la prueba automática coinciden con las capturas (lectura de GPU); no afectan al juego.

### Notas técnicas / herramientas
- En builds de editor, `-csvExitOnCompletion` no cierra el juego; `run_benchmark.py` lo cierra al detectar el CSV.
- En UE 5.8, `-ExecutePythonScript` cierra el editor al terminar el script. Para scripts que esperan frames usar `-ExecCmds="py <script>"`.
- En Git Bash, pasar rutas `/Game/...` con `MSYS_NO_PATHCONV=1`.
- `CrouchedEyeHeight` ya existe en `ACharacter`: UHT no permite sombrear miembros (se usa `EyeHeightCrouched`).
- MSVC: la carpeta se llama 14.44.35207 pero `cl.exe` es 19.44.35229 (válido; UE 5.8 prohíbe < 35211).
- Las animaciones de fusil de Epic son de **tercera persona**: en primera persona sacan el arma de cámara. Se usan solo para el cuerpo/IA; la vista FP es procedural.
- Huesos exportados desde Blender llegan con **rotación (roll 90) y escala ×100** en la pose de referencia: de los huesos-socket solo se usa la **posición**.
- La importación (Interchange) activa **Nanite** en las mallas estáticas: desactivarlo en mallas con materiales aditivos/translúcidos o diminutas. Materiales usados con ISM necesitan `used_with_instanced_static_meshes`.
- `MaterialEditingLibrary.delete_all_material_expressions` provoca una aserción en commandlets: los materiales solo se crean si no existen (para regenerarlos, borrarlos antes).
- `run_test.sh` toma el mapa como 2º argumento: `run_test.sh Weapons --nobuild` carga el mapa por defecto (la misión) y falla; usar `run_test.sh Weapons /Game/Maps/Dev/L_Dev_Movement --nobuild`.
- Las pruebas se ejecutan en una ventana: el ratón del usuario movía la cámara → `SetIgnoreLookInput` durante la prueba.
- `new_level` sobre el mapa ya cargado **no lo vacía**: el mapa acumulaba copias de todos los actores en cada reconstrucción (138 actores). El script ahora borra todo antes.
- El `PhysMaterial` de una instancia de material no llega a la colisión simple (`GetPhysicalMaterial()` devuelve el por defecto) → `ResolveSurface` + `PhysMaterialOverride` en el mapa.
- El enum `EPhysicalSurface` se expone incompleto a Python (`UBLScriptLibrary.set_physical_surface`).
- ISM con datos por instancia: usar `SetNumCustomDataFloats` y `MarkRenderStateDirty` o los datos no llegan a la GPU (partículas invisibles).
- **Materiales para mallas esqueléticas creados por script**: activar `used_with_skeletal_mesh` o en el juego se dibujan con el material por defecto (gris moteado). Era la causa del aspecto de "lija" del AR-7.
- Las listas de structs de UE en Python (`materials`, `static_materials`) devuelven **copias**: construir una lista nueva y asignarla entera.
- Diagnóstico: `BL_EXEC="r.X 0" bash Tools/run_test.sh ...` pasa comandos de consola a la prueba.
- Si el usuario tiene el juego abierto (`jugar.bat`), los assets quedan bloqueados y los scripts de importación fallan al guardar.
- Si hay una ventana del juego/editor abierta (Live Coding), no se puede compilar ni guardar los assets que tiene cargados.

### Resueltos
- ~~Mano derecha del Mannequin tapando la corredera de la P-17 en ADS~~ → el Mannequin agarra todas las armas como el fusil; en un puño de pistola quedaba 4 cm alta y 2 cm adelantada: `RightHandOffset/RightHandRotation` por arma (medido en la prueba con la posición de los huesos de la mano en espacio del arma) (2026-10-09).
- ~~Misiones 2–5 sin compilar~~ → sesión de verificación del 2026-10-09. Fallos encontrados y arreglados: `Role` sombreaba a `AActor::Role` (→ `EnemyRole`, en Python `enemy_role`); `GetTarget` con tipo incompleto en `BLAllyController.h`; `SetPhysMaterialOverride` del dron en el constructor (fallo en el CDO, rompe el cocinado → `BeginPlay`); `bFlashlight` y el componente `Flashlight` con el mismo nombre en Python (→ `ScriptName="FlashlightSpot"`); `covers_for_building` faltaba en `bl_levelkit`.
- ~~Escaleras de las casas de M03 sin NavMesh~~ → el primer peldaño estaba pegado al muro y solo se entraba de lado por una franja más estrecha que el radio del agente: tramos de 16 peldaños con rellano de 1,25 m al pie (2026-10-09).
- ~~Las puertas no avisaban al director (objetivo "Entra en la casa del práctico" nunca se cumplía)~~ → `ABLDoor::Use` llamaba a `Super::Use` después de poner la carga/abrir y el `CanInteract` de la puerta ya fallaba: ahora primero el padre (2026-10-09).
- ~~Muros invisibles que tapaban la vista, las balas y la cámara~~ → perfil de colisión `BLInvisibleWall` (DefaultEngine.ini): solo cuerpos. Las respuestas por canal sueltas desde Python no se guardaban con el mapa (2026-10-09).
- ~~Vigas del tramo del puente con colisión maciza hasta 6 m~~ → solo hasta la barandilla (1,10 m): la celosía es calada (2026-10-09).
- ~~El tramo volado no caía~~ → apoyaba justo en el estribo y la pila y con física se quedaba encima: caída guiada (gravedad + giro hasta `FallDepth`) (2026-10-09).
- ~~La ametralladora no se recalentaba nunca~~ (enfriaba más de lo que calentaba) → 0,02 por disparo: ~9 s de fuego continuo (2026-10-09).
- ~~Aliados sin disparar~~ → alcance 65 → 120 m (los milicianos aparecen a ~100 m) y vista desde la altura de pie (agachados tras los sacos soltaban el blanco en cada pausa) (2026-10-09).
- ~~AR-7 "de bloques" con aspecto de lija~~ → AR-7 v2 + material real (2026-10-08).
- ~~Mira trasera invisible en ADS~~ → plano de recorte cercano a 2 cm (2026-10-08).
- ~~Mano izquierda sin IK~~ → IK de ambas manos al arma (2026-10-08).
- ~~`.umap` de solo lectura (Git LFS `lockable`)~~ → atributo quitado de `.gitattributes` (2026-10-08).
- ~~Visual Studio no instalado~~ → VS 2022 17.14 instalado 2026-10-07.
- ~~VC++ Redistributable desactualizado~~ → 14.50.35719 instalado 2026-10-07.

## 6. Tareas pendientes
### Bloque 1 — cierre
- [x] **Prueba de sensaciones por el usuario** (2026-10-08: bien; arma recortada en sprint → corregido) (velocidades, bob, sway, FOV, lean, mantle). Ajustables en el editor: `BLCharacter` (categorías Movement/Lean/Mantle) y `FirstPersonRig` (Rig|*).

### Bloque 2 — Sistema de armas + AR-7 ✅
- [x] `UBLWeaponData` + `UBLWeaponComponent` (componente en vez de actor `ABLWeapon`: lo reutiliza la IA): hitscan, cadencia, munición, recarga, dispersión.
- [x] Retroceso: subida + desvío con tendencia y rampa (en vez de curva) + kick de cámara/arma + recuperación.
- [x] Animación FP en C++ con IK de ambas manos; recarga y equipar procedurales (ver decisiones).
- [x] Fogonazo, casquillos, impactos (decal + esquirlas + Niagara provisional), sonido provisional.
- [x] AR-7 en Blender con huesos-socket `Sight`, `Muzzle`, `Eject`, `Mag`, `HandGrip_L` y cargador desmontable.
- [x] Prueba automática `Weapons` (11/11).
- [ ] **Prueba del usuario** (sensación de disparo, retroceso, recarga, sonido).

### Bloque 3 — Salud, daño y muerte ✅
- [x] `UBLHealthComponent` (segmentos con regeneración parcial), zonas de impacto (cabeza ×2,5 / torso ×1 / extremidades ×0,75), muerte del jugador y reaparición en checkpoint.
- [x] Dianas (`ABLTargetDummy`): reacción física al impacto → ragdoll al morir; reaparecen.
- [x] Feedback: sacudida por daño, indicador direccional, pantalla dañada, latido, hitmarker (sonido + rojo en bajas), impacto en carne (neblina de sangre, sonido, salpicadura en la pared de detrás).
- [x] Prueba automática `Combat` (12/12).
- [ ] **Prueba del usuario** (estación "Dianas" del mapa de pruebas: desde el inicio, a la derecha, pasada la galería de tiro; el checkpoint está a su entrada).

### Bloque 4 — Nivel gris ✅
- [x] Grey-box de las fases 1–4 de "Amanecer roto" (`L_M01_AmanecerRoto`, ~215 × 125 m, recorrido de 245 m): ver sección "Nivel de la misión 1".
- [x] Coberturas, contenedores y vehículos con geometría final (Blender) y edificios en cajas.
- [x] NavMesh estático (agente 34 × 184) con rutas completas entre fases; checkpoints por fase de todo el ancho del paso.
- [x] Prueba automática `Level` (7/7).
- [ ] **Prueba del usuario** (`Tools/jugar.bat`): recorrido, escala, lectura de las rutas, coberturas.

### Bloque 5 — IA miliciano ✅
- [x] Miliciano (cuerpo, arma, animación en C++), controlador con percepción y estados, coberturas (351 puntos generados por el script del nivel alrededor de coberturas, contenedores, vehículos y esquinas), coordinador de escuadra.
- [x] 12 enemigos en la misión: patio (2 patrullas + centinela de la puerta), control (2 en los sacos, caseta, patrulla), calle (2 en los sacos, patrulla, centinela del callejón D, guardia del local).
- [x] Prueba automática `AI` (12/12).
- [ ] **Prueba del usuario** (`Tools/jugar.bat`; consola: `bl.AI.Debug 1`).
- [ ] Enlaces de navegación (NavLink) para que la IA salte obstáculos de mantle (p. ej. la valla del callejón B).

### Bloque 6 — Misión + HUD + checkpoint ✅
- [x] Misión 1 jugable de principio a fin: 5 objetivos (entrar por los callejones, cruzar el patio, neutralizar el control, llegar al local, recuperar el disco), briefing y radio con subtítulos (TORRE / SABLE 2-1), marcador con distancia, interacción con F, portátil y disco duro (Blender, LED emisivo), resumen y reinicio.
- [x] HUD definitivo (`ABLHUD`).
- [x] Prueba automática `Mission` (9/9).
- [ ] **Prueba del usuario** (`Tools/jugar.bat`: jugar la misión entera).

### Bloque 7 — Audio base ✅
- [x] Voces de radio (11 frases) y barks de la IA (11 categorías × 2 frases × 3 voces), provisionales con TTS.
- [x] Pasos de la IA por superficie, golpe del cuerpo al caer, mezcla cercano/lejano del disparo de la IA por distancia, chasquido/silbido de las balas que pasan cerca.
- [x] Música solo en combate + cierre de misión; atenuación de ambiente/música con la radio (SoundMix).
- [x] Ambiente por zona (agua, farolas, viento en callejones, sala) + combate lejano + acústica por zona (callejón, interior).
- [x] Prueba automática `Audio` (12/12).
- [ ] **Prueba del usuario** (`Tools/jugar.bat`, con auriculares: radio, gritos de la IA, balas cerca, música al entrar en combate, eco en callejones).

### Bloque 8 — Pase de arte ✅
- [x] Kit de fachadas modulares (15 piezas) + `ABLBuilding` en los 16 edificios y sobre el local del objetivo (2.000+ instancias Nanite).
- [x] Atrezo de calle (13 mallas) colocado sin cerrar rutas; líneas eléctricas.
- [x] Suciedad y variación en el material de entorno; iluminación (exposición local, luz de cielo).
- [x] Humo lejano, coche ardiendo y bidón con fuego (luz + sonido).
- [x] Equipo del miliciano (chaleco, casco, brazalete).
- [x] Medición a 1080p y preajuste Alto (de 45 a 62–77 fps).
- [ ] **Prueba del usuario** (`Tools/jugar.bat`): aspecto de calles, fachadas, humo/fuego, enemigos.
- [ ] Pendiente de arte (ver problemas conocidos): nave del puerto, decals, cristales rompibles, cuerpos/brazos propios.

### Bloque 9 — Test + optimización (Fase 3) ✅
- [x] Compilación (editor y juego, sin avisos), cocinado sin avisos, build empaquetada Win64 Development (`..\BlacklineBuild\Windows\Blackline.exe`, ~1,1 GB).
- [x] Todas las pruebas automáticas en la build: Menu (incl. despliegue y pausa), Mission 9/9, Audio 12/12, AI 12/12, Views 9/9, Level 8/8, Movement 17/17, Weapons 16/16, Combat 12/12. Rendimiento a 720p: 90–137 fps; a 1080p (editor): 62–77 fps sin enemigos, ~71 fps en combate.
- [x] Fallos encontrados y arreglados: (1) la build de juego no compilaba (`GetActorLabel` es solo del editor); (2) materiales sin el uso Nanite/instancias (en la build se habrían visto con el material por defecto) → `fix_material_usage.py`; (3) aviso de CommonUI; (4) **crash al cargar la misión en la build** (la animación procedural de la IA: `FCSPose::SafeSetCSBoneTransforms` fallaba con la pose cocinada) → espacio de componente propio (`FSimpleCSPose`); (5) assets cargados por nombre que no se cocinaban (mezclas de audio, sonidos del menú) → `DirectoriesToAlwaysCook` + comprobación en la prueba Audio; (6) escala de resolución 0 guardada por el editor (habría renderizado a resolución mínima al cambiar opciones).
- Cómo pasar las pruebas en la build: `MSYS_NO_PATHCONV=1 ./Blackline.exe <mapa> -windowed -ResX=1280 -ResY=720 -NoSound -unattended -BLTest=<Prueba>` desde `BlacklineBuild/Windows`; resultados en `Blackline/Saved/BLTest`. (Sin `MSYS_NO_PATHCONV` Git Bash convierte `/Game/...` en una ruta de Windows y el juego se cierra.)
- [ ] Pendiente: medir en la build a 1080p con combate; pico de ~150–200 ms en el primer frame de cada prueba (carga de shaders/PSO).

### Bloque 11 — Misión 1 completa: fases 5–9 (propuesto)
Requisitos del diseño aprobado que faltan (tabla de la misión 1). Orden propuesto, cada parte verificada antes de la siguiente:
- [x] **Granada M-6** (2026-10-09): G / RB, 2 de inicio (máx. 3), anilla + lanzamiento con el arma bajada, física con rebotes y sonido, espoleta de 3,5 s, daño radial con caída (150 → 12, letal < 2 m, paredes protegen), impulso a ragdolls, destello, polvo/humo/chispas/cascotes/marca, sonido cercano o lejano, sacudida al jugador, contador en el HUD. La IA la ve: grita "¡Granada!" y huye. Pruebas `Grenade` 6/6 (mapa de pruebas) y `GrenadeAI` 2/2 (misión). `gen_grenade.py`, `gen_grenade_sfx.py`, `setup_grenade_assets.py`.
- [x] **Humo** (2026-10-09): granada de humo (`ABLGrenade::bSmoke`) → `ABLSmokeEmitter::ConfigureSmokeScreen` (crece en 3 s, dura ~25 s); `BlocksSight` corta la línea de visión de la IA.
- [x] **Fase 5 — Asalto al bloque de viviendas**: edificio con interiores (portal, escalera, rellanos, 3 plantas de pisos con puertas, ventanas transitables desde dentro), NavMesh por plantas, milicianos dentro (CQB), puertas que se abren con F.
- [x] **Fase 6 — Rescate de Varek**: Varek atado en un piso del último nivel; liberarlo (F mantenido), que te siga (IA aliada simple), el disco en su piso.
- [x] **Fase 7 — Contraataque**: defender el piso durante oleadas de la Columna (escuadras que llegan por la calle y los patios), granadas de humo enemigas, radio de TORRE con cuenta atrás.
- [x] **Fase 8 — BTR y derrumbe** (2026-10-09): `gen_btr.py` (casco 8x8, torreta y cañón en mallas separadas para girar/elevar), `Vehicles/BLBTR` (recorrido, frenada, torreta 40°/s y cañón −8..35°, fogonazo, retroceso, faro, motor que sube de vueltas, cañonazo + explosión con daño radial que las paredes paran). Derrumbe **simulado con física en tiempo real** en vez de pre-simulado en Blender (actores `BLCollapse` que pasan a físicos y se congelan a los 8 s); escombros `BLCollapseBlock` en la escalera solo si el jugador ya está arriba (evita quedarse encerrado). Frase de Varek en persona al derrumbarse (`PlayRadio`). Tejados: C4 (`BLBuilding::CorniceMask` para dejar la azotea abierta), pasarela con barandillas, escalera de incendios separada 105 cm de la fachada (los balcones del kit sobresalen 95) con calles de 160 cm (con 115 el NavMesh no pasaba).
- [x] **Fase 9 — Extracción** (2026-10-09): `gen_heli.py` (fuselaje con cabina abierta y bancos, rotor de 4 palas, rotor de cola, ametralladora de puerta), `Vehicles/BLHelicopter` (espera, llegada por ruta, estacionario con balanceo, alabeo/cabeceo según la aceleración, ráfagas al miliciano más cercano visible por el lado de la puerta, aterrizaje y activación del interactuable). Sonidos sintetizados `gen_vehicle_sfx.py` (motor, cañón, rotor, ametralladora). Agua `M_Env_Water` (`gen_water_texture.py` + `setup_water.py`). Objetivos genéricos: `FBLObjective::ActivateTags` (interfaz `IBLActivatable`) y `bCheckHeight`.
- [x] Nivel ampliado al este hasta x 266 m (límite del este invisible en el borde del muelle) y prueba `Mission` hasta la extracción. Fases nuevas para `jugar_fase.bat`: `Bloque`, `Azotea`, `Muelle` (Varek ya liberado y a tu lado).

### Bloque 12 — Misión 2 "Manifiesto" (2026-10-09) ✅
- [x] Diseño `Docs/Mision2_Manifiesto.md` (aprobado por el usuario: "sí, sigue con la implementación").
- [x] C++ nuevo: `Environment/BLNightSettings` (la oscuridad esconde: consciencia ×0,3 y nada más allá de 26 m; luces "BLLit", linternas y el fogonazo propio delatan), `Environment/BLAlarmLight` (foco de torre que se enciende con parpadeo de arranque + sirena), `Vehicles/BLDrone`, `Vehicles/BLBoat`, `Mission/BLHazardEvent`, `Mission/BLActivatable` (ya del Bloque 11).
- [x] C++ cambiado: `ABLEnemyCharacter` (`Role` = Rifleman/**Gunner**, `bFlashlight` + `USpotLightComponent`), `ABLAIController` (visibilidad nocturna en la consciencia y en la línea de visión; el Gunner no flanquea, no persigue, no busca cobertura, no espera turno, ráfagas de 8–15 y supresión 6 s; descubrir al jugador dispara la alarma si el director lo pide), `ABLMissionDirector` (`TriggerAlarm`, `bAlarmOnDetection`, `AlarmRadio`, `PhaseObjectives` por misión, objetivo **InteractAll** con contador y marcador al más cercano, radio por interactuable, oleadas con `Gunners`/`bFlashlights` por spawn diferido), `ABLInteractable` (`bPhoto`, `UseRadio`), `ABLHUD` (destello y visor de foto, aviso ALARMA), menú (misiones con mapa y puntos de inicio propios; la 02 disponible; el mapa topográfico lee la disponibilidad de `BLMenuData`), `ABLBuilding::CorniceMask`, `ABLSmokeEmitter::FireLightRadius`, `ABLGrenade` (masa sin recalcular en el CDO: rompía el cocinado).
- [x] Blender (`blender -b`): `gen_refinery.py` (depósito, rack, tubería baja, antorcha, torre de destilación, torre de focos, valla con concertina, caja de drones, munición merodeadora) y `gen_m02_vehicles.py` (dron cuadricóptero + hélice, lancha semirrígida). Revisados con `preview_render.py`.
- [x] Audio: `gen_m02_sfx.py` (zumbido y pitido del dron, fueraborda, sirena, ambiente de refinería, antorcha, obturador) + 20 frases `M02_*` en `voice_lines.tsv` (voces TTS generadas).
- [x] Nivel: `bl_levelkit.py` (kit común para misiones 2–5, sin tocar el de la misión 1) y `build_m02_refineria.py`. Revisado a mano: arreglados hueco de la valla, grúa contra el rack, escalera de la pasarela, racks dentro de edificios, tubería en la calle, patrullas a través de muretes, bloqueo invisible que se habría hecho visible, lancha que se podía abordar antes de tiempo (bloqueo de la misión).
- [x] Prueba `Mission2` (`Debug/BLAutoTestMission2.cpp`, 17 pasos): noche, fotos (una con F), alarma + focos, tirador de supresión disparando, defensa, dron derribado, gas, zanja, muelle, lancha, resumen.
- [x] Compilado, importado, mapa generado y prueba pasada (2026-10-09; ver "Pruebas automáticas").

### Bloque 13 — Misión 3 "Ría" (2026-10-09) ✅
- [x] Diseño `Docs/Mision3_Ria.md`.
- [x] C++ nuevo: `Mission/BLDoor` (puerta con bisagra hija de `ABLInteractable`: empujar, patada, brecha con aturdimiento, la IA la abre; la hoja no afecta al NavMesh), `Mission/BLScriptedMover` (actor que recorre un camino o suena al activarse: el barco, las campanas).
- [x] C++ cambiado: `ABLEnemyCharacter` (papeles **Operator** y **Sniper**: vida/blindaje, arma propia por `DuplicateObject`, uniforme/casco/chaleco de Corvane, láser del tirador), `ABLAIController` (`Stun`, granadas de fragmentación del operador, fijación del tirador, Corvane sin barks y sin turno de disparo, mejor puntería; `IsStatic` agrupa ametralladora y tirador), `ABLInteractable` (`Use`/`CanInteract` virtuales y `GetInteractLocation`; el jugador apunta a ese punto), oleadas con `Operators`, menú (misión 3 disponible con 6 puntos de inicio).
- [x] Blender `gen_m03.py`: casco y chaleco de Corvane, puerta + marco + carga de brecha, campanario con campanas, puente de piedra, soportal, barca de pesca, cajas de pescado, mercante de 90 m. Audio `gen_m03_sfx.py`: puertas (abrir, patada), pitido de la carga, campanas (síntesis modal), sirena de barco, gaviotas. 19 frases `M03_*`.
- [x] Nivel `build_m03_ria.py` (con `bl_levelkit.py`: `dawn_fog_lighting`, `door_with_frame`, `mover`) revisado a mano: arreglados bloqueos invisibles que cerraban el puente, hueco bajo la manzana elevada, solapes con el campanario, soportales dentro de un edificio, escalera sin rellano, cuaderno sobre el hueco de la escalera, barco que barría el espigón al girar, baliza tapada por el muro invisible, atajo por la calle mayor (barricada).
- [x] Prueba `Mission3` (17 pasos).
- [x] Compilado, importado, mapa generado y prueba pasada (2026-10-09; ver "Pruebas automáticas").

### Bloque 14 — Misión 4 "Fuego cruzado" (2026-10-09) ✅
- [x] Diseño `Docs/Mision4_FuegoCruzado.md`.
- [x] C++ nuevo: `Weapons/BLMountedGun`, `Player/BLCharacterMount.cpp` (montado: sin moverse, límites de vista del `PlayerCameraManager`, gatillo al arma, F para bajarse, arma y brazos propios ocultos), `AI/BLAllyController`, `Mission/BLMortarBarrage`, `Mission/BLStrikeDesignator`.
- [x] C++ cambiado: papel `Ally` (uniforme del ejército), `ABLBTR::DestroyVehicle`, `ABLScriptedMover::bPhysicsFall` + `BlastPoints`, `FBLObjective::ProgressLabel`, HUD de la ametralladora (retícula, calor, SOBRECALENTADA, "[F] Bajarse"), menú (misión 4 con 6 puntos de inicio).
- [x] Blender `gen_m04.py` (tramo de celosía, pila, afuste y ametralladora, designador, caza, camión, antena de satélite), audio `gen_m04_sfx.py` (silbido de mortero, sobrecalentamiento, pasada de cazas con Doppler), 16 frases `M04_*` (teniente Ilić).
- [x] Nivel `build_m04_puente.py` revisado a mano: arreglados rampas del vado tapadas por el suelo, talud que cerraba el vado, parapeto que cerraba la trinchera, la pasada que caía antes de que llegaran los cazas, el punto de uso del designador.
- [x] Prueba `Mission4` (14 pasos).
- [x] Compilado, importado, mapa generado y prueba pasada (2026-10-09; ver "Pruebas automáticas").

### Bloque 15 — Misión 5 "Línea negra" (2026-10-09) ✅
- [x] Diseño `Docs/Mision5_LineaNegra.md`.
- [x] C++: `Mission/BLDestructibleTarget` (interfaz), objetivo **Destroy**, **condiciones de fallo** en el director (`TimeLimit`/`FailText`, VIP muerto, `FailMission` → pantalla "MISIÓN FALLIDA" y recarga en la fase más cercana con `PhaseObjectives`), cuenta atrás en el texto del objetivo, `bCampaignFinale`, `AI/BLVIP` (`ABLVIP` sobre `ABLVarek`, solo le daña el jugador; `ABLVIPController`: huye, espera, se rinde o escapa), `ABLEnemyCharacter` implementa `IBLActivatable` (reenvía al controlador) y `bFollowPlayer`, aliados que siguen (`ABLAllyController::TickFollow`) y que no disparan a Varek/VIP (tampoco el ametrallador del helicóptero), helicóptero dañable (`bDamageable`, `Health`, `BodyMaterial`, `bDoorGun`, estado `Disabled` con humo y aterrizaje forzoso; `IBLDestructibleTarget`), menú (misión 5 con 5 puntos de inicio).
- [x] Blender `gen_m05.py` (armario de servidores, termita, grúa STS, helipuerto, mesa de oficina), 15 frases `M05_*`, `setup_m05_assets.py`.
- [x] Nivel `build_m05_terminal.py` (con `bl_levelkit.wall_x/wall_y`) revisado a mano: grúas con una pata en el agua, sala de servidores abierta por un lado.
- [x] Pruebas `Mission5` (14 pasos) y `Mission5Fallo` (3 pasos: el fuego ajeno no le mata, la muerte por el jugador falla la misión).
- [x] Compilado, importado, mapa generado y prueba pasada (2026-10-09; ver "Pruebas automáticas").

### Bloque 16 — Pistola P-17 y cambio de arma (2026-10-09) ✅
- [x] Malla propia `gen_p17.py`: corredera con estrías inclinadas, ventana de expulsión con el bloque de recámara visible (e indicador de bala en recámara), tapa trasera, miras (muesca negra + poste con punto de tritio), marco de polímero con raíl, guardamonte de frente recto, puño a 17,7° con punteado y cola de castor, retén de corredera, botón del cargador, palanca de desmontaje, cargador de doble hilera con base y bala. Máscaras horneadas 1024.
- [x] Sonido real CC0 (Walther PPQ + cuerpo de 1911): 8 cercanos, 2 colas, 4 3D, 2 lejanos; recarga, corredera, percutor en vacío; cargador contra el suelo (síntesis).
- [x] C++: poses FP por arma (`FBLWeaponPoses` en `UBLWeaponData`; el rig las recibe al equipar), agarre de las dos manos por arma (`LeftHandGripRotation`, `RightHandOffset/Rotation`), `EBLReloadStyle::Pistol` (`BLCharacterWeapons.cpp`: suelta el cargador, coge otro del cinturón, lo mete con un golpe; en vacío monta la corredera por encima), `UBLWeaponFXComponent::DropMagazine` (cae con la proyección FP y fuera de cámara pasa a objeto físico del mundo que queda en el suelo, 60 s, máx. 6, y suena al llegar), escala del fogonazo por arma.
- [x] Cambio de arma en `UBLWeaponComponent::SwitchToSlot/CycleWeapon`: baja (`HolsterTime`), cambia fuera de la vista y sube (`EquipTime`); cancela la recarga; volver a pedir la actual a medio bajar la sube desde donde está. Input `IA_WeaponPrimary` (1), `IA_WeaponSecondary` (2), `IA_SwapWeapon` (rueda, Y). El jugador lleva siempre AR-7 + P-17; los checkpoints guardan las dos.
- [x] Pruebas `Pistol` (17/17) y `PistolHands` (ajuste visual); regresión Weapons, Combat, Movement y Mission.
- [ ] Prueba del usuario (encuadre de la recarga, sonido, sensación del retroceso).

### Requisitos del prompt original aún sin reflejar en el plan
- [x] **Lista de assets de Blender** (punto 11 del prompt) → `Docs/Assets_Blender.md` (2026-10-08). Aprobada con sus 3 decisiones (ver Registro de decisiones).
- [ ] **"2 o 3 armas muy bien hechas"**: AR-7 y P-17 hechas (Bloque 16); falta la SG-12 "Mastín" al mismo nivel.
- [ ] **Fase 3 (Bloque 9)** — comprobar: compilación, Blueprints, C++, colisiones, IA, navegación, animaciones, armas, cámara, HUD, audio, rendimiento.
- [ ] **Fase 4 — pendientes**: ~~menús, opciones~~ (hechos en el Bloque 10; el armamento es solo de consulta: no hay elección de loadout), guardado de progreso, dificultades, más misiones/zonas/objetivos, enemigo especial (candidato: Operador Corvane), vehículos solo si aportan (BTR, que la misión 1 completa necesita).

## 7. Registro de decisiones
| Fecha | Decisión |
|---|---|
| 2026-10-07 | Diseño v0.1 aprobado sin cambios. |
| 2026-10-09 | P-17: **malla propia** (no la pistola de Epic, que no se puede publicar) y **recarga procedural** como la del AR-7 (las animaciones de recarga de Epic son de tercera persona). Las poses FP y el agarre de las manos pasan a ser **datos del arma**: cada arma nueva se encuadra con números, sin tocar el rig. El cargador vacío **cae al suelo y se queda** (relevo de la partícula FP a un objeto físico fuera de cámara). |
| 2026-10-09 | Verificación de las misiones 2–5 (el usuario lo permite): la voladura del puente pasa de física a **caída guiada** (determinista, sin que el tramo se quede apoyado); muros invisibles con **perfil propio** `BLInvisibleWall` (solo cuerpos); los aliados juzgan la vista **desde la altura de pie** para no perder el blanco tras la cobertura. |
| 2026-10-09 | **No abrir Unreal ni compilar hasta nuevo aviso; trabajar sin parar en todas las misiones** (usuario). Se escribe todo y se verifica en una sola sesión cuando lo permita. (Levantado el mismo día: "sí, abre Unreal y compila todo".) |
| 2026-10-09 | Misión 2: las 3 fotos son **un** objetivo con contador (InteractAll) en vez de tres iguales; el derrumbe/dron/gas/lancha son actores `IBLActivatable` que despiertan los objetivos (sin Blueprints). Noche con niebla volumétrica corta (los haces se ven): coste pendiente de medir en la 1660S. |
| 2026-10-08 | **Varek se rescata en la misión 1** (fase 6 del diseño aprobado); la misión 3 "Ría" pasa a ser la búsqueda del barco sin bandera de Corvane. |
| 2026-10-08 | El usuario pide el **menú principal con música** antes de seguir con el Bloque 9 → Bloque 10 (menú, opciones, pausa, lore). Menús en **Slate en C++** (como el HUD en Canvas: los widgets UMG no se generan por script). Música del menú **original sintetizada** (estilo militar épico; no se puede usar la de CoD). |
| 2026-10-08 | Repositorio **público** en GitHub (yagomateos/blackline) **sin los packs de Epic ni las voces TTS** (decisión del usuario tras avisar de la licencia): se publica una instantánea filtrada con `Tools/publish_public.sh`; el repo de trabajo no tiene remoto. |
| 2026-10-08 | Bloque 8: edificios con **fachadas modulares por instancias** (un actor C++ las genera por semilla) en vez de mallas únicas por edificio; humo/fuego con el sistema de sprites propio (Niagara sigue sin poder generarse por script); **preajuste gráfico por defecto "Alto"** en AA/GI/reflejos (Épico no cabe a 60 fps a 1080p en la GTX 1660 Super). Nubes volumétricas probadas y retiradas (≈1 ms sin mejora visible con este cielo). |
| 2026-10-08 | Bloque 7: **voces provisionales con la síntesis de voz de Windows** (no hay voces CC0 en español para estas frases); **sin MetaSounds** (capas en C++, como StateTree→C++); música solo en combate; el combate lejano lo genera el código (no un bucle) para que llegue desde direcciones distintas. |
| 2026-10-07 | Benchmark: Lumen + VSM + Nanite + TSR 75% por defecto (72 fps en escena de prueba). Re-medir en Bloque 8 con contenido real. |
| 2026-10-07 | Se importan los packs de Epic de UE 5.8 (Characters con animaciones de fusil/pistola, Weapons, Input, LevelPrototyping) como base provisional. |
| 2026-10-07 | Malla FP = Mannequin completo con cabeza/piernas ocultas (no brazos sueltos), adjunto a la cámara; colocación del arma por calibración ADS en vez de offsets a mano. |
| 2026-10-07 | ADS cancela el sprint (prioridad al apuntado). Saltar estando agachado = levantarse. |
| 2026-10-07 | Input: acciones propias creadas por script (`create_input_assets.py`); WASD/salto/mirada reutilizan `IMC_Default`/`IMC_MouseLook` de Epic. |
| 2026-10-08 | Aprobadas las 3 decisiones de `Docs/Assets_Blender.md` (Mannequin provisional para el enemigo, AR-7 por script con revisión visual, grey-box con coberturas finales). |
| 2026-10-08 | **Animación FP sin Anim Blueprint**: `UBLFirstPersonAnimInstance` en C++ (los AnimBP no se pueden generar por script y así es testeable). |
| 2026-10-08 | **El arma manda y las manos la siguen por IK** (cuerpo FP fijo a la cámara). Sustituye a "malla FP colgada del rig con el arma en la mano". |
| 2026-10-08 | **Recarga y equipar procedurales** en primera persona; las animaciones de recarga de Epic (tercera persona) quedan para el cuerpo y la IA. |
| 2026-10-08 | Arma como **componente** (`UBLWeaponComponent`) en vez de actor; la IA usará el mismo componente vía `IBLWeaponOwner`. |
| 2026-10-08 | Disparar corta el sprint; recargar corta ADS y sprint; recarga automática al apretar el gatillo con el cargador vacío. |
| 2026-10-08 | Criterio del usuario: **máxima calidad visual estable en la GTX 1660 Super**; prioridad realismo > cantidad de efectos > contenido nuevo. |
| 2026-10-08 | Sonido e impactos: recursos **CC0 reales** donde existen (disparos, recarga, texturas Poly Haven) y síntesis donde no. |
| 2026-10-08 | Partículas de impacto con sistema propio (ISM) en vez de Niagara: los sistemas Niagara no se pueden crear por script; el propio es testeable y tiene coste fijo. Niagara queda para efectos grandes (explosiones, humo) en el Bloque 8. |
| 2026-10-08 | Recarga táctica con bala en recámara (30+1). |
| 2026-10-08 | Referencias visuales del usuario (CoD4 / CoD moderno): **arma a la derecha, grande y en diagonal hacia la cruz** (FOV propio del arma 50°, pose de cadera calculada: mira trasera a ~20 cm del ojo, giro 5,5° al centro). Fogonazo **realista y compacto**: apagallamas de jaula, núcleo blanco + 4-5 pétalos cortos, 30 ms, luz de boca 7000 (sin chorros laterales: el usuario los rechazó). |
| 2026-10-08 | Iluminación del mapa de pruebas = **amanecer frío nublado** del diseño: sol bajo y débil, cielo cubierto, niebla, gradación desaturada y contrastada, viñeta y grano suaves. |
| 2026-10-08 | AR-7 v2: miras metálicas de anillo (sin óptica, como en el diseño), ojo a 13 cm en ADS, plano de recorte cercano 2 cm. |
| 2026-10-08 | Bloque 3: salud 100 en 4 segmentos (regenera tras 4 s hasta el tope del segmento); daño por zonas en el arma (cabeza ×2,5, extremidades ×0,75); reacción al impacto **física** (sin Anim Blueprint); checkpoint guarda posición + munición; muerte → reaparición a los 4 s. Sin barra de vida: el estado se lee en la pantalla (viñeta/tinte) y el latido. |
| 2026-10-08 | Canal de trazado propio para armas (`Weapon`) para impactos por hueso; las cápsulas lo ignoran. |
| 2026-10-08 | Bloque 6: HUD con Canvas en C++ (no UMG: los widgets no se generan por script); sin barra de vida ni minimapa (diseño). Radio con subtítulos siempre; el indicativo del mando es "TORRE". |
| 2026-10-08 | **IA en C++ en vez de StateTree + EQS** (comunicado al usuario): mismo comportamiento del diseño, generable y probable por script; se puede migrar a StateTree más adelante. Percepción con AI Perception y navegación con NavMesh. |
| 2026-10-08 | IA: máx. 3 enemigos disparando a la vez; sin fuego amigo entre la IA; el ruido de disparo del jugador llega a 40 m (no atrae a toda la misión); los trazados de visión/cobertura usan el canal Visibility (los volúmenes no tapan). |
| 2026-10-08 | Bloque 4: misión 1 en un solo mapa sin World Partition; trazado lineal con rutas alternativas (2 callejones, filas de contenedores, callejones de flanqueo a ambos lados de la calle). Sol del amanecer por el este, de cara al avanzar. `jugar.bat` abre la misión; el mapa de pruebas pasa a `jugar_pruebas.bat`. |
