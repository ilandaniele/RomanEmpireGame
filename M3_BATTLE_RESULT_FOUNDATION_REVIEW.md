# M3.6 tactical battle-result foundation

Status: the typed tactical result and one pure victory evaluator exist and are verified. The later [live roster wiring](M3_BATTLE_WORLD_WIRING_REVIEW.md) connects known squads, training and deaths; an authored capture objective, campaign resolution and restart/continue UMG remain open, so M3.6 is not closed.

`FTacticalBattleState` accepts an objective stable ID, distinct player/enemy factions and a validated, known starting roster. Setup rejects duplicate/invalid world-local squad handles or a missing force atomically. Reinforcements can enter as new squads. Casualties update survivors and losses exactly once, with overkill/unknown/resolved events rejected. Loss of every player unit resolves defeat; eliminating all defenders alone does not imply capture. A valid player capture resolves victory once. `FTacticalBattleResult` carries the outcome, winner, objective/captured-objective IDs and ordered per-squad initial/surviving/casualty counts. Its `FGuid` squad handles are local battle references, not save/campaign identities; M5 must map them explicitly.

The state is pure rules code: no world scans, gameplay actor, second wallet, campaign outcome or UI mirror was added in this foundation block. The subsequent world coordinator now owns one instance and supplies death/reinforcement events. The user's choice of occupation versus destruction capture mechanics remains pending; this result contract works with either.

## Evidence

- `RomanEmpireGame.Rules.Battle.TacticalResultAndVictory` targeted 1/1 (`Artifacts/Automation/M3_BattleResult_Final`), log `Artifacts/M3_BattleResult-FinalTargeted.log`. It covers invalid/duplicate setup, known 15-versus-10 forces, reinforcement, exact casualty/overkill behavior, no victory from kills alone, one capture, terminal immutability, reset and player defeat.
- Full regression 35/35, zero test warnings/failures, `Artifacts/Automation/20260930_131745_536`, log `Artifacts/M3_BattleResult-Full.log`.
- Strict non-unity/no-PCH IWYU 65/65 actions, `Artifacts/M3_BattleResult-IWYU.log`; Shipping Game compilation 4/4 actions, `Artifacts/M3_BattleResult-Shipping.log`. Repository validation, LFS integrity and diff whitespace checks pass.

No new asset, module, plugin or external service was added. This test does not prove a live victory sequence or packaged-player result. Skills applied: `unreal-cpp-gameplay` for a single tactical state owner and typed public contract; `ue-testing-debugging` for pure boundary and terminal-state assertions.
