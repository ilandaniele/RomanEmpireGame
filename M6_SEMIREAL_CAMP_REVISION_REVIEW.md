# VQ-02 — architecture and materials revision02

Owner response22/oct: «Cambiar ambas cosas». This authorizes a new private source
revision of architecture AND materials, not mainmap adoption, historical approval,
release, contracts or the unrelated M7 security/QA gates.

## Delivered source and exact evidence

`SourceArt/Environment/RomanCamp/semireal_v2` contains14files: editable BLEND,
four actual GLBs, four PBR atlas PNGs, reused soil input, two author previews,
manifest and README. Thirteen generated files were copied create-only from
`Artifacts/VQ02_CampRevision2/Attempt03` and independently hash-compared;74,189,080
bytes before the README. v1 sources and Camp01 were not replaced.

Source manifest SHA256:
`aa9364411a1de6c413041ce3075c2c0a0b3457f4d8260de43d4f0a41a9164546`.
`Artifacts/VQ02_CampRevision2/Readback02/readback.json` SHA256:
`68f44f23958eacc5a332b8966418e601fb3ad4c58bf6d28efd21e2c60815575e`.
Both reference actual file/tool/geometry identities, not an approval by the owner.

Final `SourceDelivery01.json` SHA256
`b21b4c07975998becc828e8976e082abe9a6860d7c20ffda0a3b3a28bd5c95f8`
binds all14published file identities, protected/current tool and source hashes,
positive native receipts and expected negative rejection. CloseSource01 exited0.
It records the observed14-test command/result, not a new native gameplay suite.

Geometry totals32,986triangles: ground1,152/wall3,704/barracks28,130. The former
sample had51,458; lower source triangle count does NOT prove higher runtime FPS.
Ground remains12×10m; barrack wall footprint6.2×3.6m, whole bounds including
porch/roof/step6.686×5.73×3.815m. Common world-origin pivots/metres retained.
One atlas UV set; six material types; three2048embedded PNGs per GLB. No rig,
animations or reference-character mesh in the exported camp. No authored LODs,
collision/nav/interactive-door/gameplay change or historical authenticity claim.

Architecture: longer barrack, real facade openings/closed shutters, double-door
frontage, covered porch/posts/knee braces, stone courses and long-axis tile roof.
Materials: broader earth variation with worn tracks, controlled timber/clay
variation, localized plaster weathering. The existing generated1254soil input is
byte-identical; no new image generation/download/pack/purchase or tool install.
Blender5.2.2 portable and original construction/shaders/bake were used locally.
Character reference provenance and owner distribution review remain unchanged.

## Diagnosis retained, not hidden by new baselines

- Author01 generated source but the PS5.1 wrapper lost the native exit code
  (`null`); wrapper exited1. It is NOT a verified native success. Snapshot of
  author/recipe retained in Author01Sources; artifacts are historical.
- Author02 completed native0 but its roof showed black strips. Separating tile
  overlaps by6mm was insufficient. Readback01 found exported COLOR_0, potentially
  multiplying variation already in basecolor. Python raised despite Blender's
  default native0; receipt absent/stderr nonempty means FAIL, not a passing test.
- Creator now sets `export_vertex_color=NONE`. Runner uses
  `--python-exit-code73`, forces process-handle retention before wait, and requires
  native0 + required receipt + empty stderr. No native0-only approval.
- RoofDiagnostic01 disconnected only clay normals IN MEMORY: strips remained.
  RoofDiagnostic02 white emission confirmed the affected surface was clay.
  No source BLEND was saved by diagnostics or image edited to conceal the issue.
- RoofMaterialStats01/RoofUvStats01 found637/4410sampled clay faces with unbaked
  roughness0/albedo0; a sampled UV width was~0.00028×2048=0.59texel. Original
  UV gutters consumed space needed by thin faces; not a normal-greenflip issue.
- v2 uses UV margin0.0005 and2pixel bake padding instead of v1's0.004/6 defaults,
  retaining atlas2048. Shared `kit.bake` received optional parameters; original
  defaults are unchanged and tested, original helper snapshot in OriginalKit.
  This is source-level coverage repair, not a tolerance change or new baseline.

Author03 native0/required receipt/stderr0; Readback02 native0/required receipt/
stderr0. Both final reimport-render PNGs were actually inspected: clay coverage
repaired, long porch/roof and worn earth visible with the existing legionary.
No camera/lighting change was used to hide the strips. Mip/seam behaviour in
Unreal remains an explicit integration check for the tighter atlas padding.

## Verification scope

- Eight pure recipe/defaults tests pass: genuine facade openings/area coverage,
  sample bounds, roof layout/surface offsets, terrain/path bounds, preserved v1
  helper defaults. Six synthetic GLB-envelope guards pass: no external URI,
  invalid bounds/dimensions, rig/animation, double tint, wrong topology/UV/count.
  Synthetic fixtures do NOT prove Blender image/geometry decoding.
- Repository validation passes with0openM1warnings; PowerShell runner AST and
  git diff check pass (pre-existing unrelated CRLF warnings retained). LFSfilter
  policy confirmed for the new GLB/BLEND/PNG; no files staged or committed.
- Readback02 independently reimports ALL4real GLBs, verifies triangle counts,
  metre dimensions, one UV set/finite positions, intended mesh names/extras,
  no rig/reference leak/COLOR_0, three2048PNG payloads and real used materials.
- In the saved BLEND, all4410tested clay-face centroids(area≥0.006m²) now have
  roughness0.8313726186752319. That is coverage of this sampled set, NOT every
  texel, tiny face, mip, material, LOD or GPU result. Two real reimport renders
  further support appearance, not runtime performance or owner acceptance.
- Source files/current tool hashes and14v1 source hashes are checked before/
  after readback. Published source copies rechecked before delivery.
- NegativeChangedTool01 intentionally reuses obsolete Attempt02 against changed
  tool identities: native73, no receipt,816stderr bytes, wrapper fails. It verifies
  that a Python failure cannot masquerade as native0 success. Inputs untouched.

All own Blender processes are terminal; do not relaunch historical attempts or
repeat captures simply on model switch. Logs/PID/exit receipts use corresponding
Author03/Readback02/NegativeChangedTool01 names under VQ02_CampRevision2.

## Next implementation, not another source audit

Claude integration: extend existing importer to select the versioned source
explicitly; it currently defaults to v1. Use fresh `/Game/ArtCandidates/Camp02`
in the EXISTING GamePreview; hash-protect the prior169Content assets including
Camp01. Do not recreate GamePreview or overwrite Camp01/main Content.
Reuse lossless GLB descriptor dedup as needed, verify unchanged BIN/image bytes,
actual StaticMesh/PBR/units/bounds and current camera readback guards. Source
BLENDER previews do not replace actual RTS/FPS/possession screenshots.

No Unreal MCP tools were available; the skill's discovery/safety rules prevented
pretending a live connection or installing services. Work used local Blender
authoring; no live Editor/cook/build operation was performed. Do not assert UE
integration, normal import convention, functional collision/nav, environmental
performance, packaged player, mainmap or owner approval from this source proof.
G-VQ02_CampRevision02 source block finished; the whole implementation goal,
M7/M8/security/QA/rights/adoption gates are NOT finished.

## Subsequent private Unreal integration — C-018H_CampRevision02Preview

The preceding source-only statements describe G-VQ02 delivery, not the current
integration state. The existing GamePreview now contains NEW
`/Game/ArtCandidates/Camp02`:3StaticMesh/6Material/3Texture2D,12new assets.
Camp01/mainContent/sourceart were not replaced. Source v2's immutable manifest
still describes its authoring-time state; it is not a mutable integration ledger.

Tools/import_semireal_camp.py and its runner now explicitly select semireal_v1/v2
with defaultv1 retained. New pure source policy pins both manifest hashes and
checks3GLB hashes/geometry profiles before any import. Namespace metadata records
source revision/SHA; wrong-profile readback fails rather than silently inheriting
v1. Legacy Camp01's absent new tags are accepted only for that known v1 namespace;
it is not rewritten. Eight targeted policy tests pass (additional to source14).

Native import camp02-author01 completed0, protected169previous Content FILES;
read-only reopen camp02-readonly01 completed0 and protected181files. Their path
sets differ by exactly12Camp02 uassets. For precision:181files means171uasset,
8umap and2existing designJSON, not181Unreal assets. All179Unreal assets plus the
twoJSON are hashed. Initial cohort assembly incorrectly assumed181uasset/umap;
its failed assumption/snapshot is preserved, fixed by exact full-path equality,
not by rerunning imports or relaxing input hashes.

Native saved triangles1152/3704/28130; dimensions cm ground1200×1000×2.17644,
wall925×37.75×259.9696, barrack668.600006×573×381.4999999.1LOD/1UV,3shared2048
textures, actual sRGB/normal-compression/green-flip recorded; bound PBR materials
remain owned, no legacy tint/MID. Native mesh test also confirms zero GENERATED
simple collision. This does not mean no complex collision metadata exists:
preview components explicitly set NoCollision/Navfalse; no functional nav claim.

Private native fixture now supports Camp01(default)/Camp02 with explicit counts;
invalid/traversal/unknown names and invalid indices are rejected. Added native
assertions execute inside existing Readback test. Source pose/camera/hidden-witness
guards are unchanged. No new runtime dependency/module/Build.cs/rootCpp change.
Private strict nonUnity/noPCH build:4actions,18.82s,exit0. Only the owned private
test fixture was deployed; original fixture snapshot is NativeFixtureCamp01.cpp.

Camp02GPU01: D3D12/SM6, actual1280×720 game viewport, native0,2testsSuccess,
0errors/failed/pending;1known r.MotionVectorSimulation render-thread warning.
It is NOT a globally warning-free engine run. The7PNGs were all inspected:
RTSAll, hiddenGround/Wall/Barracks, FPSAll, FPShiddenCamp, RTSReturn. No imagegen,
new baseline, FOV trick or substituted SceneCapture was used. Actual controller/
RTS pawn/legionary possession and exit run through production functions.

Hidden witness pixel counts at identical cameras: ground36156,wall4205,
barrack26453,FPS840399. FPS cameraZ165.904 and held actor position are guarded.
The portrait hand/weapon belongs to the unchanged character candidate, not a
new camp mesh. The blue backdrop is the isolated fixture, not the final map.

Camp02PoseMutation01 uses SAME fixture/DLL with pose hold deliberately omitted:
ReadbackSuccess, GPUFail1, one exact expected error (actor falls/moves), native255;
wrapper validates the expected failure and181Content hashes remain identical.
This catches the former false-positive class; it does not prove physical input,
free-running combat, collision/nav, standalone player or performance.

`Artifacts/VQ02_CampRevision2/private-camp02-cohort01.json`, SHA256
`d3013ece695d6a40346da9d8469c8ac3770e2407210224c0d7a979bd7c1962aa`,
binds reports/process/exit/source/DLL identities,7PNG hashes,169original/181current
file protection and each source/staged GLB BIN chunk. Geometry/accessor/image
binary payloads are identical after dedup; only duplicate JSON texture descriptors
were changed. ClosePreview01 succeeded with actual saved/current hashes.

Author/import/read-only/build/GPU/mutation processes are terminal. Do not repeat
them solely on model switch. Capture/import logs and exit receipts are under
VQ02_CampRevision2; actual PNGs/reports under GamePreview/Artifacts/Camp02GPU01
and GamePreview/Reports/{Camp02GPU01,Camp02PoseMutation01}.

Subsequent directed private Camp02 cook/registry/shader/dependency verification
is now complete in fresh `Artifacts/VQ02_CampRevision2/Cook/Camp02Cook01`.
Native0/0errors/0warnings,537cooked;12assets,35hard-Game identifiers with0missing,
6SM6 material mappings and actual SM5+SM6 archives.181Content/14source files and
Config/descriptor remain intact;23pure tests PASS. [Exact cook evidence and
limits](M6_SEMIREAL_CAMP_COOK_REVIEW.md#camp02--semireal_v2--current-revised-private-cook).
NEXT owner decision on this actual revised sample, not another automatic cook.
Owner review of these new actual RTS/FPS views is still needed before adoption.
Mainmap/functional geometry/nav/inputs/environment FPS/trusted player/security/
rights/M7/M8 remain OPEN. No new rights or migration/Windows changes authorized.

Read-only fit preflight subsequently completed; [native data/geometry findings](M6_SEMIREAL_CAMP_FIT_REVIEW.md).
Natural6.686x5.73m bounds exceed actual saved4x4 footprint. Proposed7x6, pivot/
grounding/scale/material/state/spawn work are NOT applied or approved. This changes
the integration recipe, not the owner review still pending or prior source/cook gates.

## Owner decision — 2026-10-30

The owner answered the ACTUAL Camp02 preview question: «Pedir más ajustes visuales».
This supersedes the pending-review language above: no functional adoption of v2.
Roman semi-realism and architecture+materials remain agreed; the requested changes
to THIS result are not yet specified. NEXT G-VQ02_CampRevision03Brief / GPT:
obtain concrete notes/annotated capture/reference and acceptance criteria before
authoring new source. Preserve Camp02 and its evidence, do not recapture/rebuild/
recook unchanged v2 or assume 7x6 gameplay footprint approval. Future v3 needs its
own pertinent source/import/render/fit/cook gates after the brief is defined.
