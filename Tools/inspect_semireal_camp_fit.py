"""Native read-only Camp02/production-contract measurement. Never saves assets."""
import argparse
import hashlib
import json
from pathlib import Path
import re
import sys

ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'Tools'))
from packaged_dependency_inventory import require, safe_path
from semireal_camp_fit import analyze_fit
import unreal

PROJECT=ROOT/'Artifacts/VQ01_Semireal1/GamePreview'
parser=argparse.ArgumentParser()
parser.add_argument('--name',required=True)
args=parser.parse_args()
require(re.fullmatch(r'[A-Za-z0-9_]{1,48}',args.name),'Invalid fit evidence name')
require(Path(unreal.Paths.project_dir()).resolve()==PROJECT,'Existing private preview only')
output=safe_path(ROOT/'Artifacts/VQ02_CampRevision2/Fit'/args.name,(ROOT/'Artifacts/VQ02_CampRevision2/Fit',))
destination=output/'readback.json'
require(not destination.exists(),'Never overwrite a fit receipt')


def xyz(value):
    return [float(value.x),float(value.y),float(value.z)]


def record(path):
    path=safe_path(path,(ROOT,))
    return dict(path=str(path.relative_to(ROOT)),bytes=path.stat().st_size,
                sha256=hashlib.sha256(path.read_bytes()).hexdigest())


result=dict(schema=1,status='running',scope='Read-only saved data/CDO/mesh AABB fit; NOT live placement/nav/spawn or adoption',
            owner_approved=False,main_game_adopted=False,player_verified=False)
try:
    definition_path='/Game/Data/Buildings/PDA_Building_Barracks'
    definition=unreal.load_asset(definition_path)
    require(isinstance(definition,unreal.RomanBuildingDefinition),'Building definition missing/wrong class')
    data=definition.get_editor_property('definition')
    require(str(data.get_editor_property('building_id'))=='Barracks','Wrong stable building identity')
    barracks_class=unreal.load_class(None,'/Script/RomanEmpireGame.Barracks')
    factory=definition.get_editor_property('building_class')
    require(factory and factory.get_path_name()==barracks_class.get_path_name(),'Actual factory is not measured native Barracks')
    cdo=unreal.get_default_object(barracks_class)
    offset=xyz(cdo.get_editor_property('spawn_offset'))
    box=cdo.get_editor_property('collision_box')
    box_extent=xyz(box.get_editor_property('box_extent'))
    unit_class=unreal.load_class(None,'/Script/RomanEmpireGame.Legionary')
    unit_cdo=unreal.get_default_object(unit_class)
    capsule=unit_cdo.get_editor_property('capsule_component')
    radius=float(capsule.get_editor_property('capsule_radius'))
    scale=xyz(data.get_editor_property('prototype_scale'))
    footprint=data.get_editor_property('footprint_size')
    footprint=[float(footprint.x),float(footprint.y)]
    mesh_path='/Game/ArtCandidates/Camp02/SM_CampBarracks'
    mesh=unreal.load_asset(mesh_path)
    require(isinstance(mesh,unreal.StaticMesh),'Saved Camp02 mesh missing')
    bounds=mesh.get_bounding_box()
    lo,hi=xyz(bounds.min),xyz(bounds.max)
    inputs=dict(bounds_min=lo,bounds_max=hi,footprint_grid=footprint,prototype_scale=scale,
                spawn_offset=offset,capsule_radius=radius,anchor_half_height=box_extent[2])
    bindings=[record(Path(__file__)),record(ROOT/'Tools/semireal_camp_fit.py')]
    relative_paths=['Source/RomanEmpireGame/Building/BuildingBase.cpp',
                    'Source/RomanEmpireGame/Building/Barracks.cpp',
                    'Source/RomanEmpireGame/Building/BuildingPlacementComponent.cpp',
                    'Source/RomanEmpireGame/Public/Building/BuildingTypes.h',
                    'Content/Data/Buildings/PDA_Building_Barracks.uasset']
    for relative in relative_paths:
        current,private=record(ROOT/relative),record(PROJECT/relative)
        require(current['sha256']==private['sha256'],'Root/private contract differs: '+relative)
        bindings.extend((current,private))
    bindings.extend((record(PROJECT/'Content/ArtCandidates/Camp02/SM_CampBarracks.uasset'),
                     record(PROJECT/'Binaries/Win64/UnrealEditor-RomanEmpireGame.dll')))
    result.update(status='pass',inputs=inputs,analysis=analyze_fit(**inputs),files=bindings,
                  definition=definition.get_path_name(),factory=factory.get_path_name(),
                  mesh=mesh.get_path_name(),unit_cdo_class=unit_class.get_path_name(),
                  default_box_extent_cm=box_extent,
                  limits='CDO capsule is not a spawned-unit measurement. AABB overlap does not prove actual wall/door contact. No map, actor, asset or config mutated/saved.')
except Exception as error:
    result.update(status='fail',error=str(error))
    raise
finally:
    with destination.open('x',encoding='utf-8') as stream:
        json.dump(result,stream,indent=2)
    unreal.log('ROMAN_CAMP_FIT '+result['status'])
