# C-012 — Physical camp occupation

Implementation 2026-10-05, UE5.8.2. Replaces the decorative HQ with one ATacticalObjective, retaining its cube mesh/material, physical footprint and location. A 650cm half-extent occupation box detects actual pawn overlaps. The tactical subsystem remains the sole result owner, holds one weak registered objective, and accepts capture only from that objective's private validated path; duplicate actors cannot replace it.

Slice rule: a world-active Rome soldier physically occupying the zone captures only when the live world registry has no Carthaginian soldiers. Routed enemies remain alive and block capture, even outside the zone or original battle roster. Pure tactical evaluation still receives validated world events; killing defenders alone does not resolve victory. Captured stable ID/winner/result project through the existing GameState event.

One objective checks engine overlaps and faction-registry values every0.25 seconds; no UI polling, actor tick or world-wide per-unit scan. The timer stops on resolution and clears on EndPlay. Result has no duplicate captured flag. Expired/destroyed/duplicate/not-begun actors cannot capture; teardown preserves the committed tactical result.

Editor initial45 and final13 actions pass. Directed initial and strengthened automatic-timer ObjectiveOccupationPIE each1/1 with zero warnings/errors/pending (`C012_Targeted`, `C012_Targeted_Final`). Shipping8, strictIWYU70 and finaltest-onlyincremental4 pass. Final `C012_Full` passes40/40 with zero warnings/errors/pending, exit0 (`Artifacts/C012-Full.log`). Repo/LFS/diff/changed Markdown links pass. C-012 is verified, not the whole M3 milestone.

Tests exercise actual initial10 defenders, duplicate registration, kills without occupation, engine collision occupation, an extra live routed/unrostered enemy blocking capture, final single capture with stable ID/winner/public result, repeat rejection and objective destruction preserving result.

Limits: bounded Rome/Carthage encounter, global no-living-Carthage rule (not merely local clearance), no capture duration/rally rule, functional objective rather than a trainable/damageable building archetype. Terminal commands/UMG restart/continue, campaign ownership application, save/load and trusted-player/rendered gates remain. No new asset/config/module/dependency/cook/player evidence.

Skills: `unreal-cpp-gameplay`, `ue-testing-debugging`, `ue-data-assets-tables` for scoped test definitions (restored, never saved). Existing context skill updates authorized sections only.
