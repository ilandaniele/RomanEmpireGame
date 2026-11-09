"""Create-only delegated Camp03 art variant; no game/config/legacy source edits."""
import argparse
import json
from pathlib import Path
import sys
import bpy

ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'Tools'))
import create_semireal_camp_v2 as base
import semireal_camp_revision3_recipe as recipe
from packaged_dependency_inventory import safe_path,digest,file_record,require

kit=base.kit


def paint(obj):
    for value in obj.data.color_attributes['SurfaceVariation'].data:
        value.color=recipe.RED
    return obj


def painted_box(name,center,size,plaster):
    return paint(kit.box(name,center,size,plaster,'SM_CampBarracks',0.))


def military_details(wood,plaster,iron):
    r=base.recipe; group='SM_CampBarracks'
    for side in (-1,1):
        x=r.CX+side*(r.WIDTH/2+.102)
        painted_box('Military oxide end-wall base',(x,r.CY,.59),(.018,r.DEPTH,.58),plaster)
        painted_box('Military oxide end-wall cornice',(x,r.CY,2.40),(.018,r.DEPTH,.12),plaster)
        vertices=[(x,r.CY-.63,2.70),(x,r.CY+.63,2.70),(x,r.CY,3.21)]
        paint(kit.mesh('Military painted gable motif',vertices,[(0,1,2) if side==1 else (2,1,0)],plaster,group))
    for left,right,bottom,top in recipe.facade_paint(r.facade_panels()):
        painted_box('Military oxide facade base',((left+right)/2,r.FRONT-.102,(bottom+top)/2),
                    (right-left,.018,top-bottom),plaster)
    painted_box('Military oxide facade cornice',(r.CX,r.FRONT-.102,2.40),(r.WIDTH,.018,.12),plaster)
    # Visible forged fasteners on existing door/shutter straps, not black bars alone.
    for opening in r.facade_openings():
        for level in (.28,.76):
            z=opening['bottom']+(opening['top']-opening['bottom'])*level
            for side in (-1,1):
                kit.box('Forged hinge rivet',(opening['x']+side*opening['width']*.36,r.FRONT-.111,z),(.025,.018,.025),iron,group,.003)
    # Rack occupies the left porch bay; center door and step remain clear in source.
    cx,cy=r.CX-2,r.FRONT-.72
    for x in (cx-.65,cx+.65):
        kit.box('Equipment rack upright',(x,cy,.90),(.075,.075,1.55),wood,group,.005)
    for z in (.60,1.38):
        kit.box('Equipment rack rail',(cx,cy,z),(1.4,.09,.085),wood,group,.005)
    for x in recipe.spear_centers(cx):
        bpy.ops.mesh.primitive_cylinder_add(vertices=8,radius=.013,depth=recipe.SPEAR_SHAFT_LENGTH,location=(x,cy-.03,recipe.SPEAR_SHAFT_CENTER))
        kit.finish(bpy.context.object,'Stored spear shaft',wood,group)
        points=[(x-.032,cy-.03,recipe.SPEAR_HEAD_BOTTOM),(x,cy-.045,recipe.SPEAR_HEAD_SHOULDER),(x+.032,cy-.03,recipe.SPEAR_HEAD_BOTTOM),
                (x,cy-.015,recipe.SPEAR_HEAD_SHOULDER),(x,cy-.03,2.16)]
        kit.mesh('Forged spear tip',points,[(0,1,4),(1,2,4),(2,3,4),(3,0,4),(0,3,2,1)],iron,group)
    for x in (cx-.32,cx+.32):
        points=recipe.shield_vertices(x,cy-.19)
        faces=[]
        faces.extend((i,i+1,i+8,i+7) for i in range(6))
        faces.extend((i+14,i+21,i+22,i+15) for i in range(6))
        for i in range(6):
            faces.extend(((i+14,i+15,i+1,i),(i+8,i+22,i+21,i+7)))
        faces.extend(((7,21,14,0),(20,27,13,6)))
        paint(kit.mesh('Stored curved red shield',points,faces,plaster,group))
        bpy.ops.mesh.primitive_uv_sphere_add(segments=12,ring_count=6,radius=.065,location=(x,cy-.285,.725))
        boss=bpy.context.object; boss.scale=(1.,.45,1.)
        kit.finish(boss,'Shield iron boss',iron,group)
        for z in (.25,1.20):
            kit.box('Shield edge binding',(x,cy-.24,z),(.52,.03,.025),iron,group,0.)


def improve_ground(soil):
    nodes,links=soil.node_tree.nodes,soil.node_tree.links
    bsdf=nodes.get('Principled BSDF')
    vector,xyz=base.position(soil)
    noise=nodes.new('ShaderNodeTexNoise'); noise.inputs['Scale'].default_value=.38
    noise.inputs['Detail'].default_value=2.; links.new(vector,noise.inputs['Vector'])
    broad=base.mix(soil,noise.outputs['Fac'],(.16,.119,.076,1.),(.235,.18,.12,1.))
    image=next(n for n in nodes if n.bl_idname=='ShaderNodeTexImage')
    earth=base.mix(soil,.07,broad,image.outputs['Color'])
    path_x=base.range_node(soil,base.math_node(soil,'ABSOLUTE',base.math_node(soil,'SUBTRACT',xyz['X'],base.recipe.CX)),.30,.86,1.,0.)
    path_y=base.range_node(soil,base.math_node(soil,'ABSOLUTE',base.math_node(soil,'ADD',xyz['Y'],3.1)),.30,.86,1.,0.)
    links.new(base.mix(soil,base.math_node(soil,'MAXIMUM',path_x,path_y),earth,(.205,.145,.089,1.)),bsdf.inputs['Base Color'])


def main():
    parser=argparse.ArgumentParser(); parser.add_argument('--output',required=True)
    args=parser.parse_args(sys.argv[sys.argv.index('--')+1:])
    owner_root=ROOT/'Artifacts/VQ02_CampRevision3'
    out=safe_path(Path(args.output).absolute(),(owner_root,),missing=True)
    require(out.parent==owner_root and out.name.startswith('Attempt') and not out.exists(),'fresh_owned_attempt_required')
    protected=[]
    for revision in ('semireal_v1','semireal_v2'):
        source=ROOT/'SourceArt/Environment/RomanCamp'/revision
        rows=[file_record(safe_path(p,(source,)),ROOT) for p in sorted(source.iterdir()) if p.is_file()]
        require(len(rows)==14,'previous_source_set_changed'); protected.extend(rows)
    out.mkdir(parents=True,exist_ok=False)
    soil_input=ROOT/'SourceArt/Environment/RomanCamp/semireal_v2/Soil-Input01.png'
    (out/'Soil-Input01.png').write_bytes(soil_input.read_bytes())
    bpy.ops.wm.read_factory_settings(use_empty=True)
    bpy.context.scene.unit_settings.system='METRIC'; bpy.context.scene.unit_settings.scale_length=1.
    kit.PARTS.clear(); kit.RNG.seed(recipe.SEED); base.RNG.seed(recipe.SEED); kit.finish=base.finish
    soil=kit.material('Soil',(.16,.11,.06),(.28,.21,.14),.94)
    image=bpy.data.images.load(str(out/'Soil-Input01.png'))
    node=soil.node_tree.nodes.new('ShaderNodeTexImage'); node.image=image
    uv=soil.node_tree.nodes.new('ShaderNodeUVMap'); uv.uv_map='GroundSource'
    soil.node_tree.links.new(uv.outputs[0],node.inputs[0])
    soil.node_tree.links.new(node.outputs[0],soil.node_tree.nodes.get('Principled BSDF').inputs['Base Color'])
    wood=kit.material('Wood',(.16,.11,.065),(.28,.19,.105),.84,(7.,7.,.65))
    plaster=kit.material('Plaster',(.52,.46,.35),(.67,.61,.50),.91,(.9,.9,.9))
    clay=kit.material('Terracotta',(.27,.075,.035),(.40,.14,.065),.83,(1.8,1.8,1.8))
    stone=kit.material('Limestone',(.31,.29,.25),(.45,.42,.36),.93)
    iron=kit.material('Iron',(.035,.032,.028),(.09,.081,.066),.63,metal=.8)
    base.improve_materials(soil,wood,plaster,clay,stone,iron); improve_ground(soil)
    base.terrain(soil); kit.wall(wood,iron); base.barrack(wood,plaster,clay,stone,iron)
    military_details(wood,plaster,iron)
    triangle_count=sum(sum(len(p.vertices)-2 for p in obj.data.polygons) for obj in kit.PARTS)
    require(triangle_count<=recipe.TRIANGLE_LIMIT,'source_triangle_budget_exceeded')
    objects,images=kit.bake(out,2048,[soil,wood,plaster,clay,stone,iron],island_margin=.0005,bake_margin=2)
    inventory=[]
    for obj in objects:
        obj['RomanCampSchema']='VQ02/3'; obj['OwnerApproved']=False
        obj.data.calc_loop_triangles()
        inventory.append(dict(name=obj.name,vertices=len(obj.data.vertices),triangles=len(obj.data.loop_triangles),dimensions_m=list(obj.dimensions),material_slots=len(obj.data.materials)))
        kit.activate([obj])
        bpy.ops.export_scene.gltf(filepath=str(out/(obj.name+'.glb')),export_format='GLB',use_selection=True,export_animations=False,export_extras=True,export_vertex_color='NONE')
    kit.activate(objects)
    bpy.ops.export_scene.gltf(filepath=str(out/'RomanCamp-Proposal03.glb'),export_format='GLB',use_selection=True,export_animations=False,export_extras=True,export_vertex_color='NONE')
    reference=kit.render(out,objects,1600)
    for image in bpy.data.images:
        if image.source=='FILE' and not image.packed_file: image.pack()
    bpy.ops.wm.save_as_mainfile(filepath=str(out/'RomanCamp-Proposal03.blend'))
    for record in protected: require(digest(ROOT/record['path'])==record['sha256'],'previous_source_changed')
    manifest=dict(schema=3,scope='Delegated private visual iteration; NOT Unreal/player/adoption approval',
                  owner_request='segui la tarea no importa: agent-selected private visual refinement',seed=recipe.SEED,
                  blender=bpy.app.version_string,meshes=inventory,atlas_size=2048,units='meters',triangle_limit=recipe.TRIANGLE_LIMIT,
                  reference_legionary=reference,uv_island_margin=.0005,bake_margin_pixels=2,
                  owner_approved=False,imported_to_unreal=False,historical_period_validated=False,
                  changes=['Military oxide base/cornice/gable paint, physically clipped around facade openings',
                           'Real porch spear/shield rack and forged hinge fasteners; center door kept clear',
                           'Reduced gravel contribution, narrower wood/clay palette; unchanged source envelope/layout'],
                  geometry_source='Original project Blender geometry; v2 construction plus v3 original details',
                  texture_source='Previous GPT soil input7percent + new procedural shaders/PBR bake; no new external acquisition',
                  protected_source_files=protected,protected_sources_unchanged=True,
                  tools=[file_record(ROOT/'Tools'/n,ROOT) for n in ('create_semireal_camp_v3.py','semireal_camp_revision3_recipe.py','create_semireal_camp_v2.py','semireal_camp_revision_recipe.py','create_semireal_camp.py')],
                  files=[file_record(p,out) for p in sorted(out.iterdir()) if p.is_file()])
    (out/'source-manifest.json').write_text(json.dumps(manifest,indent=2),encoding='utf-8')
    kit.log('REVISION03 source complete; inspect actual GLB reimports before delivery; no Unreal/mainmap changes')


if __name__=='__main__': main()
