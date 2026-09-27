# M3.2 building-selection ownership

Status: building-selection ownership implemented and verified (2026-09-27). Follows live-unit registry commit `602438f`; this is not all of M3.2.

## Boundary

- Controller `IsPlayerBuildingEligible` combines the canonical non-None player faction, initialized public state, same-world actor and building readiness. Selection and player TRAIN share it; actor-level AI production remains separate.
- Building `IsSelectionReady` requires successful initialization, BeginPlay, positive health, a placed (not Placing/Destroyed) state and no actor teardown. Constructing, Complete and Damaged buildings can be selected if owned.
- Selection observes the existing committed building-state event (including ownership), player-faction changes and destruction. EndPlay publishes non-readiness before callbacks, covering level removal even without Actor OnDestroyed. Cleanup remains valid outside Tactical.
- Entering placement mode notifies a changed state, retiring an existing selection. Ended/destroyed actors cannot publish fresh ownership changes or reenter placement through those commands.
- UMG re-queries current controller selection and shared eligibility rather than trusting a stale event payload; training still requires Tactical/active-campaign state, completion and affordability. No new polling or duplicate owner state.

No input mapping, asset, module dependency, plugin, engine or security changes. Skills: `unreal-cpp-gameplay` for lifecycle boundaries, `ue-ui-umg-slate` for event-driven UI, `ue-testing-debugging` for real PIE regression, `ue-module-build-system` for reflection/build verification.

## Verification

`RomanEmpireGame.Buildings.PlayerSelectionPIE` uses the real controller, world lifecycle, canonical faction manager, UMG and Slate TRAIN button. It covers owned construction/completion, enemy/unowned/placing/deferred/foreign/ended/destroyed rejection, immediate ownership/player-faction/placement cleanup, nested selection notifications, old observer release, Strategic-mode cleanup and controller teardown. Queued UI callbacks after invalidation must preserve resources and production state; direct enemy training remains allowed.

Initial Editor compilation passed. The first targeted test exposed two fixture assumptions: the controller's native TRAIN method is not reflected, and the canonical manager rejects `SetPlayerFaction(None)`. The corrected test invokes the existing reflected UMG handler and switches to Carthage with canonical readback. Historical failed report: `Artifacts/Automation/M3_BuildingSelection_Targeted`; no runtime API was added merely to satisfy the fixture.

- Corrected targeted test: 1/1, zero result warnings/failures/pending, exit 0. Report `Artifacts/Automation/M3_BuildingSelection_FixtureFixed`; logs `Artifacts/M3_BuildingSelection-FixtureFix-Editor.log` and `Artifacts/M3_BuildingSelection-FixtureFixed.log`.
- Negative control: removing only faction equality from the shared eligibility query makes `PlayerSelectionPIE` fail on unowned/enemy selection and loss-of-ownership cleanup. Report `Artifacts/Automation/M3_BuildingSelection_OwnershipMutation`; log `Artifacts/M3_BuildingSelection-Mutation.log`. Restored before final regression; no temporary mutation remains.

- Restored full regression: 24/24, zero result warnings/failures/pending, exit 0 (`Artifacts/Automation/20260927_024720_507`, `Artifacts/M3_BuildingSelection-Final-Tests.log`). Full coverage follows the targeted test because these lifecycle callbacks also affect existing UMG, placement and world teardown tests.

- Strict non-unity/no-PCH IWYU: 60/60 actions, succeeded (`Artifacts/M3_BuildingSelection-IWYU.log`). Shipping: 6/6 actions, succeeded (`Artifacts/M3_BuildingSelection-Shipping.log`). Repository validation, Git LFS and diff checks pass.

This block does not claim a new rendered/physical-input/package gate; it changes no assets or rendering layout. The previous registry package remains historical evidence for its own binary, not the new build.

## Remaining scope

Squad identity/membership, additive selection and invalid-target feedback remain M3.2 work. Routed design remains M3.1 work. Production queue behavior on capture, damage/repair transaction reentry and refunds remain M3.5 design/implementation work; a selection guard does not certify those systems.
