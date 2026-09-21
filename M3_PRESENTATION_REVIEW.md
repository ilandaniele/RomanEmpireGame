# M3.1 packaged selection and possession presentation

Started 2026-09-21 from `71e17c8`. This packet covers a rendered Development-player check, not M3.1 or full-plan closure. The order/combat/selection ownership changes remain documented in `M3_UNIT_STATE_REVIEW.md`.

## Changes and scope

`Tools/verify_unit_presentation.ps1` starts the actual archived Windows player, records its SHA256, selects an eligible player unit, enters direct possession, returns to the original camera and clears selection. Built-in deferred console commands schedule a finite scenario; screenshots include UMG. The helper refuses to overwrite an existing output directory and rejects missing captures, failed exit status, failure signatures or inconsistent observed state. Its timeout reports the existing PID without killing or relaunching it.

Two local `roman.Debug.*` commands resolve the live player unit through normal gameplay APIs instead of guessing cooked actor names. They add no ticks, retained actor references, remote service, editor dependency or input mappings. Both registrations are guarded by `!UE_BUILD_SHIPPING`. Selection is revalidated after synchronous observers before focusing the camera.

The scenario uses Hold to stabilize the initial formation, disables motion blur for the captures and invokes commands directly. It does not inject physical keyboard/mouse input, validate every combat/navigation behavior or modify saved assets. Returning from possession can resume legacy idle target acquisition; this is not a formation-position restoration test.

The first successful captures exposed a real UMG defect: auto-wrapping retained a narrower prior mode label width, making mode/turn text spill below the fixed-height header. Resource and mode/turn labels now stay single-line. The existing live-PIE UI test checks this policy at startup, possession and return, along with actual authoritative mode text. Other multiline panels are unchanged. Small-window/localization polish remains M6 work.

## Earlier diagnostic runs

- The initial script assumed the editor-style name `Legionary_7`. In the packaged player, actors had generated names such as `Legionary_2147482114`. KE reported dispatch success but possession never happened. The helper correctly rejected the run; `Artifacts/UnitPresentation-71e17c8` and `Artifacts/M3_UnitStates-Presentation.log` are failure evidence, not a pass.
- The name-independent Development player then exited 0 and produced three inspected 1920×1080 captures under `Artifacts/UnitPresentation-Resolved`. A selected green ring, actual first-person camera/crosshair/vitals and restored tactical view were visible. Its mode/turn wrapping defect prompted the final correction. Binary SHA256: `a5d24cafffd700ca577b9d1dc7fbbc929e348d821c5b4c09719eaf36524f547d`. This historical run lacks the stricter helper's new initial/final Pawn and final possession-flag samples and is not the final acceptance artifact.

## Final verification

- Native automation: **21/21; zero warnings, failures, not-run or in-process**. Report `Artifacts/Automation/20260921_000502_613`; command log `Artifacts/M3_Presentation-Tests.log`.
- Shipping compilation: passed (`Artifacts/M3_Presentation-Shipping.log`). Both diagnostic command names are present in the Development binary and absent from the Shipping executable when checked as ASCII and UTF-16 strings, consistent with the compile-time guard.
- Strict non-unity/no-PCH Editor IWYU: passed, 59 actions (`Artifacts/M3_Presentation-IWYU.log`).
- Development package: UAT exit 0, **559 cooked packages, 7 platform-skipped (566 total processed), zero cook errors/warnings** (`Artifacts/M3_Presentation-Final-Package.log`, archive `Artifacts/Packages/M3_PresentationFinal`). No saved project assets were changed.
- Actual archived executable: `Windows/RomanEmpireGame/Binaries/Win64/RomanEmpireGame.exe`, SHA256 **`2ce074b9f71b5f766f24217b492d12776a655de5a6144d1f377433681e2805bb`**. Player PID 31080 exited 0. `Artifacts/M3_Presentation-Final-Run.log` records the passing strict state checks; `Artifacts/UnitPresentation-Final/Player.log` contains the observed transitions and references.
- All three final **1920×1080** captures were inspected: `Selected.png` shows a green selected ring and unit panel; `Possessed.png` shows the actual first-person viewpoint, crosshair and soldier vitals; `Returned.png` shows restored tactical view without ring, selection panel, crosshair or possession panel. Mode/turn stays on one line in all three. The helper confirms the original camera Pawn identity, not an exact camera transform or formation position.
- Pure PowerShell parser regression: `Tools/tests/test_unit_presentation_log.ps1` passes 3 synthetic positive fixtures and 17 expected rejection cases, with expected failure-message checks. It launches no game and is separate from the 21 native tests and real-run evidence.
- Repository/prerequisite validation and Git LFS fsck pass. Shipping receipt excludes the test module, MCP and editor toolsets. No Editor/player process or port-8100 listener remains after verification.

The actual-player log has no failure signatures, but has two engine/host warnings: NVIDIA TSR 16-bit VALU is disabled for drivers older than 610.00, and `r.MotionVectorSimulation` is accessed from the render thread without `ECVF_RenderThreadSafe`. These are recorded, not suppressed or fixed by changing drivers/security. The zero-warning counts above apply to native test results and cooking, not this player log.

## Reproduction

```powershell
.\BuildProject.ps1 -Action Test -DisableFab
.\BuildProject.ps1 -Action IWYU
.\BuildProject.ps1 -Action Game -Configuration Shipping
.\BuildProject.ps1 -Action Package -Configuration Development -ArchiveDirectory Artifacts/Packages/M3_PresentationFinal -DisableFab
.\Tools\tests\test_unit_presentation_log.ps1
.\Tools\verify_unit_presentation.ps1 -OutputDirectory Artifacts/UnitPresentation-NewRun
```

Use a fresh output directory without spaces, commas or quotes. The helper validates that the selected array, `PossessedUnit` and actual controller Pawn identify the same unit, only that unit is flagged possessed, and return restores the original camera with empty selection and all possession flags false. These checks must pass **and** all three PNGs must be inspected. Console dispatch messages or screenshot existence alone do not prove the gate.

The optional Fab browser is disabled only for editor automation/cooking on this host. No Windows policy, graphics driver, engine files or project plugin defaults are changed. Actual-player driver/render-thread warnings are recorded separately from test/cook warning counts; neither is suppressed.

## Still open

Routed-state design and implementation remain M3.1 work. Zero morale has no implicit new effect. A routed unit must remain alive and vulnerable while unable to act, with controller/UMG cleanup distinct from death. Thresholds, physical flight, objective participation and recovery need an explicit design choice. Player-faction selection boundaries and registries are M3.2; navigation, validated combat windows, encounter completion, performance, save/restore and finished art remain later work.

Skills used: `unreal-packaging` for the actual-player gate, `ue-ui-umg-slate` for the scoped native HUD correction, `ue-testing-debugging` for state/log and live-PIE checks, `unreal-cpp-gameplay` for callback-safe diagnostics, `ue-module-build-system` for include/Shipping checks, and `ue-project-context` for factual verification/status updates. No new asset or external plugin installation was needed.
