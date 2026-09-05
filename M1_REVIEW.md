# M1 Review Packet

*Prepared: 2026-09-05*
*Baseline: `89d8d66e5051d303dea9c965317cdb961d8a52f6` on `main`*
*Status: complete; human-approved dedicated commit and clean-clone reproduction passed*

## Decision recorded

The user approved M1 on 2026-09-05 and authorized the dedicated local commit, post-commit clean-clone/LFS reproduction, and continued execution of the implementation goal. The approved baseline was committed as `0b74ad1566306d7487629812de68bc6abd7f0594` and reproduced successfully. No push to a remote was requested or performed.

Reviewer checklist:

- [x] UE 5.8, VS Code, and Visual Studio Build Tools are acceptable as the project baseline.
- [x] Retiring the broken Remote Control/Python test harness is acceptable.
- [x] The two Epic-template-derived bootstrap maps and their licensing record are acceptable.
- [x] The source-control, plugin, module, and Windows security decisions match the intended workflow.
- [x] The behavioral fix that keeps building placement active after a failed confirmation is acceptable.
- [x] A dedicated M1 commit and clean-clone validation are authorized.

## Scope reviewed

The working tree contains 59 tracked change entries and 21 new source files relative to the baseline, including this review packet. Nine tracked files are removed. Generated build, package, test, and IDE outputs remain ignored.

| Area | Material change | Review focus |
|---|---|---|
| Repository policy | Git LFS rules, editor settings, license, attribution, inventory, generated-file exclusions | Binary asset policy and provenance |
| Engine/toolchain | UE 5.8 association, current target settings, portable `BuildProject.ps1`, VS Code generation, Visual Studio Build Tools prerequisites | Reproducibility and non-UBA safe default |
| Plugins/security | Remote Control and Automation Driver disabled; Epic MCP and narrow toolsets restricted to Editor | No tooling in Game or Shipping |
| C++ baseline | Duplicate declarations, illegal access, missing implementation, include paths, IWYU, dependencies, log category, and placement confirmation corrected | Recovery fixes only; canonical-state redesign is deferred |
| Tests | Editor-only native test module with resource-rule and content/framework validation | Two M1 tests are not the future full gameplay suite |
| Content | Deterministic `L_Testbed` and `L_VerticalSlice` maps, fixed runtime seed, explicit framework defaults | Placeholder quality and template-derived licensing |
| Retirements | Generated `.sln`, stale reports, unsafe legacy orchestration behavior, and broken Remote Control/Python E2E files removed or reduced | History remains recoverable in Git |

## Acceptance evidence

| M1 requirement | Authoritative evidence | Result |
|---|---|---|
| Repository and prerequisites validate | `Tools/validate_repository.ps1`; `BuildProject.ps1 -Action Validate` | Pass, 0 warnings |
| IDE metadata is reproducible | `BuildProject.ps1 -Action Generate` | Pass; generated `.sln` absent and UBT `.ignore` removed |
| Editor compiles and loads | Development Editor non-UBA build plus native test launch | Pass under Windows Smart App Control |
| Include/module boundaries hold | `BuildProject.ps1 -Action IWYU` with `-DisableUnity -NoPCH -NoUBA` | Pass, 32 direct actions |
| Game configurations compile | Full `-Rebuild -NoUBA` for Development and Shipping | Pass, 25 direct actions each |
| Native M1 tests pass | `Artifacts/Automation/20260905_135712_356/index.json` | 2 succeeded, 0 failed, 0 not run |
| Test map is stable | PIE stability run | 10 minutes 9 seconds; no error, ensure, fatal, or crash |
| Development package succeeds | `BuildProject.ps1 -Action Package -Configuration Development` | 516 cooked packages; 0 errors, 0 warnings |
| Packaged scene launches | `M1Review_NoUBA_20260905_1400/Windows/M1-Smoke.log` | `L_VerticalSlice` loaded; game systems initialized; clean exit |
| Tooling is absent from players | Development and Shipping `.target` receipt inspection | 0 MCP/toolset references in both |
| Assets are recoverable through LFS | `git lfs fsck` and map attributes | Pass; both `.umap` files use LFS |
| Repository hygiene holds | credential/private-path scan and `git diff --check` | 0 signatures, 0 private paths, 0 diff errors |
| Editor control surface is closed | process and `127.0.0.1:8100` listener inspection | 0 relevant processes, 0 listeners |

## Clean-clone reproduction

The committed baseline was cloned with `--no-local` into `C:\Users\Ilan\AppData\Local\Temp\RomanEmpireGame-M1-0b74ad1-20260905\repo`. The retained clone is local evidence and is not part of the repository.

| Check | Clean-clone result |
|---|---|
| Commit and source state | Exact HEAD `0b74ad1566306d7487629812de68bc6abd7f0594`; `git status` clean |
| LFS materialization | `git lfs pull` and `git lfs fsck` pass; both maps have the reviewed binary sizes and SHA-256 hashes |
| Repository/IDE | Both validators pass; VS Code generation passes; no root `.ignore` or legacy `.sln` remains |
| Native builds | Development Editor, strict non-unity/no-PCH IWYU, Development Game, and Shipping Game pass through `-NoUBA` |
| Native automation | `Artifacts/Automation/20260905_142850_082/index.json`: 2 succeeded, 0 failed, 0 not run |
| Package | `Artifacts/Packages/M1CleanClone`: BuildCookRun succeeds and cooks 516 packages |
| Packaged smoke | Loads `L_VerticalSlice`; initializes framework, terrain, buildings, 15 Roman units, and 10 Carthaginian units; exits through `ConsoleCtrl RequestExit` with 0 forced process stops and 0 failure signatures |
| Shipping boundary | Development and Shipping player receipts contain 0 MCP/toolset references |
| Closed surface | 0 relevant Unreal processes and 0 connections/listeners on port 8100 after validation |

The clean-clone cook emitted one transient `SE_HOST_NOT_FOUND` socket warning while optional online resolution was unavailable; UAT completed with exit code 0, all 516 required packages cooked, and the standalone smoke run was clean. The original reviewed package evidence remains 0 errors/0 warnings.

## Local artifact identity

These hashes identify the exact local evidence reviewed. Test/package outputs are intentionally ignored; the two maps are proposed LFS source assets.

| Artifact | Size | SHA-256 |
|---|---:|---|
| `Content/Maps/L_Testbed.umap` | 49,094 bytes | `78C3647777936D51B9AB5C7D26E70698216362DE9CF7D9ACCB6E8ADE403F3B30` |
| `Content/Maps/L_VerticalSlice.umap` | 49,226 bytes | `AFF27573AA036211328DE38887653BD590D80F45E345C1C63F82E42B26028F5C` |
| Packaged bootstrap `RomanEmpireGame.exe` | 171,520 bytes | `45BCB212C3799577D8F706A5D30401F75682E7541B33FFE39EFBB43FE24C0A0E` |
| Packaged Development game executable | 332,207,104 bytes | `7AEC135D70821F03B87DB4142704571FB724A227DAF853C3BCC5995DD5C89A6E` |
| Clean-clone packaged bootstrap `RomanEmpireGame.exe` | 171,520 bytes | `45BCB212C3799577D8F706A5D30401F75682E7541B33FFE39EFBB43FE24C0A0E` |
| Clean-clone packaged Development game executable | 332,218,368 bytes | `1ABB30AFE6CDC034C1E93B3E1C19A8C857015227182FC2871DC61AF60838D1D4` |

The maps are proposed source assets and will become LFS pointer entries when staged. The packaged executables are not proposed source files.

## Important findings and decisions

- Windows Smart App Control rejected a locally built unsigned Editor DLL produced through UBA with application-control error 4551. The same source rebuilt with `-NoUBA` loaded correctly. The supported default therefore preserves Windows security and disables UBA; `-UseBuildAcceleration` is an explicit opt-in for compatible machines.
- The runtime module root is private. Only the faction/resource contract required across module boundaries is exported under `Source/RomanEmpireGame/Public`.
- Native content validation resolves private gameplay framework classes through reflected `/Script/RomanEmpireGame` paths instead of importing private runtime headers.
- UBT's VS Code generator creates a root `.ignore` that hides `Content`, `Tools`, `scripts`, and agent context from ordinary repository searches. The generation entrypoint removes that generated artifact and validation rejects its presence.
- Failed building confirmation no longer leaves the controller and placement component in conflicting modes.

## Known limitations accepted only for M1

- The project is a compiling deterministic recovery baseline, not yet the complete vertical-slice game.
- Resources, turns, territory, combat, training, victory, UI, and save ownership remain duplicated or incomplete. M2 is specifically responsible for canonical state and data contracts.
- The current production presentation is placeholder geometry plus Canvas HUD. Production UMG, authored art, audio, animation, and accessibility remain later milestones.
- Only two native M1 tests exist. Actor-level Functional Tests, UMG automation, performance captures, full-loop packaged assertions, and CI are later plan requirements.
- Supabase remains disabled; local versioned save support is not implemented yet.
- The retained clean-clone evidence is intentionally outside the repository and can be removed when it is no longer needed.

## Reviewer commands

Fast structural review:

```powershell
git status --short
git diff --check
.\Tools\validate_repository.ps1
.\BuildProject.ps1 -Action Validate
```

Build and runtime evidence:

```powershell
.\BuildProject.ps1 -Action Generate
.\BuildProject.ps1 -Action Editor
.\BuildProject.ps1 -Action IWYU
.\BuildProject.ps1 -Action Game -Configuration Development
.\BuildProject.ps1 -Action Game -Configuration Shipping
.\BuildProject.ps1 -Action Test
.\BuildProject.ps1 -Action Package -Configuration Development
```

The authorized final M1 reproduction completed successfully from the dedicated commit, including `git lfs pull`, validation, build, test, package, packaged smoke, receipt inspection, and shutdown hygiene. The original source tree remained clean throughout. No push was performed.
