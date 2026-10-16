# C-006 — Canonical live settlement and production

Verified 2026-10-05. GPT implementation requested by the user; uncommitted. C-001–C-005 preserved.

## Contract

`FTerritoryCampaignState` owns `FTerritorySettlementState` per registered stable ID. Typed founding rejects unknown/empty IDs, empty/whitespace names, duplicates and int32 production overflow without partial mutation. Unknown reads reset outputs, copies are detached and Reset clears live values. Existing rules retained: free founding, residents 100, bonus Gold 100/Food 50, other four zero. Residents are not population yield. No growth/upkeep/capture rules added.

WorldMapManager exposes ID-based founding/settlement/effective-production reads and a post-commit settlement invalidation event. Generated territories initialize authored StartingSettlementName once through the existing actor wrapper; preplaced registered territories also initialize from the ready catalog. No binary asset changes. Actor settlement fields/bonus removed; HasSettlement/GetSettlementName/GetPopulation/GetResourceProduction delegate. Legacy actor founding notification remains post-commit; duplicate actor founding retains its warning. Destroyed actor lookup returns null; spatial lookup skips invalid actors.

Campaign production enumerates canonical IDs/current owner/yield, not actors. Dormant Gaul and unowned territories still receive no wallet credit. Existing per-territory wallet transaction/pipeline behavior is preserved; no atomic multi-faction full-turn transaction is claimed.

GameState snapshots add live Settlement and EffectiveResourceProduction separately from authored BaseResourceProduction. Readiness/same-world/unknown/teardown guards remain; one detachable settlement relay re-queries. UMG observes only public events, displays live name/residents and six base/effective fields, with a scrollable inspector for longer text. Notifications are invalidations, not an ordered transaction log. Buildings/spatial units/capacity remain tactical actors, not save authority.

## Evidence

| Gate | Result / artifact |
|---|---|
| Editor | Initial 36/36, integration 9/9, capture build 36/36 after build-mode change; `Artifacts/C006-Editor{,-Final,-Capture}.log` |
| Pure settlement | 1/1 clean; `Artifacts/Automation/C006_Pure` |
| Directed campaign/public/UI | 7/7 clean; `Artifacts/Automation/C006_Targeted` |
| Strict IWYU | 69/69; `Artifacts/C006-IWYU.log` |
| Shipping | 12/12; `Artifacts/C006-Shipping.log` |
| D3D12/offscreen | 1/1 successful with one engine warning, no errors; `Artifacts/Automation/C006_Visual` |
| Full NullRHI | **38/38, zero test warnings/errors**; `Artifacts/Automation/C006_Full` |
| Repository/LFS/whitespace | Pass; existing CRLF normalization notices only |

Pure coverage: registration/reset, empty/unknown/duplicate founding, detached copies, owner change preserving settlement, six exact yields, Gold/Food overflow and exact int32 boundaries. Real PIE: nine authored bootstrap names/residents, nested founding/events, no wallet/turn mutation, actor wrapper reads, settlement/yield surviving actor destruction and ownership change, six production credits exactly once per turn, dormant participation guards, UI detach/catch-up and unique relay/unbinding.

Inspected real Slate captures `Artifacts/C006_Visual/ActiveRome.png` and `FoundedSicily.png` (1024×555): live names/residents and separate yields fit within the scrollable inspector. DormantGaul was captured but not newly inspected. Render warning: `r.MotionVectorSimulation` used in render thread without ECVF_RenderThreadSafe, same known engine warning as C-005. No engine/security change, multi-resolution/physical-input/geographic-map/trusted-player claim or new cook/package.

Skills influenced reflected C++ contracts, explicit input rejection, directed tests first and event-based UMG lifecycle. No module/subsystem/dependency/plugin added.

## Next

Bounded M5.1 content/state work is implemented, not the full campaign/player journey. Four gameplay policies explicitly approved 2026-10-05 are recorded in the plan. C-007 addresses M3.5 cancellation refunds using captured paid cost and paying faction, not current prices/owner; refund failure must preserve the order and nested observers must not double-refund.
