# M3.1 order, combat and selection state integration

Started 2026-09-20 from `87f4651`. This is a progress packet, not M3.1 or full-plan closure.

## Ownership and behavior

| Owner | Authoritative state | Does not own |
|---|---|---|
| `FUnitVitalState` | Live vitals and initialization/death/end lifecycle | Orders, world effects |
| `FUnitOrderState` | None/Move/Attack/Hold, destination, weak target, pursuit | Movement execution, target discovery, damage |
| `FUnitCombatState` | One recovery clock, orthogonal blocking | Vitals, damage, target, authored rate |
| `FUnitSelectionPresentation` | Local selected indication and reusable material | Controller selected set, faction policy |

`AUnitBase` remains the public facade and world/lifecycle/event coordinator. The previous parallel order/combat/selection fields and unused `UpdateAIMovement` executor are removed. `GetActivity()` projects Unavailable/Idle/Commanded/Attacking/Dead/Ended from these owners; it is not stored separately. Attacking includes recovery, not an animation or validated impact window.

Move replaces Attack and clears its target. Pausing pursuit keeps Attack/target; resuming updates the destination without replacing the intent. Move completion becomes None. Hold persists without automatic acquisition, whereas Stop returns to idle and permits subsequent acquisition. Invalid orders are rejected without changing state. Dead/destroyed/friendly targets retire pursuit and its destination together. A replacement order issued by a synchronous damage observer is not overwritten by the old attack's continuation.

AI and direct attacks derive recovery as `1 / AttackSpeed` once, rejecting invalid/non-representable rates before spending stamina. The coordinator advances recovery once per tick, before action evaluation; newly committed attacks receive their full recovery. Recovery is committed before damage/vitals observers can reenter. Changing orders does not refund recovery. Blocking remains orthogonal, preserving existing policy until M3.4.

Selection reuses one GC-tracked material and updates both visibility and hidden-in-game flags. All sixteen actor component identities remain intact. Possession completes flag/order/movement changes before camera activation, because activation/deactivation emits synchronous component events. No stale outer effects follow a nested exit or termination.

## Verification

The first full suite passed 21/21 (`Artifacts/Automation/20260920_173412_454`, `Artifacts/M3_UnitStates-Tests.log`). The isolated OrderState test produced two warnings because its transient world lacked an engine context. The fixture now creates and destroys its own context without replacing the editor/PIE context; no log suppression was added.

Mutation control temporarily restored double elapsed-time advancement, a second division by attack speed, and camera notification before completion of possession effects. `OrdersCombatPIE` failed on early AI strikes, wrong recovery/stamina, overwritten speed and canceled callback orders; the other twenty tests passed without warnings (`Artifacts/Automation/20260920_173543_521`, `Artifacts/M3_UnitStates-Mutation-Tests.log`). All three mutations were removed.

The restored implementation passes **21/21, zero warnings/failures/not-run/in-process** (`Artifacts/Automation/20260920_173627_023`, `Artifacts/M3_UnitStates-Restored-Tests.log`). Tests include pure finite-value/atomic order transitions, real weak-target destruction/GC, recovery partitions and bounds, and real-PIE Hold/pursuit/target retirement and exact AI/direct recovery timing. The PIE fixture changes only transient instance attack rates, never canonical assets. Synchronous probes attempt nested attacks, Move/Hold replacement, attacker/target death/destruction and exit/death during actual camera activation. Existing component/vitals/death/input/UMG tests remain green.

Strict non-unity/no-PCH Editor IWYU passed, including all new test translation units (58 actions, `Artifacts/M3_UnitStates-IWYU.log`). Final Shipping compilation passed after the camera correction and restoration (`Artifacts/M3_UnitStates-Final-Shipping.log`); its receipt excludes the test module, MCP and editor toolsets. Repository/prerequisite validation and Git LFS fsck pass. No engine, security or plugin defaults changed; Test used the existing process-only `-DisableFab` option.

No package/player evidence for this source revision is claimed. The earlier unit-foundation package evidence in `M3_UNIT_FOUNDATION_REVIEW.md` applies only to its recorded source/binary. No Editor/player process or loopback MCP listener remains after verification.

## Explicit limits and next work

Routed behavior remains unimplemented. It must be alive and vulnerable while unable to accept player/AI actions; merely changing `CanAcceptCommands()` would be unsafe because legacy damage currently shares that guard. It needs an explicit availability event and controller/UMG cleanup without pretending a combat death occurred. Trigger thresholds, flight and rally/recovery must be resolved explicitly before implementation.

Rendered selection and first-person possession still require actual-player evidence. Existing per-unit scans, direct movement and immediate damage remain temporary executors for the later registry/navigation/combat packages. No new procedural animation or art was added. No performance, full encounter, save/restore or complete vertical-slice claim is made here.

Skills: `unreal-cpp-gameplay` (reflected state/GC and callback boundaries), `ue-testing-debugging` (pure rules, real actors and mutation controls), `ue-module-build-system` (target/include validation), and `ue-project-context` (scoped factual architecture update, with the proposed document shown and adjustments invited before saving). Other UE skills now read the updated context; engine/IDE/plugin choices are preserved.
