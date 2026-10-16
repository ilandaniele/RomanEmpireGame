# Current skeletal-asset load capture — C018B / provisional

2026-10-07, GPT. Provisional capture COMPLETE. Not M3.7/M7 exit or a performance promise.

Reuse the correct existing Load50PIE/Load100PIE fixtures, without changing them:
50/100 additional Rome Legionaries,25bootstrap (75/125total), real formation Move,
30warmup/240requested CSV frames, native current skeletal/material/UI/audio assets.
Other units hold; combat, active production and objective workload are NOT covered.
Original pre-art measurements remain historical in M3_PERFORMANCE_BASELINE_REVIEW.md.

New capture: three independent processes per load, D3D12/RenderOffscreen,
1920×1080, same trace channels/LLM as the earlier baseline. Logs and reports are
C018B_Load{50,100}_Run{1,2,3}; actual trace files are under
Artifacts/Profiling/C018B/. No NullRHI values are used as renderer measurements.
Each process must pass native assertions and strict actual report validation.

Tools/summarize_unit_load.ps1 validates actual report/CSV/trace receipts, full
added-group viewport projection, finite numeric CSV metrics and nonzero GPU
timings; excludes trailing repeated header/metadata rows, records sample counts,
lower-index p50/p95/max plus actual draw counter sums. Copies exact CSV outputs
with readback hashes without overwriting existing summaries/captures. This is
analysis of actual telemetry, not a log-string success proxy for game behavior.

## Actual evidence

All six independent processes exit0, each1success/0fail; only two WASAPI notices
and one known engine MotionVector warning per report, no suppression. All six
added-group actor-center projections are50/50 or100/100; this is NOT a per-mesh
pixel-visibility/screenshot assertion. Every capture has240 numeric CSV frames,
measured nonzero GPU times, a nonempty actual trace and validated report. Exact
copied CSV/trace hashes, individual p50/p95/max and sample counts:
Artifacts/Profiling/C018B/summary.json; helper exit0, C018B-Summary.log.

Ranges below are the lowest/highest per-run p95, not a pooled percentile:

| Added / total units | Game ms | Render ms | GPU ms | Frame ms | Draw-counter sum |
|---|---:|---:|---:|---:|---:|
| 50 / 75 | 13.36–14.41 | 5.99–6.31 | 2.64–5.08 | 16.67–16.71 | 1982–1983 |
| 100 / 125 | 17.57–19.13 | 7.26–7.37 | 2.72–2.80 | 17.80–19.55 | 3332 |

The game thread dominates these provisional100-added-unit samples; do not infer
a packaged-player bottleneck or rewrite architecture without inspecting the
actual trace and complete workload. Lower-load frame cap near16.67ms obscures
headroom; 100added means125total, not100total. Startup/caches, Editor process,
short samples and held non-group units limit comparison with the earlier sample.
No final FPS target has been agreed, and no complete combat/training/objective
scenario, trusted player or approved performance gate is claimed.

Pure real-parser regression tests cover sample bounds/lower-index quantiles,
missing/nonfinite/negative/malformed metrics and invariant parsing under es-UY;
they do not replace actual captures. All engine processes/handles terminal.
FPS/resolution/visible-count target and isolated CI runner asked asynchronously;
no owner agreement yet. No optimization/Mass rewrite/security change/new assets.
Next C018C: complete workload/trusted-player and remaining M7 functional/render/
security/release gates. Do not re-run or overwrite accepted C018B captures.
