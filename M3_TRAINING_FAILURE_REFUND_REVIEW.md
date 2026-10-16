# C-010 — Failed production refunds

Implementation 2026-10-05, UE 5.8.2. Barracks remains the production owner; FactionManager remains the wallet owner.

Failed spawning refunds the captured original paying faction and all six paid fields. If credit cannot commit (e.g. overflow), the active receipt and queue remain; subsequent ticks retry credit only, never spawn again. A promoted order with an unavailable factory or invalid duration retains its receipt as a failed active order and refunds on the next tick. No re-charge or current-price/current-owner refund lookup.

`IsAwaitingTrainingRefund` exposes the canonical pending state. A single `RefundPending` invalidation is emitted for blocked spawn refunds, not on every retry; UMG reads it through existing production subscriptions and shows the retained failure. `Failed` is emitted only after refund commits. Invalid promotion emits existing `Started` with the committed pending state; its terminal `Failed` follows successful credit. Manual cancellation can resolve a retained receipt through the same refund helper. The existing production guard blocks nested cancel/train/tick calls.

Evidence so far:

- Initial Editor build hit a test-only soft-class type mismatch; corrected to the actual `TSoftClassPtr<AUnitBase>` and added explicit ScopeExit include. Retry 4/4 actions and final Editor 11/11 pass.
- Directed `CanonicalEventDrivenPIE`: initial and final each 1/1, zero warnings/errors/pending (`Artifacts/Automation/C010_Targeted{_Final}/index.json`).
- Tests cover all six nonzero paid fields, original payer/cost after ownership/price changes, active overflow preserving exact receipt and queue, factory restored while credit remains blocked without spawning, invalid queued duration/restoration without spawning, exactly two refunds/Failed events, no Completed, no new registry unit, idle no double refund, nested callbacks and UMG pending/idle projection without a UI tick.
- Shipping 6/6 actions, exit 0: `Artifacts/C010-Shipping.log`.
- Repository validation zero M1 warnings, LFS fsck and diff whitespace pass (historical CRLF notices only).

Final gates: full `C010_Full` passes 38/38, zero warnings/errors/pending, exit 0 (`Artifacts/C010-Full.log`); strict IWYU 69/69 actions, exit 0 (`Artifacts/C010-IWYU.log`). C-010 is verified; M3.5 remains open for producer destruction and the remaining milestone/player gates.

Limits: no refund ledger surviving producer destruction/world teardown; no changed capture-production policy, asset/config/dependency, new cook, rendered-fit or trusted-player claim. Failure refunds do not create a transaction-wide atomic UI snapshot: wallet credit observers run before production transition. No tick warning/event spam on blocked credit; wallet transaction retry is still per active producer tick.

Skills: `unreal-cpp-gameplay`, `ue-data-assets-tables`, `ue-testing-debugging`, `ue-ui-umg-slate`; `ue-project-context` updates authorized framework/verification sections only.
