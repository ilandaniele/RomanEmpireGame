"""Independent Blender GLB reimport/embedded-PBR guard, not Unreal approval."""
import argparse
import hashlib
import json
import math
from pathlib import Path
import struct
import sys

import bpy
from mathutils import Vector

ROOT = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT/'Tools'))
from create_semireal_camp import render


def require(condition, message):
    if not condition:
        raise RuntimeError(message)


def embedded_pbr(path):
    raw = path.read_bytes()
    require(len(raw) >= 20 and struct.unpack_from('<III',raw) == (0x46546C67,2,len(raw)), 'Bad GLB header')
    chunks, offset = {}, 12
    while offset < len(raw):
        require(offset+8 <= len(raw), 'Truncated chunk header')
        size, kind = struct.unpack_from('<II',raw,offset)
        offset += 8
        require(size % 4 == 0 and offset+size <= len(raw) and kind not in chunks, 'Bad chunk bounds/duplicate')
        chunks[kind] = raw[offset:offset+size]
        offset += size
    doc = json.loads(chunks[0x4E4F534A])
    blob = chunks[0x004E4942]
    require(not doc.get('skins') and not doc.get('animations'), 'Camp export includes character/animation')
    require(len(doc['buffers']) == 1 and 'uri' not in doc['buffers'][0], 'External buffer')
    sizes = []
    for image in doc['images']:
        require('uri' not in image and image['mimeType'] == 'image/png', 'External/non-PNG image')
        view = doc['bufferViews'][image['bufferView']]
        base, size = view.get('byteOffset',0), view['byteLength']
        require(base >= 0 and base+size <= len(blob), 'Image outside embedded binary')
        png = blob[base:base+size]
        require(png[:8] == b'\x89PNG\r\n\x1a\n' and png[12:16] == b'IHDR', 'Not actual PNG')
        dimensions = struct.unpack_from('>II',png,16)
        require(dimensions == (2048,2048), 'Unexpected actual embedded texture size')
        sizes.append(dimensions)
    for mat in doc['materials']:
        pbr = mat['pbrMetallicRoughness']
        for info in (pbr['baseColorTexture'],pbr['metallicRoughnessTexture'],mat['normalTexture']):
            require(info.get('texCoord',0) == 0, 'Material references absent atlas UV')
            texture = doc['textures'][info['index']]
            require(0 <= texture['source'] < len(sizes), 'Invalid image reference')
    return dict(images=len(sizes), materials=len(doc['materials']),
                actual_png_sizes=sizes, glb_sha256=hashlib.sha256(raw).hexdigest())


def inspect_meshes(expected):
    meshes = [o for o in bpy.context.scene.objects if o.type == 'MESH']
    require(len(meshes) == len(expected), 'Unexpected imported mesh count')
    require(not any(o.type == 'ARMATURE' for o in bpy.context.scene.objects), 'Legionary leaked into camp GLB')
    results = []
    for obj in meshes:
        require(obj.name in expected, 'Unexpected real imported object '+obj.name)
        source = expected[obj.name]
        obj.data.calc_loop_triangles()
        require(len(obj.data.loop_triangles) == source['triangles'], 'Actual imported triangle mismatch')
        require(len(obj.data.uv_layers) == 1, 'Exactly one actual atlas UV required')
        for row in obj.data.uv_layers[0].data:
            require(all(math.isfinite(v) and -.00001 <= v <= 1.00001 for v in row.uv), 'Invalid atlas coordinate')
        for vertex in obj.data.vertices:
            require(all(math.isfinite(v) for v in vertex.co), 'Nonfinite actual position')
        points = [obj.matrix_world@Vector(p) for p in obj.bound_box]
        dimensions = [max(p[i] for p in points)-min(p[i] for p in points) for i in range(3)]
        require(all(abs(a-b) <= .0001 for a,b in zip(dimensions,source['dimensions_m'])), 'Exported units/bounds mismatch')
        used_slots = {p.material_index for p in obj.data.polygons}
        require(all(obj.material_slots[i].material and obj.material_slots[i].material.use_nodes for i in used_slots), 'Missing imported material')
        results.append(dict(name=obj.name,triangles=len(obj.data.loop_triangles),
                            imported_vertices=len(obj.data.vertices),dimensions_m=dimensions,
                            used_material_slots=len(used_slots)))
    return meshes, results


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--source', required=True)
    parser.add_argument('--report-dir', required=True)
    args = parser.parse_args(sys.argv[sys.argv.index('--')+1:])
    source, out = Path(args.source).resolve(), Path(args.report_dir).resolve()
    source.relative_to(ROOT/'Artifacts/VQ02_CampSource1')
    out.relative_to(ROOT/'Artifacts/VQ02_CampSource1')
    require(not out.exists(), 'Create-only fresh report directory required')
    manifest = json.loads((source/'source-manifest.json').read_text(encoding='utf-8'))
    for item in manifest['files']:
        path = source/item['path']
        require(path.parent == source and hashlib.sha256(path.read_bytes()).hexdigest() == item['sha256'], 'Source manifest mismatch')
    expected = {item['name']:item for item in manifest['meshes']}
    require(set(expected) == {'SM_CampGround','SM_CampWall','SM_CampBarracks'}, 'Wrong sample scope')
    require(sum(item['triangles'] for item in expected.values()) <= 65000, 'Triangle budget exceeded')
    # Exporter's installed texture.py warns whenever >1 socket participates in
    # packed roughness/metallic, even if the image-node samplers are identical.
    # Verify the saved source graphs rather than suppressing that warning.
    bpy.ops.wm.open_mainfile(filepath=str(source/'RomanCamp-Proposal01.blend'))
    samplers = {}
    for name in ('Soil','Wood','Plaster','Terracotta','Limestone','Iron'):
        mat = bpy.data.materials.get(name)
        require(mat is not None, 'Missing canonical saved source material')
        nodes = [n for n in mat.node_tree.nodes if n.type == 'TEX_IMAGE']
        require(len(nodes) == 4, 'Canonical PBR graph has missing/duplicate texture nodes')
        policies = {(n.extension,n.interpolation) for n in nodes}
        require(policies == {('REPEAT','Linear')}, 'Ambiguous source texture sampler policy')
        samplers[name] = dict(image_nodes=4,extension='REPEAT',interpolation='Linear')
    results = []
    for name in (*expected, 'RomanCamp-Proposal01'):
        path = source/(name+'.glb')
        pbr = embedded_pbr(path)
        bpy.ops.wm.read_factory_settings(use_empty=True)
        bpy.ops.import_scene.gltf(filepath=str(path))
        objects, actual = inspect_meshes(expected if name == 'RomanCamp-Proposal01' else {name:expected[name]})
        results.append(dict(file=path.name, **pbr, meshes=actual))
    out.mkdir(parents=True)
    reference = render(out, objects, 1600)
    result = dict(schema=1,scope='Independent exported GLB source readback/render, NOT Unreal/player acceptance',
                  source=str(source.relative_to(ROOT)),exports=results,reference_legionary=reference,
                  saved_source_pbr_sampler_policy=samplers,
                  actual_uv_pbr_units_verified=True,owner_approved=False,imported_to_unreal=False)
    (out/'readback.json').write_text(json.dumps(result,indent=2),encoding='utf-8')
    print('VQ02 independent GLB/PBR/bounds readback PASS; source-only renders saved',flush=True)


if __name__ == '__main__':
    main()
