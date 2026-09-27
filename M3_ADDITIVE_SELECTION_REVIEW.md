# M3 additive selection and interaction feedback

Date: 2026-09-27. Baseline: `e5825e5`. Scope: remaining additive-input/feedback portion of M3.2, not overall M3 completion.

## Behavior and ownership

- Plain click/rectangle replaces selection. Shift-click toggles eligible units once; Shift-rectangle adds without duplicates. The modifier is sampled at gesture start. Empty additive gestures preserve selection; plain empty gestures clear it.
- Buildings remain exclusive selections. Invalid unit/building clicks preserve the current eligible selection and display feedback. Invalid attack-target requests also display feedback.
- Replace/Add/Toggle feed the existing controller commit path, including ownership revalidation, presentation and observer cleanup. No second selected-set owner, persistent squad identity or save schema is introduced.
- UMG observes a controller event. A non-interactive warning expires after three seconds and clears on a valid selection or widget teardown. No UI tick or world scan is added.
- Boolean `IA_SelectionModifier` has LeftShift/RightShift mappings and highest-absolute accumulation. The saved config contains 20 distinct actions; Tactical contains 18 mappings, preserving its prior 16. Physical keys remain asset-owned.
- Menu, placement, mode and focus boundaries suppress/cancel additive input. Querying the evaluated action also requires a held live resolved mapping, avoiding a stale action immediately after focus flushing while preserving remaps.
- Native HUD help and `INPUT_CONTROLS.md` describe the defaults.

## Verification

| Gate | Evidence |
|---|---|
| Initial targeted regression | `Artifacts/Automation/M3_Additive_Targeted`: 4 pass, 1 failure; synchronous focus-flush assertion caught cached Shift state |
| Corrected targeted regression | `Artifacts/Automation/M3_Additive_Fixed_Targeted`: 6/6, no result warnings/failures/pending |
| Full regression | `Artifacts/Automation/20260927_142020_752`: 25/25, no result warnings/failures/pending; `Artifacts/M3_Additive-Full.log` |
| Strict non-unity/no-PCH IWYU | 60 actions, success; `Artifacts/M3_Additive-IWYU.log` |
| Shipping compilation | 6 actions, success; `Artifacts/M3_Additive-Shipping.log` |
| Repository and LFS | `Tools/validate_repository.ps1`, `git lfs fsck` and `git diff --check` pass |

`PhysicalKeysAndModeLifecyclePIE` exercises each Shift key, overlapping holds/releases, mode rebuild suppression, menu/placement rejection and synchronous focus flushing in the real Enhanced Input stack. `AdditiveSelectionPIE` covers deduplication, toggle/replace/add, eligibility, canonical cleanup/reentry and live UMG visibility/expiry/destruct. `LiveRegistryPIE` exercises additive rectangles using actual camera projection. `Buildings.PlayerSelectionPIE` covers owned building selection, non-toggle behavior, rejected enemy preservation/feedback and empty gestures.

The initial failure is retained as evidence; it is not relabeled as passing. No deliberate code mutation was needed to expose it.

## Asset operations and recovery

The three input assets were saved through the UE editor's sequential loopback MCP tools with readback of all prior mappings. The owned editor was stopped after confirmed saves; the listener is closed. No engine/plugin defaults, Windows security/driver settings, runtime dependencies or external services changed. Optional Fab was disabled only in test/package processes.

Code, docs and the three LFS assets form one local change; rollback must keep the required config reference and saved assets consistent. Tests use transient fixtures and do not save assets.

## Packaged-player gate

Fresh Development archive `Artifacts/Packages/M3_Additive` passes UAT (exit 0; `Artifacts/M3_Additive-Package.log`), processing 567 packages with no cook warning/error lines. Binary SHA256: `e977724423984b3374b70e546ee3cf74be3028a7613c1e92cc33569831f6dad8`.

The actual-player attempt through `Tools/verify_unit_presentation.ps1 -ArchiveDirectory Artifacts/Packages/M3_Additive -OutputDirectory Artifacts/Additive-Presentation` was blocked before process startup. Windows Code Integrity events 3077 and 3033 at 14:24:41–42 local time identify this exact archived executable as failing signing requirements. Authenticode reports `NotSigned`; the file has no Zone.Identifier stream. `Artifacts/M3_Additive-Presentation.log` preserves the hash/scenario; the tool output and Windows event log contain the launch rejection. No security setting was changed, and the same blocked binary was not repeatedly launched.

`Tools/verify_additive_selection.ps1` adds an explicit archive/fresh-output gate with selection identity/count/order assertions and Added/Feedback captures; its synthetic parser tests are not runtime evidence. Actual-player selection/possession and additive/feedback gates require a trusted/signed build or controlled trusted runner. Historical packages do not prove this revision.

Separate editor-game preview: `Tools/verify_additive_selection.ps1 -EditorPreview -OutputDirectory Artifacts/Additive-EditorPreview-Final` passes selection identity/count/order, repeated Add, empty Add preservation, empty Replace clear and reflected feedback text/visibility, exiting 0. Both 1920×1080 images were inspected: 15 selected units show green rings and the correct count; the default Shift help and invalid-attack warning are legible without overlap. This runs the installed Editor with `-game`, not the packaged executable, and cannot close the packaged-player gate.

The initial editor preview is retained at `Artifacts/Additive-EditorPreview`: its strict parser rejected Python initialization errors from editor-only toolsets unavailable in game mode. The final preview uses the engine's process-only `-DisablePython` flag, as no scripting bridge is needed. Editor Data Storage widget-factory warnings and the existing render-thread `r.MotionVectorSimulation` warning remain in the preview log; it is not a warning-free editor run. The parser's self-test passes valid selection/feedback fixtures and 11 expected rejection cases.

## Limits and remaining work

This does not certify physical desktop drag/focus behavior, gamepad/remapping UI, Shipping-player execution, performance, squads, routing, navigation, combat windows, placement or the complete game loop. M3.1 routed policy and M3.2 squad identity/membership remain open. The first playable and later milestones remain required by `IMPLEMENTATION_PLAN.md`.

Skills used: `ue-input-system`, `unreal-cpp-gameplay`, `ue-ui-umg-slate`, `ue-testing-debugging`, `ue-module-build-system`, `unreal-packaging`, `unreal-mcp`, `ue-data-assets-tables`, `ue-project-context`.
