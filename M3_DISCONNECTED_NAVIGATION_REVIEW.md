# M3.3 disconnected navigation and narrow passage

Status: real-PIE coverage added; M3.3 remains open for group congestion, measured runtime performance, rendered behavior and a fresh packaged-player gate.

## Scenario and result

`RomanEmpireGame.Units.DisconnectedNavigationPIE` runs on the committed `L_Testbed` with its dynamic 42 cm-radius NavMesh agent. Three transient, colliding wall segments span the NavMesh bounds. Both start and goal project onto navigable points, but a temporary gate separates their islands. The real Legionary/AI Move must settle in Hold/Unreachable, leave its path follower idle and remain on the near side. The test then destroys only the gate, waits for a complete recalculated path, issues a new Move and verifies CharacterMovement velocity and arrival through the 400 cm opening without crossing either solid wall. The test fixture does not save assets or change runtime behavior.

- Editor build: `Artifacts/M3_Disconnected-Editor.log`.
- Targeted PIE: 1/1 successful, `Artifacts/Automation/M3_Disconnected_Targeted`; log `Artifacts/M3_Disconnected-Targeted.log`.
- Full regression: 31/31 successful, zero failures/pending, `Artifacts/Automation/20260929_185920_597`; log `Artifacts/M3_Disconnected-Full.log`. Fifteen tests carry only the two known host WASAPI notices (48 kHz engine, 44.1 kHz device, conversion used).
- Strict non-unity/no-PCH IWYU: succeeded, 63 actions, `Artifacts/M3_Disconnected-IWYU.log`. Shipping target: succeeded and up to date, zero actions because this block changes tests only, `Artifacts/M3_Disconnected-Shipping.log`.
- Repository validation, Git LFS integrity and diff whitespace checks pass.

This is live in-process PIE navigation, not a rendered or packaged-player check. The previous unsigned packaged executable was blocked by host Windows Code Integrity; no security policy or plugin defaults were changed and that binary was not retried.

Skill applied: `ue-testing-debugging` for an isolated real-PIE navigation fixture and result assertions. Project context changes are limited to the approved framework/subsystem and verification sections.
