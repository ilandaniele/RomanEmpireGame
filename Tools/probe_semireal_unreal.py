"""Import/read back a candidate ONLY in the disposable Artifacts art project.

Run with UE's Python commandlet and an explicit config JSON in that project.
Never imports into RomanEmpireGame, replaces assets or changes a live session.
"""
import hashlib
import json
from pathlib import Path
import sys
import traceback

import unreal

ROOT = Path(__file__).resolve().parent.parent
PROJECT = Path(unreal.Paths.project_dir()).resolve()
EXPECTED = (ROOT / "Artifacts/VQ01_Semireal1/UEProbe").resolve()
if PROJECT != EXPECTED:
    raise RuntimeError("Art probe refused: not the exact disposable project")
config_arg = [s.split("=", 1)[1] for s in sys.argv if s.startswith("--config=")]
if len(config_arg) != 1:
    raise RuntimeError("One explicit --config=<project-relative-json> is required")
config_path = (PROJECT / config_arg[0]).resolve()
config_path.relative_to(PROJECT)
config = json.loads(config_path.read_text(encoding="utf-8"))
source = (ROOT / config["source"]).resolve()
source.relative_to(ROOT / "Artifacts/VQ01_Semireal1")
report = (PROJECT / config["report"]).resolve()
report.relative_to(PROJECT)
if report.exists():
    raise RuntimeError("Probe report exists; inspect it and use a new attempt")
folder = config["folder"]
if not folder.startswith("/Game/ArtCandidates/") or ".." in folder:
    raise RuntimeError("Probe destination not permitted")
ASSETS = unreal.EditorAssetLibrary
result = {"status": "running", "project": str(PROJECT), "source": str(source),
          "source_sha256": hashlib.sha256(source.read_bytes()).hexdigest(),
          "folder": folder, "game_content_changed": False,
          "visual_approved": False, "performance_approved": False}


def require(condition, message):
    if not condition:
        raise RuntimeError(message)


def bound_textures(material, prop):
    # Read serialized property graph, not GetUsedTextures' render resource,
    # which is absent/uncached under NullRHI.
    library = unreal.MaterialEditingLibrary
    pending = [library.get_material_property_input_node(material, prop)]
    visited, textures = set(), set()
    while pending:
        node = pending.pop()
        if node is None or node in visited:
            continue
        visited.add(node)
        if isinstance(node, unreal.MaterialExpressionTextureBase):
            texture = node.get_editor_property("texture")
            if texture:
                textures.add(texture)
        pending.extend(library.get_inputs_for_material_expression(material,node))
    return textures


def run():
    read_only = config.get("read_only", False)
    if read_only:
        require(ASSETS.does_directory_exist(folder), "Readonly destination missing")
    else:
        require(not ASSETS.does_directory_exist(folder), "Destination exists; refusing overwrite")
        import_new()
    verify_loaded(read_only)


def import_new():
    pipeline = unreal.InterchangeGenericAssetsPipeline()
    pipeline.set_editor_property("use_source_name_for_asset", False)
    mesh_pipeline = pipeline.get_editor_property("mesh_pipeline")
    mesh_pipeline.set_editor_property("import_static_meshes", False)
    mesh_pipeline.set_editor_property("import_skeletal_meshes", True)
    mesh_pipeline.set_editor_property("create_physics_asset", False)
    mesh_pipeline.set_editor_property("update_skeleton_reference_pose", False)
    pipeline.get_editor_property("material_pipeline").set_editor_property("import_materials", True)
    pipeline.get_editor_property("animation_pipeline").set_editor_property("import_animations", True)
    params = unreal.ImportAssetParameters()
    params.set_editor_property("is_automated", True)
    params.set_editor_property("replace_existing", False)
    params.set_editor_property("override_pipelines", [unreal.SoftObjectPath(pipeline.get_path_name())])
    manager = unreal.InterchangeManager.get_interchange_manager_scripted()
    require(manager.import_asset(folder, manager.create_source_data(str(source)), params), "Import failed")


def verify_loaded(read_only):
    paths = ASSETS.list_assets(folder, recursive=True, include_folder=False)
    loaded = [unreal.load_asset(path) for path in paths]
    require(all(asset is not None for asset in loaded), "Imported asset failed to load")
    meshes = [a for a in loaded if isinstance(a, unreal.SkeletalMesh)]
    skeletons = [a for a in loaded if isinstance(a, unreal.Skeleton)]
    clips = [a for a in loaded if isinstance(a, unreal.AnimSequence)]
    textures = [a for a in loaded if isinstance(a, unreal.Texture2D)]
    require(len(meshes) == 1 and len(skeletons) == 1, "Expected one real skeletal mesh and skeleton")
    mesh, skeleton = meshes[0], skeletons[0]
    require(mesh.get_editor_property("skeleton") == skeleton, "Skeleton reference mismatch")
    require(mesh.get_editor_property("physics_asset") is None, "Unexpected physics authority")
    bounds = mesh.get_imported_bounds()
    height = 2 * bounds.box_extent.z
    require(175 < height < 205, f"Imported cm/Z-up height invalid: {height}")
    slots = mesh.get_editor_property("materials")
    require(len(slots) == 8, "Material slot count changed")
    require(all(s.get_editor_property("material_interface") is not None for s in slots), "Missing imported material")
    component = unreal.SkeletalMeshComponent()
    component.set_skeletal_mesh_asset(mesh)
    bones = [str(component.get_bone_name(i)) for i in range(component.get_num_bones())]
    sys.path.insert(0, str(ROOT / "Tools"))
    from roman_character_recipe import BONES, CLIPS
    require(bones == [b[0] for b in BONES], f"Actual Unreal hierarchy names/order changed: {bones}")
    actual_clips = {}
    for name, duration in CLIPS.items():
        matches = [a for a in clips if str(a.get_name()).endswith("_" + name) or str(a.get_name()) == name]
        require(len(matches) == 1, f"Missing/ambiguous actual clip: {name}")
        sequence = matches[0]
        length = float(sequence.get_editor_property("sequence_length"))
        require(sequence.get_editor_property("skeleton") == skeleton, f"Clip skeleton mismatch: {name}")
        require(abs(length - duration) < .002, f"Clip duration mismatch: {name}/{length}")
        require(not sequence.get_editor_property("enable_root_motion"), f"Gameplay root motion imported: {name}")
        actual_clips[name] = {"path": sequence.get_path_name(), "seconds": length}
    require(len(clips) == 5, "Unexpected additional animation clips")
    require(len(textures) == 3, "Expected exactly three shared atlas textures")
    texture_details = []
    for texture in textures:
        name = texture.get_name()
        srgb = bool(texture.get_editor_property("srgb"))
        compression = texture.get_editor_property("compression_settings")
        require(srgb == (name == "BaseColor"), "PBR texture color-space mismatch: " + name)
        if name == "Normal":
            require(compression == unreal.TextureCompressionSettings.TC_NORMALMAP, "Normal-map compression mismatch")
        texture_details.append({"path":texture.get_path_name(),"srgb":srgb,"compression":str(compression)})
    material_bindings = {}
    expected_textures = {texture.get_name():texture for texture in textures}
    for slot in slots:
        material = slot.get_editor_property("material_interface")
        require(isinstance(material, unreal.Material), "Unexpected imported material class")
        binding = {}
        if material.get_editor_property("use_material_attributes"):
            actual = bound_textures(material,unreal.MaterialProperty.MP_MATERIAL_ATTRIBUTES)
            require(actual == set(textures),"Actual material-attributes graph omits/shared atlas mismatch")
            binding["MaterialAttributes"] = [t.get_path_name() for t in actual]
            material_bindings[material.get_name()] = binding
            continue
        for prop,expected_name in ((unreal.MaterialProperty.MP_BASE_COLOR,"BaseColor"),
                                   (unreal.MaterialProperty.MP_NORMAL,"Normal"),
                                   (unreal.MaterialProperty.MP_ROUGHNESS,"RomanSemiReal_Metallic-RomanSemiReal_Roughness"),
                                   (unreal.MaterialProperty.MP_METALLIC,"RomanSemiReal_Metallic-RomanSemiReal_Roughness")):
            actual = bound_textures(material,prop)
            node = unreal.MaterialEditingLibrary.get_material_property_input_node(material,prop)
            output_name = unreal.MaterialEditingLibrary.get_material_property_input_node_output_name(material,prop)
            binding[str(prop)] = {"textures":[t.get_path_name() for t in actual],
                                  "root_class":node.get_class().get_name() if node else None,"output":output_name}
            # A shared glTF function has multiple property outputs but all
            # texture inputs. Traversing the whole function legitimately finds
            # the other atlas maps too; record output pin separately.
            require(expected_textures[expected_name] in actual and actual.issubset(set(textures)),
                    f"Material graph binding mismatch: {material.get_name()}/{prop}: {binding[str(prop)]}")
        material_bindings[material.get_name()] = binding
    if not read_only:
        for asset in loaded:
            require(ASSETS.save_loaded_asset(asset, only_if_is_dirty=False), "Probe asset save failed")
    result.update(status="pass", mesh=mesh.get_path_name(), skeleton=skeleton.get_path_name(),
                  height_cm=height, bones=bones, clips=actual_clips,
                  slots=[str(s.get_editor_property("material_slot_name")) for s in slots],
                  textures=[a.get_path_name() for a in textures],
                  texture_details=texture_details,material_bindings=material_bindings,read_only=read_only,
                  assets=[{"path": a.get_path_name(), "class": a.get_class().get_name()} for a in loaded])


try:
    run()
except Exception:
    result.update(status="fail", error=traceback.format_exc())
    raise
finally:
    with report.open("x", encoding="utf-8") as stream:
        json.dump(result, stream, indent=2)
    unreal.log("SEMIREAL_UNREAL_PROBE " + result["status"] + " " + str(report))
