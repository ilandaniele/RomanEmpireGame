# M3.3 navigation foundation

Status at the navigation-foundation revision: targeted PIE, full regression, strict IWYU and Shipping compilation pass. M3.3 remained open for formation destinations and broader movement validation.

Subsequent formation implementation and its separate evidence are tracked in [M3_FORMATION_REVIEW.md](M3_FORMATION_REVIEW.md); the results below remain specific to the navigation-foundation revision.

## Implemented

- `ARomanUnitAIController` executes the unit's canonical Move/Attack intent using engine path following and CharacterMovement. Living tactical movement no longer calls `SetActorLocation`; the existing corpse sink remains presentation-only.
- Both maps contain explicit NavMesh bounds spanning `(-4500,-6000,-1000)` to `(7500,6000,1000)`. Dynamic Recast generation supports the existing runtime-spawned environment. Supported-agent radius/height are 42/192 cm, matching the capsule. Both saved actors were reloaded and read back through the loopback editor MCP with those values and `Dynamic` generation.
- CharacterMovement RVO remains the avoidance implementation. The unused Detour crowd manager is explicitly disabled; no crowd-following component is used.
- Move arrival clears Move; reaching attack range stops locomotion while preserving Attack and its target. Moving actor targets are tracked by path following. `EUnitNavigationResult` reports None/Moving/Arrived/Unreachable without owning a second order state.
- Requests require complete paths and navigable destinations. Failure settles in Hold/Unreachable instead of moving through geometry or retrying every frame. A later command replaces that state. Invalid input still preserves the prior order.
- Accepted replacements and cancellation advance an order revision; position tracking does not. Completion is scoped to request ID, revision and pawn identity. Cancellation invalidates the old request before notifying observers; Hold/Stop commit before callbacks can issue replacement intent.
- Player possession parks the same tactical AI controller; exiting restores it. Death cancels its route; EndPlay destroys the cached controller. No behavior tree, new subsystem or world scan was added to runtime movement.
- `AIModule` and `NavigationSystem` are private runtime and Editor-test dependencies. No MCP/editor dependency enters runtime.

## Evidence

- Editor builds: `Artifacts/M3_Navigation-TestBuild.log`, `M3_Navigation-LifecycleBuild.log`, `M3_Navigation-FinalBuild.log`, all successful.
- Initial real-path Units run: 10/10 successful, `Artifacts/Automation/M3_Navigation_Obstacle_Units`; seven tests carried warnings. The earlier off-nav fixtures now assert failure/no teleport rather than legacy direct movement. The independent registry acquisition fixture uses melee range; the real-path test covers distant pursuit.
- Final targeted `NavigationPIE`: 1/1 successful, `Artifacts/Automation/M3_Navigation_Final_Targeted`. A full four-point route, actual movement velocity, >500 cm obstacle detour, no obstacle crossing, goal arrival, replacement/callback ordering, Hold, unreachable goal, pursuit to a moved target, attack-range stop, two possession cycles with exact AI identity reuse, death cancellation and EndPlay destruction are checked in a real ticking PIE world. Only transient test actors are created; the test does not teleport the moving unit or manually tick its coordinator.
- Pure `OrderState` assertions cover accepted/rejected command revisions, retargeting and stable pursuit revision.
- Initial saved Recast actors had the engine fallback 35/144 agent settings. Explicit supported-agent configuration plus saving/reloading both maps resolved the registration mismatch. The final targeted run has no navigation or crowd warnings; it retains two host WASAPI notices (48 kHz engine versus 44.1 kHz device, with conversion). No host audio/security setting was changed.
- Editor asset session: `Artifacts/M3_Navigation-AgentAssetEditor.log`; owned editor PID 5932 stopped after successful saves and reload checks. No live editor session is required by the committed game.
- Full regression: `Artifacts/Automation/20260928_151129_468`, 27/27 successful, no failures/pending. Twelve tests have exactly the two host audio notices above; no other result warnings. Log: `Artifacts/M3_Navigation-Full.log`.
- Strict non-unity/no-PCH IWYU: successful, 61 actions (`Artifacts/M3_Navigation-IWYU.log`). Shipping: successful, 7 actions (`Artifacts/M3_Navigation-Shipping.log`). Repository validation, Git LFS integrity and diff whitespace checks also pass.

## Limits and next work

This is not a completed M3 or vertical slice. Formation/shared high-level destination planning, disconnected-island/narrow-passage scenarios, group congestion and 50/100-unit performance, rendered navigation, and a fresh packaged-player gate remain open. No old package proves this revision. The prior additive package's application-control rejection remains recorded in its own review; no blocked executable or OS security policy was modified here. Combat wind-up/hit windows, building validation, objectives, campaign/save, UI/audio/art and release gates remain in the implementation plan.

Skills used: `unreal-cpp-gameplay`, `ue-testing-debugging`, `ue-module-build-system`, `unreal-mcp`; documentation changes follow `ue-project-context` and remain in the approved framework/modules/verification sections.
