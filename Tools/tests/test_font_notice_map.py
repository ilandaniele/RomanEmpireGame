import copy
import hashlib
import sys
from pathlib import Path
import unittest

sys.path.insert(0,str(Path(__file__).resolve().parents[1]))
from font_notice_map import FONT_PATHS, OLD_ROBOTO, TPS_PATHS, assemble


def fixture():
    rows = []
    def record(value):
        return {'value':value,'platform':3,'encoding':1,'language':1033}
    for path in sorted(FONT_PATHS):
        name = Path(path).name
        fields = {'copyright':[record('Exact test copyright')], 'version':[record('Exact test version')],
                  'family':[record('Metadata family only')], 'fullName':[record(name)]}
        if name not in OLD_ROBOTO:
            fields['licenseDescription'] = [record('Embedded claim; not rights approval')]
            fields['licenseURL'] = [record('https://example.invalid/metadata-only')]
        sha = hashlib.sha256(name.encode()).hexdigest()
        rows.append(dict(path=path,bytes=1000,sha256=sha,installedSourceSha256=sha,
                         metadata=dict(fields=fields,warnings=[],licenseMetadata='name_ids_13_14_absent' if name in OLD_ROBOTO else 'name_ids_13_14_present_unapproved'),
                         rightsReview='pending_owner'))
    inventory = {config:dict(schema=1,configuration=config,rightsReview='pending_owner',files=copy.deepcopy(rows)) for config in ('Development','Shipping')}
    tps = dict(sources=[dict(path=path,status='unresolved_redirect' if name == 'DroidSans' else 'license_document_found',
                            declaredLocation=None if name == 'DroidSans' else '/Engine/Content/Slate/Fonts/',
                            licenseDocument=None if name == 'DroidSans' else dict(path=name+'_License.txt')) for name,path in TPS_PATHS.items()])
    mono = next(row for row in rows if row['path'].endswith('/DroidSansMono.ttf'))
    upstream = dict(sha256=mono['sha256'],exactPackagedSourceMatch=True,path='data/fonts/DroidSansMono.ttf',
                    pinnedCommit='54b6cfa9a9e5b861a9930af873580d6dc20f773c',
                    documents=[dict(name='README.txt',fontDirectoryScopeClaim=True),dict(name='NOTICE')])
    return inventory,tps,upstream


class FontNoticeMapping(unittest.TestCase):
    def test_exact_set_deduplicates_only_bold_alias_and_keeps_pending(self):
        result = assemble(*fixture())
        self.assertEqual(len(result['rows']),14)
        bold = next(row for row in result['rows'] if row['font'] == 'Roboto-Bold.ttf')
        self.assertEqual(len(bold['paths']),2)
        self.assertEqual([p['declared_font_directory_location'] for p in bold['path_scope']],[True,False])
        self.assertIsNone(result['license_determination'])
        self.assertFalse(result['final_notice_approved'])
        self.assertTrue(all(row['rights_review'] == 'pending_owner' for row in result['rows']))

    def test_different_configuration_bytes_rejected(self):
        inv,tps,up = fixture()
        inv['Development']['files'][0]['sha256'] = 'f'*64
        inv['Development']['files'][0]['installedSourceSha256'] = 'f'*64
        with self.assertRaisesRegex(ValueError,'configuration_font_metadata_mismatch'):
            assemble(inv,tps,up)

    def test_different_configuration_metadata_rejected(self):
        inv,tps,up = fixture()
        inv['Shipping']['files'][0]['metadata']['fields']['copyright'][0]['value'] = 'Changed'
        with self.assertRaisesRegex(ValueError,'configuration_font_metadata_mismatch'):
            assemble(inv,tps,up)

    def test_duplicate_or_unknown_path_rejected(self):
        inv,tps,up = fixture()
        inv['Shipping']['files'][-1]['path'] = inv['Shipping']['files'][0]['path']
        with self.assertRaisesRegex(ValueError,'invalid_font_identity'):
            assemble(inv,tps,up)
        inv,tps,up = fixture()
        inv['Shipping']['files'][0]['path'] = 'Engine/Other/Unknown.ttf'
        with self.assertRaisesRegex(ValueError,'invalid_font_identity'):
            assemble(inv,tps,up)

    def test_installed_source_must_match_raw_font_hash(self):
        inv,tps,up = fixture()
        inv['Shipping']['files'][0]['installedSourceSha256'] = '0'*64
        with self.assertRaisesRegex(ValueError,'font_source_identity_mismatch'):
            assemble(inv,tps,up)

    def test_cannot_backfill_old_roboto_license_from_a_sibling(self):
        inv,tps,up = fixture()
        for data in inv.values():
            row = next(f for f in data['files'] if f['path'].endswith('/Roboto-Regular.ttf'))
            row['metadata']['fields']['licenseDescription'] = [dict(value='Inherited claim',platform=3,encoding=1,language=1033)]
            row['metadata']['licenseMetadata'] = 'name_ids_13_14_present_unapproved'
        with self.assertRaisesRegex(ValueError,'old_roboto_metadata_changed'):
            assemble(inv,tps,up)

    def test_alias_metadata_cannot_disagree_for_identical_hash(self):
        inv,tps,up = fixture()
        for data in inv.values():
            row = next(f for f in data['files'] if '/Old/Tiles/' in f['path'])
            row['metadata']['fields']['copyright'][0]['value'] = 'Different'
        with self.assertRaisesRegex(ValueError,'duplicate_hash_metadata_mismatch'):
            assemble(inv,tps,up)

    def test_droid_mono_receipt_must_bind_exact_bytes(self):
        inv,tps,up = fixture()
        up['sha256'] = '0'*64
        with self.assertRaisesRegex(ValueError,'upstream_byte_binding_mismatch'):
            assemble(inv,tps,up)

    def test_droid_proof_not_extended_to_fallback(self):
        rows = assemble(*fixture())['rows']
        mono = next(row for row in rows if row['font'] == 'DroidSansMono.ttf')
        fallback = next(row for row in rows if row['font'] == 'DroidSansFallback.ttf')
        self.assertIsNotNone(mono['upstream_proof'])
        self.assertIsNone(fallback['upstream_proof'])
        self.assertNotEqual(mono['evidence_tier'],fallback['evidence_tier'])

    def test_missing_directory_scope_or_license_document_rejected(self):
        inv,tps,up = fixture()
        up['documents'][0]['fontDirectoryScopeClaim'] = False
        with self.assertRaisesRegex(ValueError,'missing_exact_directory_declaration'):
            assemble(inv,tps,up)
        inv,tps,up = fixture()
        next(r for r in tps['sources'] if 'Noto.tps' in r['path'])['licenseDocument'] = None
        with self.assertRaisesRegex(ValueError,'missing_declared_license_document'):
            assemble(inv,tps,up)

    def test_missing_or_invalid_embedded_records_rejected(self):
        inv,tps,up = fixture()
        inv['Shipping']['files'][0]['metadata']['fields']['copyright'] = []
        with self.assertRaisesRegex(ValueError,'missing_copyright_or_version_metadata'):
            assemble(inv,tps,up)
        inv,tps,up = fixture()
        inv['Shipping']['files'][0]['metadata']['fields']['version'][0]['language'] = '1033'
        with self.assertRaisesRegex(ValueError,'invalid_saved_name_record'):
            assemble(inv,tps,up)

    def test_metadata_uri_preserved_as_data_without_license_decision(self):
        inv,tps,up = fixture()
        for data in inv.values():
            data['files'][0]['metadata']['fields']['licenseURL'][0]['value'] = 'file:///do-not-follow'
        result = assemble(inv,tps,up)
        first = next(row for row in result['rows'] if row['paths'][0] == inv['Shipping']['files'][0]['path'])
        self.assertEqual(first['embedded_name_records']['14'][0]['value'],'file:///do-not-follow')
        self.assertIsNone(first['license_determination'])


if __name__ == '__main__':
    unittest.main()
