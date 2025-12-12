# Roman Empire Game - Blueprint Generation Script
# Run this script in Unreal Editor: Tools > Execute Python Script

import unreal

def create_widget_blueprint():
    """Create WBP_MainHUD widget blueprint based on URomanEmpireMainWidget"""
    
    asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
    editor_asset_lib = unreal.EditorAssetLibrary
    
    # Create the widget blueprint
    factory = unreal.WidgetBlueprintFactory()
    
    # Set parent class to our C++ class
    parent_class = unreal.load_class(None, "/Script/RomanEmpireGame.RomanEmpireMainWidget")
    if parent_class:
        factory.set_editor_property("parent_class", parent_class)
    
    # Create the asset
    asset_path = "/Game/Blueprints/UI"
    asset_name = "WBP_MainHUD"
    
    # Ensure directory exists
    if not editor_asset_lib.does_directory_exist(asset_path):
        editor_asset_lib.make_directory(asset_path)
    
    # Create widget blueprint
    widget_bp = asset_tools.create_asset(asset_name, asset_path, unreal.WidgetBlueprint, factory)
    
    if widget_bp:
        print(f"Created widget blueprint: {asset_path}/{asset_name}")
        
        # Save the asset
        unreal.EditorAssetLibrary.save_asset(f"{asset_path}/{asset_name}")
    else:
        print("Failed to create widget blueprint")
    
    return widget_bp


def create_legionary_blueprint():
    """Create BP_Legionary blueprint based on ALegionary"""
    
    asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
    editor_asset_lib = unreal.EditorAssetLibrary
    
    # Create blueprint factory
    factory = unreal.BlueprintFactory()
    factory.set_editor_property("parent_class", unreal.load_class(None, "/Script/RomanEmpireGame.Legionary"))
    
    asset_path = "/Game/Blueprints/Units"
    asset_name = "BP_Legionary"
    
    # Ensure directory exists
    if not editor_asset_lib.does_directory_exist(asset_path):
        editor_asset_lib.make_directory(asset_path)
    
    # Create the blueprint
    bp = asset_tools.create_asset(asset_name, asset_path, unreal.Blueprint, factory)
    
    if bp:
        print(f"Created blueprint: {asset_path}/{asset_name}")
        
        # Configure default values
        # Get the default object
        cdo = unreal.get_default_object(bp.generated_class())
        if cdo:
            # Set unit data defaults
            pass  # Properties set in C++ constructor
        
        unreal.EditorAssetLibrary.save_asset(f"{asset_path}/{asset_name}")
    else:
        print("Failed to create Legionary blueprint")
    
    return bp


def create_barracks_blueprint():
    """Create BP_Barracks blueprint based on ABarracks"""
    
    asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
    editor_asset_lib = unreal.EditorAssetLibrary
    
    factory = unreal.BlueprintFactory()
    factory.set_editor_property("parent_class", unreal.load_class(None, "/Script/RomanEmpireGame.Barracks"))
    
    asset_path = "/Game/Blueprints/Buildings"
    asset_name = "BP_Barracks"
    
    if not editor_asset_lib.does_directory_exist(asset_path):
        editor_asset_lib.make_directory(asset_path)
    
    bp = asset_tools.create_asset(asset_name, asset_path, unreal.Blueprint, factory)
    
    if bp:
        print(f"Created blueprint: {asset_path}/{asset_name}")
        unreal.EditorAssetLibrary.save_asset(f"{asset_path}/{asset_name}")
    else:
        print("Failed to create Barracks blueprint")
    
    return bp


def create_additional_blueprints():
    """Create additional useful blueprints"""
    
    asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
    editor_asset_lib = unreal.EditorAssetLibrary
    
    blueprints_to_create = [
        # (name, path, parent_class_path)
        ("BP_SeamlessCamera", "/Game/Blueprints/Core", "/Script/RomanEmpireGame.SeamlessZoomCamera"),
        ("BP_GameMode", "/Game/Blueprints/Core", "/Script/RomanEmpireGame.RomanEmpireGameMode"),
        ("BP_PlayerController", "/Game/Blueprints/Core", "/Script/RomanEmpireGame.RomanEmpirePlayerController"),
        ("BP_Territory", "/Game/Blueprints/World", "/Script/RomanEmpireGame.TerritoryRegion"),
        ("BP_AudioManager", "/Game/Blueprints/Core", "/Script/RomanEmpireGame.AudioManager"),
        ("BP_MaterialManager", "/Game/Blueprints/Core", "/Script/RomanEmpireGame.MaterialManager"),
    ]
    
    for name, path, parent_class_path in blueprints_to_create:
        if not editor_asset_lib.does_directory_exist(path):
            editor_asset_lib.make_directory(path)
        
        factory = unreal.BlueprintFactory()
        parent_class = unreal.load_class(None, parent_class_path)
        
        if parent_class:
            factory.set_editor_property("parent_class", parent_class)
            bp = asset_tools.create_asset(name, path, unreal.Blueprint, factory)
            
            if bp:
                print(f"Created: {path}/{name}")
                unreal.EditorAssetLibrary.save_asset(f"{path}/{name}")
            else:
                print(f"Failed: {name}")
        else:
            print(f"Parent class not found: {parent_class_path}")


def create_all_blueprints():
    """Main function to create all blueprints"""
    print("=" * 50)
    print("Creating Roman Empire Game Blueprints...")
    print("=" * 50)
    
    create_widget_blueprint()
    create_legionary_blueprint()
    create_barracks_blueprint()
    create_additional_blueprints()
    
    print("=" * 50)
    print("Blueprint creation complete!")
    print("=" * 50)


# Run when script is executed
if __name__ == "__main__":
    create_all_blueprints()
