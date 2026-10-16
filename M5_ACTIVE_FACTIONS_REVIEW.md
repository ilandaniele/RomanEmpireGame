# M5.1 — Two active slice factions (C-003)

Date: 2026-10-04 (America/Montevideo). HEAD remains `0726de2`; H-001/H-002 and C-001/C-002 changes are preserved. C-003 implements the active-participant policy, not the full campaign loop.

## Policy and authority

`AFactionManager` now initializes exactly Rome and Carthage from the existing content catalog. Both must exist before info/wallet maps are committed; missing required content leaves neither participant partially initialized. `IsFactionActive`, ordered `GetActiveFactions` and derived `GetAIFactions` query that same owner, with no stored second roster. Returned arrays are snapshots. The default player remains Rome; either active participant can be selected through the existing guarded API.

Gaul's authored faction and territory are preserved. Read-only Editor inspection of the nine saved PDAs found Rome owned by Rome, Africa by Carthage, Gaul by Gaul, and the other six unowned (`Artifacts/C003-Owners.log`, `SLICE_OWNER_OK regions=9 writes=0`). Making a polity inactive is not an ownership remap: Gaul stays a dormant authored owner, with no runtime wallet, player participation, campaign diplomacy or campaign AI stage. No faction or territory asset was deleted or saved in C-003. Discovery continues to expect three faction definitions.

Inactive/missing/invalid faction resource transactions reject with `InvalidFaction`, including zero-cost spending. Player changes reject without replacing the canonical player. Diplomacy getters return `Invalid` for nonparticipants; mutations return typed `InvalidFaction` without changing relationships. The active initial relationship is only Rome–Carthage War. The generic `FDiplomacyState` remains extensible and unchanged.

`ACampaignManager` skips dormant/unowned territories during production, so the retained Gaul territory cannot create a wallet or abort a valid turn. Faction AI iterates the deterministic derived AI roster. The existing prototype tactical movement in the campaign AI stage now also requires an active owner and excludes the current player rather than a hardcoded Rome. This does not add defend/recruit/attack strategic AI or globally disable sandbox actors' tactical simulation.

## Verification

- Editor initial build: 9/9; final build including the campaign-unit guard: 7/7. `Artifacts/C003-Editor.log`, `Artifacts/C003-Editor-Final.log`.
- Final targeted campaign/content/real-PIE tests: 6/6, zero test warnings/errors. `Artifacts/Automation/C003_Targeted_Final`, `Artifacts/C003-Targeted-Final.log`.
- Shipping: 5/5, `Artifacts/C003-Shipping.log`.
- Strict non-Unity/no-PCH: 67/67, `Artifacts/C003-IWYU.log`.
- Full regression: 37/37, zero test warnings/errors; `Artifacts/Automation/20261004_012636_663`, `Artifacts/C003-Full.log`.
- Repository validation, Git LFS fsck and diff whitespace pass. C-003 introduces no binary asset, config, module or dependency change.

The strengthened `CanonicalEventDrivenPIE` checks exactly two ordered active participants while Gaul remains discoverable, only Carthage in the Rome-player AI roster, all six absent/dormant/invalid IDs rejecting player/wallet/diplomacy changes, active-player roundtrip to Carthage deriving a Rome-only AI roster, preserved active war and unchanged Gaul ownership. A normally initialized extra soldier changes owner to Gaul and receives HOLD; a complete turn preserves that order and leaves Gaul resources zero, then the fixture is destroyed. Existing UI/GameState relay, training, ownership and turn assertions remain in the same real world. Pure campaign/diplomacy and content discovery tests also pass.

Skills used: `unreal-cpp-gameplay` kept scenario participation in its existing authority; `ue-testing-debugging` guided reflected public-contract PIE coverage without importing private runtime headers. No unverified MCP session was used.

`ue-project-context` records only the previously authorized framework/verification updates; no context regeneration or unrelated-section changes. All C-003 processes finished with exit 0. No commit, staging or push was performed.

## Remaining scope

M5.1 remains partial until strategic data-backed UMG is implemented. Next C-004 should expose the bounded stable-ID territory read/event contract through the existing public GameState for that UI, rather than letting UI enumerate visual actors or own territorial state. The current GameState exposes faction territory counts, but no territory row/neighbor catalog. Inspect only its read/relay lifecycle and the WorldMapManager/registry APIs needed for that projection.

Campaign armies/actions, actual strategic AI, battle travel/result application, victory refinements and save/load remain M5.2–M5.4. No fresh cook/package/player, rendered UI or whole-M5 completion is claimed for this runtime-only block. C-002's cook result remains historical; Windows Code Integrity limitations and the four unanswered design policies remain unchanged.
