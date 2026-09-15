# M2 exit review — complete

Reviewed 2026-09-08–15 against the implementation plan. **M2 is complete**: the public `ARomanEmpireGameState`, final audit corrections, native tests, strict compilation, Shipping and actual packaged-player startup are verified. The host-specific optional Fab workaround is documented below. M3–M8 and the complete playable/save/restore loop remain outstanding.

## Initial placement review (2026-09-08)

`UBuildingPlacementComponent::ConfirmPlacement` previously allowed synchronous wallet/placement observers to call it again before its preview state was cleared. One spend call in the source did not prove exactly-once execution. The component now guards reentrant confirmation/start/transform commands, snapshots the command, initializes its deferred building before spending, and clears consumed preview state before publishing `OnBuildingPlaced`.

Cancellation during the committed wallet notification can clear the preview, but cannot undo the already accepted spend or start a second command. The initialized building uses its captured identity. Insufficient funds destroy the deferred candidate without spending and leave the original preview retryable. Spatial/footprint validation remains the later tactical-placement task; this review does not certify it.

The Editor-only test `RomanEmpireGame.Content.DefinitionPropagationAndAtomicPlacementPIE`:

- Loads the real Barracks Primary Data Asset, temporarily changes its name, all six costs, hit points and construction duration, then starts normal `L_Testbed` PIE.
- Verifies the canonical registry resolves that asset, the native build menu uses the edited name/costs, the real placement factory consumes those values, construction crosses the edited time boundary, and selected-building UMG displays the edited name.
- Binds signature-checked test observers to the real resource/placement delegates. Recursive confirmation and cancel/restart attempts produce exactly one charge, one placed event and one actual building. Every currency is checked independently.
- Exercises a real insufficient-gold rejection, unchanged complete wallet, no leaked deferred actor, and successful retry after funding.
- Restores the complete definition and its original dirty state after PIE, including cleanup paths. It never saves the asset or map. This proves definition propagation at normal startup/spawn, not live editing of existing actors or a packaged hot-reload feature.

Mutation check: disabling only the confirmation recursion guard made the new test fail (`Artifacts/Automation/20260908_151313_441`, 13 passed / 1 failed). It observed 74 gold deducted instead of 37 and two buildings instead of one, with duplicated charges in all six currencies. The mutation was restored before the final run.

Final corrected suite: `Artifacts/Automation/20260908_151420_304`, **14 passed, 0 failed, 0 not-run, 0 in-process**. `Artifacts/M2_Exit-Tests.log` records the Development Editor build and execution. Strict non-unity/no-PCH compilation also passed (`Artifacts/M2_Exit-IWYU.log`). Repository validation passed. Barracks asset SHA256 remains `d54a3f10d27dd916e8abdd058573cebda51b1943616e968c6798b1f89779239c`, matching its committed LFS payload.

Shipping Game compilation passed (`Artifacts/M2_Exit-Shipping.log`). Development BuildCookRun built Editor/Game, processed 566 packages including platform skips, and archived 559 packages to `Artifacts/Packages/M2_Exit`, with 0 cook errors/warnings (`Artifacts/M2_Exit-Package.log`, UAT exit 0). Both Game receipts exclude `ModelContextProtocol`, `EditorToolset`, `AutomationTestToolset`, `AllToolsets` and `RomanEmpireGameTests`.

The actual archived player SHA256 is `96f19b3621b0483457ae1788c2a5f4a82bae5483254e8ab07e21624b598d4650`. Windows rejected its launch before Unreal started: Code Integrity events 3033/3077 at 2026-09-08 15:20:46–47 (America/Montevideo), policy `{0283ac0f-fff1-49ae-ada1-8a933130cad6}`, explicitly name the `M2_Exit` player path. No player log or screenshot was created. No security policy was changed; signing or a controlled runner that trusts the build remains necessary for this hash's standalone launch gate. The successful M2.6 player smoke is historical evidence, not proof for this new binary.

The signed Editor-hosted `-game` fallback initially rendered and exited 0, but logged 15 Python error lines from the engine's EditorToolset/ToolsetRegistry startup (`AgentSkill`/`PythonTestRunner` are unavailable in this game-mode host). That run is retained as `Artifacts/M2_Exit-EditorGame-Smoke.log`, not labeled clean. A separate process with `-DisablePlugins=ModelContextProtocol,EditorToolset,AutomationTestToolset,ToolsetRegistry` loaded `L_VerticalSlice`, production UMG and authored input, rendered the inspected 1920×1080 `Artifacts/M2_Exit-EditorGame-NoToolsetsUMG.png`, and exited 0 with zero error/fatal/assert/ensure signatures (`Artifacts/M2_Exit-EditorGame-NoToolsets-Smoke.log`). The flag is supported by the installed engine's `PluginManager.cpp`; no project/engine configuration was edited. This is a short editor-hosted runtime check, not standalone or sustained gameplay certification. No editor/player process or port-8100 listener remains.

Skills applied: `unreal-cpp-gameplay` for reflected command/lifecycle boundaries; `ue-data-assets-tables` for definition-driven consumers; `ue-testing-debugging` for scoped PIE fixtures and mutation-proven assertions; `ue-module-build-system` for strict compilation; `unreal-packaging` for distinguishing build/cook success from actual-player execution.

## Public GameState and final audit corrections

`ARomanEmpireGameMode` now deliberately creates `ARomanEmpireGameState`. This exported, Blueprint-pure single-player contract delegates readiness, mode, turn, player faction, all faction resources, objective/outcome state and territory counts to their existing owners. It introduces no mutable wallet, turn, mode or victory copy and makes no multiplayer replication claim. Unique subscriptions are established regardless of startup ordering and all ten authority relays are removed during EndPlay.

Production UMG and controller eligibility/input-context reads consume this public contract. Commands still go through their existing owners; selected-actor events stay on the relevant actors. UMG owns no gameplay counters, performs no polling or world scans, and reattaches through its normal lifecycle. Public event payloads are notifications, not guaranteed newest snapshots: observers may issue synchronous nested commands, so presentation re-queries current state.

Additional automated evidence:

- `Rules.PublicGameState.UninitializedReadContract` verifies safe empty queries and all nine pure/const reflected getters without authorities.
- `UI.CanonicalEventDrivenPIE` compares native GameState queries to all canonical owners after real mutations, across all six resource fields and all faction enum values. It checks all ten relay signatures/subscription counts and removal on EndPlay, with no later forwarding.
- Its real Slate TRAIN clicks now compare before/after balances to the unit PDA cost in every resource, for both the initial active order and the queued click after UMG reattachment. Completion/queue promotion, insufficient funds and a full queue charge nothing; each accepted queue entry charges exactly once. Producer tick is disabled between latent frames to keep assertions deterministic.
- `UI.NestedObjectiveTransactionsPIE` inserts a real wallet-mutating observer before the reattached UMG observer. Two committed transactions produce nested public objective events. Immediate visible text must match `GetObjectiveState()` without ticking or forced refresh. The observer is removed and the wallet restored before PIE teardown.

Mutation control: temporarily restoring only the old objective callback made the new test fail, displaying 1117 gold instead of the canonical 1146 (`Artifacts/Automation/20260909_164228_700`, 15 successes / 1 intended failure). The fix was restored; `Artifacts/Automation/20260909_164333_175` then passed **16/16**, with zero warnings, failures, not-run or in-process tests (`Artifacts/M2_GameState-Restored-Tests.log`).

`BuildProject.ps1` now includes successes-with-warnings in its reported total and rejects unfinished reports. Six in-memory checks of the real parser function covered clean success, warning-only success, no successful execution, failed tests, not-run tests and in-process tests. A prior 15-test run had three audio resampling warnings; those remain historical, not omitted from its total.

The intermediate GameState package also passed an actual standalone rendered launch on 2026-09-08: SHA256 `cb691543458fe17a81fcad9fab92cd43dd1ac2aa66413814a693ffba6ef2d518`, exit 0, default map, public GameState, production UMG and authored input ready, no error/fatal/assert/ensure signatures. `Artifacts/M2_GameState-PackagedUMG.png` was inspected at 1920×1080. This does not certify the subsequent objective-callback correction; the final package is verified separately below. No application-control setting was changed.

Skills influencing this package: `unreal-cpp-gameplay` for the public reflected lifecycle contract, `ue-ui-umg-slate` and `ue-input-system` for consumer ownership, `ue-testing-debugging` for real PIE/Slate and mutation controls, `ue-module-build-system` for strict compilation, and `unreal-packaging` for actual-player verification.

## Final binary gates (2026-09-15)

- Strict non-unity/no-PCH Editor build and Shipping Game passed: `Artifacts/M2_GameState-Final-IWYU.log` and `Artifacts/M2_GameState-Final-Shipping.log`.
- The first final package attempt failed before content cooking because Windows Code Integrity rejected the updated optional engine Fab 0.0.17 module. Events 3033/3077 at 13:16:43 (America/Montevideo) name `Engine/Plugins/Fab/Binaries/Win64/UnrealEditor-Fab.dll`, policy `{0283ac0f-fff1-49ae-ada1-8a933130cad6}`. DLL SHA256: `ccca6011663f1a8af6176879ba846d5a373b9eefb0a1d83a481c159a9d1f4b7d`; its signature check reports NotSigned. This is recorded in `Artifacts/M2_GameState-Final-Package.log`, not labeled successful. The previously tested Fab was 0.0.15; engine files were not edited to undo that external update.
- The game has no Fab module/content dependency. `BuildProject.ps1 -DisableFab` is an explicit opt-in for Test/Cook/Package, forwarding `-DisablePlugins=Fab` only to the launched Editor process (`-AdditionalCookerOptions` for UAT). Neither project/engine plugin defaults nor Windows security policy changes. Interactive Fab remains a separate trusted-installation issue; omitting this optional browser is not a repair or security-policy bypass.
- `./BuildProject.ps1 -Action Test -DisableFab` passed **16/16**, with zero warnings, failures, not-run or in-process entries: `Artifacts/Automation/20260915_131817_133`, `Artifacts/M2_GameState-Final-NoFab-Tests.log`.
- `./BuildProject.ps1 -Action Package -DisableFab -Configuration Development -ArchiveDirectory Artifacts/Packages/M2_Final` passed UAT with exit 0: 559 cooked packages, 7 platform skips, 566 total, zero cook errors/warnings. Evidence: `Artifacts/M2_GameState-Final-NoFab-Package.log`. Development/Shipping receipts exclude MCP, editor toolsets, the test module and Fab's Editor DLL.
- The actual archived `Artifacts/Packages/M2_Final/Windows/RomanEmpireGame/Binaries/Win64/RomanEmpireGame.exe` launched without any editor/Fab override and exited **0**. SHA256: `00c8e7f72bc8a5023eb1cfc0c7a87935f90e58f672346143cb55f4a5db1c4d17`. `Artifacts/M2_Final-PackagedRendered-Smoke.log` proves the default `L_VerticalSlice` map, public GameState readiness, production UMG, authored input, normal shutdown and zero error/fatal/assert/ensure signatures. The 1920×1080 `Artifacts/M2_Final-PackagedUMG.png` was inspected; wallet, objectives and diagnostic values agree and control help is legible.
- Repository validation passes; the Barracks asset hash still matches its committed LFS payload. No editor/player process or loopback MCP listener remains. This short startup smoke is not sustained gameplay/GPU/performance certification.

## Gate assessment

| Requirement | Current evidence / remaining work |
|---|---|
| Zero defaults, affordability, atomic spending | Native resource-rule tests plus this review's real placement/reentry/rejection/retry coverage. |
| Production | Live Slate TRAIN verifies the exact PDA cost in all six resources, active and queued acceptance, prepaid promotion, and no-charge insufficient-funds/full-queue rejection. The later tactical production/placement loop remains M3. |
| Ownership, diplomacy, turn order | Existing canonical state-rule tests and live campaign/UMG tests; source retains one ownership registry and one ordered pipeline. Full upkeep, conflict policy and actual save serialization remain the documented M5 seams. |
| Diagnostic values match canonical queries | Live UMG/PIE checks and native GameState queries agree with owners; the nested-objective regression is mutation-proven. |
| No duplicate wallets/turn/territory ownership | Repository checks plus inspected ownership: `AFactionManager::FactionResourcesMap`, `ACampaignManager::CurrentTurn`, `AWorldMapManager::CampaignTerritoryState`. GameState delegates without mutable copies. |
| Definition change reaches runtime and UI | New PIE test above; authored asset restored and disk hash unchanged. |
| Planned public GameState responsibility | Implemented and directly tested: explicit framework assignment, public queries/relays, UMG/controller consumers, safe defaults and complete subscription cleanup. |
| Latest packaged-player startup | Passed for the exact final hash above: actual standalone executable, rendered screenshot, clean log and exit 0. Earlier successful/blocked hashes remain historical. |

An independent source/test review found no further functional M2 gaps after these corrections; the final binary gates above are now also satisfied. M3.1 is the next implementation package. Navigation, combat, placement geometry, persistence, art and sustained performance are not certified by this M2 review.
