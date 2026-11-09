import math
from pathlib import Path
import unittest
from roman_character_recipe import CLIPS
from semireal_character_motion import rotations, root_vertical_offset
from semireal_legionary_mesh import load_anatomy, joint_layout, torso_weights


class SemirealMotionContracts(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.layout=joint_layout(load_anatomy(Path(__file__).resolve().parents[1],"roman_adult_v1"))

    def test_block_keeps_shield_orientation_while_arm_raises(self):
        for i in range(25):
            pose=rotations("Block",i/30)
            self.assertAlmostEqual(sum(pose[n][0] for n in ("upper_arm_l","forearm_l","hand_l")),0.)
        self.assertLess(rotations("Block",.4)["upper_arm_l"][0],-10)

    def test_walk_shoes_counter_rotate_and_vertical_bone_offset_is_bounded(self):
        for i in range(31):
            time=i/30
            pose=rotations("Walk",time)
            for side in ("l","r"):
                self.assertAlmostEqual(sum(pose[f"{n}_{side}"][0] for n in ("thigh","calf","foot")),0.)
            offset=root_vertical_offset("Walk",time,self.layout)
            self.assertTrue(math.isfinite(offset))
            self.assertGreaterEqual(offset,-.15)
            self.assertLessEqual(offset,0.)
        self.assertAlmostEqual(root_vertical_offset("Walk",0,self.layout),0.)
        self.assertLess(root_vertical_offset("Walk",.25,self.layout),-.005)

    def test_other_clips_preserve_durations_and_no_visual_root_offset(self):
        for clip,duration in CLIPS.items():
            for time in (0,duration/2,duration):
                self.assertTrue(all(math.isfinite(v) for angles in rotations(clip,time).values() for v in angles))
                if clip!="Walk": self.assertEqual(root_vertical_offset(clip,time,self.layout),0.)

    def test_both_torso_layers_use_same_normalized_arm_free_weights(self):
        for i in range(150):
            z=.7+i*.006
            a=torso_weights((-.24,-.13,z),self.layout)
            b=torso_weights((-.27,-.16,z),self.layout)
            self.assertEqual(a,b)
            self.assertAlmostEqual(sum(w for _,w in a),1.)
            self.assertTrue(all(n in ("pelvis","spine","chest") and 0<=w<=1 for n,w in a))
