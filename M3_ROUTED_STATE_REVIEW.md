# C-011 — Morale-zero routing

Implementation 2026-10-05, UE 5.8.2. User-approved policy: morale zero triggers retreat. Routing is derived from the single vital owner (`Alive && Morale == 0`), not another mutable life state. Appended Routed activity preserves previous enum values.

Routed soldiers remain alive, collidable, registered, targetable and damageable. World participation is distinct from command eligibility; player selection/possession, attacks, block, dodge, normal orders and special action initiation reject routed soldiers. Existing vitals invalidation listeners reconcile controller eligibility and unbind on exit/teardown. Routing cancels existing actions, restores the same tactical AI and replaces intent with one withdrawal move.

Bounded slice behavior: destination is 1000 cm away from the damaging actor, horizontally; environmental/initial zero morale falls back to backwards (negative X for coincident direction). Existing NavMesh/path following owns movement; no direct translation, partial path or perpetual flight. Arrival remains routed; unreachable withdrawal settles in Hold. No automatic rally, morale recovery, despawn, casualty or campaign identity was added. Real death alone publishes the existing casualty event/removes live membership.

Evidence so far:

- Editor42 actions succeeds; test-only reflected call was corrected from Attack to the actual PerformAttack, then retry4 passes. Withdrawal test addition compiles4 actions.
- Directed VitalState + PlayerOwnershipPIE final2/2, zero warnings/errors/pending (`Artifacts/Automation/C011_Targeted_Final`). Initial run was1pass/1fail only from the incorrect test function name, now resolved.
- Pure checks cover nonlethal zero morale, remaining life/vulnerability, healing without rally, lethal precedence, initial-zero morale and EndPlay.
- Real-PIE checks cover guarded possessed routing and canonical camera/selection/flag cleanup, same AI, ten-metre destination away from damage source, rejected actions, targetable/damageable routed unit, no restarted withdrawal on repeated damage, failed route Hold without teleportation and actual death counted once.
- Shipping13 actions succeeds (`Artifacts/C011-Shipping.log`); repo/LFS/diff checks pass.

Physical `RoutedWithdrawalPIE` passes 1/1 with zero warnings/errors/pending (`Artifacts/Automation/C011_Withdrawal`), using actual NavMesh, CharacterMovement velocity and arrival within 60 cm of the 1000 cm destination. Life/routing/AI remain unchanged on arrival; no rally is invented. Strict IWYU69 actions passes (`Artifacts/C011-IWYU.log`). Full `C011_Full` passes39/39 with zero warnings/errors/pending and exit0. C-011 is verified, not the whole M3 milestone.

Limits: no rendered/physical-input/trusted packaged-player evidence; no recovery tuning, tactical terminal/campaign resolution, asset/config/module/dependency changes. Morale-zero soldiers remain living enemies for a capture rule requiring no living enemies. Reconciliation listens to the existing vitals event, including non-routing vitals changes; no per-frame scan or second state owner.

Skills: `unreal-cpp-gameplay`, `ue-data-assets-tables` (scoped test definitions restored without saving), `ue-testing-debugging`; `ue-project-context` maintains only authorized sections.
