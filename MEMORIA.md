# BLACKLINE — Memoria del proyecto

> Documento vivo. Se actualiza tras cada cambio importante.
> Última actualización: 2026-10-08 · **Menú principal, pausa, opciones y LORE** hechos (adelantados de la Fase 4 a petición del usuario) · repo público en GitHub · **Bloque 8 (pase de arte)** hecho: fachadas modulares, atrezo, humo/fuego, iluminación, equipo del miliciano y preajuste gráfico medido a 1080p · **Bloque 7 (audio base)** hecho: voces de radio y de la IA (provisionales), música de combate, ambiente por zona, acústica, balas que pasan cerca, pasos de la IA · Bloque 6: el vertical slice se juega de principio a fin.
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
| 9 — Test + optimización (Fase 3) | 🔶 En curso: build empaquetada correcta; arreglados 4 fallos (compilación de juego, usos de materiales, CommonUI, crash de animación de la IA en la build) · falta repetir las pruebas en la build |
| 10 — Menú, opciones, pausa y lore (adelantado de la Fase 4) | ✅ Hecho (2026-10-08) · Menu 10/10 (incl. despliegue y pausa) · regresión OK · pendiente prueba del usuario |

---

## 2. Diseño aprobado

### Concepto
- **Nombre provisional:** BLACKLINE.
- **Ambientación:** 2031, Kessra, ciudad portuaria de la República de Varania (ficticia). Ciudad partida por la "línea negra": al este controla la **Columna Vesk** (milicia paramilitar) entrenada y armada por la contratista **Corvane Security**.
- **Protagonista:** Sargento **Adrián Roca**, indicativo **"Sable 2-1"**, del Grupo Operativo BLACKLINE.
- **Arco:** Corvane usa la guerra como banco de pruebas de armamento; BLACKLINE reúne pruebas misión a misión.
- **Tono:** realista, sobrio, operación militar moderna. Sin sci-fi.

### Misión 1 — "AMANECER ROTO" (~15–20 min, amanecer)
1. Inserción (furgón civil, briefing por radio).
2. Infiltración (patrullas, sigilo opcional).
3. Contacto (control de carretera, alarma).
4. Combate en calle (cobertura, flanqueos).
5. Asalto al bloque de viviendas (CQB piso a piso).
6. Objetivo: rescatar a **Tomas Varek** (informante) y recuperar su disco duro.
7. Contraataque (defender posición, oleadas, humo).
8. Evento: un **BTR** dispara contra el edificio → derrumbe parcial → huida por tejados.
9. Extracción en el muelle con cobertura de helicóptero.

**Vertical slice = fases 1–4 + objetivo simple** (llegar a punto + recuperar objeto).

### Arsenal
| Arma | Rol | Prioridad |
|---|---|---|
| AR-7 "Halcón" (fusil 5.56, ~750 RPM) | Principal, arma estrella | Slice |
| P-17 (pistola) | Secundaria | Fase 4 |
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

### C++ implementado (Bloques 1 y 2)
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
WASD mover · Ratón mirar · Shift sprint (mantener) · C / Ctrl agacharse (alternar) · Espacio saltar / encaramarse · Q/E inclinarse (mantener) · Clic dcho. apuntar · Clic izq. disparar · R recargar · F interactuar* · G granada* · 1/2 cambiar arma* (*mapeado, sin implementar aún). Mando: mapeado básico.

### Jugar
Doble clic en `Tools/jugar.bat` (juego en ventana, sin editor, **menú principal** `L_MainMenu`, que es también el mapa por defecto) o `Tools/jugar_pruebas.bat` (mapa de pruebas `L_Dev_Movement`). O abrir `Blackline.uproject` y pulsar Play en el editor. Empezar en una fase concreta: `Tools\jugar_fase.bat Fase3` (Fase1 inserción, Fase2 patio, Fase3 control, Fase4 calle, Objetivo local; arg. `-BLStart=<Fase>`). Build empaquetada (Bloque 9): `..\BlacklineBuild\Windows\Blackline.exe`.

### Pruebas automáticas
`bash Tools/run_test.sh [Prueba] [Mapa] [--nobuild]` compila, lanza el juego con `-BLTest=<Prueba>` y muestra resultados. Resultados y capturas (desde el render) en `Saved/BLTest/`.
- **Movement** (mapa `L_Dev_Movement`): 17 pasos — asentar, andar, sprint, frenar, agacharse, andar agachado, levantarse, ADS, soltar ADS, lean izq/der, salto, mantle 100/150, muro 250 (sin mantle), escaleras, lean contra pared.
- **Mission** (mapa de la misión): 9 pasos — briefing por radio, los 5 objetivos en orden (callejón, puerta del patio, control eliminado, local, disco recogido manteniendo F), marcador del objetivo, pantalla de misión completada con resumen. `bash Tools/run_test.sh Mission /Game/Maps/M01/L_M01_AmanecerRoto`
- **Menu** (`bash Tools/run_test.sh Menu /Game/Maps/Menu/L_MainMenu`): recorre las páginas con capturas, cambia opciones y comprueba que se guardan en el .ini (y las restaura), despliega la misión 1 desde el patio (jugador en su sitio y objetivo 2) y abre la pausa (juego en pausa).
- **Views** (mapa de la misión, ~40 s): 9 vistas fijas (inserción, callejón, patio, control, calle, fachadas, local, retrato de un miliciano, coche ardiendo) con fps medios y capturas; enemigos retirados salvo el del retrato. A 1080p: `BL_RESX=1920 BL_RESY=1080 bash Tools/run_test.sh Views /Game/Maps/M01/L_M01_AmanecerRoto --nobuild`. Perfil de GPU de una vista: `BL_ARGS="-BLProfileView=Calle"` y luego `python -I Tools/benchmark/gpu_profile_summary.py Saved/BLTest/Views_game.log 0.3 6`.
- **Audio** (mapa de la misión, con `-NoSound`: comprueba la lógica y que los sonidos existen): 12 pasos — recursos (66 barks, 11/11 frases de radio con voz), voz del briefing + atenuación de la mezcla, emisores de ambiente (16) y zonas acústicas (8), reverb activa en el local / callejón / calle, pasos de la patrulla, contacto (bark + golpe + música), balas que pasan cerca, "¡Hombre abajo!", la música se va al acabar el combate, combate lejano. `bash Tools/run_test.sh Audio /Game/Maps/M01/L_M01_AmanecerRoto --nobuild`
- **AI** (mapa de la misión): 12 pasos — 12 enemigos con controlador en patrulla, las 4 patrullas recorren su ruta, disparos oídos a 30–40 m (investigan), no detecta por la espalda agachado, detecta de frente y alerta a su escuadra, coberturas (en cobertura y protegidos del jugador), disparan con como mucho 3 turnos, recargan, flanquean, persiguen al perderlo, captura de cerca, bajas (ragdoll, salen de la escuadra). `bash Tools/run_test.sh AI /Game/Maps/M01/L_M01_AmanecerRoto`
- **Level** (mapa `/Game/Maps/M01/L_M01_AmanecerRoto`): 8 pasos (retira a los enemigos) — ruta de navegación furgón → objetivo, **recorrido a pie** (sprint, siguiendo la ruta: 245 m sin atascarse, cruzando los 4 checkpoints en orden) y vistas de cada fase con fps medios (mínimo 45). `bash Tools/run_test.sh Level /Game/Maps/M01/L_M01_AmanecerRoto`
- **Combat** (mismo mapa, estaciones "Dianas" y "Checkpoint"): 12 pasos — maniquíes presentes, disparo al torso (daño 28, reacción física, hitmarker, superficie Carne), cabeza (×2,5), pierna (×0,75), salpicaduras de sangre en la pared, baja (ragdoll + hitmarker de baja), reaparición del maniquí, checkpoint, daño al jugador (salud, indicador), regeneración hasta el tope del segmento (75), muerte (cámara al suelo) y reaparición en el checkpoint con la munición guardada.
- **Weapons** (mismo mapa, estación "Tiro": muro a 10 m + cajas con física): 16 pasos — equipar, mano izquierda en el agarre (< 3 cm), ráfaga 1 s (cadencia, impactos, munición, retroceso), recuperación de la mira, recarga, sprint cortado al disparar, vaciar cargador + recarga automática + cerrojo abierto (hueso desplazado), recarga en vacío + cerrojo cerrado, dispersión en ADS, ráfaga en ADS, captura de impactos. Capturas de la recarga a 0,4 / 1,0 / 1,4 s.
- Las estaciones del mapa se localizan por TargetPoints con tag `BLTest_Start_<Nombre>`.

### Estructura C++ prevista
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
| Objetivo | Local al final de la calle (x 195–210 m) | Planta baja abierta con mostrador y mesa; `TargetPoint` "Objetivo_Disco". Checkpoint "Objetivo" |

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
| SM_Cover_Sandbag_Straight / _Corner, SM_Cover_Jersey, SM_Cover_TWall | `gen_level_props.py` (+ `bl_kit.py`) | 8,3k / 15k / 0,8k / 0,4k | Coberturas con medidas finales (sacos 115 cm, jersey 107, T-wall 370). Nanite, colisión UCX |
| SM_Container_20ft (+ _Open), SM_Veh_Van_Civil (+ _Open), SM_Veh_Sedan_Wreck | `gen_level_props.py` | 6,4k / 6–7k / 3k | Contenedor corrugado con puertas y barras; furgón (inserción, puertas traseras abiertas); coche calcinado sin cristales. Materiales triplanares (`setup_level_assets.py`) |

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
- **Misión**: Sin menú principal ni pausa (Fase 4); al completar, F reinicia el nivel. Los enemigos muertos siguen muertos tras reaparecer en un checkpoint (no se restaura el mundo).
- **IA**: sin animaciones de agacharse ni de muerte reales (agacharse procedural, muerte = ragdoll); sin gemido al morir (solo golpe del cuerpo contra el suelo); pasos por distancia recorrida, no sincronizados con los pies de la animación; sin granadas (Operador Corvane, Fase 4); la IA no salta obstáculos (NavLinks pendientes). Primer impacto/ragdoll de la partida: pico de ~400 ms en la build de editor.
- Dificultad provisional: el jugador recibe el 55 % del daño (`DamageTakenMultiplier`); menú de dificultad en la Fase 4.
- **Arte (Bloque 8) pendiente**: la nave del puerto, los muros del patio y los límites del nivel siguen siendo cajas con textura; los edificios no tienen interiores (ventanas opacas con "interior" falso); sin cristales que se rompan, sin decals de pintadas/carteles/charcos; el cuerpo del miliciano y los brazos del jugador siguen siendo el Mannequin (con equipo encima). El fuego no daña al jugador.
- **Rendimiento a 1080p** (build de editor): 62–77 fps sin enemigos en el preajuste Alto; con combate y 12 enemigos hay que medirlo en el Bloque 9 (y en build empaquetada, que suele ir algo mejor).
- La valla del callejón B (110 cm) no está en el NavMesh: la IA necesitará NavLinks para saltar obstáculos.
- **Dianas = Mannequin blanco de Epic** (provisional hasta el miliciano del Bloque 5). Sin animaciones de reacción, solo física. Sangre sin marca sobre el cuerpo (los decals no se proyectan en personajes).
- Primer ragdoll/salpicadura de la partida: pico de ~400 ms (compilación de shaders / inicialización de físicas en la build de editor); vigilar en la build empaquetada (Bloque 9).
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

### Bloque 9 — Test + optimización (siguiente)
- [ ] Fase 3 del prompt: compilación, C++, colisiones, IA, navegación, animaciones, armas, cámara, HUD, audio, rendimiento (con combate, a 1080p y en build empaquetada).

### Requisitos del prompt original aún sin reflejar en el plan
- [x] **Lista de assets de Blender** (punto 11 del prompt) → `Docs/Assets_Blender.md` (2026-10-08). **Pendiente de aprobación**, con 3 decisiones abiertas: cuerpo del miliciano, AR-7 por script con revisión visual y grey-box con coberturas finales.
- [ ] **"2 o 3 armas muy bien hechas"**: el slice lleva solo el AR-7; las siguientes (P-17, SG-12) son las primeras de la Fase 4 y deben llegar al mismo nivel de acabado antes de añadir más.
- [ ] **Fase 3 (Bloque 9)** — comprobar: compilación, Blueprints, C++, colisiones, IA, navegación, animaciones, armas, cámara, HUD, audio, rendimiento.
- [ ] **Fase 4 — pendientes**: menús completos, loadout, opciones (gráficos/audio/controles), guardado, dificultades, más misiones/zonas/objetivos, enemigo especial (candidato: Operador Corvane), vehículos solo si aportan (BTR).

## 7. Registro de decisiones
| Fecha | Decisión |
|---|---|
| 2026-10-07 | Diseño v0.1 aprobado sin cambios. |
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
