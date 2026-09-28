# M3.3 formation destinations

Status: selection-level formation planning implemented; targeted tests, full regression, strict IWYU and Shipping compilation pass. This is not completion of M3 or the vertical slice.

## Behavior

- A multi-unit move now builds one shared, travel-facing layout centered on the clicked destination. Approximately square ranks use a centered incomplete final row; spatial assignment keeps front/back and left/right ordering where possible. Each selected unit receives a distinct slot.
- `FRomanFormationLayout` is stateless. It returns destinations in input-member order and rejects invalid/non-finite or unrepresentable coordinates without changing its output. No new subsystem, leader actor, persistent formation state or independent order owner was introduced.
- Only eligible selected units participate. An unselected member of the same tactical squad is not implicitly commanded; squad membership/identity is unchanged. Single selection retains its exact destination.
- Minimum spacing is 160 cm, increased to twice the largest selected capsule radius plus 64 cm when necessary. Every multi-unit slot is projected with a tight 16 cm XY / 250 cm Z search before dispatch. This prevents collapsing all slots onto the nearest edge. If any slot is unavailable, the entire new move is rejected, prior orders remain intact and the existing transient UMG feedback explains the failure.
- Layout/projection happen once per command, not in unit ticks. Individual AI controllers still solve their own physical routes; this is shared high-level destination planning, not a shared navigation corridor. A valid slot can still have an unreachable route from a particular unit; the existing Hold/Unreachable execution policy applies to that unit.
- Move and attack dispatch use weak member snapshots and a shared revision barrier. A newer command issued by a synchronous cancellation/feedback observer supersedes the older batch, including members not yet visited. Valid commands clear obsolete feedback. No new module dependencies or assets.

## Verification

- `FormationLayout` covers counts 1/2/3/5/15/50/100, deterministic output, mean destination, pairwise spacing, spatial assignment, rotated travel, coincident members, zero travel, invalid inputs and unrepresentable spacing at extreme coordinates. Counts 50/100 here are pure layout coverage, not a runtime performance claim.
- `FormationPIE` creates a real four-member tactical squad, selects three, verifies distinct projected slots and no implicit fourth-member order, rejects an invalid footprint atomically with visible feedback, and issues a replacement group command from a real path-completion callback. All three members then physically reach their own latest destinations; squad identity is preserved and a subsequent single-unit command has no offset.
- The existing ownership fixture still exercises mixed eligibility, ownership loss, mode and possession guards. Its group destination is now navigable and it waits for NavMesh readiness; its off-map actor fixtures remain isolated and do not claim physical movement coverage.
- Initial unit regression: 12/12 successful, `Artifacts/Automation/M3_Formation_Initial_Units`. Final targeted: 2/2 successful, `Artifacts/Automation/M3_Formation_Final_Targeted`; one test carries the two known WASAPI device/engine sample-rate conversion notices. No host audio/security settings changed.
- Editor builds pass: `Artifacts/M3_Formation-InitialBuild.log`, `Artifacts/M3_Formation-FinalBuild.log`.
- Full regression: 29/29 successful, no failures/pending (`Artifacts/Automation/20260928_152938_618`, log `Artifacts/M3_Formation-Full.log`). Thirteen tests carry only the two known WASAPI notices; no other result warnings.
- Strict non-unity/no-PCH IWYU: successful, 63 actions (`Artifacts/M3_Formation-IWYU.log`). Shipping: successful, 5 actions (`Artifacts/M3_Formation-Shipping.log`). Repository validation, LFS integrity and diff checks pass.

## Remaining

Player-facing Stop/Hold controls, disconnected-island/narrow-passage coverage, group congestion, runtime 50/100-unit performance and rendered/fresh packaged navigation verification remain open. There is no new packaged-player evidence for this revision. Navigation foundation and earlier executable trust limitations remain documented in `M3_NAVIGATION_REVIEW.md` and `M3_ADDITIVE_SELECTION_REVIEW.md`. Combat windows, production/placement, objectives, campaign/save, UI/audio/art and release gates remain in the full plan.

Skills: `unreal-cpp-gameplay` and `ue-testing-debugging` guided the stateless owner boundary and real PIE/reentry checks; `ue-module-build-system` guides strict builds. `ue-project-context` updates remain limited to the previously approved modules/framework/verification sections.
