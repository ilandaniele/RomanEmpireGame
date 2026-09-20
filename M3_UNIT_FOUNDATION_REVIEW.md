# M3.1 unit foundation — in progress

Started 2026-09-15 from M2 commit `641351c`. This is a progress review, not closure of M3.1 or the full implementation goal.

## Implemented foundations

- The sixteen constructor-created component references remain reflected `TObjectPtr` owners; fourteen erroneous null assignments were removed. No replacement art or procedural-limb expansion was introduced.
- Selection controls both visibility and hidden-in-game and reuses its dynamic material. Possession uses the retained real camera component.
- The exported plain `FUnitVitalState` owns live health/stamina/morale and the initialization/death/end lifecycle. It has no actor references, ticks or delegates. Actor commands remain the world/event boundary; initialization commits definition, faction and vitals atomically before deferred spawning finishes. BeginPlay does not reset already initialized vitals.
- Zero health is terminal. Dead, failed, uninitialized and ended units cannot accept gameplay actions; successfully initialized deferred units also wait for BeginPlay. Cleanup commands remain legal.
- Before `OnUnitDied`, the actor cancels actions, orders, blocking, selection, possession, movement, pending forces and collision, and schedules its engine lifespan. The default corpse deadline is three seconds, independent of world altitude. EndPlay clears lifespan and stops actions without manufacturing a combat-death event.
- `ALegionary` follows the same lifecycle and stamina boundary. Existing pilum/testudo are guarded and cleaned up, not promoted to completed slice features. Attack targets are weak references and an attack command no longer destroys its own target while setting its movement destination.

## Verification

Initial Editor compilation and suite passed: `Artifacts/M3_UnitFoundation-Editor.log`, `Artifacts/M3_UnitFoundation-Tests.log`, report `Artifacts/Automation/20260915_175405_910`. That run executed 18 tests, all successful; five PIE tests contained host WASAPI resampling warnings (engine 48 kHz, device 44.1 kHz). No device/driver/security settings were changed and warnings were not suppressed. This run predates subsequent coverage and rounding fixes; final evidence must be recorded separately.

Independent review found a genuine floating-point boundary bug in stamina regeneration: clamping an increment in float could still overflow through double rounding. Regeneration now adds/clamps in double before conversion; a dedicated extreme-value regression was added.

The expanded actor fixture compares full definition/faction/vitals snapshots across rejected and repeated initialization and BeginPlay, checks all sixteen component identities/attachments before and after GC, selection material reuse, actual camera activation/normal exit, existing order cancellation, terminal reentrant commands, finite/extreme damage, altitude-independent corpse timers, early destruction and nested vitals observers that kill/destroy during dodge. Delegate signatures and payload identities are checked. Post-Destroy assertions inspect retained reflected storage before GC: Unreal intentionally refuses `ProcessEvent` on destroyed objects, so querying their output parameters would only test zero-initialized defaults.

The first expanded run (`Artifacts/Automation/20260916_011754_127`) caught two fixture errors: that invalid post-Destroy query, and a launch assertion on a fixture with movement disabled. Neither failure was hidden. The fixture now explicitly activates walking for its launch scenario and reads actual terminal storage. The corrected suite passed **18/18, zero warnings/failures/not-run/in-process**: `Artifacts/Automation/20260916_012000_610`, `Artifacts/M3_UnitFoundation-Corrected-Tests.log`.

Mutation control deliberately restored premature death notification and float-only regeneration. Both new tests failed, while the other sixteen passed: `Artifacts/Automation/20260916_012105_899`, `Artifacts/M3_UnitFoundation-Mutation-Tests.log`. The actor observer detected uncleared selection, camera, blocking, attack/move state, target, collision, velocity and pending launch, and absent cleanup scheduling. The pure test observed infinite stamina. Both mutations were restored before final verification; no intentionally broken implementation is retained.

The restored suite passed **18/18, zero warnings/failures/not-run/in-process** on 2026-09-19: `Artifacts/Automation/20260919_200357_604`, `Artifacts/M3_UnitFoundation-Restored-Tests.log`. Independent final source/test review found no additional lifecycle blocker.

Coverage limits: corpse tests assert initial scheduling and removal within a 3.3-second window, not identical exact destruction timestamps. Invalid-damage cases check unchanged complete vitals/definition state, not notification counts. Camera/selection assertions inspect real component state, not rendered pixels.

Strict non-unity/no-PCH Editor compilation passed (`Artifacts/M3_UnitFoundation-IWYU.log`), as did Shipping Game compilation (`Artifacts/M3_UnitFoundation-Shipping.log`).

Development BuildCookRun passed on 2026-09-19, with 559 archived cooked packages (566 processed including platform skips), zero cook errors/warnings and UAT exit 0: `Artifacts/M3_UnitFoundation-Package.log`, archive `Artifacts/Packages/M3_UnitFoundation`. Cooking used the previously documented process-only `-DisableFab` workaround; no engine, project-plugin or Windows security settings changed. Development and Shipping receipts exclude MCP/editor toolsets/the test module/Fab's Editor DLL.

The actual archived player `Windows/RomanEmpireGame/Binaries/Win64/RomanEmpireGame.exe` has SHA256 `253e3f47a0a6d757379fb08676c4cc4cb2403ed62ad6b76aca52f1520ca82809`. It ran windowed at 1920×1080 with a four-second benchmark, without editor/Fab overrides, and exited **0**. `Artifacts/M3_UnitFoundation-Packaged-Smoke.log` proves startup of `L_VerticalSlice`, the canonical registry/GameState, production UMG and authored input, with no error/fatal/assert/ensure signatures. `Artifacts/M3_UnitFoundation-Packaged.png` was inspected: units and the tactical interface render, with matching resource/objective/diagnostic values. This is startup evidence, not proof of selected-ring or first-person rendering, sustained play or performance.

Repository validation passes. The Barracks asset still hashes to its committed LFS payload (`d54a3f10d27dd916e8abdd058573cebda51b1943616e968c6798b1f89779239c`). No editor/player process or loopback MCP listener remains after verification.

## Remaining scope

The subsequent state-separation package is tracked in `M3_UNIT_STATE_REVIEW.md`: order/combat/selection owners and derived activity are implemented and tested there, with separate evidence. M3.1 still requires a resolved routed-state policy and rendered selection/possession checks (the startup-player smoke alone does not prove those actions). Navigation/formation behavior, per-unit world scans, shared combat impact windows, tactical placement geometry, capture/resolution, save/restore, art and sustained performance remain subsequent plan packages. No existing green test or startup screenshot certifies those requirements.

Skills applied: `unreal-cpp-gameplay` for reflected ownership and lifecycle boundaries; `ue-data-assets-tables` for definition-driven initialization; `ue-testing-debugging` for pure-state, real PIE and mutation-proven regressions; `ue-module-build-system` for compilation/include checks; `unreal-packaging` for exact-binary startup verification; `ue-project-context` for the scoped factual context update. The context proposal was shown and adjustments invited before saving; engine/IDE/plugin choices were preserved.
