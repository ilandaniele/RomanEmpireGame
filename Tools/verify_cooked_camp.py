"""Verify actual private loose cook/dump/shader artifacts; no Unreal/player claim."""
import argparse
import hashlib
import json
from pathlib import Path
import re

from packaged_dependency_inventory import safe_path
from semireal_camp_import_policy import NAMES, TRIANGLES, read_source_manifest

ROOT = Path(__file__).resolve().parent.parent
PREFIX = '/Game/ArtCandidates/Camp01/'
MESHES = dict(zip(NAMES,TRIANGLES['semireal_v1']))
MATERIALS = {'Soil','Wood','Iron','Limestone','Plaster','Terracotta'}
TEXTURES = {'BaseColor','Normal','Camp_Metallic-Camp_Roughness'}


def cook_profile(camp='Camp01'):
    profiles={'Camp01':('semireal_v1','Artifacts/VQ02_CampCook1'),
              'Camp02':('semireal_v2','Artifacts/VQ02_CampRevision2/Cook'),
              'Camp03':('semireal_v3','Artifacts/VQ02_CampRevision3/Cook')}
    require(camp in profiles,'Unsupported cook source profile')
    revision,output=profiles[camp]
    return dict(camp=camp,prefix='/Game/ArtCandidates/'+camp+'/',
                meshes=dict(zip(NAMES,TRIANGLES[revision])),
                source_revision=revision,output_root=ROOT/output)


def require(condition, message):
    if not condition:
        raise ValueError(message)


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def parse_dump(pages):
    sections = {}
    for text in pages:
        current = None
        for line in text.splitlines():
            match = re.fullmatch(r'--- Begin (\w+) ---',line)
            if match:
                current = match[1]
                require(current not in sections, 'Duplicate registry dump section')
                sections[current] = []
            elif line.startswith('--- End '):
                current = None
            elif current:
                sections[current].append(line)
    require({'CachedAssetsByObjectPath','CachedAssetsByPackageName','CachedAssetsByClass','AssetTags','CachedDependsNodes'} <= sections.keys(), 'Incomplete native dump')
    objects = {line[1:] for line in sections['CachedAssetsByObjectPath'] if line.startswith('\t/')}
    packages = set()
    for line in sections['CachedAssetsByPackageName']:
        match = re.fullmatch(r'\t(/.+) : \d+ item\(s\)',line)
        if match:
            packages.add(match[1])
    classes, current = {}, None
    for line in sections['CachedAssetsByClass']:
        match = re.fullmatch(r'\t(/.+) : \d+ item\(s\)',line)
        if match:
            current = match[1]
        elif line.startswith('\t\t/'):
            name = line[2:]
            require(current is not None and name not in classes, 'Duplicate/ungrouped asset class')
            classes[name] = current
    tags, current = {}, None
    for line in sections['AssetTags']:
        if line.startswith('  /') and not line.startswith('   '):
            current = line[2:]
            require(current not in tags, 'Duplicate tagged object')
            tags[current] = {}
        elif line.startswith('    ') and ' : ' in line and current:
            key, value = line[4:].split(' : ',1)
            require(key not in tags[current], 'Duplicate asset tag')
            tags[current][key] = value
    dependencies, current, category = {}, None, None
    for line in sections['CachedDependsNodes']:
        if line.startswith('\t') and not line.startswith('\t\t'):
            # Native FAssetIdentifier roots also include primary IDs such as
            # Map:/Game/...; they are not filesystem paths/package edges.
            current, category = line[1:], None
            require(current not in dependencies, 'Duplicate dependency node')
            dependencies[current] = []
        elif line.startswith('\t\t') and not line.startswith('\t\t\t'):
            category = line[2:]
        elif current and category == 'Package':
            if not line.strip():
                continue
            match = re.fullmatch(r'\t\t\t([^\t]+)\t+\{([^}]+)\}',line)
            require(match is not None, 'Unknown package dependency encoding: '+repr(line))
            dependencies[current].append((match[1],set(match[2].split(','))))
    return dict(objects=objects,packages=packages,classes=classes,tags=tags,dependencies=dependencies)


def hard_game_closure(dependencies, roots):
    pending, visited = list(roots), set()
    while pending:
        name = pending.pop()
        if name in visited:
            continue
        visited.add(name)
        if name.startswith('/Script/'):
            continue  # Runtime module identity, not a cooked content package.
        for child, flags in dependencies.get(name,[]):
            if {'Hard','Game'} <= flags and 'EditorOnly' not in flags:
                pending.append(child)
    return visited


def verify_shader_info(data,prefix=PREFIX):
    require(data.get('AssetInfoVersion') == 2, 'Unsupported native shader asset-info schema')
    result = {}
    for row in data.get('ShaderCodeToAssets',[]):
        value = row.get('ShaderMapHash','')
        require(re.fullmatch(r'[A-Fa-f0-9]{16}',value), 'Invalid native shader map identifier')
        for asset in row.get('Assets',[]):
            if asset.startswith(prefix):
                result.setdefault(asset,[]).append(value)
    require({prefix+n for n in MATERIALS} <= result.keys(), 'Cooked SM6 shader asset-info omits a camp material')
    return result


def read_dump(folder):
    paths = sorted(folder.glob('Page_*.txt'))
    require(bool(paths), 'Missing actual native dump pages')
    return parse_dump([decode_dump(p.read_bytes()) for p in paths]), paths


def decode_dump(raw):
    # Native SaveStringToFile auto-selects UTF16 when a page contains Unicode.
    # Preserve strict decoding; never silently replace invalid registry bytes.
    return raw.decode('utf-16' if raw.startswith((b'\xff\xfe',b'\xfe\xff')) else 'utf-8-sig')


def verify(attempt,camp='Camp01'):
    profile=cook_profile(camp)
    attempt=safe_path(attempt,(profile['output_root'],))
    prefix,meshes=profile['prefix'],profile['meshes']
    process = json.loads((attempt/'process.json').read_text(encoding='utf-8-sig'))
    exit_info = json.loads((attempt/'exit.json').read_text(encoding='utf-8-sig'))
    require(process.get('camp','Camp01')==camp and process.get('source_revision','semireal_v1')==profile['source_revision'],'Cook/source profile mismatch')
    require(exit_info['exit_code'] == 0 and process.get('loose_package_writer') is True, 'Not the successful loose cook')
    require(process.get('additional_shader_format') == 'PCD3D_SM6', 'SM6 not requested explicitly')
    require((attempt/'cook.stderr.log').stat().st_size == 0, 'Unexpected cook stderr')
    log = (attempt/'cook.log').read_text(encoding='utf-8-sig')
    require('Success - 0 error(s), 0 warning(s)' in log, 'Actual cook not clean')
    counts = re.findall(r'Packages Cooked: (\d+), Packages Incrementally Skipped: (\d+), Packages Skipped by Platform: (\d+), Total Packages: (\d+)',log)
    require(len(counts) == 1, 'Missing/ambiguous native package counts')
    cooked_count, incremental, platform_skip, total = map(int,counts[0])
    require(cooked_count+incremental+platform_skip == total and incremental == 0, 'Native package totals inconsistent')
    project = Path(process['project']).resolve().parent
    require(project == ROOT/'Artifacts/VQ01_Semireal1/GamePreview', 'Wrong private project')
    source=ROOT/'SourceArt/Environment/RomanCamp'/profile['source_revision']
    _,source_sha=read_source_manifest(source,profile['source_revision'])
    if camp!='Camp01' or 'source_manifest_sha256' in process:
        require(process.get('source_manifest_sha256','').lower()==source_sha,'Cook source manifest changed')
    require('-CookDir="'+str(project/'Content/ArtCandidates'/camp)+'"' in process['arguments'],'CookDir does not select this camp')
    require(digest(project/'RomanEmpireGame.uproject') == process['descriptor_sha256'].lower(), 'Descriptor changed')
    for base, rows in ((project/'Content',process['content_before']),
                       (source,process['source_before']),
                       (project/'Config',process['config_before'])):
        require(len(list(base.rglob('*'))) >= len(rows), 'Missing protected tree')
        require(len([p for p in base.rglob('*') if p.is_file()]) == len(rows), 'Protected file count changed')
        for row in rows:
            path = (base/row['path'].replace('\\','/')).resolve()
            path.relative_to(base)
            require(digest(path) == row['sha256'].lower(), 'Protected source/content/config changed')
    runtime, runtime_pages = read_dump(attempt/'RegistryRuntime01')
    development, development_pages = read_dump(attempt/'RegistryDevelopment01')
    cooked = attempt/'Cooked/Windows/RomanEmpireGame'
    for kind, filename in (('Runtime','AssetRegistry.bin'),('Development','Metadata/DevelopmentAssetRegistry.bin')):
        info = json.loads((attempt/f'registry-{kind}01-exit.json').read_text(encoding='utf-8-sig'))
        require(info['exit_code'] == 0 and info['input_unchanged'], 'Native registry dump not verified')
        require(digest(cooked/filename) == info['input_sha256'].lower(), 'Registry changed after native dump')
    assets = []
    files = []
    for name in sorted(set(meshes)|MATERIALS|TEXTURES):
        package, object_path = prefix+name, prefix+name+'.'+name
        expected_class = '/Script/Engine.'+('StaticMesh' if name in meshes else 'Material' if name in MATERIALS else 'Texture2D')
        require(package in runtime['packages'] and object_path in runtime['objects'], 'Actual cooked runtime registry omits '+name)
        require(runtime['classes'].get(object_path) == expected_class, 'Actual cooked class mismatch')
        for extension in ('.uasset','.uexp'):
            path = cooked/('Content/ArtCandidates/'+camp+'/'+name+extension)
            require(path.is_file() and path.stat().st_size > 0, 'Missing actual cooked package/export bytes')
            files.append(path)
        tags = runtime['tags'].get(object_path,{})
        if name in meshes:
            require(tags.get('Triangles') == str(meshes[name]) and tags.get('LODs') == '1' and tags.get('CollisionPrims') == '0', 'Cooked mesh topology/collision metadata mismatch')
        if name in TEXTURES:
            require(tags.get('Dimensions') == '2048x2048' and tags.get('SRGB') == ('True' if name == 'BaseColor' else 'False'), 'Cooked texture dimensions/color-space metadata mismatch')
            require(tags.get('CompressionSettings') == ('TC_Normalmap' if name == 'Normal' else 'TC_Default'), 'Cooked compression metadata mismatch')
            bulk = cooked/('Content/ArtCandidates/'+camp+'/'+name+'.ubulk')
            require(bulk.is_file() and bulk.stat().st_size > 0, 'Actual cooked texture bulk missing')
            files.append(bulk)
        assets.append(dict(package=package,object_path=object_path,asset_class=expected_class,tags=tags))
    roots = {prefix+n for n in meshes}
    closure = hard_game_closure(development['dependencies'],roots)
    require(len(closure) > 12, 'No actual development dependency graph')
    missing = sorted(p for p in closure if not p.startswith('/Script/') and p not in runtime['packages'])
    require(not missing, 'Hard game dependency absent from cooked registry: '+str(missing))
    require(not any('/Game/Tests/' in p or 'RomanEmpireGameTests' in p for p in closure), 'Camp depends on editor test content/module')
    shader_info = cooked/'Content/ShaderAssetInfo-RomanEmpireGame-PCD3D_SM6-PCD3D_SM6.assetinfo.json'
    shader_assets = verify_shader_info(json.loads(shader_info.read_text(encoding='utf-8-sig')),prefix)
    files.append(shader_info)
    for library in ('Global','RomanEmpireGame'):
        for shader_format in ('PCD3D_SM5','PCD3D_SM6'):
            path = cooked/f'Content/ShaderArchive-{library}-{shader_format}-{shader_format}.ushaderbytecode'
            require(path.is_file() and path.stat().st_size > 0, 'Actual compiled shader archive absent')
            require(f"Shader Library '{library}' ({shader_format}-{shader_format}) Stats:" in log, 'Missing native shader-library stats')
            files.append(path)
    observations = {}
    for name in TEXTURES:
        path = cooked/('Content/ArtCandidates/'+camp+'/'+name+'.uexp')
        tokens = sorted(set(re.findall(rb'PF_[A-Z0-9_]+',path.read_bytes())))
        observations[name] = [s.decode('ascii') for s in tokens]
    files.extend(runtime_pages+development_pages+[cooked/'AssetRegistry.bin',cooked/'Metadata/DevelopmentAssetRegistry.bin',attempt/'cook.log',attempt/'process.json',attempt/'exit.json'])
    return dict(schema=1,scope='Directed private Windows content cook with SM5+SM6, NOT standalone production reachability/player/owner approval',
                camp=camp,source_revision=profile['source_revision'],source_manifest_sha256=source_sha,
                cook_counts=dict(cooked=cooked_count,incremental=incremental,platform_skipped=platform_skip,total=total),
                assets=assets,hard_game_dependency_closure=sorted(closure),missing_cooked_hard_game_dependencies=missing,
                runtime_dependency_graph_serialized=bool(runtime['dependencies']),
                dependency_source='Native DevelopmentAssetRegistry dump; runtime registry intentionally omits dependencies',
                sm6_material_shader_maps=shader_assets,
                texture_format_token_observations=observations,
                texture_observation_limit='ASCII serialized PF_* tokens in named uexp only; not a complete texture/package decoder or player load',
                protected_content_files=len(process['content_before']),protected_source_files=len(process['source_before']),
                owner_approved=False,player_verified=False,main_game_adopted=False,
                files=[dict(path=str(p.relative_to(ROOT)),bytes=p.stat().st_size,sha256=digest(p)) for p in files])


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('--name',required=True)
    parser.add_argument('--camp',choices=('Camp01','Camp02','Camp03'),default='Camp01')
    args = parser.parse_args()
    require(re.fullmatch(r'[A-Za-z0-9_]{1,48}',args.name), 'Safe fresh cook namespace required')
    attempt = cook_profile(args.camp)['output_root']/args.name
    destination = attempt/'verified01.json'
    require(not destination.exists(), 'Verification receipt exists; never overwrite')
    result = verify(attempt,args.camp)
    with destination.open('x',encoding='utf-8') as handle:
        json.dump(result,handle,indent=2)
    print(json.dumps(dict(assets=len(result['assets']),hard_game_closure=len(result['hard_game_dependency_closure']),
                         missing=result['missing_cooked_hard_game_dependencies'],
                         sm6_materials=len(result['sm6_material_shader_maps']),player_verified=False),indent=2))
