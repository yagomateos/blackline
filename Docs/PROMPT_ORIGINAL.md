<!-- Prompt original del usuario que define el proyecto (copia literal, reenviado 2026-10-08). No editar: es la referencia de requisitos. -->

Quiero crear contigo un videojuego FPS táctico para PC utilizando Unreal Engine 5 y Blender.

La referencia de diseño es un shooter militar moderno de alto nivel, con una experiencia similar en sensaciones a Call of Duty, pero con identidad propia. No quiero copiar personajes, mapas, nombres, historia, assets ni contenido protegido de otros juegos.

Quiero que el resultado tenga sensación de producción AAA, pero empezando con una escala realista para poder desarrollarlo en mi PC.

Mi hardware:

- NVIDIA GeForce GTX 1660 Super 6 GB VRAM
- 16 GB RAM

El juego debe estar diseñado teniendo en cuenta estas limitaciones.

---

# FASE 1 — DISEÑO DEL JUEGO

Antes de construir nada, desarrolla el concepto completo del FPS.

Quiero que diseñes:

### Gameplay

- FPS.
- Movimiento rápido pero creíble.
- Sprint.
- Agacharse.
- Saltar cuando tenga sentido.
- Apuntar con ADS.
- Disparar.
- Recargar.
- Cambiar de arma.
- Granada.
- Interacción con objetos.
- Sistema de cobertura cuando sea necesario.
- Daño.
- Vida.
- Muerte.
- Checkpoints.

### Armas

Diseña inicialmente un arsenal pequeño pero convincente:

- Fusil de asalto.
- Subfusil.
- Escopeta.
- Pistola.
- Granada.

Cada arma debe tener:

- Animaciones.
- Retroceso.
- Sonido.
- Muzzle flash.
- Casquillos.
- Impactos.
- Recarga.
- ADS.
- Feedback visual y sonoro.

No quiero crear 30 armas inicialmente.

Primero quiero que 2 o 3 armas estén muy bien hechas.

---

# ENEMIGOS

Diseña una IA militar convincente.

Los enemigos deben poder:

- Patrullar.
- Detectar al jugador.
- Investigar sonidos.
- Buscar cobertura.
- Disparar.
- Recargar.
- Cambiar de posición.
- Flanquear cuando sea apropiado.
- Perseguir al jugador.
- Morir mediante animaciones convincentes.

Utiliza la tecnología de IA de Unreal que resulte adecuada.

No hagas una IA artificialmente compleja si no aporta una mejora real al gameplay.

---

# PRIMERA MISIÓN

Diseña una primera misión pequeña pero espectacular.

Quiero una misión de combate urbano táctico.

Ejemplo de estructura conceptual:

1. Entrada en la zona.
2. Exploración inicial.
3. Primer contacto enemigo.
4. Combate.
5. Avance por edificios/calles.
6. Objetivo principal.
7. Aumento de la presión enemiga.
8. Evento importante.
9. Extracción.

Puedes modificar esta estructura si encuentras una opción mejor.

La misión debe sentirse como una operación militar moderna.

---

# CÁMARA

Quiero una cámara FPS inmersiva.

Debe incluir:

- Movimiento corporal.
- Head bob controlado.
- Inclinación ligera.
- Animación de arma.
- Reacción al disparo.
- Reacción al recibir daño.
- Animaciones de sprint.
- ADS.
- Transiciones suaves.

Evita efectos exagerados que dificulten jugar.

---

# DIRECCIÓN VISUAL

Quiero un aspecto realista y cinematográfico.

Referencia visual:

- Shooter militar moderno.
- Iluminación cinematográfica.
- Materiales realistas.
- Entornos urbanos detallados.
- Armas muy detalladas.
- Partículas.
- Humo.
- Polvo.
- Impactos de bala.
- Cristales.
- Ragdolls.
- Destrucción limitada pero convincente.

Debe parecer un juego moderno, pero debe seguir siendo viable en una GTX 1660 Super.

---

# UNREAL ENGINE 5

Utiliza las características de UE5 cuando realmente aporten valor:

- Nanite.
- Lumen.
- Niagara.
- Enhanced Input.
- Behavior Trees / StateTree.
- Control Rig.
- MetaSounds.
- World Partition si el tamaño del mapa lo justifica.

No utilices una tecnología únicamente porque esté disponible.

Prioriza rendimiento.

---

# BLENDER

Utiliza Blender para crear los modelos 3D necesarios.

Empieza por los assets realmente necesarios para el primer prototipo:

- Armas.
- Props.
- Elementos urbanos.
- Objetos interactivos.
- Elementos del escenario.

Define para cada asset:

- Geometría.
- Escala.
- Materiales.
- Texturas.
- Nivel de detalle.
- LOD cuando sea necesario.
- Rigging.
- Animaciones.
- Exportación a Unreal.

Si podemos generar determinados modelos mediante scripts de Blender, hazlo.

No produzcas cientos de assets antes de saber cuáles necesitamos.

---

# MENÚ PRINCIPAL

Quiero un menú principal con apariencia de shooter militar moderno.

Debe incluir:

- Jugar.
- Selección de misión.
- Armamento/loadout.
- Opciones.
- Gráficos.
- Audio.
- Controles.
- Salir.

La interfaz debe tener identidad propia.

No copies literalmente la interfaz de Call of Duty.

---

# HUD

El HUD debe mostrar solamente información útil:

- Munición.
- Granada.
- Objetivo.
- Indicadores necesarios.
- Daño.
- Interacciones.

Evita llenar la pantalla de elementos innecesarios.

---

# AUDIO

Diseña un sistema de audio inmersivo:

- Disparos.
- Recargas.
- Pasos.
- Impactos.
- Explosiones.
- Ambiente urbano.
- Voces.
- Comunicaciones.
- Música cuando sea apropiada.

El sonido debe ser una parte importante del feedback del combate.

---

# FASE 2 — PROTOTIPO

Después de diseñar el juego, no construyas todavía la campaña completa.

Crea primero un vertical slice jugable.

Debe contener:

- FPS funcional.
- Una zona urbana pequeña.
- Un arma.
- ADS.
- Disparo.
- Recarga.
- Un tipo de enemigo.
- IA funcional.
- Daño.
- Muerte.
- Una misión sencilla.
- HUD básico.
- Sonidos básicos.

El objetivo es poder jugar una pequeña sección y comprobar si el juego funciona.

---

# FASE 3 — TEST

Ejecuta el proyecto y comprueba:

- Compilación.
- Blueprints.
- C++ si existe.
- Colisiones.
- IA.
- Navegación.
- Animaciones.
- Armas.
- Cámara.
- HUD.
- Audio.
- Rendimiento.

Corrige primero los errores importantes.

---

# FASE 4 — PRODUCCIÓN

Cuando el vertical slice funcione correctamente, amplía progresivamente:

- Más armas.
- Más enemigos.
- Nuevos comportamientos de IA.
- Nuevas zonas.
- Nuevos objetivos.
- Vehículos si aportan valor.
- Boss/enemigo especial si encaja.
- Más misiones.
- Menús completos.
- Loadout.
- Opciones.
- Guardado.
- Dificultades.

No añadas una característica simplemente porque sea posible.

Cada nueva característica debe mejorar la experiencia.

---

# OPTIMIZACIÓN

Mi GPU es una GTX 1660 Super de 6 GB.

Por tanto:

- Controla el tamaño de las texturas.
- Evita saturar VRAM.
- Utiliza LODs.
- Controla polígonos.
- Controla partículas.
- Evita demasiadas luces dinámicas.
- Controla sombras.
- Controla post-processing.
- Controla número de enemigos simultáneos.
- Comprueba FPS durante el desarrollo.

Quiero priorizar una experiencia estable antes que gráficos extremos.

---

# MEMORIA DEL PROYECTO

Crea en la raíz:

MEMORIA.md

Debe registrar:

- Diseño aprobado.
- Mecánicas.
- Armas.
- Enemigos.
- IA.
- Misiones.
- Arquitectura.
- Assets.
- Decisiones técnicas.
- Problemas conocidos.
- Tareas pendientes.
- Estado actual.

Actualízalo después de cambios importantes.

---

# FORMA DE TRABAJAR

No quiero que intentes construir todo el juego de una vez.

Trabaja por bloques.

Para cada bloque:

1. Planifica.
2. Implementa.
3. Ejecuta.
4. Comprueba.
5. Corrige.
6. Documenta.
7. Continúa.

Si encuentras un problema técnico, intenta resolverlo antes de seguir acumulando código sobre él.

Si una decisión puede perjudicar el juego o el rendimiento, dímelo.

No cambies decisiones importantes de diseño sin comunicármelo.

---

# OBJETIVO FINAL

El objetivo no es crear un clon de Call of Duty.

Quiero crear un FPS militar moderno con:

- Combate satisfactorio.
- Armas convincentes.
- IA creíble.
- Entornos realistas.
- Animaciones buenas.
- Sonido inmersivo.
- VFX potentes.
- Cámara FPS cinematográfica.
- Una primera misión memorable.
- Buena optimización.

Primero quiero conseguir una pequeña misión que parezca una sección de un shooter comercial.

Después ampliaremos el juego.

---

# EMPIEZA AHORA

NO empieces todavía a construir todo el juego.

Primero analiza este diseño y propón una versión concreta de:

1. Nombre provisional.
2. Ambientación.
3. Historia básica.
4. Primera misión.
5. Arsenal inicial.
6. Enemigos.
7. IA.
8. Mecánicas.
9. Dirección visual.
10. Arquitectura de Unreal Engine 5.
11. Assets que tendremos que crear en Blender.
12. Orden exacto de desarrollo.

Después de presentar este diseño, espera mi aprobación antes de comenzar la construcción.
