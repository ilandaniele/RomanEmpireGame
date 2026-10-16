# C-007 — Manual training cancellation refund

2026-10-05, GPT implementation explicitly requested by user. Policy approved: cancellation before unit generation returns full recorded training cost. Uncommitted; previous C-001–C-006 work preserved.

## Contract

`FTrainingReservation` contains stable UnitID, PayingFaction and six-field PaidCost captured at acceptance. Barracks owns one active receipt and a bounded queue of receipts, replacing the ID-only queue and active ID. Public ID/count getters remain; new GetCurrentTrainingReservation returns a detached value. No new authority/subsystem/asset/price lookup for refunds.

TrainUnit copies the authored definition/owner before synchronous wallet observers can alter them, then records the accepted receipt. Queued orders remain prepaid and are promoted without another charge. This preserves the existing spawn/current-definition behavior; the receipt captures price, not the whole unit schema.

TryCancelTraining returns false on no active order, reentry, missing/wrong-world authority or failed canonical credit. Refund uses the receipt's payer and paid cost, never the building's current owner or current catalog price. Failed credit leaves active receipt/progress/queue untouched for retry. Success consumes the receipt and promotes the next prepaid order under the existing production guard. CancelTraining void remains the legacy wrapper. Refund observers cannot recursively cancel, tick production or append another order while the transaction runs. Production notifications follow final queue promotion as before; wallet observers fire synchronously during credit, so this is not an atomic wallet-plus-UI observer snapshot guarantee.

## Verification

- Editor 10/10: `Artifacts/C007-Editor.log`.
- Directed CanonicalEventDrivenPIE 1/1, zero test warnings/errors: `Artifacts/Automation/C007_Targeted`.
- Shipping 5/5: `Artifacts/C007-Shipping.log`.
- Strict IWYU 69/69: `Artifacts/C007-IWYU.log`.
- Full regression **38/38, zero test warnings/errors/pending**: `Artifacts/Automation/C007_Full`, `Artifacts/C007-Full.log`.
- Repository validation 0 M1 warnings, LFS fsck, whitespace and local Markdown links pass; historical CRLF normalization notices only. All C-007 processes exited 0.

Real PIE test changes current price to zero and current owner to Carthage after Rome prepaid the queue. It verifies captured six-field cost/payer, forced refund overflow preserving the receipt/queue and successful retry, exact original-payer refunds over every active/promoted order, no recharge on promotion, no inactive-order double refund, guarded nested cancellation/training, and zero-cost acceptance/cancellation. Catalog edits are scoped/restored and the building returns to original ownership/selection; no asset saves.

Skills: Unreal C++ reflection, data/receipt separation from authored assets, automation-first verification. No module/plugin/config/security change. No new render/cook/package/player claim.

## Remaining / next

C-008 wires a guarded controller cancellation facade and mouse UMG control, with real Slate click and eligibility/lifecycle tests. Manual refund logic alone does not close M3.5: automatic spawn-failure/refund handling, queue invalidation, building destruction/ownership policies, objective-ready HQ and trusted packaged journey remain. Existing failed-spawn/invalid-queued-definition behavior still consumes a reservation; do not silently claim it refunded or change its policy without recording the scope. No commits/push.
