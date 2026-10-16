# C-015D2 — Runtime campaign session and world boundaries

Date: 2026-10-05. Runtime/API foundation implemented; user-facing entry/action/Continue/recovery controls remain C-015D3. The complete implementation plan and C-015 are still open.

## Ownership and boundaries

`UCampaignSessionSubsystem` is automatically instantiated by GameInstance, without a custom GameInstance class or configuration change. It coordinates world identity, phase, strategic return map and a one-use travel nonce. Existing CampaignManager, FactionManager and WorldMapManager remain the sole live authorities. A reflected detached D1 capsule exists only during travel or quarantined recovery; successful import clears it.

- Strategic entry explicitly leaves the sandbox, retaining its current canonical campaign. It is not a new-campaign reset or disk load.
- Battle entry prepares the same validated adjacent Rome→Carthage plan used by the direct arena command, without allocating local squads. Destination comes from the selected territory's authored TacticalMap, verified as a real top-level World asset through Asset Manager metadata.
- OpenLevel actually replaces the world. Incoming GameMode suppresses prototype unit seeding, creates managers against the current catalog, validates/installs all domains and materializes fresh local bindings before publishing readiness. Both actual map metadata and selected-territory map correspondence are rechecked. Stable armies/encounter/receipt ledgers, city state, all wallets, diplomacy, turn/outcome/threshold/autosave fields and both RNG seeds use the existing D1 contract.
- Terminal return reads and applies only the canonical mapped result. AlreadyApplied permits retrying a return, never applying casualties twice. It then captures the consumed checkpoint, opens the strategic return world and resumes turns without a stale battle. Sandbox Restart is unavailable inside a session or while session travel is pending.

## Paid orders and failure handling

One bounded producer scan at travel enumerates active AND queued receipts. Preflight checks original payer, valid unique receipt IDs, nonnegative recorded costs and exact combined six-currency credit through the canonical economy. An invalid/overflowing refund changes no wallet, queue, receipt, progress or travel request. After successful validation, all wallet credits and cleared queues commit before any resource/production publication; nested campaign/resource/diplomacy/territory/producer mutations remain frozen until teardown or engine travel failure. Completed/enrolled receipts cannot be refunded again.

Return applies a terminal result before refund preflight. If refund subsequently fails, the source world and outstanding receipts remain; the result is already consumed exactly once and a later return retries settlement/travel. This is explicit progress, not an all-or-nothing rollback of a consumed result.

Wrong incoming world/nonce, invalid current catalog/content or deployment failure never publish a fresh default campaign as restored. The exact capsule is retained, the incoming world is quarantined (not ready, production/resource commands frozen), and `RetryPendingTransfer` opens the intended map with a fresh nonce and the same payload. A failed retry retains the capsule. Engine travel-failure subscriptions are removed on subsystem teardown. Original source-world travel failure releases the freeze while retaining the already completed cancellations/credits; that callback branch has not yet been tested with a real engine-induced load failure.

## Verification

- Editor builds pass after fixing a shadowed AActor Owner local and fixture field/class-type mismatches.
- Initial actual round-trip directed tests: 2/2 clean, exit 0, `Artifacts/Automation/C015D2_Targeted`.
- Strengthened directed tests: 6/6 clean, exit 0, `Artifacts/Automation/C015D2_Targeted_Verified`, `Artifacts/C015D2-Targeted-Verified.log`. Four new SessionTravel tests plus existing direct arena victory/defeat regressions.
- Full suite: 62/62 clean, exit 0, `Artifacts/Automation/C015D2_Full`, `Artifacts/C015D2-Full.log` (includes final recovery-notification changes).
- Strict IWYU: 78 actions plus final 4-action notification-only increment succeed (`Artifacts/C015D2-IWYU.log`, `Artifacts/C015D2-IWYU-Final.log`). Shipping30 succeeds (`Artifacts/C015D2-Shipping.log`). Repository validation reports zero open M1 warnings; LFS fsck and diff check pass. Engine startup's self-test diagnostics occur before the project queue and are not project test failures.

Victory/defeat journeys use actual L_Testbed→strategic world→selected TacticalMap→strategic return. They prove stable GameInstance/army IDs, preserved city/owner/wallets/turns, real 15/10 deployment, physical timer capture or real deaths, one carried casualty/retirement, consumed result ledger, complete terminal paid-order refunds, duplicate request rejection and resumed turn. Resource observers see every credited currency and every cleared receipt before notification; nested session/wallet/territory requests are rejected. Hooks at actor spawning observe the FIRST public readiness event in each of three new worlds, verifying restored values and the tactical encounter already installed.

Recovery tests use actual replacement worlds: clear the loaded unit class only during incoming world initialization, or deliberately overwrite the queued engine travel with another world/wrong nonce. They prove no readiness/default armies/battle or paid production in quarantine, preservation of the capsule, actual retry, stable identity/counts and exactly-once capsule consumption. Fixture content changes are memory-only and restored; no assets or defaults are saved.

## Remaining gates

Implement C-015D3 native event-driven entry, army action/battle, Continue and recovery controls, detached callbacks/reattachment, physical Slate input and rendered verification. No new render, cook, packaged player, save-slot/corruption envelope, strategy recruitment/AI, producer-destruction refund or full first-playable claim. The unsigned-player Code Integrity limitation remains; no security policy changes or identical blocked relaunches.

Skills used: unreal-cpp-gameplay, ue-data-assets-tables (data-driven ownership), ue-testing-debugging (real latent worlds) and ue-project-context (only authorized module/subsystem/verification sections). No new module/plugin/dependency/configuration/asset or automatic commit.
