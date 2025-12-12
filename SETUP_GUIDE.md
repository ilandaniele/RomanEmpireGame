# Roman Empire Game - Quick Setup Guide

## Step 1: Open Project
1. Double-click `RomanEmpireGame.uproject`
2. Wait for Unreal Editor to compile C++ code
3. If asked about missing modules, click "Yes" to rebuild

## Step 2: Create Blueprints (Automatic)
1. In Unreal Editor: **Tools > Execute Python Script**
2. Navigate to: `Content/Scripts/CreateBlueprints.py`
3. Click Open - blueprints will be created automatically

Created Blueprints:
- `/Game/Blueprints/UI/WBP_MainHUD`
- `/Game/Blueprints/Units/BP_Legionary`
- `/Game/Blueprints/Buildings/BP_Barracks`
- `/Game/Blueprints/Core/BP_GameMode`
- `/Game/Blueprints/Core/BP_PlayerController`
- `/Game/Blueprints/Core/BP_SeamlessCamera`

## Step 3: Create Audio Assets
1. **Tools > Execute Python Script**
2. Navigate to: `Content/Scripts/CreateAudioAssets.py`
3. Sound cues will be created in `/Game/Audio/`

## Step 4: Import Audio Files
1. Download or create .wav files matching `Content/Audio/AudioConfig.json`
2. Drag .wav files into corresponding folders in Content Browser
3. Connect imported sounds to the Sound Cue assets

## Step 5: Configure Game Mode
1. Open **Edit > Project Settings > Maps & Modes**
2. Set Default GameMode to: `BP_GameMode`
3. Open `BP_GameMode` and set:
   - Default Pawn Class: `BP_SeamlessCamera`
   - Player Controller Class: `BP_PlayerController`
   - HUD Class: `RomanEmpireHUD`

## Step 6: Setup Main Level
1. Create new level: **File > New Level > Empty Level**
2. Save as `/Game/Maps/MainMap`
3. Add to level:
   - `BP_AudioManager`
   - `BP_MaterialManager`
   - Directional Light
   - Sky Sphere
   - Landscape (optional)

## Step 7: Test!
1. Press **Play** in Editor
2. Use mouse wheel to zoom
3. Press **B** for building menu
4. Select units with left-click
5. Press **F** to enter FPS mode

## Controls
| Key | Action |
|-----|--------|
| Mouse Wheel | Zoom in/out |
| Left Click | Select |
| Right Click | Move/Command |
| B | Building menu |
| F | Enter/Exit FPS |
| WASD | Move (FPS) |
| Escape | Exit FPS |

## Troubleshooting
- **Compile Error**: Check Output Log, rebuild solution in VS
- **Missing Classes**: Ensure C++ compilation succeeded
- **No Sound**: Check audio device, verify sound cue setup
