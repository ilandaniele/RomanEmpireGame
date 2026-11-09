# Relevo vigente GPT / Claude

2026-10-19 21:32 UTC. Goal ACTIVE, NO completo. GPT continúa aquí por autorización
del usuario; switches los dirige él. Un escritor. VM RomanEmpireQA OFF.
No claimmix/.env*, seguridad Windows, stage/commit/push ni cambios remotos.

## Terminado — C-018H_SemirealRuntimeBindings / GPT

ACTIVO C-018H_SemirealCameraPreview: nuevo Tools/ArtProbe/Native/
SemirealCameraPreview.cpp copiado SOLO al módulo test privado; setup template
actualizado para futuras copias. Test RomanPreview.Cameras.Candidate usa viewport
real, RTS/FPS de producción, PBRparents/handedness, posesión/head hide/restore y
witness visible/hidden. NO Source/Content principal cambiado en este bloque.
Camera01 buildPASS. RTS01 yFPS01 reales D3D12/SM6 PASS1/1cadauno, exit0,
1warningMotionVector cadauno; capturas inspeccionadas. Handedness±50.189cm,
visible/hidden cambia1920RTS/38704FPS píxeles, headhide/exitrestorePASS. Sin errores
de shader/material detectados. Fotos en GamePreview/Artifacts/Camera{RTS,FPS}01.
Receipt01 registra camera cache de SETUP antes del render (no posición final);
fixture corregido para muestrear metadata al capturar y exigir viewtarget=pawn.

Inspección FPS detectó herrajes púrpura: fuente semireal usa slotCrest para latón
(rivets/swordhilt/trim), NO tela de facción. Runtime03 tintaba ese slot por herencia
del prototipo. Script de autor corregido aBody+Shield solamente; crear Runtime04
NUEVO, no overwrites. Runtime03-author ycamera-capture01-source respaldados en
GamePreview. IdleFPS no muestra manos/arma propia: fixture ahora agrega ataque
real congelado en apex por playrate canónico +witness ocultando SOLO propio mesh
y proyección de mano dentro viewport. Es un gate más fuerte, NO aprobado aún.

Runtime04 autor y MaterialPolicyPIE PASS1/1clean. CameraFPS02 real produjo
Visible/Hidden/Attack/OwnHidden: fallo1 reproducible, mano ataque screenX1765 fuera
1280 y brazo invade borde; no rebajar gate. Herrajes ahora sí dorados. Capturas,
receipt y error preservados. Cámara/pawn real estable, no tocar FOV/colisión.

Corrección EN IMPLEMENTACIÓN: snapshot RomanSoldierAnimInstance.h/cpp añade
bLocalFirstPerson (player REAL/local, no simple flag) y targets mano/codo en
component space. NO clock/hit/damage/colisión/cámara nueva. Legacy graphs ignoran
campos. Nuevo autor IntegrationV3 inserta two-boneIK sin stretch en MISMO cuerpo,
activo solo localFPS, tras montage y antes de death; objetivos actor-space45/25/49
con pulso visual según montage canónico. Runtime05 nuevo debe referirV3. V2 y
runtime04-author respaldados. ConfigBody+Shield conserva latón. Necesita pruebas.

IK build01 strictPASS30.59s, AuthoringV3Applied01 PASS1/1clean (pins/compiler reales),
Runtime05 authorPASS10assets conlatónsinTinte yV3. Test MaterialPolicyPIE reforzado
para excluir localFPS/targets cuando solo hay flag de posesión enAI; build02
strictPASS13.33s. CameraFPS03 conRuntime05 PASS1/1/0errors/1MotionVectorwarning,
exit0; Visible yAttack PNG inspeccionados, mano/arma reales visibles sin FOV hack.
Handscreen872.727/546.182 (antes1765 fuera de1280), witness propio cambia41690px,
idlemesh68217px. Camera real440000/440000/168.15 idéntica antes/después.

Fixture agrega asserts IKapagado/targets0 tras exit y readonlyNodeIK único/
sinstretch/pins wired; buildcamera-ik03 PASS14.07s. CharactersIK01 PASS4/4clean.
RTS02 yFPS04 actuales conRuntime05 PASS1/1cadauno,0errors/1MotionVectorwarning,
exit0; Visible yAttack inspeccionados. WitnessRTS1922px, FPSidle68359px y propio
ataque41443px; handscreen872.727/546.182. ExitIKfalse/targets0/headrestored/pawn
originalPASS. Sin errores de compilación shader/material en logs. Camera review
nuevo M6_SEMIREAL_CAMERA_REVIEW.md; bootstrapConditionFailed no es report0err.

ÚNICA operación EN MARCHA: fullNative CameraFull01, session41064;
GamePreview/camera-full01.log, Reports/CameraFull01/index.json. Handles78287/
50178/44940/35736 y anteriores cerrados. Al terminar: Shipping/mainEditor y
readonlyRuntime05. Mainbinarios pendientes para nuevos campos. No artapproval.
Main Binaries NO actualizados todavía para los nuevos campos del AnimInstance.

Candidata vigente SOLO en Artifacts/VQ01_Semireal1/GamePreview:
/Game/ArtCandidates/Attempt10_1/Runtime03/DA_RuntimePreview
Usa IntegrationV2; NO V1/Runtime02. M6_SEMIREAL_RUNTIME_REVIEW.md contiene detalle.

- RomanCharacterPresentation.h añade bUseMeshMaterialParents=false y slots
  Body/Shield/Crest. UnitBase.cpp conserva padres PBR con MIDs actor-locales
  Color/FactionTintStrength/Damage. Legacy/defaultfalse intacto, sin resave,
  migración, cargas por evento, IDs/save version ni autoridad nueva.
- CharacterMaterialPolicyTests.cpp: dos actores reales; padres/tinte/daño,
  aislamiento/selección estable; Idle/Walk/Block/Attack/Death reales, timeline/
  sección/deformación de brazo, guard/death hold y GC. Config duplicada/transitoria
  inyectada ANTES de Initialize/BeginPlay. No limpiar overrides para lograr foto.
- Tools/author_semireal_runtime_materials.py crea padres/malla/bundle NUEVOS solo
  en RuntimeNN privado. Runtime03=10assets/8parents/3texturas compartidas; bindings
  base/normal/rough/metal conservados. Autor y readonly fresco PASS/exit0. No GPU.
- Tools/ArtProbe/Native/SemirealPreviewAuthoring.cpp compila SOLO en test module
  privado. Graph nuevo con4clips del skeleton candidato, Attackmontage/bundle en
  IntegrationV2: Attacksection0/blends.035/.08/oneSlot/oneSegment/noDamageNotifies.
  Una malla en3campos es fixture técnico, NO diseños de3facciones aceptados.

## Pruebas / procesos

- Characters candidato04:4/4 PASS,0errores/0warnings,exit0, incluye legacy.
- Full privado RuntimeFull01:141success (55clean/86withwarnings), failed/pending0,
  exit0. Warnings:85×CADA aviso WASAPI48k/44.1k,2timeouts engineHTTPgoogle204,
  1missing.png intencional. No cambiar audio/red/Windows para ocultarlos.
- IWYU privado estricto01:114acciones/108.90s PASS; final incremental04 PASS13.49s.
  Shipping privado01:78acciones/80.76s PASS; no nuevo cook/player ejecutado.
- Editor PRINCIPAL runtime01:11acciones/41.28s PASS DESPUÉS de confirmar Editor
  cerrado (Get-Process/CIM). No se cerró/operó/reabrió sesión del dueño. Binarios
  generados actualizados; Content/Config/baselines originales intactos.
- Fuentes main/private byte-exact: header/UnitBase/test. CERO cambios en Content
  preexistente privado/original. Antes del build main, snapshot306inputs intactos
  salvo2fuentes runtime aprobadas; build main cambia binarios generados, NO reusar
  hashes prebuild para afirmar identidad binaria posterior. Repo0M1warnings/diff
  PASS; Python5guardsPASS. No repetir full suite/builds sin cambio pertinente.

TODOS procesos propios TERMINALES, handles anteriores cerrados, NO pollear/
reiniciar. Últimos41234/62397/28997/55522 cerrados. PID histórico del dueño27532
ya no existía21:26UTC; comprobar estado actual antes de actuar. VM sigueOFF.

Logs/receipts bajo GamePreview: native-runtime-iwyu04.log,runtime-shipping01.log,
main-editor-runtime01.log,runtime-materials03.json,runtime-materials03-readonly.json,
runtime-full01.log; Reports/{MaterialPolicyCandidate04,RuntimeFull01,
AuthoringV2Applied01}/index.json. Actual código/errores/hashes en runtime review.

## Fallos preservados / no repetir hipótesis

Runtime01 falló pin Desaturation: API vacía=GetInput(0), no nombre de campo Input.
Partial/report preservado. Runtime02 PBR pasó pero montageV1 NO era reproducible.
Candidate02/03 detectaron ataque playing SIN deformación; separar frames NO lo
arregló. Causa: autorV1 omitió CompositeSection; engine rechaza InvalidSection.
V2/Runtime03 ahora prueban avance Y deformación, sin relajar umbral. Viejos
Integration/Runtime02/fallos y fuentes respaldadas authoring-v1-source.cpp/
runtime-materials02-author.py/material-policy-candidate03-test.cpp preservados.
IWYU03 falló API inexistente; corregida a GetSectionStartAndEndTime. No bug del
juego original/AI. Detalles/ubicación exacta del código engine en runtime review.

## NEXT — C-018H_SemirealCameraPreview / CLAUDE al switch

1. Reusar GamePreview EXISTENTE y Runtime03+IntegrationV2, no clonar/reimportar.
   No tocar checkout limpio91c ni adoptar en main Content sin review del dueño.
2. Captura privada con RHI REAL D3D12/SM6 y UnitBase/config/controlador/cámaras
   RTS/FPS reales (zoom táctico de producción). Verificar shader/PBR en GPU,
   offset/escala/facing/handedness, selección, posesión real/head hide/restauración.
   No sustituir por ortho asset-only, limpiar overrides o actualizar baselines
   viejas para pasar. No editar PIE ajeno ni abrir servicios/MCP.
3. Reutilizar patrones CharacterPlaybackTests.cpp: viewport/FStartPIECommand/
   frames/EnterFirstPersonMode/SetZoomLevel. NO sus aserciones de materiales
   planos/siluetas originales como aceptación de V2. Renders/recibos create-only.
4. Owner art acceptance, installed LOD transitions/new-model100-unit performance
   y packaged QA OPEN. Full Encounter50/100 usó arte original, no performance de
   esta candidata. VQ02 camp (terreno+muro+construcción) independiente pendiente.

## Reusar fuente / decisiones

SourceArt/Characters/RomanSoldier/semireal_v2 attempt10:3GLBs49782/12444/4978tris,
17bones/5clips/8slots/3shared2048maps/4PNG/2BLENDpacked4each/6snapshots exactos.
LOD0sha3157ac079a631d6fa2a317d550b0b3aeb9a56be9d752d423c478f7d50e0211f6.
Fuente REAL/no imagen. M6_SEMIREAL_SOURCE_REVIEW.md/README/inventario guardan
provenance/hashes/23Python/import+Raw/Compressed+renderD3D12PASS. NO regenerar.
v1/intentos anteriores intactos; CC0 MakeHuman datos+morphs a8bc2d54, no appAGPL.
Blender5.2.2portable oficial ya verificado en Artifacts/Toolchains, no reinstalar.
Aún propuesta artística, no aceptación del dueño.

UV1 NO era causa: UV0/original produjeron píxeles idénticos. DedupGLB sí redujo
24→3texturas preservando binary. Material function comparte3inputs entre salidas:
no exigir sets exclusivos. Args Unreal Python FORWARD SLASH; Windows\\r se volvió
CR en fallo guardado. RawRTF_RGBA8 devuelve0–255 aunFLinearColor: normalizar/255,
no relajar umbrales. Setup create-only/guards documentados Tools/ASSET_AUTHORING.md.

## Git / gates

HEAD344c29b/main(previos91c54c2/f79f0b8), sin stage/commit/push. Dirty previos docs/
.gitattributes/SourceArt/Tools preservados. Este bloque:2runtime files+nuevo
CharacterMaterialPolicyTests.cpp, author_semireal_runtime_materials.py,
test_semireal_game_preview.py, Native/SemirealPreviewAuthoring.cpp, runtime review,
plan/ASSET_AUTHORING/handoff. Git status da lista exacta. No main Content/Config/
baselines cambios. Binarios generados main/private actualizados.

Reusar clean91c en Artifacts/C018H_CleanCandidate1/Checkout/M7_CLEAN_CHECKOUT_REVIEW:
build/IWYU/native139/Python114/LFS176/Dev+Shippingcook594+7skip/601/error0/warning0.
Evidencia INMUTABLE anterior, no proof de cambios actuales ni player.
M7 OPEN: playerQA01–10, runnerWindowsaislado/licenciado(no disponible: soloestaPC),
securityQUIC/OpenSSL/curlHOLD(retenerQUIC;UE5.8.2aúnMsQuic2.2.0), notices/derechos,
hardware/FPS100/budget, arte/cámaras reales. M8requiereM7completo. No pollear
remoto/vendor/VM sin datos nuevos ni marcar goalcomplete por builds verdes.

Históricos exactos: HANDOFF_HISTORY_2026-10-19.md(123149bytes original) y
HANDOFF_HISTORY_2026-10-19_RUNTIME1.md(17232bytes intermedio). NO cargar historia
completa ni ejecutar sus viejos NEXT. Diagnóstico de conexión anterior guardado
Artifacts/CodexStreamDiagnostics/2026-10-19.md: NO arreglo confirmado ni cambios
auth/config/red; implementación retomada después de ese turno.

HANDOFF_READY=contexto guardado, NO milestone/arte/goal completo.
NEXT=C-018H_SemirealCameraPreview. STATE=ready; sin operaciones propias pendientes.
