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
