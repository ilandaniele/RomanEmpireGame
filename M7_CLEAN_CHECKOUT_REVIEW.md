# C018H — Committed candidate and independent local checkout

Status: LOCAL PREFLIGHT VERIFIED, 2026-10-16; M7/remote/player/release gates OPEN.
Owner approved the requested local commit;
no push, remote activation, Windows security change or VM action.

## Candidate and first checkout

- `f79f0b8bef35dc408fdb113d3c35558b07986c9a` commits the previously verified
  offline implementation, original assets, reviewed visual baselines, tools,
  tests and documentation: 503 paths selected explicitly, not `git add -A`.
  Original worktree became clean; no unrelated files were discarded.
- Security text gate353 inputs/0 findings/0 policy failures, Python114 tests
  (113 pass/1 existing capability skip), report-contract25 checks/negative child1,
  repository0 M1 warnings, staged whitespace and LFS fsck passed before commit.
- Local clone created with `--local --no-hardlinks --no-checkout`, then detached
  checkout of the exact candidate under
  `Artifacts/C018H_CleanCandidate1/Checkout`. No shared Git alternates, source-copy
  overlay or reuse of the original worktree's Binaries/Intermediate.
  Local origin is the source repository, not GitHub. LFS176 files materialized,
  pointer-only0, fsck and clean status pass. Metadata records exact commit,
  dirty=false, UE5.8.2/CL56702186 in the clone's Artifacts/CI.

## Genuine clean-build failure and scoped repair

First Editor build failed exit6/49.30s; immutable log:
`Artifacts/C018H_CleanCandidate1/CleanEditor.log`.
The previous dirty-worktree adaptive builds had compiled these files separately.
A clean checkout enables ordinary Unity Build grouping and exposed:

- `GuidLess(FCampaignArmy,FCampaignArmy)` and another TU-local overload
  `GuidLess(FGuid,FGuid)` enter one anonymous namespace. Passing the overload set
  directly to `TArray::Sort` cannot deduce its predicate type (C2660).
- Army/encounter test files define identical `MakeArmy` helpers (C2084).
  Encounter/transfer tests also define unrelated `FFixture` types in the same
  merged anonymous namespace (C2011), causing cascading errors.

Scoped repair: rename the army comparator to `ArmyGuidLess`; wrap the complete
encounter and transfer test implementations in distinct file-specific namespaces.
No comparison rule, runtime domain, assertion, registered test path, build setting,
dependency or asset changes; do NOT disable Unity/adaptive checks to hide this.
Repair committed as `91c54c2ba6b697950ed96a8d7d6d74e9a192bf75`, fetched from
the local origin into the SAME clone and checked out detached/clean. Original
failed candidate/log remain preserved; its metadata was copied create-only to
`Artifacts/C018H_CleanCandidate1/Metadata-f79.json` before writing fix metadata.

## Repaired candidate evidence

- Ordinary clean Unity Editor build7 actions/12.57s/exit0:
  `Artifacts/C018H_CleanCandidate1/CleanEditor-Fix1.log`. Unity was NOT disabled.
- Directed rules27/27 clean/exit0 in clone
  `Artifacts/Automation/C018H_CleanRules1`; report validation explicitly requires
  all16 registered cases from the three affected army/encounter/transfer files.
- Full native suite139 successes (138clean/1with warning), zero failed/pending,
  Unreal and Test-action exit0. All21 Test-action anchors required and present.
  Report in clone `Artifacts/Automation/20261016_161857_790`; outer log
  `Artifacts/C018H_CleanCandidate1/CleanNativeFull1.log`.
  SHA256 `c6d757820ab3b57b8946284b2fef80fe17eae3973731aa5949863a7c73206a36`.
- Exact warning inspected: FileComparisonContract intentionally reads the owned
  fixture's nonexistent `missing.png`; LogStreaming reports failed read. No other
  test warnings/errors and no filtering/threshold or assertion changes.
- Checkout remains clean after the full run. Fix metadata records91c54c2,
  dirty=false, UE5.8.2/CL56702186. No original Binaries/Intermediate copied in.
- Sequential local packaging attempt started only AFTER native process exit:
  ignored `RunCleanPackaging1.ps1`, receipt `CleanPackaging1.json`, owner PID30728
  started2026-10-16T19:24:26.3727460Z. Guard exact clean SHA/no existing attempt,
  busy owned Editor/UBT/UAT, free disk and unique stage logs. All FOUR stages
  finished exit0; wrapper/PID30728/session4789 TERMINAL0, not a pending wait.
  Never replay this completed attempt.

## Packaging and content evidence

- Generate exit0; strict standalone/non-unity/no-PCH IWYU112 actions/72.72s/exit0.
- Development AND Shipping package exit0/BUILD SUCCESSFUL. Each cook594 packages,
  platform skips7/total601, 0 errors/0 warnings. Per-configuration metadata records
  exact91c54c2, dirty=false, engine5.8.2/CL56702186; outputs remain ignored in clone
  `Artifacts/Packages/{Development,Shipping}/Windows`. No executable was launched.
- `Package-Development1.json`:48files/970900521bytes,
  SHA256 `0620eeb073b931d73b33b239deeb5d7c3f04432fd9bb1f51c9856e149d1f6c5b`.
  `Package-Shipping1.json`:30files/617264123bytes,
  SHA256 `15f582dee2a21b724db255bf96cf17dfc5fe6818a76930c4fcd88d730790d77c`.
  Create-only manifest helper walks exact package roots without following reparse
  points, hashes each file, checks stable size/time and binds build metadata SHA.
  These are file-identity receipts, not legal/security/launch certificates.
- Final Shipping cooked registry473040bytes, SHA256
  `9a343248b40a15af5e89529405e42f0456a50aec422dfd6c0925d8ee1411aac5`.
  Installed engine DumpAssetRegistry commandlet exit0, input hash unchanged,
  `CookedRegistry-Shipping/Page_00000.txt`/homonymous `.log` under the attempt.
  Actual601 package rows, all59 presentation/input packages and the configured
  GameDefaultMap `/Game/Maps/L_VerticalSlice` present; no `/Game/Tests` or
  `/Script/RomanEmpireGameTests` references in the requested package/dependency dump.
  Initial ad-hoc assertion wrongly also required EditorStartupMap `L_Testbed`;
  rejected before further commands. Config inspection corrected the requirement,
  NOT the game/cook: editor-only startup map is not the shipping launch map.
  This registry belongs to the final SHIPPING cook, not a retained Dev registry.
- Both actual native target receipts are Game, correct configuration,
  IsTestTarget=false, no test-module name. No runtime-port/absence-of-server claim
  without a real packaged launch. Clone remains clean and LFS fsck passes.
- Relative DLL paths/bytes compared to the previous dirty QA archives:
  Development21/21, Shipping13/13 match; no added/different DLL. Existing MsQuic
  HOLD and other vendor/notice gaps are NOT resolved by rebuilding. This does not
  establish equivalence of static code or transfer old EXE/PDB identity evidence.
- Main workspace Editor also rebuilt11 actions/22.08s/exit0 after the repair:
  `WorkspaceEditor-Final.log`. No game rules/assets/thresholds changed.
- All87 reviewed visual baseline PNGs and manifests are now committed through LFS
  in f79f0b8; no baseline replacement or automatic image reapproval occurred.

## External state and remaining gates

Read-only GitHub check2026-10-16T19:36:56.8546201Z: remote main remains
89d8d66e5051d303dea9c965317cdb961d8a52f6, protected=false, repository runners0,
workflows0. No rule/environment configuration was queried in this fresh check;
do not infer their complete state from these fields. No push/remote mutation.
The owner's explicit confirmation covers local commits without push or remote
configuration. A licensed isolated runner and reviewed activation/branch policy
are still needed; a clean local checkout does not substitute for that gate.

Current source candidate is91c54c2; subsequent evidence-only Markdown commits do
not retroactively change which source built these packages. Native/Unity/IWYU/
cook/package/local file-identity gates are now evidenced; actual QA-01–10,
remote clean CI/promotion, hardware/FPS budget, signing/distribution,
dependency patch compatibility and final attribution remain OPEN. Do not restart
the deferred VM or repeat passed tests without a relevant change.

## Limits and next evidence

This is local source reproducibility, not an isolated remote runner, clean CI
package/player proof, signing, security mitigation or legal approval. QUIC is
retained; existing dependency security HOLD/attribution/owner-budget gates remain.
VM is deliberately deferred. HANDOFF.md owns current process/next action.
