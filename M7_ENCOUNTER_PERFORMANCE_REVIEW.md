# C018E — finished-encounter workload and provisional performance

2026-10-08. Local workload/capture/trace/regression/strict-build evidence VERIFIED. M3.7/M7 remain OPEN. No packaged-player or agreed FPS-budget claim.

## What actually runs

Fresh `/Game/Maps/L_Testbed` PIE, 50 or 100 INITIAL TOTAL soldiers: original25 plus25/75 Roman Legionaries, each canonically enrolled as a singleton reinforcement. Initial factions are40/90 Romans and10 defenders, not balanced50vs50 armies. Initial setup relocation is outside the capture; no teleport, forced damage, manual training tick, fake capture or collision bypass is used to complete the measured encounter.

After30 camera/world warm-up frames, the real barracks trains one Legionary on natural ticks while actual RTS/AI attack commands produce observed enemy damage. A real1-second first-person possession and return keeps time dilation1/world unpaused. Natural combat updates canonical casualties; one eligible survivor nearest the objective receives a real Move order and enters the physical occupation zone. The objective's own timer resolves PlayerVictory with the original encounter/objective IDs.

CSV is opt-in (`-RomanCaptureEncounter`); normal regression does not generate these captures. Counters record phase, sampled live units/casualties, training and observed-damage-during-training. Readiness/command/query assertions contribute fixture overhead. Snapshots are refreshed at0.5s intervals; the retained overlap flag is NOT a per-hit counter or exact concurrent-combat duration.

Phases:2=RTS combat/retargeting/training wait;3=realtime FPS;4=physical objective approach;0=CSV start/stop latency. Different phases and live counts make the whole-encounter quantile different from sustained100-live-unit combat.

## Six actual captures

Three independent D3D12/offscreen processes per total count. All6 native exit0 sidecars and automation reports pass, with the known engine MotionVector warning and connectivity-check HTTP timeouts recorded. The actual gameplay viewport is asserted1920x1080, and all40/90 initial Roman actor CENTERS project into it. This is not mesh-pixel visibility, a guarantee about all10 defenders, or100-visible-unit proof. Zoom.47 differs from the older move-only fixtures; do not compare them as an identical workload.

Immutable raw copies, SHA256s, phase quantiles, receipts and traces: `Artifacts/Profiling/C018EVerified/summary.json`. Full raw traces total about1.77GB. Old accepted `C018B/summary.json` and its six move-only captures were not overwritten.

Lower-index p95 over each independent capture's finite numeric CSV rows; milliseconds, except draw-call sum:

| Total/run | Frames | Frame | GameThread | RenderThread | GPU | Draw calls | Final live/casualties |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | --- |
| 50/1 | 2776 | 54.61 | 55.29 | 12.89 | 9.80 | 1242 | 39/12 |
| 50/2 | 3641 | 23.27 | 22.93 | 11.49 | 4.39 | 1240 | 39/12 |
| 50/3 | 3910 | 22.47 | 22.36 | 11.11 | 4.36 | 1242 | 39/12 |
| 100/1 | 2311 | 35.46 | 35.41 | 12.41 | 9.49 | 2289 | 90/11 |
| 100/2 | 2379 | 33.42 | 33.14 | 12.03 | 9.54 | 2289 | 89/12 |
| 100/3 | 2645 | 27.67 | 27.32 | 10.94 | 4.51 | 2289 | 89/12 |

Each final-live + casualties equals total+one naturally enrolled recruit. The sampled peak is bounded total..total+1, not an exact every-tick maximum. All phases and nonzero GPU timing are validated; observed-damage flag while training persists for263–475 CSV frames. CSV/latent frame counts permit at most8 frames of asynchronous start/stop latency.

The50/1 outlier is retained, not removed or pooled away. Host memory pressure/cold Editor/background activity and the frame cap prevent a stable player-budget conclusion. UBA reported66.73GB committed against31.43GB physical during the preceding build; process working-set snapshots are not game allocation budgets. No unrelated processes or Windows protections were changed.

## What the traces support

Native `TRACE_BEGIN_REGION`/`TRACE_END_REGION` delimit `RomanEncounter50/100`; region channel explicitly enabled. Six Unreal Insights exports use that region and the GameThread CPU filter. Their hashes, actual region boundaries and selected named CPU scopes are in `trace_analysis.json` alongside the summary.

Important engine5.8.2 limitation verified in installed `TimingExporter.cpp`: ExportTimerStatistics applies the CPU-thread filter but unconditionally includes GPU queues. Files named `_GameThread_RomanEncounter*.csv` therefore contain CPU-filtered PLUS GPU statistics. Do not sum all rows as CPU costs, or interpret their combined `Frame` row as a GameThread frame. Selected distinct CPU scope names below avoid that ambiguity. Inclusive parent/child totals also overlap.

- CharacterMovement exclusive total normalized by CSV frames:1.98–2.30ms/frame for50,3.82–4.58ms/frame for100. Per-invocation average in the export is a different statistic.
- Slate's inclusive frame tick:4.18–5.06ms for50,4.60–5.16ms for100.
- `UMassEntityEditorSubsystem::Tick` inclusive frame tick:1.60–7.65ms for50,2.12–3.22ms for100. This is an EDITOR subsystem, not proof this game needs a Mass rewrite.

CPU/world movement and whole-Editor overhead deserve a trusted-player comparison. These data do not justify changing gameplay authority, collision, correctness checks, animation quality or architecture to meet an unagreed target. No runtime optimization was made.

Insights logs retain engine duplicate/unclosed `Session` region warnings; the owned encounter region exports exactly once with actual start/end boundaries. The first incorrectly quoted export invocation produced no CSV and was rejected despite process exit0; `_Insights.log` remains, corrected `_Insights2.log` contains the actual commands/exports. There was no recapture to fix this export-only issue.

## Failures and verification

- Logic50 attempts1/2 timed out at occupation; the full-army footprint or first idle soldier was unsuitable. Attempt3 exposed actual Unreachable navigation. The final fixture chooses the closest eligible survivor, and checks real Move/route/outcome. Teardown timeout now begins on its first latent Update, not when the entire command queue was constructed.
- Directed logic50 attempt4 and logic100 attempt1 pass; connectivity-check HTTP timeouts are reported, not suppressed. NullRHI center count0 is not raster evidence.
- First rendered probe `C018E_Encounter50_Run1` actually passed, but its PS5.1 wrapper saw a null ExitCode. Its raw338375535-byte trace/CSV/log remain separate and unapproved as a final series. Native process handles are now retained before waiting; child exit0/7 checks prove receipt handling.
- A premature `C018EFinal` capture overlapped Editor8 build; the owned child was stopped and this evidence excluded. No other processes were killed. The launcher now rejects active Unreal editors AND UBT; the accepted C018EVerified batch began only after Editor8 terminal exit0.
- Real CSV included duplicate unmeasured `TextureStreaming/RenderAssetStreamingUpdate` columns. Reader aliases and preserves their positions, rejects ambiguous measured/counter columns including case-only duplicates, bounds input512MiB and retains only needed parsed columns.39 pure parser checks pass; all6 real CSVs validated. Summary/capture outputs refuse overwrite.
- Editor1–8 builds pass; final8 has4actions/114.03s. Runtime/Game configuration/assets/dependencies unchanged. Repository check1 passes. Launcher requires15 exact coverage anchors including both encounters.
- Actual full `BuildProject -Action Test -DisableFab` exit0:133success (129clean/4only connectivity HTTP-timeout warning reports),0failed/notRun/inProcess. Report `Artifacts/Automation/20261008_163756_281/index.json`,15required anchors; log `C018E-FullTestFinal.log`.
- Strict IWYU `C018E-IWYUFinal.log` exit0/111actions/82.40s. Four touched PowerShell ASTs clean,39pure checks, repository final0M1warnings/LFS/diff pass; all77accepted source asset hashes unchanged. Existing CRLF warnings are recorded in `C018E-DiffWarningsFinal.log`, not mistaken for a nonzero native diff exit.
- All processes terminal. Previous C018D cook/test-map exclusion and unchanged runtime C018C Shipping evidence reused: no runtime/Game/assets/dependency changes require repeating them. This is NOT a fresh trusted packaged-player gate. Next C018F rendering regression/reviewed baselines; M3.7/M7 and owner/remote/release gates remain open.

## Reproduce without overwriting evidence

Build Editor first and wait for terminal completion. Use a NEW prefix:

```powershell
.\Tools\capture_encounter_profiles.ps1 -CapturePrefix NewEncounter -Runs 3
.\Tools\summarize_unit_load.ps1 -Encounter -CapturePrefix NewEncounter -Runs 3
.\Tools\tests\test_unit_load_summary.ps1
```

For native Insights exports, use `-NoUI -AutoQuit -OpenTraceFile=<existing-trace>` and quote ONLY the value of `-ExecOnAnalysisCompleteCmd="TimingInsights.ExportTimerStatistics <new-output-with-{region}> -threads=GameThread -region=RomanEncounter50 -sortBy=TotalInclusiveTime"`. The filename must include `{region}` for a region export. Validate the actual output and region receipt, not just exit0. Do not run analysis/builds while a capture batch is active.

Remaining M3.7/M7 performance gate: agree target/hardware/visible workload with the owner, obtain a trusted packaged runner, reproduce full encounters under controlled host conditions, then make evidence-directed optimization decisions. Rendering regression baselines, remote CI/governance, owner trust/signing/licensing and distribution gates remain separate.
