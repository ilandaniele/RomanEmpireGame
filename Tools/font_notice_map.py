"""Map existing pinned QA font metadata to review actions; no extraction/legal approval."""
import argparse
import json
from pathlib import Path
import re

from packaged_dependency_inventory import (
    InventoryError, digest, file_record, read_json, relative_path, require, safe_path,
)

ROOT = Path(__file__).resolve().parent.parent
INPUT_DIR = ROOT/'Artifacts/C018H-FontAttribution2'
PINS = {
    'Fonts-Shipping1.json':'c2e9338472626e10ccfa4bc12c879dd3349f5c3b750baad28ef70dc32133dd71',
    'Fonts-Development1.json':'943d18007990e010a5ecbb7b9b18665485d1e9290d851c6c459972f13f4b7a84',
    'FontTpsSources1.json':'b6da23e66a4600ae65301a0e1c173c28e8c54b387f23c1812ebce2c943cd9379',
    'DroidMonoUpstream1.json':'daa1b70a46ff15b603cd8984ede5a6d226575feda3b650badacee9e71a71aa56',
    'RobotoPrimaryLookups1.json':'e5d56e9fa172a3322605e990382d000bec9d36d60073dfbab1bf3c5a5e560b7d',
}
OLD_ROBOTO = {'Roboto-'+face+'.ttf' for face in ('Black','BlackItalic','Bold','BoldCondensed','BoldCondensedItalic','BoldItalic','Italic','Regular')}
NEW_ROBOTO = {'Roboto-Light.ttf','Roboto-Medium.ttf'}
NOTO = {'NotoNaskhArabicUI-Regular.ttf','NotoSansThai-Regular.ttf'}
FONT_NAMES = OLD_ROBOTO | NEW_ROBOTO | NOTO | {'DroidSansFallback.ttf','DroidSansMono.ttf'}
FONT_PATHS = {'Engine/Content/Slate/Fonts/'+name for name in FONT_NAMES} | {'Engine/Content/Slate/Old/Tiles/Roboto-Bold.ttf'}
TPS_PATHS = {name:'Content/Slate/Fonts/'+name+'.tps' for name in ('Roboto','Noto','DroidSans')}


def values(metadata, field):
    rows = metadata.get('fields',{}).get(field,[])
    require(isinstance(rows,list) and len(rows) <= 1024, 'invalid_saved_name_records')
    for row in rows:
        require(isinstance(row,dict) and isinstance(row.get('value'),str) and
                0 < len(row['value']) <= 8192 and all(type(row.get(k)) is int for k in ('platform','encoding','language')),
                'invalid_saved_name_record')
    return rows


def index_inventory(data, configuration):
    require(data.get('schema') == 1 and data.get('configuration') == configuration and
            data.get('rightsReview') == 'pending_owner', 'wrong_inventory_scope')
    rows = data.get('files',[])
    require(isinstance(rows,list) and len(rows) == 15, 'unexpected_font_count')
    indexed = {}
    for row in rows:
        path, sha = row.get('path'), row.get('sha256','')
        require(path in FONT_PATHS and path not in indexed and re.fullmatch(r'[0-9a-f]{64}',sha), 'invalid_font_identity')
        require(sha == row.get('installedSourceSha256') and type(row.get('bytes')) is int and 0 < row['bytes'] <= 32*1024*1024, 'font_source_identity_mismatch')
        require(row.get('rightsReview') == 'pending_owner', 'unexpected_font_review_state')
        metadata = row.get('metadata',{})
        require(values(metadata,'copyright') and values(metadata,'version'), 'missing_copyright_or_version_metadata')
        present = bool(values(metadata,'licenseDescription') or values(metadata,'licenseURL'))
        require(metadata.get('licenseMetadata') == ('name_ids_13_14_present_unapproved' if present else 'name_ids_13_14_absent'), 'license_metadata_state_mismatch')
        indexed[path] = row
    require(set(indexed) == FONT_PATHS and len({r['sha256'] for r in rows}) == 14, 'font_alias_or_hash_count_mismatch')
    return indexed


def assemble(inventories, tps, upstream):
    source = {config:index_inventory(inventories[config],config) for config in ('Development','Shipping')}
    for path in FONT_PATHS:
        require(source['Development'][path] == source['Shipping'][path], 'configuration_font_metadata_mismatch')
    tps_rows = {r['path']:r for r in tps.get('sources',[])}
    require(set(tps_rows) == set(TPS_PATHS.values()), 'unexpected_tps_scope')
    require(tps_rows[TPS_PATHS['DroidSans']]['status'] == 'unresolved_redirect', 'droid_redirect_evidence_changed')
    for name in ('Roboto','Noto'):
        record = tps_rows[TPS_PATHS[name]]
        require(record['status'] == 'license_document_found' and record.get('licenseDocument') and
                record.get('declaredLocation') == '/Engine/Content/Slate/Fonts/', 'missing_declared_license_document')
    grouped = {}
    for path,row in sorted(source['Shipping'].items()):
        if row['sha256'] in grouped:
            require(grouped[row['sha256']]['metadata'] == row['metadata'] and
                    grouped[row['sha256']]['bytes'] == row['bytes'], 'duplicate_hash_metadata_mismatch')
            grouped[row['sha256']]['paths'].append(path)
        else:
            grouped[row['sha256']] = dict(sha256=row['sha256'],bytes=row['bytes'],metadata=row['metadata'],paths=[path])
    output = []
    for sha,group in sorted(grouped.items(),key=lambda pair:pair[1]['paths'][0]):
        names = {Path(p).name for p in group['paths']}
        require(len(names) == 1, 'different_faces_share_hash')
        name = next(iter(names)); metadata = group['metadata']
        candidates = []
        gaps = ['Owner applicable-agreement/compliance and final candidate notice review pending.']
        if name in OLD_ROBOTO | NEW_ROBOTO:
            candidates.append(tps_rows[TPS_PATHS['Roboto']])
            tier = 'installed_directory_declaration_not_hash_pinned'
            gaps.append('Roboto TPS directory declaration and2012 notice header are not exact2011/2017 per-face provenance.')
            if name in OLD_ROBOTO:
                require(not values(metadata,'licenseDescription') and not values(metadata,'licenseURL'), 'old_roboto_metadata_changed')
                gaps.append('Name IDs13/14 absent; exact older source/version/agreement unresolved, do not inherit another face license.')
        elif name in NOTO:
            candidates.append(tps_rows[TPS_PATHS['Noto']])
            tier = 'embedded_claim_plus_installed_directory_declaration'
            gaps.append('Installed Noto document does not pin this exact face/version/hash upstream.')
        elif name == 'DroidSansMono.ttf':
            require(upstream.get('exactPackagedSourceMatch') is True and upstream.get('sha256') == sha and
                    upstream.get('path') == 'data/fonts/DroidSansMono.ttf' and
                    upstream.get('pinnedCommit') == '54b6cfa9a9e5b861a9930af873580d6dc20f773c', 'upstream_byte_binding_mismatch')
            docs = upstream.get('documents',[])
            require({d['name'] for d in docs} == {'README.txt','NOTICE'} and
                    any(d['name'] == 'README.txt' and d.get('fontDirectoryScopeClaim') is True for d in docs), 'missing_exact_directory_declaration')
            tier = 'byte_pinned_upstream_directory_declaration'
            gaps.append('Retain embedded Ascender historical language alongside exact upstream directory proof; final applicability/notice review pending.')
        else:
            require(name == 'DroidSansFallback.ttf', 'unexpected_face')
            tier = 'embedded_claim_with_unresolved_installed_redirect'
            gaps.append('DroidSans installed redirect unresolved; no byte-pinned upstream proof for Fallback in these receipts.')
        if name.startswith('Droid'):
            candidates.append(tps_rows[TPS_PATHS['DroidSans']])
        bindings = [dict(path=p,declared_font_directory_location=p.startswith('Engine/Content/Slate/Fonts/'),
                         alias_same_bytes=len(group['paths']) > 1) for p in group['paths']]
        output.append(dict(font=name,sha256=sha,bytes=group['bytes'],paths=group['paths'],configurations=['Development','Shipping'],
                           embedded_name_records={'0':values(metadata,'copyright'),'13':values(metadata,'licenseDescription'),'14':values(metadata,'licenseURL')},
                           family=values(metadata,'family'),full_name=values(metadata,'fullName'),version=values(metadata,'version'),
                           metadata_warnings=metadata.get('warnings',[]),evidence_tier=tier,candidate_documents=candidates,
                           upstream_proof=upstream if name == 'DroidSansMono.ttf' else None,path_scope=bindings,
                           gaps=gaps,license_determination=None,rights_review='pending_owner',final_notice_approved=False))
    return dict(schema=1,scope='Existing C018H_LocalQA1 Development/Shipping font bytes only; not current release or legal approval',
                configuration_entries={'Development':15,'Shipping':15},distinct_font_hashes=14,rows=output,
                installed_tps_declaration_is_not_exact_face_match=True,raw_font_or_license_modified=False,
                license_determination=None,rights_review='pending_owner',final_notice_approved=False)


def checked_record(record, root, allowed):
    path = safe_path(root/relative_path(record['path']),allowed)
    require(path.stat().st_size == record['bytes'] and digest(path) == record['sha256'], 'evidence_file_identity_changed')
    return path


def build_map(engine):
    engine = safe_path(engine,(engine,))
    inputs = {}
    records = []
    for filename,expected in PINS.items():
        path = safe_path(INPUT_DIR/filename,(ROOT/'Artifacts',))
        require(digest(path) == expected,'pinned_font_receipt_changed')
        inputs[filename] = read_json(path)
        records.append(file_record(path,ROOT))
    tps, upstream = inputs['FontTpsSources1.json'], inputs['DroidMonoUpstream1.json']
    for record in tps['inputs']:
        checked_record(record,ROOT,(ROOT/'Artifacts',))
    inventory = {config:inputs[f'Fonts-{config}1.json'] for config in ('Development','Shipping')}
    verified_paths = set()
    for config,data in inventory.items():
        checked_record(data['pak'],ROOT,(ROOT/'Artifacts',))
        extraction = None
        for record in data['inputs']:
            path = checked_record(record,ROOT,(ROOT/'Artifacts',))
            if path.name.endswith('.exit.json'):
                extraction = read_json(path)
        require(extraction and extraction['exitCode'] == 0 and extraction['configuration'] == config and
                extraction['fontCount'] == 15 and extraction['pakSha256'] == data['pak']['sha256'], 'extraction_binding_mismatch')
        extracted = safe_path(Path(extraction['extractionRoot']),(ROOT/'Artifacts',))
        for record in data['files']:
            require(record['path'] in FONT_PATHS,'font_outside_saved_scope')
            for base in (engine.parent,extracted):
                path = checked_record(record,base,(engine,ROOT/'Artifacts'))
                verified_paths.add(str(path))
    for row in tps['sources']:
        checked_record(row,engine,(engine,))
        if row.get('licenseDocument'):
            checked_record(row['licenseDocument'],engine,(engine,))
    result = assemble(inventory,tps,upstream)
    result.update(inputs=records,checked_font_paths=len(verified_paths),network_accessed=False,
                  source_documents_read_as_hashes_only=True,tool_sha256=digest(Path(__file__)))
    return result


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('--engine',default='C:/Program Files/Epic Games/UE_5.8/Engine')
    parser.add_argument('--output',required=True)
    args = parser.parse_args()
    try:
        require(re.fullmatch(r'[A-Za-z0-9_-]+\.json',args.output),'simple_fresh_output_required')
        output = safe_path(ROOT/'Artifacts/C018H-FontNoticeMap1'/args.output,(ROOT/'Artifacts',),missing=True)
        require(not output.exists(),'output_exists')
        result = build_map(Path(args.engine).absolute())
        output.parent.mkdir(parents=True,exist_ok=True)
        with output.open('x',encoding='utf-8') as handle:
            json.dump(result,handle,ensure_ascii=False,indent=2)
        print(json.dumps(dict(rows=len(result['rows']),entries=result['configuration_entries'],checked_font_paths=result['checked_font_paths'],rights_review=result['rights_review'])))
    except (InventoryError,OSError,KeyError,TypeError,ValueError):
        print('font_notice_map_failed: inspect pinned input identities/schema; no package/source modified',file=__import__('sys').stderr)
        raise SystemExit(1)
