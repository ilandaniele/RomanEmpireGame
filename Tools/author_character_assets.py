"""M6.7 original rigged assets schema1. Readonly default, --apply creates missing
only. No overwrite/reimport, no gameplay/config writes. Existing objects/source
must match ownership, fingerprints and actual references/parameters/format.
"""
from pathlib import Path
import hashlib
import json
import sys
import unreal

PROJECT = Path(unreal.Paths.project_dir()).resolve()
sys.path.insert(0,str(PROJECT/"Tools"))
import roman_character_recipe as recipe

APPLY = "--apply" in sys.argv
PROBES = "--test-rejection" in sys.argv
if APPLY and PROBES: raise RuntimeError("Readonly probes cannot combine with --apply")
FOLDER = "/Game/Characters/RomanSoldier"
TAG = "RomanCharacterSchema"
HASH = "RomanCharacterRecipe_SHA256"
ASSETS = unreal.EditorAssetLibrary
LIB = unreal.MaterialEditingLibrary
TOOLS = unreal.AssetToolsHelpers.get_asset_tools()
MATERIAL_SPECS = {
    "MI_RomanMetal": ((.52,.45,.28,1),.40,.65),
    "MI_RomanSkin": ((.65,.42,.27,1),.80,0),
    "MI_RomanLeather": ((.12,.055,.025,1),.85,0),
}


def fingerprint(value): return hashlib.sha256(json.dumps(value,sort_keys=True).encode()).hexdigest()


specs = {}
for name,value in MATERIAL_SPECS.items(): specs[name] = (unreal.MaterialInstanceConstant,fingerprint(value))
rome_hash = recipe.digest("Rome")
specs["SK_Roman_Rome_Skeleton"] = (unreal.Skeleton,rome_hash)
for profile in recipe.PROFILES: specs[f"SK_Roman_{profile}"] = (unreal.SkeletalMesh,recipe.digest(profile,profile=="Rome"))
for clip,duration in recipe.CLIPS.items(): specs[f"A_Roman{clip}"] = (unreal.AnimSequence,fingerprint((rome_hash,clip,duration)))


def load_owned(name):
    path = f"{FOLDER}/{name}"
    if not ASSETS.does_asset_exist(path): return None
    asset = unreal.load_asset(path); cls,expected = specs[name]
    if asset is None or asset.get_path_name()!=f"{path}.{name}":
        raise RuntimeError(f"Refusing redirected character asset: {path}")
    if not isinstance(asset,cls) or str(ASSETS.get_metadata_tag(asset,TAG)) != recipe.SCHEMA:
        raise RuntimeError(f"Incompatible/unowned character asset: {path}")
    if str(ASSETS.get_metadata_tag(asset,HASH)) != expected:
        raise RuntimeError(f"Character recipe fingerprint mismatch: {path}")
    return asset


def save_new(name,asset):
    ASSETS.set_metadata_tag(asset,TAG,recipe.SCHEMA)
    ASSETS.set_metadata_tag(asset,HASH,specs[name][1])
    if not ASSETS.save_loaded_asset(asset,only_if_is_dirty=False): raise RuntimeError(f"Character save failed: {name}")


# Check every existing target before creating anything. Never adopt foreign assets.
loaded = {name:load_owned(name) for name in specs}
source_root = (PROJECT/"SourceArt"/"Characters"/"RomanSoldier"/"schema1").resolve()
if not source_root.is_relative_to(PROJECT/"SourceArt"):
    raise RuntimeError("Character source escaped project source-art root")
for profile in recipe.PROFILES:
    path = source_root/f"SK_Roman_{profile}.glb"
    expected = recipe.glb(profile,profile=="Rome")
    if path.exists() and path.read_bytes()!=expected: raise RuntimeError(f"Refusing changed source: {path}")
    if not path.exists() and not APPLY: raise RuntimeError(f"Missing original source: {path}; use --apply")

parent = unreal.load_asset("/Game/Materials/RomanVisual/M_RomanSurface")
if not isinstance(parent,unreal.Material) or str(ASSETS.get_metadata_tag(parent,"RomanVisualSchema"))!="1" or not parent.get_editor_property("used_with_skeletal_mesh"):
    raise RuntimeError("Original verified surface lacks required skeletal usage")


def verify_material(name,asset):
    color,roughness,metallic = MATERIAL_SPECS[name]
    if asset.get_editor_property("parent")!=parent: raise RuntimeError(f"Wrong material parent: {name}")
    actual = LIB.get_material_instance_vector_parameter_value(asset,"Color")
    if any(abs(a-b)>1e-5 for a,b in zip((actual.r,actual.g,actual.b,actual.a),color)):
        raise RuntimeError(f"Wrong material color: {name}")
    for parameter,expected in (("Damage",0),("Roughness",roughness),("Metallic",metallic)):
        if abs(LIB.get_material_instance_scalar_parameter_value(asset,parameter)-expected)>1e-5:
            raise RuntimeError(f"Wrong material parameter: {name}/{parameter}")


for name in MATERIAL_SPECS:
    if loaded[name] is not None: verify_material(name,loaded[name])

for name,(color,roughness,metallic) in MATERIAL_SPECS.items():
    asset = loaded[name]; created = asset is None
    if created:
        if not APPLY: raise RuntimeError(f"Missing material: {name}; use --apply")
        asset = TOOLS.create_asset(name,FOLDER,unreal.MaterialInstanceConstant,unreal.MaterialInstanceConstantFactoryNew())
        if not asset: raise RuntimeError(f"Material creation failed: {name}")
        LIB.set_material_instance_parent(asset,parent)
        LIB.set_material_instance_vector_parameter_value(asset,"Color",unreal.LinearColor(*color))
        for parameter,value in (("Damage",0),("Roughness",roughness),("Metallic",metallic)):
            LIB.set_material_instance_scalar_parameter_value(asset,parameter,value)
        verify_material(name,asset); save_new(name,asset); loaded[name]=asset
    verify_material(name,asset)
    unreal.log(f"CHARACTER_MATERIAL_VERIFIED {name} created={created}")


def slot_materials(profile):
    faction = unreal.load_asset(f"/Game/Materials/RomanVisual/MI_Faction{profile}")
    if not isinstance(faction,unreal.MaterialInstanceConstant): raise RuntimeError("Missing canonical faction material")
    return (faction,loaded["MI_RomanMetal"],loaded["MI_RomanSkin"],loaded["MI_RomanLeather"],faction,faction)


def verify_mesh(profile,mesh,skeleton):
    if mesh.get_editor_property("skeleton")!=skeleton: raise RuntimeError(f"Wrong shared skeleton: {profile}")
    slots = mesh.get_editor_property("materials")
    if len(slots)!=len(recipe.MATERIALS): raise RuntimeError(f"Wrong material slot count: {profile}")
    for slot,name,material in zip(slots,recipe.MATERIALS,slot_materials(profile)):
        if str(slot.get_editor_property("material_slot_name"))!=name or slot.get_editor_property("material_interface")!=material:
            raise RuntimeError(f"Wrong material slot: {profile}/{name}: actual_name={slot.get_editor_property('material_slot_name')}, actual_material={slot.get_editor_property('material_interface')}, expected_material={material}")
    bounds = mesh.get_imported_bounds()
    height = 2*bounds.box_extent.z
    if not 175<height<205: raise RuntimeError(f"Wrong actual cm/Z-up imported geometry: {profile} height={height}")
    if mesh.get_editor_property("physics_asset") is not None: raise RuntimeError("Unexpected skeletal physics authority")
    unreal.log(f"CHARACTER_MESH_VERIFIED {profile} height_cm={height} bounds={bounds}")


for profile in recipe.PROFILES:
    name = f"SK_Roman_{profile}"; mesh = loaded[name]
    skeleton = loaded["SK_Roman_Rome_Skeleton"]
    if mesh is None:
        if not APPLY: raise RuntimeError(f"Missing skeletal mesh: {name}; use --apply")
        required = [name]+(["SK_Roman_Rome_Skeleton"]+[f"A_Roman{clip}" for clip in recipe.CLIPS] if profile=="Rome" else [])
        if any(loaded[key] is not None for key in required): raise RuntimeError("Partial import bundle; refusing replacement of existing assets")
        source = source_root/f"{name}.glb"; source_root.mkdir(parents=True,exist_ok=True)
        if not source.exists(): source.write_bytes(recipe.glb(profile,profile=="Rome"))
        pipeline = unreal.InterchangeGenericAssetsPipeline()
        pipeline.set_editor_property("use_source_name_for_asset",False)
        mesh_pipeline = pipeline.get_editor_property("mesh_pipeline")
        mesh_pipeline.set_editor_property("import_static_meshes",False)
        mesh_pipeline.set_editor_property("import_skeletal_meshes",True)
        mesh_pipeline.set_editor_property("create_physics_asset",False)
        mesh_pipeline.set_editor_property("update_skeleton_reference_pose",False)
        pipeline.get_editor_property("material_pipeline").set_editor_property("import_materials",False)
        pipeline.get_editor_property("animation_pipeline").set_editor_property("import_animations",profile=="Rome")
        if skeleton is not None: pipeline.get_editor_property("common_skeletal_meshes_and_animations_properties").set_editor_property("skeleton",skeleton)
        params = unreal.ImportAssetParameters(); params.set_editor_property("is_automated",True)
        params.set_editor_property("replace_existing",False)
        params.set_editor_property("override_pipelines",[unreal.SoftObjectPath(pipeline.get_path_name())])
        manager = unreal.InterchangeManager.get_interchange_manager_scripted()
        if not manager.import_asset(FOLDER,unreal.InterchangeManager.create_source_data(str(source)),params): raise RuntimeError(f"Character import failed: {profile}")
        imported = {key:unreal.load_asset(f"{FOLDER}/{key}") for key in required}
        for key,asset in imported.items():
            if not isinstance(asset,specs[key][0]): raise RuntimeError(f"Actual import bundle missing expected asset: {key}")
        mesh = imported[name]
        if profile=="Rome": skeleton = imported["SK_Roman_Rome_Skeleton"]
        slots = mesh.get_editor_property("materials")
        if len(slots)!=len(recipe.MATERIALS): raise RuntimeError("Unexpected imported material slots")
        unreal.log(f"CHARACTER_IMPORTED_SLOTS {profile} {[str(slot.get_editor_property('material_slot_name')) for slot in slots]}")
        # Unreal Python struct-array indexing/iteration returns value copies.
        # Put each edited value back before assigning the complete array.
        for index,material in enumerate(slot_materials(profile)):
            slot = slots[index]
            slot.set_editor_property("material_interface",material)
            slots[index] = slot
        mesh.set_editor_property("materials",slots)
        verify_mesh(profile,mesh,skeleton)
        for clip,duration in recipe.CLIPS.items():
            if profile!="Rome": break
            sequence = imported[f"A_Roman{clip}"]
            if sequence.get_editor_property("skeleton")!=skeleton or abs(sequence.get_editor_property("sequence_length")-duration)>.001:
                raise RuntimeError(f"Actual imported animation mismatch: {clip}")
            if sequence.get_editor_property("enable_root_motion"): raise RuntimeError("Imported animation must not move gameplay actors")
        for key,asset in imported.items(): save_new(key,asset); loaded[key]=asset
    verify_mesh(profile,mesh,loaded["SK_Roman_Rome_Skeleton"])

for clip,duration in recipe.CLIPS.items():
    sequence = loaded[f"A_Roman{clip}"]
    if sequence is None or sequence.get_editor_property("skeleton")!=loaded["SK_Roman_Rome_Skeleton"] or abs(sequence.get_editor_property("sequence_length")-duration)>.001 or sequence.get_editor_property("enable_root_motion"):
        raise RuntimeError(f"Actual animation readback mismatch: {clip}")
    unreal.log(f"CHARACTER_CLIP_VERIFIED {clip} duration={duration} shared_skeleton=True")

if PROBES:
    def reject(label,callback):
        try: callback()
        except RuntimeError:
            unreal.log(f"CHARACTER_REJECTION_OK {label}"); return
        raise RuntimeError(f"Negative validation unexpectedly passed: {label}")
    asset = loaded["SK_Roman_Rome"]
    for tag,value,label in ((TAG,"foreign","ownership"),(HASH,"invalid","fingerprint")):
        previous = str(ASSETS.get_metadata_tag(asset,tag))
        try:
            ASSETS.set_metadata_tag(asset,tag,value); reject(label,lambda:load_owned("SK_Roman_Rome"))
        finally: ASSETS.set_metadata_tag(asset,tag,previous)
    skin = loaded["MI_RomanSkin"]
    previous = LIB.get_material_instance_scalar_parameter_value(skin,"Roughness")
    try:
        LIB.set_material_instance_scalar_parameter_value(skin,"Roughness",.13)
        reject("material_parameters",lambda:verify_material("MI_RomanSkin",skin))
    finally: LIB.set_material_instance_scalar_parameter_value(skin,"Roughness",previous)
    verify_material("MI_RomanSkin",skin)
    unreal.log("CHARACTER_REJECTION_PROBES_OK no_saves=True count=3")

unreal.log(f"CHARACTER_ASSETS_VERIFIED count={len(specs)} apply={APPLY}")
