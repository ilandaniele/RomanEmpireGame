# M3.6 tactical casualties in public GameState and UMG

Status: live squad survivors/casualties now reach the existing native HUD through the public GameState's read/event projection. This advances M3.4's death-to-UI path and M3.6's encounter presentation, but capture, terminal battle controls and campaign resolution remain open.

`ARomanEmpireGameState` keeps only a weak reference to `UTacticalBattleSubsystem`; it exposes `GetTacticalBattleResult` and relays committed change notifications after re-querying the source. The widget binds only to that public event, never directly to the tactical subsystem, and also re-queries on construct/reattachment. Its objective panel shows player/foe alive-versus-roster counts and the current capture prompt or terminal word. No widget tick, mirrored kill counter or second victory evaluator was added. The campaign outcome panel remains separate; a tactical result does not silently become a campaign win.

The real-PIE UMG test checks one relay and one root subscription, removal/re-attachment without duplicates, initial 15/10 counts, a trained reinforcement appearing in the roster, and a defender death changing visible text synchronously. An initial hard-coded 16/16 assertion failed because an original Roman also died during live simulation; the corrected assertion compares the displayed survivors to the canonical result rather than assuming a casualty-free interval. This is evidence of event-driven projection, not rendered pixel-fit proof.

## Evidence

- `RomanEmpireGame.UI.CanonicalEventDrivenPIE` targeted 1/1 (`Artifacts/Automation/M3_BattleProjection_Fixed`), log `Artifacts/M3_BattleProjection-FixedTargeted.log`; the earlier failed assumption is retained in `Artifacts/M3_BattleProjection-DiagTargeted.log`.
- Full regression 35/35, zero warnings/failures, `Artifacts/Automation/20260930_134907_222`, log `Artifacts/M3_BattleProjection-Full.log`.
- Strict non-unity/no-PCH IWYU 66/66 actions, `Artifacts/M3_BattleProjection-IWYU.log`; Shipping Game compilation 5/5 actions, `Artifacts/M3_BattleProjection-Shipping.log`. Repository validation, LFS integrity and diff whitespace checks pass.

No asset, module, plugin or external service was added. Packaged-player and rendered layout verification are still pending. Skills applied: `ue-ui-umg-slate` for event-bound UI lifecycle and `ue-testing-debugging` for real-PIE source-to-label assertions.
