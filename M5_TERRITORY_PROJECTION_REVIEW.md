# M5.1 — Public territory projection (C-004)

Date: 2026-10-04 (America/Montevideo). HEAD remains `0726de2`. Existing H-001/H-002 and C-001–C-003 changes are preserved, uncommitted. This block supplies the strategic UI read/event contract, not the widget or campaign actions.

## Contract and authority

`ARomanEmpireGameState.GetTerritorySnapshots()` returns fresh `FTerritoryPresentationSnapshot` values in lexical stable-ID order. `TryGetTerritorySnapshot(ID, OutSnapshot)` returns false and clears every output field for an unknown ID, missing/not-ready authority or content registry, foreign-world map/faction authority, or an ended state. An unowned registered territory is a successful read, distinct from an unknown territory. Lists fail closed rather than silently exposing a partial catalog.

`FTerritoryCampaignState.GetTerritoryIDs()` enumerates its registered definitions; WorldMapManager forwards that canonical query. UI does not enumerate actors or assume every catalog asset is registered. Territory name, description, terrain, bonus and base production come from the registered definition. The sorted neighbor graph and live owner come from WorldMapManager. Map coordinates and owner label/color come from immutable catalog metadata, not actor transforms or active-faction info. `bOwnerActive` queries the existing FactionManager; dormant Gaul retains its name/color/ownership without gaining a wallet or participation. Unowned/noncatalog owners have an empty label and neutral white color; the owner enum is retained.

The snapshot has no actor, UObject asset, army, settlement, save identity or loaded World pointer. Its six-field `BaseResourceProduction` is authored production, **not effective settlement production or a turn credit**. Callers may edit/discard their detached C++ values without mutating gameplay. No rows or territorial state are cached on GameState and no new gameplay owner/subsystem is introduced.

`OnTerritoryOwnerChanged(ID, PreviousOwner, NewOwner)` is bound once during normal GameState initialization and removed on EndPlay. It preserves the source signature but re-queries current ownership before publishing. This is an invalidation event, not a monotonic transaction log: `PreviousOwner` is historical and `NewOwner` is current at relay time. A later observer can also mutate synchronously; consumers must re-query. No-op/rejected ownership transactions emit no event. A future strategic widget must also catch up on construct/readiness and mode/player changes using the existing public events.

## Tests and verification

- Editor final: 7/7, `Artifacts/C004-Editor-Final.log`. The initial build caught mixed `NAME_None`/FName initializer types and an `AActor::Owner` shadow; both were fixed before tests. Original failure log: `Artifacts/C004-Editor.log`.
- Initial directed: 6/6, `Artifacts/Automation/C004_Targeted`, `Artifacts/C004-Targeted.log`.
- Final directed, including foreign-world and Blueprint contract coverage: **6/6, zero test warnings/errors**, `Artifacts/Automation/C004_Targeted_Final`, `Artifacts/C004-Targeted-Final.log`.
- Strict non-Unity/no-PCH Editor build: **67/67**, `Artifacts/C004-IWYU.log`; also compiles final test additions.
- Shipping: **10/10**, `Artifacts/C004-Shipping.log`. Later additions are Editor-only tests; runtime is unchanged after this gate.
- Full regression: **37/37, zero test warnings/errors**, `Artifacts/Automation/C004_Full`, `Artifacts/C004-Full.log`; exit 0. All C-004 processes finished; no pending handles.
- Repository validation, LFS fsck and diff whitespace pass. Line-ending notices on pre-existing CRLF runtime-private files are not whitespace failures.

The strengthened `StableAdjacency` verifies ordered canonical ID enumeration, detached return arrays and Reset. `UninitializedReadContract` checks empty/reset territory reads and reflected BlueprintPure/Const APIs. Real `CanonicalEventDrivenPIE` verifies all nine rows and every production field against their authorities, unknown-vs-unowned reads, Gaul catalog metadata and activity, current ownership after mutation, caller edits, deterministic ordering and unchanged snapshots after a visual actor moves one million units. It rejects a populated foreign-world map sharing the same GameInstance catalog, destroys/collects that auxiliary world, and checks an unbegun deferred state. An earlier ownership observer performs a nested Rome→Carthage→Rome transaction: both relays publish the final Rome owner, not a stale outer Carthage value. Listener counts, no-op/rejection silence, widget detach/reattach and GameState EndPlay/no-later-events are checked in the real lifecycle.

Skills: `unreal-cpp-gameplay` kept reflection and projection in the existing GameState/WorldMap authority; `ue-testing-debugging` guided public/reflected contract tests without runtime-private includes. `ue-project-context` updates only the authorized framework/verification sections; no context regeneration. No MCP session, saved binary, module/dependency, config or Windows security change; no commit, staging or push.

## Remaining scope / next C-005

M5.1 remains partial until data-backed strategic UMG renders these rows. Start in `UI/RomanEmpireMainWidget.{h,cpp}` and its event bind/unbind/mode presentation, using this public contract only. Add nine data-backed territory controls and inspection of owner/activity, terrain, neighbors and explicitly labeled base production; selection here is local presentation, not an army/save authority. Gate actual selection through Slate/public UI tests, ownership-driven refresh, Strategic-only visibility, detach/reattach and teardown without polling. Keep existing Tactical/Possessed UI behavior. Read `ue-ui-umg-slate` before that implementation if available.

Settlements remain actor-backed; do not display StartingSettlementName as live state or invent a settlement owner. Recruitment/armies/movement/battle travel/result application/save/load are later bounded contracts. No fresh cook/package, player, rendered pixel-fit or full-M5 completion is claimed. C-002's cook and Windows Code Integrity limitations remain historical/unchanged. The four unanswered design policies still do not block the independent strategic read-only UI.
