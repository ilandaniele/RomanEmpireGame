# C-013 — Tactical terminal behavior and sandbox restart

Date: 2026-10-05. Status: implemented and verified. This is a bounded M3.6/M4 cleanup block, not completion of the implementation plan.

## Contract

- `FTacticalBattleState` remains the only tactical outcome owner. `IsResolved()` is a constant-time derived query; actors do not copy result arrays on every tick or own a second terminal flag.
- Resolution quiesces the tracked living roster through a weak snapshot before publishing its result. Pending melee, navigation, movement forces, shield block, selection and possession flags are cleared. Survivors retain life, registration and collision; terminal cleanup is not a casualty. Untracked world units stop on their next actor tick and all command/damage initiation guards reject immediately.
- Unit commands, direct combat health/damage mutation, new training, construction progression and building combat damage reject after resolution. Production keeps existing paid receipts/queue/progress; automatic terminal refunds or producer-destruction policy are not invented. Explicit cancellation remains a cleanup path on Barracks.
- PlayerController re-queries the canonical result from its GameState event, returns from soldier possession to the original camera, restores pitch and mode context, clears selection and cancels transient input/placement. It removes the observer on EndPlay. No global pause, slow motion or extra input asset is added.
- CampaignManager rejects a new turn before economy/AI steps with appended `ECampaignTurnFailure::EncounterResolved`; direct GameMode calls cannot bypass the UI/controller guard. Campaign outcome is still a separate authority.
- Native UMG re-queries tactical result, displays tactical victory/defeat/draw and exposes a guarded mouse/no-keyboard-focus RESTART ENCOUNTER button. Existing campaign terminal panels retain precedence. Detachment unbinds and disables; reattachment catches up without UI polling. The full-screen background retains a centered fixed content area for predictable native pointer geometry.
- GameMode schedules an actual current-level `OpenLevel` once. This resets the current sandbox, including its world-local campaign actors. It does **not** apply casualties/ownership to a persistent campaign or implement Continue; those require M5.2 persistence/application first. Travel-pending guards are command reentrancy state, not another battle result.

## Tests and evidence

`RomanEmpireGame.Battle.TerminalVictoryRestartPIE` and `TerminalDefeatRestartPIE` exercise fresh actual L_Testbed worlds through public contracts/reflection:

- physical objective timer victory during live soldier possession and all-player-casualty defeat while possessing the final Roman;
- camera/pitch/block/possession/mode cleanup, unique event observers and terminal command/health/damage/production/turn guards;
- detached callback rejection, event-driven reattachment and actual Slate pointer routing (not a UMG delegate invocation);
- repeated restart rejection, real level travel to a different world and fresh 15 Rome/10 Carthage squads, zero carried casualties, new registered objective and resumed tactical commands.

Final directed run: **2/2**, zero result warnings/errors/pending, exit 0 (`Artifacts/Automation/C013_Targeted_Verified`, `Artifacts/C013-Targeted-Verified.log`). Full regression: **42/42**, zero result warnings/errors/pending, exit 0 (`Artifacts/Automation/C013_Full`, `Artifacts/C013-Full.log`). Strict IWYU **70 actions**, Shipping **19 actions**, both succeeded. Editor initial **47 actions** plus incremental test/layout/guard builds succeeded. Repository validation (0 open M1 warnings), Git LFS fsck and diff checks pass.

D3D12/offscreen renderer: **2/2 successful**, one known engine `r.MotionVectorSimulation` render-thread warning, zero errors (`Artifacts/Automation/C013_Visual`, `Artifacts/C013-Visual.log`). Inspected both `Artifacts/C013_Visual/{Victory,Defeat}.png`: the centered title/details and restart button are readable and do not overlap. Opt-in `-RomanCaptureTerminalUI` uses real Slate screenshot capture; this is not AI image generation, broad responsive/accessibility validation or packaged-player evidence.

Initial test issues were corrected: unavailable `AutomationOpenMap` helper replaced by the existing `FAutomationEditorCommonUtils::LoadMap`; two internal non-reflected input predicates replaced with public possession/mode/context observations. Native pointer testing also caught zero-width geometry under NullRHI's stretch layout; the content layout was corrected and directed real travel then passed.

## Files and limits

Runtime: Battle state/subsystem; UnitBase/AI controller; Barracks/BuildingBase; GameMode/PlayerController; MainWidget; CampaignTurn/CampaignManager. Tests remain in the Editor-only existing module. No assets, plugins, input configuration or module dependencies changed.

No new cook or trusted packaged-player evidence. Existing unsigned-player CodeIntegrity restriction remains; do not weaken Windows security. Final multi-resolution accessibility, animations/audio, campaign return/save, producer destruction receipts and the remaining milestone gates are open.

Skills used: `unreal-cpp-gameplay`, `ue-ui-umg-slate`, `ue-testing-debugging`, `ue-project-context`. They preserve the single gameplay authority, event-driven native UI, public-contract real-PIE tests and the user's limited context-update scope.
