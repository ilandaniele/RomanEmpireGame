# C-015E3A — Canonical campaign AI planning foundation

2026-10-05 (America/Montevideo), GPT, uncommitted. This is the planner/query foundation for M5.2, **not executed campaign AI**. C-015E3B must connect turn execution and real tactical defense before E3 can close. Existing [E1](M5_CAMPAIGN_ARMY_ACTIONS_REVIEW.md), [E2](M5_CAMPAIGN_ARMY_UI_REVIEW.md) and session gates are preserved.

## Implementation and policy

`FCampaignAIProposal` is a derived value, not a reservation, permission, saved action, roster or wallet owner. `RomanEmpireCampaignAI::Plan` takes detached canonical inputs, consumes no RNG and returns at most one proposal. Lexical territory/unit IDs and stable army GUIDs break ties regardless of input order.

Current defaults: recruit at most two units per turn, target settlement garrison 15, retain at least five troops when relieving from another settlement. These are slice policy constants, not new authored assets or UI limits.

Priority:

1. Relieve an owned threatened settlement using one adjacent owned army if it resolves the threat without exposing its source or emptying another settlement below the retained garrison.
2. Recruit at the first still-threatened owned settlement; if it cannot pay, hold rather than propose a speculative attack.
3. Attack an adjacent active-enemy force at war, only with valid tactical-map metadata and sufficient strength. Empty-force encounters are not yet supported by existing deployment contracts and are not silently autoresolved.
4. Rebuild other owned settlement garrisons below the target.

Recruitment reads filtered current unit options and all six authored cost fields. It degrades two to one if affordability or multiplication overflow requires it, rejects overfull existing armies and never creates an extra army to evade overflow. An invalid army GUID requests a future new garrison only when none exists; queries do not allocate identities. Strength/threat totals use 64-bit arithmetic. Invalid IDs, faction enums, duplicate armies/regions/options/neighbors, nonreciprocal/unknown adjacency, nonowned army locations, malformed army quantities or negative wallets/costs fail closed.

`ACampaignManager::GetCampaignAIProposal()` builds inputs from its current ArmyState, WorldMapManager ownership/settlement/graph, FactionManager wallet/war and the ready content registry's loaded factories/availability/map metadata. Only the actual ready Strategic session with active Carthage and Roman human player is eligible; sandbox, tactical and pending travel return None. No actor enumeration, UI polling, second state owner, module/dependency/config/production asset change.

## Verification

Final directed4/4 and full75/75 pass clean, exit0, zero warning/failure/pending (`Artifacts/Automation/C015E3A_Targeted_Final`, `C015E3A_Full`; logs `Artifacts/C015E3A-{Targeted-Final,Full}.log`). Editor54 and strictIWYU86+final4 succeed (`C015E3A-{Editor,IWYU,IWYU-Final}.log`); Shipping succeeds (`C015E3A-Shipping.log`). Repository validation, LFS integrity, whitespace and modified Markdown links pass. All processes terminal; no Unreal/Cmd/dotnet/CrashReportClient active.

Three pure tests cover all proposal kinds, retained source defense, priority, deterministic reordered/repeated reads, metadata/war/graph/identity guards, all six prices, affordability, free purchases, overflow and no mutation. `Campaign.AIQueryPIE` uses real entry and selected-map travel: current registry/settlement/army/wallet query, repeated unchanged roster/wallet/turn, live price and resource changes restored before travel, wrong mode and pending/actual tactical rejection. Temporary definition price and wallet fixture changes are memory-only; no asset writes or actual AI recruitment claims.

The initial duplicate-identity test crashed because `TArray.Add` was passed its own element by reference. A detached copy fixes the fixture; the production planner was not the crash source. No expected-message/global suppression masks it.

## Required next integration, not implemented here

- Replace the strategic session's prototype actor-movement AI with one canonical action in the existing AIActions stage after production; preserve intentional sandbox compatibility.
- Reuse/refactor E1's content/cost/candidate-roster+wallet transaction. Do not bypass turn guards or grant public commands arbitrary-faction mutation.
- Prepare attacks during the turn but perform selected-map travel only after the full pipeline/guards complete. Explicitly preserve/retry any failed transfer without repeating production, AI spending or increment.
- Separate attacker/defender from the human faction consistently. Current PrepareEncounterForTravel requires Roman attacker; transfer validation requires Plan.Attacker == PlayerFaction; physical occupation hardcodes Roman occupier/no living Carthage; tactical PlayerVictory currently means attacker win in result application. Changing one contract alone is insufficient.
- Cover incoming Carthaginian attack, Roman defense win/loss/draw, physical occupation, exact casualties/ownership/IDs, Continue, pending transfer/retry, deterministic turn order, resource publication/reentry and save-safe intent state.

No new render/cook, AI execution, inverse-defense journey, save/load, OS/player or full milestone proof. Optional user preference for defended AI attacks remains unanswered; real tactical defense is the recommended default, not an authorization blocker.

## Files

New `Public/Campaign/CampaignAIPlanning.h`, `World/CampaignAIPlanning.cpp`, `World/CampaignAIQuery.cpp`, `Tests/Private/CampaignAIPlanningTests.cpp`. Existing `World/CampaignManager.h`, `Tests/Private/CanonicalUIIntegrationTests.cpp`, plus this review/plan/README/handoff/context (only authorized module/framework/verification sections). No stage/commit/push.
