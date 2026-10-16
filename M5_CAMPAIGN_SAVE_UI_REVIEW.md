# C016C — Native local save controls

Status: directed9, strict IWYU and Shipping verified 2026-10-06; final rendered9/full95 gates verified. Uncommitted, whole milestone/goal open.

## Implementation

- Native panel in the existing owned production UMG root: SAVE, LOAD (Manual), LOAD AUTO, AUTO ON/OFF, typed status and confirm/cancel. Bottom-center in strategic sessions; top-center in sandbox so selection/production retains its existing bottom-center area. Fixed logical slots and existing subsystem contracts; no new keyboard mapping/assets/dependencies or alternate authority.
- First manual save writes immediately. Existing manual-slot overwrite and both load actions require confirmation before any I/O/source abandonment; cancel does neither. If turn/world/mode/readiness/eligibility changes, stale confirmation is invalidated. Every callback revalidates attached current-world local controller, cached GameInstance subsystem and canonical readiness; actual mapped tactical/possession hides the save controls.
- Persistence events bind once per root construction and unbind on detach/destruct/travel. One coalesced weak next-tick timer refreshes after the synchronous operation/turn guard; clear on destruct. Canonical BroadcastTurnCompleted precedes Autosave, so this handles direct/backend/keyboard turns without UI Tick/polling. Native callbacks also refresh after returning. Preferences/status queried, not duplicated; confirmation/notice are presentation-only values.
- Slot failures and recovered-backup flag surface through status, and pending transfer explicitly says restoration is not complete. Successful manual save uses its own panel notice instead of the older shared toast, which was partially covered by the strategic panel in an initial screenshot. No broad legacy UI redesign.

## Evidence

- Editor runtime and tests builds succeeded: `C016C-Editor-Verified.log`, `C016C-Editor-Tests.log`, `C016C-Editor-Resume.log`, `C016C-Editor-Recovery.log`. UHT rejected comma-delimited reflected properties and compiler rejected inherited Slot shadowing; split reflected fields/renamed the local, no suppression.
- Directed7 clean `Automation/C016C_Targeted`, then final9/9 clean exit0 `Automation/C016C_Targeted_Final`, `C016C-Targeted-Final.log`.
- UI test routes actual in-process Slate pointer paths to toggle/save/overwrite/confirm/cancel/load. Verifies no premature write/travel, generation rotation, event-only post-boundary reenable after a direct turn, saved preferences before first readiness and complete detached state equality after real reload. Detach/reattach/stale prior-world callbacks and unique persistent listener ownership verified. Not OS input/player proof.
- Resume test opens an actual new ready sandbox, not an active session, then uses native load confirmation to restore the stored canonical campaign and preferences. This is a new world in the same GameInstance, not a process-restart claim.
- Recovery test removes the current factory only AFTER valid load source preflight in incoming world initialization. Import fails explicitly, defaults never publish readiness, save UI remains collapsed, pending capsule retained and original preferences do not leak. Physical native retry after fixture repair restores the SAME saved value/preferences before its only readiness event. Observer GC runs during actual load/retry.
- Initial rendered8/8 successful `Automation/C016C_Visual` =7clean+1known engine r.MotionVectorSimulation warning; captures in `Artifacts/C016C_Visual`, inspected UI Loaded/ConfirmLoad and Resume ConfirmLoad. Controls/confirmation fit without overlapping command panel; initial shared toast clipping corrected afterwards. Final render captures still required for that last correction and Recovery.
- Original SaveDisk six disk/domain/backup/version/IO/queuedAI regressions remain in final9, clean. No new cook/asset writes/trusted-player or full milestone claim.

- Strict IWYU91 succeeded `C016C-IWYU.log`; Shipping8 succeeded `C016C-Shipping.log`. After the sandbox-position adjustment, strict incremental build succeeded `C016C-IWYU-Layout.log` and Shipping succeeded `C016C-Shipping-Layout.log`. C016D's combined native journey directed2 now passes, including the legitimate incoming AI defense after human defeat; see [journey review](M5_FIRST_PLAYABLE_JOURNEY_REVIEW.md). No broader requirement redefined around these passing tests.

Next: C-017/M6 responsive production UI/minimap, followed by remaining content/accessibility/performance/distribution gates. Native combined journey is verified in Editor, not a trusted packaged player. Preserve uncommitted work; no re-audit.

Final rendered9/9 successful exit0 `Artifacts/Automation/C016C_Visual_Final`, `C016C-Visual-Final.log` =8clean+1known engine r.MotionVectorSimulation warning. All ten final PNG in Artifacts/C016C_Visual inspected, including corrected panel notice, sandbox top-center resume and quarantine/recovery. Final full95/95 clean exit0 `Automation/C016D_Full`, `C016D-Full.log` includes the final layout/rooting/GC and combined-journey changes. Repo0M1warnings/LFS/diff pass. All processes terminal; C016C closed, M5/whole goal not closed.
