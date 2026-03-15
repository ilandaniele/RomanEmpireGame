# Roman Empire Strategy Game

Prototype in Unreal Engine 5 — Rome Total War + Age of Empires + FPS — all in one seamless camera.

## Zoom-based gameplay zones

```
Zoom 0.8–1.0  →  Campaign (global map, factions, diplomacy, turns)
Zoom 0.3–0.8  →  Regional (territories, cities, building overview)
Zoom 0.1–0.3  →  Tactical/Battle (RTS, unit control, building placement)
Zoom 0.0–0.1  →  FPS (possess a soldier, first-person combat)
```

## Project structure

```
Source/RomanEmpireGame/
├── Camera/         SeamlessZoomCamera
├── Core/           GameMode, PlayerController, HUD
├── Faction/        FactionData, FactionManager
├── Building/       BuildingBase, Barracks, BuildingPlacement, BuildingTypes
├── Units/          UnitBase, Legionary
├── World/          TerritoryRegion, WorldMapManager, CampaignManager
├── UI/             MainWidget, BuildingMenu, Minimap
├── Assets/         MaterialManager, ProceduralMeshGenerators
├── Audio/          AudioManager, SoundData
└── Online/         SupabaseSubsystem  ← new

scripts/
├── rc_wait_ready.py      # wait for Remote Control
├── run_e2e.py            # launch UE + run tests + teardown
├── ue_launch.py          # launch UE with RC flags
└── tests/
    └── test_romanempire_e2e.py  # 4 E2E tests
```

## Quick Start

### Open in UE5
Double-click `RomanEmpireGame.uproject` → Accept recompile.

### Run with Remote Control (for testing)
```bash
UnrealEditor "RomanEmpireGame.uproject" -ExecCmds="WebControl.StartServer"
```

### Run E2E tests
```bash
python scripts/rc_wait_ready.py --host 127.0.0.1 --port 30010
python scripts/tests/test_romanempire_e2e.py
```

### Or run full E2E pipeline (launch + wait + test + teardown)
```bash
python scripts/run_e2e.py \
  --ue-exe "C:\UE5\Engine\Binaries\Win64\UnrealEditor.exe" \
  --uproject "RomanEmpireGame.uproject" \
  --tests scripts/tests/
```

## Supabase Online (optional)
Edit `Config/DefaultGame.ini` → replace `URL` and `AnonKey`:
```ini
[Supabase]
URL=https://YOUR_PROJECT.supabase.co
AnonKey=YOUR_ANON_KEY_HERE
```

Create tables in [Supabase dashboard](https://app.supabase.com):
```sql
create table saves (
  id uuid primary key default gen_random_uuid(),
  player_id text not null,
  save_data jsonb not null,
  updated_at timestamptz default now()
);
create table leaderboard (
  id uuid primary key default gen_random_uuid(),
  player_id text not null,
  player_name text,
  turns_played int,
  territories_conquered int,
  faction text,
  created_at timestamptz default now()
);
```

## E2E Tests
| Test | Validates |
|------|-----------|
| zone_transitions | Camera zooms Campaign → Battle → FPS |
| building_menu | B key opens build menu |
| turn_cycle | EndTurn increments GetCurrentTurn by 1 |
| fps_mode | FPS zoom level activates first-person |

## GitHub
https://github.com/ilandaniele/RomanEmpireGame
