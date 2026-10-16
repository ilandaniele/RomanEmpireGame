# C017D — material assets and actor integration, verified

2026-10-06. M6.4 closed with actual actor-state/render/cook evidence; not whole M6/goal completion.

`Tools/author_visual_materials.py` schema1 creates missing assets only under `/Game/Materials/RomanVisual`; existing assets are never overwritten/rebuilt. Ownership/version/type, connected required outputs, declared parameters, actual parent and exact instance values are verified. Editor-only generation; no runtime generation, external images/textures or gameplay mutations.

| Asset | Actual contract |
|---|---|
| M_RomanSurface | Lit, connected Color × damage-darkening factor, Roughness, Metallic |
| M_RomanPreview | Unlit/translucent, connected Opacity and Color, optional repeating dark/light stripe shader via PreviewPattern |
| M_RomanIndicator | Unlit connected Color emission for geometric selection indicator |
| MI_Surface | Opaque default surface; actor MID will provide current color/state |
| MI_Construction | Warm construction presentation |
| MI_Damaged | Surface damage-darkening parameter1 |
| MI_PreviewValid | Green, actual opacity0.5, pattern0 |
| MI_PreviewInvalid | Red, actual opacity0.5, pattern1; non-color stripe distinction |
| MI_Selection | White emission; geometric ring/outline must supply the shape |
| MI_FactionRome/Carthage/Gaul | Existing canonical PDA-color snapshots; no alternative faction authority |

All bases support skeletal mesh usage for later M6.7. Simple shaders are not a finished art theme or a replacement for readable meshes/animation.

## Evidence and commands

The original asset-authoring block changed no native runtime. Actor integration and its separate evidence are recorded below; neither parameter readback nor a passing test replaces inspected rendering. No new trusted-player gate is claimed.

- Actual authoring: `Artifacts/C017D-MaterialAuthoring-Readback.log`, exit0,0errors/0warnings.3 bases and9 instances saved through Unreal.
- Fresh readonly processes: `C017D-MaterialVerify1.log`, `C017D-MaterialVerify2.log`, each exit0/0errors/0warnings; type/schema/property connections/parent/all values loaded from saved assets and checked.
- Transient changed value/unowned tag/missing reference are each rejected, then original values restored without saving: `C017D-MaterialRejections.log`, exit0/0errors/0warnings,3 expected rejection probes. SHA256 unchanged.
- Repeat actual `--apply`: `C017D-MaterialApplyRepeat.log`, exit0/0errors/0warnings, all created=False. SHA256 of all12 unchanged. Hash evidence `Artifacts/C017D-MaterialHashes.json`.
- Repository0M1warnings/LFS/diff pass. Asset LFS attribute confirmed; no staging/commit/push/security/config/plugin-install change. CLI enables existing Python/EditorScripting plugins per-process only; no MCP operation.

```powershell
$visualScriptArg = '-script="' + (Join-Path $PWD 'Tools/author_visual_materials.py') + '" --apply'
& 'C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe' "$PWD\RomanEmpireGame.uproject" -run=pythonscript $visualScriptArg '-EnablePlugins=PythonScriptPlugin,EditorScriptingUtilities' -DisablePlugins=Fab -unattended -nop4 -NullRHI
```

Use no trailing mode for readonly verification; `--test-rejection` for readonly negative probes. Probe mode refuses combination with `--apply`. Do not run during another Editor/PIE/build. No asset/state reset needed to repeat.

Corrected first-run issues: PowerShell's comma argument required quoting; commandlet working directory required quoted absolute script path. Single-input expression pin names use empty first-pin selection, not invented "Input" label. UE5.8 `MaterialEditingLibrary.cpp:1485/1573` setters always return unassigned false despite setting values; generator explicitly validates declared schema and exact real readback instead. No compile/error/check suppression. Earlier failed stages saved only correctly tagged/verified bases; later invocations verified/reused them, never overwrote unowned content.

## Actor integration and historical rendering diagnosis

Native integration is now implemented in `Building/BuildingBase.h/.cpp`, `Units/UnitBase.h/.cpp` and `Units/UnitSelectionPresentation.cpp`. Reflected class references load the actual persistent MICs; GC-tracked MIDs are reused during state updates, with no synchronous asset loads on gameplay events. Unit body/shield colors query the current canonical faction; skin/weapon keep their own surface instances, and committed health drives damage/restoration. Buildings preserve current preview validity, use construction/surface/damaged parents, project current definition color with 25% current faction tint, and reuse state instances through damage and repair. Four collision/navigation-free white outline edges supplement the unit's existing ring; deselection/destruction/EndPlay hide geometry. No eligibility, faction-availability, cost, construction-state or battle-rule changes.

Unused `Assets/MaterialManager.h/.cpp` and both Python Blueprint factory routes were retired: no C++ callers and no ASCII/UTF16 serialized class references in Content `.uasset`/`.umap`. Source deletion is Git-recoverable; no binary asset was deleted. Native reflected actor tests avoid leaking runtime-private headers. Legacy generators remain otherwise unvalidated; do not run full setup.

- Editor runtime build and fixture increments pass. First build's inherited `Mesh` name hiding and the test's incomplete typed soft-class access were corrected without relaxed compiler checks/private includes.
- `C017D_ActorStates1`: 1/1 clean; `C017D_Full1`: 104/104 clean, zero warnings/failures/pending/notRun. Real factory actors, current owners, transparency/stripe parameters, selection, health/repair, MID reuse, GC and lifecycle covered. `C017D_EffectiveShader` additionally passes connected shader-cache output assertions. Reports under `Artifacts/Automation/`, matching `Artifacts/C017D-*.log`.
- Strict IWYU94 and Shipping23 pass; final current-tree strictIWYU94 also passes (`C017D-IWYU-Final.log`), covering the final Editor-only diagnostic/render/pixel fixture. Final native targeted1/1 clean (`C017D_TargetedFinal`); full104 remains valid runtime regression before those Editor-only additions. Standard-library retirement tests5/5 pass (`C017D-Retirement.log`).
- Rendered parameter/state tests succeed with the known engine `r.MotionVectorSimulation` warning, but inspected orthographic **and perspective** scene captures show missing translucent previews and dark indicators. Thus visual distinguishability is NOT approved. Shader waits did not correct it. Effective unlit and connected emissive cache/inline flags pass; readonly NullRHI/RHI graph inspections show no FrontMaterial, so a Substrate conversion hypothesis is not established. No speculative material mutation/save/migration occurred. Native transient white-reference comparison is the next narrow diagnostic. Capture directory can be overridden with `-RomanMaterialCaptureDir=Artifacts/<fresh-folder>`; use a fresh path for each new capture.

Native transient-reference capture was inspected: original indicators become white while the new white reference cube appears dark; previews remain missing. It is evidence of a material/capture update discrepancy, not proof of its cause. Explicit proxy flush before/after shader wait, separate-translucency enablement and double capture did not fix the no-reference image; do not repeat those unchanged attempts. No saved graph/config changed.

`C017D_RenderedPixelGate` now correctly fails instead of silently approving that image: white=0 (requires >500), green=1182 (requires >4000, beyond the small green unit). The PNG is saved before assertions in `Artifacts/C017D_RenderedPixelGate/MaterialStates.png`. Missing preview and dark selection are two explicit test failures; this gate must stay red until real pixels and inspection pass. Pixel reads use the RGBA8 target's inline BGRA8 accessor, avoiding an unnecessary ImageCore link dependency (first CopyTo attempt's LNK2019 was corrected, no module change).

Cook passes: `C017D-Cook.log`, exit0, 572 cooked/0 incrementally skipped/7 platform-skipped/579 total, no Error/Warning lines. The cook uses Zen storage, not loose material files. Actual cooked `Saved/Cooked/Windows/RomanEmpireGame/AssetRegistry.bin` was parsed by the engine's `DumpAssetRegistry` commandlet, exit0/no errors or warnings; `Artifacts/C017D_CookedRegistry/Page_00000.txt` contains all12 exact material object paths. This proves cook inclusion, not standalone gameplay/visuals. All12 source SHA256 remain unchanged, repository0M1warnings/LFS/diff pass.

## Final visual gate — passed

The failure above was in the immediate capture fixture: retaining the actual actors/camera/render target across at least12 real engine frames and1 second fixes the same scene without changing any asset or gameplay code. Same-frame shader/proxy waits are not equivalent to ticking real frames. A20-second timeout and owned-scene/view-target cleanup bound the fixture. This establishes the necessary fixture latency, not a speculative engine/material-cache root cause.

`C017D_PlayerRegionsFinal` and `C017D_OrthoRegionsFinal` each succeed (one known engine `r.MotionVectorSimulation` warning). The former reads the actual PIE player viewport after `SetViewTarget` to a transient camera; the latter reads the actual orthographic render target. Both final PNGs inspected: green translucent valid placement, red striped invalid placement, yellow construction, darker damaged building, white building outline/unit disc, and canonical Rome/Carthage/Gaul colors all visible. Perspective scene capture `C017D_RegionsCapture1` also passes. These are in-process PIE evidence, not a trusted standalone player.

Pixel checks now project the actual six meshes' roof bounds into the real view: >80% green/red pixels in the respective preview regions, repeated invalid-only stripe transitions, distinct construction color and darker damaged roof. This avoids mistaking small green units or auto-exposure-washed translucency for preview failure. The earlier player comparison rejected visible pale-green geometry with a global saturation threshold; it remains failed historical evidence, not suppressed. White checks retain >500 overall and add actual selection-bound regions: final player building1475/unit1058 pixels, orthographic4497/3200. Roof0 green243/243 and867/867 respectively; invalid transitions198 and569. No saved material mutation, reference-material trick, dependency or expected-error suppression.

Final native `C017D_NativeLatentFinal`1/1 clean, strict `C017D-IWYU-LatentFinal.log`94 succeeded. Full104/Shipping23/cook572 and actual12 cooked-registry records above remain valid: only Editor fixture code changed. Next C017E/M6.5 audio. No active process retained, no Windows security change or repeated blocked unsigned player launch.
