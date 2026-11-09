"""Reimport an actual GLB and evaluate exported animation/skin, not author state."""
import argparse
import hashlib
import json
import math
from pathlib import Path
import sys

import bpy
from mathutils import Vector

ROOT = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT / "Tools"))
from roman_character_recipe import CLIPS

parser = argparse.ArgumentParser()
parser.add_argument("--source", required=True)
parser.add_argument("--report", required=True)
parser.add_argument("--gallery")
parser.add_argument("--portrait",action="store_true")
args = parser.parse_args(sys.argv[sys.argv.index("--") + 1:])
source, report = Path(args.source).resolve(), Path(args.report).resolve()
source.relative_to(ROOT / "Artifacts/VQ01_Semireal1")
report.relative_to(ROOT / "Artifacts/VQ01_Semireal1")
if report.exists():
    raise RuntimeError("Reimport report exists; refusing overwrite")
gallery = Path(args.gallery).resolve() if args.gallery else None
if gallery:
    gallery.relative_to(ROOT / "Artifacts/VQ01_Semireal1")
    if gallery.exists():
        raise RuntimeError("Gallery already exists")
bpy.ops.object.select_all(action="SELECT")
bpy.ops.object.delete(use_global=False)
bpy.ops.import_scene.gltf(filepath=str(source))
rigs = [o for o in bpy.context.scene.objects if o.type == "ARMATURE"]
# Blender's glTF importer creates an Icosphere bone-display custom shape.
# Exclude only shapes actually referenced by a pose bone, not arbitrary meshes.
custom_shapes = {b.custom_shape for r in rigs for b in r.pose.bones if b.custom_shape}
meshes = [o for o in bpy.context.scene.objects if o.type == "MESH" and o not in custom_shapes]
if len(rigs) != 1 or len(meshes) != 1:
    raise RuntimeError("Actual GLB did not reimport as one mesh/rig: " +
                       str([(o.name,o.type) for o in bpy.context.scene.objects]))
rig, mesh = rigs[0], meshes[0]
if len(rig.data.bones) != 17:
    raise RuntimeError("Reimported bone count differs")
tracks = list(rig.animation_data.nla_tracks)
if {t.name for t in tracks} != set(CLIPS):
    raise RuntimeError("Actual reimported NLA clips differ: " + str([t.name for t in tracks]))
rig.animation_data.action = None
scene = bpy.context.scene
scene.render.fps = 30


def positions(frame):
    scene.frame_set(frame)
    bpy.context.view_layer.update()
    evaluated = mesh.evaluated_get(bpy.context.evaluated_depsgraph_get())
    result = [tuple(evaluated.matrix_world @ vertex.co) for vertex in evaluated.data.vertices]
    if not result or not all(math.isfinite(v) for point in result for v in point):
        raise RuntimeError("Empty/nonfinite actual deformed geometry")
    if any(max(p[i] for p in result) - min(p[i] for p in result) > 3.0 for i in range(3)):
        raise RuntimeError("Exploded actual deformed mesh")
    return result


clips = {}
snapshots = []
for track in tracks:
    for other in tracks:
        other.mute = other != track
    duration = CLIPS[track.name]
    first = positions(0)
    maximum = 0.
    for phase in (.25, .5, .75, 1.):
        posed = positions(round(duration * 30 * phase))
        maximum = max(maximum, max(math.dist(a, b) for a, b in zip(first, posed)))
    if maximum < .005:
        raise RuntimeError("Exported clip does not deform mesh: " + track.name)
    clips[track.name] = {"max_vertex_movement_m": maximum, "sampled_phases": [0, .25, .5, .75, 1.]}
    if gallery:
        positions(round(duration * 30 * (1. if track.name == "Death" else .25 if track.name == "Walk" else .5)))
        data = bpy.data.meshes.new_from_object(mesh.evaluated_get(bpy.context.evaluated_depsgraph_get()))
        snapshot = bpy.data.objects.new("ActualExport_" + track.name, data)
        scene.collection.objects.link(snapshot)
        snapshot.location.x = len(snapshots) * 2.
        snapshots.append(snapshot)
if gallery:
    mesh.hide_render = True
    for custom in custom_shapes:
        custom.hide_render = True
    scene.render.engine = "CYCLES"; scene.cycles.device = "CPU"; scene.cycles.samples = 24
    scene.cycles.use_denoising = True
    scene.world.color = (.22,.22,.22)
    bpy.ops.mesh.primitive_plane_add(size=200, location=(0,0,-.02))
    floor = bpy.context.object
    mat = bpy.data.materials.new("ProbeFloor"); mat.diffuse_color = (.15,.15,.15,1)
    floor.data.materials.append(mat)
    for location, power, size in (((4,-3,6),1200,8),((0,2,4),850,6)):
        bpy.ops.object.light_add(type="AREA", location=location)
        light = bpy.context.object; light.data.energy = power; light.data.size = size
        light.rotation_euler = (Vector((4,0,1))-light.location).to_track_quat("-Z","Y").to_euler()
    bpy.ops.object.camera_add(location=(4,-10,3.5))
    camera = bpy.context.object; scene.camera = camera
    camera.rotation_euler = (Vector((4,0,.85))-camera.location).to_track_quat("-Z","Y").to_euler()
    camera.data.type = "ORTHO"; camera.data.ortho_scale = 11.5
    scene.render.resolution_x = 2048; scene.render.resolution_y = 512
    if args.portrait:
        idle = next(s for s in snapshots if s.name.endswith("Idle"))
        for other in snapshots:
            other.hide_render = other != idle
        camera.location = idle.location + Vector((2.4,-4.5,2.0))
        camera.rotation_euler = (idle.location+Vector((0,-.05,.94))-camera.location).to_track_quat("-Z","Y").to_euler()
        camera.data.ortho_scale = 2.35
        scene.render.resolution_x = 1024; scene.render.resolution_y = 1024
    scene.render.resolution_percentage = 100; scene.render.filepath = str(gallery)
    bpy.ops.render.render(write_still=True)
result = {"status": "pass", "source": str(source), "source_sha256": hashlib.sha256(source.read_bytes()).hexdigest(),
          "actual_reimported_bones": 17, "clips": clips, "visual_approved": False,
          "unreal_player_verified": False}
if gallery:
    result["native_blender_gallery"] = {"path": str(gallery), "left_to_right": [s.name for s in snapshots],
                                         "sha256": hashlib.sha256(gallery.read_bytes()).hexdigest()}
with report.open("x", encoding="utf-8") as stream:
    json.dump(result, stream, indent=2)
print("SEMIREAL_REIMPORT_PASS " + str(report), flush=True)
