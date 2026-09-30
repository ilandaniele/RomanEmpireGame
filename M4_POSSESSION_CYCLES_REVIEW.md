# M4 possession return cycles

Status: the existing possession path now has explicit real-PIE regression coverage for three repeated enter/exit cycles with the same living, owned soldier. This advances M4.3 and M4.4, but does not close M4 or its packaged-player gate.

`RomanEmpireGame.Units.PlayerOwnershipPIE` selects one Rome-owned soldier, records the tactical camera transform and selection, and enters/exits first-person control three times. Every return must restore the original camera pawn and transform, preserve the selected soldier, leave possession mode, and return that soldier to the **same** tactical AI controller. The existing test also rejects enemy, dead, deferred and foreign-world possession, and covers ownership/death/reentrant callback cleanup. The new cycle assertions use the real controller and unit in PIE; they do not synthesize a second gameplay state.

## Evidence

- Targeted `PlayerOwnershipPIE` 1/1: `Artifacts/Automation/M4_PossessionCycles_SameAI`; log `Artifacts/M4_PossessionCycles-SameAI.log`.
- Full regression 35/35, zero test warnings/failures: `Artifacts/Automation/20260930_140856_153`; log `Artifacts/M4_PossessionCycles-Full.log`.
- Strict non-unity/no-PCH IWYU 66/66 actions: `Artifacts/M4_PossessionCycles-IWYU.log`. Shipping Game target succeeds up to date (0 actions): `Artifacts/M4_PossessionCycles-Shipping.log`. Repository validation, LFS integrity and diff whitespace checks pass.

No runtime code, asset, module, plugin or dependency changed. No fresh packaged-player or rendered/physical-input claim is made. M4.1's complete battle-end transition, M4.7's camera collision/animation/feedback, M4.8's time-scale rule and the full encounter-after-possession exit gate remain open. The `ue-testing-debugging` real-PIE pattern was used for the same-actor/AI assertions.
