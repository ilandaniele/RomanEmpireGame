# M3 tactical squad membership

Date: 2026-09-28. Baseline: `20162fa`. Implements the tactical membership portion of M3.2; not persistent campaign armies or M3 closure.

## Contract and integration

`URomanUnitRegistrySubsystem` now owns tactical membership alongside its existing live-unit index. There is no second squad owner. Units retain canonical faction/lifecycle and are referenced weakly. Each unit may belong to one group; all members must be registered, live, initialized, in the same world and have the same non-None faction.

`CreateSquad` validates the entire request before changing indexes, rejects already-grouped members and deduplicates input in first-occurrence order. It returns a new world/session-local GUID. The GUID is not a save identity; campaign-to-battle identity mapping remains M5 work. Read APIs return validated snapshots. `DisbandSquad` removes membership only, preserving actors, orders and selection.

The existing actor registration boundaries also remove squad membership before death/ownership/EndPlay observers. Same-faction assignments preserve it. The last removed member retires the group; world teardown clears both directions of membership. No per-frame update, actor-world enumeration, asset or module dependency is added.

GameMode creates one group for each initial faction force from its actual successful spawns. Barracks puts a newly trained unit in a singleton group. This does not auto-merge recruits, change per-unit selection, implement formation movement or define a campaign recruitment policy.

## Verification

- Initial targeted run: `Artifacts/Automation/M3_Squads_Targeted`, 2/2 (`SquadRegistryPIE` and existing `LiveRegistryPIE`), no result warnings/failures.
- Strengthened final targeted run: `Artifacts/Automation/M3_Squads_Final_Targeted`, 1/1, no result warnings/failures/pending.
- Full regression: `Artifacts/Automation/20260928_000739_201`, 26/26, no result warnings/failures/pending; log `Artifacts/M3_Squads-Full.log`.
- Strict non-unity/no-PCH IWYU passes (60 actions), `Artifacts/M3_Squads-IWYU.log`.
- Shipping compilation passes (6 actions), `Artifacts/M3_Squads-Shipping.log`.
- Repository validation, Git LFS and final diff checks pass.

`SquadRegistryPIE` uses real initialized actors and production. It verifies initial-force membership; invalid/null/mixed/deferred/unowned input; duplicate removal and insertion order; atomic rejection when a later member is already grouped; stable membership on same-owner assignment; disband preserving Hold/live registration; fresh handles; owner callbacks observing detached state and successfully creating a new group; death callbacks observing singleton retirement and rejecting recreation; EndPlay cleanup; true GC serial/index invalidation; trained-unit singleton membership; world isolation and actual world teardown rejecting subsequent writes. All fixtures are transient and no assets are saved.

## Limits and recovery

No new package/player evidence is claimed for this source-only block. The preceding additive package is blocked by host Code Integrity; its separate editor preview is not a substitute for actual packaged runtime validation. See `M3_ADDITIVE_SELECTION_REVIEW.md` for the exact executable and events.

This is not campaign persistence, formation/path following, whole-squad selection, squad AI, spatial indexing or performance proof. Navigation/formations remain M3.3; typed battle results and campaign mapping remain later packages. Runtime handles must not be stored as campaign IDs.

Rollback must keep the registry API and its GameMode/Barracks callers together. No external state, dependencies, engine/plugin defaults or security settings changed; no remote push.

Skills: `unreal-cpp-gameplay`, `ue-testing-debugging`; existing module/IWYU conventions are preserved. The project-context skill limits documentation updates to the already-approved framework/subsystem, module and verification sections.
