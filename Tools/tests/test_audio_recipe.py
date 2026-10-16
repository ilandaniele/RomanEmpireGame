"""Read-only, engine-independent synthesis guards."""
import hashlib
import importlib.util
import io
from pathlib import Path
import struct
import unittest
import wave

SPEC = importlib.util.spec_from_file_location("roman_audio_recipe", Path(__file__).resolve().parents[1] / "roman_audio_recipe.py")
RECIPE = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(RECIPE)


class AudioRecipeTests(unittest.TestCase):
    def test_format_energy_envelope_and_unique_content(self):
        hashes = set()
        for name in RECIPE.RECIPES:
            with self.subTest(name=name), wave.open(io.BytesIO(RECIPE.wav(name)), "rb") as stream:
                self.assertEqual((stream.getnchannels(), stream.getsampwidth(), stream.getframerate()), (1, 2, 24000))
                self.assertGreater(stream.getnframes(), 2400)
                self.assertLess(stream.getnframes(), 48000)
                raw = stream.readframes(stream.getnframes())
                self.assertEqual(raw, RECIPE.pcm(name))
                samples = struct.unpack(f"<{len(raw)//2}h", raw)
                self.assertEqual(samples[0], 0)
                self.assertTrue(all(value == 0 for value in samples[-900:]))
                self.assertGreater(sum(value * value for value in samples) / len(samples), 100000)
                self.assertLess(max(abs(value) for value in samples), .8 * 32767)
                hashes.add(hashlib.sha256(raw).hexdigest())
        self.assertEqual(len(hashes), 6)

    def test_repeat_is_identical_and_unknown_id_rejected(self):
        for name in RECIPE.RECIPES:
            self.assertEqual(RECIPE.wav(name), RECIPE.wav(name))
            self.assertEqual(RECIPE.digest(name), hashlib.sha256(RECIPE.pcm(name)).hexdigest())
        with self.assertRaises(KeyError):
            RECIPE.wav("S_Unknown")
