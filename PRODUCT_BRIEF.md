# Roman Empire Game Product Brief

Status: M0 implementation baseline
Adopted: 2026-09-03

## Product statement

Roman Empire Game is a single-player Windows strategy game in which the player manages a Roman campaign, develops settlements, commands armies in real time, and can explicitly possess one soldier during a tactical battle.

The first release target is a coherent, replayable vertical slice rather than a broad collection of disconnected campaign, RTS, and first-person prototypes.

## First-playable journey

The player must be able to complete this sequence in a fresh packaged build:

1. Start a new campaign as Rome on a deterministic nine-territory campaign state.
2. Enter the tactical level for the Rome-versus-Carthage objective.
3. Inspect the available resources and place one barracks on valid terrain.
4. Recruit a small Roman force.
5. Select the force, issue formation movement and combat commands, and attack the Carthaginian objective.
6. Explicitly possess one legionary, fight in first person, and return to command mode without losing tactical state.
7. Capture the objective and resolve the result back into campaign territory ownership.
8. Save locally, exit the game, restart it, and restore the same campaign state.

## Slice scope

Included:

- Windows desktop, single-player and offline-first.
- Rome and Carthage only.
- Approximately nine campaign territories and one deterministic tactical level.
- Strategic, tactical-command, and possessed-soldier modes.
- One settlement objective, one barracks, and no more than two useful unit archetypes per faction.
- Selection, move, formation, attack, stop, hold, placement, recruitment, battle resolution, and versioned local save/load.
- Production UMG, essential feedback, automated validation, and Development and Shipping packages.

Non-goals for the slice:

- Multiplayer, dedicated servers, accounts, cloud saves, or public leaderboards.
- A continuous one-to-one Mediterranean world.
- Six complete factions, naval warfare, deep diplomacy, dynasty simulation, or sieges.
- Mass Gameplay, World Partition, PCG, or Gameplay Ability System without measured need.
- Final-quality art for every unit, building, territory, animation, and sound.

## Adopted M0 decisions

| ID | Decision | Adopted baseline |
|---|---|---|
| D-01 | Engine | Unreal Engine 5.8 Launcher build. No UE binary assets were found that require 5.7 compatibility. |
| D-02 | Legacy assets | Proceed from a deterministic clean bootstrap. No `.uasset`, `.umap`, source media, or alternate Roman Empire UE project was found in the repository, its history, or accessible standard user folders. Future recovered content enters through the asset intake process and cannot silently replace canonical content. |
| D-03 | World model | Campaign state is data; battles use authored tactical levels with a seamless-looking transition. |
| D-04 | Zoom | `0.0` is far/strategic and `1.0` is close. First-person possession is an explicit command, never a side effect of scrolling. |
| D-05 | Performance | Target 60 FPS at 1920x1080 on the reference PC with 50 active tactical units; separately measure a 100-unit stretch case. Frame pacing and game-thread cost are acceptance criteria, not only average FPS. |
| D-06 | Historical direction | Historically inspired: recognizable places, factions, equipment, and terminology, with gameplay clarity taking priority over strict simulation. |
| D-07 | Saves | Every save has an explicit schema version. Development saves may be migrated or deliberately invalidated with a visible compatibility message. |
| D-08 | Editor automation | Epic's UE 5.8 Unreal MCP may be enabled only after the project compiles and the gate in `IMPLEMENTATION_PLAN.md` is satisfied. It remains editor-only, loopback-only, manually started, and excluded from Shipping. |

## Reference platform and performance evidence

The provisional reference machine is the currently audited development laptop:

- Lenovo model 83F5.
- Intel Core Ultra 9 275HX, 24 cores / 24 logical processors.
- 31.4 GB usable RAM.
- NVIDIA GeForce RTX 5070 Ti Laptop GPU.
- Windows 11 x64.
- 1920x1080 performance test resolution.

The GPU memory value reported by legacy WMI was not reliable and is intentionally not recorded. M3 must capture exact adapter memory and driver metadata through Unreal Insights or RHI logging before performance results are accepted.

## Product acceptance

The slice is accepted only when the complete player journey passes in a fresh checkout and a packaged Windows build. Editor-only demonstrations, generated screenshots, old reports, or individually working subsystems do not establish completion.

## Licensing baseline

Project-authored code and content are proprietary and All Rights Reserved unless a file explicitly states otherwise. Third-party code or assets require recorded commercial-use rights, source, version, modifications, and attribution before entering the repository. See `LICENSE.txt`, `THIRD_PARTY_NOTICES.md`, and `ASSET_INVENTORY.md`.
