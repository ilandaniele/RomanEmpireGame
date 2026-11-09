"""Bake a source-only finger grip; export still uses the game's 17-bone rig.

Temporary finger bones use the CC0 anatomical helpers and are deleted after
deformation. No MakeHuman application code, runtime finger authority or rig swap.
"""
import math
import bpy
from mathutils import Vector, Quaternion
from semireal_legionary_mesh import segment_distance


def bake_closed_grip(skin, anatomy):
    joints = anatomy["joints"]
    side = "l" if joints["l-hand"][0] > 0 else "r"
    wrist = Vector(joints[side+"-hand"])
    chains = {f:[Vector(joints[f"{side}-finger-{f}-{i}"]) for i in range(1,5)] for f in range(1,6)}
    across = (chains[5][0]-chains[2][0]).normalized()
    palm_tip = sum((chains[f][0] for f in range(2,6)),Vector())/4
    palm_direction = (palm_tip-wrist).normalized()
    data = bpy.data.armatures.new("SourceOnlyGrip")
    rig = bpy.data.objects.new("SourceOnlyGrip",data)
    bpy.context.collection.objects.link(rig)
    bpy.ops.object.select_all(action="DESELECT")
    rig.select_set(True); bpy.context.view_layer.objects.active=rig
    bpy.ops.object.mode_set(mode="EDIT")
    root = data.edit_bones.new("_GripStatic")
    root.head=(0,0,0); root.tail=(0,0,.01)
    palm = data.edit_bones.new("_GripPalm")
    palm.head=wrist; palm.tail=palm_tip; palm.parent=root
    definitions=[]
    for finger,points in chains.items():
        parent=palm
        for i in range(3):
            name=f"_GripFinger{finger}_{i}"
            bone=data.edit_bones.new(name)
            bone.head=points[i]; bone.tail=points[i+1]; bone.parent=parent
            bone.align_roll(across)
            definitions.append((name,points[i],points[i+1]))
            parent=bone
    bpy.ops.object.mode_set(mode="OBJECT")
    groups={name:skin.vertex_groups.new(name=name) for name in ["_GripStatic","_GripPalm"]+[d[0] for d in definitions]}
    for vertex in skin.data.vertices:
        point=vertex.co
        if point.x <= wrist.x-.15 or point.z > wrist.z+.06:
            continue
        progress=max(0.,min(1.,(point-wrist).dot(palm_direction)/.055))
        if not progress:
            continue
        ranked=[(segment_distance(point,a,b),name) for name,a,b in definitions]
        ranked.sort()
        chosen=ranked[:2]+[(segment_distance(point,wrist,palm_tip),"_GripPalm")]
        weights=[(name,1./(distance+.009)**4) for distance,name in chosen]
        total=sum(w for _,w in weights)
        for name,weight in weights:
            groups[name].add([vertex.index],progress*weight/total,"REPLACE")
        if progress<1.:
            groups["_GripStatic"].add([vertex.index],1.-progress,"REPLACE")
    # Align the closed knuckle line with the sword's vertical hilt. Blend at the
    # wrist in source geometry, without changing gameplay hand bone/bind names.
    desired=across.rotation_difference(Vector((0,0,-1)))
    palm_pose=rig.pose.bones["_GripPalm"]
    basis=palm_pose.bone.matrix_local.to_quaternion()
    palm_pose.rotation_mode="QUATERNION"
    palm_pose.rotation_quaternion=basis.inverted() @ desired @ basis
    for finger in chains:
        angles=(-25,-30,-15) if finger==1 else (60,70,35)
        for i,angle in enumerate(angles):
            bone=rig.pose.bones[f"_GripFinger{finger}_{i}"]
            bone.rotation_mode="QUATERNION"
            bone.rotation_quaternion=Quaternion((0,0,1),math.radians(angle))
    bpy.context.view_layer.update()
    centers=[]
    for finger in range(2,6):
        a=rig.pose.bones[f"_GripFinger{finger}_0"].head
        b=rig.pose.bones[f"_GripFinger{finger}_2"].tail
        centers.append((a+b)/2)
    grip=sum(centers,Vector())/len(centers)
    bpy.ops.object.select_all(action="DESELECT")
    skin.select_set(True); bpy.context.view_layer.objects.active=skin
    modifier=skin.modifiers.new("BakedSourceGrip","ARMATURE"); modifier.object=rig
    bpy.ops.object.modifier_apply(modifier=modifier.name)
    for name in groups:
        skin.vertex_groups.remove(skin.vertex_groups[name])
    bpy.data.objects.remove(rig,do_unlink=True)
    bpy.data.armatures.remove(data)
    return tuple(grip)
