# C-008 — Guarded training cancellation UI

2026-10-05, GPT implementation requested by user, uncommitted. C-007 receipt/refund authority preserved.

## Implementation

PlayerController exposes `CanCancelSelectedTraining` and `CancelSelectedTraining` using the existing same-world/player-owned/live-building eligibility and tactical command guard, plus active training. It delegates the refund to Barracks; no second receipt/wallet owner. The UI uses the same query.

Native `CANCEL + REFUND` mouse HUD button sits beside TRAIN in one horizontal row without increasing the selection panel height. It inherits no-keyboard-focus behavior. Tooltip explains active-order cancellation/full paid refund/automatic prepaid queue promotion. Existing selection/mode/production/resource events refresh its visibility and enabled state. Destruct disables it; the callback rejects detached/unready/invalid or ineligible state. A failed canonical refund shows retained-order feedback instead of reporting cancellation success.

## Verification

- Editor 38/38 actions, `Artifacts/C008-Editor.log`.
- Directed CanonicalEventDrivenPIE **1/1 clean**, `Artifacts/Automation/C008_Targeted`.
- Strict IWYU 69/69, `Artifacts/C008-IWYU.log` (includes final test-only capture changes).
- Shipping 5/5, `Artifacts/C008-Shipping.log`.
- D3D12/offscreen **1/1 successful, one known engine motion-vector warning, zero errors**, `Artifacts/Automation/C008_Visual`.
- Full regression **38/38, zero test warnings/errors/pending**, `Artifacts/Automation/C008_Full`, `Artifacts/C008-Full.log`.
- Repository validation 0 M1 warnings, LFS fsck and whitespace pass; historical CRLF notices only. All C-008 processes exited 0.

The real Slate mouse click refunds all six original paid fields once and promotes exactly one queued order. UI receives the final pending count without tick. Strategic mode, ownership loss, idle producer and detached-widget callback reject/hide/disable; controller and UI eligibility agree. Existing C-007 adversarial price/payer/overflow/reentry tests still pass. No OS physical-input claim.

Inspected `Artifacts/C008_Visual/{ActiveTraining,IdleTraining}.png` (1024×555): TRAIN and CANCEL fit in the selection panel; idle producer hides CANCEL and shows idle state. Scoped test-only zero catalog price is visible in the idle capture and restored afterward, not a production economy change. Capture opt-in uses `-RomanCaptureStrategicUI -RomanCaptureTrainingUI`, with no NullRHI. Other Strategic snapshots produced in that directory are not newly inspected. No AI asset generation, module/dependency/input action/asset/config/security change. No new cook/package/trusted-player claim.

Skills influenced reflected C++ façade, shared eligibility and lifecycle-driven UMG, real Slate/automation verification. Refund authority stays in Barracks/FactionManager.

## Remaining

M3.5 still needs automatic spawn-failure/invalid queued-definition/destruction refund handling, objective-ready HQ and trusted player journey. M3/M4/M5/M6/M7 whole milestones remain partial. Next C-009 verifies the approved real-time possession contract against repeated enter/exit/death paths; do not add slow motion or a redundant time owner when existing behavior is correct.
