# Roman camp — private source proposal01

Status: source verified, owner approval and Unreal integration PENDING.
No gameplay/collision/navigation changes. NOT approved production art.

Original environment geometry/material authoring: GPT, Blender5.2.2LTS,
Tools/create_semireal_camp.py, deterministic seed1802. The imagegen skill and
built-in image_gen supplied the original soil diffuse input; exact prompt,
actual1254x1254 resolution, SHA and terms/rights limits are recorded in
[source review](../../../../M6_SEMIREAL_CAMP_SOURCE_REVIEW.md).
No downloaded environment asset pack or new external code/service.

## Deliverables

- SM_CampGround.glb:1152triangles,12x10m earth patch.
- SM_CampWall.glb:3704triangles,9.25m wide timber palisade (~2.60m high).
- SM_CampBarracks.glb:46602triangles,one limewashed timber/terracotta building.
  Wall footprint4.6x3.2m; roof-inclusive bounds4.93x4.077x3.7335m.
- RomanCamp-Proposal01.glb:all3meshes,51458triangles; no character/rig/animations.
- RomanCamp-Proposal01.blend:editable mesh/UV/PBR and SOURCE_ONLY author shaders,
  lighting/cameras plus existing semireal_v2 legionary ONLY for scale reference.
  Images packed; reference excluded from the GLBs. Existing character provenance
  remains per ASSET_INVENTORY.md, not a new environment rights claim.
- BaseColor/Roughness/Metallic/Normal.png:shared2048atlas bakes. Soil-Input01.png:
  actual1254generated input. Baking to2048does not add original source detail.
- Camp-Overview.png/Camp-HumanScale.png:1600x1000BLENDER source previews only.
- source-manifest.json:SHA256/byte sizes for generated files and source script.

All3meshes intentionally share the original kit's world origin/pivot. Coordinates
are METERS; assemble with the same transform. Individual exports are not
recentered production modular meshes. Verify100cm/m and bounds in Unreal rather
than guessing import scale or swapping onto a gameplay actor with a hidden offset.
No collision mesh, nav modifier, LODs, Nanite/performance approval or interior.

glTF embeds3PNG textures (basecolor, tangent normal, packed metallic/roughness),
6used materials total. Individual exports use1/2/5material slots. Source BLEND
retains6slots per mesh; unused slots are excluded by GLB export. Normal/roughness/
metallic are data, not sRGB. Check actual importer normal convention/channel
mapping; do not apply an unverified green-channel flip or inherit legacy tint.

Installed glTF exporter warns when packing multiple texture sockets even though
source image nodes all use identical REPEAT/Linear samplers. Fresh Readback03
verifies saved BLEND samplers, actual embedded textures/UV/triangles/dimensions,
all4GLB reimports and renders combined exported geometry. Warning retained,
not suppressed. No sampler/appearance approval is inferred merely from exit0.

Review, failure history, exact generation prompt and proof paths:
M6_SEMIREAL_CAMP_SOURCE_REVIEW.md. Independent readback remains in ignored
Artifacts/VQ02_CampSource1/Readback03; copied source files are byte-identical to
Attempt02. No blanket CC0/commercial/redistribution clearance is asserted;
applicable generated-content/tool terms and owner rights review remain open.

Next: import ONLY into the existing private GamePreview, fresh namespace,
read back actual StaticMesh/material/texture dimensions, verify original assets
unchanged, and inspect with actual RTS/possession cameras alongside the legionary.
Do not replace the map or change functional building/wall collision/nav without
separate checks. Native source rendering is not the Unreal/player/owner gate.
