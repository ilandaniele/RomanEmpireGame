"""Create/read textured runtime art ONLY in the exact private game preview."""
import json
from pathlib import Path
import re
import stat
import sys
import traceback

import unreal

ROOT = Path(__file__).resolve().parent.parent
PROJECT = ROOT / 'Artifacts/VQ01_Semireal1/GamePreview'
for ancestor in (PROJECT, *PROJECT.parents):
    if ancestor.lstat().st_file_attributes & stat.FILE_ATTRIBUTE_REPARSE_POINT:
        raise RuntimeError('Refusing a private project reparse point')
if Path(unreal.Paths.project_dir()).resolve() != PROJECT.resolve():
    raise RuntimeError('Refusing runtime material authoring outside the exact private game preview')


def argument(name):
    values = [s.split('=', 1)[1] for s in sys.argv if s.startswith('--' + name + '=')]
    if len(values) != 1:
        raise RuntimeError('Exactly one --' + name + '= value required')
    return values[0]


attempt = argument('attempt')
if not re.fullmatch(r'Runtime[0-9]+', attempt):
    raise RuntimeError('Use a fresh RuntimeNN namespace')
report_name = argument('report')
if not re.fullmatch(r'[a-zA-Z0-9_-]+\.json', report_name):
    raise RuntimeError('Report must be a simple private-project JSON filename')
REPORT = PROJECT / report_name
if REPORT.exists():
    raise RuntimeError('Report exists; refusing overwrite')
READ_ONLY = '--read-only' in sys.argv
CANDIDATE = '/Game/ArtCandidates/Attempt10_1'
FOLDER = CANDIDATE + '/' + attempt
ASSETS = unreal.EditorAssetLibrary
LIB = unreal.MaterialEditingLibrary
TAG = 'RomanSemirealRuntimeSchema'
result = dict(status='running', project=str(PROJECT), folder=FOLDER, read_only=READ_ONLY,
              game_main_assets_changed=False, owner_approved=False, gpu_verified=False)


def require(condition, message):
    if not condition:
        raise RuntimeError(message)


def textures(material, prop):
    pending = [LIB.get_material_property_input_node(material, prop)]
    seen, found = set(), set()
    while pending:
        node = pending.pop()
        if node is None or node in seen:
            continue
        seen.add(node)
        if isinstance(node, unreal.MaterialExpressionTextureBase):
            texture = node.get_editor_property('texture')
            if texture:
                found.add(texture.get_path_name())
        pending.extend(LIB.get_inputs_for_material_expression(material, node))
    return found


def connect(source, output, target, pin):
    # UE's editor API resolves an empty name to GetInput(0). Desaturation and
    # OneMinus expose their first input without the C++ field name "Input".
    target_pin = '' if pin == 'Input' else pin
    require(LIB.connect_material_expressions(source, output, target, target_pin),
            'Material connection failed: ' + pin + '; actual inputs=' +
            str(LIB.get_material_expression_input_names(target)) + '; source outputs=' +
            str(LIB.get_material_expression_output_names(source)) + '; requested=' + output)


def node(material, cls, label):
    expression = LIB.create_material_expression(material, cls, 400, 200)
    require(expression is not None, 'Material expression creation failed: ' + label)
    expression.set_editor_property('desc', 'RomanSemirealRuntime:' + label)
    return expression


def save(asset):
    ASSETS.set_metadata_tag(asset, TAG, '1')
    require(ASSETS.save_loaded_asset(asset, only_if_is_dirty=False), 'Saving private new asset failed')


def make_material(source, name):
    material = ASSETS.duplicate_asset(source.get_path_name(), FOLDER + '/' + name)
    require(isinstance(material, unreal.Material), 'Material duplicate failed')
    base = LIB.get_material_property_input_node(material, unreal.MaterialProperty.MP_BASE_COLOR)
    output = LIB.get_material_property_input_node_output_name(material, unreal.MaterialProperty.MP_BASE_COLOR)
    require(base is not None, 'Imported material has no base-color source')
    color = node(material, unreal.MaterialExpressionVectorParameter, 'Color')
    color.set_editor_property('parameter_name', 'Color')
    color.set_editor_property('default_value', unreal.LinearColor(1, 1, 1, 1))
    tint = node(material, unreal.MaterialExpressionScalarParameter, 'FactionTintStrength')
    tint.set_editor_property('parameter_name', 'FactionTintStrength')
    tint.set_editor_property('default_value', 0.)
    grey = node(material, unreal.MaterialExpressionDesaturation, 'PreserveTextileLuminance')
    full = node(material, unreal.MaterialExpressionConstant, 'FullDesaturation')
    full.set_editor_property('r', 1.)
    connect(base, output, grey, 'Input'); connect(full, '', grey, 'Fraction')
    colored = node(material, unreal.MaterialExpressionMultiply, 'TintedTextile')
    connect(grey, '', colored, 'A'); connect(color, '', colored, 'B')
    choose = node(material, unreal.MaterialExpressionLinearInterpolate, 'OnlySemanticFactionSlots')
    connect(base, output, choose, 'A'); connect(colored, '', choose, 'B'); connect(tint, '', choose, 'Alpha')
    damage = node(material, unreal.MaterialExpressionScalarParameter, 'Damage')
    damage.set_editor_property('parameter_name', 'Damage'); damage.set_editor_property('default_value', 0.)
    depth = node(material, unreal.MaterialExpressionMultiply, 'DamageAmount')
    depth.set_editor_property('const_b', .35); connect(damage, '', depth, 'A')
    remaining = node(material, unreal.MaterialExpressionOneMinus, 'DamageDarkening')
    connect(depth, '', remaining, 'Input')
    final = node(material, unreal.MaterialExpressionMultiply, 'TexturedDamagedColor')
    connect(choose, '', final, 'A'); connect(remaining, '', final, 'B')
    require(LIB.connect_material_property(final, '', unreal.MaterialProperty.MP_BASE_COLOR), 'Final material property connection failed')
    material.set_editor_property('used_with_skeletal_mesh', True)
    LIB.recompile_material(material)
    save(material)
    return material


try:
    original = unreal.load_asset(CANDIDATE + '/SK_Roman_Rome_Semireal01')
    graph_bundle = unreal.load_asset(CANDIDATE + '/IntegrationV3/DA_SemirealPreview')
    require(isinstance(original, unreal.SkeletalMesh), 'Verified candidate mesh missing')
    require(isinstance(graph_bundle, unreal.RomanCharacterPresentation), 'Verified native graph bundle missing')
    require(graph_bundle.get_editor_property('rome_mesh') == original, 'Graph bundle changed')
    original_slots = original.get_editor_property('materials')
    require(len(original_slots) == 8, 'Candidate slot count changed')
    if READ_ONLY:
        require(ASSETS.does_directory_exist(FOLDER), 'Read-only folder missing')
    else:
        require(not ASSETS.does_directory_exist(FOLDER), 'Runtime namespace exists; refusing overwrite/partial repair')
        materials = []
        for slot in original_slots:
            parent = slot.get_editor_property('material_interface')
            require(isinstance(parent, unreal.Material), 'Unexpected original material type')
            materials.append(make_material(parent, 'M_Runtime_' + str(slot.get_editor_property('material_slot_name'))))
        mesh = ASSETS.duplicate_asset(original.get_path_name(), FOLDER + '/SK_RuntimePreview')
        require(isinstance(mesh, unreal.SkeletalMesh), 'Runtime mesh duplicate failed')
        slots = mesh.get_editor_property('materials')
        for index, material in enumerate(materials):
            slot = slots[index]; slot.set_editor_property('material_interface', material); slots[index] = slot
        mesh.set_editor_property('materials', slots); save(mesh)
        bundle = ASSETS.duplicate_asset(graph_bundle.get_path_name(), FOLDER + '/DA_RuntimePreview')
        require(isinstance(bundle, unreal.RomanCharacterPresentation), 'Runtime bundle duplicate failed')
        for field in ('rome_mesh', 'carthage_mesh', 'gaul_mesh'):
            bundle.set_editor_property(field, mesh)
        bundle.set_editor_property('use_mesh_material_parents', True)
        # In this semi-real mesh the inherited "Crest" slot is BRASS fittings
        # (rivets, sword hilt, trim), not a cloth team crest. Preserve its gold.
        bundle.set_editor_property('faction_tint_slots', ['Body', 'Shield']); save(bundle)
    mesh = unreal.load_asset(FOLDER + '/SK_RuntimePreview')
    bundle = unreal.load_asset(FOLDER + '/DA_RuntimePreview')
    require(isinstance(mesh, unreal.SkeletalMesh) and isinstance(bundle, unreal.RomanCharacterPresentation), 'Actual runtime bundle missing')
    require(mesh.get_editor_property('skeleton') == original.get_editor_property('skeleton'), 'Candidate own skeleton lost')
    require(mesh.get_editor_property('physics_asset') is None, 'Unexpected mesh physics authority')
    for field in ('rome_mesh', 'carthage_mesh', 'gaul_mesh'):
        require(bundle.get_editor_property(field) == mesh, 'Runtime mesh reference changed: ' + field)
    for field in ('animation_class', 'attack_montage'):
        require(bundle.get_editor_property(field) == graph_bundle.get_editor_property(field), 'New native graph/montage not reused')
    require(bundle.get_editor_property('use_mesh_material_parents'), 'Runtime material policy opt-in missing')
    require([str(n) for n in bundle.get_editor_property('faction_tint_slots')] == ['Body', 'Shield'], 'Wrong candidate tint slots: brass must stay untinted')
    slots = mesh.get_editor_property('materials')
    require(len(slots) == 8, 'Runtime slots changed')
    properties = (unreal.MaterialProperty.MP_BASE_COLOR, unreal.MaterialProperty.MP_NORMAL,
                  unreal.MaterialProperty.MP_ROUGHNESS, unreal.MaterialProperty.MP_METALLIC)
    bindings = []
    for old_slot, slot in zip(original_slots, slots):
        name = str(slot.get_editor_property('material_slot_name'))
        require(name == str(old_slot.get_editor_property('material_slot_name')), 'Slot semantic changed')
        material = slot.get_editor_property('material_interface')
        require(isinstance(material, unreal.Material) and material.get_path_name().startswith(FOLDER + '/'), 'Wrong runtime parent')
        require(str(ASSETS.get_metadata_tag(material, TAG)) == '1', 'Foreign runtime material')
        source = old_slot.get_editor_property('material_interface')
        maps = {}
        for prop in properties:
            found = textures(material, prop)
            require(found == textures(source, prop), 'Lost original PBR property texture bindings: ' + name + '/' + str(prop))
            maps[str(prop)] = sorted(found)
        inputs = LIB.get_scalar_parameter_names(material)
        require({'Damage', 'FactionTintStrength'} <= {str(n) for n in inputs}, 'Missing runtime scalar parameters')
        require('Color' in {str(n) for n in LIB.get_vector_parameter_names(material)}, 'Missing actor-local tint parameter')
        bindings.append(dict(slot=name, material=material.get_path_name(), maps=maps))
    assets = ASSETS.list_assets(FOLDER, recursive=True, include_folder=False)
    require(len(assets) == 10, 'Unexpected runtime namespace assets')
    for asset in (mesh, bundle):
        require(str(ASSETS.get_metadata_tag(asset, TAG)) == '1', 'Foreign runtime asset')
    result.update(status='pass', material_bindings=bindings, asset_count=len(assets),
                  bundle=bundle.get_path_name(), mesh=mesh.get_path_name(),
                  shader_graph_verified=True, runtime_gameplay_verified=False)
except Exception as error:
    result.update(status='fail', error=str(error), traceback=traceback.format_exc())
    raise
finally:
    REPORT.write_text(json.dumps(result, indent=2), encoding='utf-8')
    unreal.log('SEMIREAL_RUNTIME_MATERIALS ' + result['status'])
