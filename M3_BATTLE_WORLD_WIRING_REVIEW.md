# M3.6 live battle-roster wiring

Status: the pure tactical evaluator now has one world-local runtime owner and receives the prototype's known starting squads, trained singleton reinforcements and real unit deaths. Objective capture, result projection to UMG/campaign and restart/continue are still open.

`UTacticalBattleSubsystem` owns one `FTacticalBattleState` per Game/PIE world. During deterministic GameMode bootstrap it validates the two registry squads and their live faction members before starting the `CarthaginianCamp` result with 15 Romans and 10 Carthaginians. It binds unit death delegates and retains each unit's original world-local squad handle before the unit registry removes dead members. A barracks completion registers a newly trained singleton after squad creation. Death and reinforcement transitions publish one typed result event; nested consumers must re-query the snapshot. Teardown unbinds every remaining live actor and resets the state.

The subsystem exposes a capture transition only for the matching objective ID and player faction. It does not itself scan a capture radius, accept a UI click as capture, or claim victory; a future objective actor must validate the chosen spatial/destruction rule first. Campaign outcome remains owned by `ACampaignManager`, and the battle result's `FGuid` squad handles still require an explicit M5 campaign mapping.

## Evidence

- Real-PIE `RomanEmpireGame.UI.CanonicalEventDrivenPIE` targeted 1/1 (`Artifacts/Automation/M3_BattleWorld_Death`), log `Artifacts/M3_BattleWorld-DeathTargeted.log`. It verifies normal startup's two typed 15/10 squads, stable objective ID, an actual barracks-trained Roman singleton entering the result, and a real defender death becoming one casualty/nine survivors without prematurely ending the battle.
- Full regression 35/35, zero warnings/failures, `Artifacts/Automation/20260930_133243_819`, log `Artifacts/M3_BattleWorld-Full.log`.
- Strict non-unity/no-PCH IWYU 66/66 actions, `Artifacts/M3_BattleWorld-IWYU.log`; Shipping Game compilation 6/6 actions, `Artifacts/M3_BattleWorld-Shipping.log`. Repository validation, LFS integrity and diff whitespace checks pass.

No asset, plugin, module or external service was added. The end-to-end capture/victory sequence and packaged-player evidence remain unproven. Skills applied: `unreal-cpp-gameplay` for the world/subsystem lifecycle and event ownership, `ue-testing-debugging` for live-PIE roster and death assertions.
