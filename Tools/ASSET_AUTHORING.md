# Supported asset-authoring contract — schema 1

Production uses persistent, project-authored Unreal assets under Git LFS. Opening the
project, PIE, builds, tests and cook must not run asset generation. No master setup
script is required. Validate references before deciding that an asset needs creation.

## Retained tools

| Tool | Allowed scope | Default / explicit write |
|---|---|---|
| `author_visual_materials.py` | `/Game/Materials/RomanVisual`, 3 bases / 9 instances | Read-only; `--apply` creates missing schema1 assets only |
| `author_audio_assets.py` | `/Game/Audio/RomanFeedback`, class / 6 waves | Read-only; `--apply` creates missing schema1 assets only |
| `author_character_assets.py` | Original rig, clips, three profiles and materials in `/Game/Characters/RomanSoldier` | Read-only; `--apply` creates missing recipe-owned assets only; native graph/montage/bundle verified separately |
| `roman_audio_recipe.py`, `roman_character_recipe.py` | Deterministic original source recipes | Pure generation functions / tested bytes; not editor startup hooks |
| `probe_character_import.py` | Diagnostic API inspection | Read-only; `--import-probe` is an explicit empty-namespace, unsaved experiment, not a production importer |

Schema/ownership, type and actual material parameters, audio format/references or
character fingerprints/rig/clip properties are validated. Existing files must match
their original recipes. `--apply` never means overwrite, reimport, adopt a foreign
asset or repair an existing asset. A failed multi-asset creation may leave earlier
new assets; inspect readback rather than assuming an atomic transaction. `--apply`
and `--test-rejection` are mutually exclusive; rejection probes restore their own
temporary changes and never save. Do not change a schema marker to disguise drift.

Native Editor-only input metadata and character graph/montage/bundle authoring are
also explicitly gated, owned/versioned and covered by readback/rejection tests.
Do not rerun one-time character repair/Gaul migration flags: their accepted state
and exact baselines are recorded in the reviews and HANDOFF.md.

## Validation recipe

Run sequentially with no asset edits, compilation or PIE in flight. For example:

```powershell
$authorScript = '-script="' + (Join-Path $PWD 'Tools/author_audio_assets.py') + '"'
& 'C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe' `
  "$PWD\RomanEmpireGame.uproject" -run=pythonscript $authorScript `
  '-EnablePlugins=PythonScriptPlugin,EditorScriptingUtilities' -DisablePlugins=Fab `
  -unattended -nop4 -NullRHI -stdout -FullStdOutLogOutput
if ($LASTEXITCODE -ne 0) { throw 'Read-only asset validation failed' }
```

No `--apply` in ordinary validation or CI. Compare source hashes before/after; a
second read-only pass must produce the same validated set without file changes.
Actual output plus native asset/content tests is evidence; source-policy AST guards
alone do not prove Unreal API compatibility or idempotence.

## Semi-realistic source candidates (non-integrated)

Current source: `SourceArt/Characters/RomanSoldier/semireal_v2/README.md` and
`M6_SEMIREAL_SOURCE_REVIEW.md`. Do NOT run schema1's author over these candidates
or replace baselines/game assets. Existing owner Editor/PIE remains untouched.

`create_semireal_legionary.py` runs only inside Blender and creates a NEW attempt
under Artifacts, rejects overwrite and captures exact own recipe bytes/hashes.
`--anatomy-profile roman_adult_v1 --closed-grip` selects the current draft recipe.
Use the already verified portable Blender in Artifacts/Toolchains; do not reinstall.
`validate_semireal_glb.py` reads actual exported data; `probe_semireal_blender.py`
actually reimports/evaluates deformation and can render a native pose gallery.

An isolated UE project already exists at Artifacts/VQ01_Semireal1/UEProbe. On a
fresh workspace ONLY, `Tools/New-SemirealArtProbe.ps1` copies the versioned minimal
template there. It refuses existing targets/reparse parents, does not start UE,
enable MCP, change the game or bypass Windows security. Reuse existing evidence.

`probe_semireal_unreal.py` requires that EXACT project and an explicit JSON config
inside it: source path under Artifacts/VQ01_Semireal1, fresh
`/Game/ArtCandidates/<attempt>` folder, report filename. `read_only:true` reads back
an existing folder without saving. Never overwrite partial imports; inspect first.

When passing Python script arguments, use FORWARD SLASH script paths. Unreal's
argument parser turned `\\r` in a Windows script path into CR during a preserved
failed attempt. A typical commandlet argument is
`-script="C:/.../Tools/probe_semireal_unreal.py --config=import10-1.json"`.
Use `-unattended -nop4 -NullRHI -nosound` for import/readback. GPU preview uses
`render_semireal_unreal.py --attempt=renderNN-N --import-report=<passed-report>`,
`-AllowCommandletRendering -NoTextureStreaming -d3d12 -sm6`, NOT NullRHI. Every
output is create-only; save PID/log/session for a running job, do not launch twice.
Native captures evaluate actual Raw/Compressed motion but do NOT prove gameplay,
native AnimBP/montages, RTS/FPS cameras, installed LOD transitions or packaged QA.

### Private native game-context preview

`New-SemirealGamePreview.ps1 -PlanOnly` preflights a fixed create-only target at
`Artifacts/VQ01_Semireal1/GamePreview`. Without PlanOnly it copies/hashes the exact
Source/Config/Content snapshot and five Editor binaries/dependencies, not player
executables, PDBs, Saved or user credentials. It reuses the already verified
Attempt10_1 imported namespace from UEProbe; no repeat import, no schema1 asset
replacement. Its descriptor disables Fab/MCP/toolsets and enables Python/editor
scripting ONLY in that private copy. Existing targets and reparse paths fail.
Preserve the receipt and failed attempts; never delete/recreate to conceal errors.

Run `verify_semireal_game_preview.py` using the private `.uproject` and the Python
commandlet, NullRHI, unattended/no sound/no P4. Its create-only
`game-context-readback01.json` verifies the initial copy hashes, original native
bundle and independent candidate rig/clip/material/texture references WITHOUT
saves. This is a snapshot gate BEFORE building/changing the private copy; do not
replay it over later built binaries and call expected hash changes a copy failure.

`ArtProbe/Native/SemirealPreviewAuthoring.cpp` is copied into ONLY the private
Editor test module. Build that private project, not the owner's running Editor.
Directed test: `RomanEmpireGame.ArtCandidates.SemirealV2Authoring`. With no
integration assets it must fail without `-RomanAuthorSemirealPreview`; that flag
creates a new AnimBP duplicate with all four graph clip references replaced by
the candidate clips, plus a new attack montage and reflected bundle under
`/Game/ArtCandidates/Attempt10_1/IntegrationV3`. The native canonical AnimInstance
parent/graph behavior remains reused, NOT the old skeleton/animations. Partial
bundles are rejected; a complete owned bundle is checked read-only. Run a fresh
readonly process after authoring. No damage notifies/new gameplay authority.

IntegrationV1 is retained as rejected evidence: its montage had a segment/slot
but no playable composite section. `Montage_IsPlaying` was insufficient proof;
strict pose tests caught the stationary arm. V2 authors/validates an Attack
section starting at zero plus the canonical short blends. Do not overwrite V1.

`author_semireal_runtime_materials.py --attempt=Runtime05
--report=runtime-materials05.json` runs ONLY in that private project after V3
graph authoring. It creates eight new PBR parents with actor Color/Damage/
FactionTintStrength parameters and a new mesh/bundle referencing V3. Original
texture bindings (base/normal/rough/metal) remain shared and verified. Non-tinted
skin/metal/eyes/brass preserve original color; only Body/Shield receive faction
tint. Data opt-in `bUseMeshMaterialParents` defaults FALSE, preserving schema1.
Add `--read-only` with a NEW report filename to inspect existing output without
saving. Namespace/report must be fresh for authoring; partial Runtime01 and
Runtime02's old montage are evidence, not current candidates. No shader-GPU or
owner-approval claim follows from NullRHI graph readback.

Directed runtime test: `RomanEmpireGame.Characters.MaterialPolicyPIE`, normally
uses a transient duplicate of the original bundle (no asset changes). In the
exact private project, `-RomanSemirealPresentation=/Game/ArtCandidates/Attempt10_1/Runtime05/DA_RuntimePreview`
selects the candidate. It checks actor-local MIDs/parents, semantic tint/health,
selection stability, two-actor isolation, canonical poses across separate frames,
actual attack timeline/deformation, death hold and teardown GC. Never bypass the
runtime material logic by clearing overrides in the screenshot fixture.

IntegrationV3 adds opt-in same-body local-player hand reach, no stretch/FOV/camera
hack or gameplay clock. V2 failed the stronger actual FPS hand-projection check;
V3 preserves all four base clips/death branch and uses the native local-player
snapshot after the montage. SlotCrest is brass in this mesh, not team fabric.

Camera fixture is private-only and outside the normal RomanEmpireGame suite:
`RomanPreview.Cameras.Candidate`, explicit `-RomanSemirealCamera=RTS` or `FPS`,
`-RomanSemirealCapture=<fresh_name>`, plus candidate argument above. Use actual
`-D3D12 -sm6 -RenderOffscreen -NoTextureStreaming -ResX=1280 -ResY=720 -ForceRes`.
It rejects NullRHI/other projects/existing output. PNG/receipt under the private
Artifacts/<fresh_name>; export native Automation report separately. FPS includes
idle/attack/mesh-hidden witnesses, projected hand and exit/head/IK restoration.
Read the PNGs and renderer logs; do not infer quality from file existence.

The fixture uses one legionary in its three bundle fields, NOT accepted designs
for three factions. Native real-camera evidence is in M6_SEMIREAL_CAMERA_REVIEW.md.
Owner quality approval, LOD transitions/new-model performance, real-input/player
QA and main art adoption remain separate gates; no silent main Content copy.

## Private semireal LOD candidates

`Tools/author_semireal_lods.py --attempt=LOD01 --report=lod-author01.json`
is guarded to the exact existing `Artifacts/VQ01_Semireal1/GamePreview` project.
Run through its Python commandlet with forward-slash script paths, unattended,
NullRHI, and no concurrent Editor/build. It duplicates Runtime05 mesh/bundle into
a FRESH LODNN namespace; native SkeletalMeshEditorSubsystem ImportLOD invokes
Interchange on the SHA-pinned LOD1/2 GLBs. It never repairs/overwrites an existing
namespace or report. LOD01 now EXISTS: do not rerun its author command. Append
`--read-only` with a NEW JSON report filename for non-mutating readback.

Preserves IntegrationV3, all eight Runtime05 PBR parents, shared textures,
Body/Shield faction slots and original skeleton; no physics asset, new faction
design, main Content adoption, or gameplay changes. Every existing private
Content file is hash-compared before/after (including the candidate in readonly).
Actual LOD count, vertex/section counts and bundle refs are checked after import;
the return value alone does not prove import success.

`Tools/ArtProbe/Native/SemirealLODReadback.cpp` belongs ONLY in private Editor test
module, copied by the preview creator for future snapshots. Existing preview
receives only this fixture file, then a directed strict Editor build. Native test
`RomanPreview.LOD.Readback` requires `-RomanSemirealLOD=LOD01` and
`-RomanSemirealLODReceipt=<fresh-simple-name>.txt`, plus a fresh Automation report.
It checks exact per-material imported triangles, all skin weights/bones/indices,
UVs/finite geometry/rest bounds, bind transforms, shared parents and actual
descending screen sizes. It saves a receipt, NOT assets. Inspect Automation
JSON: UE can return exit0 while a selected test FAILED.

Source GLBs retain49782/12444/4978 triangles. Native import retains
49758/12420/4954: independent source topology analysis found exactly24
coincident-corner faces per LOD, all brass slotCrest (16 near-zero-area). Source
reports and per-slot native assertions reconcile the difference; no GLB changed.
The later bRemoveDegenerates setting is FALSE and preserved: import/chunk code
can discard collapsed indices independently. Do not infer the flag is true from
the face-count difference. See `M6_SEMIREAL_LOD_REVIEW.md` for preserved failures.

Readback alone does NOT prove animated deformation or GPU LOD transitions.
Additional private fixtures now provide these directed technical checks:

- `RomanPreview.LOD.Animation` requires
  `-RomanLODAnimationReceipt=<fresh-simple-name>.txt` and fresh report/log paths.
  Runs real3actors/graph and engine skinning on every renderLOD, with regional
  vertices/minimum amplitude and held poses. NullRHI is valid for CPU readback,
  not GPU proof. `-RomanLODAnimationSuppressAttack` is an EXPLICIT negative probe:
  expect three regional Attack errors, do not use on an ordinary passing run.
  It only stops transient actors' montage; no asset mutation/restoration needed.
- `RomanPreview.LOD.GPU` requires realD3D12/SM6 (rejects NullRHI),
  `-RomanLODCapture=<fresh-alphanumeric-directory>` and fresh report/log paths.
  Reuses actual game viewport1280x720, legal production zoom.55/.6625/.70,
  actual player possession, forced FPS variants and hidden-body witness. Reads
  actual render-threadLOD, restores pawn/head/IK/autoLOD and saves nine PNGs.
  Preserve the known engine warning. Inspect images and Automation JSON, not
  only process exit. LODGPU01/02 were failed fixtures and are preserved;03 is
  current sampled technical proof. No threshold/FOV/scale/LOD asset edits.

See review for source/receipt hashes and limits. Neither fixture proves
new-model50/100-unit performance, owner quality acceptance, continuous pop-free
camera motion, physical-input or packaged-player QA. No main art adoption.

## Private semireal performance capture

`capture_semireal_encounter.ps1 -Name <freshName> -Total 50` (or100) requires
the existing exact private preview, no other UE/build process, and its compiled
matching Native/SemirealEncounterProfile.cpp. Uses the ORIGINAL Encounter50/100
tests unchanged; observer is disabled unless explicit private flags are present.
Before-spawn per-instance bootstrap binds LOD01 before init, not CDO override
(constructor reassigns its bootstrap per instance). No live-unit repair or saves.
Wrapper owns hidden process/PID, trace, logs, hardware/power/asset manifest,
Automation report and actual CSV; rejects existing evidence, nonzero exit/stderr,
failed/pending test, missing native receipt/PNG or any Content hash change.

`summarize_semireal_profile.py --name <capture> --report <fresh>.json` joins CSV
engine-frame IDs to population records. Uses FINAL native CSV union header and
checks canonical measured-frame count; camera/population windows stay separate.
Schema2 and schema3 analyses retain the complete frame set; schema3 additionally
records runtime cap/resolution observations and explicitly labelled inference.
`perf50-summary01.json` is obsolete185-frame
analysis and must NOT be used; current first pair summaries are50-summary02 and
100-summary01. Native observer cost is included, not subtracted. Names already
used: Perf50_01(failed old-art attempt), Perf50_02, Perf100_01. Never overwrite.
PreCapture.png is read only before CSV so no pixel-readback stall in timings.
No new-art FPS claim from original unmodified default-art/NullRHI tests alone.

These are provisional Editor/1080p OUTPUT captures, not accepted player60FPS.
The read-only view observer records public primary resolution upper bounds and
secondary scale. Constant1.0 with dynamic resolution disabled supports a fixed
full-scale inference from the installed legacy driver's implementation; it is NOT
a protected actual-fraction getter or GPU render-buffer pixel-size readback.
Runtime battery classification, smoothing, t.MaxFPS, VSync and resolution CVars
are recorded during capture, not merely before PIE.

Explicit diagnostic only (fresh name, do not overwrite existing evidence):

```powershell
.\Tools\capture_semireal_encounter.ps1 -Name <fresh-name> -Total 50 `
  -NativeResolutionDiagnostic -DisableEditorBatteryCapDiagnostic
```

The two switches default OFF. They set only this process's r.ScreenPercentage100
(increases resolution) and r.DontLimitOnBattery1 (bypasses Editor battery cap).
No persistent/Windows/power/security, quality-group, AI, camera or LOD change.
Do not pool capped/default-resolution results with this diagnostic cohort.
Startup ExecCmds uses COMMA-separated console commands; Automation retains its
own RunTest;Quit semicolon queue. Pure guard: test_capture_semireal_commands.ps1
(8 combinations; launches no Unreal). Python analyzer guards currently11PASS.
Perf50_NativeDiag01 is a preserved launcher failure (semicolon consumed commands,
no test started); do not overwrite or count it as a game failure.
See M6_SEMIREAL_PERFORMANCE_REVIEW.md for current evidence and remaining gates.

## VQ-02 private camp source (not Unreal adoption)

Current verified source: SourceArt/Environment/RomanCamp/semireal_v1 (Attempt02),
3staticmeshes/51458triangles,BLEND/4GLBs/shared PBR maps. Do not regenerate/import
over existing source. M6_SEMIREAL_CAMP_SOURCE_REVIEW.md records original image_gen
soil prompt/actual1254input, units/shared-origin, warnings, rights and evidence.
No paid/downloaded environment pack or new toolchain installation.

Author: Tools/create_semireal_camp.py requires a fresh owned directory under
Artifacts/VQ02_CampSource1 containing ONLY Soil-Input01.png; deterministic seed,
create-only output. Run with existing Blender --background --factory-startup
`--python-exit-code 1` and `--output <fresh>`/`--texture-size 2048`/
`--render-size 1600`. Preserve
log/exit receipt; never rerun a used attempt to hide a failure.
Verifier: Tools/verify_semireal_camp.py --source <attempt> --report-dir <fresh>
inside Blender. Latest Readback03 validates actual4GLBs and saved BLEND sampler
policy and renders exported geometry; not an Unreal/player/owner test.
source-delivery01.json hashes all14durablefiles/evidence. Blender previews do
NOT approve VQ-02 integration, nav/collision, cook, performance or main map art.

### Private camp Unreal integration

Camp01 already EXISTS in existing GamePreview: never reimport over it. Publicmesh
readback is commandlet-safe (StaticMeshEditorSubsystem is not instantiated there).
For a relevant new check use a fresh report:

```powershell
.\Tools\run_semireal_camp_import.ps1 -Attempt Camp01 -Report camp-new-readback.json -ReadOnly
```

Tools/import_semireal_camp.py guards exactproject/fresh namespace/reparse/sourceSHA;
original157assets hash-protected, private total169. New import namespace uses
LOCAL material reuse, no collision/Nanite/extraUVgeneration, complete raw geometry.
Sourcecopies losslessly dedup GLBtexture descriptors; original BIN payloads exact.

Native fixture SemirealCampPreview.cpp copied ONLY into private Editor Tests;
strict build required on fixture change. capture_semireal_camp.ps1 -Name <fresh>
checks EXACT RomanPreview.Camp.Readback/GPU, process/report/status/Content hashes,
seven fresh PNGs. -SkipPoseHold is an opt-in NEGATIVE mutation and must fail EXACT
falling-pose assertion, not any error. Production AutomationReport helper remains
restricted to RomanEmpireGame.*; do not weaken it to accept private prefixes.

Latest valid CampGPU02 plus CampPoseMutation01 have same DLL. CampGPU01 is INVALID
for FPS: possession re-enabled movement above visual-only ground. Sourceposehold
now re-applied after entry/exit, actorlocation/camera invariance checked. NO support
collision or gameplay workaround. Native use realcontroller/pawn/view target;
held source pose is NOT realtime gameplay/nav/physical input/performance/player.
See M6_SEMIREAL_CAMP_PREVIEW_REVIEW.md and private-camp-cohort01.json. Next private
cook, not reauthor/import or identical recapture without a relevant change.

### Private camp directed cook

Current evidence: Artifacts/VQ02_CampCook1/CampCook02/verified01.json; read it,
do not rerun unchanged cook or overwrite CampCook01/02. Tools/cook_semireal_camp.ps1
-Name <fresh> runs explicitCamp01Windows content cook with loose writer
(-SkipZenStore) and process-only +D3D12TargetedShaderFormats=PCD3D_SM6 (keepsSM5).
No config/default-map edits. NoDefaultMaps/NoGameAlwaysCook make this a targeted
diagnostic, NOT normal production reachability or a runnable package.

Dump BOTH cooked AssetRegistry.bin and Metadata/DevelopmentAssetRegistry.bin with
native DumpAssetRegistry (-Path/-OutDir/-All/-LinesPerPage=100000), sequentially,
fresh RegistryRuntime01/RegistryDevelopment01. Preserve native PID/exit/inputSHA
receipts as currentCook02 examples. Runtime registry omits dependencies; the
development dump provides forward Package flags. Reverse References are not deps.
Tools/verify_cooked_camp.py --name <samefresh> requires actual cook/files/tags/
SM6shaderindexes/archives/hard-gameclosure and original169Content/14source/config
hashes. Fresh verified01.json only;12tests via test_verify_cooked_camp.py cover
strictnativeUTF8/UTF16/primaryMapID/duplicate/missing/schema/graph/shader guards.

Cook01's native success with Zen storage was initially misclassified by a loose
file check; preserved and confirmed via native runtime dump. Cook02 uses explicit
loose bytes and both shaderformats, no Zen/DDC/Windows security changes. Texture
PF_* token observations are NOT complete package/texture decoding. See
M6_SEMIREAL_CAMP_COOK_REVIEW.md for artifact paths and all remaining player/owner/
functional-nav/rights/performance/M7/M8 gates. Do not launch blocked executables.

## Native FunctionalTest map authoring (commands)

Dedicated FunctionalTest maps use native automation, not startup generation:

```powershell
& 'C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe' `
  "$PWD\RomanEmpireGame.uproject" -unattended -nop4 -NullRHI -DisablePlugins=Fab `
  '-ExecCmds=Automation RunTest RomanEmpireGame.Functional.MapsReadback;Quit' `
  "-ReportExportPath=$PWD\Artifacts\Automation\<unique-readback>" -stdout -FullStdOutLogOutput -log
```

Only for authorized creation of missing fixtures, add `-RomanAuthorFunctionalMaps`.
The native author preflights all owned schema1 targets and never overwrites or
repairs them. Ordinary Test/CI runs omit the flag. `/Game/Tests` is never-cook;
verify the actual cooked registry as well. See `M7_FUNCTIONAL_MAP_REVIEW.md`.

## Retired setup and migrations

`Content/Scripts/{MasterSetup,CreateBlueprints,CreateMaterials}.py` created unused
or incomplete shells with no reliable production validation. They are retired in
favor of existing native classes/UI and persistent materials. The one-time
`author_territory_tactical_maps.py` and `migrate_territory_adjacency.py` are retired:
the nine PDAs already contain validated map references and stable adjacency IDs.
Never recalculate canonical adjacency from actor transforms during setup.

Original tracked scripts remain recoverable in Git history. Exact pre-retirement
versions, including uncommitted changes and the two untracked migrations, are in
`Artifacts/C017I_LegacyScriptBackup/manifest.json` with verified SHA256 copies.
Do not execute these historical copies. No binary asset was removed or regenerated.
