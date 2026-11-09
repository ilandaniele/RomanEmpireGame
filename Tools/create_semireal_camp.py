"""Original bounded VQ-02 source proposal; Blender only, never edits Unreal.

Outputs are create-only within Artifacts/VQ02_CampSource1. A source render is
NOT production approval or an Unreal/player test. No external asset download.
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
PARTS = []
RNG = random.Random(1802)


def log(message):
    print('VQ02: ' + message, flush=True)


def activate(objects):
    bpy.ops.object.select_all(action='DESELECT')
    for obj in objects:
        obj.select_set(True)
    bpy.context.view_layer.objects.active = objects[0]


def finish(obj, name, material, group, bevel=0.):
    obj.name = name
    obj.data.materials.append(material)
    activate([obj])
    bpy.ops.object.transform_apply(location=False, rotation=False, scale=True)
    if bevel:
        modifier = obj.modifiers.new('Physical worn edges', 'BEVEL')
        modifier.width, modifier.segments = bevel, 2
        bpy.ops.object.modifier_apply(modifier=modifier.name)
    vertex_group = obj.vertex_groups.new(name=group)
    vertex_group.add(list(range(len(obj.data.vertices))), 1., 'REPLACE')
    PARTS.append(obj)
    return obj


def box(name, center, size, material, group, bevel=.012):
    bpy.ops.mesh.primitive_cube_add(size=1, location=center)
    obj = bpy.context.object
    obj.dimensions = size
    return finish(obj, name, material, group, bevel)


def mesh(name, vertices, faces, material, group):
    data = bpy.data.meshes.new(name)
    data.from_pydata(vertices, [], faces)
    data.update()
    obj = bpy.data.objects.new(name, data)
    bpy.context.collection.objects.link(obj)
    return finish(obj, name, material, group)


def material(name, low, high, roughness, grain=None, metal=0.):
    mat = bpy.data.materials.new(name)
    mat.use_nodes = True
    nodes, links = mat.node_tree.nodes, mat.node_tree.links
    bsdf = nodes.get('Principled BSDF')
    bsdf.inputs['Roughness'].default_value = roughness
    bsdf.inputs['Metallic'].default_value = metal
    tex = nodes.new('ShaderNodeTexNoise')
    tex.inputs['Scale'].default_value = 4.
    tex.inputs['Detail'].default_value = 4.
    coord = nodes.new('ShaderNodeTexCoord')
    vector = nodes.new('ShaderNodeVectorMath')
    vector.operation = 'MULTIPLY'
    vector.inputs[1].default_value = grain or (9., 9., 9.)
    links.new(coord.outputs['Object'], vector.inputs[0])
    links.new(vector.outputs[0], tex.inputs['Vector'])
    ramp = nodes.new('ShaderNodeValToRGB')
    ramp.color_ramp.elements[0].color = (*low, 1.)
    ramp.color_ramp.elements[1].color = (*high, 1.)
    links.new(tex.outputs['Fac'], ramp.inputs[0])
    links.new(ramp.outputs[0], bsdf.inputs['Base Color'])
    bump = nodes.new('ShaderNodeBump')
    bump.inputs['Strength'].default_value = .32
    bump.inputs['Distance'].default_value = .012 if name == 'Wood' else .004
    links.new(tex.outputs['Fac'], bump.inputs['Height'])
    links.new(bump.outputs[0], bsdf.inputs['Normal'])
    mat.diffuse_color = (*high, 1.)
    return mat


def ground(soil):
    n = 24
    vertices = []
    for y in range(n + 1):
        for x in range(n + 1):
            px, py = -6 + x * 12 / n, -5 + y * 10 / n
            z = .006 * math.sin(px * 2.7) * math.cos(py * 3.1)
            vertices.append((px, py, z))
    faces = [(y*(n+1)+x, y*(n+1)+x+1, (y+1)*(n+1)+x+1,
              (y+1)*(n+1)+x) for y in range(n) for x in range(n)]
    obj = mesh('Compacted camp earth', vertices, faces, soil, 'SM_CampGround')
    uv = obj.data.uv_layers.new(name='GroundSource')
    for loop in obj.data.loops:
        position = obj.data.vertices[loop.vertex_index].co
        uv.data[loop.index].uv = ((position.x + 6)/12, (position.y + 5)/10)


def wall(wood, iron):
    for index in range(43):
        x = -4.7 + index * .212
        height = 2.5 + RNG.uniform(-.08, .08)
        bpy.ops.mesh.primitive_cone_add(vertices=8, radius1=.13, radius2=.112,
                                       depth=height-.26, location=(x, 4.15, (height-.26)/2))
        finish(bpy.context.object, 'Palisade hewn oak', wood, 'SM_CampWall')
        bpy.ops.mesh.primitive_cone_add(vertices=8, radius1=.112, radius2=.018,
                                       depth=.30, location=(x, 4.15, height-.13))
        finish(bpy.context.object, 'Sharpened timber cap', wood, 'SM_CampWall')
    for z in (.62, 1.65):
        box('Rear cross brace', (-.25, 4.29, z), (9.25, .16, .18), wood, 'SM_CampWall')
        for x in (-4.3, -2., .3, 2.6, 4.):
            box('Forged brace fastener', (x, 4.39, z), (.07, .015, .07), iron, 'SM_CampWall', .003)


def building(wood, plaster, clay, stone, iron):
    group = 'SM_CampBarracks'
    cx, cy, width, depth = 1.1, .8, 4.6, 3.2
    front, back = cy-depth/2, cy+depth/2
    for x in (cx-width/2, cx+width/2):
        box('Limewashed side', (x, cy, 1.4), (.18, depth, 2.4), plaster, group)
    box('Rear limewash', (cx, back, 1.4), (width, .18, 2.4), plaster, group)
    for sign in (-1, 1):
        box('Front beside door', (cx+sign*1.45, front, 1.4), (1.7, .18, 2.4), plaster, group)
    box('Door lintel infill', (cx, front, 2.35), (1.2, .18, .5), plaster, group)
    for y in (front, back):
        mesh('Gable', [(cx-width/2, y, 2.6), (cx+width/2, y, 2.6), (cx, y, 3.55)],
             [(0, 1, 2) if y == front else (2, 1, 0)], plaster, group)
        box('Wall plate', (cx, y-.025, 2.58), (width+.12, .22, .18), wood, group)
    for x in (cx-width/2, cx+width/2):
        for y in (front-.035, back):
            box('Structural oak post', (x, y, 1.36), (.19, .22, 2.5), wood, group)
    for x in (cx-.62, cx+.62):
        box('Door jamb', (x, front-.12, 1.17), (.14, .16, 2.12), wood, group)
    box('Door lintel', (cx, front-.14, 2.22), (1.42, .19, .19), wood, group)
    for index in range(7):
        box('Door plank', (cx-.52+index*.173, front-.065, 1.13), (.166, .075, 2.), wood, group, .005)
    for z in (.54, 1.61):
        box('Door iron hinge', (cx-.21, front-.118, z), (.65, .018, .045), iron, group, .003)
    box('Door latch', (cx+.39, front-.13, 1.12), (.13, .03, .065), iron, group, .008)
    # Actual stone courses, no texture-painted fake brick geometry.
    for row in range(2):
        for y in (front-.07, back+.07):
            for i in range(10):
                x = cx-width/2+.23+i*.46
                box('Limestone plinth', (x, y, .08+row*.135), (.447, .27, .127), stone, group, .008)
        for x in (cx-width/2-.03, cx+width/2+.03):
            for i in range(7):
                box('Limestone return', (x, front+.22+i*.46, .08+row*.135), (.27, .447, .127), stone, group, .008)
    # Tegula-like flat pans and raised rims, with separate curved imbrex covers.
    # A private visual proposal, not a validated historical reconstruction.
    slope = .95/2.3
    for side in (-1, 1):
        for row in range(6):
            distance = .08 + row*.42
            for col in range(9):
                y = front-.23+col*.41
                x = cx+side*distance
                z = 3.55-distance*slope
                tile = box('Terracotta flat pan', (x, y, z), (.47, .40, .043), clay, group, .004)
                tile.rotation_euler.y = side*math.atan(slope)
                for rim in (-1, 1):
                    rail = box('Tegula rim', (x, y+rim*.188, z+.035), (.47, .028, .065), clay, group, .003)
                    rail.rotation_euler.y = side*math.atan(slope)
                vertices = []
                for end in (-.25, .25):
                    for step in range(7):
                        angle = step*math.pi/6
                        px = x+end
                        vertices.append((px, y-.202+.065*math.cos(angle),
                                         3.55-(side*(px-cx))*slope+.027+.065*math.sin(angle)))
                mesh('Curved imbrex cover', vertices, [(i, i+1, i+8, i+7) for i in range(6)], clay, group)
    box('Roof ridge timber', (cx, cy, 3.49), (.16, depth+.35, .16), wood, group)
    for index in range(10):
        center_y = front-.24+index*.40
        vertices = [(cx+.14*math.cos(step*math.pi/8), center_y+end,
                     3.61+.14*math.sin(step*math.pi/8))
                    for end in (-.22,.22) for step in range(9)]
        mesh('Curved ridge cap', vertices, [(i,i+1,i+10,i+9) for i in range(8)], clay, group)


def bake(out, size, materials, *, island_margin=.004, bake_margin=6):
    log('Joining original geometry and UV-atlas baking PBR')
    activate(PARTS)
    bpy.ops.object.join()
    obj = bpy.context.object
    obj.name = 'CampAtlasBake'
    uv = obj.data.uv_layers.new(name='CampAtlas')
    obj.data.uv_layers.active = uv
    uv.active_render = True
    bpy.ops.object.mode_set(mode='EDIT')
    bpy.ops.mesh.select_all(action='SELECT')
    bpy.ops.uv.smart_project(angle_limit=math.radians(66), island_margin=island_margin)
    bpy.ops.object.mode_set(mode='OBJECT')
    scene = bpy.context.scene
    scene.render.engine = 'CYCLES'
    scene.cycles.device, scene.cycles.samples = 'CPU', 2
    scene.render.bake.margin = bake_margin
    images = {}
    for kind in ('BaseColor', 'Roughness', 'Metallic', 'Normal'):
        log('Baking '+kind)
        image = bpy.data.images.new('Camp_'+kind, width=size, height=size, alpha=False)
        if kind != 'BaseColor':
            image.colorspace_settings.name = 'Non-Color'
        restore = []
        for mat in materials:
            nodes, links = mat.node_tree.nodes, mat.node_tree.links
            target = nodes.new('ShaderNodeTexImage')
            target.image = image
            nodes.active = target
            if kind in ('BaseColor', 'Metallic'):
                output, bsdf = nodes.get('Material Output'), nodes.get('Principled BSDF')
                original = output.inputs['Surface'].links[0].from_socket
                emit = nodes.new('ShaderNodeEmission')
                value = bsdf.inputs['Base Color' if kind == 'BaseColor' else 'Metallic']
                if value.is_linked:
                    links.new(value.links[0].from_socket, emit.inputs['Color'])
                else:
                    v = value.default_value
                    emit.inputs['Color'].default_value = (v,v,v,1.) if isinstance(v,float) else v
                links.new(emit.outputs[0], output.inputs['Surface'])
                restore.append((mat, original, emit))
        bpy.ops.object.bake(type='EMIT' if kind in ('BaseColor','Metallic') else kind.upper())
        for mat, original, emit in restore:
            mat.node_tree.links.new(original, mat.node_tree.nodes.get('Material Output').inputs['Surface'])
            mat.node_tree.nodes.remove(emit)
        image.filepath_raw, image.file_format = str(out/(kind+'.png')), 'PNG'
        image.save()
        images[kind] = image
    for mat in materials:
        # Keep editable author shaders, but export only an unambiguous PBR graph.
        # Bake-target/image duplicates otherwise produce glTF sampler warnings.
        author = mat.copy()
        author.name = 'SOURCE_ONLY_'+mat.name
        author.use_fake_user = True
        nodes, links = mat.node_tree.nodes, mat.node_tree.links
        nodes.clear()
        bsdf = nodes.new('ShaderNodeBsdfPrincipled')
        output = nodes.new('ShaderNodeOutputMaterial')
        links.new(bsdf.outputs[0], output.inputs['Surface'])
        for kind, socket in (('BaseColor','Base Color'),('Roughness','Roughness'),('Metallic','Metallic')):
            tex = nodes.new('ShaderNodeTexImage'); tex.image = images[kind]
            links.new(tex.outputs['Color'], bsdf.inputs[socket])
        tex = nodes.new('ShaderNodeTexImage'); tex.image = images['Normal']
        normal = nodes.new('ShaderNodeNormalMap')
        links.new(tex.outputs['Color'], normal.inputs['Color'])
        links.new(normal.outputs[0], bsdf.inputs['Normal'])
    for name in ('SM_CampWall', 'SM_CampBarracks'):
        activate([obj])
        bpy.ops.object.mode_set(mode='EDIT')
        bpy.ops.mesh.select_all(action='DESELECT')
        bpy.ops.object.vertex_group_set_active(group=name)
        bpy.ops.object.vertex_group_select()
        bpy.ops.mesh.separate(type='SELECTED')
        bpy.ops.object.mode_set(mode='OBJECT')
        separated = next(o for o in bpy.context.selected_objects if o != obj)
        separated.name = name
    obj.name = 'SM_CampGround'
    objects = [bpy.data.objects[n] for n in ('SM_CampGround','SM_CampWall','SM_CampBarracks')]
    for item in objects:
        item['RomanCampSchema'] = 'VQ02/1'
        item['GameplayCollision'] = 'None: source proposal only'
        item['OwnerApproved'] = False
        # Bake/export UV0 must be the atlas, not the original soil source UV.
        for layer in list(item.data.uv_layers):
            if layer.name != 'CampAtlas':
                item.data.uv_layers.remove(layer)
        activate([item]); bpy.ops.object.transform_apply(location=True, rotation=True, scale=True)
    return objects, images


def render(out, objects, size):
    log('Staging source-only preview with existing legionary scale reference')
    before = set(bpy.data.objects)
    reference = ROOT/'SourceArt/Characters/RomanSoldier/semireal_v2/SK_Roman_Rome_Semireal01.glb'
    bpy.ops.import_scene.gltf(filepath=str(reference))
    imported = set(bpy.data.objects)-before
    roots = [o for o in imported if o.parent not in imported]
    for item in roots:
        item.location += Vector((-1.75, -2.25, .02))
        item.rotation_euler.z = math.pi
    scene = bpy.context.scene
    scene.frame_set(0)
    scene.render.engine = 'CYCLES'
    scene.cycles.samples, scene.cycles.use_denoising = 24, True
    scene.render.resolution_x, scene.render.resolution_y = size, int(size*.625)
    scene.render.resolution_percentage = 100
    if scene.world is None:
        scene.world = bpy.data.worlds.new('Camp preview sky')
    scene.world.use_nodes = True
    scene.world.node_tree.nodes.get('Background').inputs[0].default_value = (.36,.43,.52,1.)
    scene.world.node_tree.nodes.get('Background').inputs[1].default_value = .55
    bpy.ops.object.light_add(type='SUN', location=(-4,-6,10))
    light = bpy.context.object
    light.data.energy, light.data.angle = 2.1, .18
    light.rotation_euler = (.45, -.5, -.6)
    bpy.ops.object.light_add(type='AREA', location=(0,-4,8))
    light = bpy.context.object
    light.data.energy, light.data.shape, light.data.size = 650, 'DISK', 8
    bpy.ops.object.camera_add()
    camera = bpy.context.object
    scene.camera = camera
    camera.data.clip_end = 100
    for name, location, target, lens in (
            ('Camp-Overview', (12,-15,12), (0,.4,1.), 45),
            ('Camp-HumanScale', (-3.8,-7,1.8), (.2,1.,1.45), 29)):
        camera.location = location
        camera.rotation_euler = (Vector(target)-camera.location).to_track_quat('-Z','Y').to_euler()
        camera.data.lens = lens
        scene.render.filepath = str(out/(name+'.png'))
        bpy.ops.render.render(write_still=True)
    return dict(path=str(reference.relative_to(ROOT)), sha256=hashlib.sha256(reference.read_bytes()).hexdigest(),
                scope='Preview-only existing legionary; excluded from all camp GLB exports')


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--output', required=True)
    parser.add_argument('--texture-size', type=int, choices=(1024,2048), default=2048)
    parser.add_argument('--render-size', type=int, choices=(1024,1600), default=1600)
    args = parser.parse_args(sys.argv[sys.argv.index('--')+1:])
    out = Path(args.output).resolve()
    out.relative_to(ROOT/'Artifacts/VQ02_CampSource1')
    if not out.is_dir() or {p.name for p in out.iterdir()} != {'Soil-Input01.png'}:
        raise RuntimeError('Fresh owned attempt containing ONLY Soil-Input01.png required')
    bpy.ops.wm.read_factory_settings(use_empty=True)
    bpy.context.scene.unit_settings.system = 'METRIC'
    bpy.context.scene.unit_settings.scale_length = 1.
    soil = material('Soil', (.22,.16,.09), (.46,.35,.20), .94)
    image = bpy.data.images.load(str(out/'Soil-Input01.png'))
    node = soil.node_tree.nodes.new('ShaderNodeTexImage'); node.image = image
    uv = soil.node_tree.nodes.new('ShaderNodeUVMap'); uv.uv_map = 'GroundSource'
    soil.node_tree.links.new(uv.outputs[0], node.inputs[0])
    soil.node_tree.links.new(node.outputs[0], soil.node_tree.nodes.get('Principled BSDF').inputs['Base Color'])
    wood = material('Wood', (.065,.035,.016), (.29,.18,.080), .83, (16.,16.,.6))
    plaster = material('Plaster', (.43,.37,.25), (.71,.65,.49), .89)
    clay = material('Terracotta', (.23,.055,.025), (.48,.18,.075), .78)
    stone = material('Limestone', (.25,.23,.18), (.47,.44,.35), .91)
    iron = material('Iron', (.03,.025,.020), (.10,.085,.065), .57, metal=.8)
    ground(soil); wall(wood, iron); building(wood, plaster, clay, stone, iron)
    objects, images = bake(out, args.texture_size, [soil,wood,plaster,clay,stone,iron])
    inventory = []
    for obj in objects:
        obj.data.calc_loop_triangles()
        inventory.append(dict(name=obj.name, vertices=len(obj.data.vertices),
                              triangles=len(obj.data.loop_triangles),
                              dimensions_m=list(obj.dimensions), material_slots=len(obj.data.materials)))
        activate([obj])
        bpy.ops.export_scene.gltf(filepath=str(out/(obj.name+'.glb')), export_format='GLB',
                                  use_selection=True, export_animations=False, export_extras=True)
    if sum(item['triangles'] for item in inventory) > 65000:
        raise RuntimeError('Bounded camp sample exceeds declared source triangle budget')
    activate(objects)
    bpy.ops.export_scene.gltf(filepath=str(out/'RomanCamp-Proposal01.glb'), export_format='GLB',
                              use_selection=True, export_animations=False, export_extras=True)
    reference = render(out, objects, args.render_size)
    for image in bpy.data.images:
        if image.source == 'FILE' and not image.packed_file:
            image.pack()
    bpy.ops.wm.save_as_mainfile(filepath=str(out/'RomanCamp-Proposal01.blend'))
    manifest = dict(schema=1, scope='Private editable source proposal, NOT Unreal integration/owner acceptance',
                    seed=1802, blender=bpy.app.version_string, author_script_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
                    meshes=inventory, atlas_size=args.texture_size, reference_legionary=reference,
                    geometry_source='Original Blender primitives/mesh construction by GPT; no downloaded environment geometry',
                    texture_source='Built-in GPT image_gen soil input; original Blender procedural wood/plaster/clay/stone/iron; Cycles PBR atlas bake',
                    units='meters; Unreal import must verify100cm per meter',
                    owner_approved=False, imported_to_unreal=False, historical_period_validated=False,
                    files=[dict(path=p.name, bytes=p.stat().st_size, sha256=hashlib.sha256(p.read_bytes()).hexdigest())
                           for p in sorted(out.iterdir()) if p.is_file()])
    (out/'source-manifest.json').write_text(json.dumps(manifest, indent=2), encoding='utf-8')
    log('Source proposal complete; fresh GLB reimport and owner review still required')


if __name__ == '__main__':
    main()
