import hashlib
import json
from pathlib import Path
import sys
import tempfile
import unittest
from unittest.mock import patch

sys.path.insert(0,str(Path(__file__).resolve().parents[1]))
import semireal_camp_import_policy as policy


class CampImportPolicy(unittest.TestCase):
    def test_default_and_explicit_source(self):
        self.assertEqual(policy.select_revision([]),'semireal_v1')
        for revision in policy.MANIFEST_HASHES:
            self.assertEqual(policy.select_revision(['--source-revision='+revision]),revision)

    def test_reject_duplicate_unknown_or_path_revision(self):
        for arguments in (['--source-revision=semireal_v2']*2,['--source-revision=../semireal_v2'],
                          ['--source-revision=semireal_v4'],['--source-revision=']):
            with self.subTest(arguments=arguments),self.assertRaises(ValueError):
                policy.select_revision(arguments)

    def exercise(self,revision,mutation=None,repin=False):
        with tempfile.TemporaryDirectory() as directory:
            source=Path(directory)
            files=[]
            for name in policy.NAMES:
                content=name.encode(); (source/(name+'.glb')).write_bytes(content)
                files.append(dict(path=name+'.glb',bytes=len(content),sha256=hashlib.sha256(content).hexdigest()))
            data=dict(schema=policy.SCHEMAS[revision],owner_approved=False,imported_to_unreal=False,
                      meshes=[dict(name=name,triangles=tri,dimensions_m=[1.,2.,3.]) for name,tri in zip(policy.NAMES,policy.TRIANGLES[revision])],files=files)
            raw=json.dumps(data).encode(); pin=hashlib.sha256(raw).hexdigest()
            if mutation:
                mutation(data,source); raw=json.dumps(data).encode()
                if repin: pin=hashlib.sha256(raw).hexdigest()
            (source/'source-manifest.json').write_bytes(raw)
            with patch.dict(policy.MANIFEST_HASHES,{revision:pin}):
                return policy.read_source_manifest(source,revision)

    def test_both_profile_fixtures(self):
        for revision in policy.MANIFEST_HASHES:
            data,sha=self.exercise(revision)
            self.assertEqual(len(data['meshes']),3); self.assertEqual(len(sha),64)

    def test_changed_manifest_rejected_before_profile(self):
        with self.assertRaisesRegex(ValueError,'source_manifest_identity_changed'):
            self.exercise('semireal_v2',lambda d,p:d.update(schema=1))

    def test_wrong_geometry_cannot_inherit_other_profile(self):
        with self.assertRaisesRegex(ValueError,'source_geometry_profile_mismatch'):
            self.exercise('semireal_v2',lambda d,p:d['meshes'][-1].update(triangles=46602),repin=True)

    def test_changed_glb_rejected(self):
        with self.assertRaisesRegex(ValueError,'published_glb_identity_changed'):
            self.exercise('semireal_v2',lambda d,p:(p/'SM_CampWall.glb').write_bytes(b'changed'))

    def test_claimed_approval_not_accepted(self):
        with self.assertRaisesRegex(ValueError,'source_manifest_scope_mismatch'):
            self.exercise('semireal_v2',lambda d,p:d.update(owner_approved=True),repin=True)

    def test_duplicate_file_binding_rejected(self):
        with self.assertRaisesRegex(ValueError,'source_glb_identity_missing'):
            self.exercise('semireal_v2',lambda d,p:d['files'].append(d['files'][0].copy()),repin=True)

    def test_third_profile_cannot_inherit_v2_schema_or_geometry(self):
        with self.assertRaisesRegex(ValueError,'source_manifest_scope_mismatch'):
            self.exercise('semireal_v3',lambda d,p:d.update(schema=2),repin=True)
        with self.assertRaisesRegex(ValueError,'source_geometry_profile_mismatch'):
            self.exercise('semireal_v3',lambda d,p:d['meshes'][-1].update(triangles=28130),repin=True)

    def test_third_namespace_cannot_mix_revisions(self):
        policy.validate_namespace('semireal_v3','Camp03')
        policy.validate_namespace('semireal_v1','Camp01')
        policy.validate_namespace('semireal_v2','Camp02')
        for revision,namespace in (('semireal_v2','Camp03'),('semireal_v1','Camp03'),
                                   ('semireal_v3','Camp02'),('semireal_v3','Camp003')):
            with self.subTest(revision=revision,namespace=namespace),self.assertRaisesRegex(ValueError,'source_namespace_mismatch'):
                policy.validate_namespace(revision,namespace)


if __name__=='__main__': unittest.main()
