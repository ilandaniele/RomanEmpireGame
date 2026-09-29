# Roman Empire Game

Roman Empire Game is an Unreal Engine 5.8 single-player Windows project combining a campaign layer, real-time tactical command, settlement development, and explicit first-person possession of a soldier.

M1 is complete and clean-clone proven on the audited workstation: the project validates, generates VS Code metadata, builds Editor/Game/Shipping targets, passes native automation and IWYU checks, runs the deterministic test map in PIE, and produces a smoke-tested Windows Development package. The approved M1 baseline is commit `0b74ad1566306d7487629812de68bc6abd7f0594`; M2's canonical-state exit gates are complete. M3.1 unit foundation is in progress; [its review](M3_UNIT_FOUNDATION_REVIEW.md) separates verified changes from remaining work.

## Product target

The first playable slice is one complete Rome-versus-Carthage loop: build a barracks, recruit and command a small force, possess a legionary, capture an objective, resolve the territory result, save, exit, and restore.

See [PRODUCT_BRIEF.md](PRODUCT_BRIEF.md) for scope and decisions, [IMPLEMENTATION_PLAN.md](IMPLEMENTATION_PLAN.md) for milestone order and acceptance gates, and [M1_REVIEW.md](M1_REVIEW.md) for the completed M1 review and reproducibility evidence.

## Prerequisites

- Unreal Engine 5.8 Launcher build (5.8.2 is the validated local installation).
- VS Code with Microsoft's C/C++ Extension Pack, if VS Code is the chosen editor.
- Visual Studio 2022 Build Tools 17.14 or newer (or a supported full Visual Studio installation), with the MSVC x64/x86 tools, .NET Framework SDK, and Windows SDK declared in `.vsconfig`. The compiler toolchain is required even when VS Code is the editor.
- Git LFS.
- Windows 11 or a supported Windows 10 development environment.

After cloning:

```powershell
git lfs install
git lfs pull
.\Tools\validate_repository.ps1
.\BuildProject.ps1 -Action Validate
```

If Unreal is installed outside the Epic Launcher default location:

```powershell
.\BuildProject.ps1 -Action Validate -EngineRoot 'D:\Epic Games\UE_5.8'
```

The `ROMANEMPIREGAME_ENGINE_ROOT` environment variable is also supported.

## Build commands

```powershell
# Generate VS Code project files (default)
.\BuildProject.ps1 -Action Generate

# Optional: generate Visual Studio project files instead
.\BuildProject.ps1 -Action Generate -IDE VisualStudio

# Development Editor and standalone Game targets
.\BuildProject.ps1 -Action Editor
.\BuildProject.ps1 -Action Game

# Strict include audit: non-unity build with PCH disabled
.\BuildProject.ps1 -Action IWYU

# Native automation tests
.\BuildProject.ps1 -Action Test

# Cook or archive a Windows package under Artifacts/
.\BuildProject.ps1 -Action Cook
.\BuildProject.ps1 -Action Package -Configuration Development
```

Unreal Build Accelerator is disabled by default because Windows Smart App Control can reject locally built unsigned Editor DLLs emitted through UBA. This keeps the supported workflow compatible with the audited workstation without weakening Windows security. On a machine where its application-control policy accepts UBA output, opt in explicitly with `-UseBuildAcceleration`.

`build.bat` remains as a compatibility shortcut for the Development Editor build and delegates to `BuildProject.ps1`.

## Repository layout

```text
Config/                         Unreal project configuration
Content/                        Tracked design inputs and future LFS assets
Source/RomanEmpireGame/         Runtime C++ module
  Public/Faction/               Exported resource/faction contract used by tests
Source/RomanEmpireGameTests/    Editor-only native automation module
scripts/                        Retirement record for the removed Remote Control harness
Tools/                          Legacy orchestration/reporting tools
.agents/ue-project-context.md   Audited project context for UE implementation skills
```

Remote Control and Automation Driver are disabled in normal project defaults. Epic's UE 5.8 Unreal MCP is configured only for Editor targets with the narrowed `EditorToolset` and `AutomationTestToolset`; the broad `AllToolsets` plugin is disabled. The server is loopback-only, started manually per session, and absent from Game and Shipping receipts.

## Current implementation status

M1 and M2 are complete: canonical resources/turns/territories/diplomacy, Primary Data Assets, public GameState, event-driven UMG, and mode-specific Enhanced Input. M3 has focused vitals/order/combat/selection owners, player-unit/building ownership guards and a weak world/faction unit registry. Historical rendered ownership/possession evidence is in [M3_UNIT_REGISTRY_REVIEW.md](M3_UNIT_REGISTRY_REVIEW.md); building eligibility and TRAIN evidence is in [M3_BUILDING_SELECTION_REVIEW.md](M3_BUILDING_SELECTION_REVIEW.md).

Shift-click toggle, Shift-drag add and transient invalid-target feedback are implemented. That additive-selection revision passed 25/25 without result warnings, plus strict IWYU, Shipping compilation and Development packaging. Windows Code Integrity blocked its unsigned packaged executable before startup, so its actual-player gate remains open; no security policy was changed. See [M3_ADDITIVE_SELECTION_REVIEW.md](M3_ADDITIVE_SELECTION_REVIEW.md) for revision-specific evidence and limits. Automation uses process-only `-DisableFab`. [INPUT_CONTROLS.md](INPUT_CONTROLS.md) documents defaults; [IMPLEMENTATION_PLAN.md](IMPLEMENTATION_PLAN.md) tracks remaining routing, combat, campaign and release work. Art remains placeholder and the vertical slice is not finished.

Tactical squad membership is now integrated with initial forces and recruitment, with atomic same-faction grouping and lifecycle cleanup. That baseline passed 26/26 without result warnings, strict IWYU and Shipping compilation. Handles are battle-local, not persistent campaign IDs; campaign mapping remains later work. See [M3_SQUAD_REGISTRY_REVIEW.md](M3_SQUAD_REGISTRY_REVIEW.md).

Navigation uses AIController, CharacterMovement and dynamic NavMesh in both maps. Real-PIE tests cover obstacle avoidance, pursuit, cancellation, possession reuse, lifecycle cleanup, disconnected islands, a gated narrow passage and crossing formations. The latest regression passes 34/34; [M3_NAVIGATION_REVIEW.md](M3_NAVIGATION_REVIEW.md), [M3_DISCONNECTED_NAVIGATION_REVIEW.md](M3_DISCONNECTED_NAVIGATION_REVIEW.md) and [M3_GROUP_CONGESTION_REVIEW.md](M3_GROUP_CONGESTION_REVIEW.md) record revision-specific evidence and remaining gates. [M3_PERFORMANCE_BASELINE_REVIEW.md](M3_PERFORMANCE_BASELINE_REVIEW.md) records provisional 50/100-unit editor profiling; it is not a packaged-player benchmark.

Multi-selection movement assigns distinct formation slots, rejects invalid footprints without replacing prior orders, and protects newer group commands from reentrant cancellation callbacks. Partial squad selections remain partial. That revision passed 29/29; [M3_FORMATION_REVIEW.md](M3_FORMATION_REVIEW.md) records its evidence.

The tactical selection panel now exposes STOP and HOLD for selected units. Its original 30/30 regression gate passed, and the latest expanded suite passes 34/34; [M3_SELECTED_COMMANDS_REVIEW.md](M3_SELECTED_COMMANDS_REVIEW.md) records live route, Slate click and lifecycle checks. A fresh packaged-player gate remains open.

### M1 baseline evidence

- Visual Studio Build Tools 2022 17.14, MSVC 14.44, .NET Framework 4.8 SDK, and Windows SDK 10.0.22621/26100 are installed and accepted by UE 5.8.2.
- The supported build path leaves Windows Smart App Control enabled and uses direct non-UBA compilation by default; full Development and Shipping rebuilds, IWYU, tests, cook, package, and smoke launch pass on that path.
- `/Game/Maps/L_Testbed` and `/Game/Maps/L_VerticalSlice` are created, assigned to the explicit framework classes, dynamically lit, validated, cooked, and covered by Git LFS rules.
- Two native automation tests pass; the M1 PIE stability run completed for more than ten minutes without an error, ensure, fatal error, or crash.
- The archived Development package reaches `/Game/Maps/L_VerticalSlice`; Development and Shipping receipts contain no MCP/toolset modules.
- Human review accepted the M1 baseline. Commit `0b74ad1566306d7487629812de68bc6abd7f0594` reproduced from a clean clone through LFS pull, validation, Editor/IWYU/Game builds, tests, packaging, packaged smoke, and player-receipt inspection. No remote push was performed.

Asset provenance and intake rules are in [ASSET_INVENTORY.md](ASSET_INVENTORY.md). Project licensing is defined by [LICENSE.txt](LICENSE.txt) and [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md).
