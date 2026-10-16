# Supported asset-authoring contract — schema 1

Production uses persistent, project-authored Unreal assets under Git LFS. Opening the
project, PIE, builds, tests and cook must not run asset generation. No master setup
script is required. Validate references before deciding that an asset needs creation.

## Retained tools

| Tool | Allowed scope | Default / explicit write |
|---|---|---|
| `author_visual_materials.py` | `/Game/Materials/RomanVisual`, 3 bases / 9 instances | Read-only; `--apply` creates missing schema1 assets only |
| `author_audio_assets.py` | `/Game/Audio/RomanFeedback`, class / 6 waves | Read-only; `--apply` creates missing schema1 assets only |
| `author_character_assets.py` | Original rig, clips, three profiles and materials in `/Game/Characters/RomanSoldier` | Read-only; `--apply` creates missing recipe-owned assets only; native graph/montage/bundle verified separately |
| `roman_audio_recipe.py`, `roman_character_recipe.py` | Deterministic original source recipes | Pure generation functions / tested bytes; not editor startup hooks |
| `probe_character_import.py` | Diagnostic API inspection | Read-only; `--import-probe` is an explicit empty-namespace, unsaved experiment, not a production importer |

Schema/ownership, type and actual material parameters, audio format/references or
character fingerprints/rig/clip properties are validated. Existing files must match
their original recipes. `--apply` never means overwrite, reimport, adopt a foreign
asset or repair an existing asset. A failed multi-asset creation may leave earlier
new assets; inspect readback rather than assuming an atomic transaction. `--apply`
and `--test-rejection` are mutually exclusive; rejection probes restore their own
temporary changes and never save. Do not change a schema marker to disguise drift.

Native Editor-only input metadata and character graph/montage/bundle authoring are
also explicitly gated, owned/versioned and covered by readback/rejection tests.
Do not rerun one-time character repair/Gaul migration flags: their accepted state
and exact baselines are recorded in the reviews and HANDOFF.md.

## Validation recipe

Run sequentially with no asset edits, compilation or PIE in flight. For example:

```powershell
$authorScript = '-script="' + (Join-Path $PWD 'Tools/author_audio_assets.py') + '"'
& 'C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe' `
  "$PWD\RomanEmpireGame.uproject" -run=pythonscript $authorScript `
  '-EnablePlugins=PythonScriptPlugin,EditorScriptingUtilities' -DisablePlugins=Fab `
  -unattended -nop4 -NullRHI -stdout -FullStdOutLogOutput
if ($LASTEXITCODE -ne 0) { throw 'Read-only asset validation failed' }
```

No `--apply` in ordinary validation or CI. Compare source hashes before/after; a
second read-only pass must produce the same validated set without file changes.
Actual output plus native asset/content tests is evidence; source-policy AST guards
alone do not prove Unreal API compatibility or idempotence.

## Native FunctionalTest map authoring

Dedicated FunctionalTest maps use native automation, not startup generation:

```powershell
& 'C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe' `
  "$PWD\RomanEmpireGame.uproject" -unattended -nop4 -NullRHI -DisablePlugins=Fab `
  '-ExecCmds=Automation RunTest RomanEmpireGame.Functional.MapsReadback;Quit' `
  "-ReportExportPath=$PWD\Artifacts\Automation\<unique-readback>" -stdout -FullStdOutLogOutput -log
```

Only for authorized creation of missing fixtures, add `-RomanAuthorFunctionalMaps`.
The native author preflights all owned schema1 targets and never overwrites or
repairs them. Ordinary Test/CI runs omit the flag. `/Game/Tests` is never-cook;
verify the actual cooked registry as well. See `M7_FUNCTIONAL_MAP_REVIEW.md`.

## Retired setup and migrations

`Content/Scripts/{MasterSetup,CreateBlueprints,CreateMaterials}.py` created unused
or incomplete shells with no reliable production validation. They are retired in
favor of existing native classes/UI and persistent materials. The one-time
`author_territory_tactical_maps.py` and `migrate_territory_adjacency.py` are retired:
the nine PDAs already contain validated map references and stable adjacency IDs.
Never recalculate canonical adjacency from actor transforms during setup.

Original tracked scripts remain recoverable in Git history. Exact pre-retirement
versions, including uncommitted changes and the two untracked migrations, are in
`Artifacts/C017I_LegacyScriptBackup/manifest.json` with verified SHA256 copies.
Do not execute these historical copies. No binary asset was removed or regenerated.
