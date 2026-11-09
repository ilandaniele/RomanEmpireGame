# Roman semi-realistic soldier — candidate v2

Real mesh / editable source, selected from attempt10. Technical candidate for
review, NOT owner visual approval or a replacement for current game assets.
v1 is preserved. No gameplay, camera, collision, save, input or navigation rewrite.

![Local source preview](../../../../Artifacts/VQ01_Semireal1/attempt10/Legionary-ThreeQuarter.png)

## Delivered

- GLBs: LOD0 **49,782** triangles, LOD1 **12,444**, LOD2 **4,978**.
- Seventeen semantic bones; approximately1.86m including helmet. Source Blender
  Z-up/metres; glTF Y-up. Unreal import verifies186cm/Z-up.
- Five30fps clips: Idle2s,Walk1s,Attack0.6s,Block0.8s,Death1s.
- Eight semantic material slots; three shared2048px PBR maps embedded in GLB.
  Four source PNG maps also retained. Texture descriptors are deduplicated;
  Unreal imports three textures, not24 identical duplicates.
- Editable Candidate/PreviewScene BLENDs with packed source maps.
- Exact six original author/helper source snapshots under `recipe/`, with
  SHA256s in source-manifest.json. Snapshots are source evidence: restore to the
  correct Tools layout in a separate scratch checkout if replaying, not on top
  of a live workspace. GLB/BLEND can be used independently of the Python tools.

## Improvements over v1

Adult anatomical morph proposal, body-fitted tunic/mail and accessories, closed
sword grip baked with a temporary finger rig (removed before17bone export),
arm-free torso-layer weights and shoulder-bound sleeves, upright blocking shield,
flat shoe compensation and bounded in-place vertical Walk bone adjustment.
This does NOT enable root motion or move the canonical unit/capsule.

Source is derived from explicitly CC0 MakeHuman numerical data, not its AGPL
application. Full source/licenses under `SourceArt/ThirdParty/MakeHuman/a8bc2d54`.
Original equipment, materials, weights, grip and motion code are repo-authored.
Morph coefficients/provenance in ASSET_INVENTORY.md; historical accuracy or a
specific age/demographic representation is not implied by source filenames.

## Evidence / remaining work

All generated captures and receipts stay under `Artifacts/VQ01_Semireal1`:

- `attempt10/*-inspection.json`: actual GLB arrays, skin, keys and embedded maps.
- `attempt10/blender-reimport01.json` / `Actual-GLB-Poses.png`: real exported mesh
  deformation sampled in five phases per clip; gallery inspected.
- `UEProbe/import10-1-report.json`: actual new isolated UE5.8.2 import with mesh,
  skeleton, materials/color spaces/graph bindings and five clips read back.
- `UEProbe/render10-1/receipt.json`: real D3D12/SM6 front/back visibility gates and
  five clips evaluated across five phases in BOTH Raw and Compressed UE data.
  This is imported-asset verification, not a packaged player's animation proof.

No existing schema1 meshes, native animation graph/montages, gameplay references
or raster baselines changed. Integrate in a NEW namespace with this skeleton,
its own clips and textured materials, not the old flat material overrides.
LOD files are candidates, not installed engine LOD transitions. Actual RTS/FPS
cameras, local head hiding, selection/faction/damage/cycle of life, performance,
cook/package and owner acceptance remain required. Face/skin detail, hands,
cloth/animation quality, footwear and historical fit remain art-review items.
See [review](../../../../M6_SEMIREAL_SOURCE_REVIEW.md) and IMPLEMENTATION_PLAN.md.
