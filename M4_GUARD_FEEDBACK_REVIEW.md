# M4.7 event-driven guard feedback

Status: the native possession HUD now displays `Guard UP`/`Guard DOWN` from the controlled soldier's canonical block state. This is a narrow M4.7 feedback advance, not completion of the camera, animation or full combat-presentation package.

`AUnitBase::StartBlocking` and `StopBlocking` publish the existing vitals invalidation event after committing block state and movement speed. Repeated Start requests are idempotent. The widget already observes the possessed soldier and re-queries on invalidation, so no UI tick or mirrored guard variable was added. The HUD's fixed text width was increased for the extra label.

The real-PIE `PlayerOwnershipPIE` test enters possession, checks lowered guard, activates a shield block, checks the unit state and immediate HUD text, releases it, and checks the lowered text before the existing ownership-loss cleanup. This drives the live unit/widget event path, but does not inject a physical key press or inspect rendered pixels.

## Evidence

- Targeted test 1/1: `Artifacts/Automation/M4_GuardFeedback_Targeted`; log `Artifacts/M4_GuardFeedback-Targeted.log`. The final full run includes the later idempotency/text-width edits.
- Full regression 35/35, zero test warnings/failures: `Artifacts/Automation/20260930_142116_169`; log `Artifacts/M4_GuardFeedback-Full.log`.
- Strict non-unity/no-PCH IWYU 66/66 actions: `Artifacts/M4_GuardFeedback-IWYU.log`. Shipping Game compilation 5/5 actions: `Artifacts/M4_GuardFeedback-Shipping.log`. Repository validation, LFS integrity and diff whitespace checks pass.

No asset, module, plugin, service, save schema or gameplay-state owner was added. The verified feedback is textual only; attack montage/hit visuals, camera collision, time-scale policy, physical-input and trusted packaged-player gates remain open. Skills applied: `ue-ui-umg-slate` for event-bound native HUD state and `ue-testing-debugging` for the live PIE projection assertion.
