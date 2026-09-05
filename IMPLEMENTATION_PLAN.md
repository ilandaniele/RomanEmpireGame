# Roman Empire Game Implementation Plan

Status: implementation active; M0 and M1 complete, M2 canonical-state work active
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
| Public current phase, turn, resources, and match state | `ARomanEmpireGameState` | UI, controller, tests |
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

The following skills were installed globally for Codex on 2026-09-03. They are not vendored into the game repository; their reviewed source revisions are recorded so another workstation can reproduce or re-audit the selection.

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
- M2.1 is complete; M2.2 is the active implementation package.

#### M2.2 Turn and phase model

- Retain one canonical turn counter.
- Define an ordered turn pipeline: validate request, production, upkeep, AI actions, conflict resolution, victory evaluation, increment, event broadcast, autosave.
- Define `Strategic`, `Tactical`, and `Possessed` modes independently of raw zoom thresholds.
- Store zoom thresholds/configuration in one location and update documentation/tests from it.

#### M2.3 Territory and diplomacy model

- Give every territory a stable ID and display definition.
- Store ownership once in campaign state; territory actors/widgets present that state.
- Replace the diplomacy list with a normalized faction-pair key or symmetric matrix.
- Define legal transitions and invariants for war, neutrality, alliance, and invalid/self relationships.
- Add setters/factories that fully initialize territories before they become observable.

#### M2.4 Data-driven definitions

- Create Primary Data Assets for faction, unit, building, and territory definitions.
- Remove repeated costs, durations, colors, display strings, and combat values from controller/HUD code.
- Use stable IDs in save files and runtime factories.
- Add content validation for duplicate IDs, missing assets, invalid costs, and invalid faction references.

#### M2.5 UI and event consolidation

- Make UMG the production UI path.
- Keep Canvas HUD only as a temporary debug overlay, then remove its gameplay panels.
- Define events for selection, resources, phase, turn, production, possession, objectives, and victory.
- Eliminate tick-based UI resource correction and disconnected UI-owned counters.

#### M2.6 Input contexts

- Create committed Enhanced Input assets for strategic, tactical, possession, and shared/system input.
- Activate mapping contexts on mode transitions.
- Prevent build/train/command actions when their context or UI mode is inactive.
- Document keyboard/mouse defaults and ensure remapping can be added without changing gameplay code.

Evidence and exit gate:

- Unit tests prove zero-default deltas, affordability, atomic spending, production, ownership changes, diplomacy symmetry, and turn order.
- A diagnostic screen shows the same canonical values as automation queries.
- Searches find no duplicate resource wallet, turn counter, or territory-ownership store.
- Changing one data asset updates the runtime and UI without changing C++ constants.

Estimated effort: 1–2 focused weeks.

### M3 — Tactical RTS vertical slice

Goal: deliver a complete, deterministic command-and-capture encounter.

#### M3.1 Unit foundation

- Preserve all constructor-created component pointers and remove invalid post-construction null assignments.
- Define unit lifecycle: spawn, initialize definition/faction, idle, commanded, attacking, routed/dead, cleanup.
- Move health/combat/selection/order responsibilities into focused components or clearly bounded classes.
- Use skeletal placeholders and a minimal Animation Blueprint when animation work begins; do not expand the procedural limb system.

#### M3.2 Selection and ownership

- Restrict selection, box selection, commands, and possession to the player's faction.
- Define additive selection, deselection, invalid target feedback, and selection cleanup on death.
- Broadcast selection state to UMG.
- Add unit/squad registries so gameplay and HUD do not scan every actor every frame.

#### M3.3 Commands, navigation, and formations

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
