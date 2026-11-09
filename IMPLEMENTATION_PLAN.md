# Roman Empire Game Implementation Plan

Status: implementation active; M0–M2 and M3.2a complete; remaining tactical/campaign/persistence/presentation/release gates open
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

Additional gameplay policies approved by the user on 2026-10-05: morale reaching zero triggers routing/retreat (M3.1); cancellation before a trained unit is spawned refunds the full recorded training cost (M3.5); the objective is captured by occupation with no living enemies (M3.6); possession runs in real time, without slow motion (M4.8). These are approved contracts, not implementation/verification claims. Keep them in the GPT/Claude handoff.

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

### GPT / Claude handoff workflow (2026-10-02)

Continuity maintenance H-002 (2026-10-12): operational state is ONLY the compact
HANDOFF.md; HANDOFF_HISTORY.md indexes preserved checkpoint archives. Keep model
protocol/test recipes in MODEL_WORKFLOW.md, not duplicated CURRENT snapshots.
Replace the operational checkpoint when closing a block; consult archived
evidence by exact section, never restart old NEXT/PID. No milestone/gate changes.

The user assigns implementation, C++/UMG, tests, builds, asset integration and documentation to **Claude**, and image/texture/visual-reference/model generation to **GPT** with the available generation/DCC tools. Production 3D meshes require a compatible tool and the existing asset acceptance gates. This is a workflow assignment; milestone scope and acceptance stay unchanged.

`AGENTS.md` holds shared instructions, `CLAUDE.md` is the Claude entrypoint, and [HANDOFF.md](HANDOFF.md) is the current saved execution state. [MODEL_WORKFLOW.md](MODEL_WORKFLOW.md) provides copy/paste prompts, asset briefs, targeted-test commands and the closing marker. Read the compact handoff and only the next task's plan section; do not repeat the audit when switching models. One model writes at a time. Each completed or interrupted block records files, evidence, decisions, pending processes and the exact next task before the final `HANDOFF_READY` marker. No automatic commits or remote publishing are part of a switch.

Global skills and live MCP connections are client-specific; switching models does not transfer or prove their availability. Use the recorded skill sources below where needed. Operational checkpoints now belong in `HANDOFF.md`, outside ignored `Artifacts/`; historical milestone reviews retain detailed evidence.

Current execution (2026-10-07): C001–C017H preserved, uncommitted. C017D materials/C017E audio/C017F procedural retirement closed; C017G [skeletal characters](M6_CHARACTER_ASSET_REVIEW.md) CLOSED minimum offline art/animation gate: three original meshes/15assets/3GLBs, real native AnimBP/montage/owner/damage/poses/GC and actual ortho/FPS/RTS captures inspected. Directed4/full111clean/Python16/strict99/Shipping22/cook594 pass; actual cooked registry contains15character+19audio/material paths,37source hashes unchanged. One known engine render-thread warning per raster report, no suppression. C017H/M6.8 preferences portion preserved; genuine remapping now implemented over the sole EnhancedInput local profile: 29 discrete rows/33 unchanged mappings, three IMCs metadata-only migration with original recovery copies; single Mouse2D + two zoom axes + Escape locked. Native capture/save/defaults/error feedback/actual effective keys/HUD labels; directed FinalFlow6success (3clean/3device-warning reports), real RTS U/FPS K+G/held-release/file reload/conflicts/defaults. Full remap121success/exit0 (49clean/72only WASAPI) before final narrow modal cleanup; afterward ReleaseFinal6success (3clean/3WASAPI), final raster6success/text200% with only device/one engine MotionVector warning, no obsolete Slate-scroll warning, inspected actual binding-feedback PNGs. Strict109+4+9 and ShippingFinal5 succeed. CookRemapping2 exit0/594cooked/0incremental/7platform/601total,0errors/0warnings; actual engine-parsed Zen registry25input+15characters+7audio+12materials,37art/source+25accepted input hashes unchanged; repo/LFS/diff pass. First cook concurrency rejection retained in review, not SDK failure. All processes terminal. M6.8 CLOSED minimum offline; C017I/M6.9 CLOSED: five obsolete generation/migration scripts retired with exact recovery copies; retained schema1 authors actual readonly twice, native7clean/Python23/source71hashes/repo/LFS/diff pass. C018A local report/CI/release foundation verified: final launcher full121success/25report checks, controlled manual-main workflow/metadata/checklist; no remote activation. Next C018B/M7 remaining gates. Then M6.9/M7–M8, trusted-player launch/performance/distribution remain required. HANDOFF is compact state; no restart, automatic commits or premature whole-goal completion.

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
- Approved full-cost refunds are now implemented for manual cancellation and failed production from captured receipts (C-007/C-010); blocked credit retains the order for retry. Producer destruction remains separate work.
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
- The autosave stage emits the canonical request. C-016B/M5.4 persists a readonly versioned strategic snapshot via the dedicated save subsystem; I/O failures are observable but never roll back/repeat a committed turn. C-016C adds native manual/auto load/save/preference controls, confirmation and event-driven boundary refresh.
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
- Unit and building factories use deferred spawning plus stable `FName` definition IDs before actors become observable; the barracks queue also stores stable unit IDs. M5.4's C-016A/B versioned local save persists these IDs and strategic values, never raw actor pointers or transient map graphs.
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

C-011 morale-zero routing implemented (2026-10-05): the single vitals owner derives live Routed, rejecting player/action initiation while remaining vulnerable/registered. Existing vitals events restore camera/selection/same AI. One10m withdrawal away from the damage source (backwards fallback) uses NavMesh/CharacterMovement; arrival stays routed, unreachable settles Hold. No rally, despawn or false death/casualty. Directed2/2 and physical withdrawal1/1 clean; full39/39 clean, Editor42+4+4, IWYU69, Shipping13 and repo/LFS/diff pass. See [M3_ROUTED_STATE_REVIEW.md](M3_ROUTED_STATE_REVIEW.md). No new assets/dependencies/player claim; objective/terminal/campaign gates remain. Skills: `unreal-cpp-gameplay`, `ue-testing-debugging`, `ue-data-assets-tables`.

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

Melee wind-up and single-impact package implemented (2026-09-29): `FUnitCombatState` remains the sole recovery clock and now owns a cancellable pending impact that fires once after a wind-up equal to 25% of the derived `1 / AttackSpeed` recovery. Both AI-targeted and direct-control melee use that window, validate current enemy/range/world/life at impact, and enter the same `TakeCombatDamage` pipeline. Direct control traces at impact, not at button press. Replaced orders, Hold/Stop, mode changes and death cancel pending hits without refunding recovery; a target leaving range is missed. Source faction/world validation rejects friendly/self damage, while null remains valid for environmental/test damage. Pure-state and real-PIE targeted tests and full 34/34 regression pass. Evidence and remaining gates: [M3_COMBAT_WINDOWS_REVIEW.md](M3_COMBAT_WINDOWS_REVIEW.md). M3.4 is not closed: death-to-objective/campaign wiring, any retained ranged/special action policy, animation/feedback and packaged combat checks still require work. Skills: `unreal-cpp-gameplay`, `ue-testing-debugging`, `ue-module-build-system`.

- Establish one damage entry point and one cooldown source.
- Implement melee range, wind-up, hit window, cooldown, death, and faction validation.
- Remove double application between controller traces and unit attacks.
- Connect deaths to objective, UI, kill, and campaign-resolution events.
- Defer pilum/testudo unless one is explicitly included in the slice; remove misleading UI controls for unavailable actions.

#### M3.5 Buildings and production

C-010 failed-production refunds (2026-10-05): failed spawn and invalid queued promotion return original six-field paid cost to the recorded payer. Blocked credit retains receipt/queue; retries cannot spawn, double-refund or re-charge. Pending state projects through an appended RefundPending event and UMG text, without polling. Directed final 1/1 and full 38/38 clean, Editor final 11/11, strict IWYU 69/69, Shipping 6/6 and repo/LFS/diff pass. See [M3_TRAINING_FAILURE_REFUND_REVIEW.md](M3_TRAINING_FAILURE_REFUND_REVIEW.md). Producer destruction, capture-production policy and trusted-player gates remain; no new asset/module/config/dependency/cook/render claim. Skills: `unreal-cpp-gameplay`, `ue-data-assets-tables`, `ue-testing-debugging`, `ue-ui-umg-slate`.

C-008 guarded cancellation UI (2026-10-05): controller eligibility/read/command delegates to C-007 refund authority; native mouse CANCEL+REFUND appears beside TRAIN only for eligible active tactical production. Lifecycle/mode/owner/idle guards and real Slate single refund/queue promotion pass. Editor 38/38, directed 1/1 clean, IWYU 69/69, Shipping 5/5. D3D12 1/1 successful with known engine warning; two 1024×555 active/idle screenshots inspected without button overflow. Full `C008_Full` passes 38/38 with zero test warnings/errors; repository/LFS/whitespace pass. [M3_TRAINING_CANCEL_UI_REVIEW.md](M3_TRAINING_CANCEL_UI_REVIEW.md). No new authority/assets/input/dependencies; automatic failure/destruction refunds and other milestone gates remain. Skills: `unreal-cpp-gameplay`, `ue-ui-umg-slate`, `ue-testing-debugging`.

C-007 manual cancellation refund (2026-10-05): accepted active/queued training owns a captured `FTrainingReservation` (stable unit ID, original paying faction, six paid cost fields). `TryCancelTraining` returns the full original cost, preserves the order/queue on failed refund, prevents nested double-refund and promotes prepaid orders without recharge; legacy CancelTraining remains. Editor 10/10, directed real PIE 1/1 clean, IWYU 69/69 and Shipping 5/5 pass; full regression 38/38 clean in `C007_Full`, repository/LFS/whitespace/MD links pass. Test covers catalog price/owner changes, overflow/retry, every queued refund, nested calls and zero-cost/no-active cases. See [M3_TRAINING_REFUND_REVIEW.md](M3_TRAINING_REFUND_REVIEW.md). C-008 will expose guarded UMG cancellation; automatic failure/destruction refund behavior and the remaining M3.5 gates are not claimed. Skills: `unreal-cpp-gameplay`, `ue-data-assets-tables`, `ue-testing-debugging`.

Placement validation block implemented (2026-09-30): the existing definition-selected command now positions the authored footprint above the surface and validates center/rotated corners against buildable terrain, world bounds, blocking actors, canonical territory ownership and wallet affordability. `L_Testbed`'s pre-existing floor and the procedural terrain are tagged as buildable; nonphysical territory volumes are excluded from footprint blocking. Confirmation revalidates stale previews before a single charge, preserves retry after failure, and refunds through the canonical wallet if post-charge spawn completion fails. Real-PIE targeted 1/1 and full 34/34 regression pass with zero test warnings/failures; strict IWYU (63), Shipping (6), repository and LFS gates pass. [M3_PLACEMENT_VALIDATION_REVIEW.md](M3_PLACEMENT_VALIDATION_REVIEW.md) records evidence and limits. Skills: `ue-data-assets-tables`, `ue-testing-debugging`.

Initial gameplay barracks implemented (2026-09-30): GameMode now deferred-spawns one Rome-owned `ABarracks` from its authored definition at the original camp site, completes it through the normal state transition without a player charge, and omits the decorative barracks mesh. Real-PIE tests verify initial ownership, complete state, controller selection, visible TRAIN and no duplicate prop. Existing UMG/queue tests exercise canonical legionary cost and duration, exact charge, bounded prepaid queue, promotion without recharge, rejection and input guards. Targeted UI and placement 1/1 each, full 34/34 with zero warnings/failures, IWYU 63/63, Shipping 6/6, repository and LFS checks pass. See [M3_INITIAL_BARRACKS_REVIEW.md](M3_INITIAL_BARRACKS_REVIEW.md). A subsequent placement fixture destroys the deferred actor during spend and proves exact six-resource refund with no building/event; full regression still passes 34/34 (`20260930_125750_376`). M3.5 still needs an explicit production cancellation/failure policy, an objective-ready enemy HQ instead of its decorative mesh, and trusted packaged-player verification. Skills: `unreal-cpp-gameplay`, `ue-testing-debugging`.

- Use one building-placement command with a selected building definition.
- Validate terrain, bounds, overlap, ownership, and affordability before commitment.
- Use deferred spawning or explicit initialization so type data exists before health/visual/construction initialization.
- Charge resources once after all validations pass, with rollback if spawn/initialization fails.
- Replace decorative camp structures needed for gameplay with actual building actors.
- Use one barracks production queue and one data-defined legionary cost/duration.
- Ensure UI buttons consume UI input and do not depend on a world trace hitting the building.

#### M3.6 Encounter and victory

C-013 terminal encounter (2026-10-05): derived canonical resolution quiesces live tracked units and blocks action/damage/new training/construction progression, retaining survivors and paid receipts. Existing GameState events restore possession camera/pitch/context and clear transient input/selection. CampaignManager rejects terminal turns before economy/AI. Native UMG presents current tactical result plus guarded mouse restart; real current-level OpenLevel is reentrant-safe and resets the sandbox, not a persistent campaign. Directed victory/defeat realPIE2/2 and full42/42 clean, strictIWYU70 and Shipping19 pass; D3D12/offscreen2/2 succeeds with one known engine motion-vector warning; both terminal captures inspected for text/button fit. See [M3_TERMINAL_ENCOUNTER_REVIEW.md](M3_TERMINAL_ENCOUNTER_REVIEW.md). No new assets/modules/dependencies/cook/trusted-player claim. Next C014: canonical stable army identities/inventory before atomic battle application and Continue/campaign travel; the whole goal remains active. Skills: `unreal-cpp-gameplay`, `ue-ui-umg-slate`, `ue-testing-debugging`, `ue-project-context` (only authorized sections).

C-012 physical occupation objective (2026-10-05): actual HQ actor retains the visual/physical cube footprint and owns a Pawn-overlap capture zone. One weak registered objective validates actual live Rome occupation and no living Carthage in the world registry (including routed/unrostered enemies), then invokes the subsystem's private capture path. The real0.25s timer resolves the canonical result once; duplicate/destroyed actors reject, kills alone do not win and GameState projects the result. Directed final1/1 and full40/40 clean; Editor45+13, IWYU70+4, Shipping8, repo/LFS/diff/MDlinks pass. See [M3_OBJECTIVE_OCCUPATION_REVIEW.md](M3_OBJECTIVE_OCCUPATION_REVIEW.md). No new assets/modules/dependencies/cook/player claim; C013 terminal gameplay/UMG and campaign application/save remain. Skills: `unreal-cpp-gameplay`, `ue-testing-debugging`, `ue-data-assets-tables`.

Typed result/rules foundation implemented (2026-09-30): `FTacticalBattleState` is one pure tactical evaluator over validated known forces, world-local squad handles, reinforcements, casualties and objective capture. `FTacticalBattleResult` reports outcome, winner, objective/captured-objective stable IDs and ordered per-squad initial/surviving/casualty counts. Kills alone do not win without capturing the objective; elimination of all player units defeats them. Targeted 1/1, full 35/35 zero warnings/failures, strict IWYU 65/65, Shipping 4/4, repository and LFS checks pass. [M3_BATTLE_RESULT_FOUNDATION_REVIEW.md](M3_BATTLE_RESULT_FOUNDATION_REVIEW.md) records foundation limits. The following block wires live squads/deaths; physical capture, campaign/UI resolution and restart remain separate work. Skills: `unreal-cpp-gameplay`, `ue-testing-debugging`.

Live roster wiring implemented (2026-09-30): one `UTacticalBattleSubsystem` per Game/PIE world begins the known 15-versus-10 battle from the registry's initial squads, tracks the original world-local squad IDs across death, registers barracks-trained singleton reinforcements and publishes typed result changes. Real-PIE UMG integration proves the starting roster, one trained unit and one defender casualty without false victory. Targeted 1/1 and full 35/35 with zero warnings/failures, strict IWYU 66/66, Shipping 6/6, repository and LFS checks pass. [M3_BATTLE_WORLD_WIRING_REVIEW.md](M3_BATTLE_WORLD_WIRING_REVIEW.md) records evidence. Capture is intentionally not triggered until an objective actor validates the selected world rule; end-to-end victory/campaign/UI/restart and packaged-player gates remain open. Skills: `unreal-cpp-gameplay`, `ue-testing-debugging`.

Public casualty projection implemented (2026-09-30): GameState exposes/re-broadcasts the single tactical result after re-querying its world owner, and native UMG shows ally/foe survivor counts by event and after reattachment. A real-PIE test proves initial counts, a trained reinforcement, a defender death, unique bindings and canonical display values even when unrelated live casualties occur. Targeted 1/1 and full 35/35 with zero warnings/failures, IWYU 66/66, Shipping 5/5, repository and LFS checks pass. See [M3_BATTLE_UI_PROJECTION_REVIEW.md](M3_BATTLE_UI_PROJECTION_REVIEW.md). This advances M3.4 death-to-UI but does not resolve campaign kill events or M3.6 capture/terminal action/pixel-fit gates. Skills: `ue-ui-umg-slate`, `ue-testing-debugging`.

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
- M4.8: Approved and verified real-time possession without slow motion or pause. C-009 reads actual world/effective and actor dilation through repeated possession and teardown; runtime remains unchanged. Directed 1/1 clean, Editor 38/38 and IWYU 69/69 pass. See [M4_REALTIME_POSSESSION_REVIEW.md](M4_REALTIME_POSSESSION_REVIEW.md).

Evidence and exit gate:

- Automated test enters and exits the same unit repeatedly without losing AI, input, selection, or camera state.
- Possession of an enemy/dead unit is rejected.
- Death while possessed returns the player to tactical control and records one death.
- Damage and cooldown results match equivalent AI-controlled attacks.
- Tactical battle resolution remains correct after possession was used.

Possession return-cycle evidence added (2026-09-30): the real controller/player pawn enters and exits the same owned unit three times, preserving selection and camera transform while returning the unit to the identical tactical AI controller. Targeted 1/1 and full 35/35 with zero test warnings/failures; strict IWYU 66/66, Shipping Game up to date, repository/LFS/diff checks pass. This advances M4.3/M4.4, not M4's complete exit gate. No fresh packaged-player/physical-input verification. See [M4_POSSESSION_CYCLES_REVIEW.md](M4_POSSESSION_CYCLES_REVIEW.md). Skill: `ue-testing-debugging`.

Guard feedback implemented (2026-09-30): the possessed soldier's canonical block state updates the existing native HUD through its vitals event, with no UI tick or second owner. The real-PIE test proves immediate DOWN/UP/DOWN text transitions and block state; full regression 35/35 zero test warnings/failures, IWYU 66/66, Shipping 5/5, repository/LFS/diff checks pass. This advances only M4.7 feedback; camera collision, montage/hit visuals, time-scale policy and player-rendered gates remain open. See [M4_GUARD_FEEDBACK_REVIEW.md](M4_GUARD_FEEDBACK_REVIEW.md). Skills: `ue-ui-umg-slate`, `ue-testing-debugging`.

Possession pitch bounds implemented (2026-09-30): the live camera manager uses ±80° only while possession is committed, then restores its exact prior bounds on repeated ordinary exit or death. Targeted ownership/UI PIE tests each pass 1/1; final full regression 35/35 zero test warnings/failures, IWYU 66/66, Shipping Game up to date after a 4/4 runtime compilation, repository/LFS/diff checks pass. This advances only M4.7 look limits; camera collision, montage, time-scale, physical mouse and trusted player gates remain open. See [M4_PITCH_LIMIT_REVIEW.md](M4_PITCH_LIMIT_REVIEW.md). Skills: `unreal-cpp-gameplay`, `ue-testing-debugging`.

Estimated effort: 1–2 focused weeks.

### M5 — Campaign loop and persistence

Goal: connect the tactical encounter to a small but complete campaign.

#### M5.1 Campaign content

- Define approximately nine stable territory assets, their adjacency, initial ownership, production, settlements, and tactical-map reference.
- Initialize only Rome and Carthage as active factions in the slice.
- Render the campaign as data-backed UMG/map presentation rather than making visual actors authoritative.

Progress — C-001 (2026-10-03): the nine existing territory PDAs now author reciprocal neighbor IDs, preserving all 20 original connections. `FTerritoryCampaignState` owns the graph with definition copies; unknown/one-way graphs fail closed, local empty/self/duplicate references reject, and sorted ID queries no longer depend on actor transforms. Real PIE verifies an actor moved one million units cannot change campaign connections. Targeted 7/7, full 36/36 with zero test warnings/errors, strict IWYU 67/67, Shipping 6/6, Development cook 567 packages and repository/LFS checks pass. Evidence, migration backup and limits: [M5_ADJACENCY_REVIEW.md](M5_ADJACENCY_REVIEW.md). At C-001 exit, tactical-map references, two active factions and strategic UMG remained open; the subsequent C-002 progress below resolves only tactical-map references. No fresh packaged-player or save/load claim.

Progress — C-002 (2026-10-03): all nine territory PDAs now explicitly reference the existing `L_VerticalSlice` via typed soft World references. Shared metadata-only validation rejects missing/wrong-type/subobject/noncanonical paths without loading worlds. Editor authoring, independent disk readback and directed 4/4 pass; full 37/37 has zero test warnings/errors, strict IWYU 67/67, Shipping 6/6 and Development cook 567 packages pass. Evidence/backup: [M5_TACTICAL_MAP_REVIEW.md](M5_TACTICAL_MAP_REVIEW.md). At C-002 exit, active-faction initialization and strategic UMG remained open; C-003 below resolves the participant policy, not battle travel/result application/save/load.

Progress — C-003 (2026-10-04): FactionManager initializes exactly Rome/Carthage atomically and exposes ordered active/derived AI rosters. Nonparticipants reject player, wallet and diplomacy mutations; Gaul remains in the three-definition catalog and retains its authored territory, but receives no slice production or campaign AI stage. Campaign prototype unit movement now excludes inactive owners and the actual player. Real PIE preserves an extra Gaul soldier's HOLD across a successful turn. Directed 6/6 and full 37/37 have zero test warnings/errors, strict IWYU 67/67, Shipping 5/5 and repo/LFS checks pass. Evidence: [M5_ACTIVE_FACTIONS_REVIEW.md](M5_ACTIVE_FACTIONS_REVIEW.md). No new cook/player claim. At C-003 exit, public territorial projection and strategic UMG remained open; C-004 below resolves only the read/event contract.

Progress — C-004 (2026-10-04): public GameState now projects fresh, lexical-ID-ordered `FTerritoryPresentationSnapshot` values from canonical registered territories. Ownership/adjacency/definition reads remain on WorldMapManager, map/owner presentation comes from catalog metadata (including dormant Gaul), and participation comes from FactionManager. Unknown/not-ready/foreign-world/ended reads fail closed and clear output. A detachable ownership invalidation relay re-queries current owner under nested transactions; no cached rows, actor enumeration, new subsystem or asset changes. Base production is explicitly authored, not effective settlement yield. Directed 6/6 and full 37/37 have zero test warnings/errors; strict IWYU 67/67, Shipping 10/10 and repo/LFS/whitespace pass. Evidence: [M5_TERRITORY_PROJECTION_REVIEW.md](M5_TERRITORY_PROJECTION_REVIEW.md). M5.1 remains partial; next C-005 renders/inspects these rows in event-driven strategic UMG. No new cook/player or settlement/army/save contract claim.

Progress — C-005 (2026-10-04): native UMG now renders nine lexical-ID region cards and a read-only inspector from public GameState snapshots/events, visible/interactive only in Strategic mode. Cards retain dormant/unowned metadata and mouse HUD focus behavior; local selection clears on unknown ID, mode exit/destruct. Current owner, terrain, neighbors and six base-production fields re-query under nested notifications and catch up on reattachment; no gameplay selection, wallet, turn or army state is duplicated. Directed 2/2 and full 37/37 have zero test warnings/errors; strict IWYU 68/68 plus final incremental 7/7, Shipping 5/5 plus final 3/3 and repository gates pass. Real D3D12/offscreen 1/1 passes with one engine motion-vector warning; two inspected 1024×555 Slate captures drove an overflow fix. This is an abstract grid, not geographic cartography, multi-resolution polish or packaged-player evidence. See [M5_STRATEGIC_UI_REVIEW.md](M5_STRATEGIC_UI_REVIEW.md). M5.1 remains partial because live settlement state still resides on presentation actors; next C-006 moves that bounded authority into canonical territory state before exposing live/effective settlement data. No new assets, economy rules, army/save schema or cook claim.

M5.1 progress — C-006 (2026-10-05): live settlement/name/residents/bonus now belong to `FTerritoryCampaignState`; actor wrappers delegate, production enumerates stable IDs, and GameState/UMG show live settlement and explicit base/effective yield. Founding rejects unknown/empty/duplicate/overflow atomically, preserving residents 100 and Gold 100/Food 50 without new rules. State survives actor destruction. Pure 1/1, directed 7/7, full 38/38 clean, IWYU 69/69 and Shipping 12/12 pass. Render 1/1 succeeds with the known engine warning; two 1024×555 screenshots inspected and scrollable details added. Bounded M5.1 content/state work is complete, not the full campaign/player journey. Evidence: [M5_SETTLEMENT_STATE_REVIEW.md](M5_SETTLEMENT_STATE_REVIEW.md). Skills: `unreal-cpp-gameplay`, `ue-testing-debugging`, `ue-ui-umg-slate`.

#### M5.2 Campaign actions and AI

C-014 canonical army foundation (2026-10-05): CampaignManager owns persistent-identity-ready GUID/faction/territory/content-unit-ID/count values; SaveGame reflection round-trip and deterministic archive bytes are verified, without actor references. Pure mutation/import validation is atomic and output detached/sorted; current runtime initializes validated15Rome/10Carthage forces at first lexical owned territory and GameMode materializes that inventory rather than duplicate counts. GameState exposes guarded reads and unique detachable invalidation. Tactical casualties wait for explicit mapped application, not premature strategic decrements. Directed4/4/full45/45 clean, strictIWYU72+7 and Shipping9, repo/LFS/diff pass. See [M5_CAMPAIGN_ARMY_FOUNDATION_REVIEW.md](M5_CAMPAIGN_ARMY_FOUNDATION_REVIEW.md). No new assets/config/dependencies/disk-save/cross-world campaign/render/player claim. C015 next: encounter/squad-to-army/content mapping and atomic/idempotent result application; guarded strategic actions/AI and cross-world session/save/Continue remain. Skills: `unreal-cpp-gameplay`, `ue-data-assets-tables`, `ue-testing-debugging`, `ue-project-context` (authorized sections only).

C-015A encounter contract (2026-10-05): frozen validated adjacent deployment, explicit squad→army/content/count binding and complete-result/stale-snapshot guards. The pure coordinator commits casualties, retirement, victorious relocation and territory ownership together before recording the consumed encounter ID; replays cannot apply twice. The tactical evaluator/subsystem carry an explicit or fresh sandbox encounter ID. Directed4/4 clean, full49/49 successful (44 clean + 5 Editor connectivity/analytics HTTP-DNS warning results), Editor52+17/IWYU74/Shipping15 pass. See [M5_CAMPAIGN_ENCOUNTER_CONTRACT_REVIEW.md](M5_CAMPAIGN_ENCOUNTER_CONTRACT_REVIEW.md). This is a verified value contract, not an installed runtime authority or saved/travelled campaign. C-015B must integrate CampaignManager and defer WorldMap publication until both domains commit, materialize/map all content stacks, enroll paid reinforcements and prove committed observer reads/reentry in real PIE. No prototype camp→arbitrary-territory inference. Skills: `unreal-cpp-gameplay`, `ue-data-assets-tables`, `ue-testing-debugging`; no context regeneration.

C-015B runtime integration (2026-10-05): CampaignManager prepares a real adjacent Rome→Carthage encounter in a pristine Strategic arena, maps every content stack to actual squads and applies only the canonical tactical result. Guarded movement, staging rollback, complete army/owner/consumed-ID publication, nested mutation rejection, replay, reset and public relay teardown are verified in actual PIE. Directed7/7/full51/51 clean, strictIWYU74/Shipping22 and repo/LFS/diff pass. See [M5_CAMPAIGN_ENCOUNTER_RUNTIME_REVIEW.md](M5_CAMPAIGN_ENCOUNTER_RUNTIME_REVIEW.md). Prototype maps remain unchanged. Paid reinforcement enrollment was deferred to C-015C below. Session/travel/Continue/save, recruitment/action UI and Carthage AI remain. Skills: gameplay, data, testing and context (only authorized modules/subsystems/verification).

C-015C paid reinforcements (2026-10-05): acceptance captures receipt GUID and exact encounter/army destination; queued promotion does not recharge. Private producer-transaction enrollment validates the actual singleton and extends canonical inventory, snapshot/binding and tactical roster before events. Duplicate/malformed/stale/overflow enrollment, reentry, staging failure, blocked refund/no respawn, original six costs and changed-owner original-payer refund are verified; enrolled survivors/casualties apply in real victory/defeat PIE. Final directed10/10/full54/54 clean, strictIWYU74/Shipping14+3 and repo/LFS/diff pass. See [M5_CAMPAIGN_REINFORCEMENT_REVIEW.md](M5_CAMPAIGN_REINFORCEMENT_REVIEW.md). C-015D now needs selected-map session/travel/return, with explicit prepaid-order settlement/preservation before teardown. No disk save, strategy recruitment UI, full journey or trusted-player claim. Same four skills, context limited to authorized sections.

C-015D1 transfer contract (2026-10-05): versioned detached payload and temporary staged restoration for factions/wallets/diplomacy, territory/settlement values, armies, turn/outcome/threshold/autosave state, both RNG seeds and encounter/receipt ledgers. Invalid input changes no output; definitions/graph come from the current catalog. Allocated local squads cannot export as a new encounter; consumed outcomes return without a pending plan and preserve replay rejection in later encounters. Actual reflected SaveGame value archive is deterministic; player-entered city names use stable text, without serializing authored localized definitions. Directed4/4/full58/58 clean, strictIWYU76/Shipping28 and repo/LFS/diff pass. See [M5_CAMPAIGN_TRANSFER_CONTRACT_REVIEW.md](M5_CAMPAIGN_TRANSFER_CONTRACT_REVIEW.md). No runtime session/travel, real map metadata validation at import, prepaid-order travel settlement, save slot/corruption envelope or complete journey claim. C-015D2 must wire those boundaries and actual return/Continue. Same gameplay/data/testing/context skills, authorized context sections only.

C-015D2 runtime session foundation (2026-10-05): automatic GameInstance travel coordinator owns only phase/world/return metadata and a temporary one-use capsule. Actual strategic entry, selected-territory-map battle and terminal return preserve existing owners/IDs/city/wallet/turn state before first readiness; fresh local squads are materialized only in the new tactical world. Active AND queued prepaid orders are preflight-refunded or block travel unchanged; credits/cleared receipts precede notifications and nested mutators remain frozen. Wrong-world/nonce/content imports quarantine without default readiness, retaining the exact capsule for real retry. Directed6/full62 clean, strictIWYU78+4/Shipping30, repository/LFS/diff pass. See [M5_CAMPAIGN_SESSION_RUNTIME_REVIEW.md](M5_CAMPAIGN_SESSION_RUNTIME_REVIEW.md). D2's API foundation is verified, not user-facing Continue/recovery, save slot, AI or complete journey. C-015D3 now adds native event/query-based entry, army actions/selected-map battle, Continue and retry UI with lifecycle/physical Slate/render checks; retain the intentional sandbox Restart outside sessions. Same gameplay/data/testing/context skills; authorized context sections only.

C-015D3 session UI (2026-10-05): native controller commands query current world/phase/owners/adjacency/war/map metadata; GUID-only army cards inspect/select and move through existing authorities. Entry/selected-map battle/Continue/retry route actual Slate pointer events and OpenLevel. Event-driven diplomacy invalidation and independent GameInstance subscription cover not-ready quarantine; actual detach/reattach rejects stale callbacks and keeps one listener. Global campaign victory still permits Continue, returns without resuming turns and hides sandbox Restart. Strategic-session zoom and direct producers cannot create untracked sandbox units; direct arena deployment is rejected inside sessions. Directed5/full67 clean, rendered3/3 exit0 with six inspected Slate-root PNGs, strictIWYU81/Shipping12; repo/LFS/diff pass. See [M5_CAMPAIGN_SESSION_UI_REVIEW.md](M5_CAMPAIGN_SESSION_UI_REVIEW.md). In-process Slate is not OS/player proof; no new cook/save/full-milestone claim. Next C-015E: canonical strategic recruit/assign actions, their UI and constrained deterministic Carthaginian AI. Gameplay/data/testing/UI/context skills used; context changed only in the three authorized sections.

- Implement select territory, inspect, recruit/assign army, move, initiate battle, and end turn.
- Implement a deterministic Carthaginian AI with constrained goals: defend, recruit, and attack adjacent territory.
- Use seeded randomness recorded in campaign state if variation is needed.
- Apply tactical battle results atomically to casualties and ownership.

C-015E1 recruitment/assignment backend (2026-10-05): ready Strategic-session-only recruitment immediately purchases quantities at an owned settlement using the current authored six TrainingCost fields multiplied by count; the tactical barracks queue/TrainingTime remain unchanged. A chosen existing army must be owned/co-located; an invalid destination requests a fresh stable army ID, without query-side reservation. Assignment conserves selected content/count between distinct owned armies in the same region, retires empty sources and retains destination identity. Candidate roster and wallet commit before either event; nested mutations reject while read-only quotes can refresh UI from committed publication state. Unknown/content/class/availability/settlement/ownership/mode/pending/overflow/unaffordable/remote/over-assignment guards are proven. Pure3/runtime-final1/UIregression5/full69 clean, strictIWYU82/Shipping11; repo/LFS/diff pass. Actual selected-map travel preserves ID/debit and materializes purchased/assigned23 + Carthage10 only in the tactical destination. See [M5_CAMPAIGN_ARMY_ACTIONS_REVIEW.md](M5_CAMPAIGN_ARMY_ACTIONS_REVIEW.md). No new recruit/assignment UI, AI, render/cook/save/player or full-gate claim. C-015E2 must expose count/content/new-or-selected army/assignment target through native event/query controls, with real Slate/lifecycle/render verification; C-015E3 must implement defend/recruit/adjacent-attack AI through canonical army state, not prototype actor movement. Gameplay/data/testing/UI/context skills used; context only the authorized sections.

C-015E2 recruitment/assignment UI (2026-10-05): typed controller choices/quotes/commands drive mouse-only native controls for content, quantity1–100, new/selected army recruitment and GUID assignment-target cycling. Backend authority and validation remain E1; HUD cap is not a backend cap. Resource/army/session/mode/lifecycle events refresh eligibility, with no UI Tick, actor scan or duplicate wallet/roster. Full assignment selects the surviving destination; detached, hostile, pending and tactical callbacks reject. Two real Slate journeys cover six-cost debit, partial/full assignment, event-only insufficient-resource recovery, detach/reattach, actual selected-map battle/physical outcomes/Continue and persistent return quantities. Production contains only Legionary; a transient reflected-catalog duplicate tests the real unit cycle and is removed before purchases/teardown, without asset writes. Final full71/71 clean exit0; rendered2/2 exit0 (one known engine r.MotionVectorSimulation warning), six PNG inspected; strictIWYU83+final4 and Shipping9 succeed. See [M5_CAMPAIGN_ARMY_UI_REVIEW.md](M5_CAMPAIGN_ARMY_UI_REVIEW.md). No new cook, disk-save, AI, OS-input/player or complete milestone proof. Next E3: bounded canonical defend/recruit/adjacent-attack AI; optional user preference pending for real tactical defense versus autoresolve, real battle recommended. Gameplay/data/testing/UI/context skills used; context only the three authorized sections.

C-015E3A AI-planning foundation (2026-10-05): pure deterministic stable-ID/GUID planner and canonical CampaignManager readonly query propose one defend/recruit/adjacent-attack/rebuild action from current graph, ownership, armies, settlements, war, eligible factories and six-field wallet/costs. Default bounds: two recruits/action, settlement target15, retained relief-source garrison5; no RNG, query-side reservation, allocation or mutation. Malformed snapshots and graph fail closed; affordability and quantities are overflow-safe. Directed4/full75 clean exit0, Editor54/strictIWYU86+final4/Shipping succeeded; [AI planning review](M5_CAMPAIGN_AI_PLANNING_REVIEW.md) records evidence. Real session query tests prove live inputs, nonmutation and wrong-mode/pending/actual tactical rejection. No assets/config/dependencies changed. NOT installed in ProcessAIFactions yet: E3B must execute one canonical action after production, preserve/retry attack transfer after the turn pipeline, and consistently separate human faction from attacker in preparation, transfer, physical objective, outcome/application and UI. Actual incoming-attack win/loss/draw/Continue proof remains required; no render/cook/save/player/full-E3 claim. Gameplay/data/testing/context skills used; context only three authorized sections.

C-015E3B AI execution (2026-10-06): actual-session AIActions applies one canonical proposal after production, using E1's shared private faction-aware six-cost atomic recruitment or owned adjacent relief. Attack commits one unbound encounter plus save-safe queued flag; all nine stages finish before dispatch. Failed dispatch/retry preserves identity and runs no stages/spending/production/increment; terminal campaign outcome cancels only an unbound plan. Plan.PlayerFaction is independent of attacker throughout preparation/schema2 transfer/materialization/objective/evaluation/application/UI. Inverse-only deferred0.5s registered-roster AI pulse waits for readiness/NavMesh, attacks live defenders, physically occupies camp after their elimination and stops at resolution. Defender camera/formation and event/query retry/defend/draw messages verified. Directed8/8 and full83/83 clean exit0; final rendered2/2 successful (one known engine warning), five PNG inspected; strictIWYU87+final4 and Shipping25+final3 pass. Final full precedes a single cosmetic no-wrap line, verified by final builds/rendered journeys. [AI execution review](M5_CAMPAIGN_AI_EXECUTION_REVIEW.md) records exact artifacts, corrected test assumptions and limitations. No asset/dependency/config/cook/disk-save/trusted-player/full-milestone claim. Next C-016 safe local save/load. Gameplay/data/testing/UI/module-build/context skills used; context only three authorized sections.

#### M5.3 Economy and victory

- Process territory production once per turn.
- Add only the minimum upkeep/recruitment rules required by the product brief.
- Implement explicit conquest victory and loss conditions for the slice.
- Ensure maximum-turn behavior produces an intentional result rather than silently stopping.

#### M5.4 Local save/load

C-016A archive contract (2026-10-06): typed save failures, detached generation/settings/transfer2 record and dedicated USaveGame class; fixed magic/version1/length/CRC32 envelope around the existing SaveGame-reflected values. Four-MiB payload cap, deterministic bytes, atomic output, explicit incompatible-version rejection and strategic-only boundary. CRC detects accidental corruption, not authentication. Current catalog/content/map validation is still required before any live restore. Directed6/6 clean exit0, Editor58/strictIWYU88/Shipping succeeded; queued AI accepted ID/human/retry flag, all saved domains/settings/RNG/ledgers, engine USaveGame memory archive and malformed-data rejection verified. See [save archive review](M5_CAMPAIGN_SAVE_ARCHIVE_REVIEW.md). No disk writes/manual/autosave/load runtime/UI/full84/render/player claim. Next C-016B connects readonly safe capture, dedicated subsystem, bounded platform slots/backup preservation, autosave and staged current-content/session load; C-016C native controls and complete save/advance/load proof. Existing full83 remains E3B evidence, no broad retest for an unused archive helper. Gameplay/data/testing/module-build/context skills used; context only three authorized sections.

C-016B runtime (2026-10-06): dedicated GameInstance save subsystem owns preferences/status only, not campaign authority. Manual/Autosave fixed two-generation slots use bounded Windows file preflight and C016A envelope through platform raw slot APIs; inactive-file write/readback preserves good backup, explicit backup recovery, incompatible-version rejection even with an older good file and failed-write status. Readonly actual strategic/autosave capture never freezes/refunds; final stage persists committed turn/domains/IDs/RNG/ledgers/queued intent, disabled autosave does no I/O, I/O failure never replays/rolls back the turn. Load validates current domains/catalog/content and world metadata before existing source receipt/refund/freeze, then nonce-checked actual strategic reload restores owners/preferences before readiness. Legacy named wrappers use fixed Manual slot, not filesystem paths. Directed6/full90 clean exit0; final directed6 includes actual observer GC with rooted staging. Strict IWYU90 first found one missing direct include, final5 succeeded; Shipping21 succeeded. Full90 precedes only final rooting/GC fixture/direct-include changes, verified by final directed6/builds. See [save runtime review](M5_CAMPAIGN_SAVE_RUNTIME_REVIEW.md). No new asset/dependency/config/renderer/cook/trusted-player/full-milestone claim. Next C-016C native controls/confirmation/feedback, event-driven deferred boundary refresh, sandbox resume and failed saved-load quarantine/settings recovery proofs. Gameplay/data/testing/module-build/context skills used; context only three authorized sections.


C-016C/D (2026-10-06): native fixed-slot Save/Load/LoadAuto/Autosave/confirm/cancel/status reuses persistence authority, rejects stale/detached/current-world/mode/turn-invalid callbacks, binds one persistent listener and coalesces a weak cancellable next-tick refresh after operation/turn guard release, not UI Tick/polling. Panel bottom-center strategic, top-center sandbox to leave selection/production free. Directed9 clean covers original disk6 plus native pointer/lifecycle/confirmation/disabled-auto/saved settings before readiness, initial sandbox resume and failed incoming saved-load quarantine with same-capsule repaired native retry/preferences isolation. Final rendered9 successful (8clean+1known engine warning), ten actual PNG inspected. Combined FirstPlayable directed2 clean reuses correct army UI: purchases/assignment/save/nine-stage turn/selected mapped battle/materialized casualties/physical result/Continue/postbattle save/second turn/confirmedload/full canonical equality+IDs/RNG/consumed ledger. Defeat's legitimate enemy counterattack is resolved before safe strategic load, not suppressed or made loadable in tactical mode. Strict IWYU91+layout4, Shipping8+layout3 pass; final full95 verified clean. Reviews [save UI](M5_CAMPAIGN_SAVE_UI_REVIEW.md) / [journey](M5_FIRST_PLAYABLE_JOURNEY_REVIEW.md). No new assets/deps/config/cook/trusted-player/whole milestone proof. Next C-017/M6 native responsive shell/minimap/event lifecycle without redesigning canonical gameplay; production assets require GPT briefs/real available tools. Gameplay/data/testing/UI/module-build/context skills used; context only three authorized sections.

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

Pending art work package: [visual quality brief](M6_VISUAL_QUALITY_BRIEF.md), VQ-01 legionary and VQ-02 small camp sample. Owner selected Roman semi-realism on 2026-10-19: natural proportions, detailed materials, military appearance. A verified official workspace-portable Blender5.2.2 and explicitly CC0 anatomy data now support a REAL versioned [source candidate](SourceArt/Characters/RomanSoldier/semireal_v1/README.md): three GLBs/LOD candidates, rig17, five clips, baked PBR, packed editable BLENDs and reimported-pose renders. Actual isolated UE5.8.2 import/readback passes for the selected GLB (186cm/17bones/5clips/8slots/3shared textures); this does not change the live game. [Source review](M6_SEMIREAL_SOURCE_REVIEW.md) preserves attempts, tests and limits. Grip/raised-arm coverage/cloth/face quality, historical period, real RTS/FPS camera review, installed LOD performance and owner visual acceptance remain OPEN; not final art approval, a schema1 replacement, or M7 closure. VQ-02 camp sample remains pending.

Selected continuation candidate: [semireal_v2](SourceArt/Characters/RomanSoldier/semireal_v2/README.md), attempt10. Adult anatomical proposal, actual baked closed grip, body-fitted clothing/torso weights, upright shield block and in-place foot compensation; exact six-file recipe snapshot retained. Three LOD GLBs (49782/12444/4978triangles), Blender reimport/gallery and actual UE5.8.2 import plus D3D12 visibility/Raw AND Compressed17bone motion across five phases per clip PASS. These are real source/engine checks, not accepted art, native game-camera/AnimBP integration, installed engine LODs, 100-unit performance, packaged QA or milestone closure. v1/schema1/baselines/gameplay stay intact. Next C-018H_SemirealGamePreview: controlled private game-context RTS/FPS preview, not a silent replacement in the owner's running Editor; preserve actual gameplay authorities and verify new textured materials/new clips rather than old flat overrides.

GamePreview progress (2026-10-19): private game copy created/306file SHA256 snapshot and actual native-module/candidate readback PASS; already verified18candidate uassets reused without reimport. Private Editor build114actions PASS; new independent AnimBP with four new-rig graph clips, new Attack montage and reflected bundle authored in the candidate namespace. Missing-opt-in negative probe correctly failed without saves; authoring and fresh readonly native test each1/1,0errors/0warnings,exit0. Original306inputs and preexisting private Content unchanged. Tools/ASSET_AUTHORING.md documents create-only setup/guards and fixture; [review](M6_SEMIREAL_SOURCE_REVIEW.md) retains exact logs/hashes. This proves native authoring/readback, NOT runtime pose/camera or appearance acceptance. Next C-018H_SemirealRuntimeBindings within GamePreview: preserve PBR during canonical faction/damage updates, then actual actor transform/poses/selection/lifecycle/RTS/FPS. Main Source/Config/Content/baselines and clean91c checkout remain intact; M7/M8 gates remain open.

Latest runtime progress (2026-10-19 21:32UTC, supersedes graph-only status above): C-018H_SemirealRuntimeBindings verified in the private game. Optional presentation policy keeps per-slot PBR parents/MIDs and canonical faction/health parameters, defaults false preserving schema1; main Source has the compatible extension and directed Editor test, no art adoption. Actual pose/timeline test exposed missing CompositeSection in V1 (playing without deformation); preserved failures and fixed NEW IntegrationV2+Runtime03 now pass real Idle/Walk/Block/Attack/Death checks without relaxed thresholds. Characters4/4 clean; private full141success/0failed/0pending, with documented audio/network/intentional-image warnings; strict IWYU, private Shipping78actions and main Editor11actions PASS. Runtime03 material graph authoring and fresh readonly10asset receipt PASS; NO GPU shader/camera/player proof. [Runtime review](M6_SEMIREAL_RUNTIME_REVIEW.md) records evidence/cohort/limits. Next C-018H_SemirealCameraPreview: real private D3D12/SM6 RTS/FPS/controller/head-hide/scale/facing/handedness and owner art review. Main Content/Config/baselines and clean91c checkout untouched; LOD/new-model100-unit performance and M7/M8 still OPEN.

Latest camera progress (2026-10-19 22:58UTC, supersedes initial runtime status): C-018H_SemirealCameraPreview TECHNICAL gate verified privately in real D3D12/SM6 game viewport. Current Runtime05/IntegrationV3 preserves brass (slotCrest is fittings in this mesh) and adds optional same-body/local-player hand reach after a stricter real FPS test caught handX1765 outside1280. No viewmodel, stretch, camera/FOV/scale/collision/damage changes. Current hand872.727/546.182, own-mesh GPU witness41443pixels; RTS production zoom/handedness, possession/head hide and actual pawn/head/IK restoration PASS. RTS02/FPS04 inspected, each native1/1/0errors/1retained MotionVector warning/exit0; no shader/material compile failure signatures. Stronger Character checks4/4clean; final full141success(140clean/1intentional warning),0failed/pending; strict builds, private Shipping5actions/main Editor11actions and fresh Runtime05 readonly PASS. [Camera review](M6_SEMIREAL_CAMERA_REVIEW.md) preserves failures, source/PNG cohort and limits. No main Content/Config/baseline adoption or owner quality approval; source GLBs unchanged. Next C-018H_PrivateLODCheck: add verified LODs ONLY to a new private candidate, then measure actual new-model100-unit GPU performance on declared hardware; keep reference-hardware/acceptance gates distinct. VQ-02 camp sample remains independent GPT asset work; M7/M8 still OPEN.

Owner visual review (2026-10-19): the user ran L_VerticalSlice in the Editor and rejects the model quality. The provided screenshot confirms gameplay/HUD rendering in the Editor, not the full manual journey or Development/Shipping player gates. M6.7's existing closure proves the minimum original skeletal/animation integration, NOT owner acceptance of final art. The current recipe-generated characters, prototype buildings/environment and test-floor presentation must not be described as production-quality visuals. Next: agree an art target and review one representative legionary plus a small environment sample before approving broader replacement. No new art direction, purchases, DCC installations, mesh replacement or gameplay redesign is authorized by this status note; final visual acceptance remains OPEN and does not waive M7. Real usable meshes/materials/rigs/animations are required; concept images alone do not fulfill this work.

Goal: replace prototype/debug presentation with a coherent slice without changing authoritative gameplay state.

C017A/M6.2 (2026-10-06) verified: native schematic minimap uses validated weak registered units, registered objective/current result and controlled Pawn; strategic public territory positions/owners/authored adjacency. Only visible shell placement subscribes; committed spawn/reindex/death/removal invalidation, tactical indexed movement5Hz, no strategic polling/UI Tick/world scans. Stable finite projection, color-safe shapes and current-world/parent readiness/lifecycle guards. Directed2/full97 clean; rendered2 successful (one known engine warning), both PNG inspected; strictIWYU93/Shipping17/repo0M1warnings/LFS/diff pass. [Minimap review](M6_MINIMAP_REVIEW.md) and [HANDOFF.md](HANDOFF.md) record exact evidence. No new assets/deps/config/input/authority, no cook/player/full-M6 claim. Next C017B/M6.1 responsive layout/legibility; tactical debug-panel overlap remains to address.

Work packages:

- M6.1: Implement one responsive UMG shell for strategic, tactical, possession, selection, production, objectives, and end-state panels. C017B closed: SafeZone/flex/complete scrolling documents, exact real-window6 resolution/scale cases, canonical pointer/production/strategic/possession/physical-outcome proof, UI10/full103 clean; rendered6 (one known engine warning),30 PNG/seven final captures inspected, strictIWYU94/Shipping9. [Review](M6_RESPONSIVE_SHELL_REVIEW.md). Larger/general accessibility/localization/gamepad/settings remain M6.8, not silently claimed.
- M6.2: Implement a real minimap fed by registered units/objectives and territory state, with marker lifecycle handling.
- M6.3: Replace blank generated Widget Blueprints; either remove design JSON or make it a validated source consumed by an idempotent generator. C017C closed: verified native production tree is the single layout source; unused HUD JSON and both empty-widget factories retired, no binary asset deleted, readonly AST/absence3 checks pass. [Review](M6_NATIVE_UI_RETIREMENT_REVIEW.md). Other generators remain unvalidated pending their own work packages.
- M6.4: Create actual material instances and visual states for faction, selection, valid/invalid placement, damage, and construction. C017D closed: [material assets and actor integration](M6_MATERIAL_ASSET_REVIEW.md)3 bases/9 persistent MICs, strict saved schema/readback/idempotence/negative probes; canonical owner/health/state actor-local MIDs and geometric selection, unused manager/factories retired. Native1/full104 clean, final strictIWYU94/Shipping23/retirement5 pass. Cook572 exit0; actual cooked registry lists all12 materials, source hashes unchanged. Final actual player viewport and orthographic capture each pass projected-roof color/stripe/construction/damage and selection-region pixel gates (one known engine warning each); both PNGs inspected, perspective capture also passes. Immediate capture was an invalid fixture: retaining the scene across12 real frames/1 second resolves missing previews/black indicators with no runtime/asset change. Final native1 clean; bounded timeout/real view/cleanup preserved. Historical failures retained, not suppressed. No standalone/trusted-player or whole-M6 claim.
- M6.5: Replace null Sound Cue libraries with referenced assets; use audio components/classes or MetaSounds appropriate to the selected content. C017E CLOSED: [native feedback](M6_AUDIO_FEEDBACK_REVIEW.md) one persistent SFX class/six actual monoPCM16/24kHz waves, owned deterministic recipe/strict authoring/readback/idempotence/rejection probes. Automatic world-local eight-voice service projects validated volume/coalescing/result dedup and committed selection/orders/construction/combat/victory/defeat; RTS ground listener, actual playback/teardown/GC. Directed7/full108 clean, Python5/strict96/Shipping14 pass; cook579 exit0, actual cooked registry contains seven audio paths, hashes19 unchanged. Five unused sources/null-cue factories retired Git-recoverably, binary refs0/no binary deletion. Incorrect kills-only fixture corrected to real occupation without policy change; raw static mesh GC cache corrected to weak bootstrap references. No external sample/score/voice/dependency/subjective listening/trusted-player claim. Volume UI/persistence M6.8, whole M6 open.
- M6.6: Decide the fate of procedural mesh generators. C017F CLOSED: [retirement review](M6_PROCEDURAL_RETIREMENT_REVIEW.md). No production procedural helper retained: caller/binary refs0 outside own sources/dependency; retired two tracked sources/dependency, disabled unused plugin. StaticMesh terrain/buildings/unit prototypes preserved; no binary deleted. Directed13/Python13/Editor51/strict95/Shipping50/cook579 pass; actual cooked registry retains7audio+12materials, hashes19 unchanged/repo/LFS/diff pass. Full108 immediately preceding baseline reused appropriately, not another full-suite claim. Skeletal production visuals remain M6.7.
- M6.7: Add skeletal characters, Animation Blueprints, montages, and minimum readable faction silhouettes. C017G CLOSED minimum offline gate: [character review](M6_CHARACTER_ASSET_REVIEW.md). Original Rome/Carthage/Gaul696/764/728tri profiles/17shared bones/five30fps clips/three MICs plus functional12node AnimBP/attack montage/no damage notifies/one immutable required bundle,15uassets/3LFSsources. Actor-local owner/damage MIDs, capsule42x96/nav/selection/possession/IDs/save/gameplay authorities preserved; legacy swing retired. Real bone/geometry tests caught held-block pin, head occlusion and backward facing; scoped owned repairs/presentation-only fixes pass actual poses/hand-forward/lateral anatomy/player exit/GC. Additive Gaul changes only its exact owned bundle reference; previous imports/source bytes intact. GaulBinding4clean/full111clean/Python16/strict99/Shipping22 pass. Actual three-profile/five-pose ortho, possessed-player and default-tactical RTS images inspected/pixel gates pass (one known engine MotionVector warning each). Cook594/7platform/601total exit0/error0/warning0; engine-parsed actual Zen cooked registry contains15character+19audio/material paths,37source hashes unchanged. Readonly readback/rejection3 pass; no author/repair/migration rerun, no historical-authenticity/multiplayer-per-view/trusted-player/whole-M6 claim. Next M6.8.
- M6.8: Add scalable text, color-safe selection indicators, volume controls, sensitivity controls, remapping support, subtitles if voiced content is added, and pause/settings flows. C017H CLOSED minimum offline: [preferences/accessibility review](M6_PLAYER_PREFERENCES_REVIEW.md), one GI owner/strict schema1/recoverable Saved-only storage, audio8voices/live look/pan/invert; ProjectionFinal10clean, foundation strict102/Shipping before integration. Layout font projection/nested/dynamic/SpinBox/custom minimap scale, reachable native detached settings form/controller-owned offline pause/F10/focus. ApplyRoute1 exit0/12success (seven device-warning reports): actual UI commit/save/error/cancel/held-release/FPS resume, GUID fixture only/no personal-file writes. UIText2xAll3 six real resolution/DPI cases pass with text200%, fixed visible Resume/explicit Tab/Escape/pointer/geometry gates; captures inspected, known engine/device warnings reported. Footer clipping/hidden focus/navigation and premature fixture input/layout failures repaired, history retained. FullUI118success/exit0 (71only device-warning reports), strict incremental17/Shipping19/repo/LFS/source37hashes pass. Historical pointer/bootstrap2success established settings/profile enabled but0mappable/33. Genuine remapping now implemented: 29digital rows/three metadata-only IMCs (single Mouse2D/two zoom axes/Escape fixed), stable per-direction IDs, all4 registrations, profile queries/events, actual engine save/error feedback and project-only defaults; no duplicate JSON. FinalFlow6success (3clean/3WASAPI-warning reports) proves actual Slate U capture/RTS physical U/FPS K+G, mode-aware conflicts/defaults/file reload/held-release/HUD-label events. Strict109+4 succeeds. Full remap121success/exit0 (49clean/72only WASAPI) before final narrow modal cleanup; afterward ReleaseFinal6success (3clean/3WASAPI), final raster6success/text200% with only device/one engine MotionVector warning, no obsolete Slate-scroll warning, inspected actual binding-feedback PNGs. Strict109+4+9 and ShippingFinal5 succeed. CookRemapping2 exit0/594cooked/0incremental/7platform/601total,0errors/0warnings; actual engine-parsed Zen registry25input+15characters+7audio+12materials,37art/source+25accepted input hashes unchanged; repo/LFS/diff pass. First cook concurrency rejection retained in review, not SDK failure. All processes terminal. No whole-M6/goal/trusted-player/multiplayer-pause claim.
- M6.9: Make editor generation scripts idempotent, versioned, and validated, or remove them in favor of committed authored assets. C017I CLOSED: [editor generation review](M6_EDITOR_GENERATION_REVIEW.md), five obsolete shell/map migration scripts retired with exact verified dirty/untracked recovery copies; persistent native/UI/material/map/adjacency assets unchanged. Correct schema1/read-only/create-missing-only material/audio/character authors retained; [supported contract](Tools/ASSET_AUTHORING.md), no automatic setup generation. Targeted Python15/full Python23 pass; all three authors actual engine readonly twice exit0 (six processes), no asset creation. Native AuthoredReadback7/7clean exit0 confirms persistent references/rig/audio/metadata/adjacency;71hashes (input25/prior art37/ninePDAs) unchanged, five exact retirement copies intact, repo/LFS/diff pass. No C++/uasset/plugin/config changes; previous build/cook gates remain valid. Next C018A/M7. No automatic commit or whole-M6/goal claim.

Evidence and exit gate:

- No required gameplay interaction depends on Canvas coordinates or debug messages.
- Every production widget is instantiated, navigable, and bound to canonical state.
- Missing asset validation reports zero required references.
- Audio/material managers are either used and configured or removed.
- UI remains usable at agreed target resolutions and scaling settings.

Estimated effort: 2–6 weeks depending primarily on art/audio availability; engineering can overlap M3–M5 after M2 interfaces stabilize.

### M7 — Automation, CI, security, and release gate

Owner follow-up (2026-10-19): «atacan segui von todo me voy a dormir, sin frenarte» confirms enemies attack in the user's Editor test, following the training confirmation. Close both reported playtest availability issues without an AI/runtime patch; retain the navigation failure observation as an unconfirmed conditional risk, not a diagnosed regression. Continue authorized implementation autonomously, not repeated questions about these resolved reports. This is not full manual-journey, packaged-player or release-gate proof.

Owner Editor playtest report (2026-10-19): initially reported inactive enemies and missing unit creation. Follow-up «bien si, esta entrenando y anda» confirms the user found training and has it running; close the missing-training-option report without a code change. This alone does not prove completed unit spawning, refunds, persistence or Development/Shipping player QA. Enemy inactivity remains OPEN pending clarification/reproduction; do not infer it is resolved from the training confirmation. Read-only diagnosis found a conditional AI failure path (failed navigation becomes Hold, while automatic acquisition only runs from None), but no live per-unit navigation state has yet confirmed that cause. Training UI is selection-dependent: completed owned Barracks, Tactical commands available, valid trainable content; Strategic zoom, possession or resolved battle suppress availability. Reproduce the exact actor/selection/mode before changing behavior or declaring an AI fix. Do not treat prior automated gates or art concepts as proof of owner acceptance or packaged QA.

Goal: make regressions visible and produce repeatable release candidates.

Current C018A (2026-10-07), local foundation VERIFIED: [automation foundation review](M7_AUTOMATION_FOUNDATION_REVIEW.md). Shared strict per-test/count/coverage validator and25negative/positive checks (broken-invariant child CLI exit1, parent CI exit restored0); final actual BuildProject Test exit0/121success (49clean/72only two known WASAPI notices), report20261007_203241_359, after final modal cleanup. Local [CI workflow](.github/workflows/unreal-ci.yml) is manual/protected-main only, NO PR hooks until verified workflow-restricted isolation; [runner activation](CI_RUNNER_SETUP.md), bounded metadata/readback/path-escape rejection, [release checklist](RELEASE_CHECKLIST.md) and unreleased changelog implemented. YAML/PowerShell/prerequisites/repo checks pass; no remote run/runner/branch policy/commit/push configured. Next C018B; M7 remains OPEN for actual remote clean checkout, functional/render/performance/trusted-player/distribution gates. Unsigned standalone block preserved; no security workaround. Owner runner choice requested, no answer yet.

#### M7.1 Test layers

- C018I (2026-10-16), independent of deferred Windows/VM QA: added native
  `Campaign.SaveDisk.StorageConflicts` and made it a required Test-action anchor.
  Conflicting valid generations, identical-duplicate acceptance/rotation,
  incompatible versions in either file, generation exhaustion, both-file byte
  preservation and complete saved campaign equality are verified. Directed1 and
  final affected SaveDisk10 all clean/exit0; final Editor build4 and report-contract
  25 checks pass. [Evidence](M5_CAMPAIGN_SAVE_RUNTIME_REVIEW.md).
  Existing runtime behavior retained; no Shipping/cook/remote/trusted-player claim.
  User requested leaving Windows/VM work pending and continuing independent work;
  do not resume that branch automatically. M7 stays OPEN.
- C018J (2026-10-16): checksum-valid malformed payload regression implemented in
  `Rules.SaveGame.PayloadValidation`, now required by Test action (21 anchors).
  Fifteen bounded archive/semantic rejection cases assert complete output atomicity;
  valid independent wire bytes match Encode and a subsequent valid decode succeeds.
  Editor4/exit0, directed SaveGame2 clean/exit0, report25/parse/diff pass.
  [Evidence](M5_CAMPAIGN_SAVE_ARCHIVE_REVIEW.md). No codec defect found or runtime
  change needed; reuse SaveDisk10 and existing Shipping/cook/render evidence.
  Clean-candidate local-commit authority granted by owner (2026-10-16); C018H
  candidate/independent local checkout VERIFIED below, without push. Still needs an isolated
  licensed CI runner; no automatic staging/commit/push/remote policy changes.
  Deferred VM/player, dependency-security, attribution and owner budgets remain OPEN.
  Historical post-C018J permission block is superseded by that explicit approval;
  scope intact, not completed. HANDOFF.md records candidate progress and retained evidence.

C018B provisional capture (2026-10-07) COMPLETE: [current load review](M7_CURRENT_LOAD_CAPTURE_REVIEW.md). Existing correct fixtures retained, three independent D3D12/offscreen processes per50/100 added units (75/125total), six exit0/1success each with only known engine/device notices. Actual240numeric CSV frames each/full actor-center projections/nonzero measuredGPU/nonempty traces/hashed copies;16pure parser tests and repo checks pass. Game-thread p95 ranges13.36–14.41ms (+50) and17.57–19.13ms (+100); NOT a final FPS target, screenshot pixel proof, complete combat/production/objective workload or packaged-player gate. No optimization/new runtime/assets; preserve captures and inspect traces before architectural decisions. FPS target and isolated runner questions pending. Next C018C remaining functional-map/render/performance/security/release work; M3.7/M7 open.

- Add native automation tests for resource math, transactions, diplomacy pairs, turn sequencing, combat calculations, definitions, and save migration.
- Add deterministic Functional Test maps for tactical commands, building/training, possession, victory, and campaign handoff.
  - C018C five-map coverage verified (2026-10-08): persistent Editor-only schema1 maps reuse the existing native navigation/combat/production/ownership-possession/occupation scenarios through real AFunctionalTest outcomes, including broken-assertion→Failed propagation and restored negative ownership probes. Actual full128clean/exit0 with10required anchors; strict111+final5, Shipping, cook594/7platform/601total/0errors/0warnings and actual engine-parsed registry absence of /Game/Tests/test-module refs pass. Required59production packages retained; fresh author rerun1clean/no write, accepted71source+5fixture hashes unchanged. Read-only/default, explicit create-missing native author; no runtime gameplay rewrite. [Review](M7_FUNCTIONAL_MAP_REVIEW.md). Next C018D dedicated campaign phase gate +existing native world-travel coordinator; full encounter/performance/render/remote/trusted-player/release gates and M7 remain OPEN.
  - C018D campaign map verified (2026-10-08): sixth schema1 fixture runs a non-mutating source FunctionalTest phase, then the unchanged native coordinator across actual strategic/battle/return worlds; checks stable IDs/wallet/turn/casualties/refunds/consumed result/readiness3 and expired source actor. Source Success is NOT journey completion. Runtime errors stay captured after the actor phase; negative log-capture and failed-gate/no-transfer probes pass. Full131clean/exit0 with13anchors, strict111, cook594+7platform601total/0errors/0warnings, actual all-test-map/module-ref exclusion +required59production packages, author rerun1clean/no writes and77unchanged hashes pass. [Review](M7_CAMPAIGN_FUNCTIONAL_REVIEW.md). Deterministic tactical/building/possession/victory/campaign map coverage implemented; native Slate already covers UI, no Automation Driver needed. Next C018E full-encounter workload/traces/performance; rendering baselines/remote runner/branch governance/trusted-player/release gates and M7 remain OPEN.
- C018E LOCAL EVIDENCE VERIFIED (2026-10-08): [finished-encounter performance review](M7_ENCOUNTER_PERFORMANCE_REVIEW.md). Native50/100 INITIAL TOTAL soldiers (25/75 additions), canonical enrollment, natural recruitment/damage DURING training, real RTS/AI/FPS-realtime and automatic physical victory. Six independent D3D12 processes exit0/report1success each; actual1920x1080 viewport/full40/90Roman-center projection (NOT pixel/all100visibility), nonzeroGPU, variable CSV2776/2311/3641/2379/3910/2645frames, six bounded-region Insights exports and hashes verified. Game p9550=22.36–55.29ms (outlier retained),100=27.32–35.41ms; Editor Slate/Mass overhead identified, CPU-filtered export also includesGPU by engine implementation. No optimization/target-FPS/player claim. Actual full133success (129clean/4only HTTP-timeout warning reports)/exit0 with15anchors, strict111/exit0,39parser checks/4PSASTs/repo/LFS/diff/77unchanged hashes pass. Runtime/Game/assets/deps unchanged; reuse prior cook-exclusion/Shipping gates, not trusted-player. Old C018B evidence untouched, all processes terminal. Next C018F rendering regression/reviewed baselines; M3.7/M7/goal, remote runner/governance/owner-trust/distribution remain OPEN. HANDOFF.md is current checkpoint.
- Use Automation Driver from the test module for real UMG input, focus, and visible-state assertions only if native/Functional APIs cannot cover those behaviors.
- Use screenshot tests only for rendering regressions and commit reviewed baselines; never treat taking a screenshot as success.
  - C018F CHARACTER REGRESSION LOCAL CLOSED: [native raster review](M7_RENDERING_REGRESSION_REVIEW.md). Three agent-reviewed actual D3D12/1280x720 baselines (profiles/default RTS/actual possession), immutable create-missing-only copies/LFS/SHA256+SHA1. Six independent comparisons pass unchanged fixed color8/local.01/global.002; profiles/RTS0, FPS local.00510–.00673/global.000302–.000343. Tests-only world-scoped time0/no blur/exposure/temporalAA/AA, no runtime/assets/global changes. Buffer/file corruption/missing/dimension/new-image rejection and native no-capture failure/report rejection verified. Final full136success (52clean/84only WASAPI+intentional missing-file warning reports)/exit0/18anchors; strict112/83.52s/exit0;15purechecks+2authoring guards/5PSASTs/repo/LFS/diff/77accepted source+3raster hashes pass. First full9fail and directed8fail retained: inherited functional bridge setup warning elevation fixed before EACH owned PIE, no suppression; actor explicit policy/error-afterphase/no-transfer guards all verified. Character gates done, not whole-M7/HUD/temporal/player. NEXT C018F native HUD/200% UI regression family; explicit baseline commit/remote/governance/trusted-player/release OPEN, no auto commit.
  - C018F HUD/UI LOCAL CLOSED: [separate UI review](M7_UI_VISUAL_REGRESSION_REVIEW.md),84 original native resolution/DPI/text/screen images inspected; immutable PNG/SHA1/provenance family created byte-identically. Three defective cohorts remain rejected; native negative witnesses exposed label/FPS overlays, four-edge strategic-button clipping and crosshair text200 overflow before focused existing UMG wrapping/event/Footer220→180/AutoSize fixes. Fonts, scroll, aiming and authority retained. TWO independent complete84 native comparisons PASS unchanged color8/local.01/global.002, maxima .005859375/.000228407 and .006076389/.000261863; each12 known MotionVector warnings retained. Final post-fix full137Success/136clean/1intentional codec warning,19anchors/exit0; IWYU112/73.05s/exit0 and Shipping3/17.01s/exit0.48artifact/12baseline-codec/flag+negative guards, repo/LFS/9PSAST/77source+3character+84UI hashes pass. No screenshot-equality-only approval, masks/threshold changes or automatic commit. Remaining M7/clean revision/remote/trusted-player/owner budgets/release gates OPEN; HANDOFF.md owns next.
- Retain Python/Remote Control only if a genuine packaged black-box requirement remains.

#### M7.2 Test launcher repair or retirement

- Completed during M1 recovery: the broken Remote Control `run_e2e.py` harness and its helper/tests were retired in favor of native automation, future Functional Tests, and UAT/package smoke coverage. See `scripts/README.md`.

#### M7.3 CI pipeline

C018H LOCAL CLEAN-CANDIDATE PREFLIGHT VERIFIED (2026-10-16): owner authorized
local commits without push/remote configuration. f79f0b8 commits503 reviewed paths,
assets and87 LFS visual baselines. First genuinely clean Unity build caught helper
overload/redefinition collisions hidden by dirty adaptive builds; scoped rename/
test namespaces fixed in91c54c2. Same independent clone clean/LFS176; Editor7,
Rules27 clean/all16 impacted anchors, full139Success/138clean/1intentional missing-file
warning/all21 anchors, strictIWYU112, Generate and BOTH Development/Shipping
packages PASS. Each cook594+7platform/601total/0errors/0warnings. Native target
receipts exclude test module; actual final Shipping registry601rows retains59
presentation/input assets+configured launch map and excludes test paths/module.
Root Editor11 also passes. [Full evidence/manifests](M7_CLEAN_CHECKOUT_REVIEW.md).
No player launch/security mitigation/remote-CI certificate. Fresh readonly remote
check19:36:56UTC: main89d8d66e/protected=false/runners0/workflows0; no push/settings
changed. Remote runner/governance/player/security/rights/budgets and M7 remain OPEN.

C018H goal BLOCKED, not complete (2026-10-09 20:26:58UTC): independent local QA packages under M7.5 were verified after the historical13:59 block, resetting that audit count. Three NEW consecutive post-package goal turns at20:24:37/20:25:49/20:26:58 then confirmed the same external environment/authority boundary: actual mainSHA89d8d66e/unprotected/runners0/workflows0, HEAD0726de2/empty index/no UE jobs, packaged Development+Shipping NotSigned/no owner response. Goal tool returned blocked; objective intact. Artifacts/C018H-PostPackageBlockedAudit{1,2,3}.json and HANDOFF.md preserve evidence. No safe substitute for isolated CI/trusted-player/clean candidate/owner decisions; no configuration/push/security changes authorized. Explicit user resumption starts a fresh blocked audit count, not these historical3.

C018H external verification (2026-10-08 local) confirms OPEN gates: [actual read-only GitHub/Git evidence](M7_EXTERNAL_GATES_REVIEW.md). main protected=false/no rulesets; repository runners0/workflows0/environments0/trusted-ready variable absent. Local HEAD34 commits ahead of remote/current implementation uncommitted. No divergence/fetch/merge/staging/push/settings/auth changes. Owner must identify isolated runner/trusted QA and authorize clean candidate/configuration scope; admin capability is not write authorization. Local gates remain approved, no redundant reruns.

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
  - C018H explicit PE dependency metadata ADVANCED, rights/security OPEN (2026-10-12): PeImportReview1 parses all38manifest-bound native QA images (23Dev/15Ship), main normal/delay descriptors47/15and41/13, same-basename packaged edges13/10. Vorbis/VorbisFile explicitly referenceOgg both, despite ShipGame no directOgg/PDBpositive; cannot drop notices. No tbb12/MsQuic descriptors in this scope does not exclude dynamic loads or mitigateQUIC. Names/descriptor presence not runtime path/version/ABI/providers/forwarding/rights proof. Initial negativefixture expectation1 before native input/receipt corrected, six malformed plus validnormal/delay/named/ordinal fixtures pass/script0/hashbeforeafter. No PDBreparse/native/install/execute/disassembly/binary/license/QA/remote/trust change. NEXT apply completed ATR evidence to genuinely missing owner/vendor/clean-candidate/CI/trusted-player gates, no repeated scans/parse/builds. [Evidence](M7_PACKAGED_DEPENDENCY_REVIEW.md).
  - C018H PDB/PE metadata ADVANCED, rights/final attribution OPEN (2026-10-12): PdbContributionReview1 binds nativeEXE–QA hashes, RSDS/PDBGUID+age/DBIage, exact current section-header bytes and noOMAP. Positive bounded contribution intervals associate54/61Dev and45/54Ship library paths (suffix names, not archive-byte identity/full objects/notice scope); module counts3581/3494, global records2756003/1065743. SSL/crypto/curl .text footprint exists both, not specific reachable CVEs/mitigation. Missing associations do not imply exclusion, liboggShip namedmodule/0positive andtbbmallocShip .rdata-only scoped. Initial64MiB reader rejected78.6MBDevDBI before receipt; corrected128MiB bounded/aggregate metadata reader0, synthetic MSF/module/PE bounds assertions0/no originalpath/symbol/source persistence/disassembly/execute/native or binary mutation. NEXT scoped PE import/delay metadata and existing companion association gaps, reuse ATR worksheet/PDBproof. No security/right/owner/finalbundle approval. [Evidence](M7_PACKAGED_DEPENDENCY_REVIEW.md).
  - C018H owner-action consolidation PREPARED, attribution OPEN (2026-10-12): [ATR-00–14 worksheet](RELEASE_ATTRIBUTION_ACTIONS.md) links concrete security/provenance/retained-object/contract/notice tasks without repeating source review. AttributionActionCoverage1 validates28hash-bound receipts and exact JSON identity pointers; disjoint11library groups cover61Dev/54Ship, separate DLL scope21/13in11families. RetainedMetadataAvailability1 identifies existing383MB/237MB PDBs/MSF7 and hash-intact .rsp /DEBUG:FULL/no MAP or VERBOSE flags; scoped paths only/no global tool/map absence. PDB–EXE GUID/age and contribution proof still unverified. Platform names/fonts/providers/content/transitives explicitly additional. No owner approval, final bundle or M7 gate claimed, no fetch/native/engine/library/license/QA/trust/remote mutation. NEXT bounded documented PDB/PE identity/contribution analysis from existing metadata, no rebuild/install/source scan; unresolved exact contracts/clean-candidate/trusted-player/security gates require owner/environment evidence.
  - C018H declaration identity reconciliation COMPLETE, attribution/rights OPEN (2026-10-12): DeclarationReviewCoverage1 baseline49/61Dev and46/54Ship exact references in selected review bodies. CompanionImportBinding1 adds9Dev/8Ship bounded AMD64 short-import associations to existing DLL-family receipts and actual PublicLibraries owners; other archive members unclassified/no DLL ABI/retention proof. DevTailSourceReview1 adds3Development-only identities: Detours4.0.1/header changes/MIT-body candidates vs distinct5.3 notice label; DirectShow Windows7/.NET4 SDK/strictCP1252 diagnostic candidate and conditional REDIST/marked-sample/Media-credit scope; Molecular Matters2011–2020 LC_JumpToSelf not licensed by neighboring xxHash TPS. Final61/61Dev,54/54Ship identity references,33platform-name inputs each separately excluded. No full source/retention/owner/final-notice or release approval, no engine/library/license/QA/native/trust mutation.114Python/native evidence reused, QUIC+OpenSSL+curl security HOLD unchanged. NEXT consolidate exact outstanding obligations from receipts, no repeated declaration/source scans; distinct patched engine candidate remains unavailable. [Review](M7_PACKAGED_DEPENDENCY_REVIEW.md).
  - C018H GPU/DirectX declaration scope ADVANCED, rights OPEN (2026-10-12): GpuDeclarationReview1/Supplement1 binds12unchanged libraries BOTH exports. Four unique GPU/ITT declarers; DirectX8 directory associations map instead to actual DX11/DX12/DirectSound/DX11Audio/XInput, not DirectX-as-owner. AGS6.0.1 and NVAPI pinned R570commit ce6d2a18 library raw hashes match publisher; headersCRLF-only/MIT candidates, NVAPI publisher explicitly names licensed libs and scopedSPDXbodyCRLF matches localwrapper. Prior legacyAftermath/Epic scopes not relicensed. Intel extension2025MIT header/fullnotice+build unresolved; ITTAPI3.0/GPL2 ORBSD3 local/fullcommit5b8a7d7 counterparts mismatch, branch notautomaticgameGPL. June2010DXSDK EOL/None/customheaderMIT metadata not8library redistribution grants. No rawSDKpersist/install/engine/library/QA/notice mutation,114Python/native proof reused/security HOLD unchanged. NEXT reconcile61Dev/54Ship file-backed declaration coverage using receipts, then genuine remainingDev candidates, no repeated queries. [Review](M7_PACKAGED_DEPENDENCY_REVIEW.md).
  - C018H HTTP/image/HDR candidate attribution ADVANCED, rights OPEN (2026-10-12): ImageHttpSourceReview1/Supplement1 binds10unchanged libraries/unique declarants BOTH exports. nghttp2 1.64 MIT whole legacy noticeCRLF/bodyversionwhite; Imath3.2.2/OpenEXR3.4.3 notice bodiesCRLF with explicit wrapper exclusion; separate OpenJPH0.24.5/versionheaderBSD2, not OpenEXR BSD3. JPEG Turbo3.0 headerCRLF but older central copyright2020vsprimary2023/fullnotice mismatch, strictCP1252 candidate/raw unchanged; IJG/BSD3/zlib component obligations, IJG credit/TurboJPEG notice not OR-choice or old IJG6b1998 substitute. APVv0.2.0.1 first noticeCRLF, WHOLEheader/wrappedlicense mismatch; malformed TPS/quotes preserved. No retention/exact-build/patent/owner/finalbundle approval or native/engine/QA/license mutation;114Python/native proof reused/security HOLD unchanged. NEXT GPU/DirectX remaining declaration scope, reuse dynamic-family receipts. [Review](M7_PACKAGED_DEPENDENCY_REVIEW.md).
  - C018H Opus/Speex/VPX/WebM source-notice candidates ADVANCED, rights OPEN (2026-10-12): AudioVideoSourceReview1/Supplement1 binds5unchanged libraries/unique declarants BOTH exports. Opus1.3.1-12 path but archive banner libopus unknown; header v1.3.1 CRLF-only/root BSD3 noticewhite match, separate APIheaderBSD2-style and Speex2007BSD3 notices preserved, actualSpeex version/wholeheader not matched. VPX1.14.1 banner/headers; WebM1.0.0.27 local license/parserheader CRLF-only primary match. Old central TPS filenames absent/metadata scopes preserved, not licensing violation; newerOpus1.4/1.5 not substituted. WebM PATENTS URL404 resolved via actual PATENTS.TXT200/same VPX grant; conditional claim scope/patent-litigation termination and Opus IETF applicability remain owner-reviewed. No native/engine/QA/license/code mutation,114Python/native proof reused/security HOLD unchanged. NEXT nghttp2/LibJpegTurbo/UEOpenAPV/Imath/UEOpenExr and remaining GPU/DirectX declarations, no repeated dynamic/source queries. [Review](M7_PACKAGED_DEPENDENCY_REVIEW.md).
  - C018H BLAKE3/MikkTSpace/LibTiff source-notice mapping ADVANCED, rights OPEN (2026-10-12): HashTangentTiffSourceReview1/Diagnostics1 binds3unchanged libraries/unique declarants BOTH exports. BLAKE3 1.3.1 header CRLF-only and full CC0-1.0 OR Apache-2.0 notice whitespace-only primary match. Mikk1.0 local copyright matches primary, whole header differs by m_bIgnoreDegenerates+comment; central notice whitespace-matches local copyright/author comments, not binary/source revision proof. Source-origin/modification/notice restrictions, acknowledgement not required. TIFF4.2.0 header/version/COPYRIGHT CRLF-only match; preserve notice/name restrictions, static zlib/turbojpeg TPS statement not actual transitive retention proof. Original unresolved/ambiguous TPS statuses preserved; ComputingTangentSpace metadata=None not a grant. No native/engine/QA/license/code mutation,114Python/native proof reused. Security HOLD unchanged. NEXT Opus/media/vendor candidates or genuinely distinct patched engine, no repeated queries. [Review](M7_PACKAGED_DEPENDENCY_REVIEW.md).
  - C018H owner engine identity CONFIRMED, mitigation OPEN (2026-10-12): owner answered VERSION+PATH with CURRENT UE5.8.2/CL56702186 at C:\Program Files\Epic Games\UE_5.8. MsQuicOwnerBaselineConfirmation1 binds fresh Build.version and DLL version/hash checks;2.2.0.359654-official is byte-identical to the previously reviewed installed payload, not a new patched candidate. This supersedes earlier pending-answer wording, not the security HOLD. Integration files reported available do not prove a complete engine-source repository. Retain QUIC; require distinct vendor patch/integration evidence including OpenSSL/curl before compatibility/mitigation clearance. No install, migration, DLL substitution, repeated audit/GET/native/player gates. NEXT independent BLAKE3/MikkTSpace/LibTiff attribution; evaluate a genuinely distinct vendor candidate when available. [Review](M7_PACKAGED_DEPENDENCY_REVIEW.md).
  - C018H ICU/HarfBuzz/libWebSockets candidate mapping ADVANCED, rights OPEN (2026-10-12): TextWebSocketSourceReview1/Diagnostics1 bind3unchanged libraries/declarants in BOTH exports; ICU64.1/HarfBuzz2.4.0 local same-basename license candidates resolve old depot-style TPS references without UNC access/repair. StrictCP1252 diagnostics preserve raw notices; ICU whitespace-only primary match, HarfBuzz local copyright Google2019/Facebook differs from primaryCOPYING2012, body whitespace-only match. UCDN separate notice/header-template comparisons scoped. WSheader3.0.0 vsTPS1.7.3;205byte credit/source locator not license/source fulfillment. Pinned LGPL2.1+static-link exceptions do not automatically require game source, but modified-library obligations/retention/exact rights unresolved. No product/native/engine/archive/license mutation;114Python/native proof reused, security HOLD unchanged. NEXT remaining BLAKE3/MikkTSpace/LibTiff/Opus/media/vendor cohorts or exact owner Unreal candidate. [Review](M7_PACKAGED_DEPENDENCY_REVIEW.md).
  - C018H network-source evidence BOUND; security/attribution HOLD (2026-10-12): NetworkSourceReview1 binds3unchanged OpenSSL/libcurl libraries/banners and unique declaring modules in BOTH native exports. OpenSSL1.1.1t header/license matches primary only CRLF→LF; legacy OpenSSL AND SSLeay conditions, not Apache replacement. curl8.15.0 released header differs from tag DEV template in2generated macros; old notice copyright2020vs2025/email differs, body whitespace-only match. Publisher standard support excludes OpenSSL1.1.1; current curl8.15JSON43advisories29Low/14Medium are VERSION candidates, not43proven game CVEs. Review exact Epic patches/features/backend/retained code/trusted runtime; independent of MsQuic Critical HOLD, no mitigation implemented. Retain QUIC/evaluate-compatible ONLY, missing owner candidate engine VERSION+PATH; include these dependencies in evaluation. NEXT ICU/HarfBuzz/libWebSockets candidates or exact vendor candidate, no repeated GETs/native gates. [Review](M7_PACKAGED_DEPENDENCY_REVIEW.md).
  - C018H compression/font-library source candidates BOUND, binary attribution OPEN (2026-10-12): CompressionFontCoreReview1 binds3unchanged libraries in BOTH indices; zlib1.3/libPNG1.6.44/FreeType2.14.1 selected headers match primary version-tag files after ONLY CRLF-to-LF. libPNG source LICENSE/FreeType FTL match likewise; central libPNG notice has two2025vs2024 copyright token differences, not repaired. FreeType root LICENSE supplies FTL/GPLv2-or-later choice and package-wide alternate notices, not all retained. Existing111byte FreeType-only packaged credit manifest/hash unchanged. No final bundle/rights/retention/static-import/security approval;114Python/native proof reused, no product/engine/archive changes. NEXT OpenSSL1.1.1t/libcurl8.15.0 exact source/build/support candidates independent of QUIC. [Review](M7_PACKAGED_DEPENDENCY_REVIEW.md).
  - C018H exact source-gap declarations RESOLVED; attribution OPEN (2026-10-12): StaticGapSourceReview1 binds7unchanged library hashes, unique PublicLibraries declaring modules in BOTH native exports and selected source notices. libpas22UE-header BSD-style notices preserve Epic2023/2023-2024/2025 years; original modified WebKit fork notice coverage still unverified. RadAudio/Bink/Oodle/UEJpeg headers are not standalone distribution grants; Draco1.5.6/Apache header is a source candidate. UEJpeg bundled ZIP includes stb_image implementation declaration with MIT2017SeanBarrett/Unlicense alternatives and Oodle9/13 versus native SDK9/16; no owner choice/retention/build proof inferred. DevProfilerSourceReview1 binds3additional Development-only declaring modules/hashes; Null filenames/no matching notice filenames do not prove exemption. NEXT applicable original/transitive notices and retained-object/vendor build evidence from existing indices, superseding the seven-gap NEXT below. No tool/runtime/native/engine/package/license/security mutation;114Python/native proof reused. Retain QUIC/evaluate-only/Critical HOLD/owner candidate engine path pending/M7 OPEN.
  - C018H offline declared-link review index IMPLEMENTED (2026-10-12): [dependency review](M7_PACKAGED_DEPENDENCY_REVIEW.md). NEW Tools/link_attribution_index.py/14synthetic tests joins existing94/87link inputs and TPS/module references without engine scan/network/native build.61/54file-backed .lib +33platform imports each; static/import type/retention still unverified. Shipping45candidateTPS/3no selectedTPS/4unassociated/2ambiguous; invalid/duplicate/unsafe/reparse/hash/origin/redirect/expansion/input-change guards, create-only Artifacts output. LinkIndexReview1 binds exact Dev/Ship indices;14directed pass/fullPython114total113pass1existing capability skip, actual0 logs. No legal/native/engine/config/Content/archive change. NEXT exact libpas/RadAudio/Bink/OodleNetwork and OodleData/UEJpeg/Draco candidate gaps; WinPix/Vorbis directory limitations explained, no duplicate DLL work. Owner engine/source path question pending, RETAIN QUIC/evaluate-only/Critical HOLD/M7 OPEN.
  - C018H dynamic native-family candidate review COVERED; full attribution OPEN (2026-10-12): [dependency review](M7_PACKAGED_DEPENDENCY_REVIEW.md). NativeFamilyCoverage1 binds21Dev/13Ship DLL rows/11families to graded receipt hashes, not full static/content/provider/font/security/rights coverage. DependencyTailReview1 rechecks nine DLLs/seven paired Vulkan manifests. WinPix pinned1.0.240308001 production x64 matches outside narrow PE fields and declares MIT; broader18-program notices not all retained/reviewed. DbgHelp25291 vs TPS19041 and Vulkan manifest/header1.4.341 vs older validation license1.3.250.1 retain exact source/contract/component-notice gaps.87/94declared link inputs still not retained-code proof. Native/engine/config/Content/archive unchanged, prior100Python superseded by114proof above for new index tool; native gates reused. Targeted declared-link index advanced above; asked owner for other candidate engine/source tree version/path, no authority/answer inferred. Critical HOLD/retain QUIC/evaluate-only unchanged.
  - C018H Aftermath2025.5 document roles REVIEWED (2026-10-12): [dependency review](M7_PACKAGED_DEPENDENCY_REVIEW.md). Installed/BOTH QA DLL hashes exact; reported2.26/2025.5/API0x21A/TPS251114 corroborate family, not authenticated vendor build. Version-named266byte .license is wrapper only. Legacy NVAPI candidate preserved; current primary SDK agreement/dll-so supplement and2025.5 release notes hashed, no exact-package contract binding/accepted rights/current license substitution. Retained components/usage guidelines/privacy/export/pass-through terms/final notices remain owner gates. AftermathSourceReview1 receipt; no SDK download/install/execute/engine/archive/tool/config/Content change, unchanged100Python/native proof reused. Historical NEXT DbgHelp/Dev WinPix/Vulkan advanced above; preserve RETAIN QUIC/evaluate-only and Critical security HOLD.
  - C018H Ogg/Vorbis exact staging/candidate BSD terms REVIEWED (2026-10-12): [dependency review](M7_PACKAGED_DEPENDENCY_REVIEW.md). All3 installed/BOTH QA hashes rechecked, native Windows import/delayload/staging rules inspected. Pinned Ogg1.2.2/Vorbis1.3.2 selected headers differ only expanded $Id comments after splitlines, NOT raw/CRLF-only equality/authenticated DLL source/patch proof. BSD bodies excluding copyright line map installed candidate texts; separate older Xiph/aoTuV copyright years preserved, exact aoTuV retained code/version/modifications/rights/final notices/security OPEN. OggVorbisSourceReview1/HeaderComparison1 bind evidence; initial UTF-16-as-UTF-8 query failed before GET, corrected existing BOM reader0. No tools/source/native/engine/archive change,100Python/native proof reused. Historical NEXT Aftermath advanced above; preserve owner RETAIN QUIC/evaluate-only and Critical security HOLD.
  - C018H oneTBB candidate terms REVIEWED; official binary provenance OPEN (2026-10-12): [dependency review](M7_PACKAGED_DEPENDENCY_REVIEW.md). Exact installed/BOTH QA DLLs rehashed; official2022.3.0 full ZIP checksum verified. Production x64 DLLs differ BOTH raw and strict PE-normalized; no expanded masks/corruption finding/authenticated upstream origin. Complete same-package Apache body maps installed wrapper after whitespace; six-section third-party notice read, not all retained by two DLLs. OneTbbUpstream1/AttributionReview1 bind facts; initial overbroad ia32 probe terminal1 preserved, corrected query0. Epic build/source/flags/modifications/applicable notices/owner rights remain OPEN. No engine/tool/config/Content/archive mutation, unchanged100Python/native gates reused. Historical NEXT Ogg/Vorbis advanced above; retain QUIC/evaluate-only and Critical security HOLD.
  - C018H ONNX runtime candidate/header/document roles REVIEWED (2026-10-12): [dependency review](M7_PACKAGED_DEPENDENCY_REVIEW.md). Installed/BOTH QA DLL hashes rechecked. Reported version suffix resolves pinned public commit3a728b7506...; one C API header matches after ONLY CRLF/LF normalization, not raw identity/authenticated DLL provenance or final1.24.1/1.24.3 release proof. Installed runtime-named license actually contains ONNX Apache notice, not primary ORT MIT; pinned MIT and ONNX notice quote differences preserved separately. Whole325054byte upstream notice hash recorded, ONLY ONNX section examined, not complete retained-provider/license coverage. OnnxAttributionReview1/SourceReview1/TextComparison1/CommitLookup1 receipts bind facts; no license repair/raw SDK install/persistence/exec/engine/header/Content/config/archive changes, prior100Python/native proof reused for unchanged source. Final notices/applicable components/owner rights OPEN. Historical NEXT oneTBB now advanced by supplement above; preserve MsQuic critical security HOLD and RETAIN QUIC/evaluate-only decision.
  - C018H MsQuic OpenSSL2.2.0+359654 primary payload/terms VERIFIED; release security HOLD (2026-10-12): [dependency review](M7_PACKAGED_DEPENDENCY_REVIEW.md), [security scope](M7_SECURITY_DEPENDENCY_REVIEW.md). Installed/Dev/Ship hashes match source, pinned upstream matches outside narrow PE fields; MIT body plus OpenSSL Apache2.0 notices mapped, no raw identity/signature approval. Updated publisher list has6 advisories including Critical RCE and OpenSSL validation; initial4 were only a subset. OWNER CHOSE RETAIN QUIC/evaluate compatible update: do not disable plugin. MsQuicUpdateEvaluation1 binds current decision/sources/header comparison; supported candidates2.6.2(primary)/2.5.11 retain parsed31-field API2 prefix, NOT ABI/runtime verification. Epic5.8.3 exists but actual MsQuic patch unknown, no upgrade cure assumed. Concrete install/source-engine migration still needs authority; no config/tool/engine/DLL/build/archive mutation, prior100Python reused. Required mitigation: supported exact integration, new header/import-lib/DLL/TLS/native Dev+Ship regressions/staging/provenance and reviewed rights/notices. Security/text scan0 does not clear known upstream finding. NEXT exact vendor compatibility evidence or independent ONNX/oneTBB attribution, no repeat SDK downloads/blind substitution.
  - C018H DirectML1.15.4 primary selected payload/notice mapping VERIFIED (2026-10-12): [dependency review](M7_PACKAGED_DEPENDENCY_REVIEW.md). ETag-pinned HTTP ranges inspect only Windows x64 DLL and LICENSE/README/ThirdPartyNotices, not a full202MB SDK download or package-signature proof. Installed/Dev/Ship hashes rechecked; all bytes outside narrow PE checksum/security-directory/terminal cert fields match, raw bytes differ. README explicitly separates binary terms from MIT headers/build; notices identify GSL/half/Zstandard. Installed license quote differences fail strict whitespace-only equality, preserved without rewriting/acceptance. New bounded range reader12 offline tests pass/fullPython100total99pass1existing capability skip. DirectMLUpstream1/LicenseComparison1 receipts bind ranges/hashes/inputs; no raw SDK install/execute/persistence, engine/Content/native/player/Blender change. Owner terms/platform/conditional data/export/final notices and independent M7 release gates remain OPEN. NEXT MsQuic/other exact unresolved dependencies; reuse current proof on client switch.
  - C018H local packaged inventory VERIFIED (2026-10-12 UTC): [dependency attribution review](M7_PACKAGED_DEPENDENCY_REVIEW.md). Native JsonExport Development/Shipping exit0 without codegen/build/cook/player; bounded stdlib inventory verifies all48/30 existing archive files and main EXE identity, records21/13 DLLs/version resources,184/179 descriptors,497/455 modules,94/87 declared link inputs and240/235 selected TPS sources. XML/redirect/path/hash/reparse guards and create-missing-only receipts;21 directed tests pass, fullPython58/57pass/1existing symlink-capability skip. Engine non-XML/malformed TPS89 each, ambiguous/missing/version references and10/3 unmatched DLL declarations remain explicit; ten installed license candidates hashed, not approved/copied.77accepted assets unchanged. THIRD_PARTY_NOTICES now distinguishes repository-vendored inputs from packaged engine dependencies. LOCAL engineering inventory only: exact applicability/retained dependencies/final notice bundle/Epic+SDK distribution rights and clean-candidate approval OPEN; no remote, trust, commit, engine or runtime change. Older C018H external-only snapshots are historical; this new inventory was meaningful independent local progress, not M7 completion.
- Run secret scanning and dependency/plugin review.
  - C018H XAudio2 primary candidate payload/document mapping VERIFIED (2026-10-12): [dependency review](M7_PACKAGED_DEPENDENCY_REVIEW.md). PE version1.0.0.1 did not identify package; bounded primary probes stopped at retail x64 Microsoft.XAudio2.Redist1.1.0, identical outside ONLY checksum/security-directory/terminal certificate table. Full bytes differ. XAudioSourceReview1.json binds prior Dev/Ship source/staging hashes, package/LICENSE/README and unchanged19-test comparison helper. Exact document section2a Image Library wording leaves distributable-code scope/owner compliance OPEN; no blanket permission or unique-version claim. Initial bounded-query failures retained, actual sizes measured before bounded cap adjustment. No new tool/code/native/asset/engine/runtime changes, raw SDK persistence/install/exec/Blender addition or unjustified test rerun; final notices/player/CI/release remain separate gates.
  - C018H Agility1.618.5 exact primary binding VERIFIED (2026-10-12): [dependency review](M7_PACKAGED_DEPENDENCY_REVIEW.md). Official pinned NuGet inspected in bounded memory only; actual file list includes both packaged D3D12 DLLs, complete binary-license text maps to installed version candidate after whitespace normalization. Full DLL bytes differ; all bytes except the explicitly bounded PE checksum/security-directory/EOF certificate table are identical. New stdlib SDK provenance tool/19 directed tests/fullPython88total87pass1existing symlink-capability skip; final AgilityUpstream3.json binds package/docs/mappings/tool/comparison hashes. Partial1 and failed request2 retained; no live job restarted or raw SDK persisted/executed. Exact file-list/license-body/payload binding resolved, NOT Authenticode verification or unconditional redistribution; owner contract compliance/final notices/release OPEN. No native build/cook/player/engine/config/QA archive change.
  - C018H installed font-directory TPS review VERIFIED (2026-10-12): [font review](M7_FONT_ATTRIBUTION_REVIEW.md). Three exact Content/Slate/Fonts TPS sources, previously outside module inventory scope, inspected through existing bounded reader. Roboto/Noto point to exact installed Apache2/OFL1.1 documents; Droid redirect target is absent. FontTpsSources1.json hashes declarations/documents and final font receipt inputs. Roboto2011/2017 bytes,2016 TPS and2012 notice header retained separately; legacy encoding flagged, per-face applicability/final notice bundle/owner compliance OPEN. RobotoPrimaryLookups1.json records16 exact-blob404 results across two primary repositories, not global source absence or unlicensed content. No reader/runtime/font/engine/config/archive mutation, copying license text, build/test rerun or release approval.
  - C018H exact DroidMono source/declaration mapping VERIFIED (2026-10-12): [font review](M7_FONT_ATTRIBUTION_REVIEW.md). Public AOSP mirror blob/pinned original commit54b6cfa9 byte-matches actual78,296-byte font/SHA256793a6342...; same pinned directory README/NOTICE/MODULE_LICENSE_APACHE2 explicitly declares Apache2. Candidate documentary provenance is now exact, not a modern family-name guess. Receipt DroidMonoUpstream1.json records hashes/URLs, no binary/license text persisted or source/metadata changed. Embedded Ascender text retained, owner compliance/final notices remain OPEN; older Roboto directory/blob checks did not prove exact upstream mapping. No build/test/player/remote mutation/install/repeated broad audit.
  - C018H delivered-font inventory LOCAL VERIFIED (2026-10-12): [font attribution review](M7_FONT_ATTRIBUTION_REVIEW.md). Full native raw-font index check found15entries, not only initial14Slate/Fonts: Old/Tiles Bold is a duplicate (14distinct hashes). Fresh expanded native extraction15per config/exit0/archives unchanged, exact index path-set equality/installed-source hashes/byte-identical Dev+Ship sets. Bounded stdlib SFNT/name parser,11directed tests pass/fullPython69total68pass1existing symlink-capability skip. Mixed Roboto2011/2017, Droid/Noto and six explicit license-name records/nine absent/0encoding warnings documented, not legal approval. DroidMono Ascender metadata and exact older-font agreements/notice text remain review flags; no broad modern family-license substitution, replacement or engine/Content/config/runtime change. Official1.618.5 NuGet scope separates binary license from headers; primary docs recorded without endpoint bypass/download. This extends actual delivered-asset evidence, not unnamed IO/FontFace/all-license completeness, M7 completion or a release notice bundle.
  - C018H supplementary source resolution (2026-10-12):14 archive DLL rows match exact installed engine binaries and inspected native staging rules in SourceMappings2.json. XAudio2 redist, AgilitySDK1.618.5 Core/SDKLayers, WinPix and seven Development-only Vulkan layers are explained. Native module export omits RuntimeDependencies when Binary==null, and UAT stages Vulkan layers only for non-program Debug/Development; original10/3 export-gap counts preserved. Exact-version AgilitySDK license candidate found separately, NOT rights/file-list/distribution approval. No engine/rules/config/package/runtime change; [evidence](M7_PACKAGED_DEPENDENCY_REVIEW.md).
  - C018G LOCAL VERIFIED: [candidate security/dependency review](M7_SECURITY_DEPENDENCY_REVIEW.md). Standard-library read-only scanner covers current Git candidate text (not history/binaries/ignored files), redacts all values and fails on matches/unreadable/unsafe inputs. Exact11-plugin/Editor-only MCP/module/full-SHA action policy enforced; existing trusted-only workflow includes gate, no activation. Targeted14tests13pass/1actual symlink capability skip, independent synthetic link/reparse guards pass; fullPython37tests36pass/1same skip, local334text scan no findings/policy failures, existing YAML parse pass. Dirty worktree evidence does not close clean-candidate/remote promotion/license/release gates. No installs, external writes, credential rotation/deletion or Windows trust changes.
- Produce both Development QA and Shipping packages; verify test/Remote Control code is absent or disabled in Shipping.
  - CURRENT checkpoint (2026-10-15 21:48 local / Oct16 00:48UTC): offline POSTBOOT CIM license VERIFIED, complete sentinels/output Windows EnterpriseEval/LicenseStatus1/128112min, no keys/activation/rearm/clock/network/contract changes. Counted/throttled COM text helper proven, input observer42600 terminal0; same backend33044/3DOFF/isolated, console idle. HostSuspend/HostResume log + Windows42/107 explain at least one pause, not all stalls/3D causation; do not change host security/power policies. Next normal guest shutdown/restore3Don and qualify accelerated driver/UI with host awake before existingarchives -dx11; all actual player QA/fullM7 remain OPEN. [Compact current handoff](HANDOFF.md)/VM_3DOff_LicenseVerification1.json bind evidence and scripts; older postboot-license-pending/recovery-decision bullets below are historical.
  - CURRENT authorized diagnostic recovery (2026-10-15 21:09 local / Oct16 00:09UTC): VM-only forced poweroff0, temporary3DOFF0, single GUI boot0 completed; new backend33044/creation00:03:36.1861210UTC, isolated. Initial modify hit session-unlocking error and stopped before start; continuation after verified off/old backend absent, no repeated off/start. Desktop/WinR and DxDiag Display via installed COMmouse observed; VirtualBox driver7.2.20.25154/WDDM1.3/DDI12/VRAM0/reported featurelevels in3DOFF do NOT prove hardware/player readiness. Another8.85s heartbeat pause, no root cause or stable fix claimed. License postboot/player QA remain pending, no Unreal launched; restore/qualify appropriate accelerated profile in an agreed future boot window before game QA. [Handoff](HANDOFF.md)/VM_3DOffRecovery1.json/logs/captures preserve exact state; no hostsecurity/WHP/VBS/network changes/fullM7 closure.
  - CURRENT hang evidence (2026-10-15 23:53UTC): owner reports hang; same isolated VM/backend32772 running/3Don/RunLevel3, log now shows recovered heartbeat after652s and catch-up lag672s. Partial input only, no CIM/dxdiag/player results; no root cause or terminal process inferred. Proposed owner-authorized recovery: forced poweroff ONLY VM and one temporary3DOFF diagnostic boot, NOT yet approved/executed; possible unsaved-state loss, no host security/WHP/VBS changes. UI-only unaccelerated test cannot replace original Unreal/DX11/DX12 qualification. [Handoff](HANDOFF.md)/VM_OwnerHang1.json preserve evidence; fullM7 OPEN, old mouse/keyboard questions superseded.
  - CURRENT input qualification (2026-10-15 23:47UTC): owner confirms Start opens with mouse and sees console; native capture23:42 now shows PowerShell prompt. Readonly CIM payload sent ONCE (guarded helper/log, observer87407 terminal0 at23:45:09), but no guest sentinel/result verified; not license/driver proof. Target-window client capture23:46 also shows idle prompt; RestoreTrue/ForegroundFalse, no proven focus acquisition. Owner asked to type echo QA manually; stop blind retries. [Handoff](HANDOFF.md)/VM_OwnerUI_InputAttempt_20261015_2347.json preserve evidence; no reset/3D/network/security/player changes, all real M7 gates remain OPEN. Older mouse-pending/never-executed helper notes historical.
  - CURRENT postboot qualification (2026-10-15 23:30:06UTC): desktop remains visible but five captures identical after native WinR/CtrlAltDel input0; no guest console/result proven. Same backend alive/CPU increasing, AdditionsRunLevel3/facilities50 are not UI/API/root-cause proof. Owner asked to test Start with mouse; stop blind input/retries. CIM helper never executed and now DISABLED. [Handoff](HANDOFF.md)/VM_PostbootInput_20261015_2330.json preserve evidence; no reset/restart/graphics/network/security change, all dxdiag/license/player/fullM7 gates still pending.
  - VM desktop CONFIRMED (2026-10-15 23:26:40UTC): authorized single boot reached Windows desktop, capture VM_AuthorizedStart_20261015_2325_3.png; query0 running/RunLevel3/7.2.20r175154 and network isolated. No credentials/input/player; boot no longer pending. Next verify dxdiag/actual driver/featurelevels/license offline, then existing archives -dx11; fullM7 remains OPEN. Welcome/RunLevel1 below are earlier observations.
  - CURRENT authorized VM start (2026-10-15 23:25:51UTC / 20:25 local): owner explicitly requested power on; signed Oracle VBoxManage guarded same UUID/poweroff/isolation, started GUI ONCE0, post-query0 running since23:25:17.955UTC. Backend32772/creation23:25:08.7968170UTC verified via CIM/log; capture shows Windows Welcome. Network stays isolated, host protections unchanged, no credentials or player execution. [Handoff](HANDOFF.md) records actual operation/captures; older poweroff and restart-consent blocks below historical. Next verify desktop/driver/API/license postboot then existingarchives -dx11; all actual player QA/fullM7 remain OPEN.
  - CURRENT VM state (2026-10-15 03:10:37UTC / 00:10 local): owner explicitly requested forced poweroff of ONLY RomanEmpireQA. Signed Oracle VBoxManage command0 and post-query0 confirm UUID90bafe9d-68f9-4d02-8521-5f66a6a60b83 poweroff; no restart or pending worker. Isolation/VDI/DVD and host protections unchanged. VM_ForcedPoweroff1.json and [handoff](HANDOFF.md) record evidence; older running/waiting/recovery-authorization notes below are historical. Confirm owner restart intent before startvm; unsaved-state loss possible, disk integrity/driver/API/license postboot unverified. QA-01–10/fullM7 remain OPEN, M8 still requires fullM7; goal ACTIVE, not complete.
  - M7 Guest Additions reboot observation CLOSED, guest reboot STILL UNVERIFIED (UTC2026-10-15 00:23/localOct14): read-only watcher terminal0/PID28392gone after9samples/120s, final image still Reiniciando; same VM31452/creation remains live/isolated. More than10min since normal Finish restart is NOT terminal proof or reset authority. Owner recovery choice pending: continue waiting or explicitly authorize forced poweroff/restart of ONLY guest with possible unsaved-state loss; none issued. [Handoff](HANDOFF.md) footerwaiting refers to VM31452/reboot, not completed watcher. No driver/API/player/postreboot license gate cleared; goal ACTIVE/fullM7 OPEN, six-document diff and bound evidence in VM_GuestAdditionsClosing1.json.
  - M7 Guest Additions VERIFIED SOURCE / wizard COMPLETED / reboot UNVERIFIED (evidenceUTC2026-10-15/localOct14): local53,151,744-byte ISO SHA4c6ba898... EXACT official7.2.20 checksum row; guest installer7.2.20/Valid OracleAmericaInc/SHA12b0658d... guarded before guest-only execution. No host installer/mount/extraction or new reader; native tar listing1/Rockridge not corruption proof. SameVM/DVDslot changed to additionsUUID47612f43..., original WindowsISO retained. Normal wizard with Direct3D checked reached Completing, then Finish requested normal guest restart00:12:17UTC; no contracts/credentials/rootcert/manual security changes. Still «Reiniciando» with live VM31452; nativeVersion7.2.20r175154/RunLevel1/BaseDriver50/Graphics0 do NOT prove desktop or accelerated API. Read-only live watcher/handle/log and pending recovery decision in [handoff](HANDOFF.md), VM_GuestAdditionsClosing1.json. No forced reset/poweroff permitted without owner decision; do not relaunch on observation timeout or duplicate installer. NICnull/cableoff/othersnone and host protections intact; all player QA/defaultDX12/security/cleanCI/rights/performance/fullM7 OPEN. GoalACTIVE/progress then verified reboot wait; prior product suites reused, no game/engine/cook/archive/remote changes. Earlier uninstalled-additions/DVDWindows notes below historical.
  - M7 guest evaluation ACTIVATED / ISOLATION RESTORED (evidenceUTC2026-10-14): owner explicitly authorized temporary NAT/official activation. One65.19s NAT window23:44:27–23:45:32, guarded finally restored NIC1null+cableoff/NIC2–8none, no bridge/hostonly/forwarding/localhost reach. Windows automatically activated; fresh OFFLINE CIM23:51 screenshot VM_NATStatus1.png/sentinel confirms one EnterpriseEval record, LicenseStatus1 Licensed,129594grace minutes(~90days); raw Reason1074068484 not interpreted,1600/1601 evaluation date not useful. No keys/credentials/account/terms/rearm/clock/security/Activate method supplied by agent; verified Microsoft normal UAC only. Worker1 terminal by PIDabsence/transcript ending, native cleanup guards pass, worker exit not captured; no invented0. Prepared second activation helper NEVER EXECUTED and now fail-closed disabled. Current VM31452/creation/log/UUID in [handoff](HANDOFF.md), VM_ActivationClosing1.json. This closes only technical evaluation-activation prerequisite: signed offline additions/actual3D driver/API and all player QA still PENDING, defaultDX12/security/cleanCI/rights/performance gates OPEN. No second NAT/activation, game/engine/cook/archive/host-security/remote changes or repeated product suites; older licensing HOLD and permission questions below historical.
  - M7 Windows DESKTOP / activation+graphics HOLD (evidenceUTC2026-10-14): existing VM poweroff19:01 verified, signed Oracle/singleGUIstart0/query0 reached desktop; Winver26H2/26300.9457 matches media (legacy26100 watermark is not OS version). Read-only CIM reports EnterpriseEval LicenseStatus5/Notification, reason0xC004F009, grace0; 1600/1601 date is not evidence that90days elapsed. Only Microsoft Basic Display10.0.26100.1, no guest accelerated RHI proof. Two guest1074/wlms.exe/license-comment events observed, not time-correlated to latest host poweroffs; Microsoft documents hourly shutdown for unactivated/expired evaluation. Ask owner before TEMPORARY NAT/official evaluation activation; no network/activation/rearm/date/security changes, no account/terms/region selected by agent. Current PID36464/creation/log and terminal readonly sentinels in [handoff](HANDOFF.md)/VM_DesktopQualification1.json; preserve VM/media, do not reboot in loops. After legitimate activation, signed additions/actual3D driver+API qualification and existing Dev/Ship -dx11 journeys still required, baselineDX12 and fullM7/security/CI/rights/performance OPEN. US keyboard injection of '-' became LatAm apostrophe; bounded layout-safe transport fixed diagnostic input, not a security bypass. GoalACTIVE/PROGRESS, no game/engine/cook/package/remote changes or repeated product suites; older OOBE notes below historical.
  - M7 Windows OOBE REACHED (evidenceUTC2026-10-14 17:49): fresh resume found existing VM powered off since17:42 and dedicated VDI grown to17899MiB, not proof of completion. Valid Oracle tool/poweroff guards, single GUIstart/query0 reused same UUID/ISO/VDI without reset/reinstallation/boot keys; actual country/region OOBE captured in VM_ResumeScreen2.png. Current PID12900/creation/log in [handoff](HANDOFF.md), previous PID32756 historical; native observers terminal0, VM intentionally running for owner. Owner country/keyboard/terms/account next; no automatic acceptance/network/OOBE bypass/driver/install/game execution. Guest readiness/additions/GPU/API/all Dev+Ship QA remain unverified; existing archives -dx11 qualification next AFTER ready isolated guest, no baselineDX12 substitution. VM_OobeResume1.json binds new evidence; goal ACTIVE/progress, full M7/security/clean-CI/rights/performance still OPEN. Earlier language/blocked/poweroff notes below historical; do not repeat ISO verification/enable/cook or launch another instance.
  - M7 host restart VERIFIED / Windows Setup BOOTED, guest installation PENDING (evidenceUTC2026-10-14): hostBoot08:19:55UTC/WHP1/hypervisortrue, existing isolated VM GUIstart0/query0; actual Spanish Windows11 Setup language screen captured. CD timer recovery used firmware BootManager/CDROM and one boot key, no reset/boot-order/security change/terms or disk formatting. VM intentionally running for owner interaction (PID/log/creation in [handoff](HANDOFF.md)), not a completed guest or unattended install job; do not launch/reset/reboot/reverify media again. Owner language/keyboard/license decisions next, then guest installation/additions/actual GPU/RHI/player QA; full M7/security/CI/rights/performance gates OPEN. VM_PostRebootBoot1.json preserves visual/native evidence and protected command-line-observer diagnostic. Existing Python/native product gates reused, no build/cook/game executed.
  - M7 ISO CONTENT VERIFIED / ATTACHED, guest installation PENDING (2026-10-13 local): owner Spanish x64 EnterpriseEval8228114432bytes/SHA8f344beaa1c84646534e35f416b2086f6bef54e521c09a997dc326a330e4c584. Four exhaustive contiguous HTTPS206/ETag-pinned ranges SHA256match local=official across EVERY byte, final local full hash stable; native verifier exit0/614.05s, ISO_FullCoverage1.json/transcript. This is exact official CDN content comparison, not separately published checksum/ETag-as-SHA or licensing/runtime proof. Prior sequential cancellation and lookup timeout preserved. ISO attached to existing VM SATA1/device0/mediaUUID995cb764-5469-4450-912b-f9982418ca3c/storageattach0/query0, VM remains poweroff; ISO_Attachment1.json. No duplicate ISO or VM/disk recreation, host/guest install/boot/reboot/terms/security/game/engine/cook/archive change. Owner manual host restart/WHP postbootcheck remains next prerequisite, then interactive guest installation/conditions handled by owner. [Handoff](HANDOFF.md) owns current process/goal state; no live verifier or repeat download/attach. Full M7/security/CI/rights/performance gates OPEN.
  - Historical goal BLOCKED (2026-10-13 16:16UTC): three consecutive no-progress turns independently revalidated the same pending owner/external inputs after the real WHP prerequisite change; update_goal returned blocked. Host has not rebooted, existing VM is poweroff/DVDempty, no owner guest evaluation/license choice received. No live job or verified wait, no safe remaining action without pending authority/input. [Handoff](HANDOFF.md) preserves full scope, native/product evidence and next steps; M7 OPEN/M8 dependent, no gate waived or completion claimed. Resume after owner manual restart/guest ISO-license decision; dependency patches, clean CI, rights and reference-hardware gates also remain pending. Do not repeat feature enable or execute unsigned host packages.
  - C018H host VM prerequisite ENABLED (2026-10-13): [VM setup](VM_SETUP.md), Artifacts/VMSetup/WHP_Enable_1/EnableFeature1.json. Signed Microsoft DISM with normal UAC enabled ONLY HypervisorPlatform, InstallState2→1; original Process exit3010 captured, terminal PID42500. /NoRestart /LimitAccess, no /All, no automatic reboot or security protections disabled. Owner manual restart and post-boot verification pending; do not repeat enable. Existing VM remains poweroff/DVDempty, guest ISO/license/evaluation choice still pending. No game/engine/config/cook/archive/remote change or guest/player run; M7/defaultDX12/security/CI/rights/performance gates remain OPEN. Earlier WHP2 observations below are historical.
  - C018H VM shader DATA QUALIFIED (2026-10-13): [VM setup](VM_SETUP.md), ShaderQualification1.json. Both archives already contain SM5 global/project shader metadata/libraries and packaged BaseEngine D3D11+D3D12 SM5 targets; prior Pak1737/1730 and IO1210/2libraries/566ShaderCode rows per config reused.10container hashes match pinned archive manifests and31inputs stable/current; no build/cook/extract/player/config/VM/security mutation. Next technical action is qualify existing archives with documented -dx11 only on an approved licensed trusted guest before considering recook. Does not prove actual RHI/driver/features/journey/defaultDX12/render-baseline/performance or close M7. WHP2/VMpoweroff/no ISO-license/evaluation approval remains; collector scalar serialization corrected, receipt and native provenance preserved.
  - VM guest input (2026-10-12): owner has no known ISO/license available («no, creo que no»). Official Windows11 Enterprise90-day evaluation/registration/expiry rules identified in [VM setup](VM_SETUP.md), not downloaded/accepted/activated. Owner eligibility/terms/choice pending; WHP remains disabled at last check, no VM/player running. Bare VM provisioning is meaningful progress, not M7 exit-gate approval.
  - C018H local VM PROVISIONED, guest/QA PENDING (2026-10-12): owner «montemos la VM dale». [Setup](VM_SETUP.md): official pinned installer178192488bytes/hash exact/Valid Oracle; VirtualBox7.2.20 CLI+registry verified, minimal installation/no Extension Pack/USB/network drivers observed. RomanEmpireQA UUID90bafe9d-68f9-4d02-8521-5f66a6a60b83 registered poweroff,8GiB/4CPU/64GiB dynamic disk/EFI-TPM2-SecureBoot, NIC1–8 none/no sharedfolders/clipboard/dragdrop. No ISO/OS/game boot. WHP was disabled; Windows Features dialog opened for owner activation/manual reboot, guest ISO/license still required. VirtualBox advertises D3D11, not current DX12 profile: graphics/shader qualification or separately approved QA profile remains pending; no game/cook/QA archive changed. VM software/registration do not prove player/QA/M7 success; security/CI/clean/rights/performance gates intact. Initial download timeout and protected partial setup errors preserved, no recreated VM/disk.
  - C018H trusted-player preparation PROGRESS (2026-10-12): owner requests this PC or a local VM. [QA protocol](M7_TRUSTED_PLAYER_QA.md) reuses existing dirty archives; fresh manifest hashes/counts/revision match. Read-only host preflight: Windows Home25H2, virtual-machine platform active but no ready Windows guest identified; Sandbox unsupported on Home. VM software/Windows guest license+ISO or legitimate signing/trust route requires owner direction before setup. No package execution/transfer/build/cook/install/security change; all actual Development/Shipping journey rows PENDING, no performance/CI/security-HOLD/rights gate cleared. Goal ACTIVE; historical blocked audits are not current status.
  - C018H local QA packages LOCAL VERIFIED (2026-10-09): [native package review](M7_LOCAL_QA_PACKAGE_REVIEW.md). Sequential Development/Shipping BuildCookRun exit0/104.56s/36.42s using installed native -skipcook,77accepted inputs and unchanged registry/projectstore/config hashes. Fresh Windows archives48files/972,805,697bytes and30files/619,034,131bytes; dirty5.8.2/HEAD sidecars and SHA256 file manifests. Native Pak payload Verify1737/1730healthy, four IO index listings and1237named payload extractions per configuration pass; all79cooked project assets (including59required) match/no test maps. Main Game EXE hashes match native binaries, Game/non-test receipts and active plugin exclusions pass; disabled/Editor-only declarations and stock engine test descriptors/icons remain.6036scoped files have no exact local Zen credential value, not comprehensive secret/unnamed-chunk/runtime proof. Shipping Game was up-to-date, not recompiled; no source/config/launcher change, recook, player/security change or full-suite rerun. Dirty QA archives do not replace required clean candidate/CI/trusted-player/owner approval. All owned processes terminal; HANDOFF.md records next external gate.

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

- Before changing models or reaching the context limit, save the current task/evidence/next action in `HANDOFF.md` and finish with the `HANDOFF_READY` marker defined in `AGENTS.md`; partial progress never closes a milestone gate.
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

## C-018H private LOD progress — 2026-10-19 23:22 UTC

Import/static readback verified ONLY in existing GamePreview, new LOD01 derived
from Runtime05/IntegrationV3. Three actual LODs:49758/12420/4954 native triangles,
38756/12165/6000 vertices,8sections/2UV channels each, screen sizes1/.5/.25.
Source counts differ by exactly24 coincident-corner brass faces per LOD; independent
GLB analysis plus exact native per-material checks reconcile them. No source
bytes, original Runtime05, main Content/Config/baselines or gameplay changed.
Fresh readonly157Content files byte-unchanged, native1/1clean, strict private build,
Python10+9checks/repo validation PASS. Preserved failures and complete evidence:
[LOD review](M6_SEMIREAL_LOD_REVIEW.md). NEXT remains C-018H_PrivateLODCheck:
animated deformation per LOD, real GPU forced/automatic transitions and actual
new-model100-unit performance. Owner approval/reference hardware-budget/player
QA/main art adoption and M7/M8 remain OPEN. Do not substitute static readback for
those gates or repeat previous full141 tests without a relevant source change.

2026-10-20 motion/GPU update: Animation02 verifies actual engine-skinned renderLOD
vertices for Idle/Walk/Block/Attack/Death and held poses at eachLOD. Explicit
process-only suppressed-attack mutation correctly fails three regional geometry
checks; normal1/1clean. GPU03 verifies actual D3D12/SM6 render-threadLOD2/1/0 across
legal RTS zoom.55/.6625/.70, actual FPSauto0, forced1/2/0 and exitauto2, nine PNGs
inspected;1/1successWithWarnings (knownMotionVectorwarning),0errors/failed/pending.
157privateContent files hash-unchanged; no source meshes, main art or runtime
changes. [LOD review](M6_SEMIREAL_LOD_REVIEW.md) preserves failed fixture runs,
native receipts, source/PNG lineage and scope limits. Next
**C-018H_PrivateGPUPerformance**: D-05 is60FPS1080p/50 slice units plus separate
100-unit stretch test; measure new-model simulation/rendering on declared
hardware, not old original-art NullRHI or the one-body720p capture. No complete
M6/M7/M8/owner-acceptance/continuous-video/trusted-player claim.

2026-10-20 first measured semireal pair: PRIVATE observer reuses unchanged
Encounter50/100 canonical gameplay, binding LOD01 per instance BEFORE init.
Perf50_02/Perf100_01 each pass actual combat/training/FPS/physical victory with
all-new-art population/ticks/realtime and inspected1080p output screenshots;
157Content hashes unchanged. Full-population RTS means58.50/59.69FPS (1138/626
frames), GPU3.421/3.567ms, observercost included. Schema2 parser retains complete
4176/3776 native CSV frames using final union header. Failed old-art CDO attempt
and185-frame parser mistake preserved, NOT valid evidence. Technical capture
is not target acceptance: near60 median suggests Editor cap; actual runtime
battery limiter/smoothing and internal resolution still need confirmation.
[Performance review](M6_SEMIREAL_PERFORMANCE_REVIEW.md) gives exact artifacts,
scope and next checks; keep C-018H_PrivateGPUPerformance/M7/M8/owner/player OPEN.

2026-10-20 runtime observation supersedes the cap/resolution TODO above:
Perf50_03 records battery1/DontLimit0/smoothingfalse/t.MaxFPS0, confirming the
installed Editor60cap predicate, and primary resolution upper.728869–.846294
(NOT fixed100%). New opt-in, process-only diagnostics set ScreenPercentage100
and DontLimitOnBattery1 without persistent/Windows/security or other quality,
camera, unit load, AI or animation changes. Default and diagnostic cohorts remain
separate. Perf50_NativeDiag02 passes canonical gameplay at full50–51population:
1597RTSframes,81.99FPSmean/12.197ms,p9514.072; upper=secondary=1/dynres0 supports
source-grounded fixed full-scale inference, NOT a GPUbuffer size readback.
Screenshot inspected, legacy camp art still unfinished. Launcher semicolon bug
and failedDiag01 preserved; fixed comma queue8PS testsPASS, analyzer11PASS.
100-unit diagnostic, repeat variability and player/owner gates remain OPEN;
this provisional Editor sample does not approve release/M7/M8/main-art adoption.

Repeat cohort now completed and preserved in `native-diagnostic-cohort01.json`:
50units81.99/84.80FPS,p9514.072/13.549ms;100stretch51.68/65.53FPS,
p9521.591/16.930ms. All4canonical native encounters pass (known engine warning
only), same observer/DLL,157Content hashes unchanged,4PNGs inspected. No pooled
FPS/cherry-picking:100variability remains unexplained and even its faster p95
exceeds60FPS budget. Provisional50Editor budget met twice; packaged/owner gate
NOT closed. Full details/hashes in performance review. No identical recaptures
needed now without a source/asset change or concrete investigation hypothesis.
Next independent work: G-VQ02_CampSource1 private terrain+wall+onebuilding source
proposal at legionary scale; preserve canonical collision/nav and obtain owner
visual acceptance before broad adoption. M7/M8/player/security/rights remain OPEN.

2026-10-20 G-VQ02_CampSource1 source delivery verified: private original terrain,
palisade and ONEbarrack under SourceArt/Environment/RomanCamp/semireal_v1,real
editable BLEND/4GLBs/PBRatlas and previews,51458tri total. Soil generated with
built-in image_gen (actual1254, baked atlas2048; no added source-detail claim).
Independent4GLB imports validate actualtri/UV/embeddedPBR/dimensions/nocharacter;
saved samplers identical,2final sourcePNG inspected,sourcecopySHA/LFSattrs checked.
Exporter packing warning preserved/source-grounded, no main gameplay/assets edit.
[Camp source review](M6_SEMIREAL_CAMP_SOURCE_REVIEW.md) documents prompt/provenance/
failed initial assumption/units/sharedpivots/rights and technical limits. NEXT
C-018H_PrivateCampPreview: fresh private integration and realRTS/possession cameras,
original157assets hashguard, collision/nav/readback/owner review. Source rendering
alone does NOT close VQ-02, production adoption, performance/player/M7/M8 gates.

2026-10-20 C-018H_PrivateCampPreview technical proof complete: existing private
Camp01=3SM/6materials/3shared2048textures, actualnative51458tri/UV/cm/PBR/simple
collision0 verified, original157Content files preserved. Failed author verifier
(missing Editor subsystem in commandlet) fixed via publicmeshAPI/readonlyprocess,
not reimport/overwrite. ActualRTS/FPS/exit and3component visibility witnesses now
pass with identical cameras/heldsourcepose;7GPU02PNGs inspected. EarlierGPU01
success INVALIDATED after receipt showedfallingcamera; correctedguard detects
samebinary mutationGPUFail1/error1 while nativeassetreadback passes.169Content
hashes unchanged,cohortreceipt/strictbuilds/12Python guards saved. Full lineage:
[camp preview review](M6_SEMIREAL_CAMP_PREVIEW_REVIEW.md). NEXT
C-018H_PrivateCampCook: explicit private cook reachability/current registry and
PBR evidence. No owner/mainmap adoption, realinput/navgameplay, environmental
performance/player/security/rights/M7/M8 approval; full goal remains active.

2026-10-21 C-018H_PrivateCampCook technical content gate verified. Cook01 native
exit0 used defaultZen/SM5; wrapper's loose-file expectation failed, native registry
then proved12assets, original attempt preserved. FreshCook02 selects supported
loosewriter and process-onlyaddsSM6withoutremovingSM5 or editing config:537cooked+
7platformskip=544,0errors/warnings.12uasset/12uexp/3texturebulk, actualGlobal/Game
SM5+SM6shaderarchives,6materials inSM6assetinfo. Native runtime/development dumps
validate classes/tags and35hard-game dependency identifiers(34content+NavigationSystem
script),0missing;169Content/14source/config/descriptorunchanged. StrictUTF16/
primary-ID aware verifier and12targeted guardsPASS. Full evidence/limits:
[cook review](M6_SEMIREAL_CAMP_COOK_REVIEW.md). This is directed PRIVATEreachability,
not normalShippingmap adoption/player/collision/nav/performance/rights approval.
Ownerdirectionquestion pending; no silence-as-approval. NEXT independent
C-018H_FontNoticeMap uses existing exactfontmetadata, no newextraction/build/cook.
M7/M8/fullgoal remain OPEN.

2026-10-21 C-018H_FontNoticeMap technical mapping verified:
[per-identity notice map](M7_FONT_NOTICE_MAP.md), FontNoticeMap02.json.
14exactraw hashes/15paths per historical QA configuration,45 installed/extracted
fontpaths rehashed; pinned inventories/extraction/Pak/manifest/TPS/document links.
Verbatim nameID0/13/14 records preserved, mixed2011/2017 Roboto separated,
Old/Tiles Bold alias outside TPS directory explicit; exact Mono directory proof
not inherited by Fallback.12directed Python testsPASS. ATR-13 scope refined,
owner contractual/notice decisions and content/provider/platform coverage OPEN;
no final bundle, font/license rewrite, extraction/build/cook/network/native change.
Does not approve a clean release candidate, security, trusted player, M7 or M8.

Owner response to private camp: «Prefiero ajustes visuales», NOT adoption approval.
[Visual brief](M6_VISUAL_QUALITY_BRIEF.md) now distinguishes observed ground/roof/
timber polish from a possible architecture change. Subsequent explicit response:
«Cambiar ambas cosas». Source revision02 now delivered in
SourceArt/Environment/RomanCamp/semireal_v2, not adopted to mainmap.
[Revision proof](M6_SEMIREAL_CAMP_REVISION_REVIEW.md): actual4GLB/BLEND/PBR,
32986tri/2048atlas, longer barrack/porch/openings and procedural material revision.
NativeAuthor03/Readback02exit0,4actualGLBreimports/2inspectedrenders,14puretests;
4410sampled clay-face atlas coverage repaired within2K, no exported vertex-color
double tint. Python failure negative native73 is rejected (older0/stderr wasNOTpass).
Existingv1/Camp01/evidence preserved. NEXT C-018H_CampRevision02Preview: new private
Camp02 import with169existingassets hash-protected, realRTS/FPS/PBR/unit checks.
Not historical/owner/player/performance/functionalNav/collision/M7/M8 approval.

C-018H_CampRevision02Preview verified in the EXISTING privateGamePreview:
Camp02 adds12assets (3SM/6mat/3tex), native source selector/pinned v1+v2 hashes;
defaultv1 and Camp01 preserved. Native import/reopen0,169previous/181current
ContentFILES protected (179Unrealassets+2designJSON). Strict private fixture build
4actions/18.82s0; actual D3D12/SM6/1280x720 realRTS/possess/exit native2Success,
1knownMotionVectorwarning/0errors.7PNGs inspected; same-source/DLL falling-pose
mutation is ExpectedFail1/native255, unchanged181files. New8pure source-policy
tests pass; [cohort and exact limits](M6_SEMIREAL_CAMP_REVISION_REVIEW.md).
No mainmap/adoption/physicalinput/functionalNav/collision/perf/player/rights gate
cleared by that preview. Subsequent C-018H_CampRevision02Cook also VERIFIED:
fresh Camp02/v2 output, native0/0errors/0warnings,537cooked/7platform-skipped,
12actual assets/35hard-Game identifiers/0missing,6SM6material mappings and actual
SM5+SM6 archives.181Content/14source/Config/descriptor unchanged;15verifier plus
8source-policy tests PASS. [Exact cook evidence/limits](M6_SEMIREAL_CAMP_COOK_REVIEW.md#camp02--semireal_v2--current-revised-private-cook).
NEXT C-018H_CampRevision02AdoptionDecision: actual revised RTS/FPS owner review
pending, not automatic main-map adoption. After approval evaluate shared-origin
fit/offset against existing building/placement/nav authority before integration;
otherwise implement the requested visual revision. No repeat source/import/GPU/
cook gates solely on model switch. Trusted player/security/rights/CI/M7/M8 OPEN.

C-018H_CampRevision02Fit measured read-only while owner review remains pending:
actual saved Barracks4x4/PrototypeScale2.5,2.5,3; source mesh6.686x5.73x3.815m;
spawn CDO+300cm/capsule CDO42cm. Natural mesh does NOT fit current footprint;
7x6 is an unapproved enclosing-grid proposal, not a saved change. Naive swap
would distort scale/height; construction/selection/material-slot0 and spawn need
an explicit integration adapter, not just a mesh assignment. Native PID43828
0errors/0warnings,181Content/14source/Config/descriptor unchanged;9pure tests PASS.
[Exact read-only fit evidence and next contracts](M6_SEMIREAL_CAMP_FIT_REVIEW.md).
Owner decision received2026-10-30: «Pedir más ajustes visuales». This supersedes
the pending-adoption NEXT above: Camp02 is NOT approved for functional integration.
NEXT G-VQ02_CampRevision03Brief / GPT: obtain concrete changes to the current
sample or an annotated view/reference, retain agreed semi-realistic Roman style
and architecture+materials scope, then define acceptance criteria for revision03.
Do not generate speculative v3, adopt Camp02, or treat prior technical gates as
proof for new assets. No mainmap/footprint/spawn change is approved by this answer.

Subsequent owner instruction2026-10-30 «segui la tarea no importa» delegates the
next private visual iteration. NEXT G-VQ02_CampRevision03Source: oxide bands/gable
military identity, actual equipment rack/fasteners, less gravel/wood contrast;
same envelope/layout/cameras and3meshes/6materials/2Katlas/65ktri budget.
New source generation/readback needed; do not adopt rejected Camp02 or reinterpret
this as footprint/spawn/mainmap/release approval. Existing v1/v2 remain untouched.

G-VQ02_CampRevision03Source DELIVERED: real versioned semireal_v3,14files,
oxide/gable military identity, physical equipment rack/fasteners and quieter
soil/wood/clay.35,410tri/3meshes/6slots/2K/1UV, no new rig/LOD/nav/interactive
mechanics. Author02/independent fourGLB Readback02 native0;4final PNGs inspected,
28prior sources intact,20pure tests PASS. [Actual source evidence/limits](M6_SEMIREAL_CAMP_REVISION3_REVIEW.md).
NEXT C-018H_CampRevision03Preview: explicit pinned v3/Camp03 profile, create-only
import/read-only and real existing RTS/FPS fixture in current GamePreview,181files
protected. No new clone, renderer/camera shortcut or automatic mainmap adoption.

C-018H_CampRevision03Preview VERIFIED: explicit v3/Camp03 source/profile and10policy
tests; native import/reopen0,181old/193current ContentFILES intact (191assets+2JSON).
Strict private fixture build4actions/18.61s0; realD3D12/SM6/1280x720 RTS/possess/return
2Success/0errors/1knownMotionVectorwarning;7PNGs inspected. Same-source/DLL posefall
mutation expectedFail1/native255,193files unchanged. [Actual native evidence/limits](M6_SEMIREAL_CAMP_REVISION3_REVIEW.md#private-camp03-import-and-actual-engine-views--completed).
NEXT C-018H_CampRevision03Cook: fresh selected v3 content cook+native registries/
shader/dependency verification; preserve prior outputs, no default-map/engine/Zen
change or standalone reachability/player claim. No mainmap/footprint/nav/perf/
physicalinput/rights/M7/M8 gate cleared by preview. Delegated brief is not a blocker.

C-018H_CampRevision03Cook VERIFIED: fresh directed Camp03/v3 output, native0/
0errors/0warnings,537cooked+7platform-skipped; actual12assets/35hard-Game identifiers/
0missing,6SM6material associations and both shader-format archives.193Content/
14source/Config/descriptor unchanged;17verifier tests PASS. [Evidence/limits](M6_SEMIREAL_CAMP_COOK_REVIEW.md#camp03--semireal_v3--current-private-cook).
NEXT C-018H_BuildingPresentationAdapter: implement opt-in final-mesh presentation
using the existing building/content authorities, preserving prototype defaults,
construction/selection/state visuals and PBR slots. First verify privately; no
automatic mainmap/footprint/spawn changes or expanded adoption. Current v2 fit
findings inform safe activation requirements, not approval to alter game rules.

C-018H_BuildingPresentationAdapter CODE/LOGICAL GATES VERIFIED: runtime optional mesh/transform,
actual-bounds/grounding guards in registry and actor, separate state/faction plate
and preserved PBR slots implemented with null defaults. Root Editor Build04 PASS;
Test02 native0/2Success/0test errors or warnings, original Content/Config/descriptor
unchanged. Includes real canonical factory lifecycle and unsafe opt-in rejection.
Strict IWYU native0/115actions; Shipping native0/10actions (player not executed);
full regression native0/142 passed (141 clean + 1 missing-file-contract warning),
0failed/not-run/in-process. Closure confirms protected files unchanged after full
suite; validator0M1warnings/LFSfsck/diffcheck PASS, all jobs terminal. This does
NOT close actual Camp03 runtime integration, rendered approval or player QA.
NEXT C-018H_InitialPresentationCoverage: real starter-camp native coverage of the
opt-in initial-completion branch, then actual-asset integration under a fit/activation
decision. No assets/mainmap/footprint/spawn changes. [Contract/evidence/limits](M6_BUILDING_PRESENTATION_ADAPTER_REVIEW.md).

C-018H_InitialPresentationCoverage VERIFIED: new real starter-camp InitialCampPIE
checks exact transform/PBR slots/Complete/GC and canonical training receipt,
six-resource debit and cancellation refund. Build05/Test03 PASS (3cleanSuccess).
Controlled old zero-pivot mutation Build06/Test04 catches exactly the intended
initial-transform failure; source restored byte-identically, Build07/Test05 PASS
(3cleanSuccess). verified02 binds restored DLLs/unchanged6runtime files/protected
ContentConfigdescriptor; no live jobs or new adoption. Previous full suite142 and
IWYU/Shipping retained, not rerun for test-only work. [Evidence](M6_BUILDING_PRESENTATION_ADAPTER_REVIEW.md#c-018h_initialpresentationcoverage--verified).
NEXT C-018H_CampFitDecision: decide 4x4 compact architecture vs7x6 plus spawn/nav
verification, then integrate actual Camp03 (or compact successor), never equating
these transient-mesh tests with final rendered adoption. M7/M8 gates remain open.

C-018H_EditorPresentationValidation VERIFIED while fit choice pending: individual
building IsDataValid now calls existing presentation policy (WITH_EDITOR only),
closing disagreement with registry/actor. Test06 reproduced exactly8assertions;
Build09/Test07 passed4presentation tests, Test08 passed3content tests (19canonical
definitions); final Build10/Test09 passed direct editor validity cases plus all
six saved building DataAssets individually. verified03 protects Content/Config/
descriptor and unchanged six original runtime files; no new adoption or livejobs.
Editor-only production change inRomanContentDefinitions.cpp; prior full142/
IWYU/Shipping/cook/GPU not rerun or relabeled. NEXT remains C-018H_CampFitDecision;
user choice still required before footprint/spawn changes. [Evidence](M6_BUILDING_PRESENTATION_ADAPTER_REVIEW.md#c-018h_editorpresentationvalidation--verified).

2026-10-30 goal BLOCKED after three consecutive revalidated impasse turns; scope
NOT complete or reduced. Next input:4x4compact vs7x6+spawn/navigation decision.
All owned native jobs terminal; completed source/tests preserved. M7 still needs
trusted actual playerQA, isolated licensedCI, verified corrected dependency
integration and rights/attribution decisions; M8 remains downstream. No tests,
security changes, VM restarts or art/adoption changes to bypass these gates.
Current resume checkpoint: [HANDOFF.md](HANDOFF.md).

2026-10-30 OWNER FIT DECISION RECEIVED: enlarge Barracks to7x6 and adjust exit with
tests. This resolves the previous fit blocker; no compact replacement generation.
C-018H_Camp03FunctionalIntegration IN PROGRESS: optional-art exit resolver uses
actual footprint/capsule, nav projection and collision checks; legacy branch
unchanged. Root builds and4presentation regressions PASS; actual enlarged-spawn
proof still pending. Private source/DLL/PDA recovery snapshots preserved,193Content
files untouched; runtime sync/private build in progress. No root art/DA/map adoption
or release approval. [Current scope/gates](M6_CAMP03_FUNCTIONAL_INTEGRATION_REVIEW.md).

C-018H_Camp03FunctionalIntegration INITIAL-ORIENTATION FLOW VERIFIED privately:
Author01 adopts only backed-up privateBarracksPDA,7x6/realCamp03/5PBRslots/scale1;
RootContentConfigdescriptor andother192privateContentfiles unchanged. Real trained
recruit exits at442cm with42cm capsule, full navigation path and314.483cm actual
canonical movement completed (35.517cm remaining, HasMoveCommandfalse). Producer06
native0/1cleanSuccess, no teleport/dilation/heldpose. Corrected roof-level nav
selection and generated decoration overlapping footprint, preserving nil legacy.
RootBuild14/Test11 native0/fourcleanSuccess. verified01 binds source/runtime parity
and protected inputs, alljobs terminal. This is NOT full integration/visual/player
approval; fresh IWYU/Shipping/full regression still due for runtime changes.
NEXT C-018H_Camp03ExitMatrix: four orientations, obstructed-exit refund/no ghost,
canonical placement/selection, then real RTS/FPS functional views and eventual
root adoption. No extra footprint decision needed. [Evidence/limits](M6_CAMP03_FUNCTIONAL_INTEGRATION_REVIEW.md).
