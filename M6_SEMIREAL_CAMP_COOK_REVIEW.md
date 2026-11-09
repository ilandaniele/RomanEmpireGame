# Private camp Windows cook — verified content, not player approval

2026-10-21. C-018H_PrivateCampCook. Directed private content cook, native cooked
registries/dependencies and SM5+SM6 shader evidence verified. No standalone
Development/Shipping build, packaged launch, owner approval or main-map adoption
claimed. Full IMPLEMENTATION_PLAN.md goal/M7/M8 remain OPEN.

## Exact scope and protection

Reused Artifacts/VQ01_Semireal1/GamePreview; Camp01 remains12private assets:
3StaticMesh/6Material/3Texture2D. Source/preview proof and limitations:
[source review](M6_SEMIREAL_CAMP_SOURCE_REVIEW.md),
[real camera preview review](M6_SEMIREAL_CAMP_PREVIEW_REVIEW.md).
Tools/cook_semireal_camp.ps1 uses owned hidden process, fresh output namespace,
reparse guard, explicit source directory,169Content+14sourcefiles+privateConfig
and descriptor hashes. All protected bytes/counts remain unchanged. No build,
reimport, map/config rewrite, security/driver change, commit or push.

Process-only CookDir=.../Content/ArtCandidates/Camp01 and OutputDir=<fresh>/Cooked/
[Platform]. NoDefaultMaps/NoGameAlwaysCook omit default-map/configured directory
requests, NOT hard/soft dependencies or global shaders. Engine/startup/dependency
content is also cooked. This verifies explicit diagnostic reachability, NOT the
normal Shipping graph or an application with a launch map. GameDefaultMap remains
/Game/Maps/L_VerticalSlice. No SkipHardReferences/SkipSoftReferences/quality cuts.

Skill unreal-packaging informed the separation between content cook, standalone
configuration, package and actual player gates. Installed engine sources were
used for exact CookDir/OutputDir/SkipZenStore and config override semantics.

## Preserved first cook and corrected assumptions

CampCook01 on2026-10-20: native exit0,537cooked+7platformskips=544,0errors/warnings.
The wrapper failed because it expected loose .uassets but the engine defaults to
Zen Store. No missing-content conclusion was accepted from an absent loose path.
Native DumpAssetRegistry runtime export subsequently confirmed all12Camp01objects;
input registry SHA unchanged.8pages under RegistryZen01, dump exit0. Native shader
libraries were SM5only, NOT equivalent to prior D3D12/SM6 viewport evidence.

CampCook01 bytes/logs/process/exit/dump preserved under
Artifacts/VQ02_CampCook1/CampCook01; no rerun/overwrite/cache clearing. The wrapper
failure is an output-layout assumption, not a failed engine cook. Default runtime
registry omits dependency graph; do not treat0dependency rows as proof of no deps.

## CampCook02 — current verified cook

Output Artifacts/VQ02_CampCook1/CampCook02. Native exit0 at2026-10-21T14:34:56Z,
stderr0,537cooked/0incremental/7platformskips/544total,0errors/0warnings.

- SkipZenStore selects the supported loose package writer for inspectable bytes;
  it does NOT disable/change the common Zen derived-data cache/service or security.
  Installed CookCommandlet.cpp425–427 gates this storage selection.
- Command-line Engine override adds D3D12TargetedShaderFormats=PCD3D_SM6 with the
  documented '+' array-add syntax; existing SM5 retained. GenericWindowsTargetPlatformSettings
  reads these arrays; ConfigCacheIni.cpp's command-line override code supports
  ArrayAdd. No project/Windows config file edit or packaged-defaultRHI inference.
- Global AND RomanEmpireGame native shader-library stats/actual .ushaderbytecode
  archives exist for SM5andSM6, not just requested flags. SM6Global102093641bytes;
  SM6Game20804320bytes. They are cook artifacts, not rendered-player proof.

Actual Cooked/Windows/RomanEmpireGame/Content/ArtCandidates/Camp01 contains
12.uasset+12.uexp+3.ubulk, all nonempty. Native runtime registry has exact asset
classes3StaticMesh/6Material/3Texture2D. Mesh tags preserve1152/3704/46602triangles,
oneLOD and0simplecollision primitives. Texture tags2048x2048, BaseColor sRGB,
ORM/Normallinear, NormalTC_Normalmap and otherTC_Default.

Texture export byte observations: BaseColor/ORM each2793472bulkbytes, Normal5586944.
ASCII PF_DXT1/PF_DXT1/PF_BC5 format tokens observed in the corresponding .uexp files.
This is a named serialized-token observation, NOT a full package/texture decoder,
pixel comparison or player load; registry Format is “unknown”, not substituted
with a guessed format value. No mip/streaming/runtime performance approval.

CollisionPrims0 does NOT mean the mesh cannot ever collide: native tags retain
DefaultCollisionBlockAll, complex collision metadata and section collision flags.
Prior transient art components explicitly disabled collision/nav. Functional
collision/navigation of a future playable camp remains a separate OPEN gate.

## Registry, shader identity and verifier

Native DumpAssetRegistry run sequentially on CURRENT Runtime AssetRegistry.bin
and Metadata/DevelopmentAssetRegistry.bin; both exit0/stderr0, input SHA unchanged.
RegistryRuntime01/RegistryDevelopment01 contain8native sections each. Runtime
graph intentionally absent; Development dump supplies actual package edge flags.
Tools/verify_cooked_camp.py parses native sections/UTF8 or BOM UTF16 strictly:
object/package/class/tag tables, forward Package edges (not reverse References),
primary asset identifiers such as Map:/Game/... as distinct graph roots.

The3mesh roots' hard Game closure has35identifiers:34cooked content packages,
1runtime script identity /Script/NavigationSystem. ALL content members appear
in actual cooked runtime registry, no missing hard-game dependency or editor-test
package/module. EditorOnly edges are excluded explicitly; this is a HARD-game
closure, not a claim that all possible soft/dynamic/runtime loading is proven.

SM6ShaderAssetInfo native schema2 maps ALL6camp materials to shader-map identifier
76399F7F0EFDD3A7 in the current GameSM6library. Identifier is the native opaque
16hex value, NOT a SHA256 claim. Actual archives/index/log stats and hashes are
bound together; no claim of an independent shader-archive deserializer.

verified01.json:12asset records/tags,35closure,0missing,6SM6materials,27camp
package/export/bulk file hashes plus shader/registry/native dump/report inputs,
169Content/14source integrity. Player/owner/main_game_adopted flags remainFALSE.

Initial verifier probes rejected native UTF16 and then a primary Map:/... root;
reader corrected without regenerating cook/dumps or relaxing required assets.
12targeted tests PASS: UTF8/UTF16LE/BE/invalid bytes, primary-ID boundary, reverse
references excluded, malformed dependency/duplicate sections/classes/tags/missing
section rejected, cycles/editor-only/soft distinction, all6shader mappings,
missing shader material/schema/invalid identifier rejection.

cohort01.json binds verifier/cooker/test source hashes, verified01.json, native
registry exit/input hashes and saved12test log/exit0. Existing verified evidence
file hashes rechecked before creating this receipt. Repository validation0openM1
warnings, Python/PowerShell syntax and git diff --check pass. No own UE process
left alive; no release-package/candidate/source regeneration was needed.

## Reproduce only for a relevant change

Do not rerun CampCook01/02 or replace receipts. Existing verified01.json is ready
to read. For new source/runtime/shader changes, use a fresh cook name:

```powershell
.\Tools\cook_semireal_camp.ps1 -Name <fresh>
# Sequential native commandlet exports of both actual cooked registries:
# -run=DumpAssetRegistry -Path=<actual.bin> -OutDir=<fresh RegistryRuntime01 or RegistryDevelopment01> -All -LinesPerPage=100000
py Tools/verify_cooked_camp.py --name <same-fresh>
py -m unittest discover -s Tools -p test_verify_cooked_camp.py
```

Keep native dump PID/exit/inputSHA receipts named registry-Runtime01-* and
registry-Development01-* as in Cook02; verifier refuses existing verified01.json.
Output evidence is local/ignored; no automatic uploads or distribution.

## Remaining scope / next work

Cook gate for this PRIVATE content is technically verified. Owner visual direction
question sent asynchronously; pending an actual answer, no silence-as-approval.
Do not extend/adopt the camp across the main map or widen to other factions.
Real input/nav/collision/battle, new-environment50/100performance, trusted actual
Development/Shipping QA, security/rights/notices and M7/M8 remain OPEN.

Independent next task: C-018H_FontNoticeMap, existing exact packaged-font metadata
to notice/copyright worksheet without re-extraction, build/cook or legal approval.
Read only M7_FONT_ATTRIBUTION_REVIEW.md's current pending-scope and existing
FontAttribution2 receipts; do not restart upstream/VM/proxy/security investigations.

## Camp02 / semireal_v2 — current revised private cook

The Camp01 sections above remain historical evidence, not proof for v2. The
revised camp now has its OWN fresh directed cook at
`Artifacts/VQ02_CampRevision2/Cook/Camp02Cook01` in the existing GamePreview.
No old cook, source, Content asset, descriptor, Config or engine file was replaced.

`Tools/cook_semireal_camp.ps1 -Camp Camp02 -Name Camp02Cook01` binds the pinned
v2 manifest and Camp02 namespace. Default Camp01/v1 and its output root remain
available. The current private project legitimately contains181 protected files;
Camp01 also accepts its original169-file state. The new process records its exact
camp/revision/manifest hash. Loose package writer and additional SM6 are still
process-only: SM5 remains, Zen cache/service and persistent settings are unchanged.

Native cook PID42856 exited0; cook stderr empty, log explicitly reports
`Success - 0 error(s), 0 warning(s)`. Actual totals:537cooked,0incremental,
7platform-skipped,544total. All181Content files,14sourcev2 files, private Config
and descriptor remain hash-identical. No build, player or full test suite rerun.

New reusable `Tools/dump_cooked_camp_registry.ps1` exported the ACTUAL Runtime
and Development registries sequentially: PID35304/53456, both0, stderr empty,
native logs0errors/0warnings, each input registry hash unchanged. It refuses
active UE/UBT, linked inputs, mismatched cooks and existing output/receipt paths.
One initial preflight failed before creating outputs or launching UE because
FileInfo has Directory, not Parent; corrected before either successful native run.

`py Tools/verify_cooked_camp.py --camp Camp02 --name Camp02Cook01` passed:

- 12actual cooked assets:3StaticMesh,6Material,3Texture2D, all correct classes.
- Triangles1152/3704/28130;1LOD,0simple collision primitives. This does NOT imply
  all collision metadata is absent or validate physical collision/nav.
- Shared textures2048x2048; base color sRGB, normal/packed map linear with their
  expected compression settings. Actual12uasset/12uexp/3ubulk files are present.
- Native Development registry hard-Game closure35identifiers,0missing in cooked
  Runtime registry, no editor test dependency. Runtime intentionally strips its
  dependency graph; no conclusion about arbitrary soft/dynamic production loads.
- All6Camp02 material paths associate with actual SM6 shader-map identifiers;
  Global+game shader archives exist for BOTH SM5/SM6 with native library stats.
  Opaque shader identifiers are not represented as cryptographic content hashes.

15verifier tests PASS (original12 plus default/profile separation, invalid source
selection and rejection of Camp01 shader mappings as Camp02 evidence).
8source-policy tests PASS; Python compile, both PowerShell AST checks, repository
validation0M1warnings and diffcheck PASS. Existing unrelated line-ending warnings
remain. No native C++/full suite/build rerun was warranted by these tools changes.

`cohort01.json` SHA256
`8839824ea41b73e0d4f51b1483d98af61422796daf5722cabeb3d5cb947d006a`
binds the create-only closure script, verified01.json, previous actual preview
cohort, helper identities and both native dump journals/logs. All jobs terminal.

Reproduction only after a relevant change, with a NEW name:

```powershell
.\Tools\cook_semireal_camp.ps1 -Camp Camp02 -Name <fresh>
.\Tools\dump_cooked_camp_registry.ps1 -Camp Camp02 -Name <same-fresh> -Kind Runtime
.\Tools\dump_cooked_camp_registry.ps1 -Camp Camp02 -Name <same-fresh> -Kind Development
py Tools/verify_cooked_camp.py --camp Camp02 --name <same-fresh>
```

This closes PRIVATE revised-content cook verification only. Owner review of the
actual new RTS/FPS views is still pending; do not repeat the answered style/area
questions, adopt the main map by silence or reinterpret cook as packaged-player
QA. Next: owner adoption decision, then approved fit/offset/placement/nav/input
work, or another requested source revision. Environment performance, trusted
Development/Shipping QA, QUIC/security, rights, CI and M7/M8 remain OPEN.

## Camp03 / semireal_v3 — current private cook

Fresh output `Artifacts/VQ02_CampRevision3/Cook/Camp03Cook01`, selected Camp03/v3
only. Prior cook outputs preserved. Helpers now accept all three explicit profiles;
legacy defaults/output roots remain. Legacy count guards additionally accept the
current193-file private cohort, rather than rejecting coexistence with Camp03.
The Python verifier reuses the pinned source policy's triangle profiles.

Native cook PID46116 terminal0, stderr0, native log0errors/0warnings.
537cooked/0incremental/7platform-skipped/544total.193Content FILES,14sourcev3 files,
private Config and descriptor unchanged. No engine/Zenservice/persistent shader/
default-map change. Loose writer and additional SM6 are process-only; SM5 retained.

Actual Runtime/Development registry exports PID45716/16056 sequential, both0,
stderr0/log0errors/0warnings, actual input binary hashes unchanged. Verifier passed:
12assets/classes/packages, triangles1152/3704/30554,1LOD/0simple collision metadata,
3shared2048textures with correct sRGB/compression,12uasset+12uexp+3ubulk bytes.
Development hard-Game closure35identifiers,0missing in cooked runtime registry,
no editor-test dependency. Runtime graph is stripped; arbitrary soft/dynamic
production loads are not proven. All6Camp03 materials associate with SM6 shader
identifiers; actual Global+game archives/statistics exist for both SM5 andSM6.

17verifier tests PASS (prior15 plus third profile separation and rejection of
Camp02 shader mappings as Camp03 proof). Pythoncompile/2PSAST,repository0M1warnings
and diffcheck PASS; existing line-ending warnings unchanged. No fullsuite/build/
player/package launch warranted for these tool-profile changes.

`cohort01.json` SHA256
`65c9887635090960ed34a8215c213f6f07698348df0b48aa1247cf07bf9afdc9`
binds verified01.json, actual preview cohort, current helper identities and native
registry journals/logs. All jobs terminal; no handle to wait/restart. Reproduce only
after relevant change with a fresh name, replacing Camp02 with Camp03 in the
command recipe above. Existing verified01.json is create-only and must not rerun.

Cook closes this PRIVATE content gate, not standalone production reachability,
owner art approval, functional placement/nav/spawn, performance or release.
NEXT C-018H_BuildingPresentationAdapter: optional data-driven building appearance
within existing authorities, first verified privately, legacy defaults unchanged.
Do not adopt Camp03/mainmap, enlarge footprints or move spawn implicitly. M7/M8,
security/rights/CI/trusted Development+Shipping player QA remain open.
