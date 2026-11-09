"""Read the isolated import and render a real UE reference-pose preview.

No game assets or owner session writes. This is not gameplay/AnimBP/player QA.
Use -AllowCommandletRendering -d3d12, never -NullRHI.
"""
import hashlib
import json
import math
import argparse
from pathlib import Path
import re
import sys
import traceback

import unreal

ROOT = Path(__file__).resolve().parent.parent
PROJECT = Path(unreal.Paths.project_dir()).resolve()
if PROJECT != (ROOT / "Artifacts/VQ01_Semireal1/UEProbe").resolve():
    raise RuntimeError("Refusing renderer outside exact art probe")
parser = argparse.ArgumentParser()
parser.add_argument("--attempt",required=True)
parser.add_argument("--import-report",default="import06-1-report.json")
args = parser.parse_args(sys.argv[1:])
if not re.fullmatch(r"render[0-9]+-[0-9]+",args.attempt):
    raise RuntimeError("Invalid render attempt identifier")
OUT = PROJECT / args.attempt
if OUT.exists():
    raise RuntimeError("Preview attempt exists; do not replace evidence")
OUT.mkdir()
receipt = {"status": "running", "game_content_changed": False, "visual_approved": False,
           "script_sha256":hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
           "player_verified": False, "preview": "UE D3D12 reference pose, not gameplay"}
actors = []
try:
    import_report = (PROJECT / args.import_report).resolve()
    import_report.relative_to(PROJECT)
    report = json.loads(import_report.read_text(encoding="utf-8"))
    if report["status"] != "pass":
        raise RuntimeError("Import has not passed")
    mesh = unreal.load_asset(report["mesh"])
    if not isinstance(mesh, unreal.SkeletalMesh):
        raise RuntimeError("Imported mesh missing")
    editor = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
    world = editor.get_editor_world()
    subsystem = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)

    def spawn(cls, location, rotation=unreal.Rotator()):
        actor = subsystem.spawn_actor_from_class(cls, location, rotation, transient=True)
        if actor is None:
            raise RuntimeError("Owned preview actor spawn failed")
        actors.append(actor)
        return actor

    subject = spawn(unreal.SkeletalMeshActor, unreal.Vector())
    component = subject.get_component_by_class(unreal.SkeletalMeshComponent)
    component.set_skeletal_mesh_asset(mesh)
    component.prestream_textures(60.,True,0)
    for slot in mesh.get_editor_property("materials"):
        material = slot.get_editor_property("material_interface")
        if isinstance(material, unreal.Material):
            errors = unreal.MaterialEditingLibrary.recompile_material(material)
            if errors:
                raise RuntimeError("Actual material shader errors: " + str(errors))
    floor = spawn(unreal.StaticMeshActor, unreal.Vector(0,0,-2))
    floor_component = floor.get_component_by_class(unreal.StaticMeshComponent)
    floor_component.set_static_mesh(unreal.load_asset("/Engine/BasicShapes/Plane"))
    floor.set_actor_scale3d(unreal.Vector(12,12,1))
    for index,(rotation, intensity) in enumerate(((unreal.Rotator(-40,-45,0),120.),
                                                  (unreal.Rotator(-20,130,0),120.),
                                                  (unreal.Rotator(-70,270,0),80.))):
        light = spawn(unreal.DirectionalLight, unreal.Vector(0,0,300),rotation)
        light_component = light.get_component_by_class(unreal.DirectionalLightComponent)
        light_component.set_mobility(unreal.ComponentMobility.MOVABLE)
        light_component.set_intensity(intensity)
        if index:
            light_component.set_cast_shadows(False)
    sky = spawn(unreal.SkyLight,unreal.Vector(0,0,300))
    sky_component = sky.get_component_by_class(unreal.SkyLightComponent)
    sky_component.set_mobility(unreal.ComponentMobility.MOVABLE)
    sky_component.set_editor_property("source_type",unreal.SkyLightSourceType.SLS_SPECIFIED_CUBEMAP)
    cube = unreal.load_asset("/Engine/MapTemplates/Sky/DaylightAmbientCubemap")
    if not isinstance(cube,unreal.TextureCube):
        raise RuntimeError("Expected engine preview cubemap unavailable")
    sky_component.set_cubemap(cube)
    sky_component.set_intensity(1.)
    sky_component.recapture_sky()
    capture = spawn(unreal.SceneCapture2D, unreal.Vector())
    camera = capture.get_component_by_class(unreal.SceneCaptureComponent2D)
    target = unreal.RenderingLibrary.create_render_target2d(world,1024,1024,unreal.TextureRenderTargetFormat.RTF_RGBA8)
    camera.set_editor_property("texture_target",target)
    camera.set_editor_property("capture_source",unreal.SceneCaptureSource.SCS_FINAL_COLOR_LDR)
    camera.set_editor_property("capture_every_frame",False)
    camera.set_editor_property("capture_on_movement",False)
    camera.set_editor_property("projection_type",unreal.CameraProjectionMode.ORTHOGRAPHIC)
    camera.set_editor_property("ortho_width",280.)
    settings = camera.get_editor_property("post_process_settings")
    for key,value in (("override_auto_exposure_method",True),("auto_exposure_method",unreal.AutoExposureMethod.AEM_MANUAL),
                      ("override_auto_exposure_apply_physical_camera_exposure",True),
                      ("auto_exposure_apply_physical_camera_exposure",False),("override_auto_exposure_bias",True),
                      ("auto_exposure_bias",0.),("override_bloom_intensity",True),("bloom_intensity",0.)):
        settings.set_editor_property(key,value)
    camera.set_editor_property("post_process_settings",settings)
    camera.set_editor_property("post_process_blend_weight",1.)
    bounds = mesh.get_imported_bounds()
    center = bounds.origin
    images = []
    receipt["images"] = images
    for name,position in (("Back",unreal.Vector(300,-200,170)),("Front",unreal.Vector(-300,200,170))):
        capture.set_actor_location(position,False,False)
        capture.set_actor_rotation(unreal.MathLibrary.find_look_at_rotation(position,center),False)
        camera.capture_scene()
        output = OUT / (name+".png")
        unreal.RenderingLibrary.export_render_target(world,target,str(OUT),output.name)
        raw = output.read_bytes()
        if len(raw)<10000 or raw[:8]!=b"\x89PNG\r\n\x1a\n":
            raise RuntimeError("Actual GPU preview missing/empty")
        # File existence is not visibility. First attempt was nearly black;
        # retain it as rejected and gate actual foreground pixels here.
        pixels = unreal.RenderingLibrary.read_render_target_raw_pixel_area(world,target,128,128,895,895)
        # UE5.8 KismetRenderingLibrary.cpp returns byte-range FLinearColor for
        # PF_B8G8R8A8 here, even with bNormalize. Convert the readback units;
        # do not change any exported pixel or weaken the visibility thresholds.
        samples = [(p.r/255.,p.g/255.,p.b/255.) for p in list(pixels)[::8]]
        bright = sum(max(p)>.05 for p in samples)/len(samples)
        red = sum(r>.05 and r>g*1.25 and r>b*1.2 for r,g,b in samples)/len(samples)
        clipped = sum(min(p)>.98 for p in samples)/len(samples)
        images.append({"path":str(output),"bytes":len(raw),"sha256":hashlib.sha256(raw).hexdigest(),
                       "visibility":{"bright_fraction":bright,"red_foreground_fraction":red,"white_clip_fraction":clipped}})
        if bright<.10 or red<.005 or clipped>.05:
            raise RuntimeError("Actual preview too dark/clipped or missing expected red soldier foreground")
    # Evaluate the actual imported sequence data through UE, independent of Blender.
    options = unreal.AnimPoseEvaluationOptions()
    options.set_editor_property("optional_skeletal_mesh",mesh)
    clips = {}
    for name,clip in report["clips"].items():
        sequence = unreal.load_asset(clip["path"])
        evaluations={}
        for label,kind in (("Raw",unreal.AnimDataEvalType.RAW),("Compressed",unreal.AnimDataEvalType.COMPRESSED)):
            options.set_editor_property("evaluation_type",kind)
            samples=[]
            for phase in (0,.25,.5,.75,1.):
                pose=unreal.AnimPoseExtensions.get_anim_pose_at_time(sequence,clip["seconds"]*phase,options)
                if not unreal.AnimPoseExtensions.is_valid(pose):
                    raise RuntimeError("UE could not evaluate actual imported pose: "+name+"/"+label)
                names=[str(n) for n in unreal.AnimPoseExtensions.get_bone_names(pose)]
                if names!=report["bones"]:
                    raise RuntimeError("UE evaluated pose bones differ: "+name)
                transforms=[]
                for bone in names:
                    transform=unreal.AnimPoseExtensions.get_bone_pose(pose,bone,unreal.AnimPoseSpaces.WORLD)
                    p,q=transform.translation,transform.rotation
                    values=(p.x,p.y,p.z,q.x,q.y,q.z,q.w)
                    if not all(math.isfinite(v) for v in values):
                        raise RuntimeError("Nonfinite evaluated bone: "+name+"/"+bone)
                    transforms.append(values)
                samples.append(transforms)
            movement=max(math.dist(a[:3],b[:3]) for sample in samples[1:] for a,b in zip(samples[0],sample))
            rotation=max(2*math.acos(min(1.,abs(sum(x*y for x,y in zip(a[3:],b[3:])))))
                         for sample in samples[1:] for a,b in zip(samples[0],sample))
            if movement<.05 and rotation<math.radians(.1):
                raise RuntimeError("Actual UE clip is static: "+name+"/"+label)
            evaluations[label]={"phases":[0,.25,.5,.75,1.],"bones":17,
                                "max_world_translation_cm":movement,"max_world_rotation_deg":math.degrees(rotation)}
        clips[name]=evaluations
    receipt.update(status="pass",images=images,actual_UE_evaluated_clips=clips,
                   source_sha256=report["source_sha256"])
except Exception:
    receipt.update(status="fail",error=traceback.format_exc())
    raise
finally:
    for actor in reversed(actors):
        unreal.get_editor_subsystem(unreal.EditorActorSubsystem).destroy_actor(actor)
    with (OUT/"receipt.json").open("x",encoding="utf-8") as stream:
        json.dump(receipt,stream,indent=2)
    unreal.log("SEMIREAL_GPU_PREVIEW "+receipt["status"])
