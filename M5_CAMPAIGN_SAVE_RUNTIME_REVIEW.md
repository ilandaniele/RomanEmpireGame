# C016B — Local campaign persistence and staged restoration

Status: C016B verified 2026-10-06, uncommitted. Native controls C016C and whole milestone/goal remain open.

## Contract

- `UCampaignSaveSubsystem` owns persistence preferences/status only. CampaignManager, FactionManager and WorldMapManager remain the sole live authorities. Detached `URomanCampaignSaveGame` staging uses C016A's fixed bounded envelope, not a file-provided dynamic class header.
- Two fixed logical slots, Manual and Autosave, each rotate generation `_0`/`_1`. Write the inactive file, read back and verify exact bytes before publishing success. The previous verified generation is untouched if writing fails. No cloud, caller-supplied path, new module/dependency/config/asset or security changes.
- Windows `Saved/SaveGames` file-size preflight bounds allocation before platform `LoadDataFromSlot`; the decoder enforces exact length/version/CRC/current transfer2. Highest valid generation wins; a corrupt alternative produces explicitly observable backup recovery. Unsupported schema in either file is rejected, not hidden by an older backup. Equal-generation divergent records reject. CRC is accidental-corruption detection, NOT authentication or hostile-file proof.
- Readonly capture requires an actual ready strategic session, no tactical encounter/pending travel or concurrent domain/turn mutation. It does not freeze/refund/save transient actors. Only the private actual Autosave pipeline capability bypasses the active-turn guard. Sandbox turns never create user campaign saves; test slots have a unique automation/process namespace.
- Final Autosave stage captures the completed increment, both RNG seeds, six currencies, current territories/settlements, diplomacy, stable armies, consumed ledgers and accepted AI retry intent. Disabled autosave does no I/O. I/O failure updates persistence status but never rolls back/repeats a committed turn or blocks valid post-turn dispatch.
- Load validates domains/current catalog/unit factories/faction eligibility and actual destination/queued-target world metadata BEFORE abandoning the source. Then existing receipt preflight/refund/freeze policy settles the source, and the validated save/settings use nonce-checked actual OpenLevel/quiet owner restoration before readiness. Failed incoming restore retains the capsule/preferences for retry; source travel failure keeps original preferences. Load success means travel queued, not restored. Initial ready sandbox can load; actual tactical session cannot.
- Existing legacy `CampaignManager.SaveCampaign/LoadCampaign(SaveName)` wrappers now target the fixed Manual slot; names are deliberately not filesystem paths. Typed subsystem APIs are the supported diagnostic/Blueprint entry points.

## Evidence

### C018I — Generation-conflict regression (2026-10-16)

Implemented `RomanEmpireGame.Campaign.SaveDisk.StorageConflicts` in the existing
Editor-only integration fixture. Production persistence behavior was already
correct for these cases; no runtime rewrite, schema migration, asset, or dependency
change was needed. The fixture operates exclusively on process/GUID-isolated
`RomanEmpire_Automation_*` files, with the existing fixture cleanup.

- Two individually valid, checksum-correct records with the same generation but
  different preferences: both load and save must reject `CorruptData`.
- Identical duplicate records are accepted for saving: the next generation advances
  exactly once and preserves the previous good file byte-for-byte. This prevents an
  overbroad implementation that rejects every equal generation from passing.
- Unsupported outer versions 0 and current+1 in either physical file must reject
  both operations despite a valid neighboring copy; neither file may be overwritten.
- The last readable generation (`MAX_int64 - 1`) rejects another save rather than
  overflowing or destroying either copy. This case deliberately does not request a
  valid load/travel.
- After each rejected pair of operations: exact bytes of BOTH Manual files,
  explicit failure, no pending travel, no false recovery flag, and released busy
  guard are asserted. A successful capture through the OTHER isolated logical slot
  compares every detached campaign field against the initial capture, including
  wallets, territories/settlements, armies, diplomacy, RNG and encounter ledgers.
  This public-API capture writes test Autosave generations; it is not a read-only
  probe or a private-header dependency.

Verification:

- Final Editor incremental build: 4 actions, exit0, 8.74 seconds. Initial attempts
  caught an incomplete type and the repository prohibition on importing private
  runtime headers. Corrected by using the existing public persistence API, not by
  weakening the repository guard or exposing implementation types.
- Initial directed run: `Artifacts/Automation/C018I_StorageConflicts1`, 1 success,
  0 warnings/errors/pending, Unreal exit0; strict report validator passed. It
  precedes only removal of an unused fixture parameter.
- Final affected family: `Artifacts/Automation/C018I_SaveDiskFamily1`, all 10
  SaveDisk cases succeeded, 0 warnings/errors/pending, Unreal exit0. All 10 exact
  names independently required by `validate_automation_report.ps1`.
- Final report SHA256:
  `371d4cd82165589664912fcb6e59552e8f144854067682f50b34121ae81e5f97`.
  Final integration-source SHA256:
  `4fbc944fe85e0a60829a906892f0a2e8c4f40744ebb1a411ccdcc702ab35a721`.
- `BuildProject.ps1 -Action Test` now requires the new test by exact name (20
  required anchors); it cannot silently disappear from a passing report.
  PowerShell parse passed. Existing report-contract suite: 25 checks pass,
  deliberately broken-assertion child process exit1 as required.
- Repository validation/LFS checks passed during the final Editor build; diff
  whitespace check passed. Full unrelated suite, IWYU, Shipping/cook and visual
  recapture were not repeated for Editor-only fixture/assertion changes.

Scope: native Editor/PIE, NullRHI, actual platform save files and world travel for
the existing affected family. Not packaged-player QA, a renderer/performance test,
hostile-file authentication, remote CI execution, or whole-M7 completion. Windows
VM work is deferred at the user's request; no VM operation occurred in this block.
Current task/next action is owned by HANDOFF.md, not the historical C016 NEXT below.

Subsequent final full95/95 clean exit0 C016D_Full covers the complete current B/C/D tree, including the rooting/observer-GC/direct-include changes after the earlier full90. B evidence preserved; native controls and combined journeys now verified separately, whole milestone/goal still open.

- Editor runtime build61 succeeded: `Artifacts/C016B-Editor.log`. Test build5 succeeded: `C016B-Editor-Tests.log`; final extended test build succeeded `C016B-Editor-Extended-Final.log`.
- Initial four journeys succeeded with warnings caused only by the test helper attempting absent alternative files. Helper now checks existence before loading; no warnings suppressed. A test-only call to nonexistent ReplaceArmyUnits failed compilation, corrected to existing atomic ReplaceFromSnapshot fixture; no production contract change.
- Final directed6/6 clean, exit0: `Artifacts/Automation/C016B_Targeted_Extended`, `Artifacts/C016B-Targeted-Extended.log`.
- RoundTrip/Autosave prove actual disk save, advance, selected strategic world reload, first readiness saved wallets/army IDs/settings, then exact reflection equality of EVERY detached campaign field including RNG/ledgers/territories. Disabled preference survives load before readiness; manual/autosave stay independent.
- Backup deliberately corrupts newest test generation; previous value is restored and backup recovery explicitly reported. Guards exercise unsupported outer version despite good backup, missing current unit, missing world and corrupt both generations, all preserving source turn/army/wallet and not scheduling travel; good backup can repair inactive corruption.
- IOFailure creates only an exact isolated empty test-slot directory, causing platform write failure. All nine steps still complete exactly once; previous save bytes and success-turn marker remain intact; directory removed non-recursively. No user slots/security touched.
- QueuedAI saves the real accepted unbound attack at turn2, repairs a transient missing-map fixture, reloads it into actual strategic owners, compares all values, and dispatches SAME encounter as a retry with zero repeated stages/spending/production. Real incoming tactical session rejects save/load.
- Save-state events reject recursive save/load and eligibility; public save/load reject during every turn-stage notification. No UI Tick/actor scans, no trusted packaged-player/renderer/cook/complete-milestone evidence in this block.

- Full90/90 clean exit0: `Artifacts/Automation/C016B_Full`, `C016B-Full.log`. Afterwards rooted temporary USaveGame staging with TStrongObjectPtr and added observer-triggered actual CollectGarbage during load. Strict IWYU first caught missing direct UObject/Package.h in SaveBoundary; included it and final strict build succeeded (`C016B-IWYU-Final.log`). Final runtime evidence will be recorded below; no warnings/checks suppressed.

- Final directed6/6 clean exit0 including real observer GC: `Artifacts/Automation/C016B_Targeted_Final`, `C016B-Targeted-Final.log`. Strict IWYU first90 actions caught one missing direct include; final incremental5 succeeded. Shipping21 succeeded `C016B-Shipping.log`. Full90 precedes ONLY final staging rooting/observer GC fixture/direct include; final directed6/builds cover those changes, not a second full run. Repository0M1warnings/diff pass. All processes terminal.

Next: C016C native manual-save/load/autosave controls, fixed-slot overwrite/load confirmation and feedback, lifecycle/pointer/deferred-boundary refresh and complete journey verification. UI queries made DURING SaveStateChanged observe operation/turn guards: refresh once after the event stack/turn boundary, not a per-frame Tick, polling loop or new authority. Sandbox resume and failed incoming saved-load quarantine/settings retry merit directed UI/runtime proof; no previous claim of that new journey. Initial sandbox backend load eligibility is implemented, not yet a fresh-launch runtime test. Full first-playable packaged-player gate remains open (historical Code Integrity issue, no identical relaunch/security changes).
