"""Independent Blender reimport of all actual VQ02 revision GLBs; read-only inputs."""
import argparse
import json
import math
from pathlib import Path
import struct
import sys

import bpy
from mathutils import Vector

ROOT=Path(__file__).resolve().parent.parent
sys.path.insert(0,str(ROOT/'Tools'))
from packaged_dependency_inventory import safe_path,require,digest,file_record,read_json
import create_semireal_camp as kit
import semireal_camp_revision_recipe as recipe


def inspect_glb(path,expected_count):
    require(20<path.stat().st_size<32*1024*1024,'bounded_glb_required')
    raw=path.read_bytes()
    require(struct.unpack_from('<III',raw)==(0x46546c67,2,len(raw)),'invalid_glb_header')
    json_size,json_type=struct.unpack_from('<II',raw,12)
    require(json_type==0x4e4f534a and json_size<4*1024*1024,'invalid_glb_json')
    data=json.loads(raw[20:20+json_size])
    offset=20+json_size
    bin_size,bin_type=struct.unpack_from('<II',raw,offset)
    require(bin_type==0x004e4942 and offset+8+bin_size==len(raw),'invalid_glb_binary_chunk')
    binary=raw[offset+8:]
    require(not data.get('animations') and not data.get('skins'),'unexpected_rig_or_animation')
    require(len(data['meshes'])==expected_count,'unexpected_export_mesh_count')
    require(len(data.get('buffers',[]))==1 and not data['buffers'][0].get('uri') and
            data['buffers'][0]['byteLength']<=len(binary),'external_or_invalid_buffer')
    for view in data['bufferViews']:
        start,size=view.get('byteOffset',0),view['byteLength']
        require(view.get('buffer',0)==0 and 0<=start<=start+size<=len(binary),'invalid_buffer_view')
    images=[]
    for item in data.get('images',[]):
        require(not item.get('uri') and item.get('mimeType')=='image/png','external_or_non_png_image')
        view=data['bufferViews'][item['bufferView']]; start=view.get('byteOffset',0)
        header=binary[start:start+24]
        require(header[:8]==b'\x89PNG\r\n\x1a\n' and struct.unpack_from('>II',header,16)==(2048,2048),'wrong_embedded_texture_size')
        images.append(dict(name=item.get('name'),width=2048,height=2048))
    require(len(images)==3,'unexpected_pbr_image_count')
    for mesh in data['meshes']:
        for primitive in mesh['primitives']:
            require(primitive.get('mode',4)==4 and 'COLOR_0' not in primitive['attributes'],'unexpected_topology_or_double_tint')
            require('TEXCOORD_0' in primitive['attributes'] and 'TEXCOORD_1' not in primitive['attributes'],'wrong_atlas_uv_scope')
    return dict(path=path.name,sha256=digest(path),embedded_images=images,materials=len(data.get('materials',[])))


def main():
    parser=argparse.ArgumentParser(); parser.add_argument('--source',required=True); parser.add_argument('--output',required=True)
    parser.add_argument('--revision',type=int,choices=(2,3),default=2)
    args=parser.parse_args(sys.argv[sys.argv.index('--')+1:])
    root=ROOT/f'Artifacts/VQ02_CampRevision{args.revision}'
    source=safe_path(Path(args.source).absolute(),(root,))
    out=safe_path(Path(args.output).absolute(),(root,),missing=True)
    require(source.parent==root and source.name.startswith('Attempt') and out.parent==root and not out.exists(),'fresh_readback_required')
    manifest_path=source/'source-manifest.json'; manifest=read_json(manifest_path)
    require(manifest['schema']==args.revision and manifest['owner_approved'] is False and manifest['imported_to_unreal'] is False,'wrong_source_scope')
    for record in manifest['files']:
        path=safe_path(source/record['path'],(source,))
        require(path.stat().st_size==record['bytes'] and digest(path)==record['sha256'],'source_artifact_changed')
    for record in manifest['tools']+manifest['protected_source_files']:
        path=safe_path(ROOT/record['path'],(ROOT/'Tools',ROOT/'SourceArt'))
        require(digest(path)==record['sha256'],'protected_source_or_tool_changed')
    source_manifest_sha=digest(manifest_path)
    out.mkdir()
    # A valid-looking export can still contain unbaked sub-texel roof faces.
    # Check actual atlas coverage in the saved blend before inspecting GLBs.
    bpy.ops.wm.open_mainfile(filepath=str(source/f'RomanCamp-Proposal0{args.revision}.blend'),use_scripts=False)
    obj=bpy.data.objects['SM_CampBarracks']; uv=obj.data.uv_layers.active
    image=bpy.data.images['Camp_Roughness']; pixels=list(image.pixels[:])
    clay_samples=[]
    for polygon in obj.data.polygons:
        if obj.data.materials[polygon.material_index].name!='Terracotta' or polygon.area<.006:
            continue
        coords=[uv.data[i].uv for i in polygon.loop_indices]
        u=sum(v.x for v in coords)/len(coords); v=sum(v.y for v in coords)/len(coords)
        require(0<=u<1 and 0<=v<1,'atlas_uv_out_of_bounds')
        value=pixels[(int(v*2048)*2048+int(u*2048))*4]
        clay_samples.append(value)
    require(len(clay_samples)>4000 and min(clay_samples)>.80 and max(clay_samples)<.85,'roof_atlas_has_unbaked_faces')
    coverage=dict(samples=len(clay_samples),minimum=min(clay_samples),maximum=max(clay_samples),scope='Clay faces area>=.006m2 only; not a complete texel proof')
    del pixels
    expected={m['name']:m for m in manifest['meshes']}
    require(set(expected)=={'SM_CampGround','SM_CampWall','SM_CampBarracks'} and sum(m['triangles'] for m in expected.values())<=recipe.TRIANGLE_LIMIT,'source_budget_or_mesh_set_invalid')
    reports=[]
    for name in (*expected,f'RomanCamp-Proposal0{args.revision}'):
        path=source/(name+'.glb')
        result=inspect_glb(path,3 if name.startswith('RomanCamp') else 1)
        bpy.ops.wm.read_factory_settings(use_empty=True)
        bpy.ops.import_scene.gltf(filepath=str(path))
        bpy.context.view_layer.update()
        meshes=[o for o in bpy.context.scene.objects if o.type=='MESH']
        require(not any(o.type=='ARMATURE' for o in bpy.context.scene.objects),'reference_character_leaked_to_export')
        require({o.name for o in meshes}==(set(expected) if name.startswith('RomanCamp') else {name}),'wrong_imported_mesh_names')
        imported=[]
        for obj in meshes:
            obj.data.calc_loop_triangles(); reference=expected[obj.name]
            require(len(obj.data.loop_triangles)==reference['triangles'],'triangle_count_changed_on_export')
            require(len(obj.data.uv_layers)==1,'wrong_imported_uv_count')
            require(all(math.isfinite(v) for vertex in obj.data.vertices for v in vertex.co),'non_finite_vertex')
            require(all(abs(a-b)<.002 for a,b in zip(obj.dimensions,reference['dimensions_m'])),'metre_scale_or_axis_mismatch')
            require(obj.get('OwnerApproved') is False and obj.get('RomanCampSchema')==f'VQ02/{args.revision}','wrong_exported_scope_metadata')
            imported.append(dict(name=obj.name,triangles=len(obj.data.loop_triangles),dimensions_m=list(obj.dimensions),used_materials=len({p.material_index for p in obj.data.polygons})))
        result['actual_reimport']=imported; reports.append(result)
    # Inspect the exported glTF/PBR result rather than rendering the author scene again.
    render_reference=kit.render(out,meshes,1600)
    require(digest(manifest_path)==source_manifest_sha,'source_manifest_changed')
    for record in manifest['files']:
        require(digest(source/record['path'])==record['sha256'],'source_changed_during_readback')
    for record in manifest['protected_source_files']:
        require(digest(ROOT/record['path'])==record['sha256'],'v1_changed_during_readback')
    result=dict(schema=1,scope='Actual Blender GLB reimport/source render; NOT Unreal/player approval',source_manifest_sha256=source_manifest_sha,
                source=str(source.relative_to(ROOT)),glbs=reports,render_reference=render_reference,
                protected_sources_unchanged=True,source_files_unchanged=True,owner_approved=False,
                roof_bake_coverage=coverage,
                tool_sha256=digest(Path(__file__)),renders=[file_record(p,out) for p in sorted(out.glob('*.png'))])
    (out/'readback.json').write_text(json.dumps(result,indent=2),encoding='utf-8')
    print('VQ02 actual4GLB reimport PASS; source/v1 unchanged; owner/UE gates OPEN',flush=True)


if __name__=='__main__':
    main()
