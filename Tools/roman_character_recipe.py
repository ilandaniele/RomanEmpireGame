"""Original low-poly skinned soldiers, schema1. Pure stdlib glTF2/GLB recipe.

Meters/Y-up/+Z forward in source; Unreal Interchange converts to cm/Z-up.
Rigid segmented armor has one normalized bone influence per vertex. No images,
third-party meshes, historical-recording claims or filesystem/engine access.
"""
import hashlib
import json
import math
import struct

SCHEMA = "1"
PROFILES = ("Rome", "Carthage", "Gaul")
MATERIALS = ("Body", "Metal", "Skin", "Leather", "Shield", "Crest")
# Name, parent index, absolute rest-position (meters). Ordered parents first.
BONES = (
    ("root", -1, (0, 0, 0)), ("pelvis", 0, (0, .97, 0)),
    ("spine", 1, (0, 1.18, 0)), ("chest", 2, (0, 1.40, 0)),
    ("head", 3, (0, 1.62, 0)),
    ("upper_arm_l", 3, (-.25, 1.42, 0)), ("forearm_l", 5, (-.43, 1.19, 0)),
    ("hand_l", 6, (-.49, .99, .04)),
    ("upper_arm_r", 3, (.25, 1.42, 0)), ("forearm_r", 8, (.43, 1.19, 0)),
    ("hand_r", 9, (.49, .99, .04)),
    ("thigh_l", 1, (-.12, .94, 0)), ("calf_l", 11, (-.12, .51, 0)),
    ("foot_l", 12, (-.12, .09, .02)),
    ("thigh_r", 1, (.12, .94, 0)), ("calf_r", 14, (.12, .51, 0)),
    ("foot_r", 15, (.12, .09, .02)),
)
CLIPS = {"Idle": 2.0, "Walk": 1.0, "Attack": .6, "Block": .8, "Death": 1.0}
_IDS = {bone[0]: index for index, bone in enumerate(BONES)}


def _sub(a, b): return tuple(x-y for x, y in zip(a, b))
def _add(a, b): return tuple(x+y for x, y in zip(a, b))
def _mul(a, scale): return tuple(x*scale for x in a)
def _cross(a, b): return (a[1]*b[2]-a[2]*b[1], a[2]*b[0]-a[0]*b[2], a[0]*b[1]-a[1]*b[0])
def _unit(a):
    length = math.sqrt(sum(x*x for x in a))
    if length < 1e-9: raise ValueError("Degenerate recipe geometry")
    return _mul(a, 1/length)


class _Geometry:
    def __init__(self): self.groups = [[] for _ in MATERIALS]

    def face(self, points, bone, material):
        normal = _unit(_cross(_sub(points[1], points[0]), _sub(points[2], points[0])))
        # Independent flat-face vertices preserve armor edges and UV seams.
        uvs = ((0, 0), (1, 0), (1, 1), (0, 1))
        for triangle in ((0, 1, 2), (0, 2, 3)) if len(points) == 4 else ((0, 1, 2),):
            for index in triangle: self.groups[material].append((points[index], normal, uvs[index], _IDS[bone]))

    def box(self, center, size, bone, material):
        x, y, z = (s/2 for s in size)
        corners = [_add(center, p) for p in ((-x,-y,-z),(x,-y,-z),(x,y,-z),(-x,y,-z),
                                            (-x,-y,z),(x,-y,z),(x,y,z),(-x,y,z))]
        for face in ((0,3,2,1),(4,5,6,7),(0,1,5,4),(3,7,6,2),(0,4,7,3),(1,2,6,5)):
            self.face([corners[i] for i in face], bone, material)

    def prism(self, start, end, radius, bone, material, segments=8, end_radius=None):
        direction = _unit(_sub(end, start))
        seed = (1,0,0) if abs(direction[0]) < .9 else (0,0,1)
        u = _unit(_cross(direction, seed)); v = _cross(direction, u)
        top_radius = radius if end_radius is None else end_radius
        rings = [[_add(center, _mul(_add(_mul(u,math.cos(2*math.pi*i/segments)),
                                           _mul(v,math.sin(2*math.pi*i/segments))), r))
                  for i in range(segments)] for center, r in ((start,radius),(end,top_radius))]
        for i in range(segments):
            j = (i+1) % segments
            self.face((rings[0][i], rings[0][j], rings[1][j], rings[1][i]),bone,material)
            self.face((start,rings[0][j],rings[0][i]),bone,material)
            self.face((end,rings[1][i],rings[1][j]),bone,material)


def geometry(profile):
    if profile not in PROFILES: raise ValueError(f"Unknown character profile: {profile}")
    g = _Geometry()
    g.box((0,1.23,0),(.42,.46,.26),"chest",0)
    g.box((0,.99,0),(.40,.20,.26),"pelvis",3)
    g.box((0,1.06,0),(.44,.055,.28),"pelvis",3)
    # Distinct skirt strips and layered breastplate, not a texture-only silhouette.
    for x in (-.16,-.08,0,.08,.16): g.box((x,.84,.015),(.065,.23,.26),"pelvis",0)
    for y in (1.11,1.19,1.27,1.35,1.43):
        g.box((0,y,.016),(.43,.049,.28),"chest",1)
    g.prism((0,1.47,0),(0,1.55,0),.065,"head",2)
    g.prism((0,1.50,0),(0,1.72,0),.125,"head",2,8,.115)
    # Nose/front and cheek guards retain obvious facing direction.
    g.box((0,1.61,.13),(.046,.075,.048),"head",2)
    g.box((0,1.735,0),(.29,.10,.27),"head",1)
    for x in (-.13,.13): g.box((x,1.61,.04),(.035,.14,.15),"head",1)
    if profile == "Rome":
        g.box((0,1.84,-.025),(.065,.16,.27),"head",5)
    elif profile == "Carthage":
        g.prism((0,1.78,0),(0,1.87,0),.13,"head",1,8,.035)
        # Nonempty sixth material group, without giving the enemy Rome's crest.
        g.box((0,1.75,.14),(.20,.035,.025),"head",5)
    else:
        # Original stylized art, not a claim of historically universal horned helmets.
        for side in (-1,1):
            g.prism((side*.12,1.76,0),(side*.24,1.86,0),.045,"head",1,6,.015)
        g.box((0,1.73,-.14),(.24,.055,.035),"head",5)
    for side in ("l","r"):
        def pos(part): return BONES[_IDS[f"{part}_{side}"]][2]
        arm, elbow, hand = pos("upper_arm"), pos("forearm"), pos("hand")
        g.prism(arm,elbow,.075,f"upper_arm_{side}",2,6,.058)
        g.prism(elbow,hand,.055,f"forearm_{side}",2,6,.046)
        g.box(_add(arm,(0,-.018,0)),(.20,.13,.23),f"upper_arm_{side}",1)
        g.box(hand,(.105,.13,.11),f"hand_{side}",2)
        hip,knee,foot = pos("thigh"),pos("calf"),pos("foot")
        g.prism(hip,knee,.086,f"thigh_{side}",2,8,.067)
        g.prism(knee,foot,.064,f"calf_{side}",2,8,.047)
        g.box(_add(foot,(0,-.025,.045)),(.14,.13,.25),f"foot_{side}",3)
        g.box(_add(knee,(0,-.18,.05)),(.105,.25,.035),f"calf_{side}",1)
    left = BONES[_IDS["hand_l"]][2]
    shield = _add(left,(0,.015,.16))
    if profile == "Rome":
        g.box(shield,(.40,.76,.055),"hand_l",4)
        for offset in (-.37,.37): g.box(_add(shield,(0,offset,.012)),(.42,.027,.065),"hand_l",1)
        for offset in (-.20,.20): g.box(_add(shield,(offset,0,.012)),(.024,.74,.065),"hand_l",1)
    elif profile == "Carthage":
        g.prism(_add(shield,(0,0,-.03)),_add(shield,(0,0,.03)),.33,"hand_l",4,12)
        g.prism(_add(shield,(0,0,.028)),_add(shield,(0,0,.052)),.085,"hand_l",1,12,.06)
    else:
        # Eight-sided tall oval, distinct from Rome's rectangle and Carthage's disc.
        rings = [[_add(shield,(.22*math.cos(i*math.pi/4),.40*math.sin(i*math.pi/4),z))
                  for i in range(8)] for z in (-.03,.03)]
        for i in range(8):
            j=(i+1)%8
            g.face((rings[0][i],rings[0][j],rings[1][j],rings[1][i]),"hand_l",4)
            g.face((_add(shield,(0,0,-.03)),rings[0][j],rings[0][i]),"hand_l",4)
            g.face((_add(shield,(0,0,.03)),rings[1][i],rings[1][j]),"hand_l",4)
        g.box(_add(shield,(0,0,.045)),(.11,.19,.035),"hand_l",1)
    right = BONES[_IDS["hand_r"]][2]
    g.box(_add(right,(0,-.13,0)),(.05,.19,.05),"hand_r",3)
    g.box(_add(right,(0,-.225,0)),(.16,.035,.08),"hand_r",1)
    g.box(_add(right,(0,-.45,0)),(.065,.42,.025),"hand_r",1)
    return g.groups


def rotations(clip, time):
    """Local additional rotations in degrees (X pitch/Y twist). In-place motion."""
    duration = CLIPS[clip]; t = time/duration; wave = math.sin(2*math.pi*t)
    result = {}
    if clip == "Idle": result = {"spine": (1.4*wave,0), "head": (0,2*wave)}
    elif clip == "Walk":
        for side, sign in (("l",1),("r",-1)):
            result[f"thigh_{side}"] = (30*sign*wave,0)
            result[f"calf_{side}"] = (-26*max(0,sign*wave),0)
            result[f"upper_arm_{side}"] = (-15*sign*wave,0)
    elif clip == "Attack":
        swing = math.sin(math.pi*t)**2
        result = {"upper_arm_r": (-115*swing,0), "forearm_r": (-45*swing,0),
                  "chest": (0,-22*swing)}
    elif clip == "Block":
        hold = min(1,t*5,(1-t)*5); hold = hold*hold*(3-2*hold)
        result = {"upper_arm_l": (-65*hold,0), "forearm_l": (-55*hold,0)}
    elif clip == "Death":
        ease = t*t*(3-2*t); result = {"root": (88*ease,0), "head": (-12*ease,0)}
    else: raise ValueError(f"Unknown character clip: {clip}")
    return result


def _quaternion(pitch, twist):
    x = math.radians(pitch)/2; y = math.radians(twist)/2
    # Y twist * X pitch, glTF [x,y,z,w].
    return (math.sin(x)*math.cos(y),math.cos(x)*math.sin(y),-math.sin(x)*math.sin(y),math.cos(x)*math.cos(y))


def document(profile, include_animations=True):
    """Return glTF document + exact binary data; no file writes."""
    groups = geometry(profile)
    data = bytearray(); views = []; accessors = []
    def accessor(rows, component_type, shape, bounds=False):
        width = {"SCALAR":1,"VEC2":2,"VEC3":3,"VEC4":4,"MAT4":16}[shape]
        while len(data) % 4: data.append(0)
        start = len(data); code = "f" if component_type == 5126 else "H"
        for row in rows: data.extend(struct.pack("<"+code*width,*row))
        views.append({"buffer":0,"byteOffset":start,"byteLength":len(data)-start})
        item = {"bufferView":len(views)-1,"componentType":component_type,"count":len(rows),"type":shape}
        if bounds:
            item["min"] = [min(row[i] for row in rows) for i in range(width)]
            item["max"] = [max(row[i] for row in rows) for i in range(width)]
        accessors.append(item); return len(accessors)-1
    primitives = []
    for slot, vertices in enumerate(groups):
        attributes = {
            "POSITION":accessor([v[0] for v in vertices],5126,"VEC3",True),
            "NORMAL":accessor([v[1] for v in vertices],5126,"VEC3"),
            "TEXCOORD_0":accessor([v[2] for v in vertices],5126,"VEC2"),
            "JOINTS_0":accessor([(v[3],0,0,0) for v in vertices],5123,"VEC4"),
            "WEIGHTS_0":accessor([(1,0,0,0) for _ in vertices],5126,"VEC4"),
        }
        primitives.append({"attributes":attributes,"indices":accessor([(i,) for i in range(len(vertices))],5123,"SCALAR"),"material":slot})
    nodes = []
    for index,(name,parent,absolute) in enumerate(BONES):
        nodes.append({"name":name,"translation":list(_sub(absolute,BONES[parent][2]) if parent >= 0 else absolute)})
        children = [i for i,bone in enumerate(BONES) if bone[1] == index]
        if children: nodes[-1]["children"] = children
    mesh_name = f"SK_Roman_{profile}"
    nodes.append({"name":mesh_name,"mesh":0,"skin":0})
    matrices = []
    for _,_,(x,y,z) in BONES:
        matrices.append((1,0,0,0, 0,1,0,0, 0,0,1,0, -x,-y,-z,1))
    skin = {"name":"RomanSkeleton","joints":list(range(len(BONES))),"skeleton":0,
            "inverseBindMatrices":accessor(matrices,5126,"MAT4")}
    animations = []
    if include_animations:
        for clip,duration in CLIPS.items():
            frames = round(duration*30); times = [i/30 for i in range(frames+1)]
            time_accessor = accessor([(t,) for t in times],5126,"SCALAR",True)
            channels = []; samplers = []
            for bone,(name,_,_) in enumerate(BONES):
                values = [_quaternion(*rotations(clip,t).get(name,(0,0))) for t in times]
                samplers.append({"input":time_accessor,"output":accessor(values,5126,"VEC4"),"interpolation":"LINEAR"})
                channels.append({"sampler":len(samplers)-1,"target":{"node":bone,"path":"rotation"}})
            animations.append({"name":f"A_Roman{clip}","channels":channels,"samplers":samplers})
    colors = ((.65,.10,.08,1),(.5,.42,.24,1),(.65,.42,.27,1),(.16,.075,.03,1),(.65,.10,.08,1),(.75,.09,.06,1))
    materials = [{"name":name,"pbrMetallicRoughness":{"baseColorFactor":list(color),
                 "metallicFactor":.65 if name == "Metal" else 0,"roughnessFactor":.65}}
                 for name,color in zip(MATERIALS,colors)]
    doc = {"asset":{"version":"2.0","generator":"RomanEmpireGame original character recipe schema1"},
           "extras":{"RomanCharacterSchema":SCHEMA,"Profile":profile,"Copyright":"Roman Empire Game original proprietary source"},
           "scene":0,"scenes":[{"nodes":[0,len(BONES)]}],"nodes":nodes,"skins":[skin],
           "meshes":[{"name":mesh_name,"primitives":primitives}],"materials":materials,
           "buffers":[{"byteLength":len(data)}],"bufferViews":views,"accessors":accessors}
    if animations: doc["animations"] = animations
    return doc, bytes(data)


def glb(profile, include_animations=True):
    doc, binary = document(profile, include_animations)
    encoded = json.dumps(doc,sort_keys=True,separators=(",",":"),allow_nan=False).encode("utf-8")
    encoded += b" "*((-len(encoded))%4); binary += b"\0"*((-len(binary))%4)
    length = 12+8+len(encoded)+8+len(binary)
    return struct.pack("<III",0x46546c67,2,length)+struct.pack("<II",len(encoded),0x4e4f534a)+encoded+struct.pack("<II",len(binary),0x004e4942)+binary


def digest(profile, include_animations=True):
    return hashlib.sha256(glb(profile,include_animations)).hexdigest()
