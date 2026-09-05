# UE Project Context

*Last updated: 2026-09-05*
*Audit baseline: `89d8d66e5051d303dea9c965317cdb961d8a52f6`*
*M1 baseline: `0b74ad1566306d7487629812de68bc6abd7f0594`*
*Status: M1 complete and clean-clone proven; M2.1–M2.4 canonical state and Primary Data Assets complete; M2.5 active*

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
- Private: `InputCore`, `Slate`, `SlateCore`, `HTTP`, `Json`, `ProceduralMeshComponent`.
- Test module private: `Core`, `CoreUObject`, `Engine`, `RomanEmpireGame`.
- Both modules explicitly set UE 5.8's current `IWYUSupport.Full` policy. `BuildProject.ps1 -Action IWYU` performs the strict non-unity/no-PCH Editor build once the toolchain is installed.
- No general-purpose editor/developer module exists. Automation Driver remains disabled and has no dependency from either current module.

**Source organization:** One runtime module organized by domain. Cross-module contracts live under `Public` (`Building`, `Camera`, `Campaign`, `Content`, `Core`, `Faction`, and `Units`); the module root is a private include path so runtime implementations remain internal. New cross-module contracts must go under `Public`, while implementation-only code remains private.

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
- GameState: `AGameStateBase`, assigned explicitly by `ARomanEmpireGameMode`; a custom canonical GameState is planned in M2.
- PlayerController: `ARomanEmpirePlayerController`.
- Default Pawn: `ASeamlessZoomCamera`.
- Unit base: `AUnitBase`; current concrete unit: `ALegionary`.
- Building base: `ABuildingBase`; current functional building subclass: `ABarracks`.
- HUD: `ARomanEmpireHUD` Canvas implementation, with disconnected UMG classes also present.

**Current subsystem classes:**

| Class | Type | Current responsibility |
|---|---|---|
| `USupabaseSubsystem` | `UGameInstanceSubsystem` | Prototype HTTP persistence/leaderboard client; explicitly disabled for the offline slice |
| `URomanContentRegistrySubsystem` | `UGameInstanceSubsystem` | Loads and cross-validates faction, unit, building, and territory Primary Data Assets; provides canonical stable-ID/factory/menu lookups |

**Current actor-managed systems:**

- `AFactionManager` — canonical faction resource wallets and normalized unordered-pair diplomacy state; no territory ownership copy.
- `ACampaignManager` — sole turn counter and ordered nine-stage campaign pipeline; its autosave stage currently emits a request to the M5.4 persistence placeholder.
- `AWorldMapManager` — sole `FTerritoryCampaignState` owner, definition/actor registry, typed ownership transactions, and ownership events.
- `ATerritoryRegion` — presentation/spatial actor initialized from a stable territory definition; reads ownership from WorldMapManager and never owns or sets it.
- `AAudioManager` and `AMaterialManager` — currently unspawned/unconfigured helpers.
- `AProceduralMeshGenerators` — currently unused procedural geometry helper.

**GAS usage:** None. GAS is explicitly deferred unless later ability/effect complexity justifies it.

## Data, UI, Input, and Save Conventions

**Current data model:** Resource wallets are canonical in `AFactionManager`; zero-default amount/delta structs and typed atomic transaction results are shared contracts. `ACampaignManager` owns the sole turn counter and the shared turn-result/pipeline contract; `EGameplayMode` and `FZoomConfiguration` define explicit mode state and all zoom policy. WorldMapManager owns the sole stable-ID territory ownership registry, while FactionManager owns normalized symmetric diplomacy with typed legal transitions. Nineteen Primary Data Assets are the canonical faction, unit, building, and territory definitions; `URomanContentRegistrySubsystem` validates and exposes them by stable `FName` IDs. Runtime factories and training queues carry those IDs, and the M5.4 versioned save implementation will persist the same identities.
**Planned data model:** Data Tables only for future large flat tabular data; stable Primary Data Asset IDs remain the content/save boundary.
**Current UI:** Assigned Canvas HUD plus UMG classes that are not instantiated.
**Planned UI:** UMG is the only production UI; Canvas becomes debug-only and is removed from gameplay interactions.
**Current input:** Enhanced Input objects are constructed in C++ alongside legacy config mappings.
**Planned input:** Committed Enhanced Input assets with Strategic, Tactical, Possession, and Shared contexts.
**Current save:** TODO logging plus an incomplete Supabase prototype that is explicitly disabled in committed configuration.
**Planned save:** Versioned local `USaveGame` first; authenticated cloud synchronization is optional M8 work.

## Build and Test Configuration

**Current build entrypoint:** `BuildProject.ps1`, with UE 5.8 discovery, optional `-EngineRoot`, Visual Studio/Git LFS prerequisite checks, and UBT/UAT actions. UBA is disabled by default because the audited workstation's Smart App Control policy rejects its locally generated unsigned Editor DLL; `-UseBuildAcceleration` is an explicit opt-in for compatible machines. `build.bat` is a compatibility wrapper.
**Current local editor/toolchain:** VS Code 1.136.1 with Microsoft's C/C++ Extension Pack, Visual Studio Build Tools 2022 17.14, MSVC 14.44, the .NET Framework 4.8 SDK, and Windows SDK 10.0.22621/26100. VS Code remains the selected editor while Build Tools supplies the compiler and SDK.
**Latest build evidence:** Repository validation and VS Code generation succeed. UE 5.8.2 builds Development Editor, Development Game, Shipping Game, and the strict non-unity/no-PCH Editor target. On 2026-09-05, approved M1 commit `0b74ad1566306d7487629812de68bc6abd7f0594` reproduced from a clean clone through every gate. M2.1–M2.3 successively established canonical resources, turns/modes, territories, and diplomacy. M2.4 passes all four build targets and 9 succeeded/0 failed tests (`Artifacts/Automation/20260905_190532_896`); its Development BuildCookRun cooked 535 packages with 0 errors/0 warnings into `Artifacts/Packages/M2_4`. The Editor-hosted `-game -DisablePython` smoke initialized exactly 3 faction, 1 unit, 6 building, and 9 territory definitions, generated the complete world/armies, exited normally, and produced 0 project failure signatures. Development and Shipping receipts contain zero MCP/toolset references. Direct launch of newly generated unsigned players remains externally blocked before engine initialization by Smart App Control policy events 3033/3077; the protection was not weakened.
**Custom engine fork:** None recorded.
**Third-party C++ libraries:** None recorded beyond engine modules.
**Platform-specific notes:** Windows/DX12 desktop; no Server or Client target planned. Windows security policy must not be weakened to load development binaries; use the repository's non-UBA default when application control rejects UBA output. This workstation can also reject newly generated unsigned player executables even after a successful package; release candidates therefore require signing or execution on a controlled runner whose application-control policy trusts the build output.

**Current tests:**

- The broken Python/pytest Remote Control scenarios were retired; `scripts/README.md` records why and names their replacements.
- One Editor-only native automation module with nine passing tests: required content paths/map/framework defaults, zero-default resources, atomic typed resource transactions, exact campaign turn order, canonical gameplay-mode/zoom boundaries, territory ownership/definition invariants, diplomacy symmetry/legal transitions, Primary Data Asset discovery/canonical values, and cross-content validation failures.
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
- Resource, turn/mode/zoom, territory, diplomacy, and content-definition authority are canonical after M2.1–M2.4. M2.5 must now replace gameplay Canvas panels and disconnected UI state with event-driven production UMG before input-context consolidation.
- Production gameplay, UI, audio, and art assets remain future milestone work and require provenance/licensing records.

Update this context after M0 decisions, every engine/module/plugin migration, and any change to authoritative system ownership.
