# Semireal private LOD review

2026-10-20, 13:33 UTC. C-018H_PrivateLODCheck: import/static readback, animated
skinned deformation perLOD and sampled real GPU transitions verified privately.
Performance and owner visual acceptance remain OPEN. Not a continuous video gate.
Goal/M7/M8 and owner art acceptance are NOT complete.

## Scope and lineage

Project: `Artifacts/VQ01_Semireal1/GamePreview`. New mesh/bundle:
`/Game/ArtCandidates/Attempt10_1/LOD01/SK_LODPreview` and `DA_LODPreview`.
Derived from Runtime05; preserves IntegrationV3 graph/montage,17-bone own skeleton,
eight Runtime05 PBR parents/shared textures, Body/Shield tint, no physics asset.
All three faction references still use the SAME technical legionary: not three
approved designs. Runtime05 remains the verified CAMERA candidate, LOD01 the new
LOD test candidate. No main Content/Config/baselines, source GLB/PNG/BLEND, runtime
gameplay or immutable clean91c checkout changed. No owner Editor interrupted.

`Tools/author_semireal_lods.py` is create-only and exact-project/reparse/path
guarded. It pins all3 source SHA256s, refuses existing namespaces/reports, uses
native SkeletalMeshEditorSubsystem.ImportLOD and asserts actual counts after
each import. Installed source confirms GLB routes through synchronous Interchange
custom-LOD import, not legacy FBX-only fallback:

- Engine/Source/Editor/SkeletalMeshEditor/Private/SkeletalMeshEditorSubsystem.cpp,
  ImportLOD, approximately858–934.
- Engine/Source/Editor/UnrealEd/Private/FbxMeshUtils.cpp, ImportSkeletalMeshLOD.
- Engine/Source/Runtime/Interchange/Engine/Private/InterchangeMeshUtilities.cpp,
  InternalImportCustomLod, approximately179–321.
- Engine/Plugins/Interchange/Runtime/Source/Pipelines/Private/
  InterchangeGenericMeshPipeline.cpp, custom-LOD context avoids physics creation.

## Evidence

Paths below relative to the private project:

- `lod-author01.json/.log`: PASS/exit0; imports yield actual LOD counts2/3.
  Existing155 Content files hash-identical. Only two new assets. Four retained
  Interchange warnings: non-root skinned mesh and automatic LOD name-pattern
  mismatch, once per source. Import used explicit LOD indices.
- `lod-readonly01.json/.log`: fresh process PASS/exit0. All157 Content files,
  including LOD01, hash-identical before/after; no asset saves.
- `Reports/LODReadback03/index.json`: `RomanPreview.LOD.Readback`1/1clean,
  failed/pending0, exit0. Actual data in `native-lod03.txt`. Every vertex checked
  for finite positions/normals/UVs, valid bones and normalized16-bit weights
  (65535±1), max4 influences; all indices/ranges/sections/material mappings checked.
  Also checks17-bone names/hierarchy/bind transforms against Runtime05, shared
  parents, no physics, rest bounds, source filenames and screen sizes.

| LOD | Native triangles | Vertices | Sections / UV channels | Screen size |
| --- | ---: | ---: | --- | ---: |
| 0 | 49758 | 38756 | 8 / 2 | 1.0 |
| 1 | 12420 | 12165 | 8 / 2 | 0.5 |
| 2 | 4954 | 6000 | 8 / 2 | 0.25 |

Rest bounds differ by less than0.2cm per axis (test limit5cm). Source GLBs retain
49782/12444/4978 triangles. Independent source-byte analysis finds exactly24
coincident-corner faces within2e-7 metres per LOD, all brass slotCrest;16 also
near-zero-area at the recorded threshold. Source reports:
`source-topology01.json`, `source-topology_LOD101.json`, `source-topology_LOD201.json`.
Native assertions reconcile exact counts for EACH of eight materials, not a loose
total tolerance. Source mesh bytes unchanged.

Source SHA256:

- LOD0 `3157ac079a631d6fa2a317d550b0b3aeb9a56be9d752d423c478f7d50e0211f6`
- LOD1 `99e5d203ba969c17704148dcce886c03ad781d194bcf6ddfe60ddd8e2e44d1c0`
- LOD2 `6c61948ebe48db34494dfdb6f71a4994d15940b101166a33c422b5ddacad1110`

## Failures preserved, not overwritten

- Build01: PowerShell split `-Project=` caused rejection BEFORE compilation.
  Fixed with one interpolated argument.
- Build02: missing direct Skeleton include and wrong GetExecutionInfo signature.
  Fixed. Strict no-unity/no-PCH builds03/04/05 PASS4actions each in
  10.76/10.54/10.50seconds. Final tool/private source copy match.
- Native01 FAILED3checks: expected raw source faces instead of retained imported
  triangles. Independent source analysis proved24-face difference; subsequent
  test checks exact per-material counts.
- Native02 FAILED3checks: incorrectly expected bRemoveDegenerates=true. Actual
  setting FALSE for all3LODs, preserved from Runtime05, ThresholdPosition0.00002cm.
  Installed GLTFMeshFactory.cpp around687–695/751–759 skips repeated vertex IDs;
  SkeletalMeshTools.cpp around183–190 only emits distinct final chunk indices.
  These checks are independent of the later flag. Exact stage for every face
  was not instrumented; do not claim that flag caused removal. Native03 corrects
  the expectation to preserve the original flag without editing mesh/settings.
- UE returned exit0 even for tests01/02: Automation JSON is authoritative.

## Verification and changed tools

New `Tools/author_semireal_lods.py`,
`Tools/ArtProbe/Native/SemirealLODReadback.cpp` (private Editor module only).
`validate_semireal_glb.py` reports source topology by primitive/material; no mesh
editing or relaxed existing contracts. Its tests10/10PASS, including actual3LOD
face reconciliation and existing mutations. Preview guards9/9PASS, including
main project/namespace/report escape rejection and immutable receipt checks.
Preview creator includes the new fixture for FUTURE snapshots; NOT rerun here.
Python/PS AST, repository validation0M1warnings, git diff check PASS (unchanged
line-ending advisories). No full141-test rerun or main/Shipping rebuild warranted:
no runtime source or adopted main assets changed in this block.

## Motion and GPU verification — 2026-10-20

New private Editor fixtures `Tools/ArtProbe/Native/SemirealLODAnimation.cpp` and
`SemirealLODGPU.cpp`; no game-runtime changes or dependencies. Preview creator
copies them for FUTURE snapshots; not rerun here. Same existing LOD01 assets,
IntegrationV3, matrices, PBR parents and canonical soldier/controller commands.

`Reports/LODAnimation02/index.json`:1/1clean,0failed/pending,exit0, strengthened
over initial passing01. `lod-animation02.txt` records engine
USkinnedMeshComponent::ComputeSkinnedPositions against actual renderLOD buffers,
real skin palettes/weights and evaluated canonical AnimBP. One real actor perLOD,
forced animationLOD0/1/2, separate real frames, finite deformed positions for all
38756/12165/6000 vertices. At least25 regional vertices must move: spine Idle
>.1cm; thigh Walk, shield forearm Block and sword forearm Attack each >10cm;
root Death >10cm. Idlemax~1.8cm,Walk~25.48,Block~28.63–28.70,
Attack~151.33–152.34,Death~246.90. Block/finalDeath holddelta0. Actor ticks are
parked for controlled pose sampling; this is NOT physical-input/combat-timing QA.

`Reports/LODAnimationMutation01/index.json`:EXPECTED1failed/3errors, one
Attack-deformation failure perLOD. Explicit private process-only
`-RomanLODAnimationSuppressAttack` immediately cancels the montage on transient
actors, without editing any asset. Residual breathing moves sword-region vertices
at most~.52cm,0vertices exceed10cm; regional deformation assertions catch this.
Normal02 passes with the SAME source/binary and without the flag. No persistent
mutation or restoration edit. Receipt `lod-animation-mutation01.txt` retained.

`Reports/LODGPU03/index.json`:1/1successWithWarnings,0errors/failed/pending,exit0.
The sole selected-test warning is the retained known engine
r.MotionVectorSimulation render-thread warning, not suppressed. Actual D3D12
adapter0 is NVIDIA GeForce RTX5070Ti Laptop GPU; Automation's device metadata may
name the integrated GPU, so chosen-adapter engine log is authoritative. Real SM6
game viewport1280x720, no SceneCapture or replacement camera/mesh actors.
`Artifacts/LODGPU03/gpu-lod-receipt.txt` reads GetLOD on the RENDER thread after
real frames, with predicted/forced values and actual camera pose recorded:

| Production camera stage | Zoom | Forced override | Actual renderLOD |
| --- | ---: | ---: | ---: |
| Default RTS | .55 | 0 (automatic) | 2 |
| Intermediate RTS | .6625 | 0 (automatic) | 1 |
| Closest user-controlled RTS | .70 | 0 (automatic) | 0 |
| Actual local possession | canonical FPS | 0 (automatic) | 0 |
| Same FPS pose, forced variants | unchanged | 2 / 3 / 1 | 1 / 2 / 0 |
| Exit to same RTS pawn | .55 | 0 (automatic) | 2 |

Four own-body-vs-hidden witnesses change29950/29733/29327/29874 pixels (>200).
All nine PNGs inspected: AutoFarRTS, AutoMidRTS, AutoNearRTS, AutoFPS0,
ForcedFPS1/2/0, HiddenFPS, AutoReturnRTS. Body/weapons remain visible, PBR brass
retained; detailed LOD0 selected automatically in FPS. Forced LOD2 close-up is
visibly more faceted, as expected; not an approved first-person art substitution.
Exit restores same pawn/head and clears local FPS IK targets, returns autoLOD2.
No LOD thresholds, camera/FOV policy, actor scale, collision, nav or main assets
changed. Screenshots sample transitions/continuity; not an uninterrupted sweep,
full Slate/HUD capture, owner art approval or packaged-player evidence.

Failures preserved: GPU01 expected LOD1 at maximum zoom, but real production
camera selects0; also incorrectly required a render object while mesh hidden
(legitimately absent). GPU02 intermediate .625 still selected2; all other stages
passed. GPU03 uses .6625 inside the real LOD1 range, no threshold edits. Native
render feedback is recorded, not inferred from a forced flag or draw count.
GPU builds01/02 corrected const MeshObject and TObjectPtr assertion signatures;
strict no-unity/no-PCH builds03/04/05 pass17.85/17.61/13.90s. Animation builds01/02
pass25.00/14.19s. Logs retain all failures; no whole141-test/main/Shipping rerun
because only authoring/private fixtures changed. GPU03 ran before strengthening
the independent animation fixture; GPU source unchanged afterward.

`lod-dynamic-content-before01.json` plus final157/157/0delta SHA comparison prove
private Content unchanged through GPU/mutation checks. An initial PS comparator
mistakenly nested ConvertFrom-Json's array and raised a false count alarm; direct
array iteration corrected it, not asset repair. Dynamic source/receipt/PNG hashes
in `lod-dynamic-cohort01.json`; `lod-cohort01.json` is the older static snapshot.

## Next proof

C-018H_PrivateGPUPerformance: reuse LOD01/IntegrationV3. D-05 actually targets
60FPS at1920x1080 with50 slice units plus a SEPARATE100-unit stretch test, not a
newly invented mandatory60FPS100-unit gate. Measure actual new-model units, real
simulation/rendering and declared hardware/settings. Old original-art NullRHI
benchmarks and these1280x720 one-body captures do not prove that performance gate.
Reference-hardware acceptance, owner quality, uninterrupted visual polish and
trusted Development/Shipping player QA/main adoption remain distinct. Native
existing Encounter50/100 tests are in UnitFoundationTests.cpp; inspect targeted
sections before extending privately, don't rerun an unchanged original-art test
and claim new-art GPU coverage. VQ02 camp sample remains independent asset work.
