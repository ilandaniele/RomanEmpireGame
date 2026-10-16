# C-015D1 — Safe campaign transfer value contract

Verified 2026-10-05. C-015D1 is complete; C-015D/session travel and the full plan remain open. No commits, assets, configuration, dependencies or installed session subsystem.

## Contract and ownership

`FCampaignTransferSnapshot` is a versioned detached payload, not a new mutable campaign owner. Temporary `FCampaignTransferCandidates` stage complete wallet/player/diplomacy, territory/settlement, army, turn/outcome/threshold/autosave, RNG and encounter-checkpoint values. Typed validation changes no output on failure. Territories rebind against current definitions/adjacency with exact registered-ID coverage; definitions and presentation actors are not serialized. Faction wallets match the supplied active participant set, while dormant authored territory owners remain dormant. Army identity/quantity/faction/territory ownership and normalized non-neutral diplomacy are validated atomically.

Encounter export permits only an unbound frozen deployment or a consumed/empty coordinator. Allocated local squads cannot be travelled as a fresh battle. Restoration revalidates the entire pending plan against current army/territory values and preserves encounter/paid-receipt replay ledgers, without local squad bindings. A consumed outcome exports no pending plan; later deployment cannot replay the old outcome or paid receipt.

The payload has Strategic or TacticalDeployment phase and top-level map-path shape checks. Deployment requires active player/defender at war and a valid unmaterialized plan. Actual World metadata, map availability, unit content and runtime lifecycle/readiness remain required at the D2 integration boundary; shape validation is not evidence that a map exists. Native tests deliberately use synthetic paths without loading assets.

Both RNG seeds are explicit. UE5.8's reflected FRandomStream.Current Seed lacks SaveGame metadata; restoration writes the engine-declared reflected Seed field, without assuming private memory layout. Initial seed, current seed, Reset semantics and the next100 samples survive the real SaveGame value archive. Player-entered city names serialize as stable FString and reconstruct as live FText: Editor persistence otherwise generates a fresh localization GUID without a package linker. Authored localized definition text remains in the catalog and is not changed.

## Verification

- Editor55+9+34+4 actions succeeded: `Artifacts/C015D1-{Editor,TestsBuild,StableTextBuild,FinalTestsBuild}.log`.
- Final directed4/4 clean, exit0: `Artifacts/Automation/C015D1_Targeted_Verified`, `Artifacts/C015D1-Targeted-Verified.log`.
- TransferDomains: malformed/missing/duplicate/foreign ownership and settlement rows, negative/overflow yields, exact coverage, detached queries, preserved graph/base production, diplomacy symmetry/atomic import/neutral normalization.
- TransferValidation: schema/phase/map shape, participants/wallets, domain coverage, army ownership/shape, checkpoint ledgers and turn-state guards; every failed build preserves all staged domains. Terminal outcomes at MaxTurns+1 are retained if resolved before the final increment.
- TransferEncounter: frozen deployment restore, stale/malformed/consumed plan guards, no local-squad export, consumed result/receipt ledger return, later encounter and cross-transfer replay rejection.
- TransferArchive: actual reflected SaveGame archive with all nested fields, deterministic repeated bytes, valid restored payload and exact RNG continuity. This is a value archive, not a file/USaveGame slot, corruption envelope or migration implementation.
- Full58/58 clean, exit0: `Artifacts/Automation/C015D1_Full`, `Artifacts/C015D1-Full.log`. Strict IWYU76 and Shipping28 actions succeeded: `Artifacts/C015D1-IWYU.log`, `Artifacts/C015D1-Shipping.log`. All build/test handles closed.
- Repository validation0 M1 warnings, LFS fsck and diff check pass. No new PIE travel scenario, rendering, cook or trusted standalone player claim.

Initial targeted3pass/1fail found non-deterministic city FText keys; the engine source confirmed the cause and the corrected stable-name payload passes. Gameplay/data/testing skills shaped reflection, single-domain ownership and all-or-nothing staged restoration; context updates are restricted to the user-authorized sections.

## Next — C-015D2 runtime session/travel/return

Install a GameInstance-lifetime transfer coordinator with no duplicate live gameplay values. Capture canonical owners only at a guarded safe boundary, validate actual selected territory TacticalMap/content, and preserve/reconcile all prepaid producer receipts before teardown. A failed refund/validation must leave the source World and receipt intact, never initiate travel or silently discard reservations.

Split validated encounter preparation from materialization: export the unbound plan, travel through the real selected map, restore all existing owners before GameState readiness, then materialize new local bindings. Consume/reset the transfer capsule after a successful import. Apply only the canonical tactical result once, settle/preserve terminal training orders, and perform actual Strategic return/Continue. Preserve intentional prototype defaults/tests until a real campaign entry command/UI exists. Test actual OpenLevel, repeated departure/return, source guards/failure, complete first observer reads and preserved state; build/cook alone is not that proof.

Army command/recruitment UI, deterministic Carthage AI, disk save/load, producer-destruction refunds and M6/M7 gates remain after this foundation.
