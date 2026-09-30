# M3.5 initial Roman barracks

Status: the Roman camp's decorative barracks mesh has been replaced by one real, initially completed `ABarracks` producer. M3.5 is still open for the enemy objective structure, production cancellation/failure policy, injected refund failure coverage and a trusted packaged-player check.

`ARomanEmpireGameMode` resolves the authored `Barracks` definition and its factory class, deferred-spawns the building at the existing camp site, initializes its stable ID and Rome ownership before `BeginPlay`, and completes its starting construction without charging the player or advancing simulation time. Runtime placement still uses the normal construction path. The new `CompleteInitialConstruction` entry point is restricted to an initialized, begun-play building in the Placing state, so completion notifications and visuals use the existing canonical transition.

The pre-existing production and UI integration test now verifies exactly one real startup barracks at the camp, Rome ownership, completed state, selection through the controller and a visible native UMG TRAIN button. It also asserts the old decorative static mesh is absent. Existing live Slate TRAIN tests cover data-defined cost/duration, one active order plus a bounded queue, exact charge, queue promotion without recharge, unfunded/full-queue rejection, and stale/foreign-selection guards.

## Evidence

- Targeted `RomanEmpireGame.UI.CanonicalEventDrivenPIE` 1/1 (`Artifacts/Automation/M3_InitialBarracks_UI`) and placement regression `DefinitionPropagationAndAtomicPlacementPIE` 1/1 (`Artifacts/Automation/M3_InitialBarracks_Placement`). Logs: `Artifacts/M3_InitialBarracks-UITargeted.log`, `Artifacts/M3_InitialBarracks-PlacementTargeted.log`.
- Full regression: 34/34, zero test warnings/failures, `Artifacts/Automation/20260930_124558_592`, log `Artifacts/M3_InitialBarracks-Full.log`.
- Strict non-unity/no-PCH IWYU 63/63 actions; Shipping Game 6/6 actions; repository validation, LFS integrity and diff whitespace checks pass. Logs: `Artifacts/M3_InitialBarracks-IWYU.log`, `Artifacts/M3_InitialBarracks-Shipping.log`.

No new asset, module, plugin or service was added. The Carthaginian HQ remains decorative until its M3.6 capture/objective contract is defined. No packaged-player claim is made; the previously rejected unsigned executable was not retried. Skills applied: `unreal-cpp-gameplay` for deferred initialization and a single construction transition, `ue-testing-debugging` for real-PIE startup and UMG assertions.
