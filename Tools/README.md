# Development Tools

The M1 toolchain has two authoritative entry points:

```powershell
.\Tools\validate_repository.ps1
.\BuildProject.ps1 -Action Validate
```

`Tools/orchestrator.ps1` is a compatibility wrapper with only `validate`, `build`, and `test` actions. It delegates to those entry points and does not infer success from log strings.

`Tools/dev_orchestrator.ps1` is an intentional failure shim. The previous prototype could send the codebase to an external API, overwrite files from unreviewed model output, stage all changes, commit, and push directly to `main`; that behavior is not part of the controlled implementation workflow.

Generated validation reports belong under `Tools/Reports/` and are ignored. A result is release evidence only when its command, revision, environment, exit status, assertions, and artifacts are recorded by the milestone gate.

## Native UI integration

`BuildProject.ps1 -Action Test` builds the Editor target and runs the `RomanEmpireGame` automation namespace, including `RomanEmpireGame.UI.CanonicalEventDrivenPIE`. That test starts and tears down its own `L_Testbed` PIE session, reads public reflected contracts, and creates only transient fixtures. It does not save map changes.

The UI test checks synchronous canonical state updates, selection/possession/death cleanup, selected-barracks production, root detach/reattach subscriptions, and final-turn victory. Training clicks pass through Slate pointer press/release handlers using a virtual user and the live widget path; no OS cursor is moved and no UMG click delegate is invoked directly. This is an in-process routing test, not a complete desktop-input or visual-regression test. Rendered packaged smoke evidence is recorded separately in the implementation plan.

The public `ARomanEmpireGameState` contract is queried directly and compared with its canonical owners. Tests cover all relay signatures, unique subscriptions, safe uninitialized reads and EndPlay cleanup. TRAIN checks compare all six resource balances against the PDA cost for active/queued orders; promotion and rejected orders must not charge again. The separate `RomanEmpireGame.UI.NestedObjectiveTransactionsPIE` fixture commits a transaction inside a real public objective notification and verifies that production UMG re-queries current state instead of reviving an older payload. Its old-callback mutation control is recorded in `M2_EXIT_REVIEW.md`.

Automation report totals include both clean successes and `succeededWithWarnings`. A report with failed, not-run or in-process tests, or with no successes, is not accepted as a passing run.

## Enhanced Input integration

M2.6 adds configuration/negative-contract tests, authored-default/modifier checks, and `RomanEmpireGame.Input.PhysicalKeysAndModeLifecyclePIE`. The latter owns a fresh PIE session, injects simulated physical keys into the real PlayerController/Enhanced PlayerInput stack, and verifies mode isolation, command guards, held-key transitions and block cleanup. It preserves unrelated mapping contexts and does not save its transient world fixtures. See `INPUT_CONTROLS.md` for defaults and explicit coverage limits.

## Definition propagation and placement transactions

`RomanEmpireGame.Content.DefinitionPropagationAndAtomicPlacementPIE` temporarily edits the loaded authored Barracks definition before normal PIE startup, proves the change reaches the build menu, factory, construction timing and selection UI, then restores the complete definition and dirty state without saving assets. Signature-checked Editor-only observers attempt recursive placement from real wallet/placed events; assertions cover exactly one building and charge in all currencies, cancellation/restart during notification, insufficient funds and retry. The mutation-control failure and final passing report are recorded in `M2_EXIT_REVIEW.md`.

For an Editor-hosted `-game` diagnostic, use the process-only flag `-DisablePlugins=ModelContextProtocol,EditorToolset,AutomationTestToolset,ToolsetRegistry`. The UE 5.8 editor toolsets' Python startup otherwise expects editor-only reflected classes that are absent in this game host. Keep normal Editor tooling enabled in the project. This fallback never replaces an actual packaged-player launch gate.

## Unit foundation tests

`RomanEmpireGame.Units.VitalState` checks one-time/atomic initialization, terminal health, bounded stamina/morale, invalid values and an extreme float-rounding regression. `RomanEmpireGame.Units.FoundationPIE` owns a fresh `L_Testbed` session and transient native actors. It checks complete initialization snapshots, component identity/attachments through GC, ring visibility/material reuse, real camera activation/exit, active-order cancellation, death/reentrant commands, early destruction, bounded corpse timers at different altitudes, and termination from a vitals callback during dodge.

Destroyed actors deliberately reject reflected function calls in Unreal. The fixture therefore inspects retained reflected storage synchronously before GC for post-Destroy assertions; zero-filled return parameters are not accepted as evidence. These tests are not rendered-pixel, navigation or complete combat certification. Mutation controls and verification logs: `M3_UNIT_FOUNDATION_REVIEW.md`.

`RomanEmpireGame.Units.OrderState` uses real actors in an owned transient world/context for weak-target destruction/GC, alongside atomic order/pursuit/hold transitions. `CombatState` checks the sole recovery clock and orthogonal blocking. `OrdersCombatPIE` verifies actual coordinator timing, target retirement, persistent Hold, derived activity, and reentrant damage/vitals/camera callbacks. The complete suite currently has 21 tests. The old double-clock/double-rate/camera-order regressions were detected by mutation control before restoration; evidence and remaining scope: `M3_UNIT_STATE_REVIEW.md`.

## Optional Fab module failure on the audited host

The engine's external Fab 0.0.17 update is rejected by Windows Code Integrity on this workstation. The offline prototype has no Fab dependency. Use `BuildProject.ps1 -Action Test -DisableFab` or `-Action Package -DisableFab` (also supported for `Cook`) to omit only the optional Fab Editor browser for that automation process. The switch is rejected for other actions to avoid silently ineffective options. It does not change `.uproject`, engine files, Windows policy or the packaged player's launch arguments. Do not disable application control or modify the rejected DLL. For a manual Editor-hosted diagnostic, append `Fab` to its `-DisablePlugins` list until a trusted installation is available. Exact failure and passing-run evidence lives in `M2_EXIT_REVIEW.md`.
