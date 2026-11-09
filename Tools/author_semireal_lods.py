"""Create/read LOD candidates only in the exact private Unreal preview.

Uses installed SkeletalMeshEditorSubsystem -> Interchange custom-LOD import.
Does not replace Runtime05, alter its skeleton/materials, or adopt main art.
Native topology/skin/pose and GPU transition verification are separate gates.
"""
import hashlib
import json
from pathlib import Path
import re
import stat
import sys
import traceback

import unreal

ROOT = Path(__file__).resolve().parent.parent
PROJECT = ROOT / 'Artifacts/VQ01_Semireal1/GamePreview'


def require(condition, message):
    if not condition:
        raise RuntimeError(message)


def reject_reparse(path):
    for ancestor in (path, *path.parents):
        if ancestor.exists():
            require(not (ancestor.lstat().st_file_attributes & stat.FILE_ATTRIBUTE_REPARSE_POINT),
                    'Refusing a reparse point: ' + str(ancestor))


reject_reparse(PROJECT)
require(Path(unreal.Paths.project_dir()).resolve() == PROJECT.resolve(),
        'Refusing LOD authoring outside the exact private game preview')


def argument(name):
    values = [s.split('=', 1)[1] for s in sys.argv if s.startswith('--' + name + '=')]
    require(len(values) == 1, 'Exactly one --' + name + '= value required')
    return values[0]


ATTEMPT = argument('attempt')
require(re.fullmatch(r'LOD[0-9]+', ATTEMPT), 'Use a fresh LODNN namespace')
REPORT_NAME = argument('report')
require(re.fullmatch(r'[a-zA-Z0-9_-]+\.json', REPORT_NAME), 'Report must be a simple JSON filename')
REPORT = PROJECT / REPORT_NAME
reject_reparse(REPORT)
require(not REPORT.exists(), 'Report exists; refusing overwrite')
READ_ONLY = '--read-only' in sys.argv
BASE = '/Game/ArtCandidates/Attempt10_1'
FOLDER = BASE + '/' + ATTEMPT
SOURCE = BASE + '/Runtime05'
DISK_FOLDER = PROJECT / 'Content/ArtCandidates/Attempt10_1' / ATTEMPT
reject_reparse(DISK_FOLDER)
SOURCE_ART = ROOT / 'SourceArt/Characters/RomanSoldier/semireal_v2'
SOURCES = (
    ('SK_Roman_Rome_Semireal01.glb', '3157ac079a631d6fa2a317d550b0b3aeb9a56be9d752d423c478f7d50e0211f6'),
    ('SK_Roman_Rome_Semireal01_LOD1.glb', '99e5d203ba969c17704148dcce886c03ad781d194bcf6ddfe60ddd8e2e44d1c0'),
    ('SK_Roman_Rome_Semireal01_LOD2.glb', '6c61948ebe48db34494dfdb6f71a4994d15940b101166a33c422b5ddacad1110'),
)
ASSETS = unreal.EditorAssetLibrary
TAG = 'RomanSemirealLODSchema'
result = dict(status='running', project=str(PROJECT), folder=FOLDER, read_only=READ_ONLY,
              source_bundle=SOURCE + '/DA_RuntimePreview', game_main_assets_changed=False,
              topology_skin_pose_verified=False, gpu_transitions_verified=False,
              performance_verified=False, owner_approved=False, imports=[])


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def snapshot_content():
    """Protect all existing private Content files, excluding the NEW namespace."""
    files = {}
    for path in (PROJECT / 'Content').rglob('*'):
        reject_reparse(path)
        if path.is_file() and (READ_ONLY or DISK_FOLDER not in path.parents):
            files[path.relative_to(PROJECT).as_posix()] = digest(path)
    return files


def slot_bindings(mesh):
    return [(str(s.get_editor_property('material_slot_name')),
             s.get_editor_property('material_interface').get_path_name())
            for s in mesh.get_editor_property('materials')]


def save(asset):
    ASSETS.set_metadata_tag(asset, TAG, '1')
    require(ASSETS.save_loaded_asset(asset, only_if_is_dirty=False), 'Saving new private asset failed')


before = snapshot_content()
try:
    source_records = []
    for filename, expected in SOURCES:
        path = SOURCE_ART / filename
        reject_reparse(path)
        require(path.is_file() and digest(path) == expected, 'Source GLB hash changed: ' + filename)
        source_records.append(dict(filename=filename, sha256=expected))
    result['source_files'] = source_records
    original = unreal.load_asset(SOURCE + '/SK_RuntimePreview')
    original_bundle = unreal.load_asset(SOURCE + '/DA_RuntimePreview')
    require(isinstance(original, unreal.SkeletalMesh), 'Runtime05 mesh missing')
    require(isinstance(original_bundle, unreal.RomanCharacterPresentation), 'Runtime05 bundle missing')
    subsystem = unreal.get_editor_subsystem(unreal.SkeletalMeshEditorSubsystem)
    require(subsystem.get_lod_count(original) == 1, 'Runtime05 no longer has exactly LOD0')
    bindings = slot_bindings(original)
    require([s[0] for s in bindings] == ['Body', 'Metal', 'Skin', 'Leather', 'Shield', 'Crest', 'MailLinks', 'EyeWhite'],
            'Runtime05 material semantics changed')
    require(original.get_editor_property('physics_asset') is None, 'Unexpected Runtime05 physics asset')
    if READ_ONLY:
        require(ASSETS.does_directory_exist(FOLDER), 'Read-only namespace missing')
    else:
        require(not DISK_FOLDER.exists() and not ASSETS.does_directory_exist(FOLDER),
                'LOD namespace exists; refusing overwrite/partial repair')
        mesh = ASSETS.duplicate_asset(original.get_path_name(), FOLDER + '/SK_LODPreview')
        require(isinstance(mesh, unreal.SkeletalMesh), 'LOD mesh duplication failed')
        # Persist the recovery point before native import. Failed attempts stay visible.
        save(mesh)
        for index, (filename, _) in enumerate(SOURCES[1:], start=1):
            returned = subsystem.import_lod(mesh, index, (SOURCE_ART / filename).as_posix())
            count = subsystem.get_lod_count(mesh)
            result['imports'].append(dict(index=index, returned=returned, actual_lod_count=count))
            require(returned == index and count == index + 1, 'Native LOD import did not produce requested LOD')
            require(slot_bindings(mesh) == bindings, 'LOD import changed PBR materials/slot order')
            require(mesh.get_editor_property('skeleton') == original.get_editor_property('skeleton'),
                    'LOD import changed skeleton object')
            require(mesh.get_editor_property('physics_asset') is None, 'LOD import introduced physics authority')
            save(mesh)
        bundle = ASSETS.duplicate_asset(original_bundle.get_path_name(), FOLDER + '/DA_LODPreview')
        require(isinstance(bundle, unreal.RomanCharacterPresentation), 'LOD bundle duplication failed')
        for field in ('rome_mesh', 'carthage_mesh', 'gaul_mesh'):
            bundle.set_editor_property(field, mesh)
        save(bundle)
    mesh = unreal.load_asset(FOLDER + '/SK_LODPreview')
    bundle = unreal.load_asset(FOLDER + '/DA_LODPreview')
    require(isinstance(mesh, unreal.SkeletalMesh) and isinstance(bundle, unreal.RomanCharacterPresentation),
            'LOD candidate missing/partial')
    require(subsystem.get_lod_count(mesh) == 3, 'Expected three actual LODs')
    require(slot_bindings(mesh) == bindings, 'LOD material bindings differ from Runtime05')
    require(mesh.get_editor_property('skeleton') == original.get_editor_property('skeleton'), 'Skeleton changed')
    require(mesh.get_editor_property('physics_asset') is None, 'Physics asset changed')
    for field in ('animation_class', 'attack_montage', 'use_mesh_material_parents', 'faction_tint_slots'):
        require(bundle.get_editor_property(field) == original_bundle.get_editor_property(field),
                'Bundle presentation field changed: ' + field)
    for field in ('rome_mesh', 'carthage_mesh', 'gaul_mesh'):
        require(bundle.get_editor_property(field) == mesh, 'Candidate mesh binding changed: ' + field)
        require(original_bundle.get_editor_property(field) == original, 'Source bundle was changed: ' + field)
    stats = []
    for index in range(3):
        vertices = subsystem.get_num_verts(mesh, index)
        sections = subsystem.get_num_sections(mesh, index)
        require(vertices > 0 and sections == 8, 'Empty LOD or unexpected section count')
        stats.append(dict(lod=index, vertices=vertices, sections=sections))
    require(stats[0]['vertices'] > stats[1]['vertices'] > stats[2]['vertices'], 'LODs do not reduce geometry')
    assets = ASSETS.list_assets(FOLDER, recursive=True, include_folder=False)
    require(len(assets) == 2, 'Unexpected assets in LOD namespace')
    for asset in (mesh, bundle):
        require(str(ASSETS.get_metadata_tag(asset, TAG)) == '1', 'Foreign LOD asset')
    require(subsystem.get_lod_count(original) == 1, 'Source mesh LOD count changed')
    result.update(status='pass', mesh=mesh.get_path_name(), bundle=bundle.get_path_name(),
                  lod_stats=stats, material_bindings=bindings, asset_count=len(assets),
                  skeleton=mesh.get_editor_property('skeleton').get_path_name())
except Exception as error:
    result.update(status='fail', error=str(error), traceback=traceback.format_exc())
    raise
finally:
    after = snapshot_content()
    result['existing_private_content_files'] = len(before)
    result['existing_private_content_unchanged'] = before == after
    if before != after:
        result.update(status='fail', content_changes=sorted(k for k in before.keys() | after.keys()
                                                          if before.get(k) != after.get(k)))
    REPORT.write_text(json.dumps(result, indent=2), encoding='utf-8')
    unreal.log('SEMIREAL_LODS ' + result['status'])
    require(before == after, 'Existing private Content was changed; inspect receipt')
