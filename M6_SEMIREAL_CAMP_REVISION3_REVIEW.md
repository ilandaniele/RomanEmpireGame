# Camp03 — source delivery, private delegated visual iteration

Status: REAL source produced/reimported in Blender AND privately imported/rendered
in Unreal. NOT owner-approved final art, gameplay integration or release proof.
User said «segui la tarea no importa» after asking for more visual adjustments:
agent-selected refinements are authorized, not adoption of rejected Camp02.

## Visible changes

- Oxide-red base/cornice and gable motif distinguish the military barrack from
  the previous rural-looking block. Facade paint respects real door/window holes.
- Porch rack with actual curved backed shields, bosses, spear shafts/heads and
  forged hinge fasteners. No sprites, pictures of models or new interactive items.
- Fine gravel contribution reduced from32percent to7percent, broad earth wear
  retained; wood/clay palettes have narrower contrast. Atlas bakes the variation:
  COLOR_0 is not exported, avoiding a second vertex tint in glTF.
- Barrack/ground dimensions and common layout unchanged; palisade seed produces
  millimetre-scale tip-height variation, not a new gameplay footprint. No camera,
  game logic, actor placement, main Content/Config or security/VM changes.

## Delivered source

`SourceArt/Environment/RomanCamp/semireal_v3` contains14files: editable packed
BLEND, four GLBs (three individuals plus combined proposal), four2048PNG PBR maps,
unchanged previous soil input, two actual AUTHOR PNGs, manifest and README.
The reference legionary appears only in BLEND/renders, not environment GLBs.

| Mesh | Triangles | Metre bounds |
|---|---:|---|
| SM_CampGround | 1152 | 12 × 10 × .0217644 |
| SM_CampWall | 3704 | 9.25 × .3775 × 2.59644 |
| SM_CampBarracks | 30554 | 6.686 × 5.73 × 3.815 |

Total35,410 under65,000, six atlas material slots, one exported UV channel,
three embedded2048PNG images per GLB. No LOD, rig, animation, nav, gameplay
collision or interactive doors/gear. A smaller wall footprint6.2×3.6 does not
describe the full roof/porch envelope; no7×6 gameplay proposal was applied.

All28previous v1/v2 source files hash-identical. Published13generated files are
byte-identical to final Attempt02; README is14th. Git LFS filters confirmed for
GLB/BLEND/PNG. No stage/commit/push or new download/install/external art/font.
Geometry/shaders are original project Blender construction; soil input retains
previous GPT provenance. Historical period and overall distribution rights remain
unapproved; character reference retains existing provenance/licensing obligations.

## Actual evidence

Under `Artifacts/VQ02_CampRevision3`:

- Author01 PID52504 and Readback01 PID1808: terminal0/receipttrue/stderr0.
  Initial source exported/reimported; author views exposed a small spear socket
  gap. Preserved Attempt01/Readback01 and exact author/recipe in Author01Sources.
- Author02 PID7208: terminal0/receipttrue/stderr0. Real source geometry corrected
  so shaft top1.855m overlaps head shoulder1.85m; not an image paint-over.
- Readback02 PID2696: terminal0/receipttrue/stderr0. Actual four GLB imports,
  mesh names, counts, finite geometry, metre bounds, UV count, PBR envelope,
 3embedded images, no armature/animation/reference-character leak, scope metadata.
- Roof coverage in the actual saved BLEND:4410clay-face centroids(area≥.006m²),
  roughness min=max.8313726186752319. Sampling does NOT prove every texel, small
  face, mip, engine GPU or final visual approval.
- BOTH final author PNGs AND BOTH independent reimport PNGs actually inspected.
  `Readback02/Camp-Overview.png` and `Camp-HumanScale.png` are Blender renders,
  NOT actual Unreal RTS/FPS, player or performance evidence.

Final source manifest SHA256:
`b1403c940209438f18a7addd6c4afbb7e2f725471f68e66a1e2c49f82f8e83e5`

Readback02/readback.json SHA256:
`c6617cb27f37c98ced1019e66b04787bf412f4cc524b7a4c9cece4005d505d26`

SourceDelivery01.json SHA256:
`7dff2ba59a493f9d55285d31c9cafa0ea13b3261dfd36a2357f3b7262aab5e31`

Create-only PublishSource01.py binds final source/readback, native PID/exit/script
identities and actual copied bytes. All four DCC processes terminal; no handle
or job remains to wait/restart. Legacy source and Camp01/02 evidence are preserved.

## Tool changes and tests

New create_semireal_camp_v3.py / semireal_camp_revision3_recipe.py / recipe tests.
Existing source runner adds explicit `-Revision 3`, default2 preserved; existing
source verifier selects schema/root/proposal name2or3 explicitly, default2.
No changes to original modelling pipeline or v2 author/recipe were required.

20pure tests PASS:6v3 recipe (including socket overlap/paint avoiding holes),
6GLB-envelope guards,8legacy recipe/defaults. Pythoncompile/PowerShell AST and
repository0M1warnings/diffcheck PASS. Existing line-ending warnings remain.
No UE full suite, build/cook or recapture justified for source-only changes.

## Next: private Camp03 import and real engine views

This source-stage recipe below was completed in the subsequent block; its new
actual native evidence is recorded in the next section. Do not repeat on switch.

Continue into the existing `Artifacts/VQ01_Semireal1/GamePreview`, NOT a new clone.
Extend the explicit source/import/native-preview profiles to semireal_v3/Camp03,
using the manifest SHA and actual1152/3704/30554 counts above. Preserve all181
existing Content FILES and default Camp01/v1/Camp02/v2 behaviours. Expected new
namespace is `/Game/ArtCandidates/Camp03`, not rewriting existing assets.

Import/create-only plus reopen, same canonical real RTS/possess/return cameras and
pose/visibility witnesses. Do not change camera/lighting or use these Blender
images as a substitute. New content cook/fit/gameplay gates remain distinct.
The v2 fit review remains useful context, not proof for new native assets or
authorization to enlarge footprints/change spawn. Do not ask for the old missing
brief again: the owner explicitly delegated this iteration. M7/M8/rights/security/
trusted-player/CI/performance remain open; no release approval is inferred.

## Private Camp03 import and actual engine views — completed

Explicit pinned semireal_v3/Camp03 profiles added, defaultv1 and Camp01/02 retained.
Source selection rejects v2 schema/counts as v3 and prevents namespace/revision
mixing for Camp03 before import.10pure policy tests PASS, Pythoncompile/2PSAST PASS.
MCP meta-tools unavailable; native commandlets were used in the existing private
GamePreview without installing/reconnecting services or changing live owner Editor.

`camp03-author01` PID24912 native0:12new assets (3StaticMesh/6Material/3Texture2D)
under `/Game/ArtCandidates/Camp03`,181existing Content FILES hash-identical.
`camp03-readonly01` PID50908 native0:193current Content FILES hash-identical.
Those193 mean191uasset/umap and2existing designJSON files, NOT193Unreal assets.
No rewrite of Camp01/02/main Content/Config/source art. Shared2K textures and saved
source metadata/manifest SHA match; actual triangle counts1152/3704/30554,1UV/1LOD.
Native barrack size668.600006×573×381.5cm, wall925×37.75×259.644257cm.

Native fixture adds only explicit Camp03 support/count assertions; camera, FOV,
pose hold, visibility and lifecycle guards unchanged. Old private fixture copied
to NativeFixtureCamp02.cpp before sync. Strict private Editor build wrapper47440
native0/4actions/18.61s. No runtimeRootCpp/Build.cs/Config/module change.

`Camp03GPU01` PID46596 native0:2testsSuccess/0errors, one known
`r.MotionVectorSimulation` render-thread warning. This is not a warning-free
engine claim. Actual D3D12/SM6,1280×720 game viewport, canonical RTS pawn/real
legionary EnterFPS/Exit. ALL7PNGs inspected: all/hidden ground/wall/barrack,
FPSall/hidden camp, RTSreturn. Not Blender substitutes or SceneCapture/FOV tricks.
Oxide gable is visible in RTS; rack/paint/fasteners appear in FPS. Hand/weapon
remain the existing unchanged character candidate; blue horizon is the fixture.

At identical cameras hidden-pixel witnesses: ground35824,wall4058,barrack25809,
FPS840823. RTS camera438073.968/440000/2587.110, pitch−53.333332; FPS camera
439827.101/440316.999/165.904, pitch−3/yaw−55. These are held-pose art diagnostics,
NOT physical-input, free-running combat, functional collision/nav or performance.

`Camp03PoseMutation01` PID4636 native255:ReadbackSuccess/GPUFail, exactly one
expected error for falling/moving owned source actor, validated by the runner.
Same source/DLL as positive;193Content files unchanged. No code restoration needed:
only the process-local SkipPoseHold switch differs. All native jobs terminal.

`Artifacts/VQ02_CampRevision3/private-camp03-cohort01.json` SHA256
`b5f8b27a22052716eecf279e76e12dbb2869326cfbd65c0d281a722ddda7ff3a`
binds source, staged GLB binary chunks (geometry/images unchanged by JSON descriptor
dedup), native reports/process/exit/current source/DLL,193file set and7PNG hashes.
Reports in GamePreview/Reports/{Camp03GPU01,Camp03PoseMutation01}; PNGs and receipt
in GamePreview/Artifacts/Camp03GPU01. Native journals/logs under revision3 root.

Repository validation0M1warnings/diffcheck PASS; existing line-ending warnings
remain. No full suite/Shipping build or player launch warranted for this test-only
profile change. Gameplay and DataAsset authorities preserved per UE skills.

NEXT C-018H_CampRevision03Cook: fresh actual Camp03/v3 directed cook/registry/
shader/dependency proof,193Content+14source protected. Extend profiles with old
defaults intact; do not reuse cook2 for new assets, alter engine/Zen config or
reinterpret an art preview as adoption/player/nav/perf/rights/M7 approval.

Subsequent C-018H_CampRevision03Cook VERIFIED in its own fresh output: native0,
0errors/0warnings,537cooked/7platform-skipped;12actual assets/35hard-Game identifiers/
0missing/6SM6material associations and actual SM5+SM6 archives.193Content and14source
files/Config/descriptor intact.17verifier tests PASS. [Current cook evidence](M6_SEMIREAL_CAMP_COOK_REVIEW.md#camp03--semireal_v3--current-private-cook).
NEXT optional building presentation adapter in existing authorities, legacy default
preserved, first private verification; no automatic mainmap/footprint/spawn adoption.
