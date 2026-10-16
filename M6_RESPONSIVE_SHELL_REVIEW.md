# C017B — responsive native shell

2026-10-06. M6.1 implementation. Whole M6–M8 and trusted packaged-player gates remain open.

## Contract

One controller-owned native UMG tree retains all existing canonical commands, widgets, event bindings and guards. Construction reparents presentation only; no new gameplay owner, layout polling/UI Tick, plugin, asset, setting file or input binding.

- Fullscreen SafeZone/flex header separates wrapping six-currency balances from the single-line phase/turn column.
- Objectives, save/status/diagnostics/feedback and tactical minimap have separate non-overlapping columns. Only the active minimap frame occupies space.
- Strategic regions and the complete inspector/army-action document scroll independently inside the available body. Stable-ID region labels wrap within their actual cards; complete canonical tooltip remains.
- Selection/production scroll in a weighted footer beside contextual controls and existing commands. Possession uses a separate dark-backed vitals panel, without simultaneous selection overlap.
- Construction choices scroll; the complete terminal outcome scrolls in a proportional fullscreen-modal inset. Existing readiness, ownership, eligibility, detach/current-world and transaction guards are unchanged.
- No pointer gameplay command depends on a hand-maintained Canvas hit rectangle. Canvas remains only the root stretch, proportional outcome inset and centered crosshair arrangement.

Targets: Windows1280×720/1920×1080/2560×1440, project DPI curve with ApplicationScale1.0/1.25. These are the proposed defaults; the optional target-resolution question has no answer. This does not prove ultrawide/gamepad/touch/localization or the complete accessibility/settings milestone.

## Files

- New `UI/RomanResponsiveShell.cpp`; existing MainWidget.h/.cpp own construction and event visibility.
- Existing `UI/RomanCampaignSaveWidget.cpp` no longer assumes its panel is a fixed Canvas child or repositions it on state refresh.
- Existing `Tests/Private/CanonicalUIIntegrationTests.cpp` adds six actual-window responsiveness cases and strengthens the owned PIE viewport fixture.

## Test fixture corrections

The old headless docked editor SViewport/SGameLayerManager had zero size under NullRHI. Fixed Canvas slots previously produced positive button geometry despite an unusable zero viewport. A transient-settings, actual PIE window now has exact borderless requested client bounds, independent of desktop clamping/titlebar/DPI initialization. All this file's owned sessions reuse it; no saved editor preferences/config or forged per-button geometry.

Input still routes virtual Slate press/release through the generated live-window SButton path, not direct OnClicked broadcasts. The matrix also verifies scrolled centers inside actual scroll/viewport bounds, not only positive offscreen rectangles. Region widgets are found by canonical ID rather than transient unique UObject name. Active training is checked separately from pending queue length. The prior fixed-header/card nonwrap assertions were updated to the new explicit responsive contract; runtime guards were not weakened.

## Evidence

- Final directed UI10/10 clean: `Artifacts/Automation/C017B_UI_Legibility`, `Artifacts/C017B-UI-Legibility.log`.
- Each matrix case checks requested pixel bounds, positive contained shell/panels, vertical non-overlap, canonical training/refund via pointer, wrapped card bounds, stable-ID region pointer inspection, scrolling army controls, construction menu, actual possession and actual physical objective capture/terminal restart accessibility.
- Final rendered matrix6 successful:5clean+1known engine `r.MotionVectorSimulation` render-thread warning, no failures. `Artifacts/Automation/C017B_Rendered_Final`, `Artifacts/C017B-Rendered-Final.log`;30 actual PNGs in `Artifacts/C017B_Responsive`. Seven final captures inspected: all five layouts at1280×720/125%, strategic1920×1080/100% and2560×1440/125%.
- Final full103/103 clean, zero warnings/failures/pending/notRun: `Artifacts/Automation/C017B_Full`, `Artifacts/C017B-Full.log`. Covers current responsive runtime and all previous canonical save/army/session/first-playable/minimap tests.
- Editor/fixture/legibility builds, strictIWYU94 and Shipping pass; logs `Artifacts/C017B-{Editor-Legibility,Editor-LegibilityTests,IWYU,Shipping}.log`. Repository0M1warnings/GitLFSfsck/diff checks pass. All handles terminalexit0, no active process retained. No new cook or trusted-player proof.

Initial B minimap captures accidentally replaced generated `Artifacts/C017A_Visual/{Tactical,Strategic}.png`. A was genuinely inspected before that historical report; current files are B initial layout, not original A images. The minimap capture helper now accepts `-RomanMinimapCaptureDir` for unique future folders. Only generated ignored screenshots were replaced, no source/asset lost.

Skills: `ue-ui-umg-slate` informed flex/scroll/lifecycle/contrast; `ue-testing-debugging` informed owned latent fixtures and real-input assertions; `ue-project-context` updates only the three already-approved sections. No claim of a completed visual theme, required assets/audio/animation, remapping/settings, full M6 or distribution.
