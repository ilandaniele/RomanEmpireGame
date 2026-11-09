# M6 — Brief de revisión artística

Estado: ESTÉTICA APROBADA; referencia visual 01 y muestra 3D real versionada producidas. Importación aislada UE comprobada; pulido, cámaras reales y aprobación visual pendientes.

Continuación autorizada (2026-10-19): el usuario confirmó enemigos/entrenamiento y pidió seguir con todo mientras duerme. Se prepara una muestra 3D real versionada, sin reemplazar los assets aceptados técnicamente ni reclamar aprobación visual final. Ruta seleccionada para la muestra: Blender portable local a Artifacts, sin instalación de Windows, y base anatómica MakeHuman con CC0 explícito a commit fijado. Armadura/equipo siguen siendo propuesta provisional; no se compra contenido, no se fija autenticidad histórica ni se altera gameplay. Estos avances posteriores sustituyen la antigua ausencia de ruta 3D indicada abajo.
Fecha: 2026-10-19. Tarea: C-018H_VisualQualityDecision.

## Evidencia y objetivo

El usuario ejecutó L_VerticalSlice en el Editor y rechazó la calidad de los modelos. La captura muestra geometría muy simple, colores planos y suelo de prueba. Esto confirma presentación en ejecución, no el recorrido completo ni QA de los paquetes.

M6.7 verificó integración técnica mínima de mallas, rig y animaciones. No constituye aprobación del aspecto final. Este brief prepara una muestra de calidad; no autoriza cambiar assets, instalar herramientas, comprar contenido ni rediseñar gameplay.

## Decisiones abiertas antes de producir

- Estética APROBADA por el usuario (2026-10-19): «Romano semirrealista: proporciones naturales, materiales detallados y aspecto militar». No volver a preguntar semirrealista vs estilizado.
- Período histórico y referencias para armadura, casco, escudo y arquitectura. No asumir que todos los estilos romanos pertenecen a la misma época.
- Ruta de producción: autoría 3D con herramientas realmente disponibles o assets existentes con derechos comprobables. No asumir que hay Blender, un generador 3D o un MCP disponible.
- Presupuesto de adquisición, si se considera contenido de terceros. Ninguna compra autorizada aquí.

## Muestra propuesta, no una sustitución masiva

### VQ-01 — Un legionario

Muestra técnica v1 disponible en `SourceArt/Characters/RomanSoldier/semireal_v1`,
con GLB real/LOD, rig17, cinco clips, PBR y BLEND autocontenidos. Procedencia y
pruebas en [M6_SEMIREAL_SOURCE_REVIEW.md](M6_SEMIREAL_SOURCE_REVIEW.md). No sustituye
los assets del juego: agarre/axilas/ropa/cara necesitan pulido y el dueño todavía
no aprobó el resultado. Import aislado no equivale a revisión RTS/posesión.

Actualización: candidata elegida ahora **semireal_v2 / attempt10**, preservando v1.
Ya tiene agarre cerrado real, ropa ajustada al cuerpo y pesos/poses mejorados;
GLB/LOD, reimportación Blender e import/evaluación Raw+Compressed en Unreal pasan.
Próximo: verla en contexto privado del juego con cámaras RTS/FPS reales y pipeline
de materiales/animación nuevo, antes de reemplazar el aspecto del juego o extender
a otras facciones. Calidad fina/historia/rendimiento y aprobación siguen abiertas.

- Malla 3D utilizable, con proporciones humanas y silueta legible desde la cámara RTS y durante posesión.
- Casco, armadura, túnica, escudo y arma coherentes con la dirección aprobada; detalle visible también de cerca.
- Materiales y texturas donde correspondan: piel, metal, tela, cuero y escudo diferenciados, sin depender solo de colores de facción.
- Rig, pesos y animaciones de idle, marcha, ataque, bloqueo y muerte. Evaluar reutilización/retargeting del rig y clips actuales; no prometer compatibilidad sin probarla.
- Fuente editable o procedencia autorizada, exportación real e importación en Unreal. Una imagen de un personaje NO cumple este entregable.

### VQ-02 — Pequeña muestra de campamento

- Propuesta: un tramo de terreno, un tramo de muro y una construcción, no todo el mapa.
- Escala coherente con el legionario; terreno terminado, sin cuadrícula de material de prueba en la muestra.
- Materiales, iluminación y detalle compatibles con el personaje y con la lectura de unidades en RTS.
- No cambiar posiciones funcionales, colisiones o navegación sin comprobar su impacto. No añadir nuevas mecánicas.

## Cómo se aprueba

### Revisión solicitada del campamento — ambas áreas elegidas

El usuario respondió «Prefiero ajustes visuales» a la muestra privada. No aprobó
su adopción ni cambió la estética romana semirrealista ya elegida. Se revisaron
las capturas reales existentes CampGPU02/RTSAll.png y FPSAll.png, sin recapturar.
El fondo azul y el encuadre aislado son del fixture técnico: no tratar esa imagen
como composición del mapa final ni cambiar las cámaras para ocultar defectos.

Respuesta posterior explícita22/oct: «Cambiar ambas cosas» (arquitectura y
materiales). Se produjo fuentev2 real con barraca alargada/porche/aberturas y
nuevos materiales: [revisión y evidencia](M6_SEMIREAL_CAMP_REVISION_REVIEW.md).
Los cuatro GLB reimportados en Blender pasan; falta integración privada Camp02
y revisión en cámaras reales de Unreal. No volver a preguntar qué área cambiar.

Propuesta para concretar con el usuario, NO cambios ya aprobados/implementados:

| Prioridad | Observación en estas vistas | Revisión propuesta |
| --- | --- | --- |
| 1, terreno | Moteado de frecuencia y color muy uniformes en toda la parcela | Variación de escala/color más amplia, tierra transitada y transiciones junto a cimientos; conservar lectura de unidades |
| 2, barraca | Paramentos simples y cubierta con poca separación visual entre filas desde RTS | Mayor diferenciación de tejas/solapes y detalles de borde; desgaste localizado en revoco/cimientos, sin añadir interiores ni cambiar la función |
| 3, madera | Empalizada y vigas con repetición de forma/color visible | Variación contenida de tono, veta y desgaste; no aumentar arbitrariamente tamaño ni sustituir la silueta militar |

La mano/arma visible en FPS pertenece al candidato de personaje, no a la barraca;
si el usuario la señala, registrar una revisión VQ-01 separada. No atribuir todos
los defectos de apariencia a iluminación ni a la resolución de las texturas.

Ambas áreas fueron elegidas; fuente semireal_v2 entregada conservando v1/Camp01
y sus capturas/gates. No se amplió el presupuesto65ktri/atlas2K ni se adoptó
mainmap. Siguiente: importación privada versionada y nueva inspección RTS/FPS
con verificaciones pertinentes antes de solicitar aprobación de adopción.

Actualización de integración: Camp02 ya tiene importación/readback privado y
capturas RTS/FPS reales verificadas, conservando Camp01 y el mapa principal.
Ver M6_SEMIREAL_CAMP_REVISION_REVIEW.md para imágenes, prueba negativa y límites.
Decisión real del owner (2026-10-30): «Pedir más ajustes visuales». Camp02 NO se
aprueba para integrar. Se mantiene el estilo romano semirrealista y el encargo
de arquitectura Y materiales; falta especificar qué debe cambiar del resultado
actual. NEXT G-VQ02_CampRevision03Brief / GPT: recoger detalles o captura marcada/
referencia y criterios de aceptación, preservando v2. No generar v3 especulativa
ni asumir aprobación de la propuesta de footprint7x6 o salida de unidades.

Instrucción posterior2026-10-30: «segui la tarea no importa». Se continúa v3 con
criterio artístico delegado, sin pedir otra vez detalles ni aprobar Camp02.
Brief de ejecución: identidad militar mediante bandas óxido/gable, rack de equipo
y herrajes; menos moteado del suelo y contraste de vetas. Dimensiones/layout,
puerta central, cámaras,3meshes/6materiales/atlas2K/65ktri conservados. Fuente
versionada nueva y reimport real antes de importar/verificar Camp03. No gameplay,
footprint7x6/mainmap ni release autorizados por esta delegación artística.

Fuentev3 ya entregada y reimportada realmente en Blender: bandas óxido/hastial,
rack de equipo/herrajes, materiales menos moteados.14files/35,410tri/atlas2K,
fuentesv1/v2 intactas. Ver M6_SEMIREAL_CAMP_REVISION3_REVIEW.md. Próximo Camp03
privado en Unreal; los renders nuevos NO sustituyen vistas RTS/FPS del juego.
No volver a bloquear esta iteración esperando el brief que el usuario delegó.

Camp03 YA importado y verificado en Unreal privado, con cámaras reales RTS/FPS/
retorno,7PNGs inspeccionados y control negativo de pose detectado.193Content files
intactos y perfiles históricos conservados. Evidencia y límites en review3.
Próximo cook privado v3; ninguna aprobación mainmap/footprint/gameplay/player/release.

Cook privado v3 también verificado (12assets/35hard sin ausencias/6materialesSM6,
193Content+14source intactos). Siguiente: adaptador de presentación opt-in de
edificios con defaults actuales preservados y verificación privada primero;
no adoptar automáticamente mapa/huella/salida ni reinterpretar cook como QAplayer.
El cook privado NUEVO de Camp02 también pasó:12assets/35dependencias hard sin
ausencias,6materialesSM6,archivosSM5+SM6 y181Content files intactos. Evidencia en
M6_SEMIREAL_CAMP_COOK_REVIEW.md; no prueba player, nav ni aprobación visual.

### Secuencia de aprobación

1. Aprobar dirección y referencias. Los conceptos son referencias, no prueba de assets integrados.
2. Revisar VQ-01 dentro de Unreal: vistas RTS y posesión usando las cámaras reales, más idle/marcha/ataque/bloqueo/muerte. No aceptar solamente renders de otra aplicación o una miniatura del Content Browser.
3. Obtener aprobación explícita del usuario sobre el aspecto del legionario antes de ampliar a otras facciones.
4. Revisar VQ-02 junto al personaje en una muestra acotada, con aprobación visual del usuario antes de extenderla al mapa.
5. Verificar integración: referencias guardadas, rig/animación/materiales, selección, facción, daño, posesión y ciclo de vida; pruebas dirigidas de Characters/Units/Materials según los archivos afectados. Build/IWYU si cambia C++; cook y paquetes si cambian assets integrados.
6. Medir rendimiento con el hardware y presupuesto acordados en M7. No inventar un objetivo FPS ni aprobar rendimiento por una captura estática. La aceptación artística y la técnica son necesarias por separado.

## Invariantes de integración

- Conservar autoridades de gameplay, IDs y formato de guardados, cápsula, navegación, selección y reglas de combate. La animación no pasa a decidir daño ni movimiento canónico.
- No reemplazar silenciosamente assets compartidos, modificar baselines para ocultar regresiones ni relajar contratos para admitir una malla incompatible. Si hacen falta migraciones o nuevos límites, presentar el impacto primero.
- Mantener una ruta recuperable al material actual hasta aprobar la muestra. Usar rutas versionadas para candidatos y registrar qué referencias se cambian.
- Fuentes y assets Unreal bajo Git LFS; capturas y resultados generados en Artifacts/ o Saved/. Registrar autor, origen, licencia, restricciones, hashes y ruta en ASSET_INVENTORY.md al incorporar contenido real.
- No tocar la VM, seguridad de Windows, backend ni archivos .env de otros proyectos.

## Reparto y relevo

- GPT: cola VQ-01/VQ-02, referencias visuales cuando el usuario las solicite y la dirección esté aprobada; autoría de modelos solo mediante herramientas 3D realmente disponibles. No confundir bitmap con malla.
- Claude: integración/importación, adaptación técnica, código, pruebas, builds y documentación cuando se disponga de assets aprobados y el usuario cambie de modelo.
- Un solo modelo escribe a la vez. Guardar en HANDOFF.md entregables reales, decisiones, archivos, resultados y pendientes antes del switch.

Disponibilidad comprobada (2026-10-19): generador nativo de imágenes disponible; ninguna herramienta de generación de mallas 3D encontrada en el catálogo actual. Blender no está en PATH ni en su directorio habitual de Program Files; esto no descarta una instalación portátil en otra ruta. No se instalaron herramientas. La ruta de autoría/adquisición 3D sigue pendiente, sin prometer mallas mediante imagegen.

## Referencia visual VQ-01 / propuesta 01

Modo: skill imagegen, herramienta integrada, imagen nueva sin usar las capturas del usuario como entradas. La referencia no modifica el juego ni constituye malla, rig, textura PBR o aprobación histórica. Armadura de malla y equipo son una propuesta para revisar, no un período definitivo elegido automáticamente.

Estado de generación: COMPLETADA con herramienta nativa; no CLI, claves ni servicios adicionales. Imagen mostrada al usuario e inspeccionada: tres vistas completas, anatomía natural, equipo militar y materiales diferenciados; no es evidencia de un rig, geometría o aspecto final en Unreal. Período histórico y exactitud de la reconstrucción siguen sin aprobar.

- Archivo local: `Artifacts/VQ01_LegionaryConcept1/Legionary-Proposal1.png` (salida generada ignorada por Git, no importada).
- PNG, 1536×1024, 2254449 bytes; SHA256 `326C35E5138ED7571D9EDFBD4297746002AF66EE4AD4C8D83630443915414BAC`.
- Copia verificada contra la salida nativa; original conservado en `C:/Users/Ilan/.codex/generated_images/01a06958-0725-7690-a93c-740fd59fc463/exec-b9257415-9875-4423-b158-b96422c4dafe.png`.
- Solo referencia para revisión, no aprobación del legionario, licencia comercial ni asset de producción. VQ-02 todavía no generado. Procedencia registrada en ASSET_INVENTORY.md.
- La skill imagegen mantuvo el resultado como referencia raster separada de mallas y Content; no se sustituyó el requisito 3D por una imagen.

Prompt completo usado:

```text
Use case: stylized-concept.
Asset type: one game-character concept turnaround sheet for RomanEmpireGame, VQ-01, not an in-game screenshot or a deliverable mesh.
Primary request: a semi-realistic Roman legionary with natural human proportions, detailed believable materials, and a disciplined military appearance, to replace crude prototype characters.
Subject: the SAME adult male infantryman in three consistent full-body views: front, side, and rear. Practical Roman-inspired iron helmet with cheek guards, mail armor over a muted red tunic, leather belt and sandals, curved red shield and sheathed short sword. Mail is a provisional design proposal, not a claim of historical-period approval. Keep anatomy and equipment identical across all three views, including height, silhouette, shield, straps, and wear.
Style/medium: polished semi-realistic game art direction reference, grounded anatomy, realistic-sized head and hands, restrained campaign wear, clearly readable craftsmanship. Not a photograph of an existing person, not a cartoon, not low-poly.
Composition/framing: a clean landscape turnaround sheet, three aligned head-to-toe views on one plain warm-gray studio background. Neutral standing poses with arms slightly separated from the body; equipment must not obscure all torso details. Leave space around each silhouette. No camp, battle, extra characters or interface.
Lighting/mood: neutral diffuse studio lighting with subtle contact shadows, all armor and fabric details readable; no dramatic darkness.
Materials/textures: physically plausible iron, chain mail, woven cloth, worn leather and painted wood; controlled roughness and tactile surface variation, not glossy plastic.
Text (verbatim): "LEGIONARIO · PROPUESTA 01" and a small footer "REFERENCIA VISUAL · NO ES UNA MALLA 3D".
Constraints: this is visual reference only. No claims of being a rendered Unreal asset, no fake engine UI, no wireframe or polygon statistics, no watermark or third-party logos. Keep the same design in all views.
Avoid: oversized spherical heads, toy-like bodies, cubes for torsos, shiny plastic, fantasy spikes or horns, superhero anatomy, giant pauldrons, excessive gold ornament, mixed armor styles, gore, clutter, cinematic background, cropped feet or helmets.
```

M7, la prueba real de Development/Shipping, derechos de distribución y la aceptación visual final permanecen abiertos. Este brief no declara el objetivo terminado ni inicia M8.
