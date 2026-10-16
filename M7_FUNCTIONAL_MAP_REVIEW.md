# M7 dedicated functional maps — C018C

2026-10-08. M7 and the whole implementation goal remain OPEN.

## Implemented coverage

Five persistent `/Game/Tests/Maps/FT_*` maps contain one schema1 native
`ARomanFunctionalScenario : AFunctionalTest` each. They are non-destructive
copies of the existing `L_Testbed`, not a new gameplay bootstrap or authority.

| Map | Reused canonical native scenario | Scope |
|---|---|---|
| FT_Navigation | FExerciseNavigation | Real navigation around transient blocking geometry and path lifecycle |
| FT_Combat | FExerciseUnitOrders | Existing order/combat/stamina/damage behavior |
| FT_Production | FExerciseSquadRegistry | Existing membership/lifecycle and canonical barracks construction/training/recruit registration |
| FT_Possession | FExerciseOwnership | Selection, attack commands, ownership loss, possession/return and real-time policy |
| FT_Victory | FExerciseObjectiveOccupation | Physical occupation, live defenders blocking, terminal objective result/idempotence |

The original native tests and their implementations remain intact. The adapter
calls their existing latent-command `Update`, rather than starting a recursive
automation run or accepting a log string as success. A local assertion sink is
immediately unregistered from test discovery; any assertion error finishes the
real FunctionalTest as `Failed`, even if the command still returns `false`.
The bridge checks `AFunctionalTest::Result` directly and owns a fresh PIE session.
It derives from `FFunctionalTestBase`, required by UE's RunTest/FinishTest API.

Existing scenarios include explicit GC and Slate dispatch: a weak, scoped engine
core ticker executes them outside actor/world tick traversal. Shared execution
lifetime protects callbacks; finish/timeout/abort/cleanup/end-play remove the
ticker. No tick, helper, test module or server is added to the runtime game.

## Asset ownership / shipping boundary

`RomanEmpireGame.Functional.MapsReadback` is read-only by default. Explicit
`-RomanAuthorFunctionalMaps` creates missing maps only after preflighting every
existing target; it refuses unsupported schemas, foreign identities and extra
FunctionalTest actors. It never repairs/overwrites an existing map. Hash checks
protect the original source and all existing target bytes. Negative in-memory
schema/identity probes restore their fields and never save.

The Editor module alone depends on FunctionalTesting, AssetTools and
DeveloperToolSettings. `DirectoriesToNeverCook=/Game/Tests` is explicit; content
validation asserts this configuration. Config validation is not by itself
evidence of absence from cooked content; inspect the actual cooked registry.

Author evidence: `Artifacts/C018C-MapsAuthor.log`, one clean success/exit0.
First native directed execution: `Artifacts/C018C-FunctionalDirected.log` and
`Artifacts/Automation/C018C-FunctionalDirected/index.json`, seven clean successes,
exit0 (five maps, readback, deliberately broken-assertion propagation).
Accepted five-map SHA256 baseline: `Artifacts/C018C-FunctionalMapsHashes.json`.
The known engine startup UnifiedError self-test diagnostics also occur before
test execution in the prior C018A log (14 identical Condition-failed messages);
they are not a new scenario failure or grounds to suppress project errors.

## Remaining verification / scope

Initial Editor builds exposed an unavailable HashFile API, incomplete-type
UniquePtr/UHT construction, a required virtual method and the missing packaging
settings module dependency. Corrected; EditorBuild3/4 succeeded. Logs preserved.
Final verification after ticker lifecycle cleanup and negative ownership probes:

- Strict non-unity/no-PCH build111 actions succeeded; final incremental5 succeeded
  (`Artifacts/C018C-IWYU.log`, `C018C-IWYUFinal.log`).
- Actual full BuildProject Test exit0: **128 clean successes**, zero failed,
  notRun/inProcess. `Artifacts/C018C-FullTestFinal.log`, report
  `Artifacts/Automation/20261008_141206_293/index.json`. The launcher requires
  ten exact coverage anchors, including all seven functional tests. Both native
  integration families and their functional adapters ran in the same process.
- Shipping Game build exit0, receipt TargetType=Game/Shipping/IsTestTarget=false,
  no Editor/test BuildProducts (`Artifacts/C018C-Shipping.log`).
- Sequential cook exit0,594cooked/0incremental/7platform/601total, zero errors
  or warnings (`Artifacts/C018C-Cook.log`). Engine DumpAssetRegistry exit0 parses
  the actual Zen registry (`Artifacts/C018C_CookedRegistry/Page_00000.txt`): no
  `/Game/Tests`, fixture names or RomanEmpireGameTests references. Required
  25input+15character packages (16 object paths, including generated class)+7audio
  +12materials remain present. Config exclusion is therefore backed by actual
  cooked-content evidence, not absence from an empty/mocked registry.
- A fresh explicit author-flag rerun creates nothing: one clean success/exit0,
  `Artifacts/C018C-MapsIdempotentFinal.log` and same-name Automation report.
  All71 accepted prior source hashes and five new map hashes remain unchanged
  after full tests, cook and idempotence. No baseline regenerated.
- Repository validation zero M1 warnings; Build Validate/AST/report25/parser16/
  LFS/diff checks pass. All native/build processes terminal; no held operation.

C018D subsequently adds the sixth campaign handoff map and existing native
cross-world coordinator, verified by full131clean, runtime log-capture/gate-failure
negative guards, strict/cook/actual registry exclusion/idempotence/77hashes.
See [campaign functional review](M7_CAMPAIGN_FUNCTIONAL_REVIEW.md). The actor is
not retained across travel and its success is not journey completion.
Full encounter workload/performance, reviewed rendering
baselines, remote trusted-runner governance and trusted packaged-player release
gates remain open. These maps are functional integration fixtures, not a
finished-encounter benchmark or proof of distributable-player behavior.
