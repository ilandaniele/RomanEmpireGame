# C-014 — Canonical campaign army identities and inventory

Date: 2026-10-05. Status: implemented and verified. This advances M5.2, not the full campaign/travel/save journey.

## Authority and contract

- `ACampaignManager` owns one `FCampaignArmyState`. Army GUIDs identify strategic instances; faction, stable territory ID and content-unit-ID/count stacks are values, not actor pointers or local tactical squad handles.
- Reflected SaveGame fields survive an actual in-memory SaveGame archive round-trip. Canonical GUID and unit-ID ordering produce deterministic bytes regardless of insertion order. This is schema/value evidence, **not** a saved slot, process reload or campaign surviving world travel.
- Pure registration/add/remove/relocation and whole-snapshot replacement validate shape, duplicate identities/content stacks, positive quantities and int32 overflow. Failed mutations/imports leave all prior armies intact; complete loss removes an empty army. Queries return detached ordered copies and unknown lookups reset output.
- Pure mutations do not decide affordability, active-faction participation, catalog existence, adjacency or battle phase. The current runtime exposes read-only army queries and uses validated prototype initialization; future player commands must add those authority checks before mutation, not call the pure value layer directly.
- New campaign setup preserves the existing 15 Roman/10 Carthaginian Legionary forces. It resolves the existing catalog and active factions and places each initial army at the first lexical currently owned territory; this adds no capital designation or recruitment economy rule. With the initial slice ownership this is Rome/Africa. A faction with no territory receives no initial army.
- GameMode materializes the canonical inventory into the existing camp positions/formations. Quantities and unit IDs are no longer separately hardcoded there. Created combat actors and squad GUIDs remain encounter-local; a later explicit deployment mapping must connect them to persistent armies before committing tactical casualties/reinforcements.
- GameState provides same-world/initialized detached queries and a unique detachable army invalidation relay, with no cached roster or UI counter. Campaign restart replaces identities and publishes committed state; owner/read-model teardown releases state/observers. Territory presentation actor destruction does not erase army values.
- Tactical deaths remain in tactical result until atomic campaign application. Prematurely decrementing strategic units from death callbacks would make return/retry application unsafe; the live test proves this separation.

## Verification

Directed **4/4**, zero result warnings/errors/pending, exit 0:

- `RomanEmpireGame.Rules.Campaign.ArmyState`: malformed/duplicate/overflow/partial-import rejection, detached reads, merge/removal/retirement/relocation, deterministic export and count boundary.
- `RomanEmpireGame.Rules.Campaign.ArmySerialization`: actual reflected SaveGame archive round-trip, deterministic bytes, decoded snapshot validated into a fresh owner and no UObject/actor fields.
- `RomanEmpireGame.Campaign.ArmiesPIE`: real two-army bootstrap, owned stable territory, materialized squad counts, disjoint army/squad GUIDs, current lookup/copy guards, strategic inventory surviving live casualty and presentation destruction, new campaign identities, committed event re-query and lifecycle observer cleanup.
- `RomanEmpireGame.Rules.PublicGameState.UninitializedReadContract`: empty/reset unknown/unready reads and exported pure API flags.

Report `Artifacts/Automation/C014_Targeted_Verified`, log `Artifacts/C014-Targeted-Verified.log`. Editor **49 actions** plus incremental test builds pass; strict IWYU **72 + 7 actions**, Shipping **9 actions** pass. Full regression **45/45**, zero result warnings/errors/pending, exit 0 (`Artifacts/Automation/C014_Full`, `Artifacts/C014-Full.log`). Repository validation (0 open M1 warnings), Git LFS fsck and diff checks pass.

Two fixture issues were corrected before the final directed run: copying a TArray element before adding its duplicate (UE rejects self-aliasing), and using whitespace-only as the invalid stable-ID case (the existing shared helper intentionally accepts nonempty names containing spaces). Runtime faction validation reuses the existing content helper. No outstanding runtime error is inferred from those fixture failures.

## Scope and next work

New runtime `Public/Campaign/CampaignArmyState.h`, `World/CampaignArmyState.cpp`; existing CampaignManager/GameMode/GameState integration. New `CampaignArmyStateTests.cpp`, `CampaignArmyProbe.h`; existing CanonicalUIIntegration/PublicGameState tests. No assets, config, plugins or module dependencies changed. No new render/cook/trusted-player evidence.

Next C-015: explicit encounter/squad-to-army/content mapping and atomic, idempotent casualty/ownership application. Cross-world campaign preservation, guarded strategic recruitment/movement/AI, full local save/load and Continue remain open. Do not equate the value archive test with those gates.

Skills used: `unreal-cpp-gameplay`, `ue-data-assets-tables`, `ue-testing-debugging`, `ue-project-context`; reflection-safe value ownership, stable catalog IDs, real public-contract PIE/archive tests and only the user-authorized context sections.
