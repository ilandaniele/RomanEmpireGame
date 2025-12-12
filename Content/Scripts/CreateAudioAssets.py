# Roman Empire Game - Audio Asset Generation Script
# Run this script in Unreal Editor: Tools > Execute Python Script

import unreal
import os

def create_sound_cues():
    """Create Sound Cue assets for the game"""
    
    asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
    editor_asset_lib = unreal.EditorAssetLibrary
    
    # Sound categories and their cues
    sound_structure = {
        "/Game/Audio/Combat": [
            "SC_SwordSwing",
            "SC_SwordHit", 
            "SC_ShieldBlock",
            "SC_ArrowFire",
            "SC_ArrowHit",
            "SC_PilumThrow",
            "SC_Death",
            "SC_Charge",
        ],
        "/Game/Audio/Building": [
            "SC_BuildStart",
            "SC_BuildProgress",
            "SC_BuildComplete",
            "SC_BuildingDamaged",
            "SC_BuildingDestroyed",
        ],
        "/Game/Audio/UI": [
            "SC_ButtonClick",
            "SC_ButtonHover",
            "SC_MenuOpen",
            "SC_MenuClose",
            "SC_Error",
            "SC_VictoryFanfare",
            "SC_DefeatSound",
            "SC_TurnEnd",
        ],
        "/Game/Audio/Ambient": [
            "SC_CrowdCheer",
            "SC_BattleAmbience",
            "SC_CityAmbience",
            "SC_NatureAmbience",
            "SC_MarchingFootsteps",
        ],
        "/Game/Audio/Music": [
            "SC_MainMenuTheme",
            "SC_BattleTheme",
            "SC_CampaignTheme",
            "SC_VictoryTheme",
        ],
    }
    
    factory = unreal.SoundCueFactoryNew()
    
    for path, cues in sound_structure.items():
        # Create directory
        if not editor_asset_lib.does_directory_exist(path):
            editor_asset_lib.make_directory(path)
            print(f"Created directory: {path}")
        
        for cue_name in cues:
            # Check if already exists
            full_path = f"{path}/{cue_name}"
            if editor_asset_lib.does_asset_exist(full_path):
                print(f"Already exists: {cue_name}")
                continue
            
            # Create sound cue
            sound_cue = asset_tools.create_asset(cue_name, path, unreal.SoundCue, factory)
            
            if sound_cue:
                print(f"Created: {cue_name}")
                editor_asset_lib.save_asset(full_path)
            else:
                print(f"Failed to create: {cue_name}")


def create_sound_mix():
    """Create Sound Mix and Sound Classes for volume control"""
    
    asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
    editor_asset_lib = unreal.EditorAssetLibrary
    
    mix_path = "/Game/Audio/Mixing"
    
    if not editor_asset_lib.does_directory_exist(mix_path):
        editor_asset_lib.make_directory(mix_path)
    
    # Create Sound Classes
    sound_classes = ["SC_Master", "SC_SFX", "SC_Music", "SC_Ambient", "SC_Voice"]
    
    class_factory = unreal.SoundClassFactory()
    
    for class_name in sound_classes:
        full_path = f"{mix_path}/{class_name}"
        if not editor_asset_lib.does_asset_exist(full_path):
            asset = asset_tools.create_asset(class_name, mix_path, unreal.SoundClass, class_factory)
            if asset:
                print(f"Created sound class: {class_name}")
                editor_asset_lib.save_asset(full_path)
    
    # Create Sound Mix
    mix_factory = unreal.SoundMixFactory()
    mix_name = "SM_GameMix"
    
    full_path = f"{mix_path}/{mix_name}"
    if not editor_asset_lib.does_asset_exist(full_path):
        mix = asset_tools.create_asset(mix_name, mix_path, unreal.SoundMix, mix_factory)
        if mix:
            print(f"Created sound mix: {mix_name}")
            editor_asset_lib.save_asset(full_path)


def create_audio_assets():
    """Main function to create all audio assets"""
    print("=" * 50)
    print("Creating Roman Empire Game Audio Assets...")
    print("=" * 50)
    
    create_sound_cues()
    create_sound_mix()
    
    print("=" * 50)
    print("Audio asset creation complete!")
    print("Note: Import .wav files and connect them to Sound Cues")
    print("=" * 50)


if __name__ == "__main__":
    create_audio_assets()
