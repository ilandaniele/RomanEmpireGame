# M6 — Real semi-realistic source candidate

Latest technical continuation: [real-camera review](M6_SEMIREAL_CAMERA_REVIEW.md),
Runtime05/IntegrationV3. Source semireal_v2 remains byte-unchanged; native private
RTS/FPS rendering now verified, including visible own sword and pose restoration.
No owner art acceptance/main Content adoption/LOD or release gate closure implied.

2026-10-19, GPT, C-018H_SemirealSource1. SOURCE CANDIDATE VERIFIED; not production-approved or imported into the running game.

## Current deliverable — semireal_v2 / attempt10

Versioned source under `SourceArt/Characters/RomanSoldier/semireal_v2`, v1 intact.
Three GLBs49782/12444/4978triangles,17bones/five clips/8slots/3shared2048maps;
four source PNGs, two packed-image editable BLENDs and six exact own recipe files
with manifest hashes. Copy and snapshot SHA256s verified. Model BLEND packed4
images read back in Blender; source code packs before both scene saves.

Adult morph proposal plus real baked closed grip, body-fitted clothing, segmented
harness/waist accessories, arm-free shared torso weights and shoulder sleeves.
Source-art Block counter-rotates wrist to keep shield upright. Walk counter-rotates
shoes and adds bounded visual-only vertical root-bone correction; root motion
remains disabled, no canonical actor/capsule/navigation/combat changes. The actual
reimported pose gallery was inspected: more credible than09's overhead shield and
crossing torso layers. Fine art/cloth/feet/motion quality still needs review.

All3GLB byte inspections PASS. Blender actual reimport/deformation/gallery PASS,
not merely metadata. Actual UE5.8.2 import10-1 exit0:186cm/17bones/shared skeleton,
five correct-duration clips/no root motion/no physics authority/eight slots/three
textures; color-space/compression and serialized graph outputs/maps read back.
Real D3D12/SM6 render10-1 exit0/visibility PASS plus all five imported clips sampled
at0,.25,.5,.75,1 in BOTH Raw and Compressed UE data, finite/nonstatic17bone poses.
Representative compressed max world translation: Idle1.279cm,Walk45.112cm,
Attack59.808cm,Block22.021cm,Death231.488cm. These are actual asset evaluations,
not actor travel or an executed packaged player's animation pipeline.

Latest23 directed Python tests PASS: anatomy/morph/fit7, source motion/torso4,
descriptor dedup3, real GLB mutations/both source families/snapshot9. Repository
0M1warnings/diff--check/LFSattributes PASS. New probe setup PS AST and actual
existing-target rejection PASS with unchanged project hash; versioned template
recreates the disposable probe only on fresh workspaces. No reinstall, main
Editor action, game Source/Config/Content change, baseline update or full suite/
build/cook replay. New files remain unstaged/uncommitted, no push. VM stays off.

Evidence: `Artifacts/VQ01_Semireal1/attempt10`, `UEProbe/import10-1-report.json`,
`UEProbe/render10-1/receipt.json`; all processes through those runs TERMINAL.
Earlier failures and rejected render attempts below are historical, not NEXT.

### Latest runtime continuation — 2026-10-19 21:32 UTC

[Runtime review](M6_SEMIREAL_RUNTIME_REVIEW.md) supersedes the graph-only
checkpoint below. Current private candidate is Runtime03 with IntegrationV2.
V1/Runtime02 had a real missing montage-section omission: stronger tests caught
playing-without-deformation; V2 fixes it without relaxing assertions. Optional
texture-preserving runtime policy defaults false (legacy intact), actual poses/
MIDs verified. Native private141success/0failed/0pending (warnings documented),
strict IWYU/Shipping and main Editor build PASS. No main Content adoption/GPU
camera/player or owner acceptance claim. NEXT C-018H_SemirealCameraPreview.

### Private native game context / graph — 2026-10-19 20:48 UTC (historical V1)

`Artifacts/VQ01_Semireal1/GamePreview` is an actual private copy of the game, not
the minimal art-only probe and not the clean91c evidence checkout. Initial306
files were copied with SHA256 verification. Attempt10_1's already-imported18
assets reused byte-exact, no repeat import. Local descriptor disables Fab/MCP/
toolsets and enables editor Python only in this copy. Native readback01 exit0
proves the game module/original bundle plus distinct candidate rig/five clips/
eight textured slots/three textures coexist. No assets saved by that readback.

New private-only C++ fixture `Tools/ArtProbe/Native/SemirealPreviewAuthoring.cpp`
compiled in the COPY's Editor test module: build01 succeeded114actions/193.97s.
It independently duplicates the verified native-parent AnimBP graph, replaces all
four locomotion/guard/death clip references with new-rig clips, compiles with
zero compiler warnings/errors and creates a new attack montage and bundle under
Attempt10_1/Integration. New montage contains exactly one DefaultSlot/one new
Attack segment and zero damage notifies. It does not reuse the old bind pose.
The original bundle remains untouched; using the legionary in all three preview
bundle fields is only a technical loading fixture, not faction art acceptance.

Directed `RomanEmpireGame.ArtCandidates.SemirealV2Authoring`: Rejected01 failed
exactly for missing opt-in,1error/0warnings/exit255, no Integration output.
Applied01 and fresh-process Readonly01 both pass1/1,0errors/0warnings/exit0.
Read-only output timestamps remain20:45:14UTC; readonly process ended20:46:19.
Initial snapshot306/306 source inputs unchanged AFTER those tests, and zero
preexisting private Content files changed. Private binaries changed as intended
by building; the prebuild snapshot gate is NOT a postbuild binary-equality gate.
Two Python guard tests pass (refuse main project/refuse receipt overwrite), PS
AST/existing-target rejection pass, repository0M1warnings/diff pass. No repeated
full game suite/Shipping/cook, because main Source/Config/Content did not change.

Private generated assets SHA256:

- ABP_SemirealPreview: `50a85fd2424e50ca9a05860a54891367920ce73306373bd880100e9b284d328c`
- AM_SemirealAttack: `628ae9df2c5de3b4911e3cd42cf80a2322d979fd9f5be7ec3cbd4c41e6b31504`
- DA_SemirealPreview: `97edf191015a8f8b9541ec3aa20676766ab7fccd8ae98094334ff416bfb04337`
- Native fixture: `a327c5bc45249ce1fc1331443bd2c9d66d42675bacf7f5d2c6879f8edaac907c`

Evidence: GamePreview/preview-copy-receipt.json, game-context-readback01.json,
native-preview-build01.log, authoring-{rejected,applied,readonly}01.log,
Reports/Authoring{Rejected,Applied,Readonly}01/index.json. All owned processes
terminal; owner Editor27532 not operated/closed. No game asset adoption/PIE or
camera/player/owner acceptance claim. The actor still replaces slots0/4/5 with
flat faction MIs; preserve textured PBR during real faction/damage refreshes in
the next runtime-binding block, not by removing overrides only in a screenshot.

NEXT C-018H_SemirealCameraPreview within GamePreview: controlled preview using the
actual RTS/FPS cameras and new textured-material/clip/AnimBP pipeline. Do not
silently replace main assets, reuse old flat overrides, edit a live owner PIE,
change canonical authorities or refresh old screenshot baselines to pass. Owner
visual acceptance, installed LOD transitions/performance, packaged QA and release
gates remain OPEN. VQ02 camp sample is an independent pending art task.

## Completed

- Owner confirmed training and enemy attacks in the Editor, then requested autonomous continuation while asleep. No AI patch made for the unconfirmed navigation hypothesis.
- Blender5.2.2LTS portable, official ZIP SHA256/foundation Authenticode signature verified, no Windows installation. Bootstrap receipt: `Artifacts/Toolchains/Blender5.2.2.bootstrap.json`.
- Pinned MakeHuman CC0 anatomical OBJ and full licenses retained under `SourceArt/ThirdParty/MakeHuman/a8bc2d54`; no upstream application code. Exact source-specific license header and rights separation inspected; provenance in ASSET_INVENTORY.md.
- Original `Tools/semireal_legionary_mesh.py`: body-only topology, UVs, source hash, natural1.82m scale, helper-derived17 semantic joints and normalized skin weights. Four directed source-contract tests pass; these are not Unreal tests.
- Original Blender author `Tools/create_semireal_legionary.py`: clothed anatomical geometry, mail/helmet/equipment, skin,17bone rig,five30fps clips, native baked PBR textures, actual skinned GLB, editable BLEND and three source-art renders. It writes only a new attempt under Artifacts and rejects an existing output path.
- Original `Tools/validate_semireal_glb.py` inspects exported bytes: finite actual mesh/UV/normal/skin arrays, valid indices/normalized weights, actual bind matrices/17bone hierarchy, all five moving sampled clips with correct durations, embedded PNG data and PBR bindings.

## Attempt 01 — preserved

`Artifacts/VQ01_Semireal1/attempt01`, stdout/stderr `Artifacts/VQ01_Semireal1/attempt01.log`. OwnerPS17612/session18742 TERMINAL exit0. No Blender process pending at final observation.

- GLB SHA256 `6cef4881579b381fd063a87d7fc1700a15a4fb3a5b1e7c9d0752e4254b723223`,4429756bytes,48048triangles,36689exportedvertices including material/UV splits,17actualjoints,1.847m overall source height.
- Actual exported Idle2s/Walk1s/Attack0.6s/Block0.8s/Death1s,51samplers each; embedded1024px BaseColor/Normal/packedMetallicRoughness,8materialslots. Byte inspection PASS in `glb-inspection.json`; source-manifest is additional author metadata, not its replacement.
- Three renders completed. Three-quarter image inspected: materially more human anatomy than the old prototypes, but eyes sit too deep, helmet intersects forehead and mail shoulders are too broad. Hand/weapon grip and footwear also need refinement. NOT accepted as final art.
- Blender5.2 warns Material.use_nodes is deprecated for6.0. Export warns about multiple image samplers when packing roughness/metallic; no warning was hidden. Embedded maps/bindings inspected, but engine import and final visual quality still unproven.

## Remaining

### Current checkpoint — selected draft06 (supersedes attempt01 next steps)

Update after that checkpoint: native GPU render07 PASS exit0 under unchanged
visibility thresholds (Back .12083 bright/.04831red/.002567white; Front
.10504/.06203/.001908). Both images inspected; strong studio contrast, not owner
visual acceptance or RTS/FPS cameras. Actual imported five clips evaluate17bones.
GLB-only-UV0 controlled variant changes NO binary/image/geometry/animation bytes,
imports and renders08 PASS. Actual DECODED pixel buffers are BYTE-IDENTICAL to07
for both views, despite PNG metadata/hash differences: Front SHA256
c99b5daeff5bf3abf3733f6d1c57433c18fce71649ae4848beb3b02e593a5174,
Back2fa5346aaeb7ac4bf6f692965a97bc067361a1ba1ce0a2322090550782b6284c.
Thus unusedUV1 is NOT the cause of observed detail/lighting differences. Stop
investigating that hypothesis; do not introduce a runtime/engine UV fix.

Generated capture/test-receipt copies were moved out of SourceArt to recoverable
`Artifacts/VQ01_Semireal1/SourceBundleEvidenceCopies`, originals retained. Versioned
source keeps GLB/texture/packedBLEND/README/author-manifest only; local preview links
point into Artifacts. No art data lost or existing game assets replaced.

Next concrete quality step is draft07: optional `roman_adult_v1` using three pinned,
individually CC0 numerical morph targets, and narrower torso exclusion to preserve
axillary/upper-arm skin in raised-arm poses. Anatomy tests6PASS (including parser
negative cases/profile topology/UV/scale/helpers); author running ownerPS37948,
session14419, `Artifacts/VQ01_Semireal1/attempt07.log`. Observe same process before
retry; do not overwrite v1/06. Candidate must be inspected/validated, no quality
acceptance yet. Open grip remains a separate real modelling task. Morph provenance
and artistic coefficients in ASSET_INVENTORY.md, full licenses retained.

07 then completed exit0/all3GLB bytePASS (49744/12435/4973triangles). Three-quarter
review confirms more adult face, but larger shoulders clip above old clothing:
NOT accepted. 08 completed exit0, fit garment vertices against a BVH of actual
full anatomical data (12mm wool/24mm mail), retains axillary skin. Shoulder
coverage improved; harness/belt then occluded by the properly fitted coat.
Seven pure anatomy/morph/fit tests PASS. 08+ captures original recipe bytes at
startup under each attempt's `recipe/` and hashes them in manifest, preserving
reproducibility across subsequent code changes. Both BLENDs now pack source maps
at author time, not only publication time.

09 running PS35384/session74607/logattempt09.log at checkpoint: fitted belt/apron,
subdivided draped harness and optional real baked source grip. New
`Tools/pose_semireal_grip.py` uses anatomical finger helpers and TEMPORARY FK bones,
blended wrist rotation/finger curl; applies deformation to source mesh and removes
that rig/groups. Main exported rig remains17bones; weapon center derives from
actual posed helpers, not a bitmap. Still must inspect actual result and run
GLB/reimport/engine checks; a `--closed-grip` flag does NOT prove a good grip.

All author/probe/render processes through render06-6 are TERMINAL. No pending
Blender/UE commandlet at 2026-10-19 15:48 UTC. The user's separate Editor was not
closed or edited. No game Source/Config/Content changes, VM/security/remotes,
stage/commit/push or broad gameplay build/test replay.

Versioned draft: `SourceArt/Characters/RomanSoldier/semireal_v1`. Three real GLBs,
four source PNG maps, two editable BLENDs (four images actually packed in BOTH),
three source-art views and actual reimported-GLB five-pose gallery, plus receipts.
Copy hashes verified. Failed unpacked BLEND retained recoverably under
`Artifacts/VQ01_Semireal1/source06-pack-failed.blend`; pack2 completed exit0.
Old source-art source-manifest records author-time state; later Unreal receipt
does not silently rewrite that historical metadata.

| LOD | Actual triangles | GLB SHA256 |
| --- | ---: | --- |
| 0 | 48256 | fc8974cfc97e5ab21a2af333125b810520165612caa5b059c55666eac5b31dad |
| 1 | 12063 | 5d82f34791508d3c888c00e0756f27734248ac6e9ea6522d00de5e5c41271e7a |
| 2 | 4825 | 3aa39b0fbda38cbe17c0437b682f7de5d4d382f0b03db438a8507b4a2436fb85 |

Directed checks: 15/15 Python (anatomy4, texture-descriptor cleanup3, actual GLB
mutation/readback8), py_compile, repository validation0M1warnings, diff--check and
new binary LFS attribute checks PASS. No claim LFS fsck materialized uncommitted
new files into its object store. Whole gameplay suites/build/cook are unchanged.

Actual GLB data inspection: 17 joints, five moving correct-duration clips, eight
materials, three embedded2048maps,1.86m. Descriptor dedup preserves binary/image
bytes and reduced actual engine imported textures24→3, not merely manifest count.
Actual Blender reimport/deformation/gallery PASS: max movement0.0197mIdle to
2.55mDeath across five sampled phases, bounded geometry; this is NOT animation
quality acceptance. Gallery inspected: grip open and raised-arm axilla/cloth
intersections still need real art work. Eyes/helmet/wraps improved, not final.

Actual UE5.8.2 isolated import `UEProbe/import06-1-report.json` PASS exit0:
186cm/17bones/five clips/shared skeleton/no root motion/no physics asset/eight
slots/three atlas textures/18assets. Private namespace ONLY
`/Game/ArtCandidates/Attempt06_1`. Readonly graph readback4 additionally PASS:
BaseColor sRGB, Normal linear/TC_NORMALMAP, ORM linear; all shared maps bound.
glTF MaterialFunctionCall has all texture inputs shared across outputs; report
records separate BaseColor/Normal/Roughness/Metallic output pins. This is not a
claim that a backwards graph walk isolates each function output's internal code.

#### Preserved failures / limits that govern next work

- Attempts02/05 completed; retain old files.03 native stderr/PowerShell stop
  wrapper failure;04 empty sparse foot slice; nearest-surface band corrected.
- Reimport probes01/02 initially counted Blender's generated Icosphere bone
  custom shape as a second mesh. Exclude ONLY actual pose-bone referenced shapes;
  reimport03/04 then passed; no imported mesh discarded to pass.
- Source package first pass skipped lazily loaded images. Pack all IMAGE types,
  verify actual packed_file, preserve failed copy; pack2 passed.
- Unreal readonly1 used deprecated/render-resource GetUsedTextures under NullRHI.
  Readonly2/3 wrongly required exclusive texture sets per shared-function output.
  Correct serialized graph traversal/output-pin recording passes readonly4.
- GPU render1 produced nearly black files despite structural script PASS; reject
  visually. Render2 failed commandlet argument parsing (`\\r` in Windows path);
  use FORWARD SLASH script paths when passing Python arguments. Render3 overexposed
  despite visibility-only PASS; reject visually. Render4/5 rejected by white clip
  check; native UE5.8 source proves raw PF_B8G8R8A8 readback FLinearColor values are
  BYTE-RANGE0–255, not normalized0–1. Correcting sensor units is not pixel editing
  or weakening thresholds. Render6 still fails unchanged10% bright-area threshold
  (Back .08430 bright/.04992red/.001668white). No accepted UE beauty preview yet.
- Original SM5 stage compiled stock Niagara FXC error paths; use isolated DX12/SM6
  config/flag (not game/engine settings). SM6 keeps host TSR driver<610 warning;
  do not suppress or change Windows/driver security.
- Actual UE animation data evaluation succeeded for all5clips/17bones in GPU
  attempts1/3; does not establish runtime AnimBP/selection/damage/FPS integration.
- Source vs engine material-detail parity is not yet demonstrated. GLB carries
  two UV arrays; textures reference UV0, while some legacy UV1 primitives are
  constant. This is a lead to inspect, NOT a diagnosed engine/export bug. Keep
  candidate versioned and do not replace game assets to hide this uncertainty.

Next: verify material/UV/lighting parity in a new isolated attempt, preserve
receipts; then close grip/underarm/cloth issues and evaluate real RTS/FPS cameras
before owner art acceptance. Source draft is not broader M6/M7/goal completion.

1. Improve identified visible defects in a new attempt; retain01 as evidence, do not overwrite it.
2. Re-run source contracts/GLB inspection and inspect actual source renders; add readable RTS LOD candidates without silently relaxing existing contracts.
3. Publish selected candidate source as versioned LFS files, preserving original game assets. Blender renders do NOT substitute for Unreal rendering or real player-camera review.
4. Validate import/bind/material/animation compatibility in an isolated owned engine context; do not edit assets in the user's running PIE session or close their Editor.
5. Owner approves actual visual result after waking; historical period remains provisional. No blanket licensing/distribution/performance clearance.

Current game Content, gameplay, saves, input and actors remain unchanged. VM remains off. No M7 external gate, full goal or M8 completion claim.
