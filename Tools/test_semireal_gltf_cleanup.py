import json
import struct
import unittest
from semireal_gltf_cleanup import deduplicate_glb


def glb(doc, binary=b"real"):
    text = json.dumps(doc).encode("utf-8")
    text += b" " * (-len(text) % 4)
    body = struct.pack("<II", len(text), 0x4E4F534A) + text
    body += struct.pack("<II", len(binary), 0x004E4942) + binary
    return struct.pack("<III", 0x46546C67, 2, len(body) + 12) + body


class TextureDedupContracts(unittest.TestCase):
    def test_exact_duplicates_remap_without_changing_binary_or_other_indices(self):
        source = {"textures": [{"source": 0, "sampler": 0}, {"source": 1}, {"sampler": 0, "source": 0}],
                  "materials": [{"normalTexture": {"index": 2, "texCoord": 1},
                                 "pbrMetallicRoughness": {"baseColorTexture": {"index": 1}},
                                 "extras": {"index": 99}}], "images": [{"bufferView": 7}]}
        result = deduplicate_glb(glb(source))
        length = struct.unpack_from("<I", result, 12)[0]
        doc = json.loads(result[20:20 + length])
        self.assertEqual(len(doc["textures"]), 2)
        self.assertEqual(doc["materials"][0]["normalTexture"], {"index": 0, "texCoord": 1})
        self.assertEqual(doc["materials"][0]["pbrMetallicRoughness"]["baseColorTexture"]["index"], 1)
        self.assertEqual(doc["materials"][0]["extras"]["index"], 99)
        self.assertEqual(doc["images"], source["images"])
        self.assertEqual(result[20 + length:], struct.pack("<II", 4, 0x004E4942) + b"real")
        self.assertEqual(deduplicate_glb(result), result)

    def test_different_sampler_or_extensions_are_not_merged(self):
        textures = [{"source": 0}, {"source": 0, "sampler": 1},
                    {"source": 0, "extensions": {"example": {"value": 3}}}]
        result = deduplicate_glb(glb({"textures": textures}))
        length = struct.unpack_from("<I", result, 12)[0]
        self.assertEqual(json.loads(result[20:20 + length])["textures"], textures)

    def test_invalid_headers_and_missing_texture_references_reject(self):
        for raw in (b"", glb({})[:-1],
                    glb({"textures": [], "materials": [{"normalTexture": {"index": 0}}]})):
            with self.subTest(size=len(raw)), self.assertRaises(ValueError):
                deduplicate_glb(raw)


if __name__ == "__main__":
    unittest.main()
