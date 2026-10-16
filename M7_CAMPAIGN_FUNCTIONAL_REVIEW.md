# Functional campaign handoff — C018D

2026-10-08. M7 and the implementation goal remain OPEN.

`/Game/Tests/Maps/FT_CampaignHandoff` is the sixth owned schema1 Editor-only
fixture. It uses the same guarded create-missing author and `/Game/Tests`
never-cook boundary as C018C; no previous map is overwritten.

## Real cross-world contract

The source `AFunctionalTest` first queries actual GameState/session/battle/unit
registry readiness, active campaign, stable unique army IDs and bootstrap actors.
This phase does not change campaign state or install a transfer.

Only its native `Succeeded` outcome permits the automation bridge to create the
existing `FExerciseCampaignTravel` coordinator. That original scenario is
unchanged: refund-overflow rejection preserves paid receipts; real strategic
entry refunds them; actual canonical AI/turn advances before battle; the selected
territory map receives stable army/encounter IDs, wallets and turn; physical
occupation/death resolves the result; real return preserves ownership, casualty,
consumed encounter ledger and refunds; turns resume; three new worlds publish
complete first readiness. The bridge additionally verifies the source world was
replaced and the old test actor's weak handle is invalid.

The actor is not retained across world travel and its success is not treated as
journey completion. No simulated capsule, duplicate authority, rewritten journey,
new runtime module or runtime callback is introduced.

`FBridgeTestBase` derives from UE's required `FFunctionalTestBase` but keeps log
capture enabled for the whole owned test, including the coordinator after the
actor phase ended. This prevents engine defaults from ignoring runtime errors
while no FunctionalTest actor is active.

## Negative guarantees

- `CampaignGateFailure`: a deliberate real assertion gives the source actor a
  `Failed` outcome, and the actual session authority has neither transfer nor
  campaign session. The coordinator is not started.
- `LogCaptureAfterPhase`: after a successful source actor, a registered expected
  runtime `Error` must be captured once by automation. Missing capture fails the
  framework's expected-error gate; no manual success flag substitutes for it.
- Existing `FailurePropagation` remains unchanged and proves a failed assertion
  ends the actor even when the latent command returns `false`.

## Evidence

EditorBuild1/2 exit0,6/3actions. Author `Artifacts/C018D-MapAuthor.log` and same-name
Automation report: one clean success/exit0, previous five-map hashes intact.
New map SHA256 baseline `Artifacts/C018D-CampaignMapHash.json` is immutable.

Native directed `Artifacts/C018D-FunctionalDirected.log` and
`Artifacts/Automation/C018D-FunctionalDirected/index.json`: **ten clean successes**,
exit0, zero failed/pending/notRun. This includes all six maps, three negative
guards and readback; exact required coverage is checked by the shared validator.
The native launcher now requires13anchors (prior10 plus handoff/log/gate failure).

Final full launcher exit0: **131 clean successes**, zero failed/notRun/inProcess,
all13required anchors (`Artifacts/C018D-FullTestFinal.log`, report
`Artifacts/Automation/20261008_143719_654/index.json`). Strict111actions exit0,
`Artifacts/C018D-IWYUFinal.log`.

Sequential final cook `Artifacts/C018D-CookFinal.log` exit0,594cooked/0incremental/
7platform/601total, zero errors/warnings. Engine DumpAssetRegistry exit0
(`Artifacts/C018D-CookedRegistry.log`, `Artifacts/C018D_CookedRegistry/Page_00000.txt`)
confirms all test maps and test-module object references absent, while required
25input+15character packages+7audio+12materials remain present. Fresh explicit
author-flag rerun creates/writes nothing: `Artifacts/C018D-MapIdempotentFinal.log`
and same-name Automation report, one clean success/exit0.

All77accepted hashes remain unchanged:71prior assets+5C018Cmaps+1C018Dmap.
Repository zero M1 warnings/AST/LFS/diff pass. All processes terminal.

Prior Shipping C018C evidence remains applicable to unchanged runtime source and
Game configuration; only the Editor test module and never-cook fixture changed.
No trusted standalone launch, remote CI, rendering baseline, finished-encounter
performance budget or owner release approval is claimed.
