# UE Project Context

*Last updated: 2026-09-26*
*Audit baseline: `89d8d66e5051d303dea9c965317cdb961d8a52f6`*
*M1 baseline: `0b74ad1566306d7487629812de68bc6abd7f0594`*
*Status: M0–M2 complete; M1 clean-clone proven; M3.2a verified; M3.1 routed policy and remaining M3 work open*

## Engine & Project Overview

**Declared engine version:** UE 5.8, launcher association
**Engine availability on this workstation:** UE 5.8.2 Launcher build (changelist 56702186) is installed at `C:\Program Files\Epic Games\UE_5.8`
**Selected engine version:** UE 5.8. M0 found no binary assets in the worktree, Git history, or accessible standard user folders that require a 5.7 recovery.
**Project name:** RomanEmpireGame
**Description:** Single-player Roman campaign, settlement/RTS, and direct-soldier-control prototype
**Project type:** Game
**Genre/domain:** Hybrid turn-based campaign, real-time strategy, and first-person combat
**Target platforms:**

- Windows desktop only for the first playable slice.

**Scope notes:**

- Max players is currently one.
- Multiplayer, dedicated server, and client targets are not planned for the first playable slice.
- The legacy asset inventory is complete. `L_Testbed.umap` and `L_VerticalSlice.umap` provide the committed deterministic M1 bootstrap; they are validated, materialize correctly through Git LFS, and reproduce their reviewed hashes from a clean clone.

## Module Structure

**Primary game module:** `RomanEmpireGame`

| Module | Type | Notes |
|---|---|---|
| `RomanEmpireGame` | Runtime | All current gameplay, UI, campaign, online prototype, asset helpers, and audio code |
| `RomanEmpireGameTests` | Editor | Native automation tests; deliberately excluded from Game and Shipping targets |

**Current build targets:**

| Target | Type | Settings |
|---|---|---|
| `RomanEmpireGameTarget` | Game | Latest UE 5.8 build settings; include order UE 5.8 |
| `RomanEmpireGameEditorTarget` | Editor | Latest UE 5.8 build settings; include order UE 5.8; includes the test module |

**Current module dependencies:**

- Public: `Core`, `CoreUObject`, `Engine`, `EnhancedInput`, `UMG`.
- Private: `AIModule`, `NavigationSystem`, `InputCore`, `Slate`, `SlateCore`, `HTTP`, `Json`, `ProceduralMeshComponent`.
- Test module private: `Core`, `CoreUObject`, `Engine`, `AIModule`, `NavigationSystem`, `RomanEmpireGame`, `UMG`, `Slate`, `SlateCore`, `RenderCore`, `InputCore`, `ApplicationCore`, `EnhancedInput`, `UnrealEd`. `RenderCore` supports the transient scene-viewport projection fixture; AI/navigation dependencies support real path-following integration coverage.
- Both modules explicitly set UE 5.8's current `IWYUSupport.Full` policy. `BuildProject.ps1 -Action IWYU` performs the strict non-unity/no-PCH Editor build once the toolchain is installed.
- No general-purpose editor/developer module exists. Automation Driver remains disabled and has no dependency from either current module.

**Source organization:** One runtime module organized by domain. Cross-module contracts live under `Public` (`Building`, `Camera`, `Campaign`, `Content`, `Core`, `Faction`, `Input`, and `Units`); the module root is a private include path so runtime implementations remain internal. New cross-module contracts must go under `Public`, while implementation-only code remains private.

**M3 selection additions (2026-09-27):** `Public/Core/SelectionOperation.h` defines Replace/Add/Toggle for controller commands. Building ownership and additive-selection changes add no module/plugin dependencies; the existing Editor-only test module covers them.

**M3 formation addition (2026-09-28):** exported stateless `FRomanFormationLayout` in `Public/Units/RomanFormationLayout.h` computes selection-level travel-facing slots. No new modules/dependencies/assets; existing AI/navigation dependencies handle projection and physical execution.

## Plugin Dependencies

**Currently enabled engine plugins:**

- `EnhancedInput` — input foundation.
- `ProceduralMeshComponent` — retained for deterministic placeholder geometry during recovery.
- `ModelContextProtocol` — Editor-only, manual loopback Unreal MCP service.
- `EditorToolset` — narrowed Editor asset/level operations for MCP sessions.
- `AutomationTestToolset` — narrowed Editor automation access for MCP sessions.

**Disabled/quarantined plugins:**

- `Landmass` — no current source reference or slice requirement.
- `AISupport` — no direct source/plugin or module dependency requirement.
- `RemoteControl` and `RemoteControlWebInterface` — legacy Python E2E bridge, disabled in normal defaults.
- `AutomationDriver` — removed from runtime UI code and disabled until a separate test-only module exists.
- `AllToolsets` — intentionally disabled after validation showed it activates unrelated privileged toolsets and Game Features configuration paths.

**Current editor-only tooling:**

- UE 5.8 built-in `ModelContextProtocol` (friendly name Unreal MCP) with only `EditorToolset` and `AutomationTestToolset` enabled.
- The server must remain loopback-only, manually or per-user started, and unavailable in Shipping.
- Project game/runtime modules must not take a dependency on MCP/toolset modules.
- No third-party Unreal MCP is approved for a UE 5.7 fallback without a separate compatibility and security review.

**Marketplace/Fab plugins:** None recorded.
**Project plugins:** No `Plugins/` directory is currently tracked.
**Plugin license inventory:** No third-party plugin is vendored; engine-provided plugins remain governed by Epic's terms as recorded in `THIRD_PARTY_NOTICES.md`.

## Coding Conventions

**Naming prefixes:** Standard Unreal naming (`F`, `U`, `A`, `E`, `I`).
**Header style:** `#pragma once`.
**Primary log category:** `LogRomanEmpire`, declared in `RomanEmpireGame.h` and defined in `RomanEmpireGame.cpp`.
**Assertion policy:** Not yet established; use explicit validation for user/data input, `check` for impossible programmer invariants, and `ensure` only for recoverable diagnostic conditions after policy approval.
**Header organization:** Flat domain folders in the runtime module.
**Formatting:** `.editorconfig` is tracked; no `.clang-format` is currently required.

**Planned rules:**

- Prefer `TObjectPtr` for persisted reflected UObject members when touching a class, with `UPROPERTY` ownership where required.
- A header's generated header remains its final include.
- Include each directly used type; do not depend on transitive includes.
- Runtime modules may not include editor, Automation Driver, Remote Control, MCP, or test headers.
- Do not perform world-wide actor scans from per-unit tick code.
- Do not introduce a second owner for resources, turns, territory ownership, training queues, combat damage, or victory state.

## Gameplay Framework and Subsystems

**Current framework classes:**

- GameMode: `ARomanEmpireGameMode`, which delegates canonical turn processing to `ACampaignManager` and owns the explicit `EGameplayMode` selection.
- GameState: `ARomanEmpireGameState`, explicitly assigned by GameMode; exported single-player read/event projection over canonical managers, without mutable wallet/turn/mode/outcome copies. Consumers re-query on notifications because event payloads may be overtaken by nested commands. This is not multiplayer replication.
- PlayerController: `ARomanEmpirePlayerController`; owns local selection and possession, with one same-world/command-ready/player-faction eligibility query for selection, player orders and continued control. Unit ownership remains on AUnitBase; canonical player faction remains on FactionManager through public GameState. Committed ownership notifications trigger selection/possession cleanup in every mode. Camera callbacks are revalidated, recursive teardown is guarded and EndPlay releases observers/input contexts.
- Default Pawn: `ASeamlessZoomCamera`.
- Formation commands: the player controller computes one layout per eligible selection, validates every projected slot before dispatch, and leaves squad membership unchanged. An invalid footprint preserves previous orders and reports transient feedback. Weak member snapshots and a shared move/attack batch revision prevent outer dispatch from overwriting commands issued by synchronous observers. Layout is not a persistent gameplay owner; each unit's Orders remains canonical.
- Selected-unit STOP/HOLD: the controller exposes guarded selection facades using the same weak snapshots and batch revision as Move/Attack. Native UMG buttons live in the tactical selection panel; STOP clears intent and allows later idle acquisition, while HOLD persists and suppresses it. Both hide outside eligible tactical unit selection. No new input action/asset, subsystem or order owner was introduced.
- Tactical AI controller: private `ARomanUnitAIController`; executes the unit's canonical orders through CharacterMovement/NavMesh, with request/revision guards and no independent order owner. No partial paths or direct-walk fallback: failure settles in Hold/Unreachable. Moving targets retain Attack intent; attack-range arrival stops the route. Possession parks/reuses the same controller, death cancels navigation and EndPlay destroys it. Both maps persist dynamic 42/192 cm Recast agents; CharacterMovement RVO is used, not Detour crowd following. The controller plans formation destinations; each AI executes its own physical route.
- Unit base: `AUnitBase`; current concrete unit: `ALegionary`. Private `FUnitVitalState` is the single owner of live health/stamina/morale and `Uninitialized/Alive/Dead/InitializationFailed/Ended`. Actor commands coordinate world effects and notifications; initialized deferred actors wait for BeginPlay before acting. Death quiesces actions/collision/movement before notification; bounded engine lifespan and EndPlay own cleanup. `FUnitOrderState` now owns None/Move/Attack/Hold intent, destination and weak target. `FUnitCombatState` owns the single recovery clock and orthogonal blocking; `FUnitSelectionPresentation` owns local indicators/material, while the controller owns the selected set. `GetActivity()` is derived, never separately mutable. Routed policy remains M3.1 work; actual-player rendered selection/possession/return is now verified.
- Building base: `ABuildingBase`; current functional building subclass: `ABarracks`.
- Selection update (2026-09-27): the controller shares live/placed/same-world/player-faction building eligibility with TRAIN and UMG, and retires selection on ownership/state/EndPlay changes. Additive unit commands feed the same canonical selected set; buildings remain exclusive. Invalid input produces a controller feedback event consumed by a three-second UMG timer, cleared on valid selection and teardown, without a second gameplay owner.
- Production UI: controller-owned native `URomanEmpireMainWidget` and registry-driven `UBuildingMenuWidget`; no Widget Blueprint required. `ARomanEmpireHUD` is an opt-in Canvas diagnostic overlay only, disabled by default.

**Current subsystem classes:**

| Class | Type | Current responsibility |
|---|---|---|
| `USupabaseSubsystem` | `UGameInstanceSubsystem` | Prototype HTTP persistence/leaderboard client; explicitly disabled for the offline slice |
| `URomanContentRegistrySubsystem` | `UGameInstanceSubsystem` | Loads and cross-validates faction, unit, building, and territory Primary Data Assets; provides canonical stable-ID/factory/menu lookups |
| `URomanUnitRegistrySubsystem` | `UWorldSubsystem` | Game/PIE-only live units with weak references and derived faction buckets; actor lifecycle/ownership remain canonical. Registers after successful BeginPlay, removes before death notifications/EndPlay, reindexes before owner notifications and clears on teardown. Supplies idle acquisition/rectangle selection and owns tactical squad membership: ordered weak members, one same-faction group per unit, atomic creation, disband and lifecycle removal. Handles are world/session-local GUIDs, not campaign/save identities; no spatial index. |

**Current actor-managed systems:**

- `AFactionManager` — canonical faction resource wallets and normalized unordered-pair diplomacy state; no territory ownership copy.
- `ACampaignManager` — sole turn counter, terminal outcome, derived objective projection, and ordered nine-stage campaign pipeline; its autosave stage currently emits a request to the M5.4 persistence placeholder. Public events notify committed state, and recursive turn/restart requests are rejected.
- `AWorldMapManager` — sole `FTerritoryCampaignState` owner, definition/actor registry, typed ownership transactions, and ownership events.
- `ATerritoryRegion` — presentation/spatial actor initialized from a stable territory definition; reads ownership from WorldMapManager and never owns or sets it.
- `AAudioManager` and `AMaterialManager` — currently unspawned/unconfigured helpers.
- `AProceduralMeshGenerators` — currently unused procedural geometry helper.

**GAS usage:** None. GAS is explicitly deferred unless later ability/effect complexity justifies it.

## Data, UI, Input, and Save Conventions

**Current data model:** Resource wallets are canonical in `AFactionManager`; zero-default amount/delta structs and typed atomic transaction results are shared contracts. `ACampaignManager` owns the sole turn counter and the shared turn-result/pipeline contract; `EGameplayMode` and `FZoomConfiguration` define explicit mode state and all zoom policy. WorldMapManager owns the sole stable-ID territory ownership registry, while FactionManager owns normalized symmetric diplomacy with typed legal transitions. Nineteen Primary Data Assets are the canonical faction, unit, building, and territory definitions; `URomanContentRegistrySubsystem` validates and exposes them by stable `FName` IDs. Runtime factories and training queues carry those IDs, and the M5.4 versioned save implementation will persist the same identities.
**Planned data model:** Data Tables only for future large flat tabular data; stable Primary Data Asset IDs remain the content/save boundary.
**Current UI:** Native UMG is instantiated by the local controller and reads global state through the public GameState queries/events; selected-actor events remain local to those actors. Resource, mode/turn, objectives/outcome, selection/production, and possession panels have no UI tick, owned gameplay counters, or world scans. NativeDestruct unbinds observers; reattachment catches up. Canvas is debug-only, with no gameplay panels or hit testing. The UMG canonical diagnostic panel is hidden in Shipping.
**Planned UI:** M6 adds presentation polish, menus, accessibility, and richer authored widgets without changing gameplay authority; native UMG remains the functional baseline.
**Current input:** `/Game/Input/DA_RomanInputConfig` links 19 saved actions and Strategic, Tactical, Possession and Shared contexts. SetupInputComponent loads/validates and binds them; public GameState mode/readiness events select exactly one gameplay context plus Shared without clearing unrelated mappings. Keyboard/UMG share eligibility guards; building quick slots require a visible menu. Mode/focus/death cleanup clears blocking, partial selection and placement. A digital movement-key release latch supplements UE 5.8 Boolean-only rebuild suppression. No runtime-generated or legacy action/axis mappings remain. Defaults and test scope: `INPUT_CONTROLS.md`.
**Planned input:** End-user remapping UI, saved profiles, dynamic labels and gamepad support remain later work; physical keys/modifiers already live in assets, separate from gameplay commands.
**Current save:** TODO logging plus an incomplete Supabase prototype that is explicitly disabled in committed configuration.
**Planned save:** Versioned local `USaveGame` first; authenticated cloud synchronization is optional M8 work.

## Build and Test Configuration

**Current build entrypoint:** `BuildProject.ps1`, with UE 5.8 discovery, optional `-EngineRoot`, Visual Studio/Git LFS prerequisite checks, and UBT/UAT actions. UBA is disabled by default because the audited workstation's Smart App Control policy rejects its locally generated unsigned Editor DLL; `-UseBuildAcceleration` is an explicit opt-in for compatible machines. `build.bat` is a compatibility wrapper.
**Current local editor/toolchain:** VS Code 1.136.1 with Microsoft's C/C++ Extension Pack, Visual Studio Build Tools 2022 17.14, MSVC 14.44, the .NET Framework 4.8 SDK, and Windows SDK 10.0.22621/26100. VS Code remains the selected editor while Build Tools supplies the compiler and SDK.
**Latest build evidence:** M2 passed 16/16 native tests with zero warnings/failures/pending on 2026-09-15 (`Artifacts/Automation/20260915_131817_133`, `-DisableFab`); strict IWYU and Shipping also pass. The objective callback's old implementation failed a controlled mutation run before restoration. The final Development package contains 559 cooked packages with zero cook errors/warnings. Its actual standalone executable initialized the default map/GameState/UMG/input, rendered the inspected 1920×1080 `Artifacts/M2_Final-PackagedUMG.png`, and exited 0 with no failure signatures. Exact hash/logs, optional Fab workaround and historical failures: `M2_EXIT_REVIEW.md`. No Windows security or driver setting changed. M1 remains clean-clone proven at `0b74ad1566306d7487629812de68bc6abd7f0594`.
**Latest unit-foundation verification (2026-09-19):** Restored code passes 18/18 native tests with zero warnings/failures/pending (`Artifacts/Automation/20260919_200357_604`), strict IWYU and Shipping. Deliberately restoring premature death notification and float-only regeneration made both new tests fail before correction. The Development package cooked 559 packages without errors/warnings; its actual player rendered the inspected 1920×1080 startup capture and exited 0 with no failure signatures. `M3_UNIT_FOUNDATION_REVIEW.md` records the exact binary hash/logs and limits; rendered selection/possession and active-state separation remain open.
**Latest unit-state verification (2026-09-20):** The restored integration passes 21/21 with zero warnings/failures/pending (`Artifacts/Automation/20260920_173627_023`), strict non-unity/no-PCH IWYU and final Shipping compilation. A mutation run detected double time advancement, double rate division and stale effects after camera callbacks before restoration. `M3_UNIT_STATE_REVIEW.md` records evidence and limits. That state-integration revision had no new package/player evidence; the later presentation package below closes the rendered selection/possession gate. Routed policy remains open.
**Latest presentation verification (2026-09-21):** 21/21 native tests without warnings (`Artifacts/Automation/20260921_000502_613`), strict IWYU and Shipping pass. The Development package cooked 559 packages, with 7 platform skips (566 total processed), without errors/warnings. Its actual player passed strict selected/Pawn identity, original-camera restoration and cleared selection/possession checks; three inspected 1920×1080 captures show the ring and camera/HUD transitions with a corrected single-line mode/turn header. Two local diagnostic commands are compile-time absent from Shipping, introduce no services/dependencies and do not exercise physical input. The parser has separate synthetic regression checks. Exact binary/hash/logs, two player-only engine/host warnings and coverage limits: `M3_PRESENTATION_REVIEW.md`. No assets, engine/plugin defaults, drivers or security settings changed. M3.1 remains open for routed policy/implementation; M3.2a player ownership eligibility and cleanup was the next independent package (verified below).
**Latest player-ownership verification (2026-09-21):** M3.2a passes 22/22 native tests without warnings/failures/pending (`Artifacts/Automation/20260921_164623_820`), strict IWYU (59 actions), Shipping and a fresh Development package with 559 cooked packages plus 7 platform skips, without cook errors/warnings. Stale-selection-payload and missing-faction-check mutations each fail the strengthened real-PIE test before restoration. The actual archived player passes ordinary exit and automatic ownership-loss cleanup, with six inspected 1920×1080 captures, strict Pawn/selection/flag identity checks and both exits 0. SHA256 `b7e63c892fd6d23b601c5ff7c086e6e1bd9965ea3f3e4715988c1b86cc2a256d`; exact logs, fixture fixes, parser tests and two player-only engine/host warnings: `M3_OWNERSHIP_REVIEW.md`. No physical input was injected or assets saved. Building ownership selection, additive selection and live unit/squad registries remain M3.2 work; routed design remains M3.1 work.
**Custom engine fork:** None recorded.
**Third-party C++ libraries:** None recorded beyond engine modules.
**Automation workaround:** On 2026-09-15, Windows Code Integrity rejected the updated engine Fab 0.0.17 Editor DLL. The game has no Fab dependency. `BuildProject.ps1 -DisableFab` is an explicit process-only opt-in for Test/Cook/Package; normal project/engine plugin configuration and Windows security are unchanged. Interactive Fab use requires a properly trusted installation; do not weaken application control.
**Platform-specific notes:** Windows/DX12 desktop; no Server or Client target planned. Windows security policy must not be weakened to load development binaries; use the repository's non-UBA default when application control rejects UBA output. This workstation can also reject newly generated unsigned player executables even after a successful package; release candidates therefore require signing or execution on a controlled runner whose application-control policy trusts the build output.

**Current tests:**

**Latest selected-command verification (2026-09-29):** `SelectedCommandsPIE` targeted 1/1 (`M3_Commands_Final_Targeted`) checks routed Slate clicks, active path cancellation, reentrant Stop-to-Hold supersession, persistent Hold and strategic/possession/owner-loss guards. Full regression 30/30 (`20260929_185107_129`) passes with fourteen tests carrying only the two known host WASAPI notices. Strict IWYU (63 actions) and Shipping (5) pass; no package/player claim. See `M3_SELECTED_COMMANDS_REVIEW.md`.

**Latest formation verification (2026-09-28):** `FormationLayout` covers deterministic geometry, invalid inputs and counts through 100 (not a performance measurement). `FormationPIE` verifies distinct slots, partial-squad selection, atomic footprint failure/feedback, reentrant batch supersession and real three-unit arrival. Final targeted 2/2 (`M3_Formation_Final_Targeted`) and full 29/29 (`20260928_152938_618`) pass; thirteen full-suite tests carry only the two known host WASAPI notices. Strict IWYU (63 actions) and Shipping (5) pass. No new package/player evidence. See `M3_FORMATION_REVIEW.md`.

**Latest navigation verification (2026-09-28):** `NavigationPIE` adds real obstacle avoidance and moving-target pursuit, stop/hold/reentrant replacement, unreachable-goal handling, AI reuse over two possession cycles and death/EndPlay path cleanup. Final targeted 1/1 (`M3_Navigation_Final_Targeted`) and full 27/27 (`20260928_151129_468`) pass; twelve full-suite tests carry host audio sample-rate/conversion notices, not navigation warnings. Strict IWYU (61 actions) and Shipping (7 actions) pass. No new package/player evidence. `M3_NAVIGATION_REVIEW.md` records details and remaining formation/rendered/performance coverage.

**Latest tactical squad verification (2026-09-28):** `SquadRegistryPIE` brings the full regression to 26/26 without result warnings/failures/pending (`20260928_000739_201`); final targeted 1/1 (`M3_Squads_Final_Targeted`). Tests cover atomic membership, initial forces and real training, disband preserving orders, reentrant regroup after owner change, death/EndPlay, GC and world teardown. No new module/dependency/asset or packaged-player evidence. Full details and compilation results: `M3_SQUAD_REGISTRY_REVIEW.md`.

**Latest building/additive-selection verification (2026-09-27):** Building ownership baseline `e5825e5` passed 24/24. The subsequent additive-selection changes pass 6/6 targeted tests (`Artifacts/Automation/M3_Additive_Fixed_Targeted`) and 25/25 full regression, zero result warnings/failures/pending (`20260927_142020_752`), strict IWYU (60 actions) and Shipping (6). The initial targeted run caught a cached Enhanced Input action surviving `FlushPressedKeys` until the next processing pass; the controller now checks held resolved mappings as well as the evaluated action, and the synchronous assertion passes. Saved input configuration now has 20 actions and Tactical has 18 mappings, superseding the historical 19-action input description above. Both Shift keys, mode/menu/placement/focus guards, Add/Toggle/Replace deduplication, rectangle projection, building input clicks and feedback expiry/destruct are covered. Fresh package/rendered evidence is pending for this revision; no earlier package is claimed as proof.

**Additive package/preview result:** The fresh Development archive passes UAT (567 packages processed), but Windows Code Integrity events 3077/3033 block its unsigned executable before launch. Packaged runtime remains pending. Separately, `Tools/verify_additive_selection.ps1 -EditorPreview` passes assertions and exits 0 with two inspected 1920×1080 captures showing 15-unit selection/rings, Shift help and invalid-target feedback. This uses the installed Editor with process-only `-DisablePython` to avoid editor-toolset Python initialization in game mode; editor widget-factory/render warnings remain. No security/default plugin settings changed. Details: `M3_ADDITIVE_SELECTION_REVIEW.md`.

**Latest live-unit registry package (2026-09-26):** Development package UAT exit 0; 559 cooked packages plus 7 platform skips. Actual player SHA256 `a4bcce2cdda660d1e183a073a7d5c1fd8b9648072b5aafb5e1d25ec8e51b074d` passes explicit and ownership-loss return (both exit 0), with six inspected captures. Known player rendering warnings remain unchanged. `M3_UNIT_REGISTRY_REVIEW.md` records commands, logs and limitations; no physical-drag or performance gate is claimed. The live-unit portion supersedes the pending registry item in the historical M3.2a entry; squad identity/membership remains open.

- M3.2b adds the twenty-third test, `LiveRegistryPIE`: lifecycle/reindex/reentry, true weak-reference GC, world isolation/teardown, real idle acquisition and controller rectangle projection. Headless zero-size viewports get transient dimensions, not a fabricated projection matrix; no physical drag claim. Restored targeted run passes 1/1 (`Artifacts/Automation/M3_Registry_Restored_Targeted`); final suite passes 23/23 (`20260926_203959_234`), both with zero result warnings/failures/pending. Strict IWYU (60 actions) and Shipping pass. Historical Sept22 audio notices did not recur; no host settings changed. Both deliberate lifecycle mutations failed and were restored. Package evidence: `M3_UNIT_REGISTRY_REVIEW.md`.

- The broken Python/pytest Remote Control scenarios were retired; `scripts/README.md` records why and names their replacements.
- One Editor-only native automation module with twenty-three tests (LiveRegistryPIE described above): required content paths/map/framework defaults, zero-default resources, atomic typed resource transactions, exact campaign turn order, canonical gameplay-mode/zoom boundaries, territory ownership/definition invariants, diplomacy symmetry/legal transitions, Primary Data Asset discovery/canonical values, cross-content validation failures, event-driven UMG integration in live PIE, input-config negative contracts, saved input defaults/modifiers, physical-key/mode lifecycle integration in PIE, real definition-change propagation with atomic/reentrant placement, safe public GameState reads/relays/teardown, and nested objective notifications. Live Slate TRAIN asserts all six costs and no-charge promotion/funds/queue rejection. Two unit tests add pure vital-state boundaries and real-PIE component/GC/initialization/selection/camera/death/reentry/cleanup coverage. Post-Destroy assertions inspect retained reflected state rather than calling rejected script functions. OrderState and CombatState add finite/atomic transition, weak-target Destroy/GC and single-clock boundary checks; OrdersCombatPIE adds persistent Hold, pursuit/target retirement, derived activity, exact AI/direct recovery cadence and nested damage/vitals/camera callbacks. PlayerOwnershipPIE adds real controller eligibility, mixed selection, preserved enemy AI orders, unit/player-faction changes, actual camera callback interruption, nested selection observer ordering and EndPlay unbinding. Its auxiliary world is collected before subsequent Slate input dispatch. All fixtures are transient and do not save assets.
- `L_Testbed` is the deterministic world/PIE test map; dedicated Functional Test actors and scenarios remain planned.
- The retired Python/Remote Control files remain recoverable from Git history but are not part of the supported test path.

**Planned tests:**

- Native automation tests for pure rules/state.
- Functional Test maps for actor/world workflows.
- Automation Driver in a separate test/developer module for UMG behavior.
- UAT cook/package and packaged-build smoke validation.
- Unreal Insights/stat captures against the M0 reference hardware target.

## Agent-Assisted Development Context

**Installed instruction skills:** See the controlled capability table in `IMPLEMENTATION_PLAN.md`.
**Live Unreal MCP status:** The loopback health, narrowed toolsets, map mutations, saves, PIE control, and readback were proven on `127.0.0.1:8100`. The editor/server is intentionally stopped outside active sessions.
**MCP operational rules:**

- Save and create a clean Git recovery point before bulk editor changes.
- Discover/describe a toolset before calling it.
- Execute Unreal MCP calls sequentially because they run on the game thread.
- Stop on any result that is not explicit success.
- Do not mutate assets while PIE or compilation is active.
- Re-run asset/content validation after generated or bulk-edited assets.
- Keep the server on `127.0.0.1`; do not expose the port or ship the server.

**Asset assistance:**

- Built-in `imagegen` may be used for concept art and temporary bitmap references after an explicit asset brief.
- UE 5.8 Unreal MCP may inspect/create/import/configure Unreal assets after the M1 connection gate.
- Blender/Substance MCPs are not installed because those applications are not currently present and their arbitrary scripting surfaces require a separate toolchain/security decision.

## Team Context

**Team size and roles:** Unknown.
**Source control:** Git on `main`, with an `origin/main` upstream.
**Branching/review policy:** Not established; M7 plans protected review and green gates before merge.
**Asset lock policy:** Unreal binary assets and source-media formats are covered by `.gitattributes` Git LFS rules. Team checkout/locking conventions beyond LFS remain a future collaboration-policy decision.
**Documentation:** `README.md`, `SETUP_GUIDE.md`, `IMPLEMENTATION_PLAN.md`, and `M1_REVIEW.md`; the plan is authoritative for future execution sequencing, while the review packet records the completed M1 gate evidence and limitations.

## Known Baseline Blockers

- M1 is closed at approved commit `0b74ad1566306d7487629812de68bc6abd7f0594`; its clean-clone/LFS reproduction passed and no remote push was performed.
- M2 resource, turn/mode/zoom, territory, diplomacy, definition and outcome owners are canonical. The public GameState, production UMG/input consumers, definition-propagation and exact-charge/nested-event tests resolve the aggregate architecture audit. Final corrected package/player gates pass; M2 is closed. M3.1 is in progress: component references, vitals and safe death/cleanup are implemented and tested; order/combat/selection owners are implemented; their new verification passes 21/21, strict IWYU and Shipping. Rendered selection/possession/return passed the actual-player check on 2026-09-21; routed policy/implementation remain open. See `M3_PRESENTATION_REVIEW.md` and `M3_UNIT_FOUNDATION_REVIEW.md`. M3.2a ownership/cleanup is subsequently verified with 22/22, IWYU, Shipping and two actual-player scenarios; see `M3_OWNERSHIP_REVIEW.md`. The next bounded work is a world-scoped live-unit registry, not another content-definition registry or a premature squad/save contract.
- Full RTS/possession gameplay, UI polish, audio, and art assets remain future milestone work and require provenance/licensing records. The existing procedural figures/buildings remain placeholders, not finished game art.

Update this context after M0 decisions, every engine/module/plugin migration, and any change to authoritative system ownership.
