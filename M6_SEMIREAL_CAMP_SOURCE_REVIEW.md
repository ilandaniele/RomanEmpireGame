# VQ-02: private Roman camp source proposal

2026-10-20. G-VQ02_CampSource1. Source-only; NOT integrated into Unreal or approved
by the owner. Scope: one12x10m earth patch, one~9.25m timber palisade, ONE small
limewashed timber/terracotta barrack. Proposed permanent-camp visual language,
not a validated historical reconstruction. No other faction/map expansion.

## Source and provenance

Original geometry by GPT through Blender5.2.2LTS (portable toolchain already
verified; no download/install). Tools/create_semireal_camp.py uses deterministic
seed1802 and native primitives/mesh construction, actual beveled plank/stone/
tile geometry, UV unwrap, procedural wood/plaster/clay/stone/iron and Cycles PBR
atlas baking. No purchased/downloaded environment geometry, texture pack or
third-party generator. No engine/gameplay/config/collision/navigation edits.

Soil-Input01.png was generated with built-in image_gen using the imagegen skill,
no reference images/APIkey/CLI fallback. Original preserved at:
C:/Users/Ilan/.codex/generated_images/01a06958-0725-7690-a93c-740fd59fc463/
exec-ccc0fe7d-d3c6-4bdd-80b9-08ab3a227420.png.
Copied byte-identically to the attempt. Actual PNG1254x1254,4445380bytes,
SHA25628AB6DF00B0A75C85F65116A400FF662349CAA58548381D0DFB94463FB4124AF.
Prompt requested2048 but tool returned1254: do not claim native2K generation.
It supplies diffuse color only; native shader baking yields the2048atlas and
procedural roughness/metallic/normal maps. Upsampling adds no source detail.
No seamless-tiling guarantee tested; one bounded ground UV patch, no checker.

Existing semireal_v2 legionary GLB is imported ONLY as a source-render scale
reference (LOD0SHA3157ac079a631d6fa2a317d550b0b3aeb9a56be9d752d423c478f7d50e0211f6).
Character/skeleton/animations are excluded from ALL camp GLB exports. The BLEND
preview includes this existing reference; its MakeHumanCC0/source/rights lineage
remains per ASSET_INVENTORY.md, not a new environment download.

Generated/authorial status is not blanket commercial-use or redistribution
clearance. Applicable tool terms, owner rights review and final notices remain
open; no invented CC0 license for generated work. No external service connection.

## Generation prompt (built-in tool, exact)

```text
Use case: photorealistic-natural. Asset type: original diffuse BASE COLOR texture for the ground of a semi-realistic Roman military camp videogame, not a scene render. Primary request: a single square 2048x2048 texture of compacted dry Mediterranean earth, warm muted tan-ochre soil, fine irregular gravel and tiny worn pale limestone fragments, sparse very subtle straw fibers. Perfect orthographic overhead, uniform neutral flat diffuse illumination, no directional light, no cast shadows, no ambient-occlusion darkening, no highlights, no perspective, no large stones or identifiable props, no text or borders. Small-scale natural color variation, restrained contrast so soldiers remain readable. Seamless tileable edge continuity horizontally and vertically; no radial vignette or central feature, no paths or footprints, no checker/grid. This is only albedo; do not draw fake normal-map lighting. Opaque background. Deliver only the texture, not a swatch board, sphere preview, building or camp illustration.
```

## Attempt history and validation

- Attempt01: exit0;51298source triangles,4GLB/BLEND/4atlas maps/two1600x1000
  source renders. Images inspected. Preserved glTF sampler warnings; duplicate
  author/bake image nodes were initially suspected, not a proven exporter cause.
  Not published; no need to regenerate that attempt.
- Attempt02: canonical export-only PBR graphs, original author graphs retained
  as SOURCE_ONLY materials, ceramic ridge caps added, rear gable winding fixed.
  Full source log author02.log/exit receipt under Artifacts/VQ02_CampSource1.
- Independent Tools/verify_semireal_camp.py reads actual embedded PNG/PBR
  descriptors, reimports EACH exported GLB in a fresh empty scene, checks exact
  triangle counts, atlas UV, finite positions, physical bounds/units and absence
  of character/skin/animation, then renders the actual combined exported GLB.
  Readback results and final deliverable status are recorded below when verified.

Blender source images do not close the brief's real Unreal RTS/possession review,
functional collision/nav invariants, build/cook/player/performance or owner gate.

## Final source handoff

Attempt02 author exit0, actual51458triangles: ground1152/wall3704/barracks46602.
4GLBs, packed editable BLEND,4source atlasPNG2048,soilinput1254,two1600x1000
source PNGs andmanifest copied byte-identically/create-only to
`SourceArt/Environment/RomanCamp/semireal_v1/`. README distinguishes source from
production and documents units, shared scene-origin pivots, maps/slots/rights.
Git attributes confirm GLB/BLEND/PNG all use LFS; no staging/commit/push.

Readback02 and strengthened Readback03 each exit0. Latest
Artifacts/VQ02_CampSource1/Readback03/readback.json validates actual4GLB exports,
exact counts/bounds/UV, no armature/skin/animations,3embedded2048PNG perfile and
PBR texture references; combined6used materials (individual1/2/5). All6saved
source PBR graphs have exactly4image nodes with SAME REPEAT/Linear sampler.
Two final readback PNGs inspected (overview and human-height, existing legionary).
All images remain Blender source-only, NOT Unreal gameplay/physical cameras.

Warning root check: installed Blender addon
5.2/scripts/addons_core/io_scene_gltf2/blender/exp/material/texture.py169–181
warns whenever len(shader_nodes)>1 and chooses the first sampler; it does NOT
test sampler inequality. Packing separate Roughness/Metallic images triggers
this even after graph cleanup. Saved source policies verified identical; raw
GLB textures/PBR and reimported appearance verified. Warning/deprecations
preserved in author02.log, not suppressed or called a clean warning-free export.

No further environment design expansion or source regeneration needed now.
NEXT C-018H_PrivateCampPreview: import3individualGLBs in fresh namespace of the
EXISTING GamePreview, guard its original157assets with hashes, inspect real
StaticMesh/PBR/units/bounds/actual gameplay cameras, preserve collision/nav and
owner approval. No main map adoption; any environment performance claim requires
new relevant measurement, not the earlier unchanged-scene50/100 results.

Latest2026-10-20: private Camp01 import/readback and actualRTS/FPS proof are now
verified in [integration review](M6_SEMIREAL_CAMP_PREVIEW_REVIEW.md), including
caught falling-camera false positive and a negative guard mutation. Published
source files unchanged; source-delivery01.json remains the historical delivery
snapshot (review text subsequently extended). No main-map/owner/cook/player or
environmental performance approval. Next is private cook evidence, not reauthoring.

Subsequent owner instruction «Cambiar ambas cosas» explicitly requests BOTH
architecture and material revision. This supersedes the no-reauthoring NEXT
for new work, not the historical v1 proof above. New versioned sourcev2 and
actual GLB reimport/render evidence are in
[revision review](M6_SEMIREAL_CAMP_REVISION_REVIEW.md). v1/Camp01 unchanged;
next current integration is private Camp02, not v1 reimport or mainmap adoption.
