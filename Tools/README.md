# Development Tools

The M1 toolchain has two authoritative entry points:

```powershell
.\Tools\validate_repository.ps1
.\BuildProject.ps1 -Action Validate
```

`Tools/orchestrator.ps1` is a compatibility wrapper with only `validate`, `build`, and `test` actions. It delegates to those entry points and does not infer success from log strings.

`Tools/dev_orchestrator.ps1` is an intentional failure shim. The previous prototype could send the codebase to an external API, overwrite files from unreviewed model output, stage all changes, commit, and push directly to `main`; that behavior is not part of the controlled implementation workflow.

Generated validation reports belong under `Tools/Reports/` and are ignored. A result is release evidence only when its command, revision, environment, exit status, assertions, and artifacts are recorded by the milestone gate.
