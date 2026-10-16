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
