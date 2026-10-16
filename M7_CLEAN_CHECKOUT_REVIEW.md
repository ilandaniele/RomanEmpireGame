# C018H — Committed candidate and independent local checkout

Status: IN PROGRESS, 2026-10-16. Owner approved the requested local commit;
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
Repaired source must be committed and built in the SAME clean clone before any
success claim. Original failed candidate/log remain preserved.

## Limits and next evidence

This is local source reproducibility, not an isolated remote runner, clean CI
package/player proof, signing, security mitigation or legal approval. QUIC is
retained; existing dependency security HOLD/attribution/owner-budget gates remain.
VM is deliberately deferred. HANDOFF.md owns current process/next action.
