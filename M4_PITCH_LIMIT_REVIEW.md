# M4.7 possession pitch limits

Status: first-person possession now constrains the live player camera manager to a ±80° pitch window, and every exit restores the exact prior tactical minimum and maximum. This is a narrow M4.7 movement-limit advance, not camera collision or a rendered-camera gate.

The controller saves the camera manager and both previous bounds only after the unit is actually possessed and gameplay mode has committed. Normal exit restores the bounds before synchronous unit/camera cleanup callbacks. Failed or interrupted entry before that point never changes the manager. The existing death, ownership-loss and explicit-exit paths all use the same teardown; no global camera policy or second gameplay mode was introduced.

Real-PIE tests verify the bounds during each of three repeated same-unit possession cycles and their restoration after every exit. The canonical UI integration test also verifies that death while fully possessed restores both bounds and the tactical camera pawn. These are manager-state assertions, not rendered field-of-view or physical mouse-motion checks.

## Evidence

- Targeted `PlayerOwnershipPIE` 1/1: `Artifacts/Automation/M4_PitchLimit_Targeted`; log `Artifacts/M4_PitchLimit-Targeted.log`.
- Targeted `CanonicalEventDrivenPIE` 1/1 for death fallback: `Artifacts/Automation/M4_PitchDeath_Targeted`; log `Artifacts/M4_PitchDeath-Targeted.log`.
- Final full regression 35/35, zero test warnings/failures: `Artifacts/Automation/20260930_143741_059`; log `Artifacts/M4_PitchDeath-Full.log`.
- Strict non-unity/no-PCH IWYU 66/66 actions: `Artifacts/M4_PitchDeath-IWYU.log`. Shipping Game target succeeds up to date (0 actions) after the prior runtime Shipping 4/4 compilation in `Artifacts/M4_PitchLimit-Shipping.log`. Repository validation, LFS integrity and diff whitespace checks pass.

No asset, module, plugin or new input mapping was added. Camera collision, animation/hit feedback, time-scale policy, physical look-input and trusted packaged-player checks remain open. Skills applied: `unreal-cpp-gameplay` for ownership of camera transition state and `ue-testing-debugging` for real-PIE exit/death assertions.
