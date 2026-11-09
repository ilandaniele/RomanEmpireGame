# Roman camp — semireal_v3 / Camp03 source candidate

Private art iteration made with delegated judgment after the owner asked for
further visual changes and then said to continue. NOT owner-approved final art,
Unreal integration, launch approval or a historical reconstruction certification.

Actual editable Blender source and four usable GLB files, not AI model pictures:

- SM_CampGround: 12x10m broad earth/wear tracks, reduced gravel contrast.
- SM_CampWall: existing palisade layout, restrained grain/palette.
- SM_CampBarracks: 6.2x3.6m wall footprint, tiled roof/porch, oxide base/cornice/gable,
  clipped paint around real openings, static spear/shield rack and hinge fasteners.
- RomanCamp-Proposal03.glb combines the same three meshes. Shared scene origin,
  metres, atlas UV0 and embedded PBR; no exported COLOR_0 double tint, rig or clips.

Bounds including roof/porch are approximately6.686x5.73x3.815m; this is not a new
gameplay footprint. Three meshes/6material slots/2Katlas;35,410triangles total
(1152ground/3704wall/30554barrack). No LOD/collision/nav/interactive props authored.
Do not replace production mesh using its cube PrototypeScale. Refer to the v2 fit
review before separately measuring/adapting final geometry in Unreal.

Procedural modelling/shaders and PBR baking in the already installed portable
Blender5.2.2; no new download, installation, external geometry, font or AI image
request. Soil-Input01.png is the unchanged prior GPT-generated input at7percent
material contribution. Reference legionary is the existing project candidate,
visible in BLEND/renders only, never exported into these environment GLBs.
Previous source/provenance and rights review still apply; no blanket release license
is asserted by this candidate. The paint/equipment are proposals, not period proof.

The source-manifest records actual file/tool hashes and protects both v1/v2 source
sets. Local Author02 and independent Readback02 evidence is retained under
Artifacts/VQ02_CampRevision3; the versionable review records checks and limits.
Rendered PNGs are Blender author views, NOT Unreal cameras or player evidence.

Next integration belongs in a NEW /Game/ArtCandidates/Camp03 private namespace,
without rewriting Camp01/02, main Content, Config, baselines or gameplay authorities.
New import/read-only/real RTS+FPS/state/fit/cook gates are required for new assets;
older v2 passes do not cover v3. Keep GamePreview/181existing Content files intact.
