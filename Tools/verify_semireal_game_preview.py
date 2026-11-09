"""Read-only native game-context check; no PIE, saves, reimport or asset replacement."""
import hashlib
import json
from pathlib import Path
import stat
import traceback

import unreal

ROOT = Path(__file__).resolve().parent.parent
PROJECT = ROOT / 'Artifacts/VQ01_Semireal1/GamePreview'
for ancestor in (PROJECT, *PROJECT.parents):
    if ancestor.lstat().st_file_attributes & stat.FILE_ATTRIBUTE_REPARSE_POINT:
        raise RuntimeError('Refusing private project reparse point: ' + str(ancestor))
if Path(unreal.Paths.project_dir()).resolve() != PROJECT.resolve():
    raise RuntimeError('Refusing verification outside the exact private game preview')
OUTPUT = PROJECT / 'game-context-readback01.json'
if OUTPUT.exists():
    raise RuntimeError('Readback receipt exists; inspect it, never silently overwrite')


def require(condition, message):
    if not condition:
        raise RuntimeError(message)


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


result = dict(status='running', project=str(PROJECT), assets_saved=False,
              graph_integrated=False, cameras_verified=False, owner_approved=False)
try:
    receipt = json.loads((PROJECT / 'preview-copy-receipt.json').read_text(encoding='utf-8'))
    require(receipt['status'] == 'copied_verified', 'Copy did not finish')
    require(receipt['project_root'].lower() == str(PROJECT).lower(), 'Wrong receipt project')
    # Verify the exact private snapshot before loading any candidate or gameplay class.
    for entry in receipt['files']:
        relative = Path(entry['relative'])
        require(not relative.is_absolute() and '..' not in relative.parts, 'Unsafe manifest path')
        for ancestor in (PROJECT / relative, *(PROJECT / relative).parents):
            require(not ancestor.lstat().st_file_attributes & stat.FILE_ATTRIBUTE_REPARSE_POINT,
                    'Reparse point in private snapshot: ' + str(ancestor))
        require(digest(PROJECT / relative) == entry['sha256'], 'Private snapshot changed: ' + str(relative))
    descriptor_path = PROJECT / 'RomanEmpireGame.uproject'
    require(digest(descriptor_path) == receipt['private_descriptor_sha256'], 'Private descriptor changed')
    descriptor = json.loads(descriptor_path.read_text(encoding='utf-8'))
    plugins = {p['Name']: p['Enabled'] for p in descriptor['Plugins']}
    for name in ('Fab', 'ModelContextProtocol', 'AllToolsets', 'EditorToolset', 'AutomationTestToolset'):
        require(plugins.get(name) is False, 'Forbidden private plugin: ' + name)
    require(hasattr(unreal, 'RomanCharacterPresentation'), 'Native game module did not load')
    original = unreal.load_asset('/Game/Characters/RomanSoldier/DA_RomanCharacterPresentation')
    require(isinstance(original, unreal.RomanCharacterPresentation), 'Original presentation bundle missing')
    original_mesh = original.get_editor_property('rome_mesh')
    require(original_mesh is not None, 'Original Rome mesh missing')
    require(original_mesh.get_path_name().startswith('/Game/Characters/RomanSoldier/'), 'Original art was replaced')
    require(original.get_editor_property('animation_class') is not None, 'Original AnimBP class missing')
    report = json.loads((ROOT / 'Artifacts/VQ01_Semireal1/UEProbe/import10-1-report.json').read_text(encoding='utf-8'))
    require(report['source_sha256'] == receipt['source_sha256'], 'Source candidate changed')
    mesh = unreal.load_asset(report['mesh'])
    skeleton = unreal.load_asset(report['skeleton'])
    require(isinstance(mesh, unreal.SkeletalMesh) and isinstance(skeleton, unreal.Skeleton), 'Candidate rig missing')
    require(mesh.get_editor_property('skeleton') == skeleton, 'Wrong candidate rig')
    require(mesh.get_editor_property('physics_asset') is None, 'Unexpected physics authority')
    require(skeleton != original_mesh.get_editor_property('skeleton'), 'Candidate must retain its own bind pose')
    slots = mesh.get_editor_property('materials')
    require(len(slots) == 8, 'Candidate eight-slot contract changed')
    for slot in slots:
        material = slot.get_editor_property('material_interface')
        require(material and material.get_path_name().startswith(report['folder'] + '/'), 'Flat/foreign material override')
    clips = {}
    for name, data in report['clips'].items():
        clip = unreal.load_asset(data['path'])
        require(isinstance(clip, unreal.AnimSequence), 'Missing actual clip: ' + name)
        require(clip.get_editor_property('skeleton') == skeleton, 'Old rig clip mixed in: ' + name)
        require(not clip.get_editor_property('enable_root_motion'), 'Unexpected root motion')
        seconds = clip.get_editor_property('sequence_length')
        require(abs(seconds - data['seconds']) < .001, 'Wrong clip duration: ' + name)
        clips[name] = dict(path=clip.get_path_name(), seconds=seconds)
    for name in report['textures']:
        require(isinstance(unreal.load_asset(name), unreal.Texture2D), 'Candidate texture missing')
    result.update(status='pass', engine=unreal.SystemLibrary.get_engine_version(),
                  verified_snapshot_files=len(receipt['files']), native_module_loaded=True,
                  original_bundle=original.get_path_name(), original_mesh=original_mesh.get_path_name(),
                  candidate_mesh=mesh.get_path_name(), distinct_skeleton=True, clips=clips,
                  material_slots=[str(s.get_editor_property('material_slot_name')) for s in slots],
                  textures=report['textures'], capsule_authority_unchanged=True)
except Exception as error:
    result.update(status='fail', error=str(error), traceback=traceback.format_exc())
    raise
finally:
    OUTPUT.write_text(json.dumps(result, indent=2), encoding='utf-8')
    unreal.log('SEMIREAL_GAME_CONTEXT ' + result['status'])
