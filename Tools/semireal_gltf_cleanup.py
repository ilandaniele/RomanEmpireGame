"""Lossless deduplication of exact glTF texture descriptors, not image editing.

Blender exports one identical texture descriptor per material even with a shared
atlas. Unreal then creates duplicate Texture2D assets. Preserve every image and
binary/accessor byte and remap only material texture-info references.
"""
import json
import struct


def deduplicate_glb(raw):
    if len(raw) < 20 or struct.unpack_from("<III", raw) != (0x46546C67, 2, len(raw)):
        raise ValueError("Invalid GLB header/size")
    offset, chunks = 12, []
    while offset < len(raw):
        if offset + 8 > len(raw):
            raise ValueError("Truncated GLB chunk")
        length, kind = struct.unpack_from("<II", raw, offset)
        offset += 8
        if length % 4 or offset + length > len(raw):
            raise ValueError("Invalid GLB chunk length")
        chunks.append((kind, raw[offset:offset + length]))
        offset += length
    if not chunks or chunks[0][0] != 0x4E4F534A:
        raise ValueError("Missing first JSON chunk")
    document = json.loads(chunks[0][1])
    unique, indices, seen = [], {}, {}
    for old, texture in enumerate(document.get("textures", [])):
        key = json.dumps(texture, sort_keys=True, separators=(",", ":"))
        if key not in seen:
            seen[key] = len(unique)
            unique.append(texture)
        indices[old] = seen[key]

    def visit(value):
        if isinstance(value, dict):
            for key, child in value.items():
                if key.endswith("Texture") and isinstance(child, dict) and "index" in child:
                    if child["index"] not in indices:
                        raise ValueError("Material texture reference outside table")
                    child["index"] = indices[child["index"]]
                visit(child)
        elif isinstance(value, list):
            for child in value:
                visit(child)

    visit(document.get("materials", []))
    document["textures"] = unique
    encoded = json.dumps(document, separators=(",", ":"), ensure_ascii=False).encode("utf-8")
    encoded += b" " * (-len(encoded) % 4)
    chunks[0] = (chunks[0][0], encoded)
    payload = b"".join(struct.pack("<II", len(blob), kind) + blob for kind, blob in chunks)
    return struct.pack("<III", 0x46546C67, 2, len(payload) + 12) + payload
