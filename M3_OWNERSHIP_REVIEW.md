# M3.2a player-unit ownership and reactive cleanup

Started 2026-09-21 from `687d06b`. This bounded package does not close M3.1, all of M3.2, or the full implementation plan.

## Changes and authority

- `AUnitBase` remains the owner of its faction. `SetOwnerFaction` emits a committed invalidation notification, without a duplicate faction store or notification for unchanged ownership. Observers re-query current state.
- The controller shares one eligibility query: initialized canonical player faction, same world, command-ready unit, matching owner, and a controller that is not ending play. Selection, player move/attack dispatch and possession use it. Actor-level orders remain available to enemy AI.
- Ownership and player-faction notifications prune invalid selection and release invalid possession, even outside Tactical mode. Cleanup restores the camera and input mode, releases blocking and unit possession, and removes observers. EndPlay also releases possession and prevents contexts from being reinstalled during teardown.
- Possession subscribes before synchronous camera activation and revalidates after callback-producing operations. Exit commits absence before camera deactivation and guards recursive exit/entry. Final notifications have no stale outer state writes after them.
- Selection events carry a snapshot. UMG binds to the controller's current selection, because an earlier observer can supersede the outer payload with a nested selection.

No new dependencies, plugins, assets, per-frame scans or gameplay authority copies were added. Replace-selection semantics remain unchanged. Building ownership selection, additive selection, invalid-target feedback, registries and squad membership remain later M3.2 work. This package does not define morale/retreat, diplomacy-based attack permission, physical flight, navigation or combat hit windows.

## Native regression coverage

`RomanEmpireGame.Units.PlayerOwnershipPIE` uses the real L_Testbed controller, canonical faction manager, UMG and transient native legionaries. It checks mixed-faction/null/dead/deferred/foreign-world eligibility, deduplication and indicators, player move/attack dispatch, preserved enemy actor commands, unit and player-faction ownership loss, actual camera/input/HUD cleanup, and observer removal at EndPlay. Actual camera activation callbacks interrupt possession through owner change, death, destruction or exit. Deactivation attempts recursive exit and entry. Nested selection changes exercise the real production UMG observer after the mutating observer.

The foreign-world fixture initializes an auxiliary game world with the existing canonical definition registry; it proves that the actor is otherwise command-ready before testing world exclusion. It destroys its world/context and collects its deinitialized subsystem before Slate next dispatches input. No engine source or diagnostic policy was changed.

## Diagnostic history and mutation controls

- Initial fixture run failed: its foreign-world actor was not command-ready, and delayed collection of the auxiliary Enhanced Input world subsystem caused ensures. `Artifacts/M3_Ownership-Tests.log` and `Artifacts/Automation/20260921_114811_722` are failure evidence. A subsequent compile attempt used nonexistent `Engine/URL.h`; the fixture now directly includes `Engine/EngineBaseTypes.h` for FURL.
- Corrected and expanded fixtures passed 22/22 (`20260921_115105_026` and `20260921_115337_127`), but these were not the final test revision.
- Restoring the stale UMG payload initially **survived** the test (`Artifacts/M3_Ownership-UI-Mutation-Tests.log`, report `20260921_115510_589`). That revealed missing observer-order coverage, not proof of correctness.
- After explicitly placing the real UMG observer after the mutating observer, that same mutation fails precisely at `UMG observes current selection, not stale outer payload`. Report `Artifacts/Automation/20260921_164212_042`, log `Artifacts/M3_Ownership-UI-MutationCorrected-Tests.log`: 21 pass, 1 fail.
- Removing faction equality from the eligibility predicate fails precisely at `Enemy unit ineligible`. Report `Artifacts/Automation/20260921_164542_861`, log `Artifacts/M3_Ownership-Eligibility-Mutation-Tests.log`: 21 pass, 1 fail.
- Both mutations were restored. Final native suite: **22/22, zero warnings/failures/not-run/in-process**, report `Artifacts/Automation/20260921_164623_820`, log `Artifacts/M3_Ownership-Final-Tests.log`.
- Pure PowerShell parser regression passes **4 synthetic positives and 20 expected rejection cases** in total. The ownership variant additionally rejects missing/wrong ownership and an ownership sample preceding exit. These are parser tests, separate from native tests and actual-player evidence.

## Packaged ownership-loss scenario

`Tools/verify_unit_presentation.ps1 -LoseOwnership` selects and possesses one player unit, then uses a built-in reflected command to transfer the initial legionaries to Carthage. This is a transient fixture affecting all initial legionaries, not a new gameplay transfer UI. It does **not** issue explicit exit or clear-selection commands: production ownership notifications must restore the original camera and clear selection/possession themselves. The helper additionally verifies that the exact formerly selected unit is observed as Carthage after automatic exit. No saved assets are modified.

The ordinary explicit-exit scenario is retained. Both require strict identity/phase/flag checks, process exit 0, three captures and separate pixel inspection. Neither injects physical input, certifies a complete visual regression suite or executes a Shipping player.

## Final build and actual-player verification

- Strict non-unity/no-PCH Editor IWYU: **passed, 59 actions**, `Artifacts/M3_Ownership-IWYU.log`.
- Shipping compilation: **passed**, `Artifacts/M3_Ownership-Shipping.log`. Receipt excludes the test module, MCP and editor toolsets. Both diagnostic command names are absent from the Shipping executable as ASCII and UTF-16.
- Development package: **UAT exit 0, 559 cooked packages, 7 platform-skipped (566 total processed), zero cook errors/warnings**, `Artifacts/M3_Ownership-Package.log`; archive `Artifacts/Packages/M3_Ownership`.
- Actual archived executable `Windows/RomanEmpireGame/Binaries/Win64/RomanEmpireGame.exe`, SHA256 **`b7e63c892fd6d23b601c5ff7c086e6e1bd9965ea3f3e4715988c1b86cc2a256d`**.
- Ordinary explicit-exit scenario: PID 1832, exit 0, all strict state checks pass. Log `Artifacts/M3_Ownership-Explicit-Run.log`; player log and captures under `Artifacts/Ownership-Explicit`.
- Ownership-loss scenario: PID 7104, exit 0, all strict state checks pass, including exact formerly selected actor ownership. Log `Artifacts/M3_Ownership-Loss-Run.log`; player log and captures under `Artifacts/Ownership-Loss`. Exit occurs during the ownership-transfer command, without an explicit exit/clear command.
- All **six 1920×1080 captures were inspected**. Both show a selected ring/unit panel, actual first-person view/crosshair/vitals, then restored tactical view with neither selection nor possession HUD. The ownership-loss return additionally shows the transferred formation in Carthage's purple. The header remains single-line. Original camera identity is verified, not exact camera transform or restored formation positions.
- Repository/prerequisite validation and Git LFS fsck pass. No tracked binary assets changed. No Editor/player process or port-8100 listener remains. Historical evidence from `M3_PRESENTATION_REVIEW.md` was not substituted for this revision.

Both player logs have no failure signatures and retain the same two engine/host warnings: NVIDIA TSR 16-bit VALU disabled with drivers older than 610.00, and render-thread access to `r.MotionVectorSimulation` without `ECVF_RenderThreadSafe`. These warnings are recorded, not suppressed. Zero-warning native-test/cook results do not mean the player logs are warning-free. Windows security, drivers, engine files and plugin defaults remain unchanged.

M3.2a is verified. Remaining M3.2 selection UX/building ownership and live unit/squad registry work, plus M3.1 routed policy/implementation, are not closed.

## Reproduction

```powershell
.\BuildProject.ps1 -Action Test -DisableFab
.\BuildProject.ps1 -Action IWYU
.\BuildProject.ps1 -Action Game -Configuration Shipping
.\BuildProject.ps1 -Action Package -Configuration Development -ArchiveDirectory Artifacts/Packages/M3_Ownership -DisableFab
.\Tools\tests\test_unit_presentation_log.ps1
.\Tools\verify_unit_presentation.ps1 -ArchiveDirectory Artifacts/Packages/M3_Ownership -OutputDirectory Artifacts/Ownership-Explicit-NewRun
.\Tools\verify_unit_presentation.ps1 -ArchiveDirectory Artifacts/Packages/M3_Ownership -OutputDirectory Artifacts/Ownership-Loss-NewRun -LoseOwnership
```

Use fresh output directories. `-DisableFab` remains process-only for the optional Fab module rejected by this host. No engine/plugin defaults, Windows security or graphics drivers were changed. No remote push is authorized.

Skills: `unreal-cpp-gameplay`, `ue-ui-umg-slate`, `ue-input-system`, `ue-testing-debugging`, `ue-module-build-system`, `unreal-packaging`; project context updates follow `ue-project-context`. These guide canonical ownership, callback/lifecycle cleanup, real-PIE tests, strict includes and actual-player verification. No skill/plugin installation or new art was needed.
