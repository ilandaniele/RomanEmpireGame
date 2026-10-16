# M5.1 — Stable campaign adjacency (C-001)

Date: 2026-10-03. Baseline: `0726de2`. Status: adjacency implemented and verified; M5.1 and the full implementation goal remain partial. Changes are local and uncommitted.

## Contract

`FTerritoryDefinition.AdjacentTerritoryIDs` authors an undirected graph in the existing nine Primary Data Assets. `FTerritoryCampaignState`, still owned solely by `AWorldMapManager`, copies the definitions and supplies sorted stable-ID queries. Ownership changes, returned-array edits and actor movement cannot change connections. No cached second graph, distance-based runtime fallback, new dependency or new subsystem was introduced.

Local definition validation rejects empty, self and duplicate neighbor IDs. Content-set validation additionally rejects unknown and nonreciprocal references with `InvalidTerritoryReference`. Forward registration is allowed, but adjacency queries fail closed until the entire registered graph is reciprocal and known. Isolated territories are valid. Existing actor wrappers resolve only registered valid actors; new campaign consumers can query IDs directly without actor pointers.

## Authored migration

The migration preserves the legacy map's connectivity exactly, including its unusually connected Rome hub. This is preservation of existing geography, not a new realism/balance decision.

| Territory ID | Neighbor IDs |
|---|---|
| Africa | Hispania, Rome, Sicily |
| Alps | Gaul, Greece, Hispania, Macedonia, Rome |
| Egypt | Macedonia, Rome, Sicily |
| Gaul | Alps, Hispania, Rome |
| Greece | Alps, Macedonia, Rome |
| Hispania | Africa, Alps, Gaul, Rome, Sicily |
| Macedonia | Alps, Egypt, Greece, Rome, Sicily |
| Rome | Africa, Alps, Egypt, Gaul, Greece, Hispania, Macedonia, Sicily |
| Sicily | Africa, Egypt, Hispania, Macedonia, Rome |

There are 20 undirected connections / 40 directed entries. `Tools/migrate_territory_adjacency.py` is a one-time Editor migration: it preflights all assets, refuses conflicting authored edges and saves only on explicit `--apply-legacy`. It derives this initial snapshot from the existing asset locations and the legacy 15,000-unit threshold; runtime does not use that threshold. Native reflected property names avoid Python's acronym-conversion ambiguity. A second fresh Editor process without the apply flag verified persisted data.

Exact nine-asset backup: `Artifacts/C001_TerritoryBackup_20261003_233940`. No map or unrelated asset was saved. All nine territory binaries retain the Git LFS filter. Locations, owners, production and settlements were preserved by modifying only the neighbor field in each existing definition.

## Verification

- Editor build: 28/28 actions, `Artifacts/C001-Editor.log`.
- Migration and separate disk readback: exit 0, `CAMPAIGN_ADJACENCY_OK regions=9 directed_edges=40`; `Artifacts/C001-Migration.log` and `Artifacts/C001-Readback.log`.
- Targeted campaign/content/UI: 7/7, zero test warnings/errors; `Artifacts/Automation/C001_Targeted`, `Artifacts/C001-Targeted.log`.
- Full regression: 36/36, zero test warnings/errors; `Artifacts/Automation/20261003_234317_056`, `Artifacts/C001-Full.log`.
- Strict non-Unity/no-PCH build: 67/67, `Artifacts/C001-IWYU.log`.
- Shipping: 6/6, `Artifacts/C001-Shipping.log`.
- Development cook: 567 packages, exit 0, no cook warnings/errors; `Artifacts/C001-Cook.log`.
- Repository validation, Git LFS fsck, all nine LFS attributes and whitespace check pass.

The new `StableAdjacency` test covers incomplete/unknown/one-way graphs, local rejection, reciprocal/island/self queries, deterministic order, defensive copies, owner changes, reset and atomic rejection. Content tests cover bad references and every one of the 81 original map pairs. Real `CanonicalEventDrivenPIE` moves a campaign neighbor one million units away, asserts actor and ID queries are unchanged, then restores its position; self/null queries also reject.

The map-pair assertion is an explicit migration-preservation check, not a runtime spatial rule. Future intentional graph/geography changes must revise that fixture. Existing Engine startup self-test messages (`Condition failed`) and editor-layout warning also occur in the prior passing baseline; they are outside project test reports. This review does not claim a globally warning-free Editor startup.

Skills applied: `ue-data-assets-tables` guided stable IDs and asset validation; `unreal-cpp-gameplay` kept one campaign authority; `ue-testing-debugging` guided pure/PIE regression and gates. The migration used the installed Editor Python commandlet, not an unverified MCP connection.

## Remaining scope

Next C-002: explicit soft tactical-map reference in territory content, validation and asset authoring. M5.1 also needs the two-active-faction policy and strategic data-backed UMG. Campaign armies/actions/AI, atomic battle-to-campaign application and saves remain M5.2–M5.4. No fresh package/player, rendered interaction, save/load or whole-M5 completion is claimed. The recorded Windows Code Integrity limitation remains unchanged; no security setting was changed and the rejected unsigned player was not retried.
