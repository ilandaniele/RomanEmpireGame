# M5.1 — Territory tactical-map references (C-002)

Date: 2026-10-03. Baseline HEAD remains `0726de2`, with the earlier H-001/C-001 work preserved. C-002 adds content and validation for future campaign-to-battle transitions; it does not implement travel or close M5.1.

## Implementation

All nine existing territory PDAs explicitly reference `/Game/Maps/L_VerticalSlice.L_VerticalSlice` through `URomanTerritoryDefinition.TacticalMap`, a `TSoftObjectPtr<UWorld>`. This reuses the configured game map and the plan's one deterministic tactical scenario. The canonical reference remains on the content asset, not a duplicate mutable territory actor field or save-time actor pointer.

`HasValidTacticalMap()` queries Asset Manager metadata and requires an exact existing top-level World object path. Empty, missing, wrong-class, subobject and generated-class-style paths reject; unresolved manager initialization also fails closed. Editor data validation and runtime content-set validation share this check (`InvalidTacticalMap` report). No `TryLoad`, `LoadSynchronous` or world travel occurs in this validator. Redirected paths must be reauthored to their canonical destination rather than silently accepted. No dependency, module, subsystem or runtime fallback path was added.

The soft-reference choice follows [Epic's asset loading guidance](https://dev.epicgames.com/documentation/unreal-engine/asynchronous-asset-loading-in-unreal-engine); loading the map for an actual encounter remains future M5.2 work.

## Asset authoring

`Tools/author_territory_tactical_maps.py` is an Editor-only helper for the exact nine known slice territories. It checks asset class/ID and conflicting resolved map references before saving; `--apply` opts into writes, while omission verifies without saving. It intentionally loads the existing World in the authoring process so Python can assign the correctly typed property. This is not a runtime-registry load.

Pre-C-002 copies, already containing C-001 adjacency, are under `Artifacts/C002_TerritoryBackup_20261003_235159`. Only nine territory packages were saved. No map, graph, owner, production, settlement, unrelated asset or host-security configuration was modified. A separate fresh Editor process verified all saved references. Existing AlwaysCook territory rules and the configured default map already cover this slice; no config change was needed.

## Verification

- Editor build: 12/12, `Artifacts/C002-Editor.log`.
- Authoring and separate disk readback: exit 0, `TACTICAL_MAP_OK regions=9`; `Artifacts/C002-Authoring.log`, `Artifacts/C002-Readback.log`.
- Targeted content/UI: 4/4, zero test warnings/errors; `Artifacts/Automation/C002_Targeted`, `Artifacts/C002-Targeted.log`.
- Strict non-Unity/no-PCH build: 67/67, `Artifacts/C002-IWYU.log`.
- Shipping: 6/6, `Artifacts/C002-Shipping.log`.
- Full regression: 37/37, zero test warnings/errors; `Artifacts/Automation/20261003_235701_322`, `Artifacts/C002-Full.log`.
- Development cook retry: 567 packages, exit 0, no cook warnings/errors; `Artifacts/C002-Cook-Retry.log`. The runtime Asset Registry output is present; this is cook evidence, not a packaged-player execution of the metadata validator.
- Repository validation, LFS fsck and whitespace checks pass; the territory assets retain the existing LFS policy. No map binary changed.

The new `TacticalMapReference` test covers bad path/type/subobject/generated-class references, Editor data validation and unchanged World residency across both validators. Content-set failure coverage checks missing references; discovery asserts the exact saved map on every territory and retains C-001's 81-pair graph regression. Real PIE verifies registry startup and existing UI/campaign behavior.

The first cook attempt ended before cooking because UAT's build overlapped the full-test build (`ConflictingInstance`, mutex, exit 10). This was scheduling contention, not a missing SDK or asset defect. The retry started after the test build had exited and passed; do not run UAT builds concurrently with another UBT invocation. All processes are finished.

Skills used: `ue-data-assets-tables` guided typed soft references and metadata-only validation; `unreal-cpp-gameplay` guided reflection/rebuild; `ue-testing-debugging` guided focused negative/content/PIE coverage. No MCP connection was assumed.

## Remaining scope

C-003: initialize only Rome and Carthage as active slice factions, retaining Gaul's content definition for future expansion. Targeted inspection found `AFactionManager` currently creates wallets/info for every registered faction and starts Rome–Gaul at War. Define and test active-faction guards consistently for player faction, diplomacy and AI; inspect exact authored starting-owner IDs before deciding how inactive-faction territories should be represented. Do not delete the third PDA or force discovery count down from three.

Strategic data-backed UMG, army/action/AI contracts, battle-to-campaign application and save/load still remain. This block does not prove fresh packaged-player startup, rendered fit, battle travel or the complete campaign journey. The earlier Windows Code Integrity limitation is unchanged; no rejected player execution was retried.
