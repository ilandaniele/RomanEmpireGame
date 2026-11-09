# Private camp integration — real Unreal readback and cameras

2026-10-20. C-018H_PrivateCampPreview. Technical private preview verified below.
Owner approval, main-map adoption, functional navigation/gameplay, cook/player
and environmental performance remain OPEN. This is NOT completion of M6/M7/M8.

## Scope and source protection

Reused EXACT existing Artifacts/VQ01_Semireal1/GamePreview, no new project.
SourceArt/Environment/RomanCamp/semireal_v1 remains byte-unchanged. Provenance,
actual soil1254 vs atlas2048, source rights and common-origin pivots:
[source review](M6_SEMIREAL_CAMP_SOURCE_REVIEW.md). No new service/download/plugin.
MCP tools unavailable; used owned sequential Editor processes, no MCP server.

Before import saved157private Content file hashes in
Artifacts/VQ02_CampSource1/private-content-before01.json. New namespace ONLY
/Game/ArtCandidates/Camp01. Source GLBs contain3/6/15texture descriptors for
3actual PNG images each. Existing tested semireal_gltf_cleanup.deduplicate_glb
(3targeted testsPASS) prepares private staged copies, changing ONLY duplicate
JSON texture descriptors/references; geometry/accessor/image binary unchanged.
Original3individual source GLBs/manifest not overwritten. Stagedcopies/hashes
are in GamePreview/Artifacts/CampImportSources/Camp01 and camp-author01.json.

## Import and readback

Tools/import_semireal_camp.py exact-project/create-only/read-only/reparse guards;
Tools/run_semireal_camp_import.ps1 owned hidden process, fresh logs/PID/exit/report.
Interchange imports3individual static meshes into same new folder; material search
LOCAL reuses existing new camp materials/atlas. No search/replacement outside it.
Static-only, animation/skeleton/collision/Nanite/lightmapUVgenerationOFF explicitly;
full native geometry retained, not a simplified performance substitute. Six new
materials reuse THREE texture assets;12new uassets total, privateContent169.
Saves ONLY owned new assets, never global dirty content or original files.

First author01 saved12assets but failed its verification because
StaticMeshEditorSubsystem is NOT instantiated in this commandlet. No reimport or
new namespace: corrected to public reflected StaticMesh.GetNumLods and performed
fresh readonly01, exit0/PASS,169/169file hashes identical. Collision verification
was explicitly deferred to native GetBodySetup, never claimed from a pipeline flag.
Failed author01/report/exit-1 preserved; it is not an import geometry failure.

| Mesh | Native triangles | Sections | cm bounds size |
|---|---:|---:|---|
| SM_CampGround |1152|1|1200 ×1000 ×1.177|
| SM_CampWall |3704|2|925 ×37.75 ×259.920|
| SM_CampBarracks |46602|5|493 ×407.700 ×373.350|

All oneLOD/oneUV, no generated simple collision in native BodySetup (0elements).
Texture sizes2048x2048: BaseColor sRGB/TC_DEFAULT; packed Camp_Metallic-Camp_Roughness
linear/TC_DEFAULT; Normal linear/TC_NORMALMAP and importer flip_green=true.
No manual green flip/tint/UVscale/geometry scaling. Material graph traversal records
the actual shared3texture refs and separate BaseColor/Normal/Roughness/Metallic
function output pins; no empty/foreign parent. Python readonly receipt records
bounds min/max including the normal coordinate-system Y flip, not guessed offsets.

Native fixture Tools/ArtProbe/Native/SemirealCampPreview.cpp is copied ONLY into
private Editor Tests. No root runtime/Build.cs/module/config/asset changes.
Future preview creator includes this fixture; creator was NOT rerun. Strict
non-unity/noPCH private builds01/02 pass4actions18.43/16.71s. Readback01 actual
native report1clean/0errors/failed/pending, exit0, then repeated with latest binary
as part of GPU02 family below. Production Assert-AutomationReportPassed rejects
RomanPreview.* intentionally (allows only RomanEmpireGame.*); incidental wrapper
failure preserved, helper NOT weakened. Private wrapper validates EXACT two tests.

## Real-player camera proof and caught false positive

Owned fresh L_Testbed PIE; transient camp actors share original imported pivot,
scale1 and source dimensions, at isolated origin440000/440000. Components explicitly
NoCollision and CanEverAffectNavigation=false, imported PBR parents unchanged.
Canonical soldier, LOD01/IntegrationV3 bundle, canonical InitializeUnit and actual
controller EnterFirstPersonMode/ExitFirstPersonMode; NOT a camera or mesh proxy.
Normal RTS pawn/default tactical zoom, real viewport1280x720 D3D12/SM6. No custom
FOV/projection/SceneCapture/rescaling/baseline replacement. Source pose held ONLY
on the owned actor; not real-time battle/physical input/nav/performance evidence.

CampGPU01 returned nativeSuccess but receipt inspection caught cameraZ falling
-402→-2163 after possession re-enabled movement above NoCollision art. Pixel delta
could be camera motion. This capture is INVALIDATED for FPS/visibility proof and
preserved, never relabelled a quality pass. No gameplay/collision change to hide it.
Fixture re-parks ONLY owned source actor after possession/exit and now asserts
actor position remains within.1cm and each hidden witness uses IDENTICAL camera
position/rotation within.1cm/.001degree. It does not invent support collision or
park units in a performance test. This is an explicit static source-review fixture.

CampGPU02: exit0,2native tests (1clean Readback,1GPUwith known MotionVectorSimulation
warning),0errors/failed/pending.169Content hashes unchanged. Seven actual PNGs ALL
inspected in GamePreview/Artifacts/CampGPU02: RTSAll,RTSHiddenGround,RTSHiddenWall,
RTSHiddenBarracks,FPSAll,FPSHiddenCamp,RTSReturn. FPS body/headhide/exit restoration
checked; RTS returns same pawn/viewpoint. Native receipt completed1/test_errors0.
RTSdefault is deliberately a small bounded12x10m sample within the production
camera's larger field, not a redesigned map or scale hack. FPS shows real plaster,
timber, terracotta, stone base/ground and same-body sword/hand at human height.

| Visibility witness | Changed pixels | Identical camera proof |
|---|---:|---|
| Ground hidden in RTS |47372|RTS snapshot checked|
| Wall hidden in RTS |4216|RTS snapshot checked|
| Barracks hidden in RTS |14440|RTS snapshot checked|
| Camp hidden in FPS |705359|FPS snapshot checked|

Both FPSshots camera X439827.101/Y440316.999/Z165.904,yaw-55/pitch-3.
Native receipt records actual pawn/viewpoint for every frame. Delta includes
normal geometry/shadow contribution, not individual surface segmentation.

CampPoseMutation01 SAME fixture/DLL with process-only -RomanCampSkipPoseHold:
expected GPUFail1/error1, exact “Owned source actor never falls/moves between art
witnesses”, ReadbackSuccess0errors, known GPUwarning only; processexit255.
New capture_semireal_camp.ps1 requires this EXACT failure, not any failed test,
and verifies all169Content hashes unchanged. No asset restore needed; flag is
private Editor opt-in, no production runtime or Shipping path. Positive GPU02
passes without flag. Mutation proves the falling-source false positive is caught.

Startup “Condition failed” log strings precede current tests and also appear14times
in the previous unmodified Perf50_NativeDiag02 log. Do not claim globally error-free
logs from per-test success; exact native reports/current receipt are authoritative.
No fatal crash or current-test unexpected error was accepted.

## Evidence / remaining gates

Artifacts/VQ02_CampSource1:camp-author01*,camp-readonly01*,camp-build01/02*,
camp-native-readback01*,camp-gpu01* (INVALIDFPS),CampGPU02*,CampPoseMutation01*.
GamePreview:camp-author01.json/camp-readonly01.json,
Reports/{CampReadback01,CampGPU01,CampGPU02,CampPoseMutation01}/index.json,
Artifacts/CampGPU02/7PNG+camp-gpu-receipt.txt. Native source/private-copy SHA equal.
Do not rerun used namespaces or replace evidence/baselines. Processes terminal.

private-camp-cohort01.json in VQ02_CampSource1 records original157/current169
Content hash checks, SAME positive/mutation DLL/source SHA,7PNG/12asset/report
hashes and each published/staged GLB SHA. Independent BIN-chunk hashing confirms
all3staged files retain byte-identical geometry/accessor/image payloads. Source
JSON descriptor dedup did not silently modify image/geometry content.
Targeted Python guards:3dedup+9preview=12PASS; Python/PowerShell syntax guards,
repository validation0openM1warnings and git diff --check PASS. No full suite,
root/main build, cook or player rerun was claimed in this block.

NEXT C-018H_PrivateCampCook: read packaging skill, verify/cook this private static
content with explicit reachability, actual cooked registry/dependency/texture
evidence, without pretending it is a trusted player. Reuse existing private project;
do not alter production maps, original157assets, collision/nav or adopt new art
without owner review. Source/current preview acceptance is not owner acceptance.
Full functional camp integration/inputs/navigation, cook/player/environmental
performance/rights and M7/M8 remain OPEN. Earlier50/100performance captures had NO
camp assets drawn, so they are NOT evidence for performance of this new environment.

Latest2026-10-21: private directed cook/registry/dependency/SM6 content evidence
now verified in [cook review](M6_SEMIREAL_CAMP_COOK_REVIEW.md), without changing
the169source assets/config or adopting mainmap. This supersedes the NEXTcook
instruction above, not the owner/functionalNav/input/player/performance limits.
