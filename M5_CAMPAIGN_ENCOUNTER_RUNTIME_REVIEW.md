# C-015B — Runtime campaign encounter transaction

Verified 2026-10-05. C-015B is complete; C-015 and the full implementation plan remain open. No commits, new assets, configuration or dependencies.

## Runtime authority

CampaignManager owns the encounter coordinator and strategic armies; WorldMapManager retains canonical territory state. Guarded player movement permits only owned adjacent territories. Encounter preparation requires ready same-world owners, Strategic mode, active Rome/Carthage at war, owned attacker/source, authored adjacency, valid loaded content, a registered physical objective and no prior tactical encounter. All planned army/content stacks are materialized and explicitly bound to actual local squads. Failed staging destroys newly generated units/squads without changing campaign inventory or publishing a plan.

The default prototype bootstrap remains enabled for existing maps. A false bootstrap flag supplies a pristine Strategic arena with strategic camera framing; real PIE fixtures configure the actual spawned GameMode instance without modifying CDOs or assets. Pristine objectives can register before combat, but cannot capture until an encounter exists.

Application accepts only the canonical TacticalBattleSubsystem result, not caller-supplied casualties. Both army and territory values plus consumed identity commit before victory checks and deferred owner/army/encounter publication. All published observers read complete state. Reentry into apply, movement, ownership, settlement, turn and campaign reset is guarded. Replay is rejected without further events; new campaign and EndPlay reset transient mapping/ledger and detach GameState relays. No second mutable campaign owner was added.

## Verification

- Editor initial52 actions plus incremental test/runtime builds succeeded. Final directed7/7 clean, exit0: `Artifacts/Automation/C015B_Targeted_Verified`, `Artifacts/C015B-Targeted-Verified.log`.
- Directed coverage: pure encounter4, public GameState1, actual Campaign.EncounterVictoryPIE and EncounterDefeatPIE2. Real timer/overlap occupation, casualties, relocation/capture or retirement, canonical outcome, observer reads, replay, reentry, reset and teardown pass. Invalid mode/owner/war/content reject before deployment; a deliberately invalidated factory after the first actor verifies partial-spawn rollback and receipt-free inventory preservation.
- Full suite51/51 clean, exit0: `Artifacts/Automation/C015B_Full`, `Artifacts/C015B-Full.log`.
- Strict non-unity/no-PCH IWYU74 actions and Shipping22 actions succeeded: `Artifacts/C015B-IWYU.log`, `Artifacts/C015B-Shipping.log`.
- Repository validation0 open M1 warnings, Git LFS fsck and diff check pass. No new render, cook or trusted standalone player evidence. All build/test handles closed.

The saved graph really contains Rome↔Africa; Sicily starts unowned. Movement coverage gives Sicily to Rome through the real WorldMap command as controlled fixture setup, not a new neutral-occupation rule. The partial factory error is explicitly expected by the test; final reports are clean. Live deployment coverage uses the existing Legionary15/10 inventory; do not claim a rendered multi-content army journey.

## Remaining work at C-015B exit

C-015C must enroll paid reinforcements using an explicit producer receipt and stable army/content mapping, keeping tactical/strategic registration atomic and refunding failed enrollment. Current unmapped extra squads fail closed at campaign application. No minting armies from arbitrary tactical handles.

Campaign session/travel/return/Continue, paid recruitment and army-action UI, neutral occupation policy, deterministic Carthage AI, disk saves/load, producer-destruction refunds and remaining M6/M7 gates stay open. Committed maps still use the prototype flag by default; this block does not author a complete campaign entry flow or selected TacticalMap travel.

Skills influenced single-domain ownership, stable data IDs, canonical result validation, real public-contract PIE tests and context updates limited to authorized modules, subsystems and verification sections.
