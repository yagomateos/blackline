# Lista de assets de Blender — vertical slice

> Propuesta 2026-10-08 · **Aprobada 2026-10-08** (las 3 decisiones de la sección 4). Responde al punto 11 de `Docs/PROMPT_ORIGINAL.md`.
> Alcance: solo lo necesario para el vertical slice (fases 1–4 de "Amanecer Roto" + objetivo simple).
> Los assets de la Fase 4 aparecen al final solo como referencia; no se crean hasta que el slice funcione.

---

## 1. Reglas comunes

### Escala y métricas de juego
Las medidas salen del personaje ya implementado (cápsula radio 34 cm, altura 184 cm, ojos 164 cm de pie / 104 cm agachado, escalón 42 cm, mantle 45–165 cm, salto ~78 cm).

| Métrica | Valor | Por qué |
|---|---|---|
| Cobertura baja | **115–120 cm** | Tapa al personaje agachado; de pie se dispara por encima. Se puede saltar (mantle) |
| Cobertura parcial | 80–110 cm | Tapa el cuerpo, no la cabeza agachada. Para ritmo, no para descansar |
| Cobertura alta | **≥ 190 cm** | Tapa de pie; no se puede saltar. Se dispara asomándose (lean Q/E) |
| Obstáculo saltable | 45–165 cm | Rango del mantle |
| Escalón máximo | 40 cm (escaleras: 18 cm de contrahuella) | `MaxStepHeight` = 42 |
| Puerta | 100 × 220 cm (interior 90 × 210) | Cápsula 68 cm + margen para la IA |
| Pasillo mínimo | 140 cm | Dos personajes se cruzan |
| Altura libre interior | ≥ 260 cm | Sin choques de cámara al saltar |

Estas medidas valen igual para el jugador y para la IA (mismo esqueleto Mannequin).

### Exportación (ya implementada en `bl_lib.py`)
Metros en Blender → FBX con `FBX_SCALE_UNITS`, forward −Y, up Z, smoothing FACE, sin leaf bones. Pivote en la base, centrado, salvo que se indique otro. Colisión con mallas `UCX_<nombre>_NN` (cajas/convexos simples). Nunca "complex as simple" en props.

### Nanite y LOD
| Tipo de malla | Nanite | LOD |
|---|---|---|
| Entorno estático opaco (edificios, muros, props grandes) | **Sí** | No hace falta (Nanite lo gestiona) |
| Cristal / translúcido | No (Nanite no soporta translucidez) | 1 LOD, geometría mínima |
| Arma en primera persona | No (skeletal) | LOD0 solo (siempre está cerca) |
| Arma en manos de enemigos / en el suelo | No | LOD0 / LOD1 50% / LOD2 15% |
| Props pequeños dinámicos (cargador, casquillo) | No | 1–2 LOD; casquillo sin LOD |
| Personajes | No | 3–4 LOD |

### Materiales y texturas
- **Entorno:** materiales de repetición (tiling) CC0 de Poly Haven / ambientCG + máscaras de suciedad por color de vértice pintadas en el script. Se evitan texturas únicas por objeto → poco VRAM.
- **Armas:** materiales PBR de repetición (metal anodizado, polímero, goma) + máscara de **AO y curvatura horneada en Blender** (2K) que el material de UE usa para el desgaste de bordes. Así un script genera un arma con aspecto detallado sin pintar texturas a mano.
- **Masters en UE:** `M_Master_Env` (tiling + vertex paint + decals), `M_Master_Weapon` (desgaste por curvatura), `M_Master_Glass`, `M_Master_Decal`.
- **Tamaños:** armas 2K · props 1K · props pequeños 512 · tiling de entorno 2K.
- **Coste estimado de VRAM:** un juego de texturas 2K (BaseColor + Normal + ORM, comprimidas, con mips) ≈ 11 MB. Slice ≈ 15 juegos de entorno + arma + props ≈ **200 MB**, muy dentro del margen de ~2 GB que dejó el benchmark.

---

## 2. Assets del vertical slice

Origen: **Script** = generado por script de Blender (`ArtSource/Blender/scripts/gen_*.py`) · **Externo** = Epic / CC0 / Fab, sin crear en Blender.

### A. Arma — AR-7 "Halcón" (Bloque 2)

| Asset | Origen | Geometría / tris | Escala | Materiales / texturas | Rigging / animación |
|---|---|---|---|---|---|
| `SK_AR7` | Script | Hard-surface con bevels; LOD0 ~35k tris (FP); LOD1 ~12k, LOD2 ~4k para enemigos/suelo | Largo 84 cm con culata extendida, cañón 37 cm (tipo 5.56 compacto) | 3 slots: cuerpo (anodizado), polímero, goma. AO + curvatura 2K horneados | Huesos: `root`, `bolt_carrier`, `charging_handle`, `trigger`, `magazine`, `selector`. Sockets: `Sight`, `Muzzle`, `Eject`, `Mag`, `HandGrip_L` (para IK de la mano izquierda) |
| `SM_AR7_Mag` | Script | ~2,5k tris | Cargador 30 balas, ~19 cm | Comparte texturas del arma | Estática; se suelta en la recarga (física) y se engancha a la mano izquierda con notifies |
| `SM_Casing_556` | Script | ~60 tris, sin LOD | 5,7 cm × 0,96 cm | Latón (material simple, sin texturas) | Niagara (mesh renderer), instanciado |
| `SM_AR7_MuzzleFlash` | Script | 3 planos cruzados, ~12 tris | ~20 cm | Textura de flash 512 (CC0 o generada) | Lo usa Niagara |

**Animaciones del arma:** no se animan a mano en Blender. Los brazos usan las animaciones de fusil de Epic (`MM_Rifle_Fire`, `MM_Rifle_Reload`, `MM_Rifle_Equip`…). El ciclo del cerrojo, el gatillo y el cargador se mueven en el Anim Blueprint (curvas y notifies). Así una sola malla vale para cualquier animación de brazos que añadamos después.

**Bloque 4 (2026-10-08):** coberturas (sacos recto y esquina, jersey, T-wall), contenedor de 20 pies (cerrado y abierto), furgón civil (cerrado y con puertas traseras abiertas) y coche calcinado creados por `gen_level_props.py` con medidas finales, colisión UCX y Nanite; usados en `L_M01_AmanecerRoto`. Pendientes del kit: caseta, bidones, palés, contenedores de basura y vallas (ahora cajas).

**Estado (2026-10-08, v2):** `SK_AR7` v2: 23.416 tris con piezas reales (ver MEMORIA.md), UV únicas, máscaras AO/aristas/cavidades horneadas a 2K y material maestro con desgaste; mira de anillo a 13 cm del ojo en ADS. Pendiente: LOD1/LOD2 para la IA.

**Estado anterior (v1):** `SK_AR7` v1 creado (`gen_ar7.py`): 8.804 tris, 11 huesos, 4 materiales planos (metal, polímero coyote, polímero oscuro, goma). Agarre izquierdo a 25 cm del pistolete y 6,5 cm bajo el eje; mira de apertura a 9 cm del ojo en ADS. Pendiente para el Bloque 8: subir detalle hacia los ~35k tris previstos, UV y horneado de AO/curvatura, desgaste de bordes.

**Riesgo:** un fusil "muy detallado" generado solo por script puede quedarse corto respecto a un shooter comercial. Plan: lo genero por script, te enseño capturas renderizadas y, si no llega al nivel, lo refino a mano en Blender o valoramos un modelo de Fab con licencia válida.

### B. Personaje enemigo — Miliciano Vesk (Bloque 5)

| Asset | Origen | Notas |
|---|---|---|
| Cuerpo | **Externo** (provisional: Mannequin de Epic) | Un humano realista no es viable por script. Ver decisión pendiente abajo |
| `SK_Vesk_Gear_*` (chaleco con bolsillos, casco, brazalete negro de la Columna, pasamontañas) | Script | ~8–12k tris en total, 1K. Piezas sueltas del mismo esqueleto (*skinned* al Mannequin) para variar enemigos sin tocar el cuerpo |
| Arma enemiga | Reutiliza `SK_AR7` LOD1 | Sin trabajo extra |

### C. Entorno del slice — kit modular (Bloques 4 y 8)

Fases del slice: 1 inserción (furgón), 2 infiltración (callejones y zona de contenedores del puerto), 3 control de carretera, 4 combate en calle.

| Asset | Origen | Tris aprox. | Medidas | Materiales | Rol de juego |
|---|---|---|---|---|---|
| `SM_Bldg_Facade_*` (3 variantes: viviendas, bajo comercial con persiana, ciego) | Script (evolución de `gen_benchmark_building.py`) | 20–60k, Nanite | Módulos de 4 × 3,2 m por planta | Enfoscado, hormigón, metal, cristal | Calles y cobertura alta |
| `SM_Bldg_Corner`, `SM_Bldg_Roof_Parapet` | Script | 5–15k, Nanite | Ídem | Ídem | Cierre del kit |
| `SM_Glass_Window_*` | Script | <100 | Según hueco | `M_Master_Glass` | Separado del edificio (no Nanite) |
| `SM_Street_Road`, `SM_Street_Curb`, `SM_Street_Sidewalk` | Script | Bajos, Nanite | Bordillo 15 cm | Asfalto, hormigón, vertex paint de suciedad | Suelo |
| `SM_Cover_Sandbag_*` (recto, esquina, final) | Script | 4–8k, Nanite | **115 cm** de alto, módulo de 120 cm | Tela de saco 1K | Cobertura baja (control de carretera) |
| `SM_Cover_Jersey` | Script | ~1k, Nanite | 107 × 60 × 300 cm | Hormigón + pintura | Cobertura parcial |
| `SM_Cover_TWall` | Script | ~1k, Nanite | 370 × 120 × 150 cm | Hormigón | Cobertura alta |
| `SM_Container_20ft` (+ variante con puertas abiertas) | Script | ~6k, Nanite | 6,06 × 2,44 × 2,59 m | Chapa pintada (tinte por instancia), óxido | Infiltración: cobertura alta y rutas |
| `SM_Veh_Van_Civil` (+ puertas traseras abiertas) | Script | ~25k, Nanite | ~5,3 × 2,0 × 2,3 m | Pintura, cristal, goma, metal | Inserción; cobertura después |
| `SM_Veh_Sedan_Wreck` (quemado) | Script | ~15k, Nanite | ~4,5 × 1,8 × 1,4 m | Metal quemado, óxido | Cobertura parcial en la calle |
| `SM_Checkpoint_Booth`, `SM_Checkpoint_BarrierArm` | Script | 3–5k | Caseta 2 × 2 × 2,6 m | Chapa, madera | Control de carretera |
| `SM_Drum_Oil`, `SM_Tire_Pile`, `SM_Pallet`, `SM_Crate_Wood` | Script | 0,5–3k | Medidas reales | Tiling 1K/512 | Relleno y cobertura baja |
| `SM_Pole_Power`, `SM_Streetlight_Sodium`, cables | Script | 2–4k | Poste 8 m, farola 7 m | Hormigón, metal | Lectura de la calle; la farola de sodio es el acento naranja |
| `SM_Debris_Pile_*`, `SM_Rubble_*` (3 variantes) | Script (aleatorio por semilla) | 5–20k, Nanite | 0,5–2 m | Hormigón, ladrillo | Detalle y cobertura baja |
| `SM_Fence_Chainlink`, `SM_Wall_Garden` | Script | 1–3k | Valla 2,2 m; muro 1,15 m | Metal (máscara), ladrillo | Cerrar zonas y cobertura baja |
| `SM_Dumpster` | Script | ~3k | 1,8 × 1,1 × 1,2 m | Chapa pintada | Cobertura baja |

### D. Objetos interactivos (Bloque 6)

| Asset | Origen | Tris | Medidas | Notas |
|---|---|---|---|---|
| `SM_Obj_HardDrive` | Script | ~1k | 15 × 10 × 2 cm | Objetivo del slice (el disco de Varek). LED emisivo para encontrarlo; 512 |
| `SM_Obj_Laptop_Rugged` | Script | ~3k | 35 × 26 cm | Donde está el disco; 1K |
| `SM_Door_Metal` (+ marco) | Script | ~2k | 100 × 220 cm | Puerta que se abre con F. Pivote en la bisagra |
| `SM_AmmoCrate` | Script | ~1,5k | 60 × 30 × 25 cm | Reponer munición (si las pruebas lo piden) |

### E. Lo que NO se hace en Blender
- **Brazos y cuerpo del jugador:** Mannequin de Epic (ya integrado).
- **Animaciones de personaje:** Epic + Mixamo, retargeting con IK Retargeter de UE.
- **Texturas de materiales base:** CC0 (Poly Haven, ambientCG).
- **Partículas, humo y polvo:** Niagara con texturas CC0.
- **Grey-box del nivel (Bloque 4):** primitivas de LevelPrototyping de Epic, salvo las coberturas y el furgón, que se generan ya con sus medidas reales porque definen el combate.

**Total para el slice:** ~35 mallas, todas por script salvo el cuerpo del enemigo.

---

## 3. Orden de creación

| Bloque | Assets |
|---|---|
| 2 — Armas | `SK_AR7`, `SM_AR7_Mag`, `SM_Casing_556`, `SM_AR7_MuzzleFlash` |
| 4 — Nivel gris | Coberturas (sacos, jersey, T-wall), contenedor, furgón, coche quemado (geometría final, material gris) |
| 5 — IA | Equipo del miliciano |
| 6 — Misión | Disco duro, portátil, puerta, caja de munición |
| 8 — Arte | Kit de edificios, calle, props de relleno, escombros, postes y farolas, cristales + materiales y texturas definitivos |

Cada asset se valida antes de pasar al siguiente: render de previsualización (`preview_render.py`), recuento de tris, importación en UE y prueba en el mapa (colisión, escala frente al personaje, coste en fps).

---

## 4. Decisiones pendientes de aprobación

1. **Cuerpo del Miliciano Vesk.** Recomendación: Mannequin provisional en el Bloque 5 y, en el Bloque 8, un personaje humano de Fab o Mixamo retargeteado al esqueleto de Epic, con el equipo generado en Blender encima. Descarto MetaHuman: demasiado pesado en VRAM para 8–12 enemigos en una GTX 1660 Super.
2. **AR-7 por script con revisión visual** (y refinado manual o Fab si no llega al nivel).
3. **Grey-box con primitivas + coberturas finales desde el principio.**

---

## 5. Referencia — Fase 4 (no se crea todavía)
P-17, SG-12 "Mastín", granada M-6, SMG-9 "Vesper" (mismo sistema de huesos y sockets que el AR-7) · BTR (vehículo guionizado) · equipo de Operador Corvane y LMG · interiores para CQB · tramo de derrumbe pre-simulado · muelle y helicóptero de extracción.
