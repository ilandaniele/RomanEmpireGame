# C018H local QA packages — LOCAL VERIFIED, not release/player approval

2026-10-09. The final UI binary was compiled but fresh Development/Shipping QA
archives were still missing. This is independent local work toward M7.5, without
configuring remote CI, signing, Windows security, commits or distribution.
The earlier conclusion that only external work remained was too broad: packaging
the final binaries can be verified locally even while those external gates remain.

## Reused cook / native workflow

Existing C018D cook succeeded (594+7platform/601total,0errors/warnings); its actual
engine-parsed registry contains59 required production packages and excludes test
maps/module references. All77 accepted content/source-art/fixture bytes were
rechecked before packaging and match immutable receipts. No content/config edit.
Fresh prerequisite validation passes; UE5.8.2/VS14.44/nativeSDK22621 remain selected.

Installed AutomationTool ProjectParams.cs confirms `-skipcook` implies Cook=true
but skips cooking; `-stagingdirectory` is supported. Direct installed UAT
BuildCookRun builds/stages/packages/paks/archives using that existing cook. Normal
BuildProject.ps1 remains unchanged, and clean CI still uses the fresh-cook recipe.
Fresh contained staging/archive destinations refuse existing paths; prior packages
are not replaced. No packaged runtime launch or security bypass is performed.

`Artifacts/C018H-QAInputs1.json` records registry/projectstore SHA256, configuration
hashes,77input count, fresh paths and explicit dirty-QA scope, NOT secret values.
Archive root `Artifacts/Packages/C018H_LocalQA1`; stage root
`Artifacts/LocalQAStages/C018H_LocalQA1`, configurations in separate children.

## Confidentiality boundary

The internal Zen projectstore contains a local credential. One diagnostic read
showed it; it is not repeated in documentation or persisted as a source asset.
Do not print/copy/upload the projectstore or values. UAT output is value-redacted
before the owned log is written. No credential rotation/deletion, service-policy
change or Windows security change is authorized here. Archives must be inspected
to reject any projectstore/private-key/credential/cache/Editor/test leakage.

## Terminal evidence

Both native BuildCookRun operations completed successfully, sequentially, without
recooking or launching a player:

| Configuration | Native/wrapper | Exit / duration | Windows archive |
| --- | --- | --- | --- |
| Development | PID15024 /81955 | 0 /104.56s | 48 files;972,805,697 bytes |
| Shipping | PID10172 /24806 | 0 /36.42s | 30 files;619,034,131 bytes |

Owned sanitized logs `Artifacts/C018H-QA{Development,Shipping}1.log` and matching
`.exit.json` receipts preserve actual native statuses. Development compiled its
normal Editor prerequisite using observed VS14.44.35228 /WindowsSDK10.0.22621.0.
Shipping used `-skipbuildeditor`; its Game target was already up to date (zero
compile actions), not a new full compile. Both archive roots contain existing
`write_build_metadata.ps1` sidecars marked `dirty=True`, engine5.8.2 and HEAD
0726de25b05f84736b421480a1f1d633f87ac29c. They are not embedded binary identities.
Distribution file counts above cover `Windows/` only, excluding sidecars.

## Native container and content verification

Installed UnrealPak commands ran sequentially after UAT terminated:

- `-List` and `-Test` pass both `.pak` files. Source inspection showed `-Test`
  only checks opening/index validity, so additional **`-Verify`** checked actual
  payload hashes: Development1737 /Shipping1730 files healthy, exit0.
- Native `.pak` extraction yields exactly those file counts. Four IOStore TOC
  listings pass: each project container1210 chunks and each global container1.
  Native `IoStore -FindAndExtract=... -PackageFilter=* -OutPath=...` reads and
  extracts1237 named payload files per configuration, exit0. This does not
  independently rehash all unnamed shader/container/global chunks.
- Both project IO indexes match all79 distinct `/Game` asset paths in the prior
  engine-parsed cooked registry (including its previously verified59 required
  production packages); actual default `/Game/Maps/L_VerticalSlice` is present,
  and no `/Game/Tests` asset is indexed. Registry/projectstore and three
  configuration SHA256 values remain byte-identical to the preflight receipt.
- Both packaged main Game EXEs, not bootstrap launchers, match native binary
  SHA256. Native target receipts are Game/IsTestTarget=false. Archive filenames,
  receipt products/dependencies and active plugin manifests exclude the project
  test module, UnrealEditor, RemoteControl, MCP/toolset binaries and private
  projectstore/key/cache files. Active manifests contain184/179 plugin entries.
- Extracted project descriptors retain the Editor-only test-module declaration,
  disabled RemoteControl/WebInterface/AutomationDriver flags and Editor-only MCP
  allowlists. Stock engine Slate automation icons and InterchangeTests plugin
  descriptors remain. These are not an active project-test/Editor server; do not
  describe this package as containing no test-related text or icons whatsoever.
  The actual no-server startup gate still requires a trusted player.
- A bounded streaming exact-value UTF8/UTF16 probe checks6036 archive/extracted
  named payload/log files with no local Zen credential match. Four in-memory
  positive/negative encoding/boundary checks pass; no credential value is written
  into the receipt. This is not a general secret scan, credential-validity check,
  unnamed-chunk inspection or history/security certification.

Generated evidence: `Artifacts/C018H-{NativeContainerChecks,PakPayloadChecks,
IoPayloadChecks,ArchiveChecks}1.json`; per-configuration sanitized PakList/Test/
Verify/Extract and IOList/Extract logs/CSVs. Inspection copies remain isolated in
`Artifacts/C018H-{Pak,Io}Inspection1`, outside Content, Git and distribution.
Archive sidecar SHA256 manifests enumerate every Windows file, including any
native QA debug symbols; they do not approve distributing symbols or licenses.

| Configuration | Archive manifest SHA256 | Main Game EXE SHA256 |
| --- | --- | --- |
| Development | 28f9b94128a5fa18c305e20dee808711f29c543f6e3c610f41a43cd1fbf99859 | 63330b9e509f080f82041f8b1738d4a94039f4fd6572eeffbc3dab64c78bc39c |
| Shipping | 4773d3fa6483f1c58f3df1625da7f94134110955b737422b18bb402cd8b315d0 | 3e3da2b7cab7a869479b4c62134f6ada51c0e5e1086e57749195a56c15db9c2a |

Two pre-extraction guards initially stopped on a wrong shared1730-file assumption
(Development actually1737) and an unrecognized native `<ContainerHeader>` index
label. Both stopped before native extraction/output creation, then were corrected
using the actual footer/index schema. No content was modified or guard bypassed.

The `unreal-packaging` skill informed native cook/package separation, reuse of the
verified cook and the explicit requirement to run an actual packaged player.
No source/config/launcher redesign, signing, credential rotation, remote write,
automatic commit or full-suite rerun was needed for this packaging-only block.

## Next boundary

Update2026-10-12: el usuario solicita QA en esta PC o una VM local. El
[protocolo del player](M7_TRUSTED_PLAYER_QA.md) reutiliza estos mismos archivos,
con hashes de ambos manifests confirmados de nuevo; no nuevo build/cook/copia/
arranque. Host Home sin Sandbox soportado, VM lista no identificada; entorno y
prerequisites gráficos/Windows con licencia requieren resolución. El bloqueo de
octubre9 descrito abajo es histórico; goal ACTIVE y trabajo de preparación nuevo,
no QA aprobado. Todos los gates de clean-candidate/security/owner siguen OPEN.

Closing checks: repository validation exit0/zero open M1 warnings
(`Artifacts/C018H-RepositoryFinal1.log`), existing candidate text/dependency gate
exit0/337 texts/no findings or policy failures
(`Artifacts/C018H-SecurityGateFinal1.json`), Git LFS fsck and diff whitespace exit0.
Git retains HEAD0726de2 and an empty index; existing line-ending notices are saved
in `Artifacts/C018H-DiffWarningsFinal1.log`, not treated as runtime warnings.
No UnrealEditor/UnrealPak/UAT/UBT process remained at closing observation.

Both archives must be labeled DIRTY LOCAL QA, not clean release candidate, trusted
player or remote CI success. No owned operation remains running. M7 still requires all external gates in
M7_EXTERNAL_GATES_REVIEW.md and owner target/license/version/distribution approval.

Post-package read-only audits at2026-10-09 20:24:37/20:25:49/20:26:58UTC confirmed
three consecutive no-progress turns at the same missing external environment and
authority boundary: runners/workflows0, main unprotected, both packaged Game EXEs
NotSigned, no live UE jobs or owner response. Goal tool returned BLOCKED, not
complete. This does not invalidate the local QA evidence above or authorize
commits, external configuration, unsigned-player retries or trust-policy changes.
