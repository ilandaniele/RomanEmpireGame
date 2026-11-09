# C-018H — Building presentation adapter

2026-10-30. Runtime implementation, targeted/full native tests and compiler gates
verified. Not asset adoption, rendered approval or player QA.

## Contract

`FBuildingData` now owns optional `PresentationMesh` and `PresentationTransform`.
The mesh is a reflected hard reference resident with its loaded definition; there
is no per-frame asset loading. Existing definitions default to null and retain
their engine cube, `PrototypeScale`, materials and existing selection layout.
No DataAsset, map, footprint, cost, production/spawn rule, stable ID or save
authority was changed to enable art.

`RomanEmpireBuildingPresentation::Validate` inspects the actual static mesh bounds
after the authored actor-local transform. It rejects invalid/non-positive scales,
empty geometry, non-finite/invalid footprints, XY overhang beyond 0.1 cm tolerance
and ground error over 1 cm. Ground is the existing placement-box base, local
Z=-200 cm. Both definition-set validation and actor initialization enforce it.
This is bounds validation, not proof of a clear doorway, spawn area or navigable
interior; a definition can never expand its footprint implicitly.

For an opt-in actor:

- The authored transform replaces, rather than multiplies, `PrototypeScale`.
  Construction retains its pivot/translation, including completion after an
  oversize timestep; the initial-completion path also retains the translation.
- All authored PBR material slots remain untouched by placement, construction,
  selection, damage, repair and faction feedback. A separate thin footprint plate
  reuses the existing state materials; selection outlines remain on the ground.
- Art and feedback geometry are non-colliding/non-navigating. The existing
  definition-owned box remains the placement/collision authority. BeginPlay-only
  factories and deferred placement receive the same opt-in box dimensions.
- Mesh references remain GC-visible on the definition, copied actor data and
  components. No new gameplay subsystem or test-only runtime subclass exists.

Definitions are immutable after actor initialization; live replacement of an
initialized actor's definition/mesh is not an authored feature of this adapter.

## Evidence

Local create-only outputs: `Artifacts/C018H_BuildingPresentation1/`.

| Gate | Result / scope |
| --- | --- |
| Root Editor Build04 | Native 0; 7 actions, 8.93 s. Final source after negative-test additions. |
| Test02 | PID41904, native 0; 2 Success, 0 failed/not-run/in-process, 0 test errors/warnings; stderr 0. |
| Protected files | Test02 before/after hashes: root Content, Config and project descriptor unchanged. |
| Strict IWYU | Native 0; 115 actions, 598.24 s, stderr 0; `IWYU01/`. |
| Shipping | Native 0; 10 actions, 76.40 s, stderr 0; `Shipping01/`. Player NOT executed. |
| Regression | Native 0, stderr 0; 142 passed (141 clean + 1 with warning), 0 failed/not-run/in-process; `Regression01/`. |
| Closure | `verified01.json`: seven adapter-source fingerprints; protected root Content/Config/descriptor still unchanged after the full suite; all jobs terminal. |
| Repository | Validator: 0 open M1 warnings; Git LFS fsck and diff whitespace check passed. |

Full report: `Artifacts/Automation/20261030_141712_395/index.json`. Both new
building tests passed again without warnings. The sole suite warning is the
deliberately missing `missing.png` in
`RomanEmpireGame.VisualRegression.FileComparisonContract` (source
`VisualRegression.cpp:192` asserts that a missing approved image cannot pass).
It is recorded, not hidden or described as a warning-free suite. The full run
includes its normal incremental Editor prebuild, native 0 / 71.45 s.

`RomanEmpireGame.BuildingPresentation.Policy` verifies null fallback, a valid
rotated/offset mesh, overhang, floating/buried geometry, negative/zero scale,
non-finite footprint and rejection of a Camp03-sized envelope inside 4x4.

`RomanEmpireGame.BuildingPresentation.LifecyclePIE` uses the real canonical
Barracks factory and BeginPlay in an owned PIE world. It temporarily changes the
in-memory definition only, restoring it before teardown. It verifies legacy
scale, actual mesh/transform and collision bounds, all PBR slots through the
lifecycle, feedback state/selection/faction/damage, repair, GC and destruction.
It validates the complete canonical definition set with a valid opt-in, then
requires exactly the Barracks invalid-definition issue after moving the mesh
outside its footprint. The actual factory must reject that same definition,
install no authored mesh and disable ticking. Its one precise expected native
error (`has no valid content definition`) is asserted, not an ignored global
error. These two tests run with NullRHI: no appearance/navigation/player proof.

Earlier attempts are retained: Build01 failed preflight for a private test
include; Build02 failed on two new test API/type errors. Neither changed module
boundaries or validator policy. Build03/Test01 then passed; Build04/Test02 include
the additional registry/factory rejection and confirmed-placement assertions.

## Remaining integration

The actual Camp03 mesh has not been exercised through this adapter. Existing
Camp03 GPU/cook evidence remains valid for its frozen private art fixture, not
for this new runtime path. No equivalent claim is made here.

Camp03's natural 6.686x5.73 m roof/porch envelope cannot fit the current 4x4 m
Barracks footprint. A minimum 7x6 proposal also requires separate doorway/spawn,
placement, selection, navigation and production verification; it is not activated
or approved by passing the bounds policy. Do not silently shrink the art, expand
the footprint, move the spawn or adopt it in the main map. The alternative is a
deliberately redesigned compact asset with natural proportions.

Direct native initial-completion coverage is now verified below. Next: stage
actual-asset runtime verification under an explicit fit/activation decision.
Do not repeat closed gates solely for a model switch.
M7 security, trusted Development/Shipping QA, CI/rights and M8 gates stay open.

## C-018H_InitialPresentationCoverage — verified

`RomanEmpireGame.BuildingPresentation.InitialCampPIE` loads the real building
catalog through AssetManager and substitutes a transient two-slot mesh in memory
before starting `/Game/Maps/L_VerticalSlice`. Strong references and a shared RAII
fixture protect the pre-PIE interval and restore the definition on failure/teardown.
The test never calls `CompleteInitialConstruction` through a private header or a
test-only production hook: the real GameMode starter-camp factory invokes it.

The sole GameMode-owned Barracks must be Complete at its canonical spawn, retain
the exact authored transform, canonical collision footprint and all PBR slots.
After restoring the catalog and dropping the fixture's mesh reference, the actual
actor must retain it through GC. `TrainUnit`/`TryCancelTraining` must produce the
real receipt, debit all six canonical resource fields once and refund the exact
original wallet without changing art slots. These are logical/PIE assertions,
not actual Camp03 geometry, rendered, doorway/navigation or player evidence.

Additional evidence in `Artifacts/C018H_BuildingPresentation1/`:

| Gate | Result |
| --- | --- |
| Build05 | Native 0, wrapper PID52272, 5.99 s; test-only additions. |
| Test03 | PID53512, native 0; all 3 building tests Success, 0 test errors/warnings; stderr 0. |
| Controlled mutation | Build06 native 0, PID2964, 4.85 s: only initial completion resets mesh translation to zero. `Mutation01.json` binds source snapshot and native DLL identities. |
| Test04 | PID47340, native 255; exactly InitialCampPIE fails its authored-transform assertion, other 2 tests pass; no extra test errors/warnings. Protected files unchanged. This is an expected negative result, not an approved build. |
| Restoration | Source restored **before** Test04; Build07 PID50796, native 0, 4.71 s restores the runtime DLL afterward. `Restored01.json` binds restored native identities. |
| Test05 | PID43480, native 0; all 3 tests clean Success, stderr 0; original Content/Config/descriptor unchanged. |
| Closure | `verified02.json` checks exact mutation failure, restored DLL identities, all six runtime source hashes byte-identical to prior `verified01.json`, unchanged protected files and no live native jobs. Repository validator 0 M1 warnings; diff check passed. |

Only `BuildingPresentationTests.cpp` changed in code for this block. No persistent
runtime, map, definition, asset, footprint/spawn rule or packaging change. No full
suite/IWYU/Shipping rerun: runtime is byte-identical to the previously verified
version and all tests in the affected family passed again. The earlier full-suite
count remains 142, not 143; the new initial-camp test has targeted native evidence.

Fit choice asked without changing rules: retain 4x4 and author compact architecture,
or approve 7x6 plus separately verified spawn/navigation changes. Awaiting a choice
does not reopen art direction or require repeating Camp03 source/import/cook work.

## C-018H_EditorPresentationValidation — verified

Closed a genuine authoring gap: `URomanBuildingDefinition::IsDataValid` originally
accepted unsafe presentations although registry/actor initialization rejected them.
It now calls the same `RomanEmpireBuildingPresentation::Validate` policy and emits
one actionable validation diagnostic. No duplicated dimensions/tolerances. Both
the include and call are inside existing `WITH_EDITOR` guards; no Game/Shipping
behaviour, property, content, footprint or spawn rule changed.

Evidence remains under `Artifacts/C018H_BuildingPresentation1/`:

- Build08 native 0 / 5.60 s; Test06 PID40272, native 255, reproduces exactly eight
  assertion failures: overhang/floating/mirrored/empty geometry were each accepted
  with zero diagnostics. Null/fitting/null-restored baselines already passed.
- Build09 native 0 / 13.21 s adds the production editor hook. Test07 PID28160,
  native 0: all four building-presentation tests pass with no test errors/warnings.
- Test08 PID52252, native 0: three existing content-definition tests pass, covering
  discovery/cross-validation of the 19 canonical definitions. These tests do NOT
  themselves exercise each building's individual virtual validation hook.
- Therefore the new editor test additionally validates all six saved building
  DataAssets individually. Build10 native 0 / 5.63 s; Test09 PID46168, native 0:
  that final focused test passes, including all seven transient validity cases
  and six saved assets with no validation errors or native test warnings.
- `verified03.json` asserts the precise original failure, successful fixed runs,
  protected root Content/Config/descriptor hashes, unchanged original six adapter
  runtime files and terminal native jobs; fingerprints the updated test, definition
  source and final Editor DLLs. `RomanContentDefinitions.cpp` is the only additional
  production file, and only its editor-only validation changed.

Earlier native cohorts remain immutable historical evidence. No full-suite,
strict IWYU, Shipping, cook, GPU or player rerun was needed for this editor-only
hook. Do not relabel the earlier 142-test full suite as a current 144-test run.
Current full implementation/release and actual Camp03 adoption are still unproven.
