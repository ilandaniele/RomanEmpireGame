# M7 candidate secrets/dependency gate — local verified, release OPEN

2026-10-08, C018G. No matching secret or reviewed plugin/action-policy violation
was found in335 current candidate text files. This is a bounded pattern-based
worktree check, not a comprehensive security certificate, license grant, clean
revision, remote CI execution or trusted-player evidence.

## Implemented scope

### Later engine dependency evidence —2026-10-12, separate from text gate

NetworkSourceReview1 ([review](M7_PACKAGED_DEPENDENCY_REVIEW.md)) binds exact
OpenSSL/libcurl library hashes/declarations/banners, not retained-object or
vendor-patch proof. Publisher standard support excludes OpenSSL1.1.1; curl8.15.0
JSON lists43 advisories (29Low/14Medium). These source-version candidates require
feature/backend/backport/retention/runtime evaluation, NOT43 proven game CVEs.
Only two selected advisory condition sets reviewed; no packet/exploit probes.
[Support policy](https://openssl-library.org/policies/releasestrat/),
[curl version metadata](https://curl.se/docs/vuln-8.15.0.json).

Release/security HOLD remains independent of MsQuic's six/Critical findings.
Current350candidate-text scan/0findings/0policyFailures does not clear either
dependency finding. Owner-selected compatible Unreal update evaluation must
check OpenSSL/libcurl too; no engine/plugin/library/security mutation authorized
or performed. Owner supplied UE5.8.2/CL56702186 at the existing UE_5.8 path;
MsQuicOwnerBaselineConfirmation1 verifies the DLL hash is unchanged from the
reviewed2.2.0.359654 baseline, NOT a patched upgrade candidate. A distinct
vendor-patched candidate/patch evidence remains needed. Existing native/test
gates do not establish patched libraries or trusted player behavior.

`Tools/security_gate.py` uses Python's standard library and local Git only.
It reads tracked plus nonignored untracked text, excludes actual tracked deletions,
limits input to4MiB per file and strict UTF8, resolves within the repository and
rejects symlinks/Windows reparse points. It makes no filesystem/network mutations.
Ignored files, Git history, Unreal/binary content, credential validity and engine
transitive supply-chain review are explicitly outside this check.

Private-key/provider-token patterns, credential-bearing URLs/literals and candidate
environment files produce only relative path/line/rule receipts. Never emit
matched values, source lines or exception text. A finding, unreadable input or
policy violation fails the CLI. No blanket directory/test exemption or automatic
credential deletion/rotation is introduced.

The exact existing11-plugin set, enabled states and Editor-only MCP restrictions,
runtime/Editor module split, and both already-reviewed full-SHA GitHub actions are
enforced. Changed dependencies require deliberate review/policy/notices updates;
this does not prove licenses or upstream binary integrity. Current provenance
remains in `ASSET_INVENTORY.md` and `THIRD_PARTY_NOTICES.md`.

The existing trusted-only workflow calls the gate before native jobs and keeps
`Artifacts/CI/SecurityGate.json`; failure blocks that job. Runner readiness,
permissions, environment and branch governance were not activated or changed.

## Actual evidence

- `Artifacts/C018G-SecurityGate3.json`: exit0,334 text files, no findings/policy failures.
- Final documented candidate `Artifacts/C018G-SecurityGate4.json`: exit0,335 text
  files, no findings/policy failures; repository validation0 M1 warnings.
- Closing docs checked again: `Artifacts/C018G-SecurityGateFinal2.json` exit0/335
  text files/no findings/policy failures; RepositoryFinal0 M1 warnings/LFS/diff PASS.
- `Artifacts/C018G-SecurityTests3.log`:14 tests,13 pass and1 skip. Windows lacks
  real symlink-creation privilege; no security-setting change. Synthetic symlink
  and reparse-point rejection both pass independently.
- End-to-end synthetic credential witness: failed audit, CLI return1, receipt
  excludes credential value. Missing/malformed inputs, changed/unpinned/duplicate
  dependencies and path traversal all reject. Fixtures contain no real credentials.
- `Artifacts/C018G-PythonFull1.log`:37 total,36 pass/1 same symlink skip.
- Existing local PyYAML parses changed workflow and confirms gate inclusion;
  no installation/new CI dependency. First inline shell quoting check failed,
  corrected stdin-based read-only check passed.
- Initial gate correctly rejected missing tracked legacy files; candidate
  enumeration was corrected to subtract Git's actual deleted set. Preserved
  `SecurityGate1.json`; no skipped arbitrary failures or restored obsolete files.

## Packaged MsQuic advisory finding —2026-10-12, release HOLD

The later [packaged dependency review](M7_PACKAGED_DEPENDENCY_REVIEW.md) binds the
installed/Dev/Shipping DLL to OpenSSL2.2.0+359654 outside narrow PE fields. Initial
four High advisories are a subset: MsQuicUpdateSources1 adds two current advisories,
including Critical RCE and OpenSSL certificate validation. Six publisher records
affect this candidate; NuGet warning/deprecation supported, actual runtime
exploitability/comprehensive binary audit not established. Advisory/usage receipts
are under C018H-DependencyInventory1. Project identifiers/selected descriptors
show no explicit QUIC references, not absence of implicit use/network exposure.

This is OUTSIDE Tools/security_gate.py's candidate-text/explicit plugin-action
policy scope. Its clean exit0 remains valid only for that scope, NOT an engine-
transitive security pass. Owner chose retain QUIC/evaluate compatible update.
MsQuicUpdateEvaluation1 records2.5.11/2.6.2 supported header-prefix candidates,
not ABI/runtime clearance. Available UE5.8.3 MsQuic patch status unverified;
no install/migration authority, policy exceptions/engine/trust/config mutation.
Keep M7 release security OPEN until reviewed mitigation is verified. Independent
attribution may continue; don't delete a staged DLL to manufacture gate success.

## Remaining release risks / owner gates

No critical secret finding was observed in the stated scope. Coverage is limited:
owner-approved tooling/history review remains necessary if secrets ever entered
history or ignored files; do not upload them or claim this scan clears them.
Actual isolated remote runner/pre-merge status/protection and deliberately failing
remote promotion remain unverified. Engine/template/plugin redistribution rights,
signed/trusted Development+Shipping launches, release version/channel and owner
distribution approval remain OPEN. No Windows trust bypass or external writes.

Skill `security-best-practices` informed redaction, input/path bounds, explicit
scope and fail-closed behavior. Its supplied references do not cover Python CLI
or Unreal C++; no web-server guidance or unsupported C++ audit was substituted.
