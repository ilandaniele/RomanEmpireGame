# M7 automation/release foundation — C018A

2026-10-07, GPT. C018A local foundation VERIFIED; M7 and the complete goal remain OPEN.

Moved the existing launcher report check into Tools/AutomationReport.ps1 and
strengthened it: typed nonnegative bounded counts, nonempty unique project test
paths, every test Success/errors0, warning summary matches actual rows, no
failed/unfinished tests, optional exact required coverage. BuildProject Test uses
this same function and requires discovery/remapping/adjacency anchors. Standalone
validate_automation_report.ps1 exposes the identical failing process boundary.

25 pure contract checks pass, including an actual child CLI exit1 for a failed
assertion. First fixture correctly hit Windows PowerShell NativeCommandError on
expected child stderr before reading its exit; capture preference scoped/restored
only inside that negative fixture. No production exception/warning suppression.
Existing actual Full121/WASAPI72 and AuthoredReadback7clean reports pass the
strengthened validator. Five PowerShell AST parses and BuildProject Validate pass.
Final actual Test launcher exit0: Artifacts/C018A-TestLauncherFinal.log,
report Artifacts/Automation/20261007_203241_359/index.json,121success (49clean/
72only the two known WASAPI device conversion notices),0failed/pending/notRun.
This is a full regression after the final modal release/scroll cleanup as well as
the actual shared-validator/required-coverage launcher route. No gameplay/asset
changes after that run. The negative child fixture also restores LASTEXITCODE:
verified successful CI shell stays0 despite its deliberately failed child exit1.

Local .github/workflows/unreal-ci.yml now defines explicit hosted authorization,
manual dispatch/protected-main only (NO PR hooks), opt-in controlled Windows
runner/protected environment, read-only
contents token, no persisted checkout credentials, fixed SHA actions, serialized
build/test/IWYU/QA+Shipping package and private evidence artifacts. YAML parsed and
structural policy checked with locally available PyYAML6.0.3; no dependency
installed/required by repository tests. This is not GitHub validation or a remote
run. Reviewed primary sources and owner setup are in CI_RUNNER_SETUP.md. Initial
draft PR trigger removed before activation: YAML fork guards cannot secure a
runner against replacement of the workflow. Runner-group/workflow restrictions
and maintainer review are required externally; main-only CI is not a pre-merge
status. Final manual/main-only YAML policy parse passes.

write_build_metadata.ps1 actual local readback passes, records dirty=True and
engine5.8.2 with no credential data; resolved output restricted to an Artifacts
child, reparse-point redirection refused. Sidecar identity is not embedded binary
or smoke proof. Added RELEASE_CHECKLIST.md and unreleased CHANGELOG.md; release
version/signing/distribution remain owner choices. M1-retired dev_orchestrator
stub and compatibility wrapper were already safe; not reimplemented.

No runner/remote settings/branch rules/commit/push/Windows security mutation.
No gameplay/uasset/config/dependency change; M6 verified build/cook evidence still
stands. Remaining C018B/M7: actual controlled clean-checkout CI, functional/render and
performance coverage, secret/plugin/attribution review, real trusted Development
and Shipping player/distribution gates. M8 optional and gated after M7.
