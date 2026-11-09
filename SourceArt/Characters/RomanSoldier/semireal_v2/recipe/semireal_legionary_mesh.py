"""Pinned CC0 anatomy data for the versioned, non-integrated art candidate.

No MakeHuman application code is imported. This parser consumes only the OBJ
mesh, excludes helper geometry, and preserves the original body UV topology.
Coordinates returned are metres, Blender Z-up, forward -Y.
"""
import hashlib
import math
from pathlib import Path

BASE_SHA256 = "8e761e6624b8f54536409135d1636da63b32486a90d4897f84e121d144f6fb4c"
BASE_RELATIVE = "SourceArt/ThirdParty/MakeHuman/a8bc2d54/base.obj"
HEIGHT = 1.82
MORPH_PROFILE = (
    ("caucasian-male-young.target", "70e228ba7164737dae664454394536fc5935fa48d333c1a97d77e2dc6eacc5f5", .75),
    ("caucasian-male-old.target", "b0d2b1d58cba0358af59833740e70229265f7da1c20a11240f4123b7192cfd6d", .25),
    ("universal-male-young-maxmuscle-averageweight.target", "c171ff9cc95e96273beb3e3b1969a01988c2d4982ebc6fe04bf81cf72ee3b295", .35),
)


def fit_surface_offset(point, surface, normal, margin):
    """Fit garment vertices outside an actual nearest body surface, in metres."""
    if margin < 0 or not all(math.isfinite(v) for row in (point,surface,normal) for v in row):
        raise ValueError("Invalid garment fit data")
    length = math.sqrt(sum(v*v for v in normal))
    if length < 1e-9:
        raise ValueError("Degenerate garment-fit surface normal")
    axis = tuple(v/length for v in normal)
    signed = sum((a-b)*n for a,b,n in zip(point,surface,axis))
    return tuple(surface[i]+axis[i]*margin for i in range(3)) if signed < margin else tuple(point)


def parse_target(text, vertex_count):
    """Read numeric CC0 morph data only; never import MakeHuman application code."""
    result = {}
    for line in text.splitlines():
        fields = line.split()
        if not fields or fields[0].startswith("#"):
            continue
        if len(fields) != 4:
            raise ValueError("Invalid morph row")
        index, delta = int(fields[0]), tuple(float(v) for v in fields[1:])
        if not 0 <= index < vertex_count or index in result or not all(math.isfinite(v) for v in delta):
            raise ValueError("Invalid/duplicate morph index or nonfinite delta")
        result[index] = delta
    if not result:
        raise ValueError("Empty morph data")
    return result


def parse_obj(text):
    vertices, uvs, groups = [], [], {}
    current = "default"
    for line in text.splitlines():
        fields = line.split()
        if not fields or fields[0].startswith("#"):
            continue
        if fields[0] == "v":
            point = tuple(float(value) for value in fields[1:4])
            if len(point) != 3 or not all(math.isfinite(v) for v in point):
                raise ValueError("Invalid OBJ vertex")
            vertices.append(point)
        elif fields[0] == "vt":
            uv = tuple(float(value) for value in fields[1:3])
            if len(uv) != 2 or not all(math.isfinite(v) for v in uv):
                raise ValueError("Invalid OBJ UV")
            uvs.append(uv)
        elif fields[0] == "g":
            if len(fields) != 2:
                raise ValueError("Expected one explicit OBJ group")
            current = fields[1]
        elif fields[0] == "f":
            face = []
            for field in fields[1:]:
                parts = field.split("/")
                vi = int(parts[0])
                vi = vi - 1 if vi > 0 else len(vertices) + vi
                ti = int(parts[1]) if len(parts) > 1 and parts[1] else None
                ti = ti - 1 if ti is not None and ti > 0 else len(uvs) + ti if ti is not None else None
                if not 0 <= vi < len(vertices) or (ti is not None and not 0 <= ti < len(uvs)):
                    raise ValueError("OBJ index outside source arrays")
                face.append((vi, ti))
            if len(face) < 3:
                raise ValueError("Degenerate OBJ face")
            groups.setdefault(current, []).append(face)
    if not vertices or not groups.get("body"):
        raise ValueError("Missing original body geometry")
    return vertices, uvs, groups


def load_anatomy(repo_root, profile="base"):
    raw = (Path(repo_root) / BASE_RELATIVE).read_bytes()
    if hashlib.sha256(raw).hexdigest() != BASE_SHA256:
        raise ValueError("Anatomy source hash mismatch; no unreviewed source substitution")
    if b"explicitly released as CC0" not in raw[:1000]:
        raise ValueError("Missing source-specific CC0 statement")
    vertices, uvs, groups = parse_obj(raw.decode("utf-8"))
    morph_sources = []
    if profile == "roman_adult_v1":
        for name, expected, weight in MORPH_PROFILE:
            morph_raw = (Path(repo_root)/"SourceArt/ThirdParty/MakeHuman/a8bc2d54/macrodetails"/name).read_bytes()
            if hashlib.sha256(morph_raw).hexdigest() != expected or b"explicitly released as CC0" not in morph_raw[:1000]:
                raise ValueError("Unreviewed morph source/hash/license")
            for index, delta in parse_target(morph_raw.decode("utf-8"),len(vertices)).items():
                vertices[index] = tuple(a+weight*b for a,b in zip(vertices[index],delta))
            morph_sources.append({"file":name,"sha256":expected,"weight":weight})
    elif profile != "base":
        raise ValueError("Unknown anatomy profile")
    body = groups["body"]
    used = sorted({vi for face in body for vi, _ in face})
    minimum_y = min(vertices[vi][1] for vi in used)
    maximum_y = max(vertices[vi][1] for vi in used)
    scale = HEIGHT / (maximum_y - minimum_y)

    def transform(point):
        return point[0] * scale, -point[2] * scale, (point[1] - minimum_y) * scale

    remap = {original: index for index, original in enumerate(used)}
    joints = {}
    for name, faces in groups.items():
        if not name.startswith("joint-"):
            continue
        indices = {vi for face in faces for vi, _ in face}
        center = tuple(sum(vertices[vi][axis] for vi in indices) / len(indices) for axis in range(3))
        joints[name[6:]] = transform(center)
    body_uvs = []
    for face in body:
        if any(ti is None for _, ti in face):
            raise ValueError("Body face lacks UVs")
        body_uvs.append([uvs[ti] for _, ti in face])
    return {
        "vertices": [transform(vertices[vi]) for vi in used],
        "faces": [[remap[vi] for vi, _ in face] for face in body],
        "face_uvs": body_uvs,
        "joints": joints,
        "excluded_groups": sorted(name for name in groups if name != "body"),
        "source_sha256": BASE_SHA256,
        "profile": profile,
        "morph_sources": morph_sources,
    }


def joint_layout(anatomy):
    """Same 17 semantic names/parent hierarchy as the current game, new bind pose.

    This does not assert that old animation assets can be retargeted unchanged.
    The candidate exports its own clips for subsequent engine validation.
    """
    from roman_character_recipe import BONES
    joints = anatomy["joints"]
    positions = {
        "root": (0., 0., 0.), "pelvis": joints["pelvis"],
        "spine": joints["spine-3"], "chest": joints["spine-1"],
        "head": joints["head"],
    }
    # Preserve the game's spatial sign convention, rather than assuming that
    # the source's anatomical left is the game's negative-X arm.
    negative = "l" if joints["l-shoulder"][0] < 0 else "r"
    for side, source in (("l", negative), ("r", "r" if negative == "l" else "l")):
        for bone, helper in (("upper_arm", "shoulder"), ("forearm", "elbow"),
                             ("hand", "hand"), ("thigh", "upper-leg"),
                             ("calf", "knee"), ("foot", "ankle")):
            positions[f"{bone}_{side}"] = joints[f"{source}-{helper}"]
    return [(name, parent, positions[name]) for name, parent, _ in BONES]


def segment_distance(point, start, end):
    axis = tuple(b-a for a, b in zip(start, end))
    length2 = sum(value*value for value in axis)
    t = max(0., min(1., sum((p-a)*d for p, a, d in zip(point, start, axis)) / length2)) if length2 > 1e-12 else 0.
    return math.sqrt(sum((p-(a+t*d))**2 for p, a, d in zip(point, start, axis)))


def skin_weights(point, layout):
    if point[2] > 1.57:
        return [("head", 1.)]  # Local possession must be able to hide the whole head.
    points = {name: location for name, _, location in layout}
    segments = [("pelvis", "spine"), ("spine", "chest"), ("chest", "head")]
    for side in ("l", "r"):
        segments += [(f"upper_arm_{side}", f"forearm_{side}"), (f"forearm_{side}", f"hand_{side}"),
                     (f"thigh_{side}", f"calf_{side}"), (f"calf_{side}", f"foot_{side}")]
    ranked = [(segment_distance(point, points[name], points[end]), name) for name, end in segments]
    for name in ("head", "hand_l", "hand_r", "foot_l", "foot_r"):
        start = points[name]
        offset = (0., -.09, 0.) if name.startswith("foot") else (0., 0., .18) if name == "head" else (0., 0., -.07)
        ranked.append((segment_distance(point, start, tuple(a+b for a,b in zip(start,offset))), name))
    ranked.sort()
    chosen = ranked[:3]
    raw = [(name, 1. / (distance + .025)**4) for distance, name in chosen]
    total = sum(weight for _, weight in raw)
    return [(name, weight/total) for name, weight in raw]


def torso_weights(point, layout):
    """Shared torso-layer weights: sleeves must not pull the shirt's waist."""
    levels=[(name,next(p[2] for n,_,p in layout if n==name)) for name in ("pelvis","spine","chest")]
    z=point[2]
    if z<=levels[0][1]: return [(levels[0][0],1.)]
    for (a,low),(b,high) in zip(levels,levels[1:]):
        if high<=low: raise ValueError("Torso joints are not ordered")
        if z<high:
            t=(z-low)/(high-low)
            return [(a,1-t),(b,t)]
    return [(levels[-1][0],1.)]
