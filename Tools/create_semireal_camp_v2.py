"""Create-only owner-requested architecture AND material revision, Blender source only.

Reuses the original tested atlas/export/render pipeline; never edits v1 or Unreal.
No image-generation endpoint, network access, downloaded geometry, or installation.
"""
import argparse
import hashlib
import json
import math
from pathlib import Path
import random
import sys

import bpy
from mathutils import Vector

ROOT = Path(__file__).resolve().parent.parent
sys.path.insert(0,str(ROOT/'Tools'))
import create_semireal_camp as kit
import semireal_camp_revision_recipe as recipe
from packaged_dependency_inventory import safe_path, digest, file_record, require

RNG = random.Random(recipe.SEED)
ORIGINAL_FINISH = kit.finish


def finish(obj,name,mat,group,bevel=0.):
    obj = ORIGINAL_FINISH(obj,name,mat,group,bevel)
    layer = obj.data.color_attributes.new(name='SurfaceVariation',type='FLOAT_COLOR',domain='CORNER')
    factor = RNG.uniform(.89,1.08) if mat.name in ('Wood','Terracotta','Limestone') else 1.
    for value in layer.data:
        value.color = (factor,factor,factor,1.)
    return obj


def math_node(mat,operation,a,b=0.):
    node = mat.node_tree.nodes.new('ShaderNodeMath'); node.operation = operation
    for index,value in enumerate((a,b)):
        if isinstance(value,(int,float)):
            node.inputs[index].default_value = value
        else:
            mat.node_tree.links.new(value,node.inputs[index])
    return node.outputs[0]


def mix(mat,factor,a,b,blend='MIX'):
    node = mat.node_tree.nodes.new('ShaderNodeMixRGB'); node.blend_type = blend
    for index,value in enumerate((factor,a,b)):
        if isinstance(value,(int,float)):
            node.inputs[index].default_value = value
        elif isinstance(value,tuple):
            node.inputs[index].default_value = value
        else:
            mat.node_tree.links.new(value,node.inputs[index])
    return node.outputs[0]


def range_node(mat,value,start,end,low,high):
    node = mat.node_tree.nodes.new('ShaderNodeMapRange')
    node.interpolation_type = 'SMOOTHSTEP'; node.clamp = True
    mat.node_tree.links.new(value,node.inputs['Value'])
    for key,v in (('From Min',start),('From Max',end),('To Min',low),('To Max',high)):
        node.inputs[key].default_value = v
    return node.outputs[0]


def position(mat):
    node = mat.node_tree.nodes.new('ShaderNodeNewGeometry')
    separate = mat.node_tree.nodes.new('ShaderNodeSeparateXYZ')
    mat.node_tree.links.new(node.outputs['Position'],separate.inputs[0])
    return node.outputs['Position'],separate.outputs


def improve_materials(soil,wood,plaster,clay,stone,iron):
    for mat in (wood,plaster,clay,stone):
        bsdf = mat.node_tree.nodes.get('Principled BSDF')
        color = bsdf.inputs['Base Color'].links[0].from_socket
        attribute = mat.node_tree.nodes.new('ShaderNodeAttribute')
        attribute.attribute_name = 'SurfaceVariation'
        mat.node_tree.links.new(mix(mat,1.,color,attribute.outputs['Color'],'MULTIPLY'),bsdf.inputs['Base Color'])
    # Earth keeps the original generated diffuse as a subdued fine-scale ingredient.
    # Macro variation and path masks are native procedural shaders, baked to PBR.
    nodes,links = soil.node_tree.nodes,soil.node_tree.links
    bsdf = nodes.get('Principled BSDF')
    detail = bsdf.inputs['Base Color'].links[0].from_socket
    vector,xyz = position(soil)
    noise = nodes.new('ShaderNodeTexNoise'); noise.inputs['Scale'].default_value = .52
    noise.inputs['Detail'].default_value = 2.
    links.new(vector,noise.inputs['Vector'])
    broad = mix(soil,noise.outputs['Fac'],(.16,.112,.067,1.),(.31,.235,.145,1.))
    earth = mix(soil,.32,broad,detail)
    path_x = range_node(soil,math_node(soil,'ABSOLUTE',math_node(soil,'SUBTRACT',xyz['X'],recipe.CX)),.38,.86,1.,0.)
    path_y = range_node(soil,math_node(soil,'ABSOLUTE',math_node(soil,'ADD',xyz['Y'],3.1)),.38,.86,1.,0.)
    path = math_node(soil,'MAXIMUM',path_x,path_y)
    links.new(mix(soil,path,earth,(.245,.175,.103,1.)),bsdf.inputs['Base Color'])
    links.new(range_node(soil,path,0.,1.,.94,.85),bsdf.inputs['Roughness'])
    # Localized damp/dust band, not a uniform dark outline or baked lighting.
    vector,xyz = position(plaster)
    dirt = range_node(plaster,xyz['Z'],.25,.95,.50,0.)
    bsdf = plaster.node_tree.nodes.get('Principled BSDF')
    color = bsdf.inputs['Base Color'].links[0].from_socket
    plaster.node_tree.links.new(mix(plaster,dirt,color,(.27,.225,.16,1.)),bsdf.inputs['Base Color'])


def terrain(soil):
    count=24
    vertices=[(-6+x*.5,-5+y*10/count,recipe.ground_height(-6+x*.5,-5+y*10/count))
              for y in range(count+1) for x in range(count+1)]
    faces=[(y*25+x,y*25+x+1,(y+1)*25+x+1,(y+1)*25+x) for y in range(count) for x in range(count)]
    obj=kit.mesh('Camp earth with broad wear tracks',vertices,faces,soil,'SM_CampGround')
    uv=obj.data.uv_layers.new(name='GroundSource')
    for loop in obj.data.loops:
        p=obj.data.vertices[loop.vertex_index].co
        uv.data[loop.index].uv=((p.x+6)/12,(p.y+5)/10)


def beam(name,a,b,width,mat,group):
    a,b=Vector(a),Vector(b)
    obj=kit.box(name,(a+b)/2,(width,width,(b-a).length),mat,group,.008)
    obj.rotation_euler=(b-a).to_track_quat('Z','Y').to_euler()
    return obj


def roof_pan(x,y,z,slope,clay,group):
    tile=kit.box('Overlapping terracotta pan',(x,y,z),(.43,.49,.038),clay,group,0.)
    tile.rotation_euler.x=math.atan(slope)
    # Real raised edges and curved covers; no fake highlights painted into albedo.
    for sign in (-1,1):
        rim=kit.box('Terracotta raised edge',(x+sign*.20,y,z+.032),(.027,.49,.059),clay,group,0.)
        rim.rotation_euler.x=math.atan(slope)
    points=[(x-.22+.061*math.cos(i*math.pi/8),y+end,z+slope*end+.044+.061*math.sin(i*math.pi/8))
            for end in (-.265,.265) for i in range(9)]
    kit.mesh('Curved tile seam cover',points,[(i,i+9,i+10,i+1) for i in range(8)],clay,group)


def barrack(wood,plaster,clay,stone,iron):
    cx,cy,w,d=recipe.CX,recipe.CY,recipe.WIDTH,recipe.DEPTH
    front,back=recipe.FRONT,recipe.BACK
    group='SM_CampBarracks'
    for x in (cx-w/2,cx+w/2):
        kit.box('End limewash wall',(x,cy,1.415),(.18,d,2.37),plaster,group)
        points=[(x,front,recipe.EAVE),(x,back,recipe.EAVE),(x,cy,recipe.RIDGE)]
        kit.mesh('End gable',points,[(0,1,2) if x>cx else (2,1,0)],plaster,group)
    kit.box('Rear wall',(cx,back,1.415),(w,.18,2.37),plaster,group)
    for left,right,bottom,top in recipe.facade_panels():
        kit.box('Facade opening infill',((left+right)/2,front,(bottom+top)/2),
                (right-left,.18,top-bottom),plaster,group,.006)
    for opening in recipe.facade_openings():
        x,width,bottom,top=opening['x'],opening['width'],opening['bottom'],opening['top']
        for sign in (-1,1):
            kit.box('Opening jamb',(x+sign*(width/2+.045),front-.13,(bottom+top)/2),
                    (.11,.15,top-bottom+.12),wood,group,.007)
        kit.box('Opening lintel',(x,front-.13,top+.045),(width+.22,.18,.13),wood,group,.008)
        if 'shutter' in opening['name']:
            kit.box('Window stone sill',(x,front-.15,bottom-.045),(width+.24,.30,.11),stone,group,.008)
        for plank in range(8):
            kit.box(opening['name']+' plank',(x-width/2+(plank+.5)*width/8,front-.045,(bottom+top)/2),
                    (width/8-.009,.08,top-bottom-.025),wood,group,.003)
        for level in (.28,.76):
            kit.box('Opening iron strap',(x,front-.092,bottom+(top-bottom)*level),
                    (width*.87,.02,.036),iron,group,.003)
    # Separate courses/offsets survive the export as actual geometry.
    for row in range(2):
        for y in (front,back):
            for index in range(14):
                kit.box('Foundation block',(cx-w/2+(index+.5)*w/14,y,.07+row*.13),
                        (w/14-.014,.29,.123),stone,group,.006)
        for x in (cx-w/2,cx+w/2):
            for index in range(8):
                kit.box('Foundation return',(x,front+(index+.5)*d/8,.07+row*.13),
                        (.29,d/8-.014,.123),stone,group,.006)
    for x in (cx-w/2,cx-1.06,cx+1.06,cx+w/2):
        for y in (front-.055,back):
            kit.box('Structural timber',(x,y,1.43),(.15,.21,2.45),wood,group,.008)
    for y in (front,back):
        kit.box('Long wall plate',(cx,y,2.61),(w+.13,.21,.16),wood,group,.008)
    for side in (-1,1):
        panel=kit.box('Solid roof backing',(cx,cy+side*.95,recipe.RIDGE-.95*(1./1.8)-.035),
                      (w+.45,2.35,.07),wood,group,0.)
        panel.rotation_euler.x=-side*math.atan(1./1.8)
    for tile in recipe.roof_tiles():
        roof_pan(*tile,clay,group)
    for column in range(15):
        x=cx+(column-7)*.44
        points=[(x+end,cy+.14*math.cos(i*math.pi/8),recipe.RIDGE+.07+.14*math.sin(i*math.pi/8))
                for end in (-.24,.24) for i in range(9)]
        kit.mesh('Rounded ridge cap',points,[(i,i+1,i+10,i+9) for i in range(8)],clay,group)
    # Long covered frontage changes the silhouette, not merely the material.
    porch_y=front-1.34
    kit.box('Porch stone platform',(cx,front-.72,.06),(w+.12,1.55,.13),stone,group,.012)
    kit.box('Entrance step',(cx,front-1.57,.025),(1.65,.35,.055),stone,group,.012)
    for x in (cx-w/2,cx,cx+w/2):
        kit.box('Porch post base',(x,porch_y,.13),(.28,.28,.22),stone,group,.008)
        kit.box('Porch oak post',(x,porch_y,1.18),(.16,.16,2.13),wood,group,.009)
        for sign in (-1,1):
            if cx-w/2<=x+sign*.42<=cx+w/2:
                beam('Porch knee brace',(x,porch_y,1.7),(x+sign*.42,porch_y,2.17),.09,wood,group)
    kit.box('Porch head beam',(cx,porch_y,2.23),(w+.25,.18,.19),wood,group,.01)
    panel=kit.box('Porch roof backing',(cx,front-.76,2.64-.60*.29-.04),(w+.45,1.64,.065),wood,group,0.)
    panel.rotation_euler.x=math.atan(.29)
    for tile in recipe.roof_tiles(porch=True):
        roof_pan(*tile,clay,group)


def main():
    parser=argparse.ArgumentParser(); parser.add_argument('--output',required=True)
    args=parser.parse_args(sys.argv[sys.argv.index('--')+1:])
    owner_root=ROOT/'Artifacts/VQ02_CampRevision2'
    out=safe_path(Path(args.output).absolute(),(owner_root,),missing=True)
    require(out.parent==owner_root and out.name.startswith('Attempt') and not out.exists(),'fresh_owned_attempt_required')
    source_dir=ROOT/'SourceArt/Environment/RomanCamp/semireal_v1'
    originals=[safe_path(p,(source_dir,)) for p in source_dir.iterdir() if p.is_file()]
    require(len(originals)==14,'original_source_set_changed')
    protected=[file_record(p,ROOT) for p in sorted(originals)]
    out.mkdir(parents=True,exist_ok=False)
    soil_input=source_dir/'Soil-Input01.png'
    # Keep the original generated image byte-for-byte; new variation is procedural.
    (out/'Soil-Input01.png').write_bytes(soil_input.read_bytes())
    bpy.ops.wm.read_factory_settings(use_empty=True)
    bpy.context.scene.unit_settings.system='METRIC'; bpy.context.scene.unit_settings.scale_length=1.
    kit.PARTS.clear(); kit.RNG.seed(recipe.SEED); kit.finish=finish
    soil=kit.material('Soil',(.16,.11,.06),(.34,.25,.14),.94)
    image=bpy.data.images.load(str(out/'Soil-Input01.png'))
    node=soil.node_tree.nodes.new('ShaderNodeTexImage'); node.image=image
    uv=soil.node_tree.nodes.new('ShaderNodeUVMap'); uv.uv_map='GroundSource'
    soil.node_tree.links.new(uv.outputs[0],node.inputs[0])
    soil.node_tree.links.new(node.outputs[0],soil.node_tree.nodes.get('Principled BSDF').inputs['Base Color'])
    wood=kit.material('Wood',(.11,.075,.040),(.31,.215,.12),.84,(11.,11.,.45))
    plaster=kit.material('Plaster',(.48,.42,.31),(.72,.665,.54),.91,(.9,.9,.9))
    clay=kit.material('Terracotta',(.24,.065,.028),(.46,.18,.085),.83,(1.8,1.8,1.8))
    stone=kit.material('Limestone',(.26,.235,.18),(.43,.39,.31),.93)
    iron=kit.material('Iron',(.027,.025,.021),(.09,.081,.066),.63,metal=.8)
    improve_materials(soil,wood,plaster,clay,stone,iron)
    terrain(soil); kit.wall(wood,iron); barrack(wood,plaster,clay,stone,iron)
    # Guard the actual source geometry BEFORE any expensive bake.
    triangle_count=sum(sum(len(face.vertices)-2 for face in obj.data.polygons) for obj in kit.PARTS)
    require(triangle_count<=recipe.TRIANGLE_LIMIT,'source_triangle_budget_exceeded')
    # Thousands of thin tile faces became sub-pixel with v1's8px UV gutters.
    # Retain the2K budget, but allocate actual texels instead of mostly gutters.
    objects,images=kit.bake(out,2048,[soil,wood,plaster,clay,stone,iron],island_margin=.0005,bake_margin=2)
    inventory=[]
    for obj in objects:
        obj['RomanCampSchema']='VQ02/2'; obj['OwnerApproved']=False
        obj.data.calc_loop_triangles()
        inventory.append(dict(name=obj.name,vertices=len(obj.data.vertices),triangles=len(obj.data.loop_triangles),
                              dimensions_m=list(obj.dimensions),material_slots=len(obj.data.materials)))
        kit.activate([obj])
        bpy.ops.export_scene.gltf(filepath=str(out/(obj.name+'.glb')),export_format='GLB',
                                  use_selection=True,export_animations=False,export_extras=True,export_vertex_color='NONE')
    kit.activate(objects)
    bpy.ops.export_scene.gltf(filepath=str(out/'RomanCamp-Proposal02.glb'),export_format='GLB',
                              use_selection=True,export_animations=False,export_extras=True,export_vertex_color='NONE')
    reference=kit.render(out,objects,1600)
    for image in bpy.data.images:
        if image.source=='FILE' and not image.packed_file:
            image.pack()
    bpy.ops.wm.save_as_mainfile(filepath=str(out/'RomanCamp-Proposal02.blend'))
    for record in protected:
        require(digest(ROOT/record['path'])==record['sha256'],'original_source_changed')
    manifest=dict(schema=2,scope='Owner-requested source revision, NOT Unreal/player/adoption approval',
                  owner_request='Cambiar ambas cosas: architecture AND materials',seed=recipe.SEED,
                  blender=bpy.app.version_string,meshes=inventory,atlas_size=2048,units='meters',
                  triangle_limit=recipe.TRIANGLE_LIMIT,reference_legionary=reference,
                  uv_island_margin=.0005,bake_margin_pixels=2,
                  owner_approved=False,imported_to_unreal=False,historical_period_validated=False,
                  changes=['6.2x3.6m long barrack; actual facade openings/covered porch',
                           'long-axis tile roof and porch roof; physical pans/covers',
                           'native procedural macro-earth/wear tracks, plaster dirt and surface variation'],
                  geometry_source='Original GPT Blender construction; no downloaded environment geometry',
                  texture_source='Unchanged v1 GPT soil input + new native procedural shaders/PBR bake',
                  protected_source_files=protected,protected_sources_unchanged=True,
                  tools=[file_record(ROOT/'Tools'/n,ROOT) for n in ('create_semireal_camp_v2.py','semireal_camp_revision_recipe.py','create_semireal_camp.py')],
                  files=[file_record(p,out) for p in sorted(out.iterdir()) if p.is_file()])
    (out/'source-manifest.json').write_text(json.dumps(manifest,indent=2),encoding='utf-8')
    kit.log('REVISION02 complete: inspect actual exports and source renders; no Unreal/mainmap changed')


if __name__=='__main__':
    main()
