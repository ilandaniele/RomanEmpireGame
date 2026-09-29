# M3.3 crossing-group congestion

Status: real-PIE crossing-formation coverage passes. M3.3 remains open for measured 50/100-unit runtime performance, rendered behavior and a fresh packaged-player gate.

`RomanEmpireGame.Units.GroupCongestionPIE` uses six real Legionaries with CharacterMovement, AI path following and RVO enabled. The player controller issues one three-unit formation Move from each side of the same corridor before either group completes. The test checks that all six orders survive selection changes, each member reaches its assigned slot, none reports an unreachable route, moving velocity occurs and no pair overlaps below 65 cm. The nearest opposing pair reaches 84 cm, matching two 42 cm capsule radii; this confirms actual shared-corridor congestion rather than independent distant paths. Fixtures are transient PIE actors; no assets or runtime behavior changed.

- Editor builds: `Artifacts/M3_Congestion-Editor.log`, `Artifacts/M3_Congestion-FinalBuild.log`.
- Final targeted 1/1: `Artifacts/Automation/M3_Congestion_Final_Targeted`, log `Artifacts/M3_Congestion-FinalTargeted.log`. An earlier targeted run also passed before the close-contact assertion was added.
- Full regression 32/32, zero failures/pending: `Artifacts/Automation/20260929_190751_272`, log `Artifacts/M3_Congestion-Full.log`. Sixteen tests carry only the known host WASAPI 48 kHz/44.1 kHz sample-rate and conversion notices.
- Strict non-unity/no-PCH IWYU: succeeded, 63 actions, `Artifacts/M3_Congestion-IWYU.log`. Shipping target: succeeded and up to date with zero actions because this block changes only tests, `Artifacts/M3_Congestion-Shipping.log`. Repository validation, Git LFS integrity and diff whitespace checks pass.

This does not establish a 50/100-unit performance baseline or prove rendered/packaged-player behavior. The previously blocked unsigned package was not retried and Windows security was not changed. Skill applied: `ue-testing-debugging` for the real PIE multi-agent crossing fixture and outcome assertions. Project context changes stay within the approved verification section.
