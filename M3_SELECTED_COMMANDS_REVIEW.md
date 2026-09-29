# M3.3 selected-unit Stop and Hold

Status: player-facing controls, targeted real-PIE, full regression, strict IWYU and Shipping compilation pass. M3.3 remains open for broader navigation and runtime verification.

## Behavior

- The tactical selection panel has native STOP and HOLD buttons. They are visible and enabled only while the controller can issue tactical commands to a nonempty unit selection. Building selection, strategic mode and possession hide them. Existing `URomanHUDButton` styling and Slate pointer routing are reused; no new input assets or bindings were added.
- STOP cancels each eligible selected unit's current order and route. It leaves the unit idle, so normal enemy acquisition may resume on a later tick. HOLD cancels its route and keeps the explicit Hold order, preventing automatic acquisition until a new command. Tooltips and the tactical help text explain the controls.
- Both actions use controller command facades with weak member snapshots, the existing faction/world/command-ready eligibility rule, and the shared batch revision. A command issued from a synchronous cancellation observer supersedes the remainder of an older batch. A hidden or stale widget callback still passes through these guards. Neither command expands a partial squad selection or changes squad identity.

## Evidence

- Corrected Editor build: `Artifacts/M3_Commands-CorrectedBuild.log`. The initial build caught one test-only `FArrangedChildren` iteration error; the fixture now uses indexed access.
- Final Editor build: `Artifacts/M3_Commands-FinalBuild.log`.
- `SelectedCommandsPIE` targeted 1/1 successful: `Artifacts/Automation/M3_Commands_Final_Targeted`. Two real Legionaries follow live routes; cancellation triggers a nested Hold which wins for both; isolated virtual Slate pointer press/release reaches STOP and HOLD buttons and is consumed by each SButton. The test checks stopped path followers, persistent Hold after a world update, strategic/possession/owner-loss hiding and rejection of stale callbacks. This is in-process Slate coverage, not a physical mouse or packaged-player claim.
- The targeted result carries exactly two host WASAPI notices (48 kHz engine and 44.1 kHz device sample rates, with conversion). No host settings changed.
- Full regression: 30/30 successful, no failures/pending (`Artifacts/Automation/20260929_185107_129`, log `Artifacts/M3_Commands-Full.log`). Fourteen tests have only the two host WASAPI notices above; no other result warnings.
- Strict non-unity/no-PCH IWYU: successful, 63 actions (`Artifacts/M3_Commands-IWYU.log`). Shipping: successful, 5 actions (`Artifacts/M3_Commands-Shipping.log`). Repository validation, Git LFS integrity and diff whitespace checks pass.

## Remaining

Disconnected islands, narrow passages, group congestion, runtime 50/100-unit performance, rendered navigation and a fresh packaged-player gate remain open, followed by later M3–M7 work in the implementation plan. Previous packaged-executable application-control evidence remains specific to the older binary; no host security policy was changed or blocked binary retried.

Skills applied: `unreal-cpp-gameplay` for the controller boundary, `ue-ui-umg-slate` for native widgets and click lifecycle, and `ue-testing-debugging` for real PIE/Slate assertions. Project context changes stay within the approved framework and verification sections.
