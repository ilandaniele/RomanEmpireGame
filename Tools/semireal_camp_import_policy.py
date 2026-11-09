"""Explicit, hash-pinned source selection for the private camp importer; no UE API."""
import hashlib
import json
import math
from pathlib import Path
import re

from packaged_dependency_inventory import digest, require, safe_path

MANIFEST_HASHES = {
    'semireal_v1':'0669bef5066f8204730935da2276bcf4cac08e5cc78e5d7f4d6158bdfa905912',
    'semireal_v2':'aa9364411a1de6c413041ce3075c2c0a0b3457f4d8260de43d4f0a41a9164546',
    'semireal_v3':'b1403c940209438f18a7addd6c4afbb7e2f725471f68e66a1e2c49f82f8e83e5',
}
TRIANGLES = {'semireal_v1':(1152,3704,46602),'semireal_v2':(1152,3704,28130),'semireal_v3':(1152,3704,30554)}
SCHEMAS = {'semireal_v1':1,'semireal_v2':2,'semireal_v3':3}
NAMES = ('SM_CampGround','SM_CampWall','SM_CampBarracks')


def select_revision(arguments):
    values=[value.split('=',1)[1] for value in arguments if value.startswith('--source-revision=')]
    require(len(values)<=1,'duplicate_source_revision')
    revision=values[0] if values else 'semireal_v1'
    require(revision in MANIFEST_HASHES,'unsupported_source_revision')
    return revision


def read_source_manifest(source,revision):
    require(revision in MANIFEST_HASHES,'unsupported_source_revision')
    source=Path(source).absolute()
    source=safe_path(source,(source,))
    path=safe_path(source/'source-manifest.json',(source,))
    require(path.stat().st_size<1024*1024,'source_manifest_size_limit')
    raw=path.read_bytes(); sha=hashlib.sha256(raw).hexdigest()
    require(sha==MANIFEST_HASHES[revision],'source_manifest_identity_changed')
    data=json.loads(raw.decode('utf-8'))
    require(data.get('schema')==SCHEMAS[revision] and
            data.get('owner_approved') is False and data.get('imported_to_unreal') is False,
            'source_manifest_scope_mismatch')
    meshes=data.get('meshes',[])
    require(len(meshes)==3 and {m['name'] for m in meshes}==set(NAMES),'source_mesh_set_mismatch')
    indexed={m['name']:m for m in meshes}
    for name,triangles in zip(NAMES,TRIANGLES[revision]):
        item=indexed[name]; dimensions=item.get('dimensions_m',[])
        require(item.get('triangles')==triangles and len(dimensions)==3 and
                all(isinstance(v,(int,float)) and math.isfinite(v) and v>0 for v in dimensions),
                'source_geometry_profile_mismatch')
    for name in NAMES:
        filename=name+'.glb'
        records=[item for item in data.get('files',[]) if item.get('path')==filename]
        require(len(records)==1 and re.fullmatch('[0-9a-f]{64}',records[0].get('sha256','')),
                'source_glb_identity_missing')
        item=records[0]; glb=safe_path(source/filename,(source,))
        require(glb.stat().st_size==item['bytes'] and digest(glb)==item['sha256'],'published_glb_identity_changed')
    return data,sha


def validate_namespace(revision,namespace):
    # Preserve legacy callers, but the new approved candidate has one identity.
    if revision=='semireal_v3' or namespace=='Camp03':
        require(revision=='semireal_v3' and namespace=='Camp03','source_namespace_mismatch')
