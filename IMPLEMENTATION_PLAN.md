# Roman Empire Game Implementation Plan

Status: implementation active; M0–M2 and M3.2a complete; M3.1 routed policy and remaining M3 work open
Audit baseline: commit `89d8d66e5051d303dea9c965317cdb961d8a52f6`
Plan date: 2026-09-03

## 1. Purpose

This document converts the repository audit into an ordered implementation program. It is the execution source of truth for recovering the project, producing a playable vertical slice, and then extending it without recreating the duplicated state and disconnected systems found in the audit.

The plan deliberately separates the north-star game from the first product milestone.

North star:

> A single-player Windows game in which the player manages a Roman campaign, develops settlements and commands armies as an RTS, and can directly possess a soldier during tactical battles.

First playable vertical slice:

> As Rome, build one barracks, recruit a small force, capture one Carthaginian objective, possess one legionary during the battle, return to command mode, resolve the territorial result, save, exit, and restore the game.

The vertical slice is complete only when that loop works in a fresh checkout and a packaged build. A collection of individually demonstrable systems is not sufficient.

## 2. Scope boundaries

### Included in the first complete loop

- Windows single-player.
- Rome and Carthage.
- One deterministic tactical map and a small campaign of approximately nine territories.
- Strategic, tactical, and possessed-soldier modes.
- One functional settlement objective, one barracks, and at most two useful unit archetypes per faction.
- Selection, formation movement, attack, stop, hold, building placement, recruitment, territory resolution, and local save/load.
- Production UMG, basic audio/animation feedback, automated validation, and a packaged build.

### Explicitly deferred

- Multiplayer or network replication as a product feature.
- A literal one-to-one Mediterranean world.
- Six complete factions.
- Deep diplomacy, trade, naval warfare, sieges, or family/political simulation.
- Thousands of fully simulated Actor-based soldiers.
- Cloud saves, public leaderboards, and account systems.
- Gameplay Ability System unless combat complexity later justifies it.
- Mass Gameplay, World Partition, and PCG as mandatory foundations; they are adopted only when measured requirements justify them.

## 3. Adopted implementation decisions

These milestone M0 decisions were adopted in `PRODUCT_BRIEF.md` on 2026-09-03. Reopening one requires recording the compatibility and rework impact before implementation changes direction.

| ID | Adopted decision | Why it matters |
|---|---|---|
| D-01 | Use the Unreal Engine 5.8 Launcher build. | The repository had no binary assets requiring 5.7 recovery, and no engine was installed at audit time. |
| D-02 | Start from deterministic bootstrap content; quarantine any later recovered legacy content for provenance and compatibility review. | Worktree, history, and accessible standard user folders contained no Unreal binary/source-media assets. |
| D-03 | Campaign data plus authored tactical levels, with a seamless-looking transition. | A literal campaign-to-FPS physical scale would dominate production cost and performance risk. |
| D-04 | `0.0 = far/strategic`, `1.0 = close`; FPS is entered explicitly. | This matches the intended camera semantics and avoids accidental possession during scrolling. |
| D-05 | 60 FPS at 1080p on the recorded Lenovo/RTX reference laptop, with 50 slice units and a separate 100-unit stretch test. | Unit architecture should be selected from a measurable target rather than an aspirational army size. |
| D-06 | Historically inspired. | Recognizable history guides content while gameplay clarity remains practical for the first slice. |
| D-07 | Version every save; migrate or explicitly invalidate development saves with a visible compatibility result. | Campaign data will change substantially during recovery. |
| D-08 | Gate Epic's built-in UE 5.8 Unreal MCP behind a compiling baseline; keep it editor-only, manual, loopback-only, and absent from Shipping. | Epic supports Codex, while the editor service remains a privileged experimental surface. |

D-01 and D-02 are resolved for the accessible workstation and repository history: UE 5.8 is selected and no legacy Unreal binary assets were found. Inaccessible external content is not a dependency; anything recovered later follows the quarantined intake process in `ASSET_INVENTORY.md`.

## 4. Engineering invariants

Every implementation ticket must preserve these rules:

1. A domain has one authoritative owner. UI and actors may cache presentation data, but may not create competing resource, turn, ownership, training, or victory state.
2. Definitions and state are different types. Unit/building/faction definitions are immutable data assets; balances, health, queues, and ownership are runtime state.
3. Resource amounts used as costs, deltas, or production default to zero. Starting balances are explicit data.
4. UI observes state through events or queries; UI never corrects or mutates mirrored gameplay values on tick.
5. Test and Remote Control functionality is excluded from Shipping runtime code.
6. No gameplay system performs `GetAllActorsOfClass` per unit per frame. Registries, perception, spatial queries, or squad controllers own discovery.
7. A fresh clone plus documented prerequisites must reproduce the editor, tests, and packaged build.
8. Maps, Blueprints, materials, and other source assets are version-controlled through Git LFS. Generated caches and packaged output remain ignored.
9. The offline game loop has no network dependency. Online features are an optional adapter around stable local state.
10. Each milestone ends with executable evidence. Logs, screenshots without assertions, or old reports do not prove completion.

## 5. Target architecture

```text
Primary Data Assets / Data Tables
  - unit, building, faction and territory definitions
                     |
Enhanced Input      v
      |        PlayerController ---- issues commands ----+
      |                                               Tactical session
      +---- mode-specific mapping contexts                 |
                                                          v
Campaign model <---- resolution result ---- squads / units / buildings
      |                                                   |
      +---- canonical turn, resources, ownership          +---- gameplay events
      |                                                               |
      +---- versioned SaveGame                                      UMG HUD
```

### Responsibility map

| Domain | Authoritative owner | Consumers |
|---|---|---|
| Session rules and win/loss orchestration | `ARomanEmpireGameMode` | GameState, campaign/tactical coordinators |
| Public current phase, turn, resources, and match state | `ARomanEmpireGameState` read/event projection; no mutable copies | UI, controller, tests |
| Campaign territories, diplomacy, armies, progression | Campaign model/subsystem | GameMode, save system, strategic UI |
| Input and player intent | `ARomanEmpirePlayerController` | Command/selection/possession systems |
| Tactical unit behavior | Squad/AI controller plus focused Actor components | GameMode, UI, battle resolver |
| Building placement and production | Building/production components | Controller commands, UI |
| Immutable content definitions | Primary Data Assets/Data Tables | Campaign and tactical factories |
| Presentation | UMG widgets | Reads state/events only |
| Persistence | Save subsystem and versioned `USaveGame` | Campaign and settings models |

Keep one runtime game module during recovery. Add a separate developer/test module for Automation Driver and test-only helpers. Split more runtime modules only after boundaries are stable and build-time or ownership benefits justify it.

## 6. Agent-assisted implementation toolchain

Skills are workflow instructions, not proof that the engine, plugin, application, or external service they describe is installed. They never replace milestone acceptance evidence or authorize a broader mutation than the active implementation ticket.

The following skills were installed globally for Codex on 2026-09-03, with `ue-input-system` added on 2026-09-06 for M2.6. They are not vendored into the game repository; their reviewed source revisions are recorded so another workstation can reproduce or re-audit the selection.

| Skill | Reviewed source | Intended use | Activation point / constraint |
|---|---|---|---|
| `find-skills` | [Vercel Labs](https://github.com/vercel-labs/skills/tree/435076e78988e1e6ec40d00b0b1d76bdbbc5419a/skills/find-skills) | Search the open skills ecosystem when a milestone needs a missing specialist workflow | Use only for a concrete gap; re-review third-party candidates before installing them. |
| `unreal-mcp` | [Epic Games](https://github.com/EpicGames/unreal-engine-skills-for-claude-code-plugin/tree/7e3b09bbf6d2984c155233f9d3de5fcf523d2d42/skills/unreal-mcp) | Navigate the live project; inspect/edit actors, Blueprints, UMG, materials, meshes, Data Assets, logs, tests, and screenshots | Installed and proven against UE 5.8.2 using the narrowed Editor-only toolsets. The skill itself does not install or connect the engine plugin. |
| `unreal-cpp-gameplay` | [GameDev Skills](https://github.com/gamedev-skills/awesome-gamedev-agent-skills/tree/7110607ab816ece9669274bc84937857a8819796/skills/unreal/unreal-cpp-gameplay) | C++ reflection, gameplay framework responsibilities, component ownership, and module dependencies | M1–M4; repository evidence and current Epic docs override generic examples. |
| `unreal-packaging` | [GameDev Skills](https://github.com/gamedev-skills/awesome-gamedev-agent-skills/tree/7110607ab816ece9669274bc84937857a8819796/skills/unreal/unreal-packaging) | UAT cook/stage/package workflow and packaged-build validation | M1 and M7; always run the packaged executable after a successful cook. |
| `ue-project-context` | [Quod Soler](https://github.com/quodsoler/unreal-engine-skills/tree/231c8571be6f3335685edc566a28ec6f9621361d/skills/ue-project-context) | Maintain the repository-specific UE context read by the other UE skills | Active now through `.agents/ue-project-context.md`; update after M0 and architecture migrations. |
| `ue-module-build-system` | [Quod Soler](https://github.com/quodsoler/unreal-engine-skills/tree/231c8571be6f3335685edc566a28ec6f9621361d/skills/ue-module-build-system) | UBT modules, targets, plugins, IWYU, and build-error diagnosis | M1 and M7; validate all advice against the selected UE version. |
| `ue-testing-debugging` | [Quod Soler](https://github.com/quodsoler/unreal-engine-skills/tree/231c8571be6f3335685edc566a28ec6f9621361d/skills/ue-testing-debugging) | Native automation tests, Functional Tests, logging, assertions, and profiling | Tests inside M1–M7; the dedicated module design must be proven by UBT. |
| `ue-data-assets-tables` | [Quod Soler](https://github.com/quodsoler/unreal-engine-skills/tree/231c8571be6f3335685edc566a28ec6f9621361d/skills/ue-data-assets-tables) | Primary Data Assets, Data Tables, soft references, Asset Manager, and content validation | M2, M5, and M6; avoid synchronous loads in active gameplay. |
| `ue-ui-umg-slate` | [Quod Soler](https://github.com/quodsoler/unreal-engine-skills/tree/231c8571be6f3335685edc566a28ec6f9621361d/skills/ue-ui-umg-slate) | Replace the Canvas/UMG split with a production UMG lifecycle and correct input routing | M2 and M6; Common UI/MVVM remain optional decisions, not automatic additions. |
| `ue-input-system` | [Quod Soler](https://github.com/quodsoler/unreal-engine-skills/tree/231c8571be6f3335685edc566a28ec6f9621361d/skills/ue-input-system) | Enhanced Input assets, mapping ownership, trigger lifecycle, modifiers, held-key suppression, and legacy migration | Added for M2.6; documentation only, verified against installed UE 5.8 APIs and behavioral tests. |
| `security-best-practices` | [OpenAI curated](https://github.com/openai/skills/tree/49f948faa9258a0c61caceaf225e179651397431/skills/.curated/security-best-practices) | Secure-by-default review for supported Python, JavaScript/TypeScript, and Go tooling | M7/M8 scripts or services only; it does not cover the core UE C++ module. |
| `security-threat-model` | [OpenAI curated](https://github.com/openai/skills/tree/49f948faa9258a0c61caceaf225e179651397431/skills/.curated/security-threat-model) | Repository-grounded threat model for MCP, CI, credentials, Supabase, and distribution boundaries | Run by explicit request before enabling online/MCP exposure or release infrastructure. |
| `gh-fix-ci` | [OpenAI curated](https://github.com/openai/skills/tree/49f948faa9258a0c61caceaf225e179651397431/skills/.curated/gh-fix-ci) | Inspect and diagnose failing GitHub Actions PR checks | M7 only, after a PR-based GitHub Actions workflow exists and `gh` is authenticated. |
| `imagegen` | Built-in Codex skill | Concept art, mood boards, temporary icons, and bitmap references | M6 after an asset brief; outputs require provenance/license review and are not automatically production-ready. |

### Unreal MCP gate

Epic's UE 5.8 `ModelContextProtocol` plugin is the selected live-editor integration because Epic documents Codex as a supported client. It is experimental and grants privileged editor access. The broad `AllToolsets` plugin was tested and then disabled; the project now enables only `EditorToolset` and `AutomationTestToolset` for Editor targets. The global `unreal-mcp` skill is the operating procedure, while the loopback server is started only for an intentional session and is otherwise stopped.

M1 must establish all of the following before MCP-driven edits:

1. UE 5.8 is the selected and compiling baseline.
2. `ModelContextProtocol` and either `AllToolsets` or an explicitly narrowed toolset set are enabled for Editor work without a dependency from the game runtime module.
3. Codex configuration is generated with `ModelContextProtocol.GenerateClientConfig Codex` or reviewed by hand if `.codex/config.toml` already exists.
4. The server binds only to `127.0.0.1`, has no source-controlled auto-start, and is started only for an active editor session.
5. Game/Client/Server targets deny or otherwise exclude the tooling, and a Shipping inspection proves there is no listening MCP/Remote Control server.
6. A clean Git recovery point exists before bulk asset work; Unreal calls are sequential; compilation and PIE are idle; every result is checked; assets are saved and validated afterward.

If UE 5.7 must be retained, continue with filesystem/UBT/editor-manual workflows during recovery. Selection of a third-party UE 5.7 MCP requires its own code, network, license, update, and packaged-build exclusion review.

### Asset capability policy

- Use `imagegen` for exploratory 2D work and temporary bitmap references, never as an implicit source of final licensed game content.
- Use Unreal MCP, once gated, to inspect/import/configure committed Unreal assets and build repeatable Data Asset, material, UMG, and test-map workflows.
- Do not install a Blender or Substance MCP until that application is selected and installed. Those integrations expose arbitrary application scripting and require the same recovery-point, loopback, provenance, and export-validation controls as Unreal MCP.
- Track every production asset's source, license, modifications, engine import settings, and LFS path. A successful AI/DCC operation is not asset acceptance; the UE import, collision, LOD, UV, material, animation, cook, and performance checks are acceptance.
- Prefer Fab/Marketplace or custom-authored assets with clear commercial rights. Do not use search-result popularity as evidence of asset licensing or production fitness.

### Capability activation by milestone

| Milestone | Required skills/capabilities |
|---|---|
| M0 | `ue-project-context`, `find-skills` only for identified gaps; manual asset/license inventory |
| M1 | `ue-module-build-system`, `unreal-cpp-gameplay`, `unreal-packaging`, `ue-testing-debugging`; gated `unreal-mcp` after the project compiles |
| M2 | `ue-data-assets-tables`, `ue-ui-umg-slate`, `unreal-cpp-gameplay`, gated `unreal-mcp` |
| M3–M5 | `unreal-cpp-gameplay`, `ue-testing-debugging`, `ue-data-assets-tables`, gated `unreal-mcp` |
| M6 | `ue-ui-umg-slate`, `ue-data-assets-tables`, `imagegen`, gated `unreal-mcp`; optional DCC decision |
| M7 | `unreal-packaging`, `ue-testing-debugging`, `gh-fix-ci`; security skills when explicitly reviewing the new boundary |
| M8 | `security-threat-model` before online exposure; `security-best-practices` for supported service/tool languages |

## 7. Milestone roadmap

### M0 — Product contract and asset inventory

Goal: remove decisions that would cause rework during repository recovery.

Status: complete for the current workstation on 2026-09-03. Evidence is recorded in `PRODUCT_BRIEF.md`, `ASSET_INVENTORY.md`, `LICENSE.txt`, and `THIRD_PARTY_NOTICES.md`.

Work packages:

- M0.1: Search known development machines and backups for `NewMap.umap`, Blueprint assets, input assets, UI assets, materials, audio, meshes, and animations.
- M0.2: Record asset origin, license, engine version, dependencies, and whether the asset is source or generated.
- M0.3: Resolve D-01 through D-08.
- M0.4: Write the vertical-slice player journey and non-goals in `PRODUCT_BRIEF.md`.
- M0.5: Record reference hardware and measurable performance targets.
- M0.6: Choose a code license and establish a third-party attribution policy before public distribution.

Evidence and exit gate:

- Product brief approved.
- Asset inventory exists and every recovered asset has a known source/license.
- Engine and zoom conventions are explicit.
- No unresolved decision can invalidate the recovery approach.

Estimated effort: 0.5–2 focused days, excluding time needed to find external assets.

### M1 — Reproducible repository and compiling baseline

Goal: make a clean checkout build, open, test, and package without private machine state.

Completed 2026-09-05: source-asset/LFS and formatting policy, the UE 5.8 migration, portable build entrypoint, plugin quarantine, Editor-only native tests, compile/IWYU fixes, explicit framework defaults, deterministic bootstrap maps, and narrow Unreal MCP integration are implemented. VS Code generation, full non-UBA Development/Shipping rebuilds, strict IWYU, two native tests, ten-minute PIE stability, Development packaging, and packaged-scene smoke launch all succeed while Windows Smart App Control remains enabled. The user accepted the human review gate, and approved commit `0b74ad1566306d7487629812de68bc6abd7f0594` reproduced from a clean `--no-local` clone through LFS pull, validation, all required builds, tests, package, smoke, receipt inspection, and clean shutdown. No push was performed. M2 is authorized and active.

#### M1.1 Source-control policy

- Stop ignoring `.uasset` and `.umap` source files.
- Add `.gitattributes` with Git LFS rules for Unreal binary assets and other large binary source formats.
- Keep `Binaries`, `DerivedDataCache`, `Intermediate`, `Saved`, staged builds, packaged builds, and generated solution files out of source control.
- Decide whether the existing `.sln` should be removed from tracking after project-file generation is documented.
- Add `.editorconfig`, formatting policy, repository prerequisites, and contribution/build documentation.
- Add the selected code license and third-party attribution file.

Decision: retain the existing `.sln` only as a transitional file until `BuildProject.ps1 -Action Generate` succeeds under UE 5.8; then remove it from tracking before the M1 exit gate because generated IDE files are not authoritative source.

#### M1.2 Engine and build tooling

- Update `.uproject`, Target files, include-order setting, and plugin versions together if UE 5.8 is selected.
- Replace hard-coded engine/project locations with `BuildProject.ps1`, accepting `-EngineRoot` and optionally discovering an Epic Launcher installation.
- Provide commands for Editor Development build, Game Development build, tests, cook, and Windows package.
- Fail early with actionable diagnostics when the engine, compiler workload, SDK, or LFS content is missing.
- Keep `.vsconfig`, but validate its component identifiers against the selected engine/toolchain.

IDE decision: VS Code is the selected local editor. `BuildProject.ps1 -Action Generate` therefore requests UE 5.8's native `-VSCode` project-file generator by default, with `-IDE VisualStudio` retained as an explicit alternative. Generated `.vscode`, workspace, compile-command, and solution files remain disposable and untracked. VS Code does not replace the MSVC/Windows SDK build toolchain. The checked-in `.vsconfig` is intentionally Build-Tools-compatible and pins the MSVC 14.44 LTSC, matching ATL, .NET Framework 4.8 SDK required by SwarmInterface, and Windows SDK 10.0.22621 components suggested by the installed UE 5.8.2 `Windows_SDK.json`; full-IDE-only Unreal extensions were removed because VS Code supplies the editing experience.

Build-executor decision: UBA is disabled by default in the repository entrypoint because Windows Smart App Control on the audited workstation rejects UBA-produced local unsigned Editor DLLs. A full `-NoUBA -Rebuild` produced a DLL accepted by the same policy and loaded the project successfully, so `-UseBuildAcceleration` remains an explicit opt-in only for machines whose application-control policy accepts UBA output. Windows security controls must not be disabled to build the project.

#### M1.3 Confirmed compile blockers

- Remove the duplicate `ABuildingBase::SetBuildingTypeData` declaration/definition and retain one fully implemented function.
- Remove duplicate `AUnitBase` health/morale accessors.
- Replace GameMode's direct `DisplayGold` access with canonical resource state and a public/event-driven UI update.
- Replace direct `HUDSelectedBuilding` access with the public setter while the Canvas HUD still exists.
- Implement or remove `ARomanEmpireHUD::ShowFPSOverlay`.
- Correct the Supabase module include path and log category.
- Remove Automation Driver includes from runtime widgets.
- Audit forward declarations/includes with IWYU enabled where practical.
- Either enable and correctly depend on Procedural Mesh Component or remove it from the recovery build until its generators are selected for use.

Implementation note: the duplicate declarations, illegal HUD access, missing HUD method, Supabase include/log/config state, runtime Automation Driver includes, and Procedural Mesh plugin declaration were corrected and compiler-proven. Unused module dependencies were removed, public/private dependencies were narrowed, UE 5.8's current `IWYUSupport.Full` policy was enabled in both modules, direct engine/world/collision/input/material/JSON includes were made explicit, and parent-relative includes were removed. The resource contract used by the test module was moved under `Public/Faction` and exported with the runtime API. The `IWYU` non-unity/no-PCH build and documented automation-test invocation now succeed; each test run gets a fresh report and the launcher rejects missing, empty, or failed results.

#### M1.4 Plugin and configuration cleanup

- Disable Remote Control and its web UI in normal editor/runtime defaults.
- The legacy Remote Control harness has been retired; do not restore its server or hard-coded object paths as a default workflow.
- Keep Automation Driver disabled. If later UMG automation requires it, add the dependency only to an Editor/test module and prove Shipping exclusion.
- Enable Python Editor scripting only if the asset scripts are retained and made useful; it must not be a packaged-game dependency.
- Configure Epic's `ModelContextProtocol` with an explicitly narrowed Editor-only toolset set under the Unreal MCP gate in Section 6, generate the Codex client configuration, and prove Game/Shipping exclusion. `AllToolsets` must remain disabled unless a later review justifies its broader surface.
- Keep MCP server startup manual or per-user. Do not source-control an always-on server setting.
- If UE 5.7 is retained, defer MCP rather than adding an unreviewed bridge during build recovery.
- Remove unused plugins after a reference audit.
- Replace Supabase placeholder detection with explicit disabled/configured state, while keeping the backend disabled in the slice.

#### M1.5 Deterministic bootstrap content

- Create and commit one minimal `L_Testbed` map and one `L_VerticalSlice` map.
- Assign GameMode, controller, GameState, and HUD deliberately rather than relying on fallback defaults.
- Use committed placeholder assets. Runtime primitive generation may remain temporarily but must be deterministic through a fixed seed.
- Validate every soft object path during an automated content-validation step.

Evidence and exit gate:

- A clean clone with Git LFS fetches all required content.
- `RomanEmpireGameEditor` builds in Development.
- The deterministic test map opens without missing asset/class warnings.
- PIE runs for ten minutes without an ensure/crash.
- UAT cooks and packages a Windows Development build.
- A packaged smoke launch reaches the main tactical scene.
- Build instructions succeed without editing a hard-coded path.

Implementation evidence recorded 2026-09-05:

- Unreal MCP created and saved `/Game/Maps/L_Testbed` and `/Game/Maps/L_VerticalSlice`; both maps explicitly use `ARomanEmpireGameMode`, force dynamic lighting, pass Map Check, and load through the native content-path test.
- Runtime primitive generation uses the fixed seed `1337`, and the GameMode explicitly assigns the pawn, controller, GameState, and HUD classes.
- Repository validation, VS Code generation, Development Editor, Development Game, Shipping Game, and the strict non-unity/no-PCH IWYU build succeed with UE 5.8.2 and Visual Studio Build Tools 2022 17.14.
- The generated legacy `RomanEmpireGame.sln` was removed after VS Code generation succeeded; repository validation now rejects a tracked generated solution that remains present. The generation entrypoint also removes UBT's root `.ignore` artifact because it hides `Content`, `Tools`, `scripts`, and agent context from normal repository audits.
- The runtime module root remains private, while cross-module contracts are exported only under `Public`; the content-path test validates private gameplay framework classes through reflected `/Script/RomanEmpireGame` class paths rather than importing runtime-private headers.
- Failed building confirmation no longer exits placement mode; the controller clears placement mode only after `ConfirmPlacement()` succeeds.
- Windows Smart App Control rejected a UBA-produced unsigned Editor DLL with application-control error 4551. Without disabling that protection, a full `-NoUBA` rebuild loaded successfully; the repository now defaults direct UBT and UAT builds to non-UBA and exposes acceleration only as explicit `-UseBuildAcceleration` opt-in.
- `RomanEmpireGame.Rules.Resources.AffordabilityAndDeduction` and `RomanEmpireGame.Content.RequiredPathsAndMapDefaults` both pass; the latest exported report contains two successes and zero failures.
- `L_Testbed` ran in PIE for 10 minutes 9 seconds with no error, ensure, critical error, fatal error, or crash after PIE startup.
- The final UAT BuildCookRun propagated `-NoUBA`, compiled every required action through the direct executor, cooked 516 packages with 0 errors/0 warnings, and produced `Artifacts/Packages/M1Review_NoUBA_20260905_1400`. Its unattended `-NullRHI` smoke launch loaded `/Game/Maps/L_VerticalSlice`, instantiated `ARomanEmpireGameMode`, initialized the HUD/controller, terrain, buildings, and both armies, produced no error/fatal/ensure/application-control signature, and exited cleanly.
- Development and Shipping target receipts contain zero references to `ModelContextProtocol`, `ToolsetRegistry`, `EditorToolset`, `AutomationTestToolset`, or `AllToolsets`; no MCP listener remains after the intentional Editor session.
- Human review accepted on 2026-09-05. The dedicated local commit is `0b74ad1566306d7487629812de68bc6abd7f0594`.
- Clean-clone proof completed at `C:\Users\Ilan\AppData\Local\Temp\RomanEmpireGame-M1-0b74ad1-20260905\repo`: exact commit and clean status, LFS hashes reproduced, validators/VS Code generation passed, Editor/IWYU/Development/Shipping builds passed, 2/2 tests passed, 516 packages cooked, and the archived player loaded `L_VerticalSlice` before a graceful close with zero failure signatures. Both player receipts remained free of MCP/toolset modules. One transient optional host-resolution warning occurred during this clean-clone cook; UAT exited successfully and the original reviewed package retained its 0-error/0-warning evidence.
- M1 is closed. The user authorized continuation into M2; no remote push was requested or performed.

Estimated effort: 2–5 focused days after asset and engine decisions.

### M2 — Canonical state and data architecture

Goal: eliminate competing models before rebuilding gameplay on top of them.

#### M2.1 Resource model

- Replace the overloaded `FFactionResources` use with zero-default resource amounts/deltas and explicit starting-resource definitions.
- Make Faction/Campaign state the only resource wallet.
- Remove `GameMode::PlayerGold` and HUD-owned resource state as authorities.
- Route affordability and atomic deduction through one transaction API returning a typed failure reason.
- Ensure building placement and training reserve/deduct resources exactly once.

Implementation evidence recorded 2026-09-05:

- `FFactionResourceAmounts` and `FFactionResourceDelta` now default every field to zero; explicit Rome, Carthage, and Gaul starting wallets are copied into the sole runtime wallet in `AFactionManager`.
- `TrySpendFactionResources` combines affordability validation and all-resource deduction into one atomic request with typed failure reasons and before/after balances. Rejected costs and deltas leave the complete wallet unchanged; successful nonzero changes broadcast `OnFactionResourcesChanged`.
- GameMode and Canvas HUD no longer own or synchronize resource copies. HUD presentation reads the canonical faction wallet, and the old `PlayerGold`, `SubtractGold`, `UpdateHUDResources`, and `Display*` paths are absent from runtime source.
- Building confirmation and Barracks order acceptance each contain exactly one canonical spend call. Placement uses deferred spawning so a rejected transaction creates no live building; the controller delegates training state, timing, queueing, cost, and spawn responsibility to `ABarracks`.
- Cancellation currently consumes the accepted training reservation; partial/full refunds remain an explicit later production-policy decision rather than an implicit second transaction.
- Repository validation enforces the resource types, rejects the legacy resource/authority paths, and asserts one placement spend plus one training spend call.
- UE 5.8.2 Development Editor, strict non-unity/no-PCH IWYU, Development Game, and Shipping Game builds pass. The exported automation report at `Artifacts/Automation/20260905_150636_277` contains 3 successes and 0 failures.
- Development BuildCookRun cooked 516 packages into `Artifacts/Packages/M2_1`; the archived player loaded `L_VerticalSlice`, initialized the gameplay framework/managers/armies, closed through the viewport, left no game process, and produced 0 smoke failure signatures.
- M2.1 is complete; its canonical resource contract is the foundation used by later M2 packages.

#### M2.2 Turn and phase model

- Retain one canonical turn counter.
- Define an ordered turn pipeline: validate request, production, upkeep, AI actions, conflict resolution, victory evaluation, increment, event broadcast, autosave.
- Define `Strategic`, `Tactical`, and `Possessed` modes independently of raw zoom thresholds.
- Store zoom thresholds/configuration in one location and update documentation/tests from it.

Implementation evidence recorded 2026-09-05:

- `ACampaignManager` is the sole runtime owner of `CurrentTurn`; GameMode and HUD expose/read that value without maintaining a synchronized copy.
- `RomanEmpireCampaign::GetCanonicalTurnPipeline()` defines the nine required stages in one ordered contract. `ProcessTurn()` validates its dependencies, executes production, upkeep, AI actions, conflict resolution, victory evaluation, increment, completion broadcast, and autosave request in that exact order, and returns `FCampaignTurnResult` with a typed failure and failed-step value.
- Enemy strategic unit movement now belongs to the AI stage instead of a second GameMode turn path. Upkeep and conflict resolution are explicit no-op policy seams until their later milestone rules exist.
- `EGameplayMode` is the explicit `Strategic`/`Tactical`/`Possessed` authority. Camera framing may choose Strategic or Tactical exploration presentation, but first-person framing alone cannot imply possession; entering and leaving possession changes the gameplay mode explicitly.
- `FZoomConfiguration` is the only owner of zoom limits, semantic boundaries, default tactical framing, user input step, and classification. The camera is the only owner of current/target zoom state; controller, GameMode, and HUD no longer keep parallel zoom values or hard-coded thresholds.
- Repository validation enforces one turn counter, the exact pipeline cases, the public mode/config contracts, camera-owned zoom state, and removal of the legacy `EGamePhase`, `bIsInFirstPersonMode`, and `ZOOM_*` paths.
- UE 5.8.2 Development Editor, strict non-unity/no-PCH IWYU, Development Game, and Shipping Game builds pass. The post-change automation report at `Artifacts/Automation/20260905_162130_576` contains 5 successes and 0 failures, including canonical turn-order and gameplay-mode/zoom-boundary tests.
- Development BuildCookRun compiled, cooked 516 packages, staged, packaged, and archived successfully into `Artifacts/Packages/M2_2`. An Editor-hosted `-game` smoke loaded `/Game/Maps/L_VerticalSlice`, instantiated `ARomanEmpireGameMode`, brought the world up for play, initialized the engine, and produced 0 smoke failure signatures.
- A direct smoke of the newly built unsigned player is blocked before Unreal starts by this workstation's Smart App Control policy (Code Integrity events 3033/3077, policy `{0283ac0f-fff1-49ae-ada1-8a933130cad6}`); the same machine still launches the previously trusted M1/M2.1 binaries. Security was not weakened. Signing or a controlled build runner is required to restore this specific packaged-player gate.
- The autosave stage currently emits the canonical request and reaches the existing persistence placeholder. Actual versioned local `USaveGame` serialization remains deliberately assigned to M5.4.
- M2.2 is complete; M2.3 is the active implementation package.

#### M2.3 Territory and diplomacy model

- Give every territory a stable ID and display definition.
- Store ownership once in campaign state; territory actors/widgets present that state.
- Replace the diplomacy list with a normalized faction-pair key or symmetric matrix.
- Define legal transitions and invariants for war, neutrality, alliance, and invalid/self relationships.
- Add setters/factories that fully initialize territories before they become observable.

Implementation evidence recorded 2026-09-05:

- `FTerritoryDefinition` gives every region a validated stable `FName` ID, display text, terrain/resource definition, and explicit starting owner. Empty IDs/names, invalid enums, negative production, and duplicate IDs are rejected before registration.
- `FTerritoryCampaignState`, owned by `AWorldMapManager`, contains the sole `TMap<FName, EFactionID>` runtime ownership store. Transfers return `FTerritoryOwnershipChangeResult` with previous/new owners and typed rejection reasons; successful changes update the matching actor presentation and broadcast the manager event.
- `ATerritoryRegion` no longer has an `OwnerFaction` field or ownership setter. It retains only its stable definition/ID handle and reads the canonical manager state when presenting ownership, color, production, and UI-facing data.
- Default territory creation now uses `SpawnActorDeferred`, assigns the complete validated definition and manager reference, registers canonical campaign state, and only then calls `FinishSpawningActor`. Existing authored actors must pass the same definition/duplicate-ID registration gate.
- Campaign production and conquest/defeat checks read WorldMapManager ownership. The disconnected FactionManager territory map was removed, and the AI action stage no longer adds a second territory-income calculation after canonical production.
- `FDiplomacyState` stores one relation per normalized unordered faction pair. Valid unstored pairs are neutral; `None`, out-of-range, and self pairs are invalid. Legal transitions are Neutral→War/Allied and War/Allied→Neutral, so direct War↔Allied jumps and repeated/invalid requests return typed failures without mutation.
- Repository validation enforces the single ownership map, presentation-only territory actor, deferred initialization order, normalized diplomacy state, canonical campaign ownership reads, and required automation coverage.
- The final automation report at `Artifacts/Automation/20260905_164215_075` contains 7 successes and 0 failures, including canonical ownership transfers/duplicate rejection and diplomacy symmetry/transition invariants. Development Editor, strict non-unity/no-PCH IWYU, Development Game, and Shipping Game builds pass.
- Development BuildCookRun cooked 516 packages with 0 errors/0 warnings and archived successfully into `Artifacts/Packages/M2_3`. The signed Editor-hosted `-game` smoke loaded `L_VerticalSlice`, created and registered all 9 prototype territories, initialized the faction/world/campaign managers, and produced 0 failure signatures. Direct execution of newly unsigned player hashes remains subject to the M2.2 Smart App Control limitation.
- M2.3 is complete and its evidence remains the baseline for M2.4.

#### M2.4 Data-driven definitions

- Create Primary Data Assets for faction, unit, building, and territory definitions.
- Remove repeated costs, durations, colors, display strings, and combat values from controller/HUD code.
- Use stable IDs in save files and runtime factories.
- Add content validation for duplicate IDs, missing assets, invalid costs, and invalid faction references.

Implementation evidence recorded 2026-09-05:

- Applying `ue-data-assets-tables` and the gated `unreal-mcp` workflow, the Editor created and saved 19 project-authored Primary Data Assets: 3 factions, 1 unit, 6 buildings, and 9 territories. The four asset classes, Asset Manager scan rules, and exact provenance are recorded in `ASSET_INVENTORY.md`.
- `URomanContentRegistrySubsystem` discovers the four Primary Asset types at GameInstance startup, validates the complete cross-asset graph, preloads only the configured factory classes, and exposes stable-ID and quick-slot lookups. Missing categories/assets/classes, duplicate stable/runtime IDs or quick slots, invalid values, unknown factions, and unknown trainable units fail closed with typed issues.
- Faction wallets, map territory placement/settlements, unit stats/combat/formation behavior, building costs/durations/visuals/production, menu strings, and faction colors now originate in the assets. PlayerController, Canvas HUD, UMG menu scaffolding, factions, world map, unit/building initialization, placement, and barracks training consume the same registry definitions instead of rebuilding them.
- Unit and building factories use deferred spawning plus stable `FName` definition IDs before actors become observable; the barracks queue also stores stable unit IDs. No local save serializer exists before M5.4, so that future versioned save contract will persist these same IDs rather than object pointers or enum-only identities.
- Unit/building data contracts moved under the runtime module's public boundary after the Editor test module exposed their former private-header dependency. Repository validation now enforces the four scan categories, all 19 assets, stable-ID registry use, removed legacy definition factories, and the two new automation scenarios.
- `Artifacts/Automation/20260905_190532_896` contains 9 successes and 0 failures. It proves live Primary Asset discovery/counts and canonical authored values, plus transient rejection of duplicate IDs, missing asset categories, negative costs, and invalid faction references.
- Development Editor, strict non-unity/no-PCH IWYU, Development Game, and Shipping Game builds pass. Development BuildCookRun cooked 535 packages with 0 errors/0 warnings and archived to `Artifacts/Packages/M2_4`; Development and Shipping receipts contain zero MCP/toolset references.
- `Artifacts/M2_4-EditorGame-Smoke.log` shows the registry initialized with exactly 3 factions, 1 unit, 6 buildings, and 9 territories before `L_VerticalSlice` brought up the three managers, both settlements, all territories, and both armies. The smoke exited normally with 0 project error/fatal/assert/ensure signatures; direct newly unsigned player execution remains subject to the documented Smart App Control limitation.
- M2.4 is complete; M2.5 is the active implementation package.

#### M2.5 UI and event consolidation

- Make UMG the production UI path.
- Keep Canvas HUD only as a temporary debug overlay, then remove its gameplay panels.
- Define events for selection, resources, phase, turn, production, possession, objectives, and victory.
- Eliminate tick-based UI resource correction and disconnected UI-owned counters.

Implementation evidence (completed 2026-09-06):

- The local player controller creates the native `URomanEmpireMainWidget` production tree. It supplies resources, mode/turn, objectives, selection, selected-barracks training, possession vitals, building choices, command buttons, and campaign outcome. No Widget Blueprint or external art dependency is required for this milestone.
- UMG subscribes to canonical gameplay-ready, selection, wallet/player-faction, mode, turn, production, possession, objectives, and outcome events. Construction/damage/death events update or prune the selected actor. Removing the root unsubscribes observers; reattaching catches up from authoritative queries without duplicate bindings or a UI tick.
- Canvas contains only an opt-in diagnostic overlay, disabled by default. Its gameplay panels, actor scans, hit-test rectangles, kill counter, and independent victory calculation are removed. The native UMG diagnostic view is hidden in Shipping.
- `FCampaignObjectiveState` is a read-only projection; campaign outcome remains owned by CampaignManager. Final-turn victory/defeat is retained instead of overwritten by a draw. Campaign/production operations guard reentrant requests and publish committed state; failed unit spawns emit `Failed`, never `Completed`.
- Building choices come from the canonical stable-ID registry. TRAIN routes through the controller to the selected eligible barracks; affordability is a query and the actual spend remains in the production command. Unit selection/vitals and possession/death notifications restore the camera and remove stale HUD state.
- Skills applied: `ue-ui-umg-slate` for native widget lifecycle and event routing; `unreal-cpp-gameplay` and `ue-data-assets-tables` for canonical commands/contracts; `ue-testing-debugging` for real-world PIE assertions; `ue-module-build-system` and `unreal-packaging` for non-unity and distribution gates.
- The native `RomanEmpireGame.UI.CanonicalEventDrivenPIE` test exercises the actual framework/managers and UMG tree in `L_Testbed`, with transient fixtures and no saved map mutations. It proves resource/objective synchronization, unit damage/selection, possession and camera restoration on death, selected-barracks training, root detach/reattach subscription counts, and economic victory on the final allowed turn. TRAIN uses virtual-user Slate pointer press/release routing through the live widget path, with the real `SButton` consuming both events; it does not broadcast UMG click delegates directly or move the OS cursor.
- The final automation report `Artifacts/Automation/20260906_230652_892` contains 10 successes, 0 failures, 0 not-run, and 0 in-process tests. Repository validation and `git diff --check` also pass. The pointer fixture explicitly initializes per-widget pointer entries: a navigation-only `FWidgetPath` is not a valid substitute for UE 5.8 event routing.
- Development Editor, strict non-unity/no-PCH IWYU, Development Game, and Shipping Game compile successfully. The final Development BuildCookRun compiled, cooked 535 packages with 0 errors/0 warnings, and archived to `Artifacts/Packages/M2_5` (`Artifacts/M2_5-Package.log`). Game receipts exclude `ModelContextProtocol`, `EditorToolset`, `AutomationTestToolset`, `AllToolsets`, and the Editor-only test module.
- The archived Development player passed both startup and rendered smoke checks. `Artifacts/M2_5-PackagedRendered-Smoke.log` proves `L_VerticalSlice`, canonical content/gameplay initialization, the production UMG root, normal shutdown, and no error/fatal/assert/ensure signatures. `Artifacts/M2_5-PackagedUMG.png` was inspected at 1920×1080: wallet/objective/diagnostic values agree, buttons have readable contrast, and help text fits. These are short startup smokes, not sustained gameplay/performance certification. The M2.2–M2.4 application-control restriction did not reproduce for this final player; no Windows security setting was changed and future hashes still require their own launch check.
- M2.5 is complete. M2.6 is the active implementation package; M2 as a whole remains open.

#### M2.6 Input contexts

- Create committed Enhanced Input assets for strategic, tactical, possession, and shared/system input.
- Activate mapping contexts on mode transitions.
- Prevent build/train/command actions when their context or UI mode is inactive.
- Document keyboard/mouse defaults and ensure remapping can be added without changing gameplay code.

Execution preparation from the M2.5 review:

- Skill intake: `find-skills` checked the directory and CLI search; `skill-installer` installed `ue-input-system` at pinned revision `231c8571be6f3335685edc566a28ec6f9621361d`. At review, the CLI reported 827 installs and the GitHub API reported 334 repository stars; these are popularity signals, not correctness guarantees. The selected folder contains only `SKILL.md` and `references/input-action-reference.md`, no executables. Its exact input/mode coverage and compatibility with the already selected UE context suite motivated the choice. Reproduce with the installer helper using `--repo quodsoler/unreal-engine-skills --ref 231c8571be6f3335685edc566a28ec6f9621361d --path skills/ue-input-system`.

- Author `DA_RomanInputConfig`, actions under `/Game/Input/Actions`, and `IMC_Strategic`, `IMC_Tactical`, `IMC_Possession`, and `IMC_Shared` under `/Game/Input/Contexts`. Separate camera pan from soldier movement, and map soldier attack/block directly in the possession context. Training remains a UMG command.
- Let the local controller own exactly one gameplay mapping context plus Shared, synchronized from gameplay readiness and canonical mode changes. Remove only project-owned mappings; cancel held input, selection drags, and placement before transitions.
- Consolidate eligibility checks shared by keyboard and UMG: tactical commands require an active campaign; build quick slots additionally require the open building menu; end turn is unavailable while possessed. Use the canonical player faction for command ownership.
- Fix the inherited RMB block-release defect: the old mapping bound press through Command but never its release. Handle Completed/Canceled and explicitly clear blocking on possession exit/death/context removal/focus loss. Remove dead legacy config mappings instead of advertising unsupported jump/sprint controls.
- Add asset/value validation and real local-player mapping/injection tests for exclusivity, held-key transitions, guarded shortcuts, and block cleanup. Record keyboard/mouse defaults in `INPUT_CONTROLS.md`. M2 remains open until these gates pass.

Implementation evidence (M2.6 completed 2026-09-07):

- Twenty-four saved, LFS-managed assets now supply 19 distinct actions, four mode/shared contexts and `DA_RomanInputConfig`. The primary config is scanned/cooked by Asset Manager and held with its small action/context dependencies by the local controller. No runtime-created action/context fallback or legacy action/axis mappings remains.
- `SetupInputComponent` loads and validates the config before binding. Gameplay-ready/mode events activate exactly one owned mode context plus Shared; unrelated subsystem mappings survive. Camera pan and soldier movement are separate cumulative Axis2D actions. Enter and exit possession use distinct Boolean actions.
- Controller eligibility is shared by keyboard and native UMG. Strategic/possessed modes reject tactical build/train/command requests; possession rejects turns; quick slots require the live visible building menu. The placement component also guards direct start/confirm requests and cancels ineligible previews. Commands use the canonical player faction.
- Blocking handles Completed/Canceled and clears on exit, death, mode changes and viewport key flushing. Cleanup is idempotent and preserves possession movement speed. Mode changes clear partial selection, menus, placement and pending movement; a data-driven held-digital-movement latch supplements UE 5.8's Boolean-only mapping-rebuild suppression. Held W cannot carry movement from camera to soldier or back.
- Skills applied: `ue-input-system` for mapping ownership and action lifecycles; `ue-data-assets-tables` for config/validation; `unreal-mcp` for sequential Editor asset creation/save/readback; `ue-ui-umg-slate` and `unreal-cpp-gameplay` for command guards and cleanup; `ue-testing-debugging`, `ue-module-build-system` and `unreal-packaging` for verification.
- `Artifacts/Automation/20260907_142956_715`: 13 successes, 0 failures/not-run/in-process. The three new input tests cover invalid references/types/identities/profile contracts, exact saved key defaults and modifier execution, and real Enhanced PlayerInput routing in an owned PIE session. The lifecycle test proves menu-gated shortcuts, one-shot turns, context exclusivity, foreign-context preservation, held F/W transitions, RMB release, focus flushing, external mode change and death cleanup. It does not claim complete OS-input/gamepad acceptance.
- Repository validation, Development Editor, strict non-unity/no-PCH IWYU, Development Game and Shipping Game pass. Development BuildCookRun archives `Artifacts/Packages/M2_6` with 559 packaged/cooked assets (566 processed including 7 platform skips), 0 cook errors and 0 warnings. Evidence: `Artifacts/M2_6-Tests.log`, `M2_6-IWYU.log`, `M2_6-Shipping.log`, `M2_6-Package.log`.
- The actual archived player exits 0 and renders `Artifacts/M2_6-PackagedUMG.png` at 1920×1080 with the complete updated control help visible. `Artifacts/M2_6-PackagedRendered-Smoke.log` proves default-map/gameplay/UMG startup, authored input initialization, normal shutdown and zero error/fatal/assert/ensure signatures. Player SHA256: `585A822D818FB9EC006826D8B174C5D4E4861A845CB622565CFE9419D5A4A653`. Development/Shipping receipts exclude MCP, editor toolsets and tests; no MCP listener remains.
- One long-idle asset-editor D3D12 session ended with a GPU-device error before its initial two assets were saved. They were recreated in a fresh null-RHI Editor session; all 24 assets were saved/read back and then loaded independently by tests/cook/player. The final short rendered player smoke passed. This does not constitute a long-duration GPU stability certification; no driver or Windows security policy was changed.
- Defaults and future remapping boundary are documented in `INPUT_CONTROLS.md`. A user-facing remapping screen/persistence and dynamic labels remain later UI work. M2.6 is complete, but M2's aggregate exit audit remains open: add explicit runtime-and-UI evidence for changing a definition asset, then reconcile all M2 acceptance criteria before starting M3.

Evidence and exit gate:

- Unit tests prove zero-default deltas, affordability, atomic spending, production, ownership changes, diplomacy symmetry, and turn order.
- A diagnostic screen shows the same canonical values as automation queries.
- Searches find no duplicate resource wallet, turn counter, or territory-ownership store.
- Changing one data asset updates the runtime and UI without changing C++ constants.

Initial aggregate exit review (2026-09-08; historical gaps, superseded by the follow-up below):

- The new `RomanEmpireGame.Content.DefinitionPropagationAndAtomicPlacementPIE` test proves a real loaded Barracks definition change reaches the normal registry, build menu, placement factory, construction duration and selected-building UI. The fixture restores the definition/dirty state without saving; the asset's disk hash matches its committed LFS payload. This is startup/spawn propagation, not live hot-reload of existing actors.
- The audit found that a single placement spend call did not ensure exactly-once execution: synchronous wallet observers could recursively confirm. Placement now guards confirmation/start/transform reentry, initializes the captured definition before spending, and publishes only after clearing consumed preview state. Tests cover all six currencies, event/actor counts, cancel/restart observers, insufficient funds and retry.
- The regression is mutation-proven: temporarily removing the confirmation guard produced two buildings and 74 gold spent instead of 37. After restoring the guard, the final suite passed 14/14 (`Artifacts/Automation/20260908_151420_304`), with strict non-unity/no-PCH compilation passing as well. Full evidence and limits are in `M2_EXIT_REVIEW.md`.
- **Architecture gap identified on 2026-09-08:** the responsibility map required `ARomanEmpireGameState`, while that revision instantiated `AGameStateBase`. The follow-up implements the required contract and consumer/test integration instead of dropping the requirement.
- Shipping and Development BuildCookRun also pass; `Artifacts/Packages/M2_Exit` contains 559 archived packages and cook reports 0 errors/warnings. The new standalone player is rejected before startup by Windows Code Integrity (3033/3077, 2026-09-08); no security policy was changed. Its hash and the pending standalone launch gate are recorded in `M2_EXIT_REVIEW.md`. The older M2.6 smoke does not certify this new binary.

Final aggregate follow-up (2026-09-09–15):

- The explicitly assigned public `ARomanEmpireGameState` delegates all reads to canonical owners, relays their events once, and releases every subscription in EndPlay. UMG and controller global-state reads/eligibility use it; commands and actor-specific events retain their proper owners. No replicated multiplayer state or duplicate mutable store was introduced.
- Native tests verify the nine pure/const queries, safe defaults, all factions/resources/objectives against owners, unique relays and complete teardown. Live Slate TRAIN assertions now prove the exact PDA cost across all six currencies, active/queued acceptance and no-charge promotion or rejection.
- A new real nested-objective transaction test exposed stale UMG snapshots. UMG now re-queries GameState. Restoring the old callback deliberately failed with 1117 displayed versus 1146 canonical gold; restoring the fix passed all 16 tests, with zero warnings/failures/pending (`Artifacts/Automation/20260909_164333_175`).
- Strict IWYU and Shipping compilation pass for this correction. After an external Fab update was rejected by Windows Code Integrity, an explicit process-only `-DisableFab` option allowed Test/Cook/Package to omit the unused Editor browser without changing project/engine/security settings. The final suite again passed 16/16 with no warnings (`Artifacts/Automation/20260915_131817_133`); Development packaging produced 559 cooked packages with zero errors/warnings in `Artifacts/Packages/M2_Final`.
- The actual final player launched, rendered the inspected `Artifacts/M2_Final-PackagedUMG.png` at 1920×1080 and exited 0, with default map/GameState/UMG/input ready and no failure signatures. Hash, commands, logs, historical failures and scope limits are in `M2_EXIT_REVIEW.md`. M2 is closed on 2026-09-15; proceed to M3.1. The full M3–M8 goal remains active.

Estimated effort: 1–2 focused weeks.

### M3 — Tactical RTS vertical slice

Goal: deliver a complete, deterministic command-and-capture encounter.

#### M3.1 Unit foundation

- Preserve all constructor-created component pointers and remove invalid post-construction null assignments.
- Define unit lifecycle: spawn, initialize definition/faction, idle, commanded, attacking, routed/dead, cleanup.
- Move health/combat/selection/order responsibilities into focused components or clearly bounded classes.
- Use skeletal placeholders and a minimal Animation Blueprint when animation work begins; do not expand the procedural limb system.

Execution preparation (source audit 2026-09-15):

- At the source audit, `AUnitBase` cleared fourteen constructor-created component references. Preserve them and test their identity/attachment and GC lifetime. Selection must control both visibility and hidden-in-game state; camera tests must verify actual component activation, not only controller mode.
- Make initialization and death explicit lifecycle boundaries. Preserve deferred initialization from GameMode/Barracks; reject partial or repeated initialization. Dead/uninitialized units must reject new actions while cleanup operations remain legal.
- Commit death and cancel orders/blocking/movement/collision before notifying observers; prevent repeated death and accidental resurrection through the health setter. Replace global-altitude corpse disposal with bounded time-based cleanup and safe EndPlay teardown.
- Keep health/combat/selection/order ownership in focused components or clearly bounded classes without duplicating fields. M3.1 is not complete merely when the null assignments are removed.
- Add actor-level tests for valid/invalid initialization, preserved components, visible selection, camera activation, reentrant death observers, rejected post-death commands and cleanup at different altitudes; keep existing UMG/input integration green. Follow with IWYU, Shipping and rendered selection/possession checks.
- Skills: `unreal-cpp-gameplay`, `ue-testing-debugging`, `ue-module-build-system`; `unreal-packaging` for the actual-player gate. Navigation, formation commands, validated combat windows, faction registries and new animation/art remain their later M3 packages. Do not expand the procedural-limb presentation while repairing its existing references.

M3.1 implementation progress (2026-09-15; not an exit approval):

- `FUnitVitalState` owns live health, stamina, morale and explicit `Uninitialized/Alive/Dead/InitializationFailed/Ended` transitions. `AUnitBase` coordinates world effects and notifications; there are no duplicate live-vitals members. Invalid/repeated initialization and revival through health commands are rejected.
- All sixteen reflected component references retain their constructor-created objects through `TObjectPtr`. Selection updates visibility and hidden-in-game together and reuses one material; possession activates the actual FPS camera.
- Death commits before observers, stops special actions/blocking/selection/possession/orders/movement, disables collision and schedules a bounded lifespan. EndPlay cancels timers/forces and seals the lifecycle. Attack targets are weak references; establishing an attack no longer immediately clears its target.
- New pure-state and real-PIE actor tests cover these foundations. Evidence and current limitations are tracked in `M3_UNIT_FOUNDATION_REVIEW.md`; an initial successful run is not the final gate for later edits.
- Remaining M3.1 work: finish separation of command/combat/selection responsibilities and their explicit active states (including the routed-state policy), and inspect rendered selection/possession with an actual-player check. Existing direct movement, per-unit scans and legacy combat timing are not certified; their replacement remains M3.2–M3.4. No new animation or procedural limbs were added.

M3.1 state integration package (verified 2026-09-20; 21/21 tests, strict IWYU and Shipping):

- Introduce a single `FUnitOrderState` for None/Move/Attack/Hold, destination and weak target. Replacing, completing and canceling orders must be explicit; pursuit updates must preserve Attack. It must not move actors, scan the world or apply damage.
- Introduce bounded combat state for blocking/action/recovery, with one temporal update and one cooldown derivation. Remove the unused parallel `UpdateAIMovement` executor instead of moving both implementations. Shared wind-up/hit-window and damage validation remain M3.4.
- Isolate local selection presentation (selected flag, ring/decal and reusable material) while the controller remains owner of the selected set. Faction eligibility and registries remain M3.2.
- Keep `AUnitBase` as the lifecycle/world/event coordinator and existing public API facade, without parallel mutable fields. Derive idle/commanded/attacking activity from state owners. A routed unit is still alive; document its cancellation/action-rejection contract before selecting morale thresholds, flight or recovery policy. Do not silently invent those gameplay rules.
- Verify order replacement/cancel/pursuit/dead targets, single cooldown advancement, idempotent selection and simultaneous cleanup on death/EndPlay with pure tests and the real PIE fixture.

Implementation details:

- `FUnitOrderState` is the sole intent owner. Hold persists without acquiring enemies; Stop clears the current order but still allows later idle acquisition. Finishing a Move clears it, while pausing pursuit retains Attack and its weak target. Invalid replacements are atomic. Dead/destroyed/friendly attack targets retire the order and destination together.
- `FUnitCombatState` owns recovery and blocking, with readiness/attack activity derived from remaining time. Both AI and direct attacks derive recovery as `1 / AttackSpeed`; the actor advances it once before action evaluation. Damage observers see recovery already committed. Movement/order cancellation does not refund recovery. The unused second AI executor has been removed; direct movement and the remaining per-unit scan await M3.2–M3.3.
- `FUnitSelectionPresentation` owns selected presentation and one reflected material; the actor retains its constructor components. `GetActivity()` derives Unavailable/Idle/Commanded/Attacking/Dead/Ended, without another mutable state. Attacking currently includes recovery, not a validated hit window.
- Camera activation/deactivation is the last possession effect, because it emits synchronous component callbacks. A nested exit/death cannot be followed by stale outer movement effects.
- Routed behavior is not implemented and zero morale does not silently trigger it. Its cancellation, vulnerability, controller notifications, trigger and recovery policy remain explicit work before M3.1 closure. The rendered selected-ring/first-person/return gate subsequently passed in the presentation package below.
- Full-suite and mutation-control evidence, exact logs and current limits: `M3_UNIT_STATE_REVIEW.md`. Its build evidence remains specific to the state-integration revision; the later package has separate evidence.

M3.1 presentation package (verified 2026-09-21; not M3.1 closure):

- Actual archived Development player selected an eligible unit, entered first person and restored the original tactical camera. The strict helper checks the same actor across selection, `PossessedUnit` and actual Pawn, exactly one possessed flag, and empty selection/possession with all flags false on return. Three 1920×1080 captures were inspected, including the selected ring and camera/HUD transitions.
- Fix mode/turn auto-wrapping below the fixed-height native HUD header, with live-PIE regression assertions. The `roman.Debug.*` helpers invoke existing commands without changing input mappings and are compile-time absent from Shipping. This is not a physical-input or complete visual-regression test.
- Verification: 21/21 native tests without warnings, strict IWYU, Shipping, 559 cooked packages (7 platform-skipped; 566 total processed) without cook errors/warnings, actual-player exit 0, and parser fixtures with 17 expected rejection cases. Exact hash, logs, captures, two player-only engine/host warnings and limits: `M3_PRESENTATION_REVIEW.md`.
- Skills: `unreal-packaging`, `ue-ui-umg-slate`, `ue-testing-debugging`, `unreal-cpp-gameplay`, `ue-module-build-system`, `ue-project-context`. No engine/plugin/driver/security changes or new art assets. M3.1 remains open for routed policy/implementation; independent M3.2a ownership work is next.

#### M3.2 Selection and ownership

- Restrict selection, box selection, commands, and possession to the player's faction.
- Define additive selection, deselection, invalid target feedback, and selection cleanup on death.
- Broadcast selection state to UMG.
- Add unit/squad registries so gameplay and HUD do not scan every actor every frame.

M3.2a ownership package (implemented and verified 2026-09-21):

- Share a controller-side eligibility query for a same-world, command-ready unit owned by the current canonical player faction. Reuse it for selection, player order dispatch, possession entry and continued control. Do not restrict actor command APIs to the player faction: campaign AI also uses them.
- Notify committed unit ownership changes and observe public player-faction changes. Re-query current state, prune selection and exit invalid possession, including block/camera/input/UMG cleanup. Cleanup must remain legal outside Tactical and after permissions are lost.
- Revalidate possession after synchronous camera callbacks; selection UMG observers must bind to the controller's current selection, not an event payload overtaken by a nested change.
- Keep replace-selection semantics for this repair. Additive input, invalid-target feedback and building-selection ownership remain explicit M3.2 scope, not implied by unit tests. Cover mixed-faction/dead/deferred/foreign-world selections, ownership loss during callbacks, repeated cleanup, and preserved enemy AI commands.
- Follow with M3.2b live unit/squad registries. Keep actor ownership canonical, with only derived faction indexes and weak live-actor references. The content-definition registry is not a live-unit registry. Squad identity/membership needs its own contract; the current spawn formation does not provide one.

Implementation evidence:

- Shared eligibility now protects selected-unit move/attack dispatch and possession without restricting enemy actor command APIs. Committed unit/player-faction events synchronously prune selection, restore invalid possession and clean up blocking/input/UMG. Controller EndPlay unbinds observers and cannot reinstall contexts.
- Possession revalidates after actual camera callbacks; recursive teardown is guarded. UMG reads current canonical selection after nested notifications, with an immutable event snapshot.
- `PlayerOwnershipPIE` brings the suite to 22/22 with zero result warnings/failures/pending. Both stale-UMG-payload and missing-faction-boundary mutations fail before restoration. Strict IWYU and Shipping pass. The fresh Development package cooked 559 packages plus 7 platform skips without cook errors/warnings; exact binary/hash/logs and limits are in `M3_OWNERSHIP_REVIEW.md`.
- `-LoseOwnership` adds a transient packaged-player scenario that returns automatically after faction transfer, with no explicit exit/clear commands. Both ordinary and ownership-loss actual-player runs passed strict identity/state checks and exited 0; six 1920×1080 captures were inspected. Parser regression separately has 4 synthetic positives and 20 expected rejections. Two known player-only engine/host warnings are recorded, not suppressed.
- Skills: `unreal-cpp-gameplay`, `ue-ui-umg-slate`, `ue-input-system`, `ue-testing-debugging`, `ue-module-build-system`, `unreal-packaging`, `ue-project-context`. No new assets, dependencies, plugins or security changes.

M3.2b live-unit registry package (implemented and verified 2026-09-26; squad registry remains open):

- `URomanUnitRegistrySubsystem` indexes only live initialized Game/PIE actors through weak references and derived faction buckets. Actor ownership/lifecycle stay canonical; death/EndPlay removal and ownership reindexing precede external callbacks. Deferred, failed and ended actors cannot register early or reenter the index.
- Idle enemy acquisition and rectangle selection now use the index instead of whole-world actor enumeration. The real controller rectangle facade normalizes bounds, validates input and preserves player eligibility. Enemy radius/None/Hold semantics remain unchanged; equal distances have a lexical-name tie-break.
- `LiveRegistryPIE` covers registration, death, EndPlay, true GC, reindexing, nested owner/death/destruction events, world isolation/teardown and real camera projection/rectangle selection. A temporary viewport size fixes zero-size NullRHI projection without simulating physical input. Only the Editor test module gains `RenderCore`.
- Restored targeted test: 1/1; final regression: 23/23, both with zero result warnings/failures/pending. Strict IWYU (60 actions), Shipping and repository/LFS validation pass. Both temporary negative-control mutations fail the intended observer/removal assertions. Full evidence, historical host warnings and final package results: [M3_UNIT_REGISTRY_REVIEW.md](M3_UNIT_REGISTRY_REVIEW.md).
- Fresh Development package passes UAT/cook (559 cooked + 7 platform skips). Both actual-player possession/return scenarios pass with exit 0, including automatic ownership-loss cleanup; all six captures inspected. Two known player rendering warnings remain. No physical-drag, Shipping-player or performance claim.
- Skills: `unreal-cpp-gameplay`, `ue-testing-debugging`, `ue-module-build-system`, `unreal-packaging`, `ue-project-context`. No assets, plugins, Windows settings or runtime dependencies changed.

Building-selection ownership boundary (implemented and verified 2026-09-27):

- Share canonical player-faction, same-world and live/placed readiness across building selection, player TRAIN and UMG. Own constructing/damaged buildings remain selectable; previews, deferred/failed/ended/destroyed actors and enemies do not. Direct enemy training is preserved.
- Observe committed building-state and player-faction notifications. Ownership loss, placement reentry and EndPlay retire selection immediately, including outside Tactical and during nested selection callbacks. Old building observers and controller teardown are cleaned up.
- The new `Buildings.PlayerSelectionPIE` uses real Slate TRAIN and verifies no spending or production changes from queued UI callbacks after invalidation. A missing-faction-check mutation fails before restoration. Targeted 1/1 and full regression 24/24 pass without result warnings/failures/pending. Evidence and limits: [M3_BUILDING_SELECTION_REVIEW.md](M3_BUILDING_SELECTION_REVIEW.md).
- Strict IWYU (60 actions), Shipping (6 actions), repository validation and Git LFS pass. This native boundary change does not reuse a historical player package as evidence for the new executable.
- Skills: `unreal-cpp-gameplay`, `ue-ui-umg-slate`, `ue-testing-debugging`, `ue-module-build-system`. No new assets, input mappings, dependencies or services; no new packaged/rendered gate claimed for this non-layout boundary change.

Additive selection and invalid-target feedback (implemented 2026-09-27; packaged-player gate pending):

- Shift-click toggles eligible units, Shift-drag adds; plain gestures replace. The modifier is captured at gesture start. Empty additive gestures preserve selection; plain empty gestures clear it. Buildings remain exclusive and invalid actor clicks preserve existing selection with transient UMG feedback.
- Replace/Add/Toggle reuse the controller's canonical commit path. Feedback is event-driven, expires after three seconds and unbinds/clears at widget teardown. The saved input config has 20 actions and Tactical has 18 mappings, including both Shift keys; existing mappings are preserved.
- Targeted 6/6 and full 25/25 pass without result warnings/failures/pending, including a repaired synchronous focus-flush regression. Strict IWYU (60 actions), Shipping (6), repository validation and Git LFS pass. A fresh Development package passes UAT, but Windows Code Integrity blocks its unsigned player before startup (events 3077/3033). No security policy change or packaged runtime success is claimed. Evidence: [M3_ADDITIVE_SELECTION_REVIEW.md](M3_ADDITIVE_SELECTION_REVIEW.md).
- Skills: `ue-input-system`, `unreal-cpp-gameplay`, `ue-ui-umg-slate`, `ue-testing-debugging`, `ue-module-build-system`, `unreal-packaging`, `unreal-mcp`, `ue-data-assets-tables`, `ue-project-context`. No new runtime dependencies or services.
- Separate editor-game preview passes selection-state and reflected-feedback assertions; both 1920×1080 captures were inspected (15 selected units, rings, Shift help and warning). This is explicitly not packaged-player evidence. Preview-only Python/toolset startup incompatibility is avoided with `-DisablePython`; editor widget-factory/render warnings remain documented.

Remaining M3.2 gate: fresh packaged/rendered verification on a runner that trusts the binary. Tactical squad membership follows the explicit contract below; persistent campaign identities and their battle mapping remain M5 work. The live registry removes enumeration but not per-unit all-enemy search complexity; spatial indexing and cadence must follow measurement. M3.1 routed policy remains open. Production capture/queue/refund policies remain M3.5, not implied by selection eligibility.

M3.2 squad foundation contract (implementation started 2026-09-27):

- Extend the existing world-scoped unit registry as the sole owner of tactical squad membership. Each live registered unit belongs to at most one squad; all members have the same non-None actor-owned faction. Creating a group validates the complete input atomically, deduplicates it and rejects already-grouped or invalid members without moving them implicitly.
- Handles are generated `FGuid`s local to one world/battle. They are not persistent campaign army/unit identifiers and must not be serialized as a save contract. M5 will define the explicit mapping from campaign records to tactical groups and battle results.
- Members are weak actor references, ordered by explicit insertion. Death, EndPlay and ownership changes remove membership before actor observers; a squad disappears when its last member leaves. Disband changes only membership, never ownership/orders/selection. Possession alone does not change membership.
- Bootstrap Romans and Carthaginians form one group each. Newly trained units start as singleton tactical groups; no automatic merge or recruitment-to-army policy is implied. Selection remains per-unit. Formation/path execution remains M3.3.
- Verify atomic rejection, deduplication, single membership, ordering, actual bootstrap/training integration, owner/death/EndPlay and nested notifications, world isolation/teardown and non-retention. No additional assets, subsystem, dependency, world scan or per-frame update is required.

Squad implementation evidence (2026-09-28):

- The existing registry now exposes atomic creation/disband and validated weak-member/reverse/faction queries. Actor removal/reindex boundaries retire membership before observers. GameMode creates the initial faction groups and Barracks creates singleton recruited groups.
- `SquadRegistryPIE` passes the strengthened targeted run (1/1); full regression passes 26/26 without result warnings/failures/pending. It covers actual bootstrap/training and nested owner/death notifications, disband/order preservation, world isolation/teardown and real GC. Revision-specific build results and limits: [M3_SQUAD_REGISTRY_REVIEW.md](M3_SQUAD_REGISTRY_REVIEW.md). No packaged-player or performance gate is implied.
- Strict IWYU (60 actions), Shipping (6), repository/LFS validation and diff checks pass. Skills: `unreal-cpp-gameplay`, `ue-testing-debugging`, `ue-project-context`; existing build/module conventions remain unchanged.

#### M3.3 Commands, navigation, and formations

Navigation foundation implemented (2026-09-28); M3.3 remains in progress. `ARomanUnitAIController` executes canonical orders with CharacterMovement and dynamic NavMesh. Both maps persist 42/192 cm agents; paths must be complete, unreachable orders settle in Hold, request/revision guards protect replacement commands, and possession reuses the cached controller. The real-PIE test verifies an obstacle detour, moving-target pursuit, cancellation/reentry, possession and lifecycle cleanup. Navigation evidence: [M3_NAVIGATION_REVIEW.md](M3_NAVIGATION_REVIEW.md). Skills: `unreal-cpp-gameplay`, `ue-testing-debugging`, `ue-module-build-system`, `unreal-mcp`.

Formation destinations implemented (2026-09-28): a shared stateless layout assigns distinct travel-facing slots to the eligible selection, without expanding squad membership. Tight NavMesh projection validates the complete footprint before dispatch; invalid footprints preserve prior orders and report feedback. A batch revision prevents synchronous callbacks from overwriting newer commands. Pure layout and real three-unit arrival/reentrant-dispatch tests pass; full gate status: [M3_FORMATION_REVIEW.md](M3_FORMATION_REVIEW.md). Skills: `unreal-cpp-gameplay`, `ue-testing-debugging`, `ue-module-build-system`.

Player-facing STOP/HOLD controls implemented (2026-09-29): selected-unit controller facades apply the same eligibility and batch supersession rules as Move/Attack. Native UMG buttons are restricted to tactical unit selection. A real PIE test drives Slate pointer events through both buttons, checks live route cancellation and persistent Hold, and verifies mode/ownership/stale-callback guards. Targeted evidence and full gate status: [M3_SELECTED_COMMANDS_REVIEW.md](M3_SELECTED_COMMANDS_REVIEW.md). Skills: `unreal-cpp-gameplay`, `ue-ui-umg-slate`, `ue-testing-debugging`.

Disconnected-island and narrow-passage coverage implemented (2026-09-29): a separate real-PIE test places transient walls across the saved NavMesh bounds. A reachable-on-mesh but disconnected destination must settle in Hold/Unreachable without crossing the gate; removing the gate must yield a full path and actual CharacterMovement arrival through the 400 cm opening. Targeted 1/1 and full 31/31 regression pass, with only the known host WASAPI sample-rate notices; strict IWYU and Shipping compilation pass. Evidence: [M3_DISCONNECTED_NAVIGATION_REVIEW.md](M3_DISCONNECTED_NAVIGATION_REVIEW.md). Skill: `ue-testing-debugging`.

Opposing-group congestion coverage implemented (2026-09-29): six real Legionaries receive two three-unit formation moves simultaneously through the player controller and cross the same corridor. Each reaches its distinct slot with CharacterMovement and no route failure or capsule overlap. The closest opposing pair reaches 84 cm, so the test exercises actual contact-distance congestion rather than separate lanes. Targeted 1/1 and full 32/32 regression pass. Gate details: [M3_GROUP_CONGESTION_REVIEW.md](M3_GROUP_CONGESTION_REVIEW.md). Skill: `ue-testing-debugging`.

- Replace direct `SetActorLocation` movement with AIController/Character Movement/NavMesh path following.
- Implement move, attack, attack-move if selected for the slice, stop, and hold with explicit command state.
- Ensure an attack command does not clear its target while establishing movement.
- Use squad/formation destinations to avoid every unit independently solving the same high-level order.
- Define unreachable-path, dead-target, and command-cancellation behavior.

#### M3.4 Shared combat

- Establish one damage entry point and one cooldown source.
- Implement melee range, wind-up, hit window, cooldown, death, and faction validation.
- Remove double application between controller traces and unit attacks.
- Connect deaths to objective, UI, kill, and campaign-resolution events.
- Defer pilum/testudo unless one is explicitly included in the slice; remove misleading UI controls for unavailable actions.

#### M3.5 Buildings and production

- Use one building-placement command with a selected building definition.
- Validate terrain, bounds, overlap, ownership, and affordability before commitment.
- Use deferred spawning or explicit initialization so type data exists before health/visual/construction initialization.
- Charge resources once after all validations pass, with rollback if spawn/initialization fails.
- Replace decorative camp structures needed for gameplay with actual building actors.
- Use one barracks production queue and one data-defined legionary cost/duration.
- Ensure UI buttons consume UI input and do not depend on a world trace hitting the building.

#### M3.6 Encounter and victory

- Build a deterministic objective scenario with fixed seed and known forces.
- Establish one victory evaluator for tactical objectives.
- Report a typed battle result containing winner, surviving squads, casualties, and captured objective.
- Display restart/continue actions through UMG.

#### M3.7 Performance baseline

Preliminary 50/100-unit load capture (2026-09-29): two repeatable PIE fixtures spawn 50 or 100 additional Legionaries, issue live formation paths, warm up 30 frames and capture 240 frames with CSV profiling and an Unreal Insights trace. A D3D12 offscreen run frames all added units and records game/render/GPU timings, categorized draw-call counters and process memory on the local RTX 5070 Ti Laptop GPU; the full regression passes 34/34. The committed summary and exact captures are in [M3_PERFORMANCE_BASELINE_REVIEW.md](M3_PERFORMANCE_BASELINE_REVIEW.md). This is an Editor-only provisional baseline, not a packaged-player measurement or M3.7 exit: the scene lacks finished combat/building/objective load, the frame clock is capped near 16.67 ms, memory includes Editor, and a target FPS has not yet been agreed. Repeat on the finished encounter in a trusted packaged runner before optimization decisions. Skill: `ue-testing-debugging`.

- Remove per-frame world-wide actor scans.
- Establish stat captures and Unreal Insights traces at 50 and 100 units.
- Record game-thread, render-thread, GPU, draw-call, and memory baselines on reference hardware.
- Optimize measured bottlenecks only; do not migrate to Mass pre-emptively.

Evidence and exit gate:

- Functional test selects owned units, issues a pathing command around an obstacle, attacks an enemy, produces a unit, and captures the objective.
- Enemy units cannot be selected, commanded, trained from, or possessed.
- Placement rejects blocked/invalid locations and does not charge on failure.
- Successful building/training operations charge exactly once.
- Combat applies one validated hit per attack window.
- The encounter completes in PIE and a packaged build at the agreed frame target.

Estimated effort: 2–4 focused weeks for greybox quality.

### M4 — First-person possession

Goal: make direct control a reliable tactical mode using the same unit state as RTS play.

Work packages:

- M4.1: Implement a possession state machine with explicit enter, active, exit, unit-death, battle-end, and failure transitions.
- M4.2: Validate that the unit is owned, alive, possessable, and not reserved by another state.
- M4.3: Preserve the tactical camera transform/selection and restore them on exit.
- M4.4: Suspend the unit AI controller cleanly and restore/spawn the correct AI controller on return.
- M4.5: Activate the possession input context and FPS HUD only while possessed.
- M4.6: Reuse shared combat, health, cooldown, team, and death logic. Player input requests attacks; it does not apply separate damage.
- M4.7: Add camera collision, movement limits, attack montage/hit window, block/stamina feedback, and death fallback.
- M4.8: Decide whether possession changes campaign/tactical time scale; implement one explicit rule.

Evidence and exit gate:

- Automated test enters and exits the same unit repeatedly without losing AI, input, selection, or camera state.
- Possession of an enemy/dead unit is rejected.
- Death while possessed returns the player to tactical control and records one death.
- Damage and cooldown results match equivalent AI-controlled attacks.
- Tactical battle resolution remains correct after possession was used.

Estimated effort: 1–2 focused weeks.

### M5 — Campaign loop and persistence

Goal: connect the tactical encounter to a small but complete campaign.

#### M5.1 Campaign content

- Define approximately nine stable territory assets, their adjacency, initial ownership, production, settlements, and tactical-map reference.
- Initialize only Rome and Carthage as active factions in the slice.
- Render the campaign as data-backed UMG/map presentation rather than making visual actors authoritative.

#### M5.2 Campaign actions and AI

- Implement select territory, inspect, recruit/assign army, move, initiate battle, and end turn.
- Implement a deterministic Carthaginian AI with constrained goals: defend, recruit, and attack adjacent territory.
- Use seeded randomness recorded in campaign state if variation is needed.
- Apply tactical battle results atomically to casualties and ownership.

#### M5.3 Economy and victory

- Process territory production once per turn.
- Add only the minimum upkeep/recruitment rules required by the product brief.
- Implement explicit conquest victory and loss conditions for the slice.
- Ensure maximum-turn behavior produces an intentional result rather than silently stopping.

#### M5.4 Local save/load

- Define a versioned save schema containing stable IDs, turn, factions, resources, ownership, armies, settings, and RNG state where required.
- Use a dedicated save subsystem and `USaveGame` implementation.
- Add manual save, autosave at a safe turn boundary, load validation, corruption handling, and migration/version rejection behavior.
- Never serialize raw actor pointers or transient map actors.

Evidence and exit gate:

- A test starts the campaign, advances turns, launches the tactical encounter, applies its result, and returns to updated campaign state.
- Production and AI run once in the documented order.
- Ownership counts and victory conditions agree with the territory model.
- Save, process another turn, load, and compare produces the original canonical state.
- A packaged build can complete the full first-playable journey.

Estimated effort: 2–4 focused weeks.

### M6 — Production UI, content, audio, and accessibility

Goal: replace prototype/debug presentation with a coherent slice without changing authoritative gameplay state.

Work packages:

- M6.1: Implement one responsive UMG shell for strategic, tactical, possession, selection, production, objectives, and end-state panels.
- M6.2: Implement a real minimap fed by registered units/objectives and territory state, with marker lifecycle handling.
- M6.3: Replace blank generated Widget Blueprints; either remove design JSON or make it a validated source consumed by an idempotent generator.
- M6.4: Create actual material instances and visual states for faction, selection, valid/invalid placement, damage, and construction.
- M6.5: Replace null Sound Cue libraries with referenced assets; use audio components/classes or MetaSounds appropriate to the selected content.
- M6.6: Decide the fate of procedural mesh generators. Fix and test only generators retained for production; remove unused/dead systems from the runtime module.
- M6.7: Add skeletal characters, Animation Blueprints, montages, and minimum readable faction silhouettes.
- M6.8: Add scalable text, color-safe selection indicators, volume controls, sensitivity controls, remapping support, subtitles if voiced content is added, and pause/settings flows.
- M6.9: Make editor generation scripts idempotent, versioned, and validated, or remove them in favor of committed authored assets.

Evidence and exit gate:

- No required gameplay interaction depends on Canvas coordinates or debug messages.
- Every production widget is instantiated, navigable, and bound to canonical state.
- Missing asset validation reports zero required references.
- Audio/material managers are either used and configured or removed.
- UI remains usable at agreed target resolutions and scaling settings.

Estimated effort: 2–6 weeks depending primarily on art/audio availability; engineering can overlap M3–M5 after M2 interfaces stabilize.

### M7 — Automation, CI, security, and release gate

Goal: make regressions visible and produce repeatable release candidates.

#### M7.1 Test layers

- Add native automation tests for resource math, transactions, diplomacy pairs, turn sequencing, combat calculations, definitions, and save migration.
- Add deterministic Functional Test maps for tactical commands, building/training, possession, victory, and campaign handoff.
- Use Automation Driver from the test module for real UMG input, focus, and visible-state assertions only if native/Functional APIs cannot cover those behaviors.
- Use screenshot tests only for rendering regressions and commit reviewed baselines; never treat taking a screenshot as success.
- Retain Python/Remote Control only if a genuine packaged black-box requirement remains.

#### M7.2 Test launcher repair or retirement

- Completed during M1 recovery: the broken Remote Control `run_e2e.py` harness and its helper/tests were retired in favor of native automation, future Functional Tests, and UAT/package smoke coverage. See `scripts/README.md`.

#### M7.3 CI pipeline

- Use a controlled Windows runner with the selected Unreal version and Visual Studio toolchain installed.
- Pipeline stages: clean checkout, Git LFS fetch, project generation, Editor build, native tests, Functional Tests, content validation, cook/package, packaged smoke test, artifact publication.
- Record engine version, commit, build configuration, test reports, logs, and performance captures with each artifact.
- Protect `main`; require review and green gates before merge.

#### M7.4 Developer tooling safety

- Archive or remove `Tools/dev_orchestrator.ps1` from the supported workflow.
- Prohibit unreviewed full-file AI replacements, `git add -A`, automatic commits, and automatic pushes.
- Validate every tool-produced path by resolving it under an allowed root before writing.
- Keep API credentials out of URLs/logs and use standard secret storage.
- Treat Unreal/DCC MCP access as privileged code execution: loopback only, explicit session startup, clean recovery point, sequential calls, narrow toolsets, and post-operation verification.
- Review and pin agent-skill sources before team-wide use; global skill installation is not a repository dependency or a CI prerequisite.

#### M7.5 Release and repository governance

- Add version/build metadata, changelog discipline, crash/log collection instructions, and a release checklist.
- Audit third-party licenses and attribution for all shipped assets/plugins.
- Run secret scanning and dependency/plugin review.
- Produce both Development QA and Shipping packages; verify test/Remote Control code is absent or disabled in Shipping.

Evidence and exit gate:

- A deliberately broken invariant causes the relevant test and CI job to fail.
- CI creates a launchable Windows artifact from a clean checkout.
- Test results contain assertions tied to requirements, not log-pattern proxies.
- No test web server starts in a normal or Shipping launch.
- Protected-branch policy prevents automated direct pushes.
- Release checklist and attribution review are complete.

Estimated effort: 1–3 focused weeks for the initial pipeline, then continuous maintenance.

### M8 — Optional scale and online expansion

M8 starts only after M7 proves the complete offline slice.

#### M8.1 Scaling decision gates

- Adopt Mass Gameplay when Unreal Insights demonstrates Actor/squad representation cannot meet the agreed visible/background army targets.
- Adopt World Partition/HLOD when authored world size and memory/streaming measurements exceed the simple-level approach.
- Use PCG for deterministic world dressing, not canonical territory/resource state.
- Evaluate Gameplay Ability System only when ability/effect/cooldown complexity or multiplayer requirements make the simpler combat model expensive to maintain.

#### M8.2 Supabase

- Keep local saves authoritative and operational offline.
- Add user authentication and per-user ownership.
- Use publishable client credentials with Row Level Security.
- Put privileged validation, leaderboard submission, and conflict-sensitive operations in an Edge Function or trusted service.
- Use safe JSON serialization, URL/query construction, timeouts, retries, structured errors, schema versioning, idempotency, and controlled upserts.
- Test offline, reconnect, duplicate submission, stale-version, authorization, and malicious-score cases.

#### M8.3 Content expansion

- Add factions, units, diplomacy, regions, battles, and abilities through data definitions and tested factories.
- Require a performance/content budget for each expansion.
- Do not increase faction breadth until Rome-versus-Carthage campaign behavior is stable and enjoyable.

## 8. Verification strategy

Each requirement needs evidence at the same scope as the claim.

| Requirement type | Required evidence |
|---|---|
| Pure calculation/invariant | Native automation test with explicit assertions |
| Actor interaction | Deterministic Functional Test map |
| UI behavior | Automation Driver action plus state/visibility/focus assertion |
| Asset/reference validity | Content validation commandlet/test |
| Full player loop | Packaged-build scenario with canonical end-state assertion |
| Reproducibility | Clean checkout on a separate workspace/runner |
| Performance | Unreal Insights/stat capture on reference hardware |
| Shipping security | Shipping configuration inspection and runtime port/process check |
| Save compatibility | Round-trip, migration, corruption, and version tests |

Test naming should encode the requirement, for example:

- `Resources.CostDelta_DefaultsToZero`
- `Resources.PlaceBuilding_DeductsExactlyOnce`
- `Diplomacy.Relation_IsScopedAndSymmetric`
- `TurnPipeline.ProductionAndAI_RunOnceInOrder`
- `Orders.Attack_RetainsTargetDuringApproach`
- `Possession.EnemyUnit_IsRejected`
- `SaveGame.RoundTrip_PreservesCampaignState`
- `VerticalSlice.CaptureAndSave_CompletesInPackagedBuild`

## 9. Audit-to-work traceability

| Audit finding | Planned resolution |
|---|---|
| `.uasset`/`.umap` ignored; `NewMap` missing | M0.1–M0.2, M1.1, M1.5 |
| Hard-coded engine/project paths and stale solution | M1.2 |
| Duplicate declarations, illegal protected access, missing implementation, bad include/log category | M1.3 |
| Runtime Automation Driver and always-on Remote Control | M1.4, M7.1, M7.5 |
| No safe live-editor navigation/asset workflow | Section 6 Unreal MCP gate, M1.4, M7.4 |
| Empty/generated Blueprint/material/audio shells | M1.5, M6.3–M6.5, M6.9 |
| Unit component pointers nulled | M3.1 |
| Attack command clears target; cooldown/double-damage problems | M3.3–M3.4 |
| Direct movement and no reliable AI/pathfinding | M3.3 |
| Enemy selection/possession | M3.2, M4.2 |
| World-wide per-unit scans | M3.2, M3.7 |
| Duplicate gold/resource state and double deduction | M2.1, M3.5 |
| Building type/cost leakage, always-valid placement, duplicate training systems | M2.4, M3.5 |
| Decorative rather than functional buildings | M3.5 |
| Territory IDs unset and ownership stores disconnected | M2.3, M5.1 |
| Nonzero default resource deltas/production/costs | M2.1 |
| Diplomacy ignores first faction | M2.3 |
| Separate turn counters and victory evaluators | M2.2, M3.6, M5.3 |
| Contradictory zoom conventions | D-04, M2.2, M2.6 |
| Canvas and UMG disconnected; minimap/kill state incomplete | M2.5, M6.1–M6.2 |
| Save/load TODO | M5.4 |
| Procedural mesh/audio/material systems unused or broken | M6.4–M6.6 |
| Weak E2E assertions, incorrect paths/methods, process leak, stale reports | M7.1–M7.2 |
| Unsafe autonomous file replacement/commit/push tool | M7.4 |
| No CI, license, LFS attributes, formatting policy | M0.6, M1.1, M7.3, M7.5 |
| Supabase auth/RLS/upsert/serialization/security gaps | M8.2 |
| Full-world and army-scale risk | D-03, D-05, M3.7, M8.1 |

## 10. Dependency and execution order

Critical path:

```text
M0 decisions/assets
        -> M1 reproducible compile/package
        -> M2 canonical state/data contracts
        -> M3 tactical RTS loop
        -> M4 possession
        -> M5 campaign/save handoff
        -> M7 release gate
        -> M8 optional scale/online
```

M6 presentation work may begin after M2 interfaces stabilize and can proceed alongside M3–M5, but it must not invent state or APIs ahead of the owning gameplay work.

Testing is written inside every milestone. M7 builds the full CI/release system; it is not the first time tests are added.

For a solo developer, execute one work package at a time. For a team, safe parallel lanes after M2 are:

- Tactical commands/combat.
- UMG presentation against frozen events/interfaces.
- Authored map/art/audio content against frozen data definitions.
- Automation fixtures against written acceptance criteria.

## 11. Change and review protocol

Every implementation work package should use this ticket shape:

1. Goal and linked plan ID.
2. Observable user outcome.
3. Explicit non-goals.
4. Authoritative state owner affected.
5. Files/assets expected to change.
6. Migration or backward-compatibility impact.
7. Tests to add before calling it complete.
8. Manual packaged-build check, if applicable.
9. Rollback/recovery note.

Integration rules:

- Do not combine recovery, architecture migration, and new gameplay in one opaque change.
- Keep each commit buildable where feasible.
- Do not stage unrelated workspace changes.
- Run the milestone's narrow tests first, followed by the repository gate.
- Update this plan only when scope, dependency, or acceptance evidence changes.
- Record removed/deferred systems so dead code does not remain as a misleading alternative path.

## 12. Risk register

| Risk | Impact | Mitigation / trigger |
|---|---|---|
| Important assets exist only on an old machine | Critical data loss or wrong engine upgrade | Complete M0 inventory before engine migration. |
| Scope expands toward three complete games | Slice never becomes playable | Enforce first-loop scope and milestone exit gates. |
| Literal seamless Mediterranean scale | Extreme streaming, content, AI, and precision cost | Use campaign data plus tactical instances; revisit only after a slice. |
| Engine 5.8 migration reveals API/plugin changes | Recovery delay | Upgrade before authoring new assets; keep migration isolated in M1. |
| State consolidation breaks prototype behavior | Short-term regression | Characterize intended behavior with tests, then remove duplicates deliberately. |
| Unit count drives premature Mass rewrite | Lost iteration speed | Measure Actor approach at 50/100 units before M8. |
| UI is built against unstable gameplay | Rework and new duplicate state | Freeze M2 events/contracts before production UI. |
| Existing automated reports create false confidence | Broken releases | Replace log/screenshot success with assertions and clean-run evidence. |
| Online backend compromises player data/leaderboards | Security and trust failure | Keep offline first; require Auth, RLS, and trusted validation in M8. |
| Binary repository growth | Slow clones and hosting limits | LFS patterns, asset budgets, source/derived policy, periodic audits. |
| MCP or DCC automation damages assets or exposes an editor control port | Data loss or local code execution | Apply the Section 6 gate, use loopback/manual sessions, checkpoint first, serialize calls, and exclude tooling from Shipping. |
| Unpinned third-party skills drift or provide stale UE advice | Incorrect implementation or supply-chain exposure | Record reviewed revisions, prefer Epic/OpenAI sources, verify against live schemas/current docs, and re-audit before updates. |

## 13. Program completion criteria

The audited implementation program is complete only when all of the following are proven:

- A clean clone retrieves every required source asset.
- The selected Unreal version builds Editor, Development Game, and Shipping Game targets.
- Native and Functional Test suites pass from a clean runner.
- A packaged Windows build completes the entire first-playable journey.
- Resources, turns, territories, training, combat, victory, and saves each have one authoritative owner.
- Strategic, tactical, and possession transitions preserve consistent state.
- Save/reload restores the canonical campaign after a tactical result.
- Performance meets the agreed target on reference hardware.
- Remote Control/test modules are absent or disabled in Shipping.
- Unreal MCP is either disabled or excluded from Shipping, no tooling port is listening, and its live-editor workflow passes a read-only health/toolset check when intentionally started.
- License, attribution, build instructions, release artifacts, and validation reports are current.
- Optional M8 systems are not required for the offline game to function.

## 14. Recommended first implementation batch

When implementation is authorized, execute only this batch first:

1. Complete M0 asset inventory and engine decision.
2. Create the Git LFS/source-asset policy.
3. Make build tooling portable.
4. Fix only the confirmed compile blockers and plugin/module boundaries.
5. Commit the deterministic test map and required placeholder assets.
6. Prove clean Editor build, PIE launch, cook, package, and packaged smoke launch.

The M1 human-review gate was accepted and its commit/clean-clone reproduction completed on 2026-09-05. Canonical-state refactoring may now proceed as the first M2 work package.
