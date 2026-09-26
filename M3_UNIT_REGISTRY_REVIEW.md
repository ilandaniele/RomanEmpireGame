# M3.2b live unit registry review

Status: live-unit registry implemented and verified, including packaged-player regression (2026-09-26). This closes neither the squad contract nor all of M3.2.

## Contract and integration

- `URomanUnitRegistrySubsystem` exists only in Game/PIE worlds. Weak actor keys and derived faction buckets do not own units or replace their canonical faction/lifecycle state. Deinitialize clears both indexes.
- Units register after successful initialization and BeginPlay. Deferred/failed/ended actors are excluded; death and EndPlay remove records idempotently. Ownership reindexes before external notifications, including nested transfer, death and destruction callbacks.
- Idle acquisition queries enemy buckets with the existing strict 5000-unit 2D radius, command-readiness and non-None enemy rules. Equal-distance ties use lexical actor names. Hold still prevents acquisition.
- Rectangle selection queries the canonical player's bucket, projects real actor positions and routes through normal controller selection eligibility. Bounds normalize in either direction; nonfinite input is rejected. Missing mouse coordinates no longer read uninitialized values.
- Removed the two production world enumerations from idle unit acquisition and rectangle selection. This is not spatial indexing: repeated all-enemy queries can still be quadratic. No performance gate is claimed.
- No new runtime dependency, plugin, asset, input mapping, network service or engine/security setting. `RenderCore` is an Editor-test-only dependency for the projection fixture.

## Regression evidence

`RomanEmpireGame.Units.LiveRegistryPIE` covers deferred/failed initialization, idempotency, EndPlay, immediate death removal, real weak-reference collection, None-faction indexing, nearest-enemy radius/ties, real idle/Hold behavior, snapshot independence, owner-event ordering/reentry, world isolation and subsystem teardown. It also invokes the real controller rectangle command with actual Unreal projection, checking own/enemy selection, reversed bounds and nonfinite input.

The commandlet viewport measured 0 × 0. A transient test-only `FSceneViewport` supplies 1920 × 1080 dimensions when needed; the actual camera, spring arm and engine projection remain in use and are restored. This proves the rectangle command, not physical mouse dragging or rendered rectangle feedback.

- Initial and expanded suites: 23/23 pass, no result warnings (`Artifacts/Automation/20260921_170203_183`, `20260921_170346_203`).
- Projection-ready suite: 23/23 pass (`20260922_125432_179`): 15 succeeded, 8 succeeded with warnings. The 16 events are host audio 48000/44100 Hz mismatch/conversion notices; no host settings were changed.
- Negative control: `Artifacts/M3_Registry-Lifecycle-Mutation-Tests.log`, report `20260922_125548_762`: 22 pass, `LiveRegistryPIE` fails. Publishing ownership before reindexing breaks canonical-bucket observer assertions; omitting death removal breaks immediate record-count and death-observer assertions. Both temporary mutations were restored on 2026-09-26 before final verification.
- Historical fixture failures are retained: rectangle projection without dimensions, then the viewport pointer type and missing RenderCore link dependency. They were fixed in the test harness, not hidden or bypassed.

## Restored final verification (2026-09-26)

- Targeted `LiveRegistryPIE`: 1/1, zero result warnings/errors/pending, exit 0. Report: `Artifacts/Automation/M3_Registry_Restored_Targeted`; logs: `M3_Registry-Restored-Build.log`, `M3_Registry-Restored-Targeted.log` under `Artifacts`.
- Final full regression after the targeted pass: 23/23, zero result warnings/failures/pending, exit 0. Report: `Artifacts/Automation/20260926_203959_234`; `Artifacts/M3_Registry-Final-Tests.log`. Full coverage is justified by lifecycle/controller integration. The prior host audio notices did not recur; settings were not changed. Startup engine self-test/layout messages are outside the project test results; this is not a claim that every raw log line is warning-free.
- Strict non-unity/no-PCH IWYU passed all 60 actions (`Artifacts/M3_Registry-IWYU.log`); Shipping passed (`Artifacts/M3_Registry-Shipping.log`). Repository validation and Git LFS fsck passed. A bounded independent review found no concrete lifecycle/correctness defect.

## Packaged-player regression

`BuildProject.ps1 -Action Package -DisableFab -ArchiveDirectory Artifacts/Packages/M3_Registry` passed UAT with exit 0. Cook processed 566 packages: 559 cooked and 7 platform skips, no cook warning/error lines. Log: `Artifacts/M3_Registry-Package.log`.

The actual Development player SHA256 is `a4bcce2cdda660d1e183a073a7d5c1fd8b9648072b5aafb5e1d25ec8e51b074d`. The existing helper was used unchanged with this archive explicitly selected:

```powershell
.\Tools\verify_unit_presentation.ps1 -ArchiveDirectory Artifacts/Packages/M3_Registry -OutputDirectory Artifacts/Registry-Explicit
.\Tools\verify_unit_presentation.ps1 -ArchiveDirectory Artifacts/Packages/M3_Registry -OutputDirectory Artifacts/Registry-Loss -LoseOwnership
```

- Explicit return: PID 58552, exit 0, strict selected/possessed Pawn identity, restored camera and cleared selection/flags checks pass. Driver log: `Artifacts/M3_Registry-Explicit.log`.
- Ownership-loss return: PID 54996, exit 0, same checks plus the exact formerly selected actor becoming Carthage; no explicit return/clear command. Driver log: `Artifacts/M3_Registry-Loss.log`.
- Each output directory contains `Player.log`, `Selected.png`, `Possessed.png`, `Returned.png`. All six 1920 × 1080 captures were inspected: selected green ring and unit HUD, first-person view and possession HUD, then tactical camera and cleared selection. Ownership loss also shows the purple Carthage formation.
- Both player logs contain exactly the two known rendering warnings: NVIDIA pre-610 TSR 16-bit workaround and `r.MotionVectorSimulation` render-thread flag. No error/fatal/assert/ensure signatures. No drivers, console defaults or security settings changed.

These are real rendered possession regressions, not physical-input, packaged rectangle-drag, Shipping execution or performance proof.

## Remaining scope

Squad identity/membership and save semantics need a separate contract. Additive selection, invalid-target feedback and building ownership selection remain M3.2 work. Routed policy remains M3.1 work. Navigation, shared combat improvements, measured 50/100-unit performance and the full vertical slice remain later gates. Registry queries do not add diplomacy, visibility, path reachability or acquisition scheduling.

Skills applied: `unreal-cpp-gameplay`, `ue-testing-debugging`, `ue-module-build-system`, `unreal-packaging`, `ue-project-context`.
