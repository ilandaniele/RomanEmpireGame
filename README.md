# Roman Empire Game

Roman Empire Game is an Unreal Engine 5.8 single-player Windows project combining a campaign layer, real-time tactical command, settlement development, and explicit first-person possession of a soldier.

Para alternar modelos: Claude implementa y GPT genera imágenes/modelos; ambos continúan desde [HANDOFF.md](HANDOFF.md). [MODEL_WORKFLOW.md](MODEL_WORKFLOW.md) contiene los prompts de cambio y la marca final de cada entrega. Las reglas comunes están en [AGENTS.md](AGENTS.md), con entrada para Claude en [CLAUDE.md](CLAUDE.md).

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

M3 placement now checks buildable terrain, rotated footprint, territory ownership, blocking actors and canonical affordability before charging; stale previews are revalidated. The Roman camp's barracks is now a real completed producer rather than a decorative mesh. The 34/34 regression passes, but the enemy objective structure, production failure policy and a trusted packaged-player check remain open. See [M3_PLACEMENT_VALIDATION_REVIEW.md](M3_PLACEMENT_VALIDATION_REVIEW.md) and [M3_INITIAL_BARRACKS_REVIEW.md](M3_INITIAL_BARRACKS_REVIEW.md).

M3.6 now has a typed tactical battle result and one world-local evaluator fed by initial squads, training and deaths; native UMG displays live casualty counts through GameState events (35/35 regression). Capture and restart/continue remain open. See [M3_BATTLE_RESULT_FOUNDATION_REVIEW.md](M3_BATTLE_RESULT_FOUNDATION_REVIEW.md), [M3_BATTLE_WORLD_WIRING_REVIEW.md](M3_BATTLE_WORLD_WIRING_REVIEW.md) and [M3_BATTLE_UI_PROJECTION_REVIEW.md](M3_BATTLE_UI_PROJECTION_REVIEW.md).

M4 possession now has three-cycle real-PIE return evidence: the original tactical camera transform and selected soldier survive, and the soldier regains the same AI controller each time. The 35/35 regression passes; M4's battle-end, polish and packaged-player gates remain open. See [M4_POSSESSION_CYCLES_REVIEW.md](M4_POSSESSION_CYCLES_REVIEW.md).

The possession HUD also shows live guard UP/DOWN feedback from the controlled soldier's block state without polling or copied state. Its 35/35 regression passes, while rendered fit and the rest of M4.7 remain open. See [M4_GUARD_FEEDBACK_REVIEW.md](M4_GUARD_FEEDBACK_REVIEW.md).

First-person pitch is limited to ±80° only during possession; repeated exits and death restore the exact tactical camera bounds. The final 35/35 regression passes; physical mouse/rendered camera checks remain open. See [M4_PITCH_LIMIT_REVIEW.md](M4_PITCH_LIMIT_REVIEW.md).

M5.1 now has authored campaign adjacency by stable territory IDs, independent of visual actor positions. The nine saved territory assets preserve all 20 existing connections; the C-001 validation and real-PIE movement regression passed with 36/36 tests, strict IWYU, Shipping and Development cook. See [M5_ADJACENCY_REVIEW.md](M5_ADJACENCY_REVIEW.md).

The nine territory assets also now reference the existing tactical map through typed soft World references, checked without loading worlds. The C-002 full 37/37 tests, strict IWYU, Shipping and Development cook pass. Strategic UI and actual battle transitions still remain. See [M5_TACTICAL_MAP_REVIEW.md](M5_TACTICAL_MAP_REVIEW.md).

C-003 now initializes only Rome and Carthage as active participants. Gaul's content/territory remain authored but have no slice wallet, production or campaign AI stage. Player/diplomacy guards and real-PIE inactive-unit HOLD verification pass; latest full suite is 37/37 with zero test warnings/errors, strict IWYU and Shipping approved. See [M5_ACTIVE_FACTIONS_REVIEW.md](M5_ACTIVE_FACTIONS_REVIEW.md).

C-004 adds public stable-ID territory snapshots and ownership invalidation events through GameState, without UI actor enumeration or another gameplay owner. It preserves dormant-owner metadata, labels production as base, rejects unknown/not-ready/foreign-world reads and handles nested ownership/lifecycle changes. Latest full suite is 37/37 with zero test warnings/errors; strict IWYU and Shipping pass. See [M5_TERRITORY_PROJECTION_REVIEW.md](M5_TERRITORY_PROJECTION_REVIEW.md).

C-005 now renders nine region cards and read-only inspection in Strategic mode using the public GameState contract. Owner changes refresh by events, dormant/unowned regions are inspectable, and base production is explicitly distinguished from effective settlement yield. Full 37/37 is clean; strict IWYU and Shipping pass. Two real Slate/D3D12 captures were inspected and card overflow fixed; the visual test passes with one engine motion-vector warning. See [M5_STRATEGIC_UI_REVIEW.md](M5_STRATEGIC_UI_REVIEW.md). This is an abstract grid, not a geographic map or an army-action UI.

C-006 now owns live settlements and effective production in canonical stable-ID territory state, not map actors. The inspector shows real residents and separate base/effective yields; production survives actor destruction. Full 38/38 clean, strict IWYU and Shipping pass; render passes with the known engine warning. See [M5_SETTLEMENT_STATE_REVIEW.md](M5_SETTLEMENT_STATE_REVIEW.md).

C-007 implements full manual refunds from captured original payer/cost, queued promotion and failed-refund retry; C-008 exposes guarded mouse CANCEL+REFUND beside TRAIN. Both full regressions pass 38/38 clean; IWYU/Shipping pass. Two active/idle C-008 screenshots confirm button fit; render has only the known engine warning. See [refund authority](M3_TRAINING_REFUND_REVIEW.md) and [cancellation UI](M3_TRAINING_CANCEL_UI_REVIEW.md). C-009 verifies approved real-time possession: directed 1/1 clean, Editor/IWYU pass, runtime unchanged ([review](M4_REALTIME_POSSESSION_REVIEW.md)). C-010 failed-production refunds pass full38/38 clean and IWYU/Shipping ([review](M3_TRAINING_FAILURE_REFUND_REVIEW.md)). C-011 morale-zero routing passes full39/39 clean, physical withdrawal and IWYU/Shipping ([review](M3_ROUTED_STATE_REVIEW.md)). C-012 physical camp capture passes automatic directed1/1, full40/40 clean and IWYU/Shipping ([review](M3_OBJECTIVE_OCCUPATION_REVIEW.md)). C-013 closes terminal action/turn guards and possession cleanup with guarded real sandbox restart: directed2/2/full42/42 clean, IWYU70/Shipping19, inspected render2/2 with one known engine warning ([review](M3_TERMINAL_ENCOUNTER_REVIEW.md)). C-014 establishes canonical army IDs/inventory with real SaveGame value round-trip and inventory-driven bootstrap: directed4/4/full45/45 clean, IWYU72+7/Shipping9 ([review](M5_CAMPAIGN_ARMY_FOUNDATION_REVIEW.md)). This is not a disk save or campaign preserved across travel.

C-015A/B/C provide mapped outcomes and paid reinforcement/refunds; D1 defines their stable transfer/archive and D2 installs actual selected-map session travel. C-015D3 adds native entry, stable army selection/movement/battle, terminal Continue (including global victory) and quarantined retry controls, verified through actual Slate input, world travel and lifecycle checks ([session UI](M5_CAMPAIGN_SESSION_UI_REVIEW.md)). Directed5/full67 clean, rendered3/3 with six captures, strictIWYU81/Shipping12 pass. Next C-015E implements strategic recruitment/assignment and Carthaginian AI, then C-016 save/load. No disk save or trusted packaged-player journey is claimed. Start from [CLAUDE.md](CLAUDE.md) and [HANDOFF.md](HANDOFF.md), preserving uncommitted work. Producer-destruction refunds and remaining milestones/gates stay open.

C-015E1/E2 provide canonical six-cost recruitment/assignment and native event/query UI. E3A/E3B provide [deterministic AI planning](M5_CAMPAIGN_AI_PLANNING_REVIEW.md) and [canonical execution/real Roman defense](M5_CAMPAIGN_AI_EXECUTION_REVIEW.md), with accepted post-turn travel/retry without duplicate turn work. Final directed8/full83 clean, rendered2 successful (one known engine warning), five captures inspected; strictIWYU87+final4/Shipping25+final3 pass. Final layout-only no-wrap correction has separate build/render proof. Next C-016 local save/load; no disk-save, trusted packaged-player or whole-plan completion claim. Continue from HANDOFF, preserving uncommitted changes.

Tactical squad membership is now integrated with initial forces and recruitment, with atomic same-faction grouping and lifecycle cleanup. That baseline passed 26/26 without result warnings, strict IWYU and Shipping compilation. Handles are battle-local, not persistent campaign IDs; campaign mapping remains later work. See [M3_SQUAD_REGISTRY_REVIEW.md](M3_SQUAD_REGISTRY_REVIEW.md).

Navigation uses AIController, CharacterMovement and dynamic NavMesh in both maps. Real-PIE tests cover obstacle avoidance, pursuit, cancellation, possession reuse, lifecycle cleanup, disconnected islands, a gated narrow passage and crossing formations. The latest regression passes 34/34; [M3_NAVIGATION_REVIEW.md](M3_NAVIGATION_REVIEW.md), [M3_DISCONNECTED_NAVIGATION_REVIEW.md](M3_DISCONNECTED_NAVIGATION_REVIEW.md) and [M3_GROUP_CONGESTION_REVIEW.md](M3_GROUP_CONGESTION_REVIEW.md) record revision-specific evidence and remaining gates. [M3_PERFORMANCE_BASELINE_REVIEW.md](M3_PERFORMANCE_BASELINE_REVIEW.md) records provisional 50/100-unit editor profiling; it is not a packaged-player benchmark.

Multi-selection movement assigns distinct formation slots, rejects invalid footprints without replacing prior orders, and protects newer group commands from reentrant cancellation callbacks. Partial squad selections remain partial. That revision passed 29/29; [M3_FORMATION_REVIEW.md](M3_FORMATION_REVIEW.md) records its evidence.

The tactical selection panel now exposes STOP and HOLD for selected units. Its original 30/30 regression gate passed, and the latest expanded suite passes 34/34; [M3_SELECTED_COMMANDS_REVIEW.md](M3_SELECTED_COMMANDS_REVIEW.md) records live route, Slate click and lifecycle checks. A fresh packaged-player gate remains open.

Melee attacks now have one wind-up, one validated hit and one recovery clock for AI and direct control. The impact can miss or be cancelled without refunding recovery; [M3_COMBAT_WINDOWS_REVIEW.md](M3_COMBAT_WINDOWS_REVIEW.md) records the real-PIE and pure-state tests. Objective/death wiring and a fresh player check remain open.

C-016A defines the [versioned save archive](M5_CAMPAIGN_SAVE_ARCHIVE_REVIEW.md) and USaveGame record. Directed6 clean, strictIWYU88/Shipping pass; this is a data/archive contract only. Disk I/O, dedicated save subsystem, manual/autosave/load and native controls remain C-016B/C. Continue from HANDOFF rather than rerunning the audit.

C-016B adds [actual local persistence and validated world restoration](M5_CAMPAIGN_SAVE_RUNTIME_REVIEW.md): rotating Manual/Autosave generations, readback/backup/corruption/version handling, safe end-turn capture and queued-AI recovery without repeated work. Directed6/full90 clean; final GC-hardening directed6, strictIWYU and Shipping21 pass. Full90 preceded only final rooting/GC fixture/direct-include changes. Native controls/confirmation/feedback remain C-016C; no new renderer/cook/trusted-player/milestone claim. Continue from HANDOFF.

C-016C adds [native Save/Load/Autosave controls](M5_CAMPAIGN_SAVE_UI_REVIEW.md), confirmation, status and event-driven lifecycle refresh; actual sandbox resume and failed saved-load repaired retry preserve preferences/canonical state. Directed9 clean, final rendered9 successful (one known engine warning), ten captures inspected; strictIWYU91+final4/Shipping8+final3 pass. C-016D [combined first-playable journeys](M5_FIRST_PLAYABLE_JOURNEY_REVIEW.md) pass directed2 Victory/Defeat, including legitimate enemy counterattack before safe load. Final full95 verified clean; C017A follows below. No packaged-player/cook/whole-plan completion claim.

C017A adds a [registered-state native minimap](M6_MINIMAP_REVIEW.md): tactical living units/selection/objective/Pawn anchor, strategic territory ownership/positions/authored adjacency, lifecycle-safe active presentation and indexed movement sampling without global actor scans. Directed2/full97 clean; rendered2 successful (one known engine warning), two captures inspected; strictIWYU93/Shipping17/repository/LFS/diff pass. Historical proof; C017B follows below.

C017B adds the [responsive native shell](M6_RESPONSIVE_SHELL_REVIEW.md) with separated panels, full scrolling documents and readable possession/region text. Windows1280×720/1920×1080/2560×1440 at normal/125% pass actual geometry/input/physical-result cases: UI10/full103 clean, rendered6 successful (one known engine warning),30 PNG/seven inspected, strictIWYU94/Shipping9. C017C [retires unused HUD JSON/empty factories](M6_NATIVE_UI_RETIREMENT_REVIEW.md), standard-library Python3 checks pass without running Unreal setup. Material/audio progress follows below. Remaining M6–M8 and trusted packaged-player gates are open; [HANDOFF.md](HANDOFF.md) is ready for model switching.

C017D [material integration](M6_MATERIAL_ASSET_REVIEW.md) closes M6.4: canonical actor-local material states/selection, unused manager retired; native1/full104 clean, strictIWYU94/Shipping23/cook572 pass and actual cooked registry includes all12 materials. Final actual PIE player viewport and delayed orthographic capture pass per-object preview/stripe/construction/damage/selection pixel gates; PNGs inspected (one known engine warning each). Fix was real-frame capture-fixture latency, not an asset/runtime rewrite. Next audio C017E; whole M6–M8 and trusted-player gates remain open.

C017E [native audio feedback](M6_AUDIO_FEEDBACK_REVIEW.md) closes M6.5: six original synthetic waves/one SFX class, eight owned world-local voices, canonical selection/orders/construction/combat/physical victory/defeat, volume and actual PIE teardown/GC. Directed7/full108 clean, Python5/strict96/Shipping14/cook579 pass; actual cooked registry lists seven assets, hashes19 unchanged. Null-library sources/factories retired without binary deletion. Volume UI/persistence M6.8; no voice/score/subjective listening/trusted-player claim. Next C017F procedural retirement. Compact HANDOFF.md plus preserved HANDOFF_HISTORY.md support model switching.

C017F [retires the unused procedural helper](M6_PROCEDURAL_RETIREMENT_REVIEW.md), its module dependency and enabled plugin; working StaticMesh prototypes remain. Directed13/allPython13/Editor51/strict95/Shipping50/cook579 pass, actual cooked registry retains all19 material/audio assets with original hashes. M6.6 closed; next real skeletal characters/animation M6.7, not image-only models. Whole implementation goal remains open.

C017G [skeletal character integration](M6_CHARACTER_ASSET_REVIEW.md) is CLOSED for minimum offline art/animation: three original meshes/shared17bones/five clips/15assets/3GLBs, actual graph/montage/ownership/GC and inspected ortho/FPS/RTS. Full111/strict99/Shipping22/cook594 pass; actual cooked registry15characters+19audio/materials,37source hashes unchanged. One known engine warning per raster report; no trusted-player/multiplayer-per-view/whole-M6 claim. C017H [preferences/accessibility](M6_PLAYER_PREFERENCES_REVIEW.md) closes M6.8 minimum offline: live volume/sensitivity/text, native settings/pause/focus and actual EnhancedInput remapping29rows/33original mappings; sole local profile, physical capture/conflicts/defaults/truthful save, RTS/FPS/held-release and event-driven shortcuts. Full remap121success/exit0 (49clean/72only WASAPI) before final narrow modal cleanup; afterward ReleaseFinal6success (3clean/3WASAPI), final raster6success/text200% with only device/one engine MotionVector warning, no obsolete Slate-scroll warning, inspected actual binding-feedback PNGs. Strict109+4+9 and ShippingFinal5 succeed. CookRemapping2 exit0/594cooked/0incremental/7platform/601total,0errors/0warnings; actual engine-parsed Zen registry25input+15characters+7audio+12materials,37art/source+25accepted input hashes unchanged; repo/LFS/diff pass. First cook concurrency rejection retained in review, not SDK failure. All processes terminal. Next C018A/M7; no trusted-player/whole-M6/goal claim. HANDOFF holds exact next task.

C017I/M6.9 [editor-generation retirement](M6_EDITOR_GENERATION_REVIEW.md) CLOSED: five obsolete shell/spatial-migration scripts retired with exact SHA256 recovery copies; no binary/C++/config changes. Retained owned schema1 authors pass actual engine readonly twice each (six exit0 processes), native authored readback7clean/Python23/71unchanged hashes/repo/LFS/diff pass. [Supported authoring](Tools/ASSET_AUTHORING.md); no startup generation or rerun of migrations. Next C018B/M7 automation/release; no trusted-player/whole-goal claim.

C018A local automation/CI foundation verified: shared typed per-test/count/required-coverage report gate;25contract checks/real broken-invariant child exit1 with parent native exit restored0; final actual Test launcher121success (49clean/72only WASAPI), report20261007_203241_359 AFTER modal cleanup. Manual/protected-main-only workflow, read-only token/no persisted credentials/pinned SHAs/serialized UE/QA+Shipping package template, bounded schema1 metadata/path-escape rejection and release docs. YAML parse/PowerShell/prerequisites/repo pass; no remote runner/settings/branch policy/player launch claim. M7 open, see M7_AUTOMATION_FOUNDATION_REVIEW.md/CI_RUNNER_SETUP.md; next C018C.

C018B [current rendered load captures](M7_CURRENT_LOAD_CAPTURE_REVIEW.md) complete PROVISIONALLY: existing fixtures unchanged;3independent D3D12 processes per50/100added units (75/125total), six1success/exit0 with only known WASAPI/MotionVector notices. Actual240numeric CSVframes each, group-center projections50/50 and100/100/nonzeroGPU/nonempty traces; accepted CSV/trace hashes and per-run quantiles in Artifacts/Profiling/C018B/summary.json. Parser16pure checks; source71hashes/repo/LFS/diff pass. Game p95 ranges13.36–14.41ms and17.57–19.13ms; no agreed target/full combat/production/objective workload/trusted-player/pixel-visible-unit gate. No optimization/new runtime/uasset. M3.7/M7/goal remain open, next C018C.

C018C [dedicated functional maps](M7_FUNCTIONAL_MAP_REVIEW.md) verified: five persistent Editor-only fixtures reuse canonical navigation/combat/production/possession/occupation scenarios, with real FunctionalTest terminal outcomes and broken-assertion propagation. Actual full128clean/exit0,10coverage anchors, strict111+final5, Shipping/cook594+7platform601total/0errors/0warnings pass. Engine-parsed cooked registry excludes /Game/Tests/test-module refs and preserves required59production packages; fresh author rerun1clean/no writes,71prior+5fixture hashes unchanged. Read-only by default, explicit create-missing author; no runtime gameplay rewrite. Next C018D campaign phase gate +existing world-travel coordinator. M7/full encounter/render/performance/remote/trusted-player/release gates remain OPEN; HANDOFF.md holds current state.

C018D [campaign functional handoff](M7_CAMPAIGN_FUNCTIONAL_REVIEW.md) verified: sixth Editor-only map/source phase gate plus unchanged real cross-world journey; actor Success is not journey completion. Post-phase runtime-error capture and failed-gate/no-transfer negative tests pass. Actual full131clean/exit0 with13anchors, strict111, cook594+7platform601total/0errors/0warnings, actual all-test-map/module-ref exclusion and required59production packages, fresh author rerun1clean/no writes and77unchanged hashes pass. Unchanged runtime Shipping C018C reused; no trusted-player/remote/release claim. Compact HANDOFF.md and archived history ready, next C018E finished-encounter workload/traces/performance. M7/goal remain OPEN.

C018E [finished-encounter workload/performance](M7_ENCOUNTER_PERFORMANCE_REVIEW.md) locally verified:50/100 INITIAL TOTAL soldiers, natural recruitment/damage during training, real RTS/AI/FPS and physical victory. Six independent D3D12 captures at actual1920x1080, finite CSV/nonzeroGPU/phase counters and region-bounded native Insights exports with hashes; old C018B preserved. Actual full133success (129clean/4HTTP-timeout warning reports)/exit0,15anchors, strict111/exit0,39parser checks/PSASTs/repo/LFS/diff/77unchanged hashes pass. Performance remains provisional: host/Editor overhead/outlier recorded, no agreed FPS target/trusted-player/optimization/remote release claim. All processes terminal; next C018F rendering regression/reviewed baselines. HANDOFF.md ready for model switch; M3.7/M7/goal OPEN.

C018F [character rendering regression](M7_RENDERING_REGRESSION_REVIEW.md) LOCAL CLOSED: three agent-reviewed actual D3D12/1280x720 baselines, six independent comparisons within unchanged thresholds, real misuse/file-corruption/report rejection verified. Full136success/exit0 (52clean/84documented-warning reports),18anchors, strict112/exit0 and repo/LFS/diff/77source+3raster hashes pass. Functional bridge setup policy reset before each PIE, all errors/warnings retained and actor policy preserved; failed attempts recorded. No gameplay/assets/global change or auto commit. Next native HUD/200% UI regression family; explicit baseline commit, remote/trusted-player/release and M7/goal OPEN. HANDOFF.md governs the relevo.

C018F HUD/UI local regression CLOSED: [native UI review](M7_UI_VISUAL_REGRESSION_REVIEW.md). All84 original images inspected and copied byte-identically into a reviewed test family; two independent complete84 comparisons pass unchanged tolerances. Actual negative witnesses led to focused label/mode/footer/crosshair fixes without changing fonts, scrolling, aiming or canonical gameplay. Final post-fix137tests/19anchors, strict112 actions and Shipping3 actions pass; warnings retained. C018G [candidate secret/dependency gate](M7_SECURITY_DEPENDENCY_REVIEW.md) also passes locally, with redacted receipts and explicit coverage limits. HANDOFF.md records next gates. Agent/native/build evidence is not owner approval, a trusted player, remote CI, release or whole-M7 completion.

C018H [local QA packages](M7_LOCAL_QA_PACKAGE_REVIEW.md) VERIFIED: Development and
Shipping archives in `Artifacts/Packages/C018H_LocalQA1`, dirty metadata, native
container/content checks and SHA256 manifests. No player ran; clean candidate,
trusted runtime, isolated CI, owner budgets and release approval remain open.

### M1 baseline evidence

- Visual Studio Build Tools 2022 17.14, MSVC 14.44, .NET Framework 4.8 SDK, and Windows SDK 10.0.22621/26100 are installed and accepted by UE 5.8.2.
- The supported build path leaves Windows Smart App Control enabled and uses direct non-UBA compilation by default; full Development and Shipping rebuilds, IWYU, tests, cook, package, and smoke launch pass on that path.
- `/Game/Maps/L_Testbed` and `/Game/Maps/L_VerticalSlice` are created, assigned to the explicit framework classes, dynamically lit, validated, cooked, and covered by Git LFS rules.
- Two native automation tests pass; the M1 PIE stability run completed for more than ten minutes without an error, ensure, fatal error, or crash.
- The archived Development package reaches `/Game/Maps/L_VerticalSlice`; Development and Shipping receipts contain no MCP/toolset modules.
- Human review accepted the M1 baseline. Commit `0b74ad1566306d7487629812de68bc6abd7f0594` reproduced from a clean clone through LFS pull, validation, Editor/IWYU/Game builds, tests, packaging, packaged smoke, and player-receipt inspection. No remote push was performed.

Asset provenance and intake rules are in [ASSET_INVENTORY.md](ASSET_INVENTORY.md). Project licensing is defined by [LICENSE.txt](LICENSE.txt) and [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md).
