"""M6.7 capability probe. Default readonly API inspection; --import-probe imports
original recipe into an empty, unsaved in-memory namespace. Never saves assets.
Source GLB under resolved project Artifacts, create-missing-only/hash checked.
"""
from pathlib import Path
import sys
import unreal

PROJECT = Path(unreal.Paths.project_dir()).resolve()
sys.path.insert(0,str(PROJECT/"Tools"))
import roman_character_recipe as recipe

for type_name in ("InterchangeManager","ImportAssetParameters","InterchangeGenericAssetsPipeline",
                  "SkeletalMeshEditorSubsystem","AnimSequence","AnimBlueprintFactory","AnimMontageFactory"):
    cls = getattr(unreal,type_name,None)
    unreal.log(f"CHARACTER_IMPORT_API {type_name} present={cls is not None}")

if "--import-probe" in sys.argv:
    folder = "/Game/Characters/RomanSoldierProbe"
    library = unreal.EditorAssetLibrary
    registry = unreal.AssetRegistryHelpers.get_asset_registry()
    if registry.get_assets_by_path(folder,recursive=True):
        raise RuntimeError("Probe namespace must be empty; no overwrite/reimport allowed")
    source_root = (PROJECT/"Artifacts"/"RomanCharacterSource"/"schema1").resolve()
    if not source_root.is_relative_to(PROJECT/"Artifacts"):
        raise RuntimeError("Source path escapes project artifact root")
    source = source_root/"SK_Roman_Rome.glb"
    data = recipe.glb("Rome")
    if source.exists() and source.read_bytes() != data:
        raise RuntimeError("Refusing changed existing probe source")
    source_root.mkdir(parents=True,exist_ok=True)
    if not source.exists(): source.write_bytes(data)
    pipeline = unreal.InterchangeGenericAssetsPipeline()
    pipeline.set_editor_property("use_source_name_for_asset",False)
    mesh = pipeline.get_editor_property("mesh_pipeline")
    mesh.set_editor_property("import_static_meshes",False)
    mesh.set_editor_property("import_skeletal_meshes",True)
    mesh.set_editor_property("create_physics_asset",False)
    pipeline.get_editor_property("material_pipeline").set_editor_property("import_materials",False)
    animation = pipeline.get_editor_property("animation_pipeline")
    animation.set_editor_property("import_animations",True)
    params = unreal.ImportAssetParameters()
    params.set_editor_property("is_automated",True)
    params.set_editor_property("replace_existing",False)
    params.set_editor_property("override_pipelines",[unreal.SoftObjectPath(pipeline.get_path_name())])
    manager = unreal.InterchangeManager.get_interchange_manager_scripted()
    source_data = unreal.InterchangeManager.create_source_data(str(source))
    translator = manager.get_translator_for_source_data(source_data)
    if not translator: raise RuntimeError("No actual GLB translator available")
    unreal.log(f"CHARACTER_IMPORT_TRANSLATOR {translator.get_class().get_name()}")
    if not manager.import_asset(folder,source_data,params): raise RuntimeError("Actual skeletal GLB import failed")
    assets = [item.get_asset() for item in registry.get_assets_by_path(folder,recursive=True)]
    for asset in assets:
        unreal.log(f"CHARACTER_IMPORT_RESULT {asset.get_path_name()} type={asset.get_class().get_name()}")
        if isinstance(asset,unreal.AnimSequence):
            unreal.log(f"CHARACTER_IMPORT_ANIMATION skeleton={asset.get_editor_property('skeleton').get_path_name()} length={asset.get_editor_property('sequence_length')}")
        if isinstance(asset,unreal.SkeletalMesh):
            unreal.log(f"CHARACTER_IMPORT_MESH skeleton={asset.get_editor_property('skeleton').get_path_name()} slots={len(asset.get_editor_property('materials'))}")
    counts = {name:sum(asset.get_class().get_name()==name for asset in assets) for name in ("SkeletalMesh","Skeleton","AnimSequence")}
    if counts != {"SkeletalMesh":1,"Skeleton":1,"AnimSequence":5}: raise RuntimeError(f"Actual rig/clip import incomplete: {counts}")
    # Do not save/load another session: this probe deliberately retains no assets.
    unreal.log(f"CHARACTER_IMPORT_VERIFIED_UNSAVED assets={len(assets)} source_sha256={recipe.digest('Rome')}")
