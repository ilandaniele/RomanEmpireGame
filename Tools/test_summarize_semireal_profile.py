"""Synthetic CSV parser regressions; never launch UE or rewrite real receipts."""
import csv
from pathlib import Path
import tempfile
import unittest

from summarize_semireal_profile import observed_settings, read_engine_csv, stats


class NativeCSVContracts(unittest.TestCase):
    HEADERS = ['EVENTS','FrameTime','GameThreadTime','RenderThreadTime','GPUTime',
               'RomanSemireal/EngineFrame','RomanSemireal/CandidateUnits',
               'RomanSemireal/FPS','RomanEncounter/AliveUnits','RomanSemireal/GameThread/ObserverTick']

    def parse(self, rows):
        with tempfile.TemporaryDirectory() as folder:
            path=Path(folder)/'native.csv'
            with path.open('w',encoding='utf-8',newline='') as handle:
                csv.writer(handle).writerows(rows)
            return read_engine_csv(path)

    def frame(self, index=100):
        return ['',16.7,10,5,3,index,50,0,50,.15]

    def test_dynamic_columns_use_final_header_and_keep_all_frames(self):
        rows=[self.HEADERS,self.frame(),self.frame(101)+[2],
              self.HEADERS+['Late/Stat'],['[HasHeaderRowAtEnd]',1]]
        frames,column=self.parse(rows)
        self.assertEqual(len(frames),2)
        self.assertEqual([r['RomanSemireal/EngineFrame'] for r in frames],[100,101])
        self.assertEqual(column,'RomanSemireal/GameThread/ObserverTick')

    def test_reordered_final_columns_reject(self):
        final=self.HEADERS.copy(); final[1],final[2]=final[2],final[1]
        with self.assertRaisesRegex(ValueError,'reordered'):
            self.parse([self.HEADERS,self.frame(),final])

    def test_duplicate_measured_column_reject(self):
        with self.assertRaisesRegex(ValueError,'ambiguous'):
            self.parse([self.HEADERS+['FrameTime'],self.frame()+[16.7]])

    def test_nonfinite_numeric_frame_is_not_silently_skipped(self):
        row=self.frame(); row[1]='nan'
        with self.assertRaisesRegex(ValueError,'Non-finite'):
            self.parse([self.HEADERS,row])

    def test_short_or_overwide_data_not_silently_dropped(self):
        for row in (self.frame()[:-1],self.frame()+[1]):
            with self.subTest(width=len(row)),self.assertRaisesRegex(ValueError,'row width'):
                self.parse([self.HEADERS,row])

    def test_zero_gpu_is_not_gpu_measurement(self):
        row=self.frame(); row[4]=0
        with self.assertRaisesRegex(ValueError,'GPU timing'):
            self.parse([self.HEADERS,row])

    def test_lower_index_quantile_and_fps_denominator(self):
        result=stats([10,20,30,40])
        self.assertEqual(result['mean'],25)
        self.assertEqual(result['median'],20)
        self.assertEqual(result['p95'],30)

    def test_output_dimensions_alone_do_not_prove_full_raster(self):
        settings=observed_settings('view_output_width=1920\nview_output_height=1080\n')
        self.assertFalse(settings['fixed_full_scale_inferred'])

    def test_constant_full_fraction_requires_disabled_dynamic_resolution(self):
        receipt='\n'.join(f'{key}=1.000000' for key in ('primary_resolution_upper_min','primary_resolution_upper_max','secondary_fraction_min','secondary_fraction_max'))
        receipt+='\nview_output_width=1920\nview_output_height=1080\n'
        self.assertFalse(observed_settings(receipt)['fixed_full_scale_inferred'])
        self.assertTrue(observed_settings(receipt+'runtime.r.DynamicRes.OperationMode=0\n')['fixed_full_scale_inferred'])

    def test_reduced_primary_bound_not_promoted_to_full_resolution(self):
        receipt='primary_resolution_upper_min=.666667\nprimary_resolution_upper_max=.666667\nsecondary_fraction_min=1\nsecondary_fraction_max=1\nview_output_width=1920\nview_output_height=1080\nruntime.r.DynamicRes.OperationMode=0\n'
        self.assertFalse(observed_settings(receipt)['fixed_full_scale_inferred'])

    def test_battery_cap_predicate_requires_all_runtime_observations(self):
        self.assertIsNone(observed_settings('')['editor_battery_cap_source_predicate_matches'])
        receipt='runtime.engine_battery_classification=1\nruntime.r.DontLimitOnBattery=0\nruntime.t.MaxFPS=0\nruntime.smooth_frame_rate=0\n'
        self.assertTrue(observed_settings(receipt)['editor_battery_cap_source_predicate_matches'])
        self.assertFalse(observed_settings(receipt.replace('OnBattery=0','OnBattery=1'))['editor_battery_cap_source_predicate_matches'])


if __name__=='__main__':
    unittest.main()
