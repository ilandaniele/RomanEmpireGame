# C017A — registered-state minimap

2026-10-06. M6.2 implementation; M6.1 responsive shell and whole M6/release gates remain open.

Follow-up C017B: M6.1 is now closed in [responsive-shell review](M6_RESPONSIVE_SHELL_REVIEW.md). Initial B captures replaced only generated `Artifacts/C017A_Visual/{Tactical,Strategic}.png`; the original A screenshots were inspected at the time of this historical evidence, but current files show B's initial layout. Final current rendered matrix is `Artifacts/C017B_Responsive`, not those old paths. Whole M6/release remains open.

## Contract

The native shell owns tactical and strategic placements of the same `UMinimapWidget` class. Only the currently visible presentation is active. No new gameplay owner, external dependency, asset, input binding or server is introduced.

- Tactical: validated `URomanUnitRegistrySubsystem::GetRegisteredUnits()` weak-index snapshot, current actor positions/factions/selection, the single registered tactical objective and the actual controlled Pawn position. Living routed units remain visible; death/removal eliminates their markers. Square means player-owned, X means other, white diamond means selected; objective uses a diamond, camera/Pawn a cross. Canonical faction colors supplement shapes, not the sole distinction.
- Strategic: public detached territory snapshots supply stable IDs, authored positions, current ownership/colors and reciprocal adjacency. Lines are authored graph edges, not invented geographical borders. No copied territory owner, army inventory or result.
- A square world fit is fixed per active presentation, so movement cannot continuously rescale it. Out-of-bounds values clamp; coordinate orientation explicitly says +X right/+Y down. Finite nondegenerate custom bounds reject atomically otherwise. Projection helpers fail closed for invalid/nonfinite inputs.
- Native committed registry invalidation refreshes spawn/owner/death/removal. Reindexing removes the old bucket/membership and publishes only after both indexes agree; no intermediate remove notification. Tactical movement/selection/camera sample the index at 5Hz; strategic has no polling timer. No UI Tick or runtime global actor scan.
- Activation requires the actual owning shell in the viewport, local current-world controller/GameInstance and initialized GameState. Deactivation/destruction removes native/dynamic observers, cancels movement timer, clears detached markers/edges and resets fit. Stale activation after parent detach is rejected; reattach re-queries.

This is a schematic information display, not a terrain render target, fog-of-war system, camera frustum, click-to-navigate feature or finished visual theme. Old unreferenced manual-marker/camera-box placeholder APIs were removed; canonical state is the only source.

## Files

- `Public/UI/MinimapPresentation.h`, `UI/MinimapPresentation.cpp`: detached marker values + finite geometry.
- `UI/MinimapWidget.h/.cpp`: native paint, indexed sampling, event/query lifecycle.
- `Public/Units/RomanUnitRegistry.h`, `Units/RomanUnitRegistry.cpp`: validated snapshot + native committed invalidation; no alternate unit authority.
- `UI/RomanEmpireMainWidget.h/.cpp`: active tactical placement and strategic placement below region cards, explicit detach/mode lifecycle.
- `Tests/Private/MinimapPresentationTests.cpp`, `CanonicalUIIntegrationTests.cpp`: pure projection and owned real-PIE integration; existing tests unchanged.

## Verification — closed

- Initial Editor64 and final fixture Editor4 builds pass. Strict non-unity/no-PCH IWYU93 passes, log `Artifacts/C017A-IWYU.log`.
- Final directed `RomanEmpireGame.UI.Minimap`: 2/2 clean, exit0, report `Artifacts/Automation/C017A_Targeted_Final/index.json`, log `Artifacts/C017A-Targeted-Final.log`. Pure finite/clamp/round-trip/invalid cases; actual initialized deferred spawn, owner reindex/no duplicate, indexed movement, selected shape, death before destruction, nine canonical territories/ownership refresh, real detach/GC/reattach, one active listener, stale activation rejection and destroyed registered objective removal.
- Initial test compilation corrected fixture strong/weak pointer usage. First runtime fixture failed because it bypassed mandatory deferred definition initialization; corrected to the existing gameplay factory contract, not relaxed runtime validation. Final tests preserve all checks.
- Rendered2 successful, exit0: `Artifacts/Automation/C017A_Visual/index.json`, `Artifacts/C017A-Visual.log`; one clean + one known UE engine `r.MotionVectorSimulation` render-thread warning, no suppression. Both actual `Artifacts/C017A_Visual/{Tactical,Strategic}.png` inspected: unit/selection/objective and canonical region markers/edges display. The debug diagnostic panel currently shares tactical top-right space; production responsive/legibility work remains C017B. Screenshots are real Editor rendering, not mockups or OS/player proof.
- Fresh full97/97 clean, exit0, no warnings/failures/pending/notRun: `Artifacts/Automation/C017A_Full/index.json`, `Artifacts/C017A-Full.log`. Includes all final runtime/test sources and earlier gameplay/persistence regression. Shipping17 succeeds in `Artifacts/C017A-Shipping.log`; strictIWYU93 succeeds. Repository0openM1warnings `Artifacts/C017A-Repository.log`, LFSfsck and diff checks pass. All operations terminal, no active process retained.

## Next

C017B/M6.1 responsive shell: fit/scroll panels and toast/header/safe-area at target resolutions without changing canonical gameplay. M6.3–M6.9 production content/audio/accessibility and M7–M8 gates remain. Trusted packaged-player proof is still open. This block used the UMG, gameplay, testing, module-build and project-context skills; context edits stay within the three user-authorized sections.
