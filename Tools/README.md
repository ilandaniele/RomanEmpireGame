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

`RomanEmpireGame.Units.OrderState` uses real actors in an owned transient world/context for weak-target destruction/GC, alongside atomic order/pursuit/hold transitions. `CombatState` checks the sole recovery clock and orthogonal blocking. `OrdersCombatPIE` verifies actual coordinator timing, target retirement, persistent Hold, derived activity, and reentrant damage/vitals/camera callbacks. The old double-clock/double-rate/camera-order regressions were detected by mutation control before restoration; evidence and remaining scope: `M3_UNIT_STATE_REVIEW.md`.

The complete suite now has 22 tests. `PlayerOwnershipPIE` adds same-world/player-faction/lifecycle boundaries, mixed selection, player orders versus enemy AI orders, unit/player-faction changes, interrupted possession, nested selection and real controller teardown. Its observer ordering is explicit so stale UMG payloads cannot accidentally pass. Both stale-payload and missing-faction mutations fail before restoration. Fixtures are transient; the auxiliary world's deinitialized subsystem is collected before Slate next dispatches input. Evidence: `M3_OWNERSHIP_REVIEW.md`.

## Packaged selection and possession presentation

```powershell
.\BuildProject.ps1 -Action Package -Configuration Development -ArchiveDirectory Artifacts/Packages/M3_PresentationFinal -DisableFab
.\Tools\tests\test_unit_presentation_log.ps1
.\Tools\verify_unit_presentation.ps1 -OutputDirectory Artifacts/UnitPresentation-NewRun
```

The helper launches the **actual Development player**, records its hash, holds the initial formation, selects/focuses one player unit, enters possession, restores the original camera and clears selection. It records `Player.log`, `Selected.png`, `Possessed.png` and `Returned.png`. Each run requires a new output directory without spaces, commas or quotes. `-ArchiveDirectory` can select another newly built Development archive; older binaries without the two diagnostic commands must fail.

The helper compares observed actor/controller identities, phase ordering, restored camera and cleared selection/possession flags; a successful console dispatch is insufficient. It also checks exit status, captures and failure signatures. Its bounded timeout reports the existing PID without killing or restarting it. Manually inspect all three PNGs before accepting the rendered result. This is **not** OS input injection, a full visual-regression suite, a performance benchmark or Shipping-player execution.

The two local `roman.Debug.*` commands use normal gameplay facades and are compile-time excluded from Shipping. No remote service or editor/test dependency is introduced. The parser self-test extracts only the validation function through PowerShell's syntax tree, without running the player-launch body; its synthetic fixtures are parser evidence only. Exact real-run evidence and limits: `M3_PRESENTATION_REVIEW.md`.

For the M3.2a ownership build, pass its archive explicitly. `-LoseOwnership` transfers the initial legionaries to Carthage after possession and requires automatic camera restoration and selection/possession cleanup, without an explicit exit or clear command:

```powershell
.\Tools\verify_unit_presentation.ps1 -ArchiveDirectory Artifacts/Packages/M3_Ownership -OutputDirectory Artifacts/Ownership-Explicit-NewRun
.\Tools\verify_unit_presentation.ps1 -ArchiveDirectory Artifacts/Packages/M3_Ownership -OutputDirectory Artifacts/Ownership-Loss-NewRun -LoseOwnership
```

The exact formerly selected actor must also be observed as Carthage after exit. This changes only the temporary player session, not saved assets. Parser coverage totals 4 positive fixtures and 20 expected rejections. Current package evidence and limitations: `M3_OWNERSHIP_REVIEW.md`.

## Live-unit registry regression

The M3.2b live-unit registry regression uses the same helper, unchanged, with `-ArchiveDirectory Artifacts/Packages/M3_Registry` and fresh output directories. Both explicit-return and `-LoseOwnership` scenarios passed with six inspected captures. See `M3_UNIT_REGISTRY_REVIEW.md` for the exact binary hash, native rectangle-projection coverage and remaining limits. The default archive above remains historical; explicitly select the build being tested.

## Optional Fab module failure on the audited host

The engine's external Fab 0.0.17 update is rejected by Windows Code Integrity on this workstation. The offline prototype has no Fab dependency. Use `BuildProject.ps1 -Action Test -DisableFab` or `-Action Package -DisableFab` (also supported for `Cook`) to omit only the optional Fab Editor browser for that automation process. The switch is rejected for other actions to avoid silently ineffective options. It does not change `.uproject`, engine files, Windows policy or the packaged player's launch arguments. Do not disable application control or modify the rejected DLL. For a manual Editor-hosted diagnostic, append `Fab` to its `-DisablePlugins` list until a trusted installation is available. Exact failure and passing-run evidence lives in `M2_EXIT_REVIEW.md`.
