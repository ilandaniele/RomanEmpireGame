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

## Enhanced Input integration

M2.6 adds configuration/negative-contract tests, authored-default/modifier checks, and `RomanEmpireGame.Input.PhysicalKeysAndModeLifecyclePIE`. The latter owns a fresh PIE session, injects simulated physical keys into the real PlayerController/Enhanced PlayerInput stack, and verifies mode isolation, command guards, held-key transitions and block cleanup. It preserves unrelated mapping contexts and does not save its transient world fixtures. See `INPUT_CONTROLS.md` for defaults and explicit coverage limits.

## Definition propagation and placement transactions

`RomanEmpireGame.Content.DefinitionPropagationAndAtomicPlacementPIE` temporarily edits the loaded authored Barracks definition before normal PIE startup, proves the change reaches the build menu, factory, construction timing and selection UI, then restores the complete definition and dirty state without saving assets. Signature-checked Editor-only observers attempt recursive placement from real wallet/placed events; assertions cover exactly one building and charge in all currencies, cancellation/restart during notification, insufficient funds and retry. The mutation-control failure and final passing report are recorded in `M2_EXIT_REVIEW.md`.

For an Editor-hosted `-game` diagnostic, use the process-only flag `-DisablePlugins=ModelContextProtocol,EditorToolset,AutomationTestToolset,ToolsetRegistry`. The UE 5.8 editor toolsets' Python startup otherwise expects editor-only reflected classes that are absent in this game host. Keep normal Editor tooling enabled in the project. This fallback never replaces an actual packaged-player launch gate.
