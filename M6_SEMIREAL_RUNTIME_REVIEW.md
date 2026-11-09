# M6 — Semi-realistic runtime bindings (private candidate)

Latest continuation: [camera review](M6_SEMIREAL_CAMERA_REVIEW.md). Current private
candidate is Runtime05/IntegrationV3, superseding the INITIAL Runtime03/V2 records
below. Brass stays untinted; same-body local-player reach fixes measured FPS hand
clipping without FOV/scale/collision changes. Actual RTS/FPS/head/IK restoration,
latest full141(140clean/1intentional warning), Shipping and main Editor pass.
Older source hashes/tests below describe their historical revision, not latest.

2026-10-19, GPT, C-018H_SemirealRuntimeBindings. Current candidate:
`/Game/ArtCandidates/Attempt10_1/Runtime03/DA_RuntimePreview` in
`Artifacts/VQ01_Semireal1/GamePreview` ONLY. Not adopted in main Content, not
owner-approved art and not rendered-camera/player evidence.

## Implementation

- `URomanCharacterPresentation::bUseMeshMaterialParents` defaults FALSE, retaining
  the original schema1 faction-material policy with no asset resave/migration.
- Opt-in bundles keep each skeletal slot's authored PBR parent. Actor-owned MIDs
  receive canonical faction Color/FactionTintStrength and committed health Damage.
  Semantic Body/Shield/Crest slots are tinted; skin/metal/leather/mail/eyes retain
  their original color. Shared mesh/material/texture assets are never mutated by
  faction, selection or damage events. No loads, scans, callbacks/notifies owning
  combat, new collision/nav authority, save IDs/version or module dependency.
- Native MaterialPolicyPIE uses an unsaved config duplicate and two real units.
  It verifies exact parent/actor-local MID ownership, semantic colors, health,
  selection stability, isolation, actual Idle/Walk/Block/Attack/Death deformation,
  held guard/final death pose and GC. Animation states advance on separate frames.
- Main Source changes are limited to the presentation header, UnitBase material
  branch and new Editor test. Main Binaries/Content/Config/Editor session remain
  untouched; builds/tests use exact copies of these sources in the private game.

## Real failures preserved / correction

Runtime01 material authoring failed on the first Desaturation pin name. Installed
MaterialEditingLibrary.cpp resolves an empty pin name to GetInput(0); the C++
field name Input is not that exposed input name. Fixed without ignoring errors.
Partial namespace/report retained. Runtime02 material graph checks passed.

Candidate01 passed the initial weaker pose check. Candidate02 and03 then failed
the stronger actual sword-arm deformation assertion. Splitting frames alone did
NOT fix it. The new IntegrationV1 montage had a segment/slot but no composite
section. Installed AnimMontage.cpp2162–2165/2604–2608 rejects an invalid section
and stops advancement; the original schema1 author explicitly adds Attack0.
This was a real new-author omission, not a gameplay/AI regression or a proven
engine bug. A playing flag/asset load was insufficient evidence.

IntegrationV2 is a NEW namespace: playable Attack section at zero, canonical
.035/.08s blends, own new-rig clips, one DefaultSlot, one segment, no damage
notifies. Runtime03 uses V2; V1/Runtime02 remain rejected playback evidence.
Test requires playable sections, positive timeline advancement AND real arm
deformation. No thresholds/baselines were relaxed. IWYU03's additional invalid
GetSectionStartTime API call was corrected to installed GetSectionStartAndEndTime;
failed build log preserved. Backup old author/test sources are under GamePreview.

## Verified evidence

- Private Editor builds runtime01/02 PASS; strict non-unity/no-PCH IWYU01
  114actions/108.90s PASS. Final IWYU04 incremental13.49s PASS after fixture fixes.
- AuthoringV2Applied01: native1/1,0errors/0warnings,exit0. Verifies compiled graph,
  new montage section/range/blends and bundle; all old outputs preserved.
- runtime-materials03.json: native author/readback PASS,10assets (eight new
  materials, mesh and bundle), original3textures shared and four PBR property
  texture bindings retained. This graph receipt is NOT GPU shader/quality proof.
- MaterialPolicyBase01: native1/1 PASS. Final MaterialPolicyCandidate04:
  Characters4/4 PASS,0errors/0warnings,exit0, including legacy originals and actual
  new-rig poses/timeline/deformation with runtime materials.
- RuntimeFull01: native141 successful,0failed/0pending,exit0.55clean and86with
  warnings:85 occurrences EACH of the two WASAPI48k/44.1k notices, two engine
  HTTP connectivity-probe timeouts to google.com/generate_204, one intentionally
  missing.png visual-contract warning. No Windows/audio/network settings changed.
  Encounter50/100 tests still use original default art, NOT new-model FPS evidence.
- Python private guard tests:5 PASS. Repository/diff and source-integrity checks
  PASS. Main/private header/UnitBase/test byte-exact; original Content/Config and
  preexisting private Content unchanged. No full cook/player/visual-baseline claim.
- Shipping private01:78actions/80.76s,exit0. Fresh runtime-materials03-readonly.json
  PASS/read_only=true/10assets/exit0. No source/asset mutation in that readback.
- Main Editor runtime01:11actions/41.28s PASS after Get-Process/CIM confirmed no
  Editor was running. No owner session was closed/operated/reopened. Generated
  main binaries now reflect the compatible code change; main art references and
  Content remain original. The private141-test result is NOT a separate main
  suite run; the private-only authoring fixture does not enter the main module.

Logs and native reports are under GamePreview: native-runtime-{build*,iwyu*}.log,
authoring-v2-applied01.log, runtime-materials03.{json,log},
material-policy-{base01,candidate01..04}.log, runtime-full01.log;
Reports/AuthoringV2Applied01,MaterialPolicyCandidate04,RuntimeFull01/index.json.
Shipping log runtime-shipping01.log; final material readback
runtime-materials03-readonly.{json,log}; main build main-editor-runtime01.log.
All owned processes terminal. Old prebuild snapshot hashes must not be used to
claim equality of generated binaries changed intentionally by these builds.

Source SHA256 at final tested runtime revision:

- RomanCharacterPresentation.h: `978a12bbea891ca0cba2c069e35986f96cf67c8d06d1b11e605fced1de5e3e89`
- UnitBase.cpp: `9dda0c2373b3b6c4b4f93a185a99f04d9b626d233c60b049eb4de6304923a815`
- CharacterMaterialPolicyTests.cpp: `bd167141e317b8ab9b39bb9844ac80e63c5ab561280a2381cd5af0bac668813b`

## Next / open gates

Actual private D3D12/SM6 shader/material rendering and production RTS/FPS camera
preview, scale/facing/handedness, controller possession/head hiding/restoration,
selection/camera screenshots and owner art review remain OPEN. Do not fake these
with an orthographic asset-only render, flat overrides removed only for a photo,
or a NullRHI result. Main art adoption, installed LOD transitions/new-model100-unit
performance, packaged QA, security/vendor holds, isolated CI and M7/M8 remain open.
VQ02 camp sample is independent pending work. Preserve the clean91c evidence.
