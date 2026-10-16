# M7 rendering regression — C018F

2026-10-08. CHARACTER REGRESSION LOCAL CLOSED; not whole-M7, HUD/UI, packaged-player, temporal-AA or owner approval. Existing C018E performance evidence remains untouched.

## Contract

Actual native D3D12 raster, 1280x720, BGRA8/sRGB. Three views reuse the accepted canonical character fixture: orthographic three-faction/five-pose profiles, actual default tactical camera and actual first-person possession. Existing faction-color pixel assertions remain mandatory. Tests are Editor-only; ScreenshotComparisonTools/RHI dependencies stay in RomanEmpireGameTests, never Game/Shipping.

Each owned PIE world has a public FWorldSceneViewExtension freezing rendering time at zero and disabling motion blur, eye adaptation, temporal AA and AA. This is static geometry/color/composition regression coverage, NOT temporal effects, animation playback quality, full UMG/accessibility or standalone performance. No production CVar, runtime gameplay or source asset changed.

Native FImageComparer: channel tolerance8/8/8/0; brightness0..255; no color/AA ignore, masking, resizing or autoapproval; maximum local difference0.01 and global0.002. Thresholds were fixed before independent comparisons. Missing/corrupt/unreviewed baseline and integrity mismatch fail, not approve. Comparison artifacts/deltas/metrics are new GUID paths. Default logic-only CanonicalPosePIE is not a visual regression gate; the dedicated runner requires actual capture flags, successful native report and comparison receipt.

Reference rendering adapter from the actual candidate log: NVIDIA GeForce RTX5070Ti Laptop GPU, driver592.01/internal32.0.15.9201. Logs retain native adapter/driver data; no Windows/TDR/security settings were changed. This validated reference environment does not establish equivalence across every GPU/driver. Versioned deliberate baseline changes need separate review; do not overwrite/reapprove on failure. Recipes and process handoff are in MODEL_WORKFLOW.md.

## Candidate review

`Artifacts/VisualRegressionRuns/C018F_Candidates1/manifest.json` binds three new native exit0/one-success reports and unchanged PNG/report SHA256 hashes. All three actual PNGs were inspected by GPT, not generated/rescaled by AI:

- CharacterProfiles: all15 figures/five poses visible, Rome rectangular red equipment, Carthage round violet and Gaul round green; dead poses, grounded feet and shadows retained. Color pixels Rome2271/Carthage2082/Gaul1671.
- CharacterRTS: actual tactical camera sees all three rows/five poses at default zoom, no missing army. Native viewport explicitly1280x720 rather than historical variable1017x554.
- CharacterPossession: actual player's sword/hand at lower right, visible opposing equipment, own head does not obstruct the view, no replaced scene-capture screenshot.

Review is agent validation against the already accepted prototype art; user approval and any requested commit are separate. Historical C017G screenshots remain untouched. Candidate success alone DOES NOT pass regression.

Baselines are created only with explicit `review_visual_baselines.ps1 -Reviewed -CandidatePrefix C018F_Candidates1 -ReviewReference M7_RENDERING_REGRESSION_REVIEW.md`. The complete set is preflighted; no existing file/manifest is overwritten. PNGs are copied byte-for-byte under `Tests/VisualBaselines/Win64_D3D12` (outside Content/cook); SHA1 sidecars provide native accidental-integrity checks and SHA256 manifest provides strong artifact identity. PNGs follow existing Git LFS rules. No auto commit.

## Evidence / remaining work

- Editor1 exit0/91actions/92.55s; Editor2 exit0/5actions/8.75s. Initial native ComparisonContract1 exit0/1clean validates identical buffers, one changed pixel under zero tolerance, visible palette corruption, dimension mismatch and unapproved-new-image rejection.
- Candidate three-process cohort exit0/one-success each, device sample-rate/MotionVector warnings retained. No failures suppressed.
- Fifteen pure PowerShell visual-artifact checks pass: root containment, full PNG decode, fixed dimensions, truncated/header-only/non-PNG rejection, platform/engine/count/view/hash checks. Initial header-only negative fixture expected an English .NET message on Spanish Windows; replaced with a stable project error prefix preserving the original exception. Failed attempt retained; no engine/image acceptance relaxed.

Final native file-based negative contract passed: `Artifacts/Automation/C018F-ComparisonContracts2/index.json`, nativeexit0/2success (one clean, one expected missing-file LogStreaming warning). Identical decoded files pass; genuine changed palette fails production limits and measures excess global error; missing approved/corrupt incoming files fail. Synthetic inputs are not rendered baselines. Editor3 exit0/4actions/9.66s.

Two independent comparison cohorts passed: `Artifacts/VisualRegressionRuns/C018F_Compare1/manifest.json`, six new D3D12 processes nativeexit0/one-success each, actual comparison receipts and image/report SHA256 identities. Device/MotionVector warnings remain. Thresholds/baselines unchanged:

| Actual view | Run1 local / global difference | Run2 local / global difference |
|---|---|---|
| CharacterProfiles | 0 / 0 | 0 / 0 |
| CharacterRTS | 0 / 0 | 0 / 0 |
| CharacterPossession | 0.005099826 / 0.000301649 | 0.006727431 / 0.000342882 |

Native baseline SHA1 and full-set SHA256 validation passes after each comparison; PNG LFS attributes verified. First-person rendering retains small interprocess differences rather than demanding byte identity or loosening tolerances. Six comparison child PIDs62640/41720/28892/11528/33572/63972 and batch parent9140 are terminal.

Misuse failure propagation passed: `C018F-FlagMisuse1.log` and `Artifacts/Automation/C018F-FlagMisuse1/index.json`, nativeexit255/one explicit failed test for requesting regression WITHOUT capture. No PIE started; strict report validator rejected the failed report. This intentionally failed invocation is retained, not counted as a successful test.

Repository0M1warnings/LFS/diff/5PSAST checks and77immutable accepted-source+3new baseline hashes pass. Two actual authoring guard probes reject approval without -Reviewed and replacement of the existing manifest; hashes unchanged.

FullTest1 FAILED, retained: nativeexit255/report `Artifacts/Automation/20261008_173225_076/index.json`,135completed,51clean/75warning-success/9fail, none unfinished. All nine failures have ONLY the two WASAPI48000vs44100/device-conversion warnings wrongly elevated to Error in the functional bridge, including its deliberate failure/log-capture variants. Actual functional assertions did not fail. Engine FFunctionalTestBase constructor initializes bElevateLogWarningsToErrors=true before CDO settings can be read; map/PIE startup logs occur before AFunctionalTest::RunTest resets the policy to project/actor settings. Other native tests preserve these messages as warnings.

Fix confined to Editor test bridge constructor: initial protected bElevateLogWarningsToErrors=false, while SuppressLogs staysfalse and errors/warnings remain fully captured. No global settings/device/security/audio/runtime/source maps changed. AFunctionalTest::RunTest still applies explicit/project policy, including OutputIsError; no unconditional override or warning filter. New Functional.LogPolicy contract verifies setup warning severity, errors/warnings unsuppressed and explicit actor elevation still works. Existing actual post-phase Error capture and failure/no-transfer negative tests must pass again. Visual fixture/renderer unchanged: approved comparisons remain valid, not recaptured.

First directed functional rerun `C018F-FunctionalFinal` retained native255/11completed (2clean/1warning-success/8fail). Constructor-only repair passed the first variant, but complex automation reuses the SAME test instance: a previous actor installs the project/explicit elevation policy and the next variant inherits it during its own PIE setup. Final bridge now calls PrepareOwnedSession BEFORE each variant's PIE to reset suppression=false and elevation=false. The actor still applies its own policy afterward, preserving strict actor-level handling. LogPolicy also verifies prior explicit elevation is reset only for the next session, with error/warning capture intact. No global policy/actor asset/audio changes.

Corrected directed `C018F-FunctionalFinal2` nativeexit0/11success (2clean/9WASAPI-warning reports),0failed/notRun/inProcess. Actual post-phase runtime Error capture, failed-actor/no-transfer probes, all six dedicated functional maps, LogPolicy and unchanged-map readback pass. Warnings remain visible. Editor4 exit0/4actions/11.82s and Editor5 exit0/4actions/5.96s. Both historical failed attempts retained; no global/device/security/runtime/map change and no visual reapproval.

Final full suite PASSED: `Artifacts/C018F-FullTestFinal2.log`, nativeexit0/report `Artifacts/Automation/20261008_174507_291/index.json`,136success (52clean/84warning reports),0failed/notRun/inProcess,18exact anchors accepted by the strict runner. The only warning messages are the two WASAPI sample-rate/conversion notices and the synthetic file contract's deliberate missing-PNG LogStreaming warning. No gameplay warning/error suppressed; prior failures preserved.

Strict IWYU PASSED: `Artifacts/C018F-IWYUFinal.log`, nativeexit0/112actions/83.52s. Repository0M1warnings/LFS/diff/5PSASTs,15pure visual artifact checks plus2actual authoring rejection probes and77accepted source+3raster SHA256 identities pass. All owned build/capture/test processes are terminal. Runtime/Game/config/source assets unchanged, so prior Shipping/cook/test-content exclusion evidence is reused; not a fresh trusted-player gate. Final docs-only checks are recorded in HANDOFF.md.

Next C018F: extend real regression coverage to the already-correct native HUD/200% UI fixtures, with independent reviewed cases/immutable versioned family rather than replacing the accepted character set. Reuse these approved gates unless relevant code/fixture/renderer changes; no restart/audit/art generation. M7 commit-reviewed-baselines instruction remains unfulfilled without explicit user commit authorization. Remote runner/governance and trusted-player/release gates remain open.
