import unittest

from verify_cooked_camp import (MATERIALS, PREFIX, cook_profile, decode_dump,
                                hard_game_closure, parse_dump, verify_shader_info)


def dump(dependencies=''):
    sections = {
        'CachedAssetsByObjectPath':'\t/Game/A.A\n',
        'CachedAssetsByPackageName':'\t/Game/A : 1 item(s)\n\t\t/Game/A.A\n',
        'CachedAssetsByClass':'\t/Script/Engine.Material : 1 item(s)\n\t\t/Game/A.A\n',
        'AssetTags':'  /Game/A.A\n    ShadingModel : MSM_DefaultLit\n',
        'CachedDependsNodes':dependencies,
    }
    return '\n'.join(f'--- Begin {key} ---\n{value}--- End {key} ---\n' for key,value in sections.items())


class CookedCampArtifacts(unittest.TestCase):
    def test_cook_profile_default_is_legacy_and_revised_is_separate(self):
        legacy=cook_profile(); revised=cook_profile('Camp02')
        self.assertEqual(legacy['camp'],'Camp01')
        self.assertEqual(legacy['prefix'],PREFIX)
        self.assertEqual(legacy['meshes']['SM_CampBarracks'],46602)
        self.assertEqual(revised['meshes']['SM_CampBarracks'],28130)
        self.assertEqual(revised['source_revision'],'semireal_v2')
        self.assertNotEqual(legacy['output_root'],revised['output_root'])

    def test_unknown_or_path_source_profile_rejected(self):
        for camp in ('Camp04','../Camp01','Camp02/Other',''):
            with self.subTest(camp=camp),self.assertRaisesRegex(ValueError,'Unsupported'):
                cook_profile(camp)

    def test_camp02_shader_mapping_cannot_inherit_camp01(self):
        revised=cook_profile('Camp02')['prefix']
        data={'AssetInfoVersion':2,'ShaderCodeToAssets':[{'ShaderMapHash':'76399F7F0EFDD3A7','Assets':sorted(PREFIX+name for name in MATERIALS)}]}
        with self.assertRaisesRegex(ValueError,'omits a camp material'):
            verify_shader_info(data,revised)
        data['ShaderCodeToAssets'][0]['Assets']=sorted(revised+name for name in MATERIALS)
        self.assertEqual(set(verify_shader_info(data,revised)),{revised+name for name in MATERIALS})

    def test_third_cook_profile_has_own_geometry_source_and_output(self):
        profiles=[cook_profile(name) for name in ('Camp01','Camp02','Camp03')]
        self.assertEqual(len({p['output_root'] for p in profiles}),3)
        third=profiles[-1]
        self.assertEqual(third['source_revision'],'semireal_v3')
        self.assertEqual(third['meshes']['SM_CampBarracks'],30554)
        self.assertEqual(third['prefix'],'/Game/ArtCandidates/Camp03/')

    def test_third_shader_evidence_cannot_be_inherited_from_second(self):
        second=cook_profile('Camp02')['prefix']; third=cook_profile('Camp03')['prefix']
        data={'AssetInfoVersion':2,'ShaderCodeToAssets':[{'ShaderMapHash':'76399F7F0EFDD3A7','Assets':sorted(second+name for name in MATERIALS)}]}
        with self.assertRaisesRegex(ValueError,'omits a camp material'):
            verify_shader_info(data,third)
        data['ShaderCodeToAssets'][0]['Assets']=sorted(third+name for name in MATERIALS)
        self.assertEqual(set(verify_shader_info(data,third)),{third+name for name in MATERIALS})

    def test_native_tables_reconcile_object_package_class_and_tags(self):
        result = parse_dump([dump()])
        self.assertEqual(result['objects'],{'/Game/A.A'})
        self.assertEqual(result['packages'],{'/Game/A'})
        self.assertEqual(result['classes']['/Game/A.A'],'/Script/Engine.Material')
        self.assertEqual(result['tags']['/Game/A.A']['ShadingModel'],'MSM_DefaultLit')

    def test_native_utf8_utf16le_utf16be_decode_strictly(self):
        text = dump()+'\né'
        for encoded in (text.encode('utf-8-sig'),text.encode('utf-16'),b'\xfe\xff'+text.encode('utf-16-be')):
            self.assertEqual(decode_dump(encoded),text)
        with self.assertRaises(UnicodeDecodeError):
            decode_dump(b'\xff')

    def test_primary_asset_id_is_a_new_dependency_root_not_package_edge(self):
        text = '\t/Game/A\n\t\tPackage\n\t\t\t/Game/B\t\t{Hard,Game,NotBuild}\n'
        text += '\tMap:/Game/Maps/L_Testbed\n\t\tPackage\n\t\t\t/Game/C\t\t{Hard,Game,NotBuild}\n'
        nodes = parse_dump([dump(text)])['dependencies']
        self.assertEqual([p for p,_ in nodes['/Game/A']],['/Game/B'])
        self.assertEqual([p for p,_ in nodes['Map:/Game/Maps/L_Testbed']],['/Game/C'])

    def test_references_are_not_forward_dependencies(self):
        text = '\t/Game/A\n\t\tReferences\n\t\t\t/Game/Referencer\n'
        self.assertEqual(parse_dump([dump(text)])['dependencies']['/Game/A'],[])

    def test_unknown_dependency_encoding_rejected(self):
        with self.assertRaisesRegex(ValueError,'Unknown package dependency'):
            parse_dump([dump('\t/Game/A\n\t\tPackage\n\t\t\t/Game/B BAD\n')])

    def test_duplicate_sections_rejected(self):
        with self.assertRaisesRegex(ValueError,'Duplicate registry dump section'):
            parse_dump([dump(),dump()])

    def test_missing_section_rejected(self):
        with self.assertRaisesRegex(ValueError,'Incomplete native dump'):
            parse_dump(['--- Begin CachedAssetsByObjectPath ---\n--- End CachedAssetsByObjectPath ---'])

    def test_duplicate_classes_and_tags_rejected(self):
        with self.assertRaisesRegex(ValueError,'Duplicate/ungrouped asset class'):
            parse_dump([dump().replace('\t\t/Game/A.A\n--- End CachedAssetsByClass','\t\t/Game/A.A\n\t\t/Game/A.A\n--- End CachedAssetsByClass')])
        with self.assertRaisesRegex(ValueError,'Duplicate asset tag'):
            parse_dump([dump().replace('    ShadingModel : MSM_DefaultLit','    ShadingModel : MSM_DefaultLit\n    ShadingModel : other')])

    def test_hard_game_closure_handles_cycles_and_excludes_editor_soft_edges(self):
        graph = {
            '/Game/A':[('/Game/B',{'Hard','Game'}),('/Editor/Only',{'Hard','EditorOnly'}),('/Soft/Only',{'Soft','Game'})],
            '/Game/B':[('/Game/A',{'Hard','Game'}),('/Script/Engine',{'Hard','Game'})],
        }
        self.assertEqual(hard_game_closure(graph,{'/Game/A'}),{'/Game/A','/Game/B','/Script/Engine'})

    def test_all_six_materials_can_share_one_real_shader_map(self):
        expected = {PREFIX+name for name in MATERIALS}
        result = verify_shader_info({'AssetInfoVersion':2,'ShaderCodeToAssets':[{'ShaderMapHash':'76399F7F0EFDD3A7','Assets':sorted(expected)}]})
        self.assertEqual(set(result),expected)

    def test_missing_material_or_unsupported_schema_rejected(self):
        with self.assertRaisesRegex(ValueError,'omits a camp material'):
            verify_shader_info({'AssetInfoVersion':2,'ShaderCodeToAssets':[]})
        with self.assertRaisesRegex(ValueError,'Unsupported'):
            verify_shader_info({'AssetInfoVersion':3,'ShaderCodeToAssets':[]})

    def test_invalid_shader_identity_rejected(self):
        with self.assertRaisesRegex(ValueError,'Invalid native shader map'):
            verify_shader_info({'AssetInfoVersion':2,'ShaderCodeToAssets':[{'ShaderMapHash':'not-a-hash','Assets':[]}]})


if __name__ == '__main__':
    unittest.main()
