# C-015E1 — Strategic recruitment and army assignment authority

Date: 2026-10-05. Implements the canonical backend for M5.2 recruit/assign, without a new gameplay owner. UI/AI and the rest of C-015 remain open.

## Minimum strategic policy

- Recruitment immediately purchases strategic quantities at an owned settlement. It uses the unit definition's exact six-field `TrainingCost` multiplied by the requested positive count, and current faction availability/factory validation. No tactical actor, barracks queue, prepaid receipt or second inventory is created. Authored `TrainingTime` continues to govern tactical barracks, not this discrete strategic purchase.
- A specified existing army must be owned and already in that settlement's region. An invalid destination GUID explicitly requests a fresh canonical army with a newly generated persistent ID; queries do not reserve IDs. Purchases never silently relocate an army.
- Assignment transfers a chosen content/count between two distinct, existing, same-faction armies in the same owned region. It costs nothing and conserves quantities. Depleted stacks disappear; an emptied source retires. The surviving destination keeps its stable ID.
- Commands require the actual ready Strategic campaign session, active campaign, no pending travel/encounter, matching authoritative managers and live world. Strategic zoom, mapped tactical gameplay, sandbox and quarantine cannot purchase strategic quantities.

This is the smallest explicit strategic policy layered over existing authored costs, rather than a new turn-based production queue or inferred tactical actor-to-army mapping. Tactical barracks placement/training and mapped paid enrollment retain their existing separate behavior.

## Atomicity and read contract

`FCampaignArmyState::TransferUnits` stages both rows before replacing its current value. Same-ID, unknown, foreign-faction, remote, malformed content/count, insufficient quantities and destination overflow leave the complete roster unchanged, including candidate source retirement.

`CampaignArmyActions.cpp` adds `AssignCampaignArmyUnits`, `GetCampaignRecruitmentQuote` and `RecruitCampaignUnits` to the existing CampaignManager. Recruitment stages inventory and a pure wallet debit from FactionManager values, then commits BOTH existing owners before either resource or army notification. Guards block reentrant campaign, territory, wallet and player-faction changes throughout publication. No observable debit-then-compensating-rollback sequence is used.

A quote is a current-state, non-reserving read. It remains readable from fully committed publication callbacks so an event-driven UI does not remain disabled after a successful command; actual commands always perform strict validation first. Travel/start remain unavailable even to quotes. Costs retain their computed value on an unaffordable quote, allowing UI to display a real price with its rejection reason. Invalid content does not imply a free unit.

Army-card reconstruction now follows the existing territory-card pattern: unique UObject names and stable GUID payloads. Tests look up army cards through `GetArmyID`, not a name that can change after rebuilding. No mutable UI inventory is introduced.

## Evidence checkpoint

- Editor50 + incremental8 succeeded; `Artifacts/C015E1-{Editor,Editor2}.log`.
- Pure3/3 clean, exit0 (`Artifacts/Automation/C015E1_Pure`, `Artifacts/C015E1-Pure.log`): existing state/archive tests plus the new atomic assignment contract, multiple content stacks, retirement, detached IDs and overflow rollback.
- Final runtime1/1 clean, exit0 (`Artifacts/Automation/C015E1_Runtime_Final`, `Artifacts/C015E1-Runtime-Final.log`): actual session entry, positive/new/existing/zero-cost purchases, six exact wallet fields, free quantity conservation, event counts, complete observer reads, readable publication quotes and rejected reentry. Negative sandbox/pending/mode/ownership/content/class/availability/count/unknown/no-settlement/remote/overflow/unaffordable/over-assignment paths reject. The fixture restores memory-only definition edits; no asset writes.
- Purchased/assigned canonical 23 Romans travel to the actual selected tactical map, keep identity and the six-cost debit, and only there materialize into the expected 33 tactical actors including Carthage's 10. Mapped tactical sessions reject strategic purchases.
- Existing session UI regression5/5 clean, exit0 (`Artifacts/Automation/C015E1_UIRegression`, `Artifacts/C015E1-UIRegression.log`) after unique army-card naming and GUID lookups.
- Full69/69 clean, exit0 (`Artifacts/Automation/C015E1_Full`, `Artifacts/C015E1-Full.log`), strict IWYU82 succeeded (`Artifacts/C015E1-IWYU.log`), final Shipping11 succeeded (`Artifacts/C015E1-Shipping-Final.log`). Repository validation reports zero open M1 warnings; LFS fsck and diff check pass. All observed Editor/Cmd/dotnet/CrashReportClient processes are terminal. Earlier incremental artifacts are superseded for final evidence.

## Remaining work

C-015E2 connects controller/native UI recruit/count/new-or-selected army/assignment controls and proves real pointer-routed actions, eligibility after notifications, lifecycle and rendering. C-015E3 replaces prototype strategic AI with canonical Carthaginian defend/recruit/adjacent-attack behavior using the ordered turn pipeline. C-016 provides disk save/load/autosave/corruption/version handling. No new cook, rendered recruitment UI, trusted packaged player, upkeep policy, producer-destruction refund or full milestone/goal completion is claimed.
