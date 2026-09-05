# Roman Empire Game Setup Guide

This guide describes the validated M1 development environment and the commands required to reproduce it from a reviewed checkout.

## 1. Install prerequisites

1. Install the Unreal Engine 5.8 Launcher build.
2. If VS Code is the editor, install Microsoft's C/C++ Extension Pack. VS Code does not include a Windows C++ compiler.
3. Install Visual Studio 2022 Build Tools 17.14 or newer (or a supported full Visual Studio installation) using `.vsconfig`. It records UE 5.8's preferred VS 2022 MSVC 14.44 LTSC toolset, matching ATL component, .NET Framework 4.8 SDK for SwarmInterface, and Windows SDK 10.0.22621.0.
4. Install Git LFS and ensure `git`, `git lfs`, and PowerShell are available on `PATH`.

The repository currently targets Windows x64 only.

On the workstation audited on 2026-09-05, UE 5.8.2, VS Code 1.136.1, the official C/C++ Extension Pack, Visual Studio Build Tools 2022 17.14, MSVC 14.44, the .NET Framework 4.8 SDK, and Windows SDK 10.0.22621/26100 are installed. VS Code generation plus Development Editor, Development Game, Shipping Game, and strict non-unity/no-PCH builds succeed.

## 2. Fetch the repository and assets

```powershell
git clone https://github.com/ilandaniele/RomanEmpireGame.git
Set-Location RomanEmpireGame
git lfs install
git lfs pull
```

Source `.uasset` and `.umap` files are tracked through Git LFS. Generated `Binaries`, `Intermediate`, `Saved`, `DerivedDataCache`, and `Artifacts` directories are ignored.

## 3. Validate the workstation

```powershell
.\BuildProject.ps1 -Action Validate
```

The script validates the engine minor version, Visual Studio C++ installation, and Git LFS state. For a non-default engine location:

```powershell
.\BuildProject.ps1 -Action Validate -EngineRoot 'D:\Epic Games\UE_5.8'
```

Do not edit a private absolute engine path into repository files.

## 4. Generate and build

```powershell
# VS Code is the default project-file format
.\BuildProject.ps1 -Action Generate
.\BuildProject.ps1 -Action Editor
.\BuildProject.ps1 -Action IWYU
```

Use `-IDE VisualStudio` with the `Generate` action only when a full Visual Studio IDE is intentionally selected. Generated `.vscode`, workspace, compile-command, and solution files are disposable and ignored by Git.

Header/reflection changes require a full Editor rebuild. Live Coding is appropriate only after a clean build and for compatible implementation-only changes.

Every `BuildProject.ps1` action runs `Tools/validate_repository.ps1` first. The `Test` action writes a fresh timestamped report under `Artifacts/Automation/` and fails if UE does not produce `index.json`, no test succeeds, or any test fails.

The repository disables Unreal Build Accelerator by default. On the audited Windows 11 workstation, Smart App Control blocks locally built unsigned Editor DLLs produced through UBA; the equivalent direct UBT build with `-NoUBA` loads successfully. Do not disable Smart App Control. If another workstation's application-control policy accepts UBA output, it may opt in explicitly, for example `.\BuildProject.ps1 -Action Editor -UseBuildAcceleration`.

## 5. Bootstrap content

The audited repository contains no legacy Unreal binary assets. M1 created these source assets through Unreal Editor:

- `/Game/Maps/L_Testbed`, the deterministic development and PIE map.
- `/Game/Maps/L_VerticalSlice`, the packaged-game default map.

Both maps explicitly use `ARomanEmpireGameMode`, pass Map Check, cook/package successfully, and are covered by `.gitattributes` Git LFS rules. Dedicated Enhanced Input, Data Asset, UMG, and Blueprint content remains planned for the later gameplay/UI milestones; the M1 scene uses the existing deterministic C++ bootstrap.

The scripts under `Content/Scripts` are unvalidated prototype inputs. Do not execute `MasterSetup.py` until it has been reviewed against UE 5.8 and a clean Git recovery point exists.

## 6. Required recovery evidence

Before M2 starts, record successful evidence for:

```powershell
.\BuildProject.ps1 -Action Editor
.\BuildProject.ps1 -Action Game
.\BuildProject.ps1 -Action Test
.\BuildProject.ps1 -Action Cook
.\BuildProject.ps1 -Action Package -Configuration Development
```

Then launch the archived executable and verify that it reaches the deterministic tactical scene. PIE must also run for ten minutes without a crash or ensure. This evidence was recorded on 2026-09-05; after the M1 changes are reviewed and committed, repeat the repository/LFS validation from a clean clone to close the final reproducibility check.

## Tooling safety

- Remote Control and its web interface are disabled by default.
- Automation Driver does not belong in the Shipping runtime module.
- Unreal MCP uses only the Editor-targeted `ModelContextProtocol`, `EditorToolset`, and `AutomationTestToolset` plugins; `AllToolsets` is disabled. Start it manually on `127.0.0.1:8100` only for an intentional editor session, and stop the editor afterward.
- Supabase placeholders do not enable online behavior; the first slice remains offline-first.
