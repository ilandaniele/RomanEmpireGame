"""Inspect actual exported bytes; this is not Unreal import or visual approval."""
import argparse
import hashlib
import json
import math
from pathlib import Path
import struct

from roman_character_recipe import BONES, CLIPS

COMPONENTS = {5120:("b",1,127),5121:("B",1,255),5122:("h",2,32767),
              5123:("H",2,65535),5125:("I",4,4294967295),5126:("f",4,None)}
WIDTHS = {"SCALAR":1,"VEC2":2,"VEC3":3,"VEC4":4,"MAT4":16}


def inspect(path):
    raw = Path(path).read_bytes()
    if len(raw) < 20 or struct.unpack_from("<III",raw) != (0x46546C67,2,len(raw)):
        raise ValueError("Invalid GLB header/size")
    chunks,offset = {},12
    while offset < len(raw):
        size,kind = struct.unpack_from("<II",raw,offset)
        offset += 8
        if offset+size > len(raw) or kind in chunks:
            raise ValueError("Invalid/duplicate GLB chunk")
        chunks[kind] = raw[offset:offset+size]
        offset += size
    doc = json.loads(chunks[0x4E4F534A].decode("utf-8"))
    binary = chunks[0x004E4942]
    if doc["asset"]["version"] != "2.0":
        raise ValueError("Not glTF2")

    def accessor(index):
        item = doc["accessors"][index]
        view = doc["bufferViews"][item["bufferView"]]
        if view.get("buffer",0) != 0:
            raise ValueError("Unexpected external buffer")
        code,size,maximum = COMPONENTS[item["componentType"]]
        width = WIDTHS[item["type"]]
        stride = view.get("byteStride",width*size)
        base = view.get("byteOffset",0)+item.get("byteOffset",0)
        values = []
        for i in range(item["count"]):
            position = base+i*stride
            if position+width*size > view.get("byteOffset",0)+view["byteLength"]:
                raise ValueError("Accessor exceeds its view")
            value = struct.unpack_from("<"+code*width,binary,position)
            if item.get("normalized") and maximum:
                value = tuple(max(-1.,v/maximum) for v in value)
            if not all(math.isfinite(v) for v in value):
                raise ValueError("Nonfinite exported attribute")
            values.append(value)
        return values

    if len(doc.get("skins",[])) != 1:
        raise ValueError("Expected exactly one real skin")
    skin = doc["skins"][0]
    names = [doc["nodes"][i]["name"] for i in skin["joints"]]
    if len(names) != 17 or set(names) != {name for name,_,_ in BONES}:
        raise ValueError("Actual exported joint names/count differ")
    matrices = accessor(skin["inverseBindMatrices"])
    if len(matrices) != len(names):
        raise ValueError("Missing actual inverse bind matrices")
    nodes_by_name = {node.get("name"):i for i,node in enumerate(doc["nodes"])}
    for name,parent,_ in BONES:
        if parent >= 0 and nodes_by_name[name] not in doc["nodes"][nodes_by_name[BONES[parent][0]]].get("children",[]):
            raise ValueError("Exported bone hierarchy differs")

    positions,triangles,vertices = [],0,0
    topology = []
    for source in doc.get("meshes",[]):
        for primitive in source["primitives"]:
            attrs = primitive["attributes"]
            if "COLOR_0" in attrs:
                raise ValueError("Baked material would receive a second vertex-color tint")
            required = {"POSITION","NORMAL","TEXCOORD_0","JOINTS_0","WEIGHTS_0"}
            if not required.issubset(attrs):
                raise ValueError("Missing real mesh/UV/normal/skin attributes")
            rows = {key:accessor(attrs[key]) for key in required}
            count = len(rows["POSITION"])
            if any(len(values) != count for values in rows.values()):
                raise ValueError("Mismatched attribute lengths")
            for joint,weights in zip(rows["JOINTS_0"],rows["WEIGHTS_0"]):
                if any(not 0 <= j < len(names) for j in joint):
                    raise ValueError("Invalid joint index")
                if any(not 0 <= w <= 1 for w in weights) or abs(sum(weights)-1) > 1e-4:
                    raise ValueError("Invalid exported weights")
            indices = accessor(primitive["indices"])
            if primitive.get("mode",4) != 4 or len(indices)%3 or any(not 0 <= i[0] < count for i in indices):
                raise ValueError("Invalid triangle indices")
            # Report, do not silently remove or accept a different source mesh.
            # Pole/seam faces can have separate indices at coincident positions.
            # This is evidence for reconciling native import counts, not an import gate.
            coincident, zero_area = 0,0
            for offset in range(0,len(indices),3):
                a,b,c = (rows["POSITION"][indices[offset+i][0]] for i in range(3))
                ab,ac = tuple(b[i]-a[i] for i in range(3)),tuple(c[i]-a[i] for i in range(3))
                cross = (ab[1]*ac[2]-ab[2]*ac[1],ab[2]*ac[0]-ab[0]*ac[2],ab[0]*ac[1]-ab[1]*ac[0])
                zero_area += sum(v*v for v in cross) <= 1.e-28
                coincident += any(all(abs(x[i]-y[i]) <= 2.e-7 for i in range(3)) for x,y in ((a,b),(a,c),(b,c)))
            topology.append({"mesh":source.get("name"),"material":doc["materials"][primitive["material"]].get("name"),
                             "triangles":len(indices)//3,"coincident_corner_triangles_2e_7_m":coincident,
                             "zero_area_triangles_1e_28_m4":zero_area})
            positions.extend(rows["POSITION"])
            vertices += count
            triangles += len(indices)//3
    if not positions or triangles < 1000:
        raise ValueError("No substantive mesh geometry")
    minimum = [min(p[i] for p in positions) for i in range(3)]
    maximum = [max(p[i] for p in positions) for i in range(3)]
    if not 1.75 <= maximum[1]-minimum[1] <= 2.05:
        raise ValueError("Unexpected source-metre height")

    clips = {}
    for animation in doc.get("animations",[]):
        name = animation["name"]
        if name not in CLIPS or name in clips:
            raise ValueError("Unexpected or duplicate exported clip: "+name)
        maximum_time,minimum_time,moving = 0.,float("inf"),False
        for sampler in animation["samplers"]:
            times,values = accessor(sampler["input"]),accessor(sampler["output"])
            if len(times) != len(values) or len(times) < 2:
                raise ValueError("Clip lacks actual sampled keys")
            if any(b[0] <= a[0] for a,b in zip(times,times[1:])):
                raise ValueError("Nonmonotonic animation keys")
            minimum_time = min(minimum_time,times[0][0])
            maximum_time = max(maximum_time,times[-1][0])
            moving |= any(any(abs(a-b) > 1e-5 for a,b in zip(values[0],value)) for value in values[1:])
        duration = maximum_time-minimum_time
        if not moving or abs(duration-CLIPS[name]) > 1/30.+1e-5:
            raise ValueError("Wrong or static actual clip: "+name)
        clips[name] = {"duration":duration,"samplers":len(animation["samplers"])}
    if set(clips) != set(CLIPS):
        raise ValueError("Missing exported clips: "+str(set(CLIPS)-set(clips)))

    images = []
    for image in doc.get("images",[]):
        if "bufferView" not in image or image.get("mimeType") != "image/png":
            raise ValueError("Expected embedded baked PNG")
        view = doc["bufferViews"][image["bufferView"]]
        blob = binary[view["byteOffset"]:view["byteOffset"]+view["byteLength"]]
        if blob[:8] != b"\x89PNG\r\n\x1a\n":
            raise ValueError("Invalid embedded PNG bytes")
        width,height = struct.unpack_from(">II",blob,16)
        if min(width,height) < 512:
            raise ValueError("Undersized actual baked map")
        images.append({"name":image.get("name"),"width":width,"height":height,"sha256":hashlib.sha256(blob).hexdigest()})
    if len(images) < 3:
        raise ValueError("Missing base-color, material or normal maps")
    for mat in doc.get("materials",[]):
        pbr = mat["pbrMetallicRoughness"]
        if not {"baseColorTexture","metallicRoughnessTexture"}.issubset(pbr) or "normalTexture" not in mat:
            raise ValueError("Material missing actual baked texture bindings")
    return {"status":"pass","path":str(Path(path).resolve()),"sha256":hashlib.sha256(raw).hexdigest(),
            "bytes":len(raw),"exported_vertices":vertices,"triangles":triangles,"topology":topology,
            "joint_names":names,"bounds_meters_gltf_y_up":{"min":minimum,"max":maximum},
            "clips":clips,"images":images,"material_count":len(doc["materials"]),
            "unreal_import_verified":False,"visual_approved":False,"performance_approved":False}


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("path")
    parser.add_argument("--report")
    args = parser.parse_args()
    result = inspect(args.path)
    if args.report:
        with Path(args.report).open("x",encoding="utf-8") as target:
            json.dump(result,target,indent=2)
    print(json.dumps(result,indent=2))
