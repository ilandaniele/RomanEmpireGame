# Preliminary 50/100-unit tactical load baseline

Status: reproducible Editor PIE CPU/render/GPU capture established; M3.7 remains open. Do not infer a packaged-player frame target or optimize from this single provisional sample.

## Fixture and captures

`RomanEmpireGame.Performance.Load50PIE` and `Load100PIE` spawn 50 or 100 transient Rome Legionaries on `L_Testbed`, select each entire group and issue one real controller formation Move across the dynamic NavMesh. They warm up 30 frames, then run `CsvProfile Start`/`Stop` around 240 frames and mark the interval in an Unreal Insights trace. Every added unit receives a Move and none reports an unreachable route. The loaded level also has 25 bootstrap Legionaries, so the recorded actor counts are 75 and 125. The fixture positions the tactical camera over the moving group at zoom 0.52; the D3D12 offscreen run reports 50/50 and 100/100 added units initially inside the viewport. This is Editor/PIE, not a packaged player.

Local reference workstation: Intel Core Ultra 9 275HX (24 cores/24 logical processors), NVIDIA GeForce RTX 5070 Ti Laptop GPU (D3D12, driver 592.01), 31.4 GiB system RAM, UE 5.8.2. The run uses `UnrealEditor-Cmd.exe -RenderOffscreen -DisablePlugins=Fab`, `-trace=cpu,frame,gpu,stats,bookmark,counters,memtag`, `-llm`, and 240 CSV frames per fixture. The CSV/trace files are preserved under ignored `Artifacts/Profiling/` as `M3_Load50_Framed.csv`, `M3_Load100_Framed.csv`, and `M3_Load_Framed.utrace`; test report: `Artifacts/Automation/M3_Load_Framed`, log: `Artifacts/M3_Load-Framed.log`. Separate NullRHI captures are retained but cannot supply render/GPU timings.

| Metric, per-frame p50 / p95 | +50 (75 total) | +100 (125 total) |
|---|---:|---:|
| Game thread | 9.63 / 10.68 ms | 11.98 / 13.01 ms |
| Render thread | 5.46 / 5.96 ms | 6.40 / 6.77 ms |
| GPU | 2.07 / 4.99 ms | 2.21 / 5.96 ms |
| Frame | 16.67 / 16.67 ms | 16.67 / 16.69 ms |
| CharacterMovement exclusive | 1.47 / 1.74 ms | 2.55 / 3.01 ms |
| `DrawCall/*` counter sum | 1,580 / 1,586 | 2,683 / 2,690 |
| Whole Editor process physical memory | 3,769 / 3,771 MiB | 3,765 / 3,768 MiB |

Percentiles use the 240 numeric CSV frames per capture; p95 selects the lower indexed 95th-percentile sample. Draw calls are the sum of CSV `DrawCall/*` category counters, including UI and rendering passes, not a standalone GPU driver counter. Process memory includes the Editor, automation and profiler. The 100-unit test ran before the 50-unit test in one process, so the memory figures are not incremental scaling evidence. The editor frame clock stays near 16.67 ms and masks headroom; Game/Render/GPU thread metrics are the useful preliminary comparisons. Only one 240-frame sample per load and one camera/scene were captured. Combat, production, objectives, final art and packaged-player behavior are not represented.

`AUnitBase::Tick` now queries the live-unit registry, not a world actor iterator. A targeted C++ search finds remaining `GetAllActorsOfClass` calls in manager initialization/turn handling or static manager getters, not per-unit tick; this is not a complete Blueprint/profiler proof that all world scans are gone. No Mass migration or speculative optimization was made.

The Unreal tracing channels and CSV workflow follow the [UE 5.8 Unreal Insights reference](https://dev.epicgames.com/documentation/unreal-engine/unreal-insights-reference-in-unreal-engine-5) and [UE 5.8 profiling overview](https://dev.epicgames.com/documentation/unreal-engine/introduction-to-performance-profiling-and-configuration-in-unreal-engine). [UE 5.8 stat commands](https://dev.epicgames.com/documentation/unreal-engine/stat-commands-in-unreal-engine) distinguish Game, Draw/Render and GPU timings.

## Verification and remaining gate

- Corrected Editor build and framed D3D12 targeted tests: `Artifacts/M3_Load-FramingBuild.log`, `Artifacts/Automation/M3_Load_Framed`, 2/2 pass.
- Full NullRHI regression: 34/34 pass, zero failures/pending, `Artifacts/Automation/20260929_192436_087`, log `Artifacts/M3_Load-Full.log`. Eighteen tests carry only the two known WASAPI sample-rate/conversion notices.
- Strict non-unity/no-PCH IWYU: succeeded, 63 actions, `Artifacts/M3_Load-IWYU.log`. Shipping target: succeeded and up to date with zero actions because this block changes only tests, `Artifacts/M3_Load-Shipping.log`. Repository validation, Git LFS integrity and diff whitespace checks pass.

M3.7 exit still requires a complete encounter workload, a target frame budget, repeated captures on reference hardware and a trusted packaged-player run with rendered stat/Insights comparison. The older unsigned player remains blocked by host Windows Code Integrity; it was not retried and no security policy was changed. Skill applied: `ue-testing-debugging` for repeatable live-unit PIE fixtures and evidence boundaries. Project context changes remain in the approved verification section.
