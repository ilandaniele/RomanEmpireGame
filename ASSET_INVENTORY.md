# Asset Inventory

Inventory date: 2026-09-05
Status: M0 legacy search and M1 bootstrap complete; M2.4 Primary Data Assets created and validated

## Search evidence

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

| Path | Kind | Origin | License/status | Runtime requirement |
|---|---|---|---|---|
| `Content/Audio/AudioConfig.json` | Audio specification | Project-authored prototype | Proprietary, source | No; reference only until licensed audio is imported |
| `Content/Blueprints/Buildings/BP_Barracks.design.json` | Blueprint design specification | Project-authored prototype | Proprietary, source | No; reference only |
| `Content/Blueprints/UI/WBP_MainHUD.design.json` | UMG design specification | Project-authored prototype | Proprietary, source | No; reference only |
| `Content/Blueprints/Units/BP_Legionary.design.json` | Blueprint design specification | Project-authored prototype | Proprietary, source | No; reference only |
| `Content/Scripts/MasterSetup.py` | Editor generation script | Project-authored prototype | Proprietary, source; unvalidated | No; do not run until reviewed against UE 5.8 |
| `Content/Scripts/CreateBlueprints.py` | Editor generation script | Project-authored prototype | Proprietary, source; unvalidated | No; do not run until reviewed against UE 5.8 |
| `Content/Scripts/CreateMaterials.py` | Editor generation script | Project-authored prototype | Proprietary, source; unvalidated | No; do not run until reviewed against UE 5.8 |
| `Content/Scripts/CreateAudioAssets.py` | Editor generation script | Project-authored prototype | Proprietary, source; unvalidated | No; do not run until reviewed against UE 5.8 |
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

## Required bootstrap assets

M1 created these source assets through Unreal Editor and they are committed:

- `/Game/Maps/L_Testbed.umap`.
- `/Game/Maps/L_VerticalSlice.umap`.

Faction, unit, building, and territory Data Assets are now present from M2.4. Dedicated Enhanced Input actions/mapping contexts and production UMG assets remain planned for M2.5–M2.6 and must follow the same provenance, LFS, validation, cook, and package rules.

No generated asset is accepted merely because a script or MCP call succeeds. It must load without warnings, validate, cook, package, and remain covered by Git LFS.

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
