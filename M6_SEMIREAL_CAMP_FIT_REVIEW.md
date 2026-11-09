# Camp02 — encaje previo a integración funcional

Estado: medición privada verificada; NO adopción, colocación real, navegación,
spawn jugable ni aprobación visual. Fuente/cook anteriores siguen válidos.
El owner aún debe responder la revisión de las vistas reales de Camp02.

## Evidencia actual, no valores del JSON de diseño

`Tools/run_semireal_camp_fit.ps1 -Name Camp02Fit01` ejecutó una consulta nativa
Python/NullRHI sobre el GamePreview existente, sin MCP conectado, actor/world/PIE,
importar, guardar ni modificar assets. PID43828 terminal0; stderr0; log nativo
`Success - 0 error(s), 0 warning(s)`.

`Artifacts/VQ02_CampRevision2/Fit/Camp02Fit01/readback.json`, SHA256
`f3b51f5a5eaccf94f97e98624109e9394f44dfe3f4c1bb2bdd6b8e519b7743d6`,
contiene valores reales de `PDA_Building_Barracks`, CDO nativo Barracks, cápsula
CDO Legionary y bounding box de `Camp02/SM_CampBarracks`. No se usó como autoridad
`BP_Barracks.design.json`: es un diseño, no los valores actuales del Data Asset.
Factory real `/Script/RomanEmpireGame.Barracks`, ID estable Barracks.

| Dato consultado | Valor |
|---|---|
| Huella en definición | 4 × 4 unidades, equivalentes a 400 × 400 cm en el código actual |
| PrototypeScale | 2.5 / 2.5 / 3.0 |
| SpawnOffset CDO | +300 / 0 / 0 cm, rotado por la orientación del actor |
| CollisionBox CDO | semiextensión 200 / 200 / 200 cm |
| Radio cápsula CDO Legionary | 42 cm; no es una cápsula medida en unidad instanciada |
| Mesh Camp02 min cm | −256.100006 / −298.5 / −0.49999988 |
| Mesh Camp02 max cm | 412.5 / 274.5 / 381 |
| Tamaño completo, con techo/porche | 668.600006 × 573 × 381.5 cm |

Receipt liga hashes de datos, mesh, DLL de gameplay y fuentes exactas de
BuildingBase/Barracks/BuildingPlacement/BuildingTypes. Esas cuatro fuentes y el
Data Asset coinciden byte a byte entre raíz y copia privada. El runner guarda
snapshots antes/después:181Content files,14sourcev2 files, Config y descriptor
intactos. Python/helper también sin cambios durante la consulta.

## Por qué no basta sustituir el mesh

1. `ABuildingBase::ApplyDefinitionPresentation` aplica PrototypeScale al componente.
   Con el mesh nuevo sin adaptación resultaría **16.715 × 14.325 × 11.445 m**,
   no los 6.686 × 5.73 × 3.815 m naturales. No se cambió ese código.
2. Placement coloca el actor en el centro de CollisionBox,200cm sobre la superficie.
   Con origen compartido del mesh y escala anterior, la base quedaría198.5cm sobre
   esa superficie. Para centrar XY y apoyar el mesh natural a escala1, el offset
   geométrico calculado es aproximadamente **(−78.2,+12,−199.5) cm** relativo al
   actor. Es un candidato matemático, NO una transformación aplicada.
3. A escala natural NO cabe en la huella actual4×4. La envolvente mínima es
   6.686×5.73 unidades; redondeada a unidades completas da una propuesta7×6.
   No se cambió el Data Asset ni se aprobó esa expansión. Reducir uniformemente
   a0.5983 para entrar en4×4 alteraría también la escala humana solicitada.
4. Tras ese centrado hipotético, SpawnOffset+300 queda dentro de la envolvente XY.
   Clearance conservador de cápsula contra AABB:−76.3cm. El centro en+X debería
   alcanzar al menos376.3cm para tangencia con esa envolvente; con una futura
   CollisionBox7×6 serían392cm. Son límites sin margen, NO posiciones de spawn
   autorizadas ni consultas de contacto con paredes/puertas. No confiar en
   AdjustIfPossibleButAlwaysSpawn como garantía de salida segura.
5. Construction/CompleteInitial resetean la ubicación relativa del mesh; un offset
   añadido solo al inicializar se perdería. SelectionOutline calcula su altura
   con PrototypeScale. Cambiar simplemente ese campo a1 no adapta ambos caminos.
6. UpdateVisuals reemplaza el material del slot0 con el material provisional de
   estado. En el nuevo mesh ese slot pertenece a Wood; no borra todos los slots,
   pero sí alteraría ese material PBR. Debe conservarse el contrato visual de
   colocación/construcción/daño sin destruir las asignaciones PBR del asset.

Todos esos puntos proceden de datos/código, no de una prueba de gameplay real.
Una AABB puede incluir aire, aleros y huecos: el clearance negativo NO demuestra
por sí solo contacto con geometría, un bloqueo real o un defecto observado en PIE.

## Implementación posterior a aprobación

Si el owner aprueba el aspecto, Claude deberá integrar primero en privado:

- Mantener Registry/BuildingDefinition como autoridad de datos, Placement como
  autoridad de huella/transacción y Barracks como autoridad de entrenamiento/spawn.
  No cambiar coste, reservas, reembolsos o contratos de campaña por una sustitución
  visual. La huella propuesta y su efecto en colocación deben quedar explícitos.
- Separar el transform/materiales de presentación de la escala del cubo provisional;
  respetar origen, construcción, outline, preview válido/inválido y selección.
- Validar espacio de salida y nav usando las autoridades existentes, orientaciones,
  terreno y cápsula reales. No convertir esta calculadora en autoridad de gameplay.
- Probar primero los casos pertinentes de colocación, estados, producción, cola/
  reembolso y salida; luego vistas RTS/FPS y gates adicionales afectados. El cook
  privado previo NO prueba esta futura integración ni reachability del player.

Si solicita otra revisión visual, actualizar el brief y encargar fuente a GPT.
No preparar geometría03 especulativa ni repetir las preguntas de estilo ya respondidas.

## Herramientas y verificación

`Tools/semireal_camp_fit.py` es matemática pura sin Unreal ni escritura;
`Tools/inspect_semireal_camp_fit.py` consulta propiedades guardadas/CDOs sin save;
`Tools/run_semireal_camp_fit.ps1` exige namespace nuevo, proyecto privado exacto,
UE/UBT inactivos, sin enlaces y preservación de inputs. No dependencia/módulo/C++
de runtime nuevos. No activar ni instalar MCP al cambiar de cliente.

9tests puros PASS: origen compartido, fit/escala, transformación ingenua, cápsula
contra envolvente/huella, expansión sin mover spawn, tangencia, distancia diagonal,
inmutabilidad e inputs inválidos. Pythoncompile/PSAST,repository0M1warnings ydiffcheck
PASS; advertencias CRLF previas sin cambios. Medición nativa pasada
arriba; no fullsuite/build/cook/recaptura/player justificados para consulta read-only.
Seguridad, licencias, QA confiable Development/Shipping, CI y M7/M8 siguen abiertos.
