# Controlled Unreal CI — activation is pending

The local workflow is a build/automation/package gate, not a release certificate.
No runner, repository variable, environment, branch rule or GitHub setting was
created by this implementation. It does not commit/push or upload anything locally.

## Owner activation

Local preflight verified2026-10-16: [source91c54c2 clean checkout](M7_CLEAN_CHECKOUT_REVIEW.md),
139 native successes, Unity/IWYU and both Dev/Ship packages. This closes the local
commit/reproducibility preparation, NOT the remote runner gate. Readonly remote
check19:36:56UTC: main89d8d66e/protected=false/repository runners0/workflows0.
No push or remote setting changed; current approval is local-commit-only.
Continue the activation steps below only with a genuinely isolated licensed runner
and authority for the specific remote changes. Do not attach this normal desktop
or revive the deferred VM merely to produce a green status.

1. Provide an isolated, preferably disposable Windows x64 runner with a licensed
   UE5.8.2 installation, VS2022 C++ Build Tools/Windows SDK, Git LFS, Python3 and
   Windows PowerShell5.1. Use a dedicated account and runner group limited to this
   repository and explicitly allow only the reviewed workflow at a protected ref.
   Confirm the selected runner-group/workflow restrictions exist on the chosen
   GitHub account/plan. Do not attach the developer's normal desktop or credentials.
2. Assign labels `self-hosted`, `Windows`, `X64`, `roman-ue-5.8.2`. Permit only
   reviewed repository code; never run fork PRs there. No Epic account/token or
   production backend credential is required in the workflow.
3. Configure environment `unreal-trusted` with required maintainer review,
   prevention of self-review and appropriate deployment-branch restrictions.
   GitHub environment features depend on repository visibility/plan; confirm the
   actual controls exist before activation. An environment name alone is not a
   security boundary.
4. Set `ROMAN_CI_ENGINE_ROOT` to the installation root on the runner and ONLY THEN
   set repository variable `ROMAN_CI_TRUSTED_RUNNER_READY=true`. Without readiness
   the hosted authorization job fails explicitly. There is no pull_request or
   pull_request_target trigger: only explicit maintainer dispatch and pushes to
   protected main. Do not set readiness merely to make a status green. YAML guards
   alone cannot enforce runner isolation if someone can replace the workflow.
5. Run a reviewed `workflow_dispatch` from a clean, committed checkout. Review
   automation reports, metadata, logs and both Development/Shipping packages.
   Require review without bot/direct-push bypass. Before adding a required
   pre-merge Unreal status, establish how reviewed changes run through the
   workflow-restricted runner and verify the actual status associates with the
   candidate commit; main-only CI is not a pre-merge check. Verify a deliberately
   failed invariant blocks promotion. Do not use guessed status names or enable
   automatic PR execution before that runner policy is proven.

Checkout uses LFS, a fixed revision, no persisted Git credentials, read-only
contents permission and reviewed full-SHA actions. Workflow concurrency is one
repository-wide group, no cancellation of an in-progress Unreal build; all engine
operations inside a job are sequential. Operators must still confirm old processes
have exited after runner interruption before reusing that machine.

## What currently runs

Source/LFS/recipe tests → strict JSON report negative fixtures → project generation
→ Editor build → full native/real PIE/FunctionalTest suite → strict IWYU → Development package →
Shipping package → private artifacts (seven-day retention, no hidden credential
folders). Failing logs/reports are retained. `Tools/write_build_metadata.ps1` records
schema/revision/dirty state/configuration/exact engine version/changelists/run ID
under resolved `Artifacts/CI`; this is a sidecar, not embedded binary identity.

C018G adds `Tools/security_gate.py` before native jobs, with a redacted
`Artifacts/CI/SecurityGate.json` receipt. Current Git candidate text/private-key
patterns and exact reviewed plugin/module/action policies fail closed. Local
37 Python tests pass with1 documented real-symlink capability skip; independent
synthetic link/reparse negatives pass. [Scope/evidence](M7_SECURITY_DEPENDENCY_REVIEW.md).
No history/binary/ignored-file, license, remote-runner or clean-revision claim.

## Gates still required

C018H [point-in-time external verification](M7_EXTERNAL_GATES_REVIEW.md),
2026-10-09 01:42:50UTC: main unprotected; zero repository runners/workflows/
environments; no trusted-ready variable. Local HEAD34 commits ahead of remote,
implementation uncommitted. Capability is not authorization; no activation/writes.

- Real remote clean-checkout run, runner/process isolation and branch protection.
- Dedicated reviewed rendering/screenshot baselines, functional-map coverage and
  performance captures/budgets; a headless test report does not prove these.
- Trusted Development and Shipping player launches on the intended QA machine,
  actual input/UI/rendered journey and absence of test/RemoteControl/MCP server.
- Owner-approved signing/trust/distribution and license/attribution review.

Windows currently blocks this developer machine's unsigned standalone. Do not
disable protections, repeat the identical failed launch, or call cook a smoke test.
No smoke success is fabricated by this workflow; M7 remains open.

## Primary references reviewed 2026-10-07

- [GitHub secure-use guidance](https://docs.github.com/en/actions/reference/security/secure-use)
  for full-SHA pinning, least privilege and self-hosted runner risks.
- [checkout v4.2.2 commit](https://github.com/actions/checkout/commit/11bd71901bbe5b1630ceea73d27597364c9af683)
  and [upload-artifact v4.6.2 commit](https://github.com/actions/upload-artifact/commit/ea165f8d65b6e75b540449e92b4886f43607fa02).
  These are reviewed compatibility pins, not a claim to be latest; re-review
  updates before changing them. No action source is vendored or shipped in-game.
