# Private semireal performance — Editor cohorts

2026-10-20. C-018H_PrivateGPUPerformance: capture/analysis implementation and first
50/100 default pair and explicit full-scale diagnostics verified below.
Target acceptance remains OPEN; no player60FPS claim.

## Scope / workload

Existing `Artifacts/VQ01_Semireal1/GamePreview`; LOD01/IntegrationV3/Runtime05 PBR.
No main Content/Config/baselines, source meshes, collision/nav, gameplay clocks or
simulation changed. Runtime unit methods and UnitFoundationTests.cpp untouched.
New PRIVATE Editor observer `Tools/ArtProbe/Native/SemirealEncounterProfile.cpp`
uses explicit opt-in flags with the EXISTING Encounter50PIE/Encounter100PIE tests.
Those run canonical combat, reinforcement enrollment, natural training and one
recruit, actual FPS possession/realtime return, casualties, movement and physical
objective capture to victory.50 initially40Rome/10Carthage;100 initially90/10.
No alternative balanced battle was invented and no actors/animations were parked.

Native scope guards: exact private project, current matching Automation test,
actual D3D12/SM6, safe fresh namespace and desired50/100. Per-instance presentation
binds BEFORE Actor::PostSpawnInitialize through the installed world's
OnActorPreSpawnInitialization event. Only native soldier instances in the owned
PIE world are affected; CDO and assets remain unchanged. All registered soldiers
and the naturally trained recruit must use the candidate/native graph/actor-local
PBR parents. Source reference: Engine/Source/Runtime/Engine/Private/LevelActor.cpp
around753–755 broadcasts before PostSpawnInitialize.

Observer records per-world engine-frame identity, canonical registered count,
candidate mesh count, actor/animation ticks, movement, actual recent on-screen
render timestamps, projected centers, RTS/FPS, predictedLOD and effective dilation.
Registry invalidation rebuilds the weak actor cache, not per-unit world scans.
CSV separately measures observer cost; it is NOT subtracted from frame time.
One real1920x1080 viewport PNG is read before original CSV capture starts; no
pixel readback/forced render flush in measured frames. PredictedLOD telemetry is
labelled predicted, not falsely presented as render-thread GetLOD proof.

## First pair: valid, not accepted as final target

Native outputs under GamePreview/Artifacts/SemirealPerformance:
`Perf50_02` and `Perf100_01` folders contain PreCapture.png (both inspected),
started.txt, profile-receipt.txt and population.csv. Sibling named log/process/exit/
trace/CSV files preserve exact binaries, source hashes, power state and before-
Content manifest. Reports/Perf50_02 and Reports/Perf100_01 each show1successWith-
Warnings,0errors/failed/pending,exit0. The only selected-test warning is the known
MotionVectorSimulation render-thread warning, unsuppressed. Each real encounter
reaches physical victory; observe raw log separately from selected test status.

Hardware: chosen NVIDIA RTX5070Ti Laptop GPU, Intel Core Ultra9 275HX,
33752997888bytes RAM, D3D12/SM6,1920x1080 OUTPUT. Win32 battery status2,92% at
first capture (AC according to that status), recorded power scheme in process
receipts. All recorded scalability quality groups3; dynamic resolution0,
VSync0, t.MaxFPS0, r.ScreenPercentage0 at pre-PIE start. These are observations,
NOT overrides. ScreenPercentage0 means default/automatic selection: native
internal resolution has NOT been established, so do not claim native1080p raster.

Latest valid analyses: `perf50-summary02.json` and `perf100-summary01.json`, schema2.
Full-population RTS frames only (candidate count equals alive, alive>=initial;
other camera/population windows reported separately):

| Measured quantity | 50 target workload | 100 stretch workload |
| --- | ---: | ---: |
| Native CSV frames, whole encounter | 4176 | 3776 |
| Joined population frames, whole encounter | 4175 | 3775 |
| Full-population RTS frames | 1138 | 626 |
| Actual population in that window | 1110frames50 +28frames51 | 626frames100 |
| FPS from mean frame time | 58.50 | 59.69 |
| Mean / p95 frame time ms | 17.094 /16.798 | 16.754 /16.781 |
| Mean / p95 game thread ms | 11.323 /13.693 | 14.506 /16.413 |
| Mean / p95 GPU ms | 3.421 /3.816 | 3.567 /3.996 |
| Mean observer cost ms (included) | .144 | .232 |

At least60 measured full-population RTS frames are required; both exceed that.
Native callbacks bound51/101 total soldier instances including the one recruit.
All actor/animation ticks and real-time dilation retained in full-load windows.
Screenshot proof uses all initial centers projected and every mesh recently on
screen, not just a large actor count in an empty viewport. SourceGLBs unchanged;
each wrapper verifies all157 private Content hashes after capture. Both raw stderr
files empty. Initial100 sample peaks100 despite natural101 total enrollment because
casualties may precede recruitment; do not confuse enrolled with live population.

These single Editor captures do NOT meet a strict <=16.6667ms mean/p95 assertion,
and should not be averaged together to disguise workload differences. Median frame
time is almost exactly16.6667ms in BOTH. GPU cost is much lower than that; do not
immediately decimate assets or lower materials based on the aggregate frame time.

## Preserved failed capture / parser regression

- Perf50_01 exit255/failed test: original native constructor resets its bootstrap
  per instance (UnitBase.cpp46–47); changing a CDO did NOT install the candidate.
  CandidateUnits0; full-load/PNG guards correctly fail. The original encounter
  itself still won (4198frames/70.4s). Those data are NOT new-art performance.
  Corrected per-instance pre-spawn binding, without live-unit repair. Failure,
  process receipt and trace remain untouched. Build01/02 pass15.98/14.05s;
  corrected hook build03 pass13.79s, strict no-unity/no-PCH,4actions each.
- `perf50-summary01.json` is INVALID/obsolete analysis: first parser required each
  row to equal the initial CSV header width, dropping later frames and returning
  only185 samples. Do not consume it. Native FCsvStreamWriter appends stat columns
  and writes a final UNION header before metadata (CsvProfiler.cpp3122–3153).
  Corrected parser checks append-only headers, handles valid shorter early rows,
  retains ALL4176 numeric frames and reconciles canonical measured-frame receipt.
  New `perf50-summary02.json` supersedes it, original file preserved.
- `Tools/test_summarize_semireal_profile.py`:7/7PASS, dynamic header growth,
  reordering/duplicate/short/overwide/nonfinite/zero-GPU rejection and quantiles.
  Quantile convention remains floor((n-1)*p), matching existing summarizer.

## Historical investigation — superseded by runtime observations below

Read-only installed engine evidence suggests an Editor limiter, but native runtime
confirmation remains needed:

- UnrealEd/Private/EditorEngine.cpp2523–2566 caps Editor at60 when
  FPlatformMisc::IsRunningOnBattery() and r.DontLimitOnBattery==0; also consults
  smoothing bounds. t.MaxFPS0 alone does NOT establish uncapped operation.
- Core/Private/Windows/WindowsPlatformMisc.cpp4295–4317 calls battery PRESENT
  flags (including charging) “running on battery”, without checking ACLineStatus.
  The laptop has a battery/AC status2. This is a source-grounded inference, not
  confirmation of the runtime flag/CVar value in these particular samples.
- Record native runtime battery classification, r.DontLimitOnBattery, smoothing
  bounds and active t.MaxFPS during measurement, not only pre-PIE values.
- Observe the actual fixed screen-percentage driver fraction and secondary
  fraction via an OWNED read-only view extension. Do not infer full resolution
  from viewport dimensions or r.ScreenPercentage0. Installed GameViewportClient.cpp
  around1993–2011 creates a two-argument fixed LegacyScreenPercentageDriver when
  dynamic resolution is off; public resolution upper bound then equals the fixed
  fraction (LegacyScreenPercentageDriver.cpp113–160). Document any inference.
- Only after those observations decide whether a clearly labelled process-only
  uncapped DIAGNOSTIC cohort is useful. Do NOT change Windows power/security,
  persistent settings, quality, camera, AI/animation ticks or actual unit load.
  Keep default baseline and diagnostics distinct. Repeat valid50/100 cohorts for
  variability; measure by camera/population, no one-pair player acceptance.

D-05 remains60FPS1080p with50 slice units plus SEPARATE100-unit stretch; no invented
mandatory60FPS100 requirement. Main art/owner acceptance, uninterrupted visual
polish, trusted Development/Shipping QA and M7/M8 remain OPEN.

## Runtime observation and full-scale diagnostic — 2026-10-20

Native observer now uses an owned FWorldSceneViewExtension, game-thread
BeginRenderViewFamily, public primary resolution upper bound/secondary scale and
output rect; reads only, no render/camera/quality mutation. During CSV it records
runtime battery classification/smoothing/t.MaxFPS/ScreenPercentage/dynamicres/VSync.
Strict private perf-build04 passed4actions/16.35s; no later native source changes.

- Default Perf50_03: exit0,1successWithWarnings/0errors/failed/pending. Runtime
  battery1/DontLimit0/t.MaxFPS0/smoothingfalse confirms the installed Editor
  battery-cap predicate. Output1920x1080 but primary upper.728869–.846294 and
  secondary1 across3697 view families: do NOT call this fixed100% native raster.
  perf50-summary03.json preserves schema3 analysis; no target acceptance.
- Opt-in wrapper switches -NativeResolutionDiagnostic and
  -DisableEditorBatteryCapDiagnostic set process-only ScreenPercentage100 and
  DontLimitOnBattery1. Both defaultOFF; no persistent/Windows power/security change,
  no lower quality, actor/animation parking, LOD threshold/camera/gameplay change.
- Perf50_NativeDiag01 did NOT start Automation: launcher incorrectly joined startup
  CVars using semicolons. Log showed first CVar consuming the rest. Verified only
  owned PID35900, normal-close request returnedfalse, stopped that owned idle
  process; exit-1 preserved. No PIE/asset mutation. Startup now uses commas while
  Automation's internal queue retains RunTest;Quit.8purePS combinationsPASS.
- Perf50_NativeDiag02: exit0 at14:37:39UTC;1successWithWarnings/0failed/pending,
  stderr0bytes. Runtime battery1/DontLimit1/t.MaxFPS0/smoothingfalse/VSync0/dynres0,
  ScreenPercentage100. Primary upper min=max1, secondary1, output1920x1080,
  6109 observed view families. Installed fixed legacy driver implies fixed100%
  scale: this is source-based inference, NOT protected actual getter/GPUbuffer
  size readback. Analyzer explicitly distinguishes these claims (schema3).

Fresh analysis `perf50-native-diag02-summary01.json`:6070native CSV/6069joined
frames,1597full-populationRTS (1555with50alive,42with51), all projected centers
and actor/anim ticks minimum50; recently-on-screen minimum30 (natural occlusion,
not forced visibility). FPS81.9898 from mean FrameTime12.196635ms; p9514.0715,
p9914.9707,max36.6845. Game mean12.196533/p9513.7896, render5.448350/p956.2569,
GPU4.307862/p954.7716, observer.177227ms INCLUDED. Native51binds beforeInit.
PreCapture.png inspected at1920x1080: canonical camera/population, new soldiers,
legacy checker camp still present; no claim that environment art is complete.
Mean/p95 meet provisional60FPS budget in this sample, NOT player/owner acceptance.

Analyzer tests11PASS (includes missing-field cap predicate and resolution guards);
launcher8PASS.100-unit diagnostic and independent repeat cohort remain next;
all results must stay separate from default capped/sub-native measurements.

Perf100_NativeDiag01 subsequently completed: exit0,1successWithWarnings/0failed/
pending, stderr0,157Content files hash-identical. Fresh
`perf100-native-diag01-summary01.json`:3633CSV/3632joined;556fullRTS frames100alive,
FPS51.6842,FrameMean19.348270ms,p9521.5911,p9923.3646. GT19.366790, render6.890328,
GPU4.536476,observer.326504ms INCLUDED. Minimum actor/anim ticks/projected centers
100, recently-on-screen29 (natural scene).101binds includes recruit, peakalive100
after natural casualties. Same full-scale inference/settings as50, screenshot
inspected:90Rome+10Carthage, canonical scene, unfinished checker environment.
The100-unit STRETCH does not meet60FPS here; CPU/game-thread-dominated timings,
not proof of a specific hot function without inspecting the trace. No fabricated
mandatory60FPS100 gate, no visibility/AI/LOD/quality adjustment to make it pass.
Independent repeat50/100 follows to quantify variability.

Repeat50 (`Perf50_NativeRepeat01`, `perf50-native-repeat01-summary01.json`):
exit0,1successWithWarnings/0failed/pending,stderr0,157Content hash-identical.
6758CSV/6758joined,1654fullRTS frames,84.8018FPS,FrameMean11.792202ms,p9513.5487,
p9914.6103. GT11.788531,render5.195512,GPU4.330248,observer.182217ms included.
Same full-scale inference and process-only flags; screenshot inspected. Across
these TWO50 runs meanFPS81.99–84.80 and p9513.549–14.072, both within provisional
60FPS budget. These are independent runs, not an averaged pooled population;
different combat/casualty timing gives different sample counts. No statistical
confidence/thermal endurance or packaged-player acceptance inferred from two runs.

Repeat100 (`Perf100_NativeRepeat01`, `perf100-native-repeat01-summary01.json`):
exit0,1successWithWarnings/0failed/pending,stderr0,157Content hash-identical.
4595CSV/4594joined,723fullRTSframes100alive,65.5272FPS,FrameMean15.260841ms,
p9516.9302,p9918.0057. GT15.276083,render5.945512,GPU4.726595,observer.252128ms
included. Minimum actor/anim ticks/projected centers100/recently-on-screen29.
Same settings/full-scale inference, screenshot inspected. Mean improves markedly
over the first100 run but p95 STILL exceeds16.6667ms. Do not cherry-pick this
mean, pool runs or infer a cause for21% frame-time variation without trace/power/
thermal evidence. Both100 captures remain stretch evidence, not acceptance.

| Full-population RTS capture | Frames | Mean FPS | Frame p95 ms |
|---|---:|---:|---:|
| Perf50_NativeDiag02 |1597|81.99|14.072|
| Perf50_NativeRepeat01 |1654|84.80|13.549|
| Perf100_NativeDiag01 |556|51.68|21.591|
| Perf100_NativeRepeat01 |723|65.53|16.930|

`native-diagnostic-cohort01.json` records all4captures plus summary/process/exit/
log/stderr/CSV/trace/receipt/population/PNG SHA256 and tool hashes. It checks SAME
observer/native test DLL and157currentContent hashes against EACH capture's
before-manifest. This is NOT the old default `cohort01.json`; neither overwritten.
All4 Automation reports contain only the known MotionVectorSimulation warning;
no native error/failed/pending.11analyzer/8launcher guards, repository validation
0openM1warnings and git diff --check pass. No main runtime/source/asset change,
no build/cook/full141suite repeated in this block; existing binary reused.

Technical Editor measurement work is ready for handoff; no further identical
captures needed absent a relevant change or specific trace hypothesis. Remaining:
packaged-player/owner/reference-hardware gate, continuous visual review, actual
camp art and isolated trusted QA/security/notices. Independent next art work is
G-VQ02_CampSource1: bounded terrain+wall+onebuilding proposal per visual brief,
NOT broad map adoption or faction redesign. Player/M7/M8 remain OPEN.
