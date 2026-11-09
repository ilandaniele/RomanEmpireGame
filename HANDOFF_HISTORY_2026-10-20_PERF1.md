# Relevo vigente GPT / Claude

2026-10-20 13:36 UTC. Goal ACTIVE, NO completo. Objetivo íntegro:
IMPLEMENTATION_PLAN.md C-006→gates autorizados; switches los dirige el usuario.
GPT autorizado a implementar aquí. Un escritor. VM RomanEmpireQA OFF.
No claimmix/.env*, auth/red/seguridad Windows, stage/commit/push ni checkout91c.

## En curso — C-018H_PrivateGPUPerformance / GPT, 2026-10-20

Nuevo Tools/ArtProbe/Native/SemirealEncounterProfile.cpp copiado SOLOaprivateTests.
Observer opt-in PRE-BeginPIE cambia CDOCharacterPresentation aLOD01 y restaura
exactooriginal alEnd; noeditaassets niUnitFoundationTests ni gameplay. Reusa
Encounter50/100 completos (combate/training/FPS/victoria). Captura poblaciones,
ticks/visibilidad/LODpredicho/realtime +CSV original yPNG ANTES delCSV.
Flags -RomanSemirealProfile=<freshName> -RomanSemirealProfileUnits=50/100 junto
-RomanCaptureEncounter enGamePreviewD3D12/SM6/1080p. No calidadoverride/congelado.
Strictbuild01 PASS4actions/15.98s; build02 añadeCSVObserverTick PASS terminal.
Nuevo Tools/capture_semireal_encounter.ps1: preflight/manifest/hardwarepower/PID/
trace+CSV+report/hashes; ASTPASS. Captura Perf50_01 EN CURSO:
GamePreview/Artifacts/SemirealPerformance/Perf50_01{.log,.process.json,.utrace},
Reports/Perf50_01, nativefolder Perf50_01. ObservarPIDguardado, NO relanzar.
Perf50_01 terminalEXIT255/failed1: canonicalgameplay sívictoria70.4s/4198frames,
peroCandidateUnits=0; nativeconstructorUnitBase.cpp46–47 resetea artporINSTANCIA,
CDOoverride insuficiente. No usarCSV01comoarte nuevo. Fallasguardas población/PNG
esperadas, archivos PRESERVADOS. Solucióninstalada API LevelActor.cpp753:
OnActorPreSpawnInitialization ANTES dePostSpawnInitialize, handler soloPIE/clase,
asignaCandidateantesInit/BeginPlay. CDOahorasololectura. Sin sourcegameplay/assetedit.
Build03 PASS4actions/13.79s. Perf50_02 EN CURSO, nombres02fresh,
PIDenPerf50_02.process.json; OnActorPreSpawnInitialization asignaantesInit.
No cambiarfuentescompiladas mientrasUEvivo. Perf01qualitysg3,realtime/1080output,
pero r.ScreenPercentage=0 (defaultauto): NO asumir rasterinterno100% sinverificar.
Siguiente: verificarcaptura02/artreal/telemetría; luego100 yanalizarCSV;
separar framesRTSpoblacióncompleta vsbajas/FPS. No prometer target hasta medir.
STATE=waiting. BloqueLOD anterior completo técnico ypreservado debajo.

## Terminado — C-018H_PrivateLODCheck / GPT (técnico privado)

REUSAR Artifacts/VQ01_Semireal1/GamePreview/RomanEmpireGame.uproject.
Candidata LOD: /Game/ArtCandidates/Attempt10_1/LOD01/DA_LODPreview
malla SK_LODPreview, graph IntegrationV3, padres PBR Runtime05.
Runtime05/DA_RuntimePreview es candidata cámara original; NO Runtime03/04/V2.
Main Content/Config/baselines y fuenteGLB/PNG/BLEND SIN adopción/cambio.
Owner art acceptance sigue OPEN; no tres diseños aprobados: refs3facciones
usan MISMO legionario técnico. NO actor/collision/nav/IDs/damage/save redesign.

- Static: LOD01 import+freshreadonly PASS; 3LODs reales49758/12420/4954tri,
  38756/12165/6000vertices,8slots/2UV cadauno,17bones, screens1/.5/.25.
  Native LODReadback03 1/1clean/0failed/pending,exit0, bind/pesos/indices/slots/UV/
  restbounds comprobados. Fuentes raw49782/12444/4978:24caras coincidentes porLOD,
  SOLOlatónCrest; análisisfuente+conteosEXACTOSporSlot reconcilian. No editarGLB.
  bRemoveDegenerates FALSE preservado; import/chunk descartaindicescolapsados
  independientemente. Fallasstatic01/02 eran expectativas equivocadas, archivadas.
- Animation02:1/1clean/0failed/pending,exit0.3actores reales/graph canónico,
  engine ComputeSkinnedPositions sobre renderLOD ypaletas/pesos reales.
  >=25vertices regionales se mueven: Idle>.1cm,Walk/Block/Attack/Death>10cm.
  MaxIdle~1.8,Walk25.48,Block28.63–28.70,Attack151.33–152.34,Death246.90cm.
  Block/finalDeathhold delta0. NullRHI pruebaCPU NOpixels. Actor ticks aparcados/
  animación manual porframes: NO QA inputfísico/tiempocombate completo.
- Mutation01 EXPECTEDfailed1/error3: flag privada
  -RomanLODAnimationSuppressAttack cancela SOLOmontage deactores temporales;
  cadaLOD falla exacta comprobación Attackdeform. Residuo breathing~.52cm,
  0vertices>10. Sin editarassets/restauración pendiente. Normal02 SAMEsource/binary
  pasa sinflag. No relajar a.5cm: ese umbral confundía posible breathing con ataque.
- GPU03:1successWithWarnings,0errors/failed/pending,exit0; SOLOwarning conocido
  MotionVectorSimulation render-thread, no suprimido. D3D12/SM6 real elegido
  adapter0 NVIDIA RTX5070Ti LaptopGPU (metadataAutomation puede decirIntel).
  ViewportREAL1280x720, NOSceneCapture/cámarafalsa/escala/FOVhack.
  Zoomproducción.55/.6625/.70 renderLOD2/1/0 automáticos; FPSreal auto0;
  forcedFPS1/2/0; hiddenwitness; exitmismoRTSpawn/head/IKclear/autoLOD2.
  Witnesspixels29950/29733/29327/29874. NuevePNG inspeccionados TODOS.
  FuenteLOD2 closeFPS es másfacetada; autoFPSeligeLOD0. NOownerqualityapproved,
  vídeo continuo sinpop, Slate/UMG completo, packagedplayer ni performance.
- GPU01falló2expectativas: maxzoom yaLOD0 y hiddenmesh noMeshObject. GPU02
  midpoint.625 aúnLOD2. GPU03 sample.6625 legítimoLOD1, SIN editarthresholds.
  Todasfallas/PNG/recibos preservados. LeerAutomationJSON, exit0 puede serFAILED.
- Strict private builds Animation01/02 PASS25.00/14.19s; GPU03/04/05
  PASS17.85/17.61/13.90s,4actions cadauno. GPUbuild01/02 erroresAPIconst/TObjectPtr
  preservados. No141full/main/Shipping rerun: cambios SOLOtools/privatefixtures.
- Private Content157/157 hashes idénticos tras GPU/mutation. ComparadorPS inicial
  anidóJSONArray y diofalsoalarmcount; iteración directa corrigió, SIN assetrepair.
  Python guards9/9, PS AST, repo0M1warnings/diffPASS. Sourcecopiasnative SHAigual.
  No procesos propios vivos. Handles92731/45041/71072/32507/41767/66138/64396/
  41543/39799/15420/45983/51992/50287 TERMINALES, no pollear/relanzar.

## Evidencia / archivos de este bloque

M6_SEMIREAL_LOD_REVIEW.md (incluye historia+latest), IMPLEMENTATION_PLAN.md
sección finalLOD, ASSET_INVENTORY.md, Tools/ASSET_AUTHORING.md actualizados.
NUEVOS Tools/ArtProbe/Native/{SemirealLODAnimation,SemirealLODGPU}.cpp;
Tools/New-SemirealGamePreview.ps1 incluyeambos paraFUTURAScopias: NO rerun creador.
Scripts author_semireal_lods.py y Native/SemirealLODReadback.cpp previos correctos.
Nuevo archivoarchivo HANDOFF_HISTORY_2026-10-20_LOD1.md, byte-exact16087B previo.

Dentro GamePreview:
 Reports/{LODReadback03,LODAnimation02,LODAnimationMutation01,LODGPU03}/index.json;
 native-lod03.txt; lod-animation02.txt; lod-animation-mutation01.txt;
 Artifacts/LODGPU03/{AutoFarRTS,AutoMidRTS,AutoNearRTS,AutoFPS0,ForcedFPS1,
 ForcedFPS2,ForcedFPS0,HiddenFPS,AutoReturnRTS}.png + gpu-lod-receipt.txt.
 lod-author01.json, lod-readonly01.json, source-topology{,_LOD1,_LOD2}01.json;
 lod-dynamic-content-before01.json; lod-dynamic-cohort01.json currenthashes.
 lod-cohort01.json es snapshot ESTÁTICO ANTERIOR (tools ahoraactualizados).
 Logs lod-animation{01,02,-mutation01}.log, lod-gpu0{1,2,3}.log,
 lod-{animation,gpu}-buildNN.log. GPU03 antes del fortaleceAnimationfixture;
 GPUsource no cambió después. No fingir mismobinario antes/después deese build.

## NEXT — C-018H_PrivateGPUPerformance / Claude al switch (GPT autorizado aquí)

1. NO repetir import/static/motion/GPU ni crear otropreview. ReusarLOD01/IntegrationV3.
2. D-05 REAL:60FPS1920x1080 en laptopLenovo/RTX registrada con50 unidades slice,
   más100-unit STRETCH separado. NO inventar60FPS100 obligatorio ni cerrar con
   este probe1cuerpo720p. Declarar adapter/RHI/CPU/RAM/settings/resolución/power/
   muestra/cámara/LOD/recuento real y alcanceEditor vsplayer. No menorfidelidad,
   viewportvacío/menosunidades/ticksAIanimaparados para darverde.
3. Source/RomanEmpireGameTests/Private/UnitFoundationTests.cpp contiene
   Encounter50/100 originales. Inspeccionar SOLOsecciones necesarias; esos viejos
   NullRHI/artoriginal NO pruebannewmodelGPU. Preparar fixturePRIVADA con bootstrap
   LOD01 antesInit, simulación+render reales/cámara existente, recibos create-only.
   Separar costeGPU/frame deoverheadEditor; no prometer player60 por buildgreen.
4. Si performance requiere arquitecturaambigua, medir primero; no sustituir gameplay
   por proxymeshes ni ocultarsombras/materials/anim para bajar coste.
5. Independiente assetGPT G-VQ02_CampSource1: terreno+muro+una construcción,
   legionarioescala/materiales semirealistas, sinchecker, nav/collision funcional
   intactos. M6_VISUAL_QUALITY_BRIEF.md,VQ02. PropuestaPRIVADA/períodohistórico
   noaprobado; no expandir mapa/otrasfacciones hastaownerreview.

## Invariantes / trabajo anterior NO rehacer

- Main C++compatible (previo): RomanCharacterPresentation.h bUseMeshMaterialParents
  defaultfalse y FactionTintSlots legacyBody/Shield/Crest. UnitBase.cpp actor-local
  PBR MIDs Color/FactionTintStrength/Damage. Semireal Body+Shield SOLO; Crest en
  estamalla es LATÓN(hilt/rivets/trim), NO tela. No cambiar defaultgloballegacy.
- RomanSoldierAnimInstance.h/cpp snapshot player REAL/local, mano/codo component
  space. V3 same-body TwoBoneIK hand_r SINstretch trasmontage/antesdeath; local→
  component→IK→local; reutiliza clockmontage, sin damageNotifies ni nuevosowners.
  AI flagpossession NO localFPS. Exittargets0. Oldgraphs ignoran campos.
- Runtime05=8padres+mesh+bundle10assets,3textures compartidas. Skeletonpropio17bones,
  rootmotionoff/noPhysics. V1montage sinAttacksection jugabaSINdeformar; V2correcto
  section0/blends.035/.08, V3 mantiene y corrige manoFPS fuera pantalla1765→872px
  SINFOV/camera/scalehack. OldV1/V2/fallos preservados.
- CameraFull01 previo141success(140clean/1intentionalmissing.pngwarning),0failed/
  pending; CharactersIK01 4clean, mainEditor/privateShipping builds PASS previos.
  NO prueba nuevos100units ni cook/player actual. CameraRTS02/FPS04 imágenes/
  recetas/cohort enM6_SEMIREAL_CAMERA_REVIEW.md. No repetir sin cambiojustificado.
- SourceArt/Characters/RomanSoldier/semireal_v2 attempt10:
  3GLB/17bones/5clips/8slots/3shared2048PBR/4PNG/2BLEND packed4each/6recipe.
  LOD0SHA3157ac079a631d6fa2a317d550b0b3aeb9a56be9d752d423c478f7d50e0211f6,
  LOD1SHA99e5d203ba969c17704148dcce886c03ad781d194bcf6ddfe60ddd8e2e44d1c0,
  LOD2SHA6c61948ebe48db34494dfdb6f71a4994d15940b101166a33c422b5ddacad1110.
  CC0 MakeHumandata/morphs a8bc2d54, noappAGPL; derechos/inventario sourceReview.
  Blender5.2.2portableOFICIAL ya verificado Artifacts/Toolchains: NO reinstalar.
  UV1≠causa, GLBdedup24→3correcto. UnrealPython scriptargs FORWARDSLASH
  (Windows\r se volvióCR). RTF_RGBA8raw0–255 normalizar/255.
- MCP no disponible/usado, privateFab/MCP/toolsets OFF. No conectarservicios.
  Skills unreal-mcp/data-assets/testing/gameplay/module ya leídas; reutilizar.

## Git / gates / operación

HEAD344c29b/main (previos91c54c2/f79f0b8). Dirtydocs/.gitattributes/SourceArt/Tools
y5runtime/testfuentes previos siguen SINcommit. git status da listaexacta.
Este bloque solo2fixtures+previewcreator+docs/archive, NO mainruntime changes.
Noautostage/commit/push. Owner Editor ausente último check; verificar antes deactuar,
nunca cerrar sesión ajena ni compilar/editassets mientras Editor/PIE activo.

Clean91c Artifacts/C018H_CleanCandidate1/Checkout inmutable:
M7_CLEAN_CHECKOUT_REVIEW139native/114Python/LFS176/cook594+7skip/601/error0/warn0
es evidenciaANTERIOR, NOactualplayer/arte. M7OPEN: QA01–10Development/Shipping;
runnerWindowsaislado/licenciado no disponible(soloestaPC), securityQUIC/OpenSSL/
curlHOLD(retenerQUIC;UE5.8.2aúnMsQuic2.2.0), notices/derechos, performance yownerarte.
M8requiereM7. NoWindowssecuritybypass, VM/vendor/remoteloops sin nuevosdatos.

Streamdisconnected: Artifacts/CodexStreamDiagnostics/2026-10-19.md,
NO arregloconfirmado/cambiosauth/config/red. RecomendaciónmanualReloadWindow/chat
nuevo corto desdeestehandoff; no reiniciar clienteautomáticamente en turnoactivo.
Contextoprevio completo en HANDOFF_HISTORY_2026-10-19{,_RUNTIME1,_CAMERA1}.md
y2026-10-20_LOD1.md; NO leerlosenteros ni ejecutarviejosNEXT.

HANDOFF_READY=contextoguardado, NOmilestone/goalcompleto.
NEXT=C-018H_PrivateGPUPerformance. STATE=ready; sinoperacionespropiaspendientes.
