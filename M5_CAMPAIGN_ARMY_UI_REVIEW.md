# C-015E2 — Strategic recruitment and assignment UI

Date: 2026-10-05 (America/Montevideo). GPT implementation, uncommitted. This extends the [E1 authorities](M5_CAMPAIGN_ARMY_ACTIONS_REVIEW.md); it does not close M5 or the complete plan.

## Implementation

- Existing controller exposes current-world unit choices, recruitment quotes/commands and assignment eligibility/commands. Eligibility requires the actual ready strategic session. Loaded factories and faction availability come from the existing catalog; there is no UI roster, wallet or reservation authority.
- Native `RomanCampaignArmyActionsWidget.cpp` extends the existing root with unit choice, quantity minus/plus (HUD range 1–100), new/selected-army recruitment, destination-army cycling and assignment. Backend validation remains authoritative and does not inherit the HUD quantity cap.
- Assignment targets are distinct owned co-located army GUIDs. Partial assignment retains the source selection; full assignment retires the empty source and selects the surviving destination. New recruitment selects the actual returned army ID.
- Resources, army/session changes and existing selection/mode events refresh queries without UI Tick or actor scans. Detached, wrong-mode and pending-transfer callbacks cannot mutate state. UI stores only transient choices, clearing them when unavailable.
- Mouse-only buttons preserve gameplay keyboard focus. The strategic panel grows to 1100×660; the inspector remains scrollable. No module, dependency, config, input binding, production asset or ownership architecture changes.

## Verification

Final expanded rendered directed tests: 2/2 successful, exit 0 (one clean, one with the engine render-thread warning below), `Artifacts/Automation/C015E2_Visual_CatalogFixture`, `Artifacts/C015E2-Visual-CatalogFixture.log`. Final full regression: 71/71 clean, zero failures/warnings/pending, exit 0, `Artifacts/Automation/C015E2_Full`, `Artifacts/C015E2-Full.log`. The initial directed logical run was 2/2 clean (`C015E2_Targeted`); the final full suite also executes the expanded fixture under NullRHI.

Editor build 51 actions plus test increment 4 succeeded. Strict IWYU 83 actions and final test increment 4 succeeded (`Artifacts/C015E2-IWYU-Verified3.log`). Shipping 9 actions succeeded (`Artifacts/C015E2-Shipping.log`); late fixture changes affect only the Editor test module, not that runtime build. Repository validation reports zero M1 warnings; Git LFS integrity and whitespace checks pass. No processes remain active.

The two real-PIE `Campaign.ArmyActionsUI.{Victory,Defeat}` journeys route Slate pointer events through the production widget. They cover quantity controls, new/existing purchases, destination cycling, partial/full assignment, retired selection, exact six-field debit, insufficient resources and event-only wallet recovery, real detach/reattach, hostile territory, mode and travel guards, selected-map battle, physical victory/defeat and Continue back to persistent campaign quantities.

Production currently contains only the Legionary unit definition. An explicitly test-only transient duplicate is inserted into the reflected catalog to exercise the real unit-cycle button, then removed before any purchase and again on teardown. Its factory is the loaded canonical factory. No asset is written, no second production unit is claimed. Earlier verification attempts assumed another catalog definition existed and failed; the corrected fixture requires multiple actual choices rather than skipping cycling. Compile errors from a nonexistent result field and an incomplete private actor type were corrected using the public squad seed and reflection/metadata respectively; no private runtime test include was added.

## Evidence limits

Six generated screenshots under `Artifacts/C015E2_Visual/` show resource-disabled, ready and outcome states for both journeys. These are in-process offscreen Slate-root captures, not OS input or a trusted packaged-player run. The engine's `r.MotionVectorSimulation` render-thread warning is recorded without global suppression. The scrollable inspector and fixed-size shell still need M6 responsive/polish work.

No new cook, disk-save, campaign AI, remapping, audio, character or whole-milestone proof. Unsigned packaged-player Code Integrity restrictions are unchanged. Next: C-015E3 canonical Carthaginian defend/recruit/adjacent-attack AI, then C-016 local saves.

## Changed files

- New `Source/RomanEmpireGame/UI/RomanCampaignArmyActionsWidget.cpp`.
- Existing Core `RomanCampaignCommands.cpp`, `RomanEmpirePlayerController.h`; UI `RomanCampaignWidget.cpp`, `RomanEmpireMainWidget.{h,cpp}`; Tests `Private/CanonicalUIIntegrationTests.cpp`.
- This review, plan, README, handoff and only the authorized module/framework/verification context sections.

All earlier uncommitted work and LFS assets are preserved. No staging, commit or push.
