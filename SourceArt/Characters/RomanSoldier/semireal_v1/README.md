# Roman semi-realistic soldier — source candidate v1

Draft, 2026-10-19. Real geometry, rig, textures and animation; NOT final visual
approval or a replacement for the assets currently used by RomanEmpireGame.

![Local source-art preview](../../../../Artifacts/VQ01_Semireal1/attempt06/Legionary-ThreeQuarter.png)

## Contents / contract

- Three GLBs: LOD0 48,256 triangles, LOD1 12,063, LOD2 4,825.
- Seventeen semantic bones, source height 1.86 m; glTF Y-up / source Blender Z-up.
- Idle 2 s, Walk 1 s, Attack 0.6 s, Block 0.8 s, Death 1 s, sampled at 30 fps.
- Eight slots: Body, Metal, Skin, Leather, Shield, Crest, MailLinks, EyeWhite.
- Shared 2048 px BaseColor/Normal/packed metallic-roughness embedded in GLB.
  Separate Metallic/Roughness/Normal/BaseColor PNGs are also retained for editing.
- `Legionary-Candidate01.blend` is editable source. `Legionary-PreviewScene01.blend`
  adds a source-art-only studio scene. All four source images were packed and
  verified in both BLENDs; log `Artifacts/VQ01_Semireal1/source06-pack2.log`.
- `Artifacts/VQ01_Semireal1/attempt06/Actual-GLB-Poses.png` is a native Blender rendering of the REIMPORTED GLB,
  left-to-right Death, Block, Attack, Walk, Idle. It is not an Unreal screenshot.

## Evidence / limits

Generated captures/test receipts stay under `Artifacts`, not production assets.
The three `attempt06/*-inspection.json` files inspect actual GLB arrays/maps/keys,
not just author metadata. `attempt06/blender-reimport04.json` samples real
deformation at five phases per clip. `UEProbe/import06-1-report.json` records a real UE5.8.2 import in an isolated
project: 186 cm, 17 actual bones, five clips, eight material slots, three textures.
All paths above are relative to `Artifacts/VQ01_Semireal1`. Its `/Game/ArtCandidates/...` paths belong ONLY to that disposable probe, not the
main game. JSON paths pointing into Artifacts preserve original evidence; they
are not required runtime dependencies. See [review](../../../../M6_SEMIREAL_SOURCE_REVIEW.md).

Still draft: face/skin detail, closed weapon grip, underarm coverage/intersections
in raised-arm poses, cloth deformation, footwear and historical accuracy. Clip
motion existing in the file does not mean convincing mocap or gameplay tuning.
LOD files are source candidates, NOT installed automatic Unreal LODs. Eight
sections/draw calls and a 100-unit budget need actual profiling. No final art,
RTS/FPS camera, cook/package, player-QA or distribution approval is asserted.

## Source / rights

Anatomy: explicitly CC0 MakeHuman `base.obj`, pinned commit
`a8bc2d54ff0ac92e78ff71431b1023eda42bf482`, September-2020 asset-specific release.
Original source and full licenses live in `SourceArt/ThirdParty/MakeHuman/a8bc2d54`.
No MakeHuman AGPL application code is copied/executed. Clothing/equipment,
procedural PBR materials, rig/animation authoring and texture-table cleanup are
original repo code in `Tools/*semireal*` plus the existing `roman_character_recipe`.
The concept image was a reference, not an image-to-mesh conversion.
See `ASSET_INVENTORY.md` / `THIRD_PARTY_NOTICES.md` for provenance and limits.

Integration must use a NEW Unreal namespace and these new bind poses/clips; do
not reimport over schema1 or silently reuse old animations/baselines. Preserve
canonical gameplay authorities. Owner visual acceptance remains separate.
