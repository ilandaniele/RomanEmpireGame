# M6.9 — supported editor-generation ownership and retirement

2026-10-07, C017I / GPT. M6.9 CLOSED; M6.8 minimum offline is closed separately.

Retired five obsolete/unversioned routes in favor of existing persistent assets:
`Content/Scripts/{MasterSetup,CreateBlueprints,CreateMaterials}.py` and
`Tools/{author_territory_tactical_maps,migrate_territory_adjacency}.py`.
The first group generated duplicate/unused shells and blank material graphs; the
adjacency migration rewrote even unchanged definitions and derived connectivity
from map coordinates. The nine authored PDAs already carry stable canonical
adjacency and required map references. No asset, runtime code or native factory
was removed or rewritten. Existing dirty work is preserved in exact SHA256-verified
copies at `Artifacts/C017I_LegacyScriptBackup/manifest.json`; tracked originals are
also Git-recoverable. Do not run archived copies as setup.

Kept correct material/audio/character authors unchanged: schema1/ownership,
read-only default, explicit create-missing-only, no replacement/reimport/adoption,
actual parameters/format/recipe/rig validation. The diagnostic character import is
explicit, empty-namespace and unsaved; default API inspection has no write route.
Native graph/bundle/input metadata authoring remains Editor-only, owned and gated;
previous accepted states/baselines must not be recreated to mask changes.
[Supported operations and recipe](Tools/ASSET_AUTHORING.md).

## Verification

- First targeted Python: new generation contracts6, native UI retirement4,
  audio retirement3, material manager retirement2 — 15/15 pass.
- Guards execute the actual pure flag assignments/exclusion check, enforce
  readonly default/schema1/no replace_existing/no saved diagnostic route, and
  retain all previous no-empty-HUD/no-null-audio/no-manager guarantees against
  the current authors. They do not pretend to verify engine behavior.
- Real authors: two sequential read-only passes per script all exit0; logs
  `Artifacts/C017I-author_{visual_materials,audio_assets,character_assets}-ReadOnly{1,2}.log`.
- All existing assets validated without creation (materials12/audio7/character
  importer12; native graph/montage/bundle are separate, not missing outputs).
- Native `C017I-AuthoredReadback` exit0/7clean/0fail: actual primary asset discovery,
  malformed definition rejection, nine tactical map references, stable adjacency,
  imported rig/motion, PCM feedback and input metadata/read-only rejection probes.
- Full Python23pass (`Artifacts/C017I-PythonContracts.log`), after targeted15pass.
- All71 source hashes unchanged:25input +37prior art/source +ninePDAs.
  Exact pre-check nine PDA hashes: `Artifacts/C017I-TerritoryHashes.json`.
  Five retirement recovery copies still match manifest; originals absent.
  Repo0M1warnings/Git LFS fsck/diff-check pass. All processes terminal.
- M6.8 cook/Shipping/strict/121full plus final directed/raster checks remain valid;
  no C++/asset change here requires rebuilding/repeating those gates.

No external service, security change, license/import/definition migration or
personal preference/profile write. Next C018A/M7 automation and release; not
whole-goal, trusted-player or M7 completion.
