# Roman Empire Game - Master Setup Script
# Run this script ONCE in Unreal Editor to create all assets
# Tools > Execute Python Script > Select this file

import unreal
import importlib

# Import other scripts
import sys
import os

# Get script directory
script_dir = os.path.dirname(os.path.abspath(__file__))

def run_full_setup():
    """
    Master setup script that creates all game assets:
    - Blueprints (Core, Units, Buildings, UI)
    - Materials (Terrain, Buildings, Factions)
    - Audio (Sound Cues, Mixing)
    """
    
    print("=" * 60)
    print("  ROMAN EMPIRE GAME - FULL PROJECT SETUP")
    print("=" * 60)
    print("")
    
    # Step 1: Create folder structure
    print("[1/4] Creating folder structure...")
    create_folder_structure()
    
    # Step 2: Create Blueprints
    print("")
    print("[2/4] Creating Blueprints...")
    create_all_blueprints()
    
    # Step 3: Create Materials
    print("")
    print("[3/4] Creating Materials...")
    create_all_materials()
    
    # Step 4: Create Audio
    print("")
    print("[4/4] Creating Audio Assets...")
    create_all_audio()
    
    print("")
    print("=" * 60)
    print("  SETUP COMPLETE!")
    print("=" * 60)
    print("")
    print("Next steps:")
    print("1. Configure BP_GameMode in Project Settings > Maps & Modes")
    print("2. Import .wav audio files into /Game/Audio/")
    print("3. Import 3D models into /Game/Characters/ and /Game/Buildings/")
    print("4. Create a new level and add BP_AudioManager + BP_MaterialManager")
    print("5. Press Play to test!")
    print("")


def create_folder_structure():
    """Create all Content folders"""
    editor_asset_lib = unreal.EditorAssetLibrary
    
    folders = [
        "/Game/Blueprints/Core",
        "/Game/Blueprints/Units",
        "/Game/Blueprints/Buildings",
        "/Game/Blueprints/UI",
        "/Game/Blueprints/World",
        "/Game/Materials",
        "/Game/Materials/Terrain",
        "/Game/Materials/Buildings",
        "/Game/Materials/Characters",
        "/Game/Audio/Combat",
        "/Game/Audio/Building",
        "/Game/Audio/UI",
        "/Game/Audio/Ambient",
        "/Game/Audio/Music",
        "/Game/Audio/Mixing",
        "/Game/Characters/Legionary",
        "/Game/Characters/Centurion",
        "/Game/Buildings/Barracks",
        "/Game/Buildings/Forum",
        "/Game/Buildings/Wall",
        "/Game/Buildings/Common",
        "/Game/Weapons/Gladius",
        "/Game/Weapons/Pilum",
        "/Game/Weapons/Scutum",
        "/Game/Effects",
        "/Game/Maps",
        "/Game/UI/Icons",
        "/Game/UI/Textures",
    ]
    
    for folder in folders:
        if not editor_asset_lib.does_directory_exist(folder):
            editor_asset_lib.make_directory(folder)
            print(f"  Created: {folder}")


def create_all_blueprints():
    """Create all game blueprints"""
    asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
    editor_asset_lib = unreal.EditorAssetLibrary
    
    # Blueprint definitions: (name, path, parent_class_path)
    blueprints = [
        # Core
        ("BP_GameMode", "/Game/Blueprints/Core", "/Script/RomanEmpireGame.RomanEmpireGameMode"),
        ("BP_PlayerController", "/Game/Blueprints/Core", "/Script/RomanEmpireGame.RomanEmpirePlayerController"),
        ("BP_SeamlessCamera", "/Game/Blueprints/Core", "/Script/RomanEmpireGame.SeamlessZoomCamera"),
        ("BP_AudioManager", "/Game/Blueprints/Core", "/Script/RomanEmpireGame.AudioManager"),
        ("BP_MaterialManager", "/Game/Blueprints/Core", "/Script/RomanEmpireGame.MaterialManager"),
        
        # Units
        ("BP_Legionary", "/Game/Blueprints/Units", "/Script/RomanEmpireGame.Legionary"),
        ("BP_UnitBase", "/Game/Blueprints/Units", "/Script/RomanEmpireGame.UnitBase"),
        
        # Buildings
        ("BP_Barracks", "/Game/Blueprints/Buildings", "/Script/RomanEmpireGame.Barracks"),
        ("BP_BuildingBase", "/Game/Blueprints/Buildings", "/Script/RomanEmpireGame.BuildingBase"),
        
        # World
        ("BP_Territory", "/Game/Blueprints/World", "/Script/RomanEmpireGame.TerritoryRegion"),
        ("BP_WorldMapManager", "/Game/Blueprints/World", "/Script/RomanEmpireGame.WorldMapManager"),
        ("BP_CampaignManager", "/Game/Blueprints/World", "/Script/RomanEmpireGame.CampaignManager"),
        ("BP_FactionManager", "/Game/Blueprints/World", "/Script/RomanEmpireGame.FactionManager"),
    ]
    
    factory = unreal.BlueprintFactory()
    
    for name, path, parent_class_path in blueprints:
        full_path = f"{path}/{name}"
        
        if editor_asset_lib.does_asset_exist(full_path):
            print(f"  Exists: {name}")
            continue
        
        parent_class = unreal.load_class(None, parent_class_path)
        if not parent_class:
            print(f"  ERROR: Parent class not found for {name}")
            continue
        
        factory.set_editor_property("parent_class", parent_class)
        bp = asset_tools.create_asset(name, path, unreal.Blueprint, factory)
        
        if bp:
            print(f"  Created: {name}")
            editor_asset_lib.save_asset(full_path)
        else:
            print(f"  FAILED: {name}")
    
    # Create Widget Blueprint
    create_widget_blueprint()


def create_widget_blueprint():
    """Create WBP_MainHUD"""
    asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
    editor_asset_lib = unreal.EditorAssetLibrary
    
    path = "/Game/Blueprints/UI"
    name = "WBP_MainHUD"
    full_path = f"{path}/{name}"
    
    if editor_asset_lib.does_asset_exist(full_path):
        print(f"  Exists: {name}")
        return
    
    factory = unreal.WidgetBlueprintFactory()
    parent_class = unreal.load_class(None, "/Script/RomanEmpireGame.RomanEmpireMainWidget")
    
    if parent_class:
        factory.set_editor_property("parent_class", parent_class)
    
    widget_bp = asset_tools.create_asset(name, path, unreal.WidgetBlueprint, factory)
    
    if widget_bp:
        print(f"  Created: {name} (Widget)")
        editor_asset_lib.save_asset(full_path)
    else:
        print(f"  FAILED: {name}")


def create_all_materials():
    """Create all materials"""
    asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
    editor_asset_lib = unreal.EditorAssetLibrary
    factory = unreal.MaterialFactoryNew()
    
    materials = [
        ("/Game/Materials", "M_RomanStone"),
        ("/Game/Materials", "M_RomanWood"),
        ("/Game/Materials", "M_GhostValid"),
        ("/Game/Materials", "M_GhostInvalid"),
        ("/Game/Materials", "M_FactionRome"),
        ("/Game/Materials", "M_FactionCarthage"),
        ("/Game/Materials", "M_FactionGaul"),
        ("/Game/Materials/Terrain", "M_Grass"),
        ("/Game/Materials/Terrain", "M_Sand"),
        ("/Game/Materials/Terrain", "M_Water"),
        ("/Game/Materials/Characters", "M_RomanArmor"),
        ("/Game/Materials/Characters", "M_Leather"),
        ("/Game/Materials", "M_Selection"),
    ]
    
    for path, name in materials:
        full_path = f"{path}/{name}"
        if editor_asset_lib.does_asset_exist(full_path):
            print(f"  Exists: {name}")
            continue
        
        mat = asset_tools.create_asset(name, path, unreal.Material, factory)
        if mat:
            print(f"  Created: {name}")
            editor_asset_lib.save_asset(full_path)


def create_all_audio():
    """Create audio structure"""
    asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
    editor_asset_lib = unreal.EditorAssetLibrary
    
    # Create Sound Cues
    sound_cues = [
        ("/Game/Audio/Combat", ["SC_SwordSwing", "SC_SwordHit", "SC_ShieldBlock", "SC_Death"]),
        ("/Game/Audio/Building", ["SC_BuildStart", "SC_BuildProgress", "SC_BuildComplete"]),
        ("/Game/Audio/UI", ["SC_ButtonClick", "SC_MenuOpen", "SC_TurnEnd", "SC_Victory"]),
        ("/Game/Audio/Ambient", ["SC_BattleAmbience", "SC_CityAmbience", "SC_MarchingLoop"]),
        ("/Game/Audio/Music", ["SC_MainMenuTheme", "SC_BattleTheme", "SC_CampaignTheme"]),
    ]
    
    factory = unreal.SoundCueFactoryNew()
    
    for path, cues in sound_cues:
        for name in cues:
            full_path = f"{path}/{name}"
            if editor_asset_lib.does_asset_exist(full_path):
                continue
            cue = asset_tools.create_asset(name, path, unreal.SoundCue, factory)
            if cue:
                print(f"  Created: {name}")
                editor_asset_lib.save_asset(full_path)


# Run setup
if __name__ == "__main__":
    run_full_setup()
