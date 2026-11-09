"""Create-only private camp import/readback; never edits the main project."""
import hashlib
import json
from pathlib import Path
import re
import stat
import sys
import traceback

import unreal

ROOT = Path(__file__).resolve().parent.parent
PROJECT = ROOT/'Artifacts/VQ01_Semireal1/GamePreview'


def require(condition, message):
    if not condition:
        raise RuntimeError(message)


def reject_reparse(path):
    for ancestor in (path,*path.parents):
        if ancestor.exists():
            require(not (ancestor.lstat().st_file_attributes & stat.FILE_ATTRIBUTE_REPARSE_POINT),
                    'Refusing reparse point: '+str(ancestor))


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def argument(name):
    values = [s.split('=',1)[1] for s in sys.argv if s.startswith('--'+name+'=')]
    require(len(values) == 1, 'Exactly one --'+name+'= required')
    return values[0]


sys.path.insert(0,str(ROOT/'Tools'))
from semireal_camp_import_policy import select_revision, read_source_manifest, validate_namespace

SOURCE_REVISION = select_revision(sys.argv)
SOURCE = ROOT/'SourceArt/Environment/RomanCamp'/SOURCE_REVISION
reject_reparse(PROJECT); reject_reparse(SOURCE)
require(Path(unreal.Paths.project_dir()).resolve() == PROJECT.resolve(), 'Exact existing private preview only')
ATTEMPT, REPORT_NAME = argument('attempt'), argument('report')
require(re.fullmatch(r'Camp[0-9]+',ATTEMPT), 'Fresh CampNN namespace required')
validate_namespace(SOURCE_REVISION,ATTEMPT)
require(re.fullmatch(r'[a-zA-Z0-9_-]+\.json',REPORT_NAME), 'Simple fresh report name required')
FOLDER = '/Game/ArtCandidates/'+ATTEMPT
DISK_FOLDER = PROJECT/'Content/ArtCandidates'/ATTEMPT
REPORT = PROJECT/REPORT_NAME
READ_ONLY = '--read-only' in sys.argv
reject_reparse(DISK_FOLDER); reject_reparse(REPORT)
require(not REPORT.exists(), 'Report exists; never overwrite')
ASSETS = unreal.EditorAssetLibrary
TAG = 'RomanCampPreviewSchema'
sys.path.insert(0,str(ROOT/'Tools'))
from semireal_gltf_cleanup import deduplicate_glb


def snapshot():
    files = {}
    for path in (PROJECT/'Content').rglob('*'):
        reject_reparse(path)
        if path.is_file() and (READ_ONLY or DISK_FOLDER not in path.parents):
            files[path.relative_to(PROJECT).as_posix()] = digest(path)
    return files


def bound_textures(material, prop):
    lib = unreal.MaterialEditingLibrary
    pending = [lib.get_material_property_input_node(material,prop)]
    visited, textures = set(), set()
    while pending:
        node = pending.pop()
        if node is None or node in visited:
            continue
        visited.add(node)
        if isinstance(node,unreal.MaterialExpressionTextureBase):
            texture = node.get_editor_property('texture')
            if texture:
                textures.add(texture)
        pending.extend(lib.get_inputs_for_material_expression(material,node))
    return textures


def import_new(manifest):
    require(not ASSETS.does_directory_exist(FOLDER) and not DISK_FOLDER.exists(), 'Namespace already exists')
    staging = PROJECT/'Artifacts/CampImportSources'/ATTEMPT
    reject_reparse(staging)
    require(not staging.exists(), 'Import staging exists; preserve and use fresh namespace')
    staging.mkdir(parents=True)
    records = []
    for item in manifest['meshes']:
        source = SOURCE/(item['name']+'.glb')
        expected = next(f['sha256'] for f in manifest['files'] if f['path'] == source.name)
        require(digest(source) == expected, 'Published source changed')
        raw = source.read_bytes()
        staged = staging/source.name
        # Existing tested helper changes only exact duplicate texture descriptors;
        # all geometry/accessor/image binary bytes remain byte-identical.
        staged.write_bytes(deduplicate_glb(raw))
        pipeline = unreal.InterchangeGenericAssetsPipeline()
        pipeline.set_editor_property('use_source_name_for_asset',False)
        meshes = pipeline.get_editor_property('mesh_pipeline')
        meshes.set_editor_property('import_static_meshes',True)
        meshes.set_editor_property('import_skeletal_meshes',False)
        meshes.set_editor_property('build_nanite',False)
        meshes.set_editor_property('collision',False)
        meshes.set_editor_property('generate_lightmap_u_vs',False)
        materials = pipeline.get_editor_property('material_pipeline')
        materials.set_editor_property('import_materials',True)
        materials.set_editor_property('reuse_existing_materials',True)
        materials.set_editor_property('search_location',unreal.InterchangeMaterialSearchLocation.LOCAL)
        pipeline.get_editor_property('animation_pipeline').set_editor_property('import_animations',False)
        params = unreal.ImportAssetParameters()
        params.set_editor_property('is_automated',True)
        params.set_editor_property('replace_existing',False)
        params.set_editor_property('override_pipelines',[unreal.SoftObjectPath(pipeline.get_path_name())])
        manager = unreal.InterchangeManager.get_interchange_manager_scripted()
        require(manager.import_asset(FOLDER,manager.create_source_data(str(staged)),params), 'Import failed: '+source.name)
        records.append(dict(source=source.name,source_sha256=expected,staged_sha256=digest(staged)))
    # Save ONLY created assets in the new owned namespace. No global dirty save.
    for path in ASSETS.list_assets(FOLDER,recursive=True,include_folder=False):
        asset = unreal.load_asset(path)
        require(asset is not None, 'Created asset failed to load')
        ASSETS.set_metadata_tag(asset,TAG,'1')
        ASSETS.set_metadata_tag(asset,'RomanCampSourceRevision',SOURCE_REVISION)
        ASSETS.set_metadata_tag(asset,'RomanCampSourceManifestSha256',result['source_manifest_sha256'])
        require(ASSETS.save_loaded_asset(asset,only_if_is_dirty=False), 'Save new asset failed')
    return records


def verify(manifest):
    assets = [unreal.load_asset(p) for p in ASSETS.list_assets(FOLDER,recursive=True,include_folder=False)]
    require(all(a is not None for a in assets), 'Asset load failed')
    require(all(str(ASSETS.get_metadata_tag(a,TAG)) == '1' for a in assets), 'Foreign/partial namespace')
    for asset in assets:
        revision=str(ASSETS.get_metadata_tag(asset,'RomanCampSourceRevision'))
        source_sha=str(ASSETS.get_metadata_tag(asset,'RomanCampSourceManifestSha256'))
        legacy=SOURCE_REVISION=='semireal_v1' and ATTEMPT=='Camp01' and not revision and not source_sha
        require(legacy or (revision==SOURCE_REVISION and source_sha==result['source_manifest_sha256']),
                'Namespace source revision/manifest mismatch')
    expected = {item['name']:item for item in manifest['meshes']}
    meshes = [a for a in assets if isinstance(a,unreal.StaticMesh)]
    textures = [a for a in assets if isinstance(a,unreal.Texture2D)]
    materials = [a for a in assets if isinstance(a,unreal.Material)]
    result['asset_classes'] = {a.get_path_name():a.get_class().get_name() for a in assets}
    require(len(meshes) == 3 and len(materials) == 6, 'Expected3static meshes/6materials')
    require(len(textures) == 3, 'Expected exactly3shared atlas textures, inspect actual import assets')
    details = []
    for mesh in meshes:
        require(mesh.get_name() in expected, 'Unexpected mesh name')
        source = expected[mesh.get_name()]
        bounds = mesh.get_bounding_box()
        size = [getattr(bounds.max,k)-getattr(bounds.min,k) for k in ('x','y','z')]
        triangles = mesh.get_num_triangles(0)
        require(triangles == source['triangles'], 'Native triangle count differs: '+mesh.get_name()+'/'+str(triangles))
        require(all(abs(a-b*100) < .1 for a,b in zip(size,source['dimensions_m'])), 'Native cm/Z-up bounds mismatch')
        # StaticMeshEditorSubsystem is not instantiated in this commandlet.
        # Use the public reflected mesh API, not an uninitialized subsystem or
        # deprecated direct BodySetup access. Native proof covers collision.
        require(mesh.get_num_lods() == 1 and mesh.get_num_tex_coords(0) == 1, 'Unexpected LOD/UV generation')
        slots = mesh.get_editor_property('static_materials')
        bindings = [s.get_editor_property('material_interface') for s in slots]
        require(all(m in materials for m in bindings), 'Material binding outside owned namespace')
        details.append(dict(path=mesh.get_path_name(),triangles=triangles,vertices=mesh.get_num_vertices(0),
                            dimensions_cm=size,bounds_min=[getattr(bounds.min,k) for k in ('x','y','z')],
                            bounds_max=[getattr(bounds.max,k) for k in ('x','y','z')],
                            uv_channels=mesh.get_num_tex_coords(0),simple_collision_verified=False,
                            material_bindings=[m.get_path_name() for m in bindings]))
    tex_details = []
    for tex in textures:
        name = tex.get_name()
        size = [tex.blueprint_get_size_x(),tex.blueprint_get_size_y()]
        require(size == [2048,2048], 'Actual Unreal texture size mismatch: '+name+'/'+str(size))
        normal = tex.get_editor_property('compression_settings') == unreal.TextureCompressionSettings.TC_NORMALMAP
        srgb = bool(tex.get_editor_property('srgb'))
        require(srgb == ('BaseColor' in name), 'PBR texture color-space mismatch: '+name)
        require(normal == ('Normal' in name), 'Normal compression mismatch: '+name)
        tex_details.append(dict(path=tex.get_path_name(),size=size,srgb=srgb,compression=str(tex.get_editor_property('compression_settings')),
                                flip_green=bool(tex.get_editor_property('flip_green_channel'))))
    graphs = {}
    lib = unreal.MaterialEditingLibrary
    for mat in materials:
        props = (('attributes',unreal.MaterialProperty.MP_MATERIAL_ATTRIBUTES),) if mat.get_editor_property('use_material_attributes') else (
            ('base',unreal.MaterialProperty.MP_BASE_COLOR),('normal',unreal.MaterialProperty.MP_NORMAL),
            ('roughness',unreal.MaterialProperty.MP_ROUGHNESS),('metallic',unreal.MaterialProperty.MP_METALLIC))
        graph = {}
        for label,prop in props:
            actual = bound_textures(mat,prop)
            require(actual and actual.issubset(set(textures)), 'Material uses absent/foreign textures')
            graph[label] = dict(textures=sorted(t.get_path_name() for t in actual),
                                root_output=lib.get_material_property_input_node_output_name(mat,prop))
        graphs[mat.get_path_name()] = graph
    result.update(meshes=details,textures=tex_details,material_graphs=graphs,asset_count=len(assets))


before = snapshot()
result = dict(status='running',scope='Private static camp import/readback, NOT render/nav/player/owner acceptance',
              folder=FOLDER,read_only=READ_ONLY,owner_approved=False,main_game_assets_changed=False,
              source_revision=SOURCE_REVISION,source_directory=SOURCE.relative_to(ROOT).as_posix(),
              before_content_sha256=before)
try:
    manifest,source_sha = read_source_manifest(SOURCE,SOURCE_REVISION)
    result['source_manifest_sha256'] = source_sha
    if not READ_ONLY:
        result['imports'] = import_new(manifest)
    verify(manifest)
    result['status'] = 'pass'
except Exception as error:
    result.update(status='fail',error=str(error),traceback=traceback.format_exc())
    raise
finally:
    after = snapshot()
    result['existing_content_unchanged'] = before == after
    result['existing_content_count'] = len(before)
    if before != after:
        result.update(status='fail',content_changes=sorted(k for k in before.keys()|after.keys() if before.get(k) != after.get(k)))
    REPORT.write_text(json.dumps(result,indent=2),encoding='utf-8')
    unreal.log('ROMAN_CAMP_IMPORT '+result['status'])
    require(before == after,'Original private Content changed; inspect before proceeding')
