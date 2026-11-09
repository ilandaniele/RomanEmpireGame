# Asset Inventory

Inventory date: 2026-09-07
Status: M0/M1 bootstrap and M2.4 data assets complete; M2.6 input assets authored and automation-validated

## Search evidence

Latest input metadata integration (2026-10-07, C017H): existing25 project-authored input assets retained (20 actions, four IMCs, config). Only IMC_Strategic/IMC_Tactical/IMC_Possession changed, to add native inline PlayerMappableKeySettings schema1/stable v1 per-direction IDs/labels/categories for29digital rows. All33 original keys/actions/modifiers/triggers and Shared/config/20action bytes retained. Mouse2D is one locked mapping, alongside two zoom axes and Escape. No external art/service/license added; metadata recipe is original project C++ (`InputRemappingAssetTests.cpp`), not runtime asset generation. Exact pre-migration hashes `Artifacts/C017H-InputBeforeMetadata.json`, original3 recovery copies `Artifacts/C017H_InputMetadataBackup/`, accepted after hashes `Artifacts/C017H-InputAfterMetadata.json`. Explicit SHA-preflight before write/owned schema/read-only default; authoring2clean plus directed remap/default/physical-pipeline tests pass. These assets remain Git LFS; cook594/601total exit0/0errors/0warnings; actual engine registry confirms all25input assets plus15characters/7audio/12materials. Accepted25input bytes and earlier37art/source bytes unchanged.

The inventory covered:

- The complete repository working tree, including ignored files.
- All 39 commits reachable from local Git refs.
- Accessible `Desktop`, `Documents`, `Downloads`, and `OneDrive` folders for alternate `.uproject` files or directories named for Roman Empire Game.
- Epic Launcher manifests and standard `C:\Program Files\Epic Games` engine locations.

Results at the original M0 inventory point:

- No `.uasset`, `.umap`, `.fbx`, `.glb`, `.gltf`, `.blend`, `.spp`, image, or audio source file exists in the repository working tree.
- No Unreal binary or source-media asset appeared anywhere in Git history.
- No alternate Roman Empire Unreal project or backup was found in the searched user folders.
- No Epic Launcher Unreal installation or manifest was found during the original M0 search. UE 5.8.2 was installed afterward and is now the validated engine.

This establishes a clean-bootstrap baseline. It cannot prove that an inaccessible external drive, cloud account, or different computer contains old content. Such content is non-blocking and must be quarantined and reviewed through the intake process below if recovered later.

## Tracked design and generation inputs

C018D (2026-10-08): adds Editor-only `Content/Tests/Maps/FT_CampaignHandoff.umap`, same schema1 native create-missing author/source `L_Testbed` provenance as C018C. No new external art/media/license; inherited Epic template obligations unchanged, LFS filter verified, never-cook fixture. Full131clean/strict111/cook594+7platform601total/0errors/0warnings, real cross-world journey and two negative guards pass. Actual engine-parsed cooked registry excludes all six maps/test-module object refs and retains required59production packages. Fresh authorflag rerun1clean/no writes; baseline `Artifacts/C018D-CampaignMapHash.json` +prior76hashes unchanged. [Review](M7_CAMPAIGN_FUNCTIONAL_REVIEW.md). Not a shipped production asset or trusted-player approval.

C018C (2026-10-08): five Editor-only `Content/Tests/Maps/FT_{Navigation,Combat,Production,Possession,Victory}.umap` fixtures, authored natively by `FunctionalMapTests.cpp` with explicit `-RomanAuthorFunctionalMaps`. Each is a create-missing-only copy of the existing canonical `L_Testbed` plus one proprietary schema1 FunctionalTest actor. No new external art/media/generator; inherited Epic template provenance/license obligations remain those of the source map. LFS filter applies to all `.umap`; no automatic staging/commit. Source map bytes preserved; new five-map baseline `Artifacts/C018C-FunctionalMapsHashes.json`. Directed7clean/final full128clean/strict111+5/Shipping/cook594+7platform601total pass. Actual engine-parsed cooked registry excludes all five fixtures and test module object refs, retains required59production packages. Fresh explicit author rerun1clean creates/writes nothing;71prior+5fixture hashes unchanged. Fixtures are not shipped production assets. [Review](M7_FUNCTIONAL_MAP_REVIEW.md).

C017F/M6.6 CLOSED: unused Source/RomanEmpireGame/Assets/ProceduralMeshGenerators.h/.cpp removed Git-recoverably, runtime dependency removed/unused plugin explicitly disabled. Caller/binary refs0; no meshes/materials/audio/maps deleted. Existing engine StaticMesh prototypes retained, not skeletal characters. Native directed13/allPython13/Editor51/strict95/Shipping50/cook579 pass. Actual cooked registry retains all19audio/materials, source hashes unchanged/repo/LFS/diff pass. [Review](M6_PROCEDURAL_RETIREMENT_REVIEW.md). G-017G character asset brief queued in HANDOFF; no new mesh/animation/source-media output or external license claimed.

C017D materials (2026-10-06, M6.4 closed): `Content/Materials/RomanVisual` contains3 graph bases and9 MICs authored by `Tools/author_visual_materials.py` schema1. Project-authored proprietary shaders/colors; no external texture, mesh, audio, image generation or license dependency. All12 persisted assets verify in two fresh processes, repeat--apply leaves SHA256 unchanged, three transient negative probes reject changed values/unowned/missing assets. Reflected UnitBase/BuildingBase references and GC-tracked MIDs query current canonical ownership/health. Native1/full104 clean, final strictIWYU94/Shipping23; cook572 exit0 and actual cooked registry lists all12. Final actual player viewport/orthographic capture pass per-object color/stripe/state/selection pixel gates and inspection, one known engine warning each. Real-frame fixture latency fixes earlier false capture; no asset/graph/runtime change. Unused MaterialManager/factory routes retired, Git-recoverable source only; no binary deletion. Exact names/CLI/historical failures/evidence: [material review](M6_MATERIAL_ASSET_REVIEW.md).

C017E audio (2026-10-06, M6.5 CLOSED): `Content/Audio/RomanFeedback` contains one SFX SoundClass/six actual SoundWave assets (Select/Order/Complete/Impact/Victory/Defeat). Original proprietary project-authored synthetic monoPCM16/24kHz tones, not recordings/speech/score; no external samples/license/dependency. Recipe Tools/roman_audio_recipe.py and strict schema1 author_audio_assets.py create missing only, generated WAVs reproducible under Artifacts/RomanAudioSource/schema1. Readback/repeat/three rejection probes and unchanged hashes pass. Native8-voice canonical playback/GC directed7/full108 clean; Python5/strict96/Shipping14/cook579 pass. Actual cooked registry parsed by engine lists all7 audio paths; hashes19 unchanged. Five unused sources/factories retired Git-recoverably, no binary deletion. No subjective listening/trusted-player claim; controls/persistence M6.8. [Review](M6_AUDIO_FEEDBACK_REVIEW.md).

C017C/M6.3 (2026-10-06): retired the unused `Content/Blueprints/UI/WBP_MainHUD.design.json` and both empty-widget factory routes in MasterSetup/CreateBlueprints. Production UI is the canonical native controller-owned tree, verified by C017B real PIE/rendered matrix. No binary Widget Blueprint existed in that exact folder and none was deleted. The old source specification is recoverable from Git history. Other generators/specifications remain unvalidated pending M6.5–M6.9; do not run them as a setup shortcut. Read-only guard: `python -m unittest discover -s Tools/Tests -p test_native_ui_retirement.py`.

| Path | Kind | Origin | License/status | Runtime requirement |
|---|---|---|---|---|
| `Content/Audio/AudioConfig.json` | Retired C017E unused audio specification | Project-authored prototype | Proprietary source, recoverable from Git | No; committed feedback assets/recipe replace this unused input |
| `Content/Blueprints/Buildings/BP_Barracks.design.json` | Blueprint design specification | Project-authored prototype | Proprietary, source | No; reference only |
| `Content/Blueprints/Units/BP_Legionary.design.json` | Blueprint design specification | Project-authored prototype | Proprietary, source | No; reference only |
| `Content/Scripts/MasterSetup.py` (retired C017I) | Obsolete shell setup | Project-authored prototype | Exact pre-retirement backup + Git history | No; use persistent assets / Tools/ASSET_AUTHORING.md |
| `Content/Scripts/CreateBlueprints.py` (retired C017I) | Duplicate Blueprint shells | Project-authored prototype | Exact pre-retirement backup + Git history | No; native classes/UI and validated persistent assets |
| `Content/Scripts/CreateMaterials.py` (retired C017I) | Empty material graph setup | Project-authored prototype | Exact pre-retirement backup + Git history | No; owned RomanVisual assets/readonly author |
| `Content/Scripts/CreateAudioAssets.py` | Retired C017E empty-cue generator | Project-authored prototype | Proprietary source, recoverable from Git | No; strict versioned feedback authoring replaces it; no binary deleted |
| `Content/Maps/L_Testbed.umap` | Unreal test map | UE 5.8 `Template_Default` derivative, saved through Unreal MCP | Epic engine-content terms plus project modifications; M1 validated | Yes; Editor startup, native content validation, and PIE stability |
| `Content/Maps/L_VerticalSlice.umap` | Unreal gameplay map | UE 5.8 `Template_Default` derivative, saved through Unreal MCP | Epic engine-content terms plus project modifications; M1 validated | Yes; packaged-game default and smoke launch |

## M2.4 authored Primary Data Assets

All assets below were authored for this repository from its audited prototype values. They were created, configured, saved, and read back through Epic's UE 5.8 Unreal MCP on the loopback-only Editor session; no external art, vendor package, or generative model was used.

| Repository path / Unreal folder | Stable IDs | Class and purpose | Origin / license | Validation |
|---|---|---|---|---|
| `Content/Data/Factions/PDA_Faction_{Rome,Carthage,Gaul}.uasset` / `/Game/Data/Factions` | `Rome`, `Carthage`, `Gaul` | `URomanFactionDefinition`; display, color, bonuses, and starting wallet | Project-authored proprietary prototype; no attribution | MCP save/readback; Primary Asset discovery; cross-reference validation; cook and runtime registry smoke |
| `Content/Data/Units/PDA_Unit_Legionary.uasset` / `/Game/Data/Units` | `Legionary` | `URomanUnitDefinition`; factory class, availability, combat/formation data, equipment, cost, and duration | Project-authored proprietary prototype; no attribution | MCP save/readback; class/reference and value validation; native discovery test; cook and runtime spawn smoke |
| `Content/Data/Buildings/PDA_Building_{Barracks,Farm,Mine,Lumbercamp,Wall,Temple}.uasset` / `/Game/Data/Buildings` | `Barracks`, `Farm`, `Mine`, `Lumbercamp`, `Wall`, `Temple` | `URomanBuildingDefinition`; factory/menu slot, availability, cost, duration, stats, production, and placeholder presentation | Project-authored proprietary prototype; no attribution | MCP save/readback; duplicate slot/reference/value validation; native discovery test; cook and runtime registry smoke |
| `Content/Data/Territories/PDA_Territory_{Gaul,Alps,Greece,Hispania,Rome,Macedonia,Africa,Sicily,Egypt}.uasset` / `/Game/Data/Territories` | `Gaul`, `Alps`, `Greece`, `Hispania`, `Rome`, `Macedonia`, `Africa`, `Sicily`, `Egypt` | `URomanTerritoryDefinition`; campaign definition, owner, production, map position, and initial settlement | Project-authored proprietary prototype; no attribution | MCP save/readback; ID/owner/value validation; native discovery test; 9-territory runtime generation smoke |

These assets have no import settings or separate source-art files because they contain structured project data only. They are covered by Git LFS. The final M2.4 gates are 9/9 native tests in `Artifacts/Automation/20260905_190532_896`, a 535-package error-free Development cook/archive in `Artifacts/Packages/M2_4`, and `Artifacts/M2_4-EditorGame-Smoke.log` with exact registry counts and no project failure signature.

Engine basic shapes and engine materials referenced by C++ are engine-provided prerequisites, not repository assets. Their object paths load in the native content test and their packaging behavior was verified with UE 5.8.2.

## M2.6 authored input assets

Created on 2026-09-07 through Epic's narrowed loopback Unreal MCP, saved and read back as 24 assets. They contain project-authored control data only: no third-party art, generator output, import settings or attribution requirement. Project license applies; Git LFS covers all binary assets.

| Repository path | Count | Purpose |
|---|---:|---|
| `Content/Input/DA_RomanInputConfig.uasset` | 1 | `URomanInputConfig` primary asset linking the logical actions and contexts; AlwaysCook |
| `Content/Input/Contexts/IMC_{Strategic,Tactical,Possession,Shared}.uasset` | 4 | Mode-specific keys and modifiers |
| `Content/Input/Actions/IA_*.uasset` | 19 | CameraPan, Zoom, Select, Command, EnterPossession, SoldierMove, Look, Attack, Block, ExitPossession, BuildMenu, EndTurn, Cancel, BuildingSlot1–6 |

The defaults and lifecycle policy are in `INPUT_CONTROLS.md`. The native tests load the saved assets, validate their logical contract and Asset Manager discovery, assert exact defaults and evaluate WASD modifiers. The live PIE test exercises the actual Enhanced Input stack. Final cook/package/smoke evidence is maintained in the M2.6 implementation-plan section.

## Required bootstrap assets

M5.1 C-002 (2026-10-03) added `TacticalMap` soft references to the same nine territory PDAs, all pointing at the existing `/Game/Maps/L_VerticalSlice.L_VerticalSlice`. Editor authoring source: `Tools/author_territory_tactical_maps.py`; pre-C-002 backups with the C-001 graph intact: `Artifacts/C002_TerritoryBackup_20261003_235159`. Only territory packages were saved; no map, external art, generator output, license or import setting changed. Project license and LFS policy remain. Fresh-process readback, content/data-validation tests, real PIE, full 37/37 and Development cook pass. Evidence/limits: [M5_TACTICAL_MAP_REVIEW.md](M5_TACTICAL_MAP_REVIEW.md). C017I: one-time author retired after verified references; exact current script preserved in Artifacts/C017I_LegacyScriptBackup/Tools/author_territory_tactical_maps.py. No PDA mutation during retirement.

M5.1 C-001 (2026-10-03) updated only `Content/Data/Territories/PDA_Territory_{Africa,Alps,Egypt,Gaul,Greece,Hispania,Macedonia,Rome,Sicily}.uasset` with project-authored reciprocal neighbor IDs. Source is the existing map's connectivity (legacy 15,000-unit threshold); no external art, generator, license or import setting was added. Project license applies and all nine assets retain Git LFS. The reproducible one-time Editor migration is `Tools/migrate_territory_adjacency.py`; exact originals are backed up locally under `Artifacts/C001_TerritoryBackup_20261003_233940`. Separate disk readback, content/PIE tests and Development cook pass. Graph and evidence: [M5_ADJACENCY_REVIEW.md](M5_ADJACENCY_REVIEW.md). Actor transforms are no longer authoritative for adjacency. C017I: one-time spatial migration retired; exact current source preserved in Artifacts/C017I_LegacyScriptBackup/Tools/migrate_territory_adjacency.py. Never recalculate this canonical graph from actor coordinates at setup/runtime.

M1 created these source assets through Unreal Editor and they are committed:

- `/Game/Maps/L_Testbed.umap`.
- `/Game/Maps/L_VerticalSlice.umap`.

Faction, unit, building, and territory Data Assets are present from M2.4. M2.5 implements the functional production UI as native UMG trees in C++; no new Widget Blueprint, imported font, texture, external art, or generated image is introduced. M2.6 adds the input assets above. Visual asset authoring remains M6 work.

No generated asset is accepted merely because a script or MCP call succeeds. It must load without warnings, validate, cook, package, and remain covered by Git LFS.

## M6.7 original skeletal soldiers — integrated art

C017G/schema1 (2026-10-06): original project-authored geometry/rig/keyframes generated by `Tools/roman_character_recipe.py`; project proprietary license, no third-party mesh/image/texture/attribution. Actual source GLBs `SourceArt/Characters/RomanSoldier/schema1/SK_Roman_{Rome,Carthage,Gaul}.glb` and12saved imported assets under `Content/Characters/RomanSoldier/` use existing Git LFS rules. Three meshes696/764/728tri,17shared bones/normalized segmented skin/six semantic slots,192/187/187cm Z-up/ground0. Rome rectangle+crest, Carthage disc+cone, Gaul tall oval+stylized horned helmet: original visual shorthand, NOT historical authenticity. Shared Skeleton/five real30fps Idle/Walk/Attack/Block/Death clips/three MICs reuse skeletal-compatible M_RomanSurface and canonical faction materials. No physics asset/root-motion gameplay. Exact source hashes/import/settings/provenance/validation: [character review](M6_CHARACTER_ASSET_REVIEW.md). Authoring script is readonly by default, explicit create-missing-only/no overwrite/reimport. Added Gaul preserves both earlier source bytes and all existing imports.

Runtime adds3original assets: ABP_RomanSoldier (12node native snapshot), AM_RomanAttack (one slot/original clip/no damage notifies), DA_RomanCharacterPresentation (immutable required bundle). Total15uassets/3GLBs; no IDs/stats/save migration. Owned bundle gains Gaul only; all previous imports/source intact. Native4/full111clean/Python16/strict99/Shipping22 pass. Three-profile/five-pose ortho/actual FPS/default-tactical RTS images inspected/pixel gates pass (one known engine MotionVector warning each). Cook594/7platform/601total exit0/error0/warning0; actual cooked Zen registry15characters+19audio/materials,37source hashes unchanged. Readonly/probes3/LFS/repo/diff pass. M6.7 minimum offline CLOSED; no historical authenticity/trusted-player/multiplayer-per-view/whole-M6 claim. Review records all owned repairs/hashes.

## C018F rendering baselines (test artifacts, not production content)

Three byte-identical copies of actual UE5.8.2/D3D12/1280x720 raster captures live in `Tests/VisualBaselines/Win64_D3D12`, outside Content/cooking. Source: the project's native canonical character fixture and previously recorded project-owned character/material prototypes, no external acquisition or AI image generation. No new third-party license/attribution; underlying art provenance remains unchanged. SHA256 manifest/SHA1 native integrity sidecars, explicit create-missing-only review and original source receipt: [rendering review](M7_RENDERING_REGRESSION_REVIEW.md). PNGs use existing Git LFS rules. Agent review is not an owner approval or automatic commit. Independent comparison gates are tracked in the review; taking the candidate images alone is not success.

## C018F native UI references (test artifacts, not production content)

`Tests/VisualBaselines/UI_v1/Win64_D3D12` contains84 actual UE5.8.2/D3D12 PNGs plus SHA1 sidecars and a review/provenance manifest, outside Content/cooking and under existing PNG Git LFS rules. Resolution1280x720/1920x1080/2560x1440 x DPI100/125 x text100/200 x seven responsive screens. Source is the project's canonical live PIE/Slate fixture and previously recorded project-owned art; no AI image generation, imported fonts, third-party acquisition or new licensing obligation. GPT inspected every original PNG from `C018FUI_Candidates4`; create-missing-only author copied unchanged bytes. Baseline manifest SHA256 `6ca355415416f4ad40813595f537a6a087337061b56d4bf45d4bd548da82f79d`; source manifest `bdfff5ffec856e6f8c24eda43fa1870b2174f2929d7bd4dff19701bdb1aefd57`. [UI review](M7_UI_VISUAL_REGRESSION_REVIEW.md) records actual negative witnesses, repairs and independent comparison gates. Agent regression approval does not establish owner artistic approval, player verification, performance, distribution rights or an automatic commit.

## C018H packaged engine dependency inventory (not a new asset intake)

The existing dirty local QA archives contain21 Development/13 Shipping DLLs and
184/179 plugin descriptors. [Dependency review](M7_PACKAGED_DEPENDENCY_REVIEW.md)
and THIRD_PARTY_NOTICES.md distinguish these external engine-transitive binaries
and declared link inputs from repository-vendored art. All archive file hashes,
main EXE identities, version-resource receipts and selected module TPS/license
sources are recorded under Artifacts/C018H-DependencyInventory1; ten explicitly
named license candidates are hash references only, not copied or approved.
Original project asset provenance is unchanged;77previous accepted input hashes
still match. No new imports, authored art, DLLs, plugin/config changes or engine
license edits. Exact SDK/plugin/transitive applicability and redistribution rights
plus the final clean-candidate notice bundle remain an owner/release gate.

## C018H delivered engine font attribution (existing package content)

[Font review](M7_FONT_ATTRIBUTION_REVIEW.md):15 exact raw-font paths per recorded
native Pak index (14distinct hashes; duplicate Old/Tiles Bold), fresh scoped native
Pak extraction, matching original installed bytes and identical in both QA configs.
Roboto has2011/2017 variants, alongside Droid Fallback/Mono and Noto Arabic UI/Thai.
SFNT metadata records six explicit name IDs13/14, nine absent; this is not a
complete license test. Exact older-version terms, DroidMono's Ascender metadata
and the final attribution bundle remain unresolved. All are external engine-
transitive assets, not new original art. Fonts unchanged, not imported/executed,
not added to Content/Git/LFS; inspection copies stay under Artifacts. Source
provenance/rights must not be inferred from current Google Fonts names/licenses.

DroidMono subsequently matched exact AOSP source bytes (SHA256793a6342...) at
commit54b6cfa9 with a same-directory Apache-2.0 declaration and pinned document
hashes; [proof](M7_FONT_ATTRIBUTION_REVIEW.md). Upstream source/declaration mapping
resolved without changing metadata/font bytes; applicable agreement/final notice
compliance remain a release review. Do not extend this result to other fonts.

Additional exact installed declarations: Roboto.tps -> ROBOTO_License.txt and
Noto.tps -> Noto_License.txt; source/target hashes and final font receipt links are
in FontTpsSources1.json. These Content-directory TPS files were not covered by
the module-source inventory. DroidSans.tps has an absent declared redirect target;
no engine repair or font replacement. Roboto2011/2017 bytes,2016 TPS and2012
license header are distinct evidence, not automatic per-face rights approval.
See the font review for narrowly scoped upstream lookup results and notice needs.

## Asset intake record

Every future production asset must record:

| Field | Required evidence |
|---|---|
| Stable asset ID and repository path | Unreal object path plus source-art path, if retained |
| Creator/source | Author, vendor, Fab/Marketplace listing, or generator and prompt/brief |
| License | License name, commercial-use rights, redistribution limits, proof-of-purchase location |
| Version | Vendor/package version and acquisition date |
| Modifications | What changed and which source file can reproduce it |
| Import settings | Scale, axes, LODs, collision, UV/lightmap, compression, skeleton, sample rate as applicable |
| Validation | Editor load, reference scan, content validation, cook/package result, and performance budget |
| Attribution | Exact notice required in `THIRD_PARTY_NOTICES.md`, or `None` with rationale |

AI-generated concepts and placeholders remain non-production until provenance, commercial rights, visual consistency, import quality, and performance are explicitly accepted.

## VQ-01 — Semi-realistic legionary concept 01 (2026-10-19)

### Real 3D source candidate v1 — technical draft, not production approval

`SourceArt/Characters/RomanSoldier/semireal_v1` now contains ACTUAL skinned GLB,
two lower-detail GLBs, four source PBR PNGs and packed-image editable BLENDs.
Generated renders, reimported pose galleries and test receipts stay under
`Artifacts/VQ01_Semireal1`; nine temporary evidence copies were moved recoverably
to its `SourceBundleEvidenceCopies` folder, not deleted. Original equipment,
materials/rig/clip authoring and atlas-table cleanup are in `Tools/*semireal*`;
the explicitly CC0 anatomy source is identified below. No image-to-mesh
conversion, purchased asset or external program code was used. New binaries
have Git LFS attributes; files are uncommitted, no automatic stage/commit.

Selected attempt06 actual GLB hashes/triangles:

| File | Triangles | SHA256 |
| --- | ---: | --- |
| SK_Roman_Rome_Semireal01.glb | 48256 | fc8974cfc97e5ab21a2af333125b810520165612caa5b059c55666eac5b31dad |
| SK_Roman_Rome_Semireal01_LOD1.glb | 12063 | 5d82f34791508d3c888c00e0756f27734248ac6e9ea6522d00de5e5c41271e7a |
| SK_Roman_Rome_Semireal01_LOD2.glb | 4825 | 3aa39b0fbda38cbe17c0437b682f7de5d4d382f0b03db438a8507b4a2436fb85 |

Three embedded 2048px maps, eight material slots, 17 real joints and five clips
are byte-validated; reimported mesh actually deforms at five phases per clip.
Actual UE5.8.2 import in disposable `Artifacts/VQ01_Semireal1/UEProbe` passes:
186cm/17bones/five correct-duration clips/no root motion/no physics authority,
eight materials and THREE atlas textures after lossless descriptor deduplication.
Its `/Game/ArtCandidates/Attempt06_1` namespace exists ONLY in that probe.
No main Content replacement, automatic engine LOD assignment, accepted quality,
100-unit performance, RTS/FPS camera, cook/package/player or release claim.
Face/grip/raised-arm coverage/cloth/footwear/historical review remain OPEN.
Full evidence/failures: [source review](M6_SEMIREAL_SOURCE_REVIEW.md); intake and
notices retain asset-specific CC0 scope without clearing unrelated release rights.

### Additional CC0 numeric morph data — draft07, not a replacement for v1

The morph proposal is now used by selected **semireal_v2 / attempt10**, retaining
the v1 comparison. Actual delivered GLBs (all17bones/5clips/8slots/3maps2048):

| File in semireal_v2 | Triangles | SHA256 |
| --- | ---: | --- |
| SK_Roman_Rome_Semireal01.glb | 49782 | 3157ac079a631d6fa2a317d550b0b3aeb9a56be9d752d423c478f7d50e0211f6 |
| SK_Roman_Rome_Semireal01_LOD1.glb | 12444 | 99e5d203ba969c17704148dcce886c03ad781d194bcf6ddfe60ddd8e2e44d1c0 |
| SK_Roman_Rome_Semireal01_LOD2.glb | 4978 | 6c61948ebe48db34494dfdb6f71a4994d15940b101166a33c422b5ddacad1110 |

Source includes four PBR PNGs, packed editable BLENDs and six BYTE-EXACT original
recipe snapshots with manifest hashes (`-text` prevents EOL conversion changing
snapshot fingerprints). New original grip helper uses temporary anatomical finger
bones then bakes/deletes them; exported rig stays17. Original source-art motion
corrections keep the shield upright and offset visual Walk bones only, root
motion disabled/no actor movement or combat authority change. Body-fitted cloth
and torso/sleeve weighting are original. Nothing copied over current game assets.

All3 actual GLB byte checks, Blender actual reimport/deformation/gallery and UE
isolated import/material/color-space/graph readback PASS; real GPU preview also
evaluates all5clips at5phases in both Raw/Compressed UE data, rejects static or
nonfinite poses. Artifacts/receipts in `Artifacts/VQ01_Semireal1/attempt10` and
`UEProbe/import10-1-report.json`, `UEProbe/render10-1/receipt.json`; scalar facts
are not a game-camera/AnimBP/cook/package/player/FPS100/owner-approval claim.
Pipeline replay template under Tools/ArtProbe, create-only setup script refuses
existing projects. No MCP service or runtime game module in that private probe.

Same pinned MakeHuman revision/root licenses, each target explicitly CC0 in its
own header, original2014 Manuel Bastioni /stated2016 transfer /2020 holders listed
in THIRD_PARTY_NOTICES.md. Stored under the pinned source's `macrodetails` folder;
source URLs are the base raw URL above with
`makehuman/data/targets/macrodetails/<filename>`. No upstream program code.

| Filename | SHA256 | Draft profile coefficient |
| --- | --- | ---: |
| caucasian-male-young.target | 70e228ba7164737dae664454394536fc5935fa48d333c1a97d77e2dc6eacc5f5 | .75 |
| caucasian-male-old.target | b0d2b1d58cba0358af59833740e70229265f7da1c20a11240f4123b7192cfd6d | .25 |
| universal-male-young-maxmuscle-averageweight.target | c171ff9cc95e96273beb3e3b1969a01988c2d4982ebc6fe04bf81cf72ee3b295 | .35 |

`roman_adult_v1` applies these numeric deltas to the original ALL-vertex OBJ before
deriving body/helper joints and height normalization; topology/UVs remain intact.
Original base profile stays available. No explicit real age, ethnicity accuracy
or historical demographic claim follows from an upstream filename/linear blend.
Pure parser rejects duplicate/out-of-bounds/nonfinite data and pins source hashes;
seven anatomy/fit tests pass. The initial draft07 needed clothing refinement;
selected draft10/v2 passes its source/engine checks above. Quality review and
game-context integration remain open; no broader release rights implied.

### Source-production bootstrap (2026-10-19)

The owner's subsequent instruction to continue authorizes preparing a versioned 3D sample; visual acceptance, Unreal integration and release gates remain separate. Blender5.2.2LTS is available locally under ignored `Artifacts/Toolchains/blender-5.2.2-windows-x64`, not installed into Windows or packaged with the game. Official ZIP SHA256 `3849d17a682cba006075aaa3f3597ecb5c9c30ec31035b2e092c53e40679b535`,7500entries/path validation, Valid Blender Foundation signature and background version command exit0 verified. Receipt: `Artifacts/Toolchains/Blender5.2.2.bootstrap.json`; old worker44636/session15344 terminal, do not replay extraction.

Source-only anatomical asset: `SourceArt/ThirdParty/MakeHuman/a8bc2d54/base.obj`, from `https://raw.githubusercontent.com/makehumancommunity/makehuman/a8bc2d54ff0ac92e78ff71431b1023eda42bf482/makehuman/data/3dobjs/base.obj`, SHA256 `8E761E6624B8F54536409135D1636DA63B32486A90D4897F84E121D144F6FB4C`. Header explicitly releases this mesh as CC0 (September2020), identifying Data Collection AB, Joel Palmius and Jonas Hauquier. Pinned upstream LICENSE.md and full LICENSE.ASSETS.md retained beside it, hashes in receipt. Read in full; base-mesh/data license is distinct from MakeHuman program AGPL. NO MakeHuman program code copied/executed. OBJ tracked by a specific Git LFS rule. No imported Unreal asset, clothing, texture, ready-made rig or production acceptance claimed yet; no broader third-party/community asset rights inferred.

- Status: generated visual reference, owner approval pending; NOT a mesh, rig, PBR texture set, imported Unreal asset or accepted production art.
- Path: `Artifacts/VQ01_LegionaryConcept1/Legionary-Proposal1.png`; generated output ignored by Git, local only. No Unreal object path or import settings apply.
- Creator/source: GPT using the built-in image_gen tool and imagegen skill, new image without reference-image inputs; no external asset downloaded. Exact underlying model/version was not returned by the tool. Full prompt and brief: [M6_VISUAL_QUALITY_BRIEF.md](M6_VISUAL_QUALITY_BRIEF.md).
- Version/properties: proposal01, PNG1536×1024,2254449bytes, SHA256 `326C35E5138ED7571D9EDFBD4297746002AF66EE4AD4C8D83630443915414BAC`.
- Modifications: native output copied byte-for-byte to workspace, source preserved; no retouching or image-to-mesh conversion. Copy hash and dimensions verified, image inspected for intended style and three-view composition.
- Rights/attribution: generated concept only; commercial-use/redistribution suitability and any required notices remain subject to applicable terms and owner review. No blanket rights clearance or third-party attribution exemption asserted.
- Validation limits: no rig/geometry/animation/UV/LOD/performance, Unreal import, cook or player QA has been demonstrated by this image. Historical equipment is a proposal, not a validated period reconstruction. VQ-02 camp sample remains pending.

### Private semireal LOD lineage (2026-10-19)

GamePreview-only `/Game/ArtCandidates/Attempt10_1/LOD01/{SK_LODPreview,DA_LODPreview}`
duplicates Runtime05 and imports existing pinned semireal_v2 LOD1/2 source meshes.
No new download, purchase, third-party code or rights claim. Original source
provenance/licenses remain applicable; no wider distribution acceptance implied.
Native triangles49758/12420/4954, eight original PBR parents shared, own17bone
skeleton and IntegrationV3 retained. Actual source triangles49782/12444/4978:
24coincident-corner brass faces perLOD are not retained during native import;
source GLBs unchanged. Full sources/SHA/import warnings/failures/readback limits
in [LOD review](M6_SEMIREAL_LOD_REVIEW.md). New assets only in ignoredArtifacts,
NOT main Content, approved art or packaged distribution. Animation perLOD/GPU
transitions were initially OPEN in that static cohort. Latest2026-10-20 private
Animation02 and GPU03 verify real regional skin deformation at eachLOD and sampled
actual renderLOD transitions2→1→0→FPS/forced variants→2, with nine inspected PNGs.
No assets regenerated/reimported/adopted;157Content hashes unchanged.50-unit target
and100-unit stretch performance, owner visual acceptance and player QA remainOPEN.
Latest2026-10-20 first private measured pair uses SAME LOD01/IntegrationV3,
no new downloads/imports/rights or source edits. All157privateContent hashes
unchanged after50/100 actual encounters; performance target not yet accepted.
See [performance review](M6_SEMIREAL_PERFORMANCE_REVIEW.md) for receipts/limits.

### Private Roman camp source proposal — 2026-10-20

SourceArt/Environment/RomanCamp/semireal_v1/:original GPT-authored Blender geometry,
BLEND/4GLBs/4atlasPNG/soilinput/2previews/manifest+README. Source-only candidate,
NOT main Content, imported Unreal, approved production or packaged distribution.
Three actual static meshes: ground1152tri/wall3704tri/barracks46602tri (51458total).
Meters/shared kit origin;12x10m ground,9.25m wall,roof-inclusive barracks bounds
4.93x4.077x3.7335m.6materials/3embedded2048PBR PNG per GLB; original4atlas maps
shared. Soil is original built-in image_gen output actual1254x1254, not native2K;
exact prompt/SHA/rights limits in [camp source review](M6_SEMIREAL_CAMP_SOURCE_REVIEW.md).
imagegen skill used for soil; Blender original procedural material/geometry/bakes,
no external environment pack/download/purchase. Existing semireal_v2 character
ONLY a BLEND render reference, excluded from all GLBs; prior rights still apply.
No blanket CC0/generated-content commercial or redistribution clearance claimed.
Independent fresh4GLB reimports verify triangle/UV/PBR/units and saved sampler
policies, final2PNG inspected;sourcecopySHAs equalAttempt02. Exporter warning for
packedrough/metal retained and source-grounded, not suppressed. LFSattributes
confirmed, no stage/commit. UnrealRTS/FPS/nav/collision/performance/owner gates OPEN.

Latest2026-10-20 private integration: /Game/ArtCandidates/Camp01 in the EXISTING
GamePreview now holds3StaticMesh/6Material/3sharedTexture2D (12new uassets).
Published3GLBs unchanged; private import copies losslessly dedup texture descriptors,
BIN chunks independently hash-identical. Native triangles1152/3704/46602,1UV/LOD,
cm bounds match source, no generated simple collision. Actual2048PBR color-space/
normal compression/greenflip verified. RealRTS/FPS/exit/hidden witnesses at1280x720
D3D12/SM6 and falling-pose mutation documented in
[integration review](M6_SEMIREAL_CAMP_PREVIEW_REVIEW.md);7PNGs inspected, original157
and total169assets hash-unchanged across positive/mutation. NOmainContent import,
new rights claim, source edit, owner adoption, cook/player/functionalNav/performance
acceptance. Private held-pose camera evidence is distinct from gameplay simulation.

Latest2026-10-21 private camp cook: source/newCamp01uassets unchanged. Fresh
directedWindowsCook02 produces12uasset+12uexp+3ubulk, native cooked registry
classes/tags match,35hard-game identifiers/0missing, and6materials mapped inSM6
shader asset-info with actualSM5+SM6Global/Gamearchives. Not mainmap adoption,
normalproduction reachability, standalone player, rightsclearance or ownerapproval.
Cook01Zen/SM5layout andfailedwrapperassumption preserved. Details/sourcehashes/
strictverifier/12tests/semantic limits: [cook review](M6_SEMIREAL_CAMP_COOK_REVIEW.md).

### Roman camp source revision02 — owner requested architecture AND materials

SourceArt/Environment/RomanCamp/semireal_v2 contains14source files, versioned
create-only;13generated files byte-match Attempt03(74,189,080bytes)+new README.
Original GPT Blender construction/seed2202, no downloaded environment pack or
new external geometry/image/service/purchase. Reuses v1's exact1254soil image;
new procedural material masks/variation/PBR bake, not AI upscaling/detail claims.
Editable BLEND/4GLB/4atlas2048PNG/soil/2author previews/manifest/README.
Actual32,986tri: ground1152/wall3704/barracks28130.6.2x3.6m long barrack/covered
porch/opening geometry; wholebounds6.686x5.73x3.815m, common scene origin/metres.
Three embedded2048PBR PNGs perGLB; no exported COLOR_0/rig/animation/character.
Character/CC0-derived reference provenance inherited ONLY for BLEND/render scale,
not a new camp geometry/license grant. Generated-content/tool terms and owner
rights still apply; no blanketCC0/commercial clearance or authenticity approval.
Four independent native GLB reimports, sampled clay atlas coverage4410/4410,
two inspected reimportrenders and14pure tests documented in
[revision review](M6_SEMIREAL_CAMP_REVISION_REVIEW.md). Negative native73/absent
receipt proves Python failure rejected. LFSglob policy applies; files not staged.
v1/Camp01 unchanged. Not imported toUE, adopted mainmap, functional nav/collision,
environment FPS/player/QA/release approval. NEXT new private Camp02 integration.

Subsequent Camp02 private integration verified (not adoption):12new assets in
the existing GamePreview,3SM/6materials/3shared2048textures; sourcev2 and all169
previous Content files byte-intact. Total181files=179uasset/umap+2designJSON,
not181assets. Native import/read-only0, strict private4action build0 and actual
D3D12/SM6 RTS/FPS/return7PNG inspected. Native2Success/known1warning, same-DLL
falling-pose mutation fails exactly as expected;181files hash-identical. Source/
stagedGLB BIN payloads unchanged. [Current evidence/limits](M6_SEMIREAL_CAMP_REVISION_REVIEW.md).
These are real Unreal views, not the source Blender renders above. No mainmap,
functional collision/nav, current environment FPS/player or owner release approval.

Camp02/v2 subsequent PRIVATE directed cook also verified in its own fresh
`Artifacts/VQ02_CampRevision2/Cook/Camp02Cook01`:12actual cooked assets,
35hard-Game identifiers/0missing,6material SM6 mappings and both SM5/SM6 archives.
All181Content/14source files and Config/descriptor unchanged. No new source art,
third-party acquisition, rights approval, main-map adoption or player proof.
See [current cook evidence](M6_SEMIREAL_CAMP_COOK_REVIEW.md#camp02--semireal_v2--current-revised-private-cook).

### Camp03 — delegated source refinement, not Unreal adoption

`SourceArt/Environment/RomanCamp/semireal_v3` delivers14real source files after
owner instruction to continue with own judgment: oxide facade/gable, actual
porch spear/shield rack/fasteners and quieter soil/wood palette. Original project
Blender construction, previous GPT soil input at7percent, no new acquisition,
download/font/image-generation request or third-party geometry. Reference soldier
is the existing candidate in BLEND/renders only; its rights/provenance still apply.
Three meshes/fourGLBs/packedBLEND/2KPBR/1UV,35,410triangles; no LOD/collision/nav/
interactive props/rig/animation. Both28previous source files preserved. Final
Author02 and actual4GLB Readback02 native0; two author and two reimport renders
inspected. Published images are AUTHOR Blender renders, not Unreal or player views.
Source manifest `b1403c940209438f18a7addd6c4afbb7e2f725471f68e66a1e2c49f82f8e83e5`.
[Review/evidence and integration limits](M6_SEMIREAL_CAMP_REVISION3_REVIEW.md).
No new historical/owner/rights/performance/release approval; Unreal import still next.

Subsequent Camp03 native private integration VERIFIED:12new assets in existing
GamePreview/ArtCandidates/Camp03 (3SM/6Material/3shared2Ktex), real sourcev3 metadata
and30554barracktri. Native author/reopen0;181prior/193current Content files intact.
Strict private fixture build0; actual RTS/FPS/return7PNGs inspected, native2Success/
known1warning; same-source/DLL falling-pose mutation fails exactly as expected.
This is actual Unreal rendering, not Blender imagery. No new art acquisition,
mainmap/footprint/nav/collision/performance/player/owner/release approval. [Evidence](M6_SEMIREAL_CAMP_REVISION3_REVIEW.md#private-camp03-import-and-actual-engine-views--completed).

Camp03/v3 PRIVATE directed cook subsequently verified in its own Camp03Cook01:
12cooked assets/35hard-Game identifiers/0missing,6SM6material associations and
actual SM5+SM6 archives.193Content/14source/Config/descriptor intact; no new source
asset/provenance or owner/player/distribution approval. [Cook evidence](M6_SEMIREAL_CAMP_COOK_REVIEW.md#camp03--semireal_v3--current-private-cook).
