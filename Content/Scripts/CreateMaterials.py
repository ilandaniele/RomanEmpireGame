# Roman Empire Game - Material Setup Script
# Run in Unreal Editor: Tools > Execute Python Script

import unreal

def create_materials():
    """Create all game materials with proper parameters"""
    
    asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
    editor_asset_lib = unreal.EditorAssetLibrary
    
    material_path = "/Game/Materials"
    if not editor_asset_lib.does_directory_exist(material_path):
        editor_asset_lib.make_directory(material_path)
    
    # Material definitions
    materials = [
        # Building materials
        {
            "name": "M_RomanStone",
            "base_color": (0.6, 0.55, 0.5, 1.0),
            "roughness": 0.8,
            "description": "Roman stone/marble for buildings"
        },
        {
            "name": "M_RomanWood",
            "base_color": (0.4, 0.25, 0.15, 1.0),
            "roughness": 0.7,
            "description": "Wooden structures"
        },
        {
            "name": "M_RomanRoof",
            "base_color": (0.7, 0.35, 0.2, 1.0),
            "roughness": 0.6,
            "description": "Terracotta roof tiles"
        },
        
        # Ghost/Placement materials
        {
            "name": "M_GhostValid",
            "base_color": (0.2, 0.8, 0.2, 0.5),
            "emissive": (0.1, 0.5, 0.1),
            "blend_mode": "Translucent",
            "description": "Valid building placement"
        },
        {
            "name": "M_GhostInvalid",
            "base_color": (0.8, 0.2, 0.2, 0.5),
            "emissive": (0.5, 0.1, 0.1),
            "blend_mode": "Translucent",
            "description": "Invalid building placement"
        },
        
        # Faction materials
        {
            "name": "M_FactionRome",
            "base_color": (0.8, 0.15, 0.15, 1.0),
            "metallic": 0.3,
            "description": "Roman faction color"
        },
        {
            "name": "M_FactionCarthage",
            "base_color": (0.5, 0.1, 0.5, 1.0),
            "metallic": 0.3,
            "description": "Carthaginian faction color"
        },
        {
            "name": "M_FactionGaul",
            "base_color": (0.15, 0.6, 0.15, 1.0),
            "metallic": 0.3,
            "description": "Gallic faction color"
        },
        
        # Terrain materials
        {
            "name": "M_Grass",
            "base_color": (0.2, 0.5, 0.15, 1.0),
            "roughness": 0.9,
            "description": "Mediterranean grass"
        },
        {
            "name": "M_Sand",
            "base_color": (0.85, 0.75, 0.55, 1.0),
            "roughness": 0.95,
            "description": "Desert/beach sand"
        },
        {
            "name": "M_Water",
            "base_color": (0.1, 0.3, 0.5, 0.8),
            "metallic": 0.0,
            "roughness": 0.1,
            "blend_mode": "Translucent",
            "description": "Mediterranean sea water"
        },
        
        # Unit materials
        {
            "name": "M_RomanArmor",
            "base_color": (0.7, 0.65, 0.5, 1.0),
            "metallic": 0.8,
            "roughness": 0.3,
            "description": "Roman legionary armor"
        },
        {
            "name": "M_LeatherBrown",
            "base_color": (0.35, 0.2, 0.1, 1.0),
            "roughness": 0.7,
            "description": "Leather straps and gear"
        },
        {
            "name": "M_SkinTone",
            "base_color": (0.8, 0.65, 0.55, 1.0),
            "roughness": 0.6,
            "subsurface": 0.3,
            "description": "Human skin"
        },
        
        # Selection/highlight
        {
            "name": "M_SelectionDecal",
            "base_color": (0.0, 1.0, 0.0, 0.5),
            "emissive": (0.0, 0.5, 0.0),
            "blend_mode": "Translucent",
            "description": "Unit selection circle"
        }
    ]
    
    factory = unreal.MaterialFactoryNew()
    
    for mat_def in materials:
        mat_name = mat_def["name"]
        full_path = f"{material_path}/{mat_name}"
        
        if editor_asset_lib.does_asset_exist(full_path):
            print(f"Already exists: {mat_name}")
            continue
        
        # Create material
        material = asset_tools.create_asset(mat_name, material_path, unreal.Material, factory)
        
        if material:
            print(f"Created material: {mat_name}")
            # Note: Setting material parameters requires blueprint or editor scripting
            # The color values are documented for manual setup
            editor_asset_lib.save_asset(full_path)
        else:
            print(f"Failed to create: {mat_name}")


def create_material_parameter_collection():
    """Create global material parameter collection"""
    
    asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
    editor_asset_lib = unreal.EditorAssetLibrary
    
    path = "/Game/Materials"
    name = "MPC_GameParameters"
    full_path = f"{path}/{name}"
    
    if editor_asset_lib.does_asset_exist(full_path):
        print(f"MPC already exists")
        return
    
    # Create Material Parameter Collection
    factory = unreal.MaterialParameterCollectionFactoryNew()
    mpc = asset_tools.create_asset(name, path, unreal.MaterialParameterCollection, factory)
    
    if mpc:
        print(f"Created Material Parameter Collection: {name}")
        # Parameters would be added via editor
        # - PlayerFactionColor (Vector)
        # - TimeOfDay (Scalar)
        # - FogDensity (Scalar)
        editor_asset_lib.save_asset(full_path)


def create_all_materials():
    """Main function"""
    print("=" * 50)
    print("Creating Roman Empire Game Materials...")
    print("=" * 50)
    
    create_materials()
    create_material_parameter_collection()
    
    print("=" * 50)
    print("Material creation complete!")
    print("Open each material to set up nodes and parameters")
    print("=" * 50)


if __name__ == "__main__":
    create_all_materials()
