# M2 exit review — in progress

Reviewed 2026-09-08 against the implementation plan. M2 is **not closed**: its planned public `ARomanEmpireGameState` contract is still absent, and the latest packaged-player launch is blocked by application control. M3–M8 and the complete playable/save/restore loop remain outstanding.

## Corrections and evidence from this review

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

## Gate assessment

| Requirement | Current evidence / remaining work |
|---|---|
| Zero defaults, affordability, atomic spending | Native resource-rule tests plus this review's real placement/reentry/rejection/retry coverage. |
| Production | Existing live UMG test routes TRAIN through Slate to the selected barracks; input tests reject ineligible training. The later full tactical production loop remains M3. |
| Ownership, diplomacy, turn order | Existing canonical state-rule tests and live campaign/UMG tests; source retains one ownership registry and one ordered pipeline. Full upkeep, conflict policy and actual save serialization remain the documented M5 seams. |
| Diagnostic values match canonical queries | `RomanEmpireGame.UI.CanonicalEventDrivenPIE` checks resources, economic objective and diagnostic gold against the real wallet. |
| No duplicate wallets/turn/territory ownership | Repository checks plus inspected ownership: `AFactionManager::FactionResourcesMap`, `ACampaignManager::CurrentTurn`, `AWorldMapManager::CampaignTerritoryState`. A new public read contract must not add mutable copies. |
| Definition change reaches runtime and UI | New PIE test above; authored asset restored and disk hash unchanged. |
| Planned public GameState responsibility | **Open.** GameMode currently creates `AGameStateBase`; UI/controller access managers directly. Implement the planned public phase/turn/resources/match read contract and its consumer/tests wiring while retaining exactly one underlying authority per value. |
| Latest packaged-player startup | **Unverified.** Build/cook/archive pass, but Windows Code Integrity blocks this hash before startup. An Editor-hosted smoke cannot close this gate. |

The GameState row is an actual architecture gap, not permission to remove the responsibility-map requirement. The next implementation package addresses it, followed by a final M2 requirement audit before M3; the standalone execution limitation also remains explicit.
