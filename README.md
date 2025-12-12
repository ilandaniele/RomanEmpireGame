# Roman Empire Strategy Game

A hybrid strategy game prototype combining **Rome 2: Total War**, **Age of Empires 2**, and **Counter-Strike 1.6** mechanics.

## Features

### 🗺️ Seamless Zoom Camera
Transition smoothly from world map to first-person:
- **World View** (500m) - See continents, manage empire
- **Territory View** (200m) - See regions, borders
- **City View** (50m) - RTS building placement
- **Ground View** (5m) - Third-person unit control
- **FPS View** (1.8m) - First-person combat

### 🏛️ Roman Factions
- **Roman Republic** - Disciplined legions, +20% infantry
- **Carthage** - Trade empire, +30% economy
- **Gallic Tribes** - Fierce warriors, +20% cavalry

### 🏗️ Building System (Age of Empires Style)
Click to place buildings with ghost preview:
- **Barracks** - Train infantry
- **Archery Range** - Train ranged units
- **Forum** - City center
- **Walls & Towers** - Defense

### ⚔️ Combat (Counter-Strike Style)
Press F to possess any soldier and fight:
- WASD movement
- Mouse look
- Left-click attack
- Right-click block
- Stamina system

## Getting Started

### Requirements
- Unreal Engine 5.4
- Visual Studio 2022 with "Game development with C++"
- Windows 10/11

### Setup
1. Open `RomanEmpireGame.uproject` in Unreal Engine
2. If prompted, compile the project
3. Open the level and press Play

### Controls
| Action | Key |
|--------|-----|
| Camera Pan | WASD / Edge scroll |
| Zoom | Mouse wheel |
| Select Units | Left-click |
| Move/Attack | Right-click |
| Building Menu | B |
| Enter FPS Mode | F (with unit selected) |
| Exit FPS Mode | Escape |

## Project Structure
```
RomanEmpireGame/
├── Config/             # Game configuration
├── Content/            # Assets (to be added)
└── Source/
    └── RomanEmpireGame/
        ├── Core/       # GameMode, PlayerController
        ├── Camera/     # SeamlessZoomCamera
        ├── Faction/    # FactionData, FactionManager
        ├── Building/   # BuildingBase, Barracks
        ├── Units/      # UnitBase, Legionary
        └── World/      # Territory, Campaign
```

## Next Steps (Future Development)
- [ ] Add 3D assets (meshes, textures)
- [ ] Implement UI blueprints
- [ ] Add sound effects and music
- [ ] Create more unit types
- [ ] Add naval combat
- [ ] Multiplayer support

## License
This is a prototype for educational purposes.
