"""Author a real, versioned Blender/GLB art candidate; never touches Content.

Run using the verified portable Blender with --factory-startup --disable-autoexec
--background --threads 4 --python this_file -- --output Artifacts/<new-attempt>.
The upstream CC0 OBJ is data only. All equipment/material code here is original.
"""
import argparse
import hashlib
import json
import math
from pathlib import Path
import sys

import bpy
from mathutils import Vector
from mathutils.bvhtree import BVHTree

TOOLS = Path(__file__).resolve().parent
ROOT = TOOLS.parent
sys.path.insert(0, str(TOOLS))
from semireal_legionary_mesh import load_anatomy, joint_layout, skin_weights, fit_surface_offset, torso_weights
from roman_character_recipe import CLIPS, MATERIALS
from semireal_character_motion import rotations, root_vertical_offset
from semireal_gltf_cleanup import deduplicate_glb
from pose_semireal_grip import bake_closed_grip

PARTS = []
MATS = []
LAYOUT = []
BODY_BVH = None
RECIPE_SOURCES = {name:(TOOLS/name).read_bytes() for name in
                  (Path(__file__).name,"semireal_legionary_mesh.py","semireal_gltf_cleanup.py","roman_character_recipe.py","pose_semireal_grip.py","semireal_character_motion.py")}


def log(message):
    print("SEMIREAL: " + message, flush=True)


def material(name, color, metal=0., rough=.6, texture="noise"):
    mat = bpy.data.materials.new(name)
    mat.use_nodes = True
    nodes, links = mat.node_tree.nodes, mat.node_tree.links
    bsdf = nodes.get("Principled BSDF")
    bsdf.inputs["Base Color"].default_value = (*color, 1.)
    bsdf.inputs["Metallic"].default_value = metal
    bsdf.inputs["Roughness"].default_value = rough
    coord = nodes.new("ShaderNodeTexCoord")
    noise = nodes.new("ShaderNodeTexNoise")
    noise.inputs["Scale"].default_value = 75 if texture == "skin" else 38
    noise.inputs["Detail"].default_value = 3.
    links.new(coord.outputs["Object"], noise.inputs["Vector"])
    ramp = nodes.new("ShaderNodeValToRGB")
    ramp.color_ramp.elements[0].color = (*(v*.7 for v in color), 1.)
    ramp.color_ramp.elements[1].color = (*(min(1., v*1.18) for v in color), 1.)
    links.new(noise.outputs["Fac"], ramp.inputs["Fac"])
    links.new(ramp.outputs["Color"], bsdf.inputs["Base Color"])
    if texture == "skin":
        attribute = nodes.new("ShaderNodeVertexColor")
        attribute.layer_name = "RomanSkinTone"
        ramp.color_ramp.elements[0].color = (.86,.86,.86,1.)
        ramp.color_ramp.elements[1].color = (1.,1.,1.,1.)
        mix = nodes.new("ShaderNodeMixRGB")
        mix.blend_type = "MULTIPLY"; mix.inputs[0].default_value = 1.
        links.new(attribute.outputs["Color"],mix.inputs[1])
        links.new(ramp.outputs["Color"],mix.inputs[2])
        links.new(mix.outputs[0],bsdf.inputs["Base Color"])
    bump = nodes.new("ShaderNodeBump")
    bump.inputs["Strength"].default_value = .16 if texture == "skin" else .24
    bump.inputs["Distance"].default_value = .0006 if texture == "skin" else .0015
    links.new(noise.outputs["Fac"], bump.inputs["Height"])
    links.new(bump.outputs["Normal"], bsdf.inputs["Normal"])
    if texture == "skin" and "Subsurface Weight" in bsdf.inputs:
        bsdf.inputs["Subsurface Weight"].default_value = .055
    if texture == "mail":
        # A small linked-ring normal pattern, not hundreds of thousands of rings.
        split = nodes.new("ShaderNodeSeparateXYZ")
        links.new(coord.outputs["Object"], split.inputs["Vector"])

        def calc(operation, a, b=None):
            node = nodes.new("ShaderNodeMath")
            node.operation = operation
            if isinstance(a, (float, int)):
                node.inputs[0].default_value = a
            else:
                links.new(a, node.inputs[0])
            if b is not None:
                if isinstance(b, (float, int)):
                    node.inputs[1].default_value = b
                else:
                    links.new(b, node.inputs[1])
            return node.outputs[0]

        x = calc("MULTIPLY", split.outputs["X"], 100.)
        z = calc("MULTIPLY", split.outputs["Z"], 100.)
        stagger = calc("MULTIPLY", calc("MODULO", calc("FLOOR", z), 2.), .5)
        x = calc("SUBTRACT", calc("FRACT", calc("ADD", x, stagger)), .5)
        z = calc("SUBTRACT", calc("FRACT", z), .5)
        radius = calc("SQRT", calc("ADD", calc("MULTIPLY", x, x), calc("MULTIPLY", z, z)))
        ring = calc("LESS_THAN", calc("ABSOLUTE", calc("SUBTRACT", radius, .32)), .09)
        links.new(ring, bump.inputs["Height"])
        bump.inputs["Distance"].default_value = .0022
        ring_color = nodes.new("ShaderNodeValToRGB")
        ring_color.color_ramp.elements[0].color = (.035, .029, .025, 1.)
        ring_color.color_ramp.elements[1].color = (.27, .25, .21, 1.)
        links.new(ring, ring_color.inputs["Fac"])
        links.new(ring_color.outputs["Color"], bsdf.inputs["Base Color"])
    return mat


def finish(obj, slot, bone=None, weight_profile=None):
    bpy.ops.object.select_all(action="DESELECT")
    obj.select_set(True)
    bpy.context.view_layer.objects.active = obj
    bpy.ops.object.transform_apply(location=True, rotation=True, scale=True)
    for modifier in list(obj.modifiers):
        bpy.ops.object.modifier_apply(modifier=modifier.name)
    for mat in MATS:
        obj.data.materials.append(mat)
    for face in obj.data.polygons:
        face.material_index = slot
        face.use_smooth = True
    if bone:
        group = obj.vertex_groups.new(name=bone)
        group.add(list(range(len(obj.data.vertices))), 1., "REPLACE")
    else:
        groups = {name: obj.vertex_groups.new(name=name) for name, _, _ in LAYOUT}
        for vertex in obj.data.vertices:
            for name, weight in (torso_weights if weight_profile=="torso" else skin_weights)(vertex.co, LAYOUT):
                groups[name].add([vertex.index], weight, "REPLACE")
    PARTS.append(obj)
    return obj


def fitted(point, margin):
    surface, normal, _, _ = BODY_BVH.find_nearest(Vector(point))
    if surface is None:
        raise RuntimeError("Body surface unavailable for clothing fit")
    return fit_surface_offset(point,surface,normal,margin)


def mesh(name, vertices, faces, slot, bone=None, uvs=None, fit_body=0., weight_profile=None):
    if fit_body:
        vertices = [fitted(v,fit_body) for v in vertices]
    data = bpy.data.meshes.new(name)
    data.from_pydata(vertices, [], faces)
    data.update()
    if slot == 2:
        colors = data.color_attributes.new(name="RomanSkinTone",type="FLOAT_COLOR",domain="POINT")
        for vertex in data.vertices:
            x,y,z = vertex.co
            color = [.29,.145,.082]
            if y < -.11 and 1.59 < z < 1.72:
                # Original procedural vertex tint, not an external photo/texture.
                lips = math.exp(-((x/.033)**2+((z-1.641)/.007)**2)) if y < -.15 else 0.
                color = [a*(1-lips)+b*lips for a,b in zip(color,(.25,.075,.055))]
                stubble = .12*max(0.,1-abs(z-1.62)/.035)
                color = [v*(1-stubble) for v in color]
            colors.data[vertex.index].color = (*color,1.)
    if uvs:
        uv = data.uv_layers.new(name="SourceUV")
        for face, coords in zip(data.polygons, uvs):
            for index, coordinate in zip(face.loop_indices, coords):
                uv.data[index].uv = coordinate
    obj = bpy.data.objects.new(name, data)
    bpy.context.collection.objects.link(obj)
    return finish(obj, slot, bone, weight_profile)


def ellipsoid(name, pos, size, slot, bone=None, segments=24):
    bpy.ops.mesh.primitive_uv_sphere_add(segments=segments, ring_count=12, location=pos)
    obj = bpy.context.object
    obj.name, obj.scale = name, size
    return finish(obj, slot, bone)


def box(name, pos, size, slot, bone=None, bevel=.002, fit_body=0.):
    bpy.ops.mesh.primitive_cube_add(size=1., location=pos)
    obj = bpy.context.object
    obj.name, obj.scale = name, size
    bpy.ops.object.transform_apply(location=False, rotation=False, scale=True)
    if bevel:
        edge = obj.modifiers.new("Rounded edges", "BEVEL")
        edge.width, edge.segments = bevel, 2
    obj=finish(obj, slot, bone)
    if fit_body:
        for vertex in obj.data.vertices:
            vertex.co=fitted(vertex.co,fit_body)
        obj.data.update()
    return obj


def rings(name, sections, slot, bone=None, folds=0., segments=64, fit_body=0., weight_profile=None):
    # z, X radius, Y radius, Y center; source and resulting faces are real mesh.
    vertices = []
    for z, rx, ry, cy in sections:
        for i in range(segments):
            angle = 2*math.pi*i/segments
            wave = folds * math.sin(18*angle)
            vertices.append(((rx+wave)*math.cos(angle), cy+(ry+wave)*math.sin(angle), z))
    faces = []
    for row in range(len(sections)-1):
        for i in range(segments):
            j = (i+1) % segments
            faces.append((row*segments+i, row*segments+j, (row+1)*segments+j, (row+1)*segments+i))
    return mesh(name, vertices, faces, slot, bone,fit_body=fit_body,weight_profile=weight_profile)


def tube_between(name, start, end, radii, slot, bone=None, segments=32, fit_body=0.):
    a, b = Vector(start), Vector(end)
    axis = (b-a).normalized()
    u = axis.cross(Vector((0., 1., 0.))).normalized()
    if u.length < .1:
        u = axis.cross(Vector((1., 0., 0.))).normalized()
    v = axis.cross(u).normalized()
    verts = []
    for row in range(5):
        t = row/4
        center = a.lerp(b, t)
        radius = radii[0]*(1-t)+radii[1]*t
        for i in range(segments):
            angle = 2*math.pi*i/segments
            verts.append(tuple(center+radius*(math.cos(angle)*u+math.sin(angle)*v)))
    faces = [(r*segments+i, r*segments+(i+1)%segments, (r+1)*segments+(i+1)%segments, (r+1)*segments+i)
             for r in range(4) for i in range(segments)]
    return mesh(name, verts, faces, slot, bone,fit_body=fit_body)


def ribbon(name, a, b, width, slot, bone=None, fit_body=0.):
    a, b = Vector(a), Vector(b)
    direction = (b-a).normalized()
    sideways = direction.cross(Vector((0., -1., 0.))).normalized()*width/2
    count=20 if fit_body else 1
    vertices=[tuple(a.lerp(b,i/count)+sideways*sign) for i in range(count+1) for sign in (-1,1)]
    faces=[(2*i,2*i+1,2*i+3,2*i+2) for i in range(count)]
    return mesh(name,vertices,faces,slot,bone,fit_body=fit_body)


def build_character(anatomy,closed_grip=False):
    global BODY_BVH
    BODY_BVH = BVHTree.FromPolygons(anatomy["vertices"],anatomy["faces"],all_triangles=False)
    joints = {name: Vector(point) for name, _, point in LAYOUT}
    keep_faces, keep_uvs = [], []
    for face, uvs in zip(anatomy["faces"], anatomy["face_uvs"]):
        center = sum((Vector(anatomy["vertices"][i]) for i in face), Vector())/len(face)
        # Skin underneath the closed clothing is excluded, not exported nude.
        # Keep upper-arm/axillary skin beneath separate sleeves. The previous
        # broad rectangle removed it and left holes when lifting an arm.
        covered = .78 < center.z < 1.47 and abs(center.x) < .16
        if not covered:
            keep_faces.append(face)
            keep_uvs.append(uvs)
    skin=mesh("AnatomicalSkin", anatomy["vertices"], keep_faces, 2, uvs=keep_uvs)
    grip=bake_closed_grip(skin,anatomy) if closed_grip else None
    rings("WoolTunic", [(.77,.255,.165,0),(.85,.24,.155,0),(.96,.205,.133,0),
                        (1.08,.19,.127,0),(1.25,.212,.135,0),(1.40,.23,.133,0),
                        (1.48,.24,.125,0),(1.53,.080,.074,-.007)], 0, folds=.003,fit_body=.012,weight_profile="torso")
    rings("MailShirt", [(1.045,.201,.14,0),(1.16,.212,.14,0),(1.32,.229,.146,0),
                        (1.43,.241,.141,0),(1.49,.246,.133,0),(1.54,.084,.078,-.007)], 6,fit_body=.024,weight_profile="torso")
    for side in ("l", "r"):
        shoulder, elbow = joints[f"upper_arm_{side}"], joints[f"forearm_{side}"]
        tube_between("TunicSleeve_"+side, shoulder, shoulder.lerp(elbow,.67), (.086,.073), 0,"upper_arm_"+side,fit_body=.012)
        tube_between("MailSleeve_"+side, shoulder, shoulder.lerp(elbow,.38), (.092,.084), 6,"upper_arm_"+side,fit_body=.024)

    # Belt, diagonal harness and original fasteners.
    rings("LeatherBelt", [(1.035,.213,.151,0),(1.093,.213,.151,0)], 3, "pelvis",fit_body=.038)
    box("BeltBuckle", fitted((0,-.155,1.065),.054), (.057,.012,.047), 5, "pelvis", .005)
    ribbon("ShoulderHarness", (.19,-.16,1.49), (-.15,-.16,1.08), .039, 3, "chest",fit_body=.037)
    box("HarnessBuckle", fitted((.135,-.17,1.43),.049), (.046,.008,.05), 5, "chest", .004)
    for i in range(-3,4):
        x = i*.052
        y = -.153+.04*(abs(i)/3)**2
        box("BeltApron", (x,y,.936), (.036,.009,.205), 3, "pelvis", .004,fit_body=.040)
        for z in (.87,.935,1.00):
            ellipsoid("ApronRivet", fitted((x,y-.008,z),.050), (.005,.002,.005), 5, "pelvis", 12)

    # Helmet shell, brow ring, neck guard, cheek plates and rivets.
    head = joints["head"]
    helmet_cy = head.y+.005
    sections = []
    for row in range(13):
        phi = (math.pi/2)*row/12
        sections.append((1.732+.128*math.sin(phi), max(.001,.106*math.cos(phi)),
                         max(.001,.153*math.cos(phi)), helmet_cy))
    rings("IronHelmet", sections, 1, "head")
    rings("HelmetBrow", [(1.724,.109,.156,helmet_cy),(1.741,.109,.156,helmet_cy)], 5, "head")
    for sign in (-1,1):
        verts = [(sign*.100,-.155,1.725),(sign*.112,helmet_cy+.025,1.71),
                 (sign*.091,-.05,1.59),(sign*.065,-.145,1.605)]
        mesh("CheekGuard", verts, [(0,1,2,3)], 1, "head")
        ellipsoid("HelmetHinge", (sign*.112,helmet_cy-.035,1.715), (.004,.012,.012), 5, "head",16)
    mesh("NeckGuard", [(-.1,helmet_cy+.065,1.68),(.1,helmet_cy+.065,1.68),
                       (.13,helmet_cy+.16,1.64),(-.13,helmet_cy+.16,1.64)], [(0,1,2,3)], 1, "head")
    for i in range(12):
        t = 2*math.pi*i/12
        ellipsoid("HelmetRivet", (.111*math.cos(t),helmet_cy+.157*math.sin(t),1.733),
                  (.0035,.0035,.0035), 5, "head",12)

    # Eye positions are upstream geometry helpers, not guessed face coordinates.
    for source in ("l-eye", "r-eye"):
        # Seat the globe behind the existing eyelids, not on the face surface.
        eye = Vector(anatomy["joints"][source])
        ellipsoid("Eye", eye, (.016,.022,.013), 7, "head",24)
        ellipsoid("Iris", eye+Vector((0,-.021,0)), (.0051,.001,.0051), 3, "head",24)
    for sign in (-1,1):
        for i in range(7):
            def brow(t):
                x = .012+.047*t
                return (sign*x,-.181+.030*(x/.07)**2,1.707+.004*math.sin(math.pi*t))
            ribbon("Eyebrow",brow(i/7),brow((i+1)/7),.0035,3,"head")

    for side in ("l", "r"):
        ankle = joints[f"foot_{side}"]
        ellipsoid("SandalSole", (ankle.x,-.065,.022), (.075,.155,.018), 3, f"foot_{side}",32)
        for y in (-.155,-.095,-.035):
            vertices = []
            for row in (-1,1):
                for i in range(17):
                    angle = math.pi*i/16
                    vertices.append((ankle.x+.068*math.cos(angle),y+row*.011,
                                     .027+.042*math.sin(angle)))
            faces = [(i,i+1,18+i,17+i) for i in range(16)]
            mesh("SandalStrap",vertices,faces,3,f"foot_{side}")
        # Ankle wraps follow the actual spread of the anatomical source.
        for z in (.10,.135,.17):
            side_vertices = [v for v in anatomy["vertices"] if v[0]*ankle.x>0]
            # Sparse source rings need a nearest-surface band, not an assumed
            # vertex at an arbitrary wrap height.
            tolerance = max(.025,min(abs(v[2]-z) for v in side_vertices)+.01)
            section = [v for v in side_vertices if abs(v[2]-z)<=tolerance]
            cx = (min(v[0] for v in section)+max(v[0] for v in section))/2
            cy = (min(v[1] for v in section)+max(v[1] for v in section))/2
            rx = (max(v[0] for v in section)-min(v[0] for v in section))/2+.003
            ry = (max(v[1] for v in section)-min(v[1] for v in section))/2+.003
            sections = [(z-.009,rx,ry,cy),(z+.009,rx,ry,cy)]
            obj = rings("AnkleWrap", sections, 3, f"foot_{side}",segments=32)
            for vertex in obj.data.vertices:
                vertex.co.x += cx

    # Curved shield with a back surface and an original geometric wing motif.
    hand = joints["hand_l"]
    sx, sy, sz = hand.x+.025, hand.y-.12, 1.06
    width, height = .55, .98

    def shield_point(x,z,front=True):
        return (sx+x, sy+.17*(x/(width/2))**2 + (0 if front else .025), sz+z)

    verts, faces = [], []
    nx, nz = 20, 24
    for back in (False, True):
        for row in range(nz+1):
            z = (row/nz-.5)*height
            corner = .055*max(0.,(abs(z)-.41)/.08)
            for col in range(nx+1):
                x = (col/nx-.5)*(width-2*corner)
                verts.append(shield_point(x,z,not back))
    count = (nx+1)*(nz+1)
    for back in (0,1):
        for row in range(nz):
            for col in range(nx):
                a = back*count+row*(nx+1)+col
                face = (a,a+1,a+nx+2,a+nx+1)
                faces.append(face[::-1] if back else face)
    mesh("CurvedScutum", verts, faces, 4, "hand_l")
    ellipsoid("ShieldBoss", (sx,sy-.025,sz), (.091,.048,.091), 1, "hand_l",32)
    for sign in (-1,1):
        for i in range(5):
            x1, x2 = sign*(.07+.012*i), sign*(.23-.008*i)
            z1, z2 = .025+.035*i, .12+.055*i
            for vertical in (-1,1):
                a = Vector(shield_point(x1,vertical*z1)); a.y -= .003
                b = Vector(shield_point(x2,vertical*z2)); b.y -= .003
                ribbon("PaintedWing", a,b,.018,5,"hand_l")
    for x in (-.257,.257):
        for z in (-.40,-.27,-.13,0.,.13,.27,.40):
            p = Vector(shield_point(x,z)); p.y -= .003
            ellipsoid("ShieldRivet", p, (.004,.003,.004), 5, "hand_l",12)
    # Drawn gladius, actual hand-linked geometry rather than a texture silhouette.
    palm_source = "l-hand-2" if anatomy["joints"]["l-hand-2"][0] > 0 else "r-hand-2"
    hand = Vector(grip) if grip else Vector(anatomy["joints"][palm_source])
    x,y,z = hand.x,hand.y-.016,hand.z
    box("SwordGrip", (x,y,z-.01), (.029,.029,.10), 3,"hand_r",.006)
    ellipsoid("SwordPommel", (x,y,z+.055), (.029,.023,.023),5,"hand_r",20)
    box("SwordGuard", (x,y,z-.070), (.095,.035,.026),5,"hand_r",.006)
    mesh("GladiusBlade", [(x-.024,y,z-.085),(x,y-.009,z-.085),(x+.024,y,z-.085),(x,y+.009,z-.085),
                          (x-.019,y,z-.40),(x,y-.007,z-.40),(x+.019,y,z-.40),(x,y+.007,z-.40),
                          (x,y,z-.49)],
         [(0,1,5,4),(1,2,6,5),(2,3,7,6),(3,0,4,7),(4,5,8),(5,6,8),(6,7,8),(7,4,8)],1,"hand_r")


def make_rig():
    data = bpy.data.armatures.new("RomanSemiRealSkeleton")
    rig = bpy.data.objects.new("RomanSemiRealRig", data)
    bpy.context.collection.objects.link(rig)
    bpy.ops.object.select_all(action="DESELECT")
    rig.select_set(True)
    bpy.context.view_layer.objects.active = rig
    bpy.ops.object.mode_set(mode="EDIT")
    for name,parent,point in LAYOUT:
        bone = data.edit_bones.new(name)
        bone.head, bone.tail = point, Vector(point)+Vector((0,0,.07))
        if parent >= 0:
            bone.parent = data.edit_bones[LAYOUT[parent][0]]
    bpy.ops.object.mode_set(mode="OBJECT")
    for clip,duration in CLIPS.items():
        action = bpy.data.actions.new("A_RomanSemiReal_"+clip)
        rig.animation_data_create()
        rig.animation_data.action = action
        for frame in range(round(duration*30)+1):
            samples = rotations(clip,frame/30.)
            for bone in rig.pose.bones:
                pitch,twist = samples.get(bone.name,(0,0))
                bone.rotation_mode = "XYZ"
                bone.rotation_euler = (math.radians(pitch),math.radians(twist),0.)
                bone.keyframe_insert(data_path="rotation_euler", frame=frame)
                if bone.name=="root":
                    bone.location=(0.,root_vertical_offset(clip,frame/30.,LAYOUT),0.)
                    bone.keyframe_insert(data_path="location",frame=frame)
        track = rig.animation_data.nla_tracks.new()
        track.name = clip
        track.strips.new(clip,0,action)
    rig.animation_data.action = None
    for track in rig.animation_data.nla_tracks:
        track.mute = True
    for bone in rig.pose.bones:
        bone.rotation_euler = (0,0,0)
    return rig


def export_asset(character, rig, path):
    bpy.ops.object.select_all(action="DESELECT")
    character.select_set(True); rig.select_set(True)
    bpy.context.view_layer.objects.active = rig
    bpy.ops.export_scene.gltf(filepath=str(path), export_format="GLB", use_selection=True,
                              export_animations=True, export_animation_mode="NLA_TRACKS",
                              export_nla_strips=True, export_skins=True, export_extras=True,
                              export_yup=True, export_force_sampling=True)
    # This is a newly generated output owned by this invocation. No published
    # source/imported asset is overwritten; all binary geometry/images survive.
    path.write_bytes(deduplicate_glb(path.read_bytes()))


def export_lods(character, rig, out):
    results = []
    for level,ratio in ((1,.25),(2,.10)):
        obj = character.copy(); obj.data = character.data.copy()
        bpy.context.collection.objects.link(obj)
        obj.name = character.name+"_LOD"+str(level)
        for modifier in list(obj.modifiers):
            obj.modifiers.remove(modifier)
        bpy.ops.object.select_all(action="DESELECT")
        obj.select_set(True); bpy.context.view_layer.objects.active = obj
        decimate = obj.modifiers.new("SourceLODCandidate", "DECIMATE")
        decimate.ratio = ratio
        decimate.use_collapse_triangulate = True
        bpy.ops.object.modifier_apply(modifier=decimate.name)
        deform = obj.modifiers.new("Roman17BoneSkin", "ARMATURE"); deform.object = rig
        path = out/(obj.name+".glb")
        export_asset(obj,rig,path)
        results.append({"level":level,"path":str(path),"triangles":sum(len(f.vertices)-2 for f in obj.data.polygons),
                        "sha256":hashlib.sha256(path.read_bytes()).hexdigest()})
        bpy.data.objects.remove(obj,do_unlink=True)
    return results


def bake_textures(obj, out, size):
    log("UV unwrap and native material baking")
    bpy.ops.object.select_all(action="DESELECT")
    obj.select_set(True)
    bpy.context.view_layer.objects.active = obj
    bpy.ops.object.mode_set(mode="EDIT")
    bpy.ops.mesh.select_all(action="SELECT")
    bpy.ops.uv.smart_project(angle_limit=math.radians(66), island_margin=.006)
    bpy.ops.object.mode_set(mode="OBJECT")
    scene = bpy.context.scene
    scene.render.engine = "CYCLES"
    scene.cycles.device = "CPU"
    scene.cycles.samples = 4
    scene.render.bake.margin = 8
    textures = {}
    for kind in ("BaseColor","Roughness","Metallic","Normal"):
        log("Baking "+kind)
        image = bpy.data.images.new("RomanSemiReal_"+kind, width=size,height=size,alpha=False)
        if kind != "BaseColor":
            image.colorspace_settings.name = "Non-Color"
        restore = []
        for mat in MATS:
            nodes,links = mat.node_tree.nodes,mat.node_tree.links
            tex = nodes.new("ShaderNodeTexImage")
            tex.image = image
            nodes.active = tex
            if kind in ("BaseColor","Metallic"):
                bsdf = nodes.get("Principled BSDF")
                output = nodes.get("Material Output")
                emission = nodes.new("ShaderNodeEmission")
                original = output.inputs["Surface"].links[0].from_socket
                source = bsdf.inputs["Base Color" if kind == "BaseColor" else "Metallic"]
                if source.is_linked:
                    links.new(source.links[0].from_socket,emission.inputs["Color"])
                else:
                    value = source.default_value
                    emission.inputs["Color"].default_value = (value,value,value,1.) if isinstance(value,float) else value
                links.new(emission.outputs[0],output.inputs["Surface"])
                restore.append((mat,original,emission))
        bpy.ops.object.bake(type="EMIT" if kind in ("BaseColor","Metallic") else kind.upper())
        for mat,original,emission in restore:
            mat.node_tree.links.new(original,mat.node_tree.nodes.get("Material Output").inputs["Surface"])
            mat.node_tree.nodes.remove(emission)
        image.filepath_raw = str(out/(kind+".png"))
        image.file_format = "PNG"
        image.save()
        textures[kind] = image
    for mat in MATS:
        nodes,links = mat.node_tree.nodes,mat.node_tree.links
        bsdf = nodes.get("Principled BSDF")
        for kind,socket in (("BaseColor","Base Color"),("Roughness","Roughness"),("Metallic","Metallic")):
            node = nodes.new("ShaderNodeTexImage")
            node.image = textures[kind]
            links.new(node.outputs["Color"],bsdf.inputs[socket])
        node = nodes.new("ShaderNodeTexImage"); node.image = textures["Normal"]
        normal = nodes.new("ShaderNodeNormalMap")
        links.new(node.outputs["Color"],normal.inputs["Color"])
        links.new(normal.outputs["Normal"],bsdf.inputs["Normal"])
    return textures


def stage_and_render(out, size):
    log("Rendering source-art preview, not Unreal")
    scene = bpy.context.scene
    scene.render.engine = "CYCLES"
    scene.cycles.samples = 32
    scene.cycles.use_denoising = True
    scene.render.resolution_x = size
    scene.render.resolution_y = size
    scene.render.resolution_percentage = 100
    scene.world.color = (.22,.22,.22)
    bpy.ops.mesh.primitive_plane_add(size=200,location=(0,0,-.002))
    floor = bpy.context.object
    floor.name = "PreviewOnly_Floor"
    mat = bpy.data.materials.new("PreviewOnly_Stone")
    mat.diffuse_color = (.13,.115,.095,1.)
    floor.data.materials.append(mat)
    for name,position,power,scale in (("Key",(2,-3,4),230,3),("Fill",(-3,-2,2.5),75,3),("Rim",(0,2,3),250,2)):
        bpy.ops.object.light_add(type="AREA",location=position)
        light = bpy.context.object
        light.name = "PreviewOnly_"+name
        light.data.energy,light.data.shape,light.data.size = power,"DISK",scale
        light.rotation_euler = (Vector((0,0,1.))-light.location).to_track_quat("-Z","Y").to_euler()
    bpy.ops.object.camera_add(location=(2.4,-4.5,2.0))
    camera = bpy.context.object
    camera.name = "PreviewOnly_Camera"
    camera.data.type, camera.data.ortho_scale = "ORTHO",2.35
    scene.camera = camera
    for name,position in (("Legionary-ThreeQuarter",(2.4,-4.5,2.0)),("Legionary-Front",(0,-5,1.25)),("Legionary-Back",(1.8,4.5,1.8))):
        camera.location = position
        camera.rotation_euler = (Vector((0,-.05,.94))-camera.location).to_track_quat("-Z","Y").to_euler()
        scene.render.filepath = str(out/(name+".png"))
        bpy.ops.render.render(write_still=True)


def main():
    global MATS,LAYOUT
    parser = argparse.ArgumentParser()
    parser.add_argument("--output",required=True)
    parser.add_argument("--anatomy-profile",choices=("base","roman_adult_v1"),default="base")
    parser.add_argument("--closed-grip",action="store_true")
    parser.add_argument("--texture-size",type=int,default=1024,choices=(512,1024,2048))
    parser.add_argument("--render-size",type=int,default=1024,choices=(512,1024,1536))
    args = parser.parse_args(sys.argv[sys.argv.index("--")+1:])
    out = Path(args.output).resolve()
    out.relative_to((ROOT/"Artifacts").resolve())
    if out.exists():
        raise RuntimeError("Output exists; use a new attempt, never overwrite")
    out.mkdir(parents=True)
    snapshot = out/"recipe"
    snapshot.mkdir()
    for name,raw in RECIPE_SOURCES.items():
        with (snapshot/name).open("xb") as file:
            file.write(raw)
    bpy.ops.object.select_all(action="SELECT")
    bpy.ops.object.delete(use_global=False)
    anatomy = load_anatomy(ROOT,args.anatomy_profile)
    LAYOUT = joint_layout(anatomy)
    MATS = [material("Body",(.24,.035,.022),texture="cloth"),
            material("Metal",(.24,.22,.18),metal=.9,rough=.48),
            material("Skin",(.49,.29,.18),rough=.52,texture="skin"),
            material("Leather",(.09,.039,.018),rough=.72),
            material("Shield",(.27,.028,.016),rough=.65),
            material("Crest",(.36,.23,.075),metal=.72,rough=.46),
            material("MailLinks",(.24,.22,.18),metal=.9,rough=.52,texture="mail"),
            material("EyeWhite",(.72,.69,.62),rough=.32)]
    log("Authoring anatomy, clothing and equipment")
    build_character(anatomy,args.closed_grip)
    bpy.ops.object.select_all(action="DESELECT")
    for obj in PARTS:
        obj.select_set(True)
    bpy.context.view_layer.objects.active = PARTS[0]
    bpy.ops.object.join()
    character = bpy.context.object
    character.name = "SK_Roman_Rome_Semireal01"
    character["RomanArtSchema"] = "RomanSemiReal/1"
    character["AnatomySourceSHA256"] = anatomy["source_sha256"]
    # Remove unreferenced hidden source vertices after clothing occlusion.
    import bmesh
    bm = bmesh.new(); bm.from_mesh(character.data)
    bmesh.ops.delete(bm, geom=[v for v in bm.verts if not v.link_faces], context="VERTS")
    bm.to_mesh(character.data); bm.free()
    textures = bake_textures(character,out,args.texture_size)
    # The tint is now baked. Exporting it again as COLOR_0 would multiply it
    # twice and could blacken equipment whose source had no color attribute.
    for attribute in list(character.data.color_attributes):
        character.data.color_attributes.remove(attribute)
    rig = make_rig()
    character.parent = rig
    deform = character.modifiers.new("Roman17BoneSkin", "ARMATURE")
    deform.object = rig
    for track in rig.animation_data.nla_tracks:
        track.mute = False
    bpy.context.scene.render.fps = 30
    bpy.context.scene.frame_set(0)
    bpy.ops.object.select_all(action="DESELECT")
    character.select_set(True); rig.select_set(True)
    bpy.context.view_layer.objects.active = rig
    log("Exporting real skinned GLB with clips and embedded baked maps")
    export = out/"SK_Roman_Rome_Semireal01.glb"
    export_asset(character,rig,export)
    lods = export_lods(character,rig,out)
    for track in rig.animation_data.nla_tracks:
        track.mute = True
    rig.animation_data.action = None
    for bone in rig.pose.bones:
        bone.rotation_euler = (0,0,0)
    bpy.context.scene.frame_set(0)
    triangle_count = sum(len(face.vertices)-2 for face in character.data.polygons)
    manifest = {"schema":"RomanSemiReal/1","blender":bpy.app.version_string,
                "recipe_sha256":{name:hashlib.sha256(raw).hexdigest() for name,raw in RECIPE_SOURCES.items()},
                "source_sha256":anatomy["source_sha256"],"anatomy_profile":anatomy["profile"],
                "morph_sources":anatomy["morph_sources"],"vertices":len(character.data.vertices),
                "baked_weapon_grip":args.closed_grip,
                "triangles":triangle_count,"bones":[bone.name for bone in rig.data.bones],
                "clips":CLIPS,"semantic_slots":list(MATERIALS),"material_slots":[m.name for m in MATS],
                "glb_sha256":hashlib.sha256(export.read_bytes()).hexdigest(),
                "textures":{name:{"path":image.filepath,"size":list(image.size)} for name,image in textures.items()},
                "lod_candidates":lods,"production_approved":False,"unreal_imported":False}
    (out/"source-manifest.json").write_text(json.dumps(manifest,indent=2),encoding="utf-8")
    # Keep the editable source portable; a cloned repo has no Artifacts paths.
    for image in textures.values():
        image.pack()
    bpy.ops.wm.save_as_mainfile(filepath=str(out/"Legionary-Candidate01.blend"))
    stage_and_render(out,args.render_size)
    bpy.ops.wm.save_as_mainfile(filepath=str(out/"Legionary-PreviewScene01.blend"))
    log("COMPLETE "+str(out))


if __name__ == "__main__":
    main()
