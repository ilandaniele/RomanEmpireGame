# C018H packaged dependency inventory — LOCAL VERIFIED; rights review OPEN

2026-10-11. Independent remaining M7.5 local work identified after dirty QA
packaging. `THIRD_PARTY_NOTICES.md` currently inventories repository-vendored
inputs, not the actual engine-transitive native libraries delivered in the QA
archives. Shipping contains13 DLLs plus two EXEs, while its111-byte `NOTICES.txt`
contains only the FreeType notice. This is an attribution-inventory gap, not a
legal finding that any particular license is violated.

## Evidence already inspected

- Immutable Shipping SHA256 manifest and native files: DbgHelp, MsQuic,
  NVIDIA Aftermath, Ogg/Vorbis, XAudio2, ONNX Runtime, D3D12 Core/SDK Layers,
  DirectML and oneTBB. File-version metadata is inspected without executing DLLs.
- Existing extracted active plugin manifests contain179 Shipping/184 Development
  descriptors. Descriptor presence is not proof every module executes/links.
- Existing Shipping linker response names third-party libraries, including zlib,
  ICU, BLAKE3, Oodle, libpas, oneTBB, VTune, OpenSSL and WebM. A response lists
  declared link inputs, not proof that all code survives linker elimination.
- Installed `VCToolChain.cs` sets link working directory to Engine/Source.
- Full installed `ListThirdPartySoftware.Automation.cs` and `JsonExportMode.cs`
  inspected. Native `-Mode=JsonExport -OutputFile=<Artifacts child>` exports target
  module metadata; code-generation actions default OFF and are not requested.
  Avoid stock ListThirdPartySoftware's Engine/Intermediate output destination.
- Installed TPS metadata is not uniformly valid XML: MsQuic has unclosed tags;
  one ONNX NumPy entry is wrapped/escaped. Preserve/report malformed sources;
  never repair the engine or silently treat malformed XML as license approval.
  Ogg/Vorbis uses an explicit redirect and an older-named license document;
  multiple newer license documents coexist. Do not guess version mappings.

## Completed local evidence (2026-10-12 UTC)

Both native JsonExport operations terminated0: Shipping ~1.89s, Development
~1.6s. Actual exit receipts and native logs: `Artifacts/C018H-DependencyInventory1`.
No `-ExecCodeGenActions`, build, cook, DLL loading or packaged player execution.

`Tools/packaged_dependency_inventory.py` uses only Python's standard library.
It verifies EVERY archive file against its existing SHA256 manifest and exact
file set, matches the main Game EXE to the native binary (not the bootstrap),
records target module/plugin declarations and hashes declared link inputs.
It examines TPS sources only in target module directories (including the actual
Shaders module), follows contained redirects with cycle/depth guards and hashes
exact referenced license documents. Generic license directories, missing paths,
non-XML/wrapped or malformed metadata and unsafe declarations remain explicit.
No guessing, XML repair, URLs fetched, license text rewritten or engine edits.

| Existing QA archive | Development | Shipping |
| --- | ---: | ---: |
| Verified files / bytes | 48 / 972,805,697 | 30 / 619,034,131 |
| DLLs / unmatched declaration candidates | 21 / 10 | 13 / 3 |
| Target modules / plugin descriptors | 497 / 184 | 455 / 179 |
| Unique declared link inputs | 94 | 87 (54 file paths + 33 platform import names) |
| Selected TPS source files | 240 | 235 |
| Exact license-document references found (top-level TPS) | 33 | 31 |
| Non-XML/malformed / ambiguous license directories | 89 / 39 | 89 / 38 |
| Unresolved license references / redirects | 55 / 2 | 53 / 2 |

Remaining TPS statuses:14 redirects and8 without a license reference per config.
Redirect targets have their own nested statuses/hashes. These counts do not mean
that every source is applicable to Win64 or that89 libraries violate a license:
installed TPS formats and multiple platform/version variants require review.
Three found license documents retain `license_text_encoding_unverified` (Recast,
NVAPI, OpenGL). Bytes are hashed unchanged; no assumed conversion/approval.

Final receipts (initial `Inventory-...1.json` retained, not overwritten):

- `Inventory-Shipping2.json`: SHA256
  `56540647aacca2130cea0c1ddd4993608b47e5c06a361f005afd0d0dd741d4ed`.
- `Inventory-Development2.json`: SHA256
  `72908c986a189ea51f8821f98e53270705286f857979fd0ec6d3db19a6505329`.
- `BinaryVersions1.json`: both sets of DLL version resources/SHA256, inspected
  without executing binaries; `a389d7ded47083aa83c16ea767949526d1dd65fb5a9a16ea8e56f4b083cc56d0`.
- `CandidateLicenseDocuments1.json`: ten manually named installed documents,
  hashes only; `008c4b8b593f77eaf37b157cff95e3b18bbb4606a52708d7f7769331619fa0b1`.
  No candidate is automatically selected from a generic directory or bundled.

Final inventory receipts include tool/input hashes, source origin, candidate
module associations and `rightsReview=pending_owner`. No raw definitions,
credential values, license snippets or package/source content copied to output.

## Native-export gaps and supplementary source resolution

Shipping's unmatched declarations are XAudio2 redist, D3D12Core and SDK Layers.
Development additionally has seven unmatched Vulkan layer DLLs and one mapped
WinPixEventRuntime DLL; a basename not present in
the export does not mean the DLL is malicious or unlicensed. Exact binary hashes
and names remain available for owner/upstream review.

Supplementary `SourceMappings2.json` (SHA256
`d708814589d698b510573f51c8967fd1b7baa89d8a32d80919e00f7a014944d6`)
matches14 relevant archive rows to the exact installed engine binary SHA256 and
records inspected native staging-rule hashes. `SourceMappings1.json` is historical.
All supplementary source/staging bindings are now resolved; the original module
export's10/3 unmatched basename counts are retained honestly, not overwritten.

- AgilitySDK.Build.cs selects1.618.5 by default and stages the matching Core/
  SDKLayers pair from its SDK Binaries/x64 directory into TargetOutputDir/D3D12/x64.
  Both source binaries hash-match both archives; the exact-version-named installed
  `AgilitySDK_v1.618.5.license` is now recorded separately, not approved/bundled.
- XAudio2_9.Build.cs explicitly stages xaudio2_9redist.dll on Windows x64 despite
  delay-loading a differently named XAudio2_9.dll. Native Game receipts contain it.
- WinPixEventRuntime.Build.cs stages its DLL, matching the Development archive.
- UAT CopyBuildToStagingDirectory invokes DeploymentContext's
  StageVulkanValidationLayerFiles: only non-program Debug/Development targets stage
  that directory. All seven Development layer DLLs hash-match installed originals;
  their absence from the Shipping archive is explained without changing policy.
- UEBuildModule.cs emits RuntimeDependencies only when Binary != null. Therefore
  module export is not a complete staging manifest; no external rule was modified.

This reduces engineering uncertainty, not license/rights uncertainty. Installed
SDK candidate text references additional distribution conditions/file-list evidence;
owner review is still required. No claim of blanket permission for SDKLayers or
all dependencies follows from a found version-matching document.

MsQuic binary2.2.0.359654 has malformed TPS and a2.1.8 EULA reference. DirectML
binary1.15.4 coexists with1.15.2 TPS and a1.15.4 candidate document. oneTBB2022.3.0
and NVIDIA Aftermath2025.5 metadata reference a generic license directory.
ONNX1.24.20260304.11.3a728b7 has wrapped/non-XML TPS entries plus1.24.1-named
license candidates. Ogg/Vorbis lacks binary version resources; declared Ogg link
path1.2.2 and its older-named1.2.0 license are not proof of exact DLL source version.
D3D12 binaries1.618.5 now have the exact-version-named Agility candidate above;
the older1.613.1 headers candidate is still not interchangeable evidence.
Do not silently substitute a newer/different document based on its filename.

### Agility1.618.5 primary package binding — reviewed2026-10-12

NEW `Tools/sdk_package_attribution.py` reads the pinned public NuGet package in
bounded memory only: no install, DLL load, external README command/URL execution,
engine edit, raw package/license persistence or archive modification. It verifies
the two installed binary hashes and license hash against existing SourceMappings2
before the network request. Output is create-missing-only under Artifacts;
errors redact exception content.19 directed tests pass; full Python88total/
87pass/1existing real-symlink capability skip, actual exit0 in
AgilityPythonFull1.log/.exit.json. Synthetic reparse tests from the helper suite
pass. No native gameplay/build/cook/player gate was rerun for this tooling change.

The [official1.618.5 package page](https://www.nuget.org/packages/Microsoft.Direct3D.D3D12/1.618.5)
distinguishes binary LICENSE.txt from header LICENSE-CODE.txt. The corresponding
fixed public package GET now yielded34,430,813bytes/74entries, SHA256
`0027fc24f947c48dbded13ada7d280be221eb651644e23a8a476f0f1f0a079dd`.
Package documents were inspected without saving raw contents. `distributable
files.txt` explicitly lists D3D12Core.dll, d3d12SDKLayers.dll and d3dconfig.exe;
the last is not one of the two previously mapped packaged DLLs and was not added.
File-list SHA256: `18be111795af241547e17596a010cb7606aa8670d9a26096425ba46a0f76ef9e`.
Binary LICENSE.txt SHA256:
`5239850894610071566f7ecee0b751fde43c862032d92b99d7d0f596b3433ebd`.
Its complete text is contained in the installed AgilitySDK_v1.618.5.license after
whitespace normalization. This is **not** raw document-byte equality: the local
file retains its wrapper/header/line formatting and original f58a4b16... SHA256.

Full upstream DLL bytes differ from the installed/packaged DLLs. A narrow
read-only comparison proves all other bytes identical after excluding exactly
the PE checksum4bytes, security-directory8bytes and each strict EOF certificate
table. Header/section bounds, machine type, count limits and overlap checks reject
arbitrary certificate exclusions. Code/header mutation negatives remain unequal.
This is neither a general-purpose Authenticode verifier nor an authorization to
strip signatures, replace DLLs or ignore arbitrary binary differences. The
[Microsoft PE specification](https://learn.microsoft.com/en-us/windows/win32/debug/pe-format)
explains the checksum/certificate fields; no Windows trust settings were changed.

| Binary | Official bytes / certificate bytes | Installed bytes / certificate bytes | All nonexcluded bytes |
| --- | --- | --- | --- |
| D3D12Core.dll |4,821,064 /10,312 |4,831,768 /21,016 | Identical |
| d3d12SDKLayers.dll |4,932,640 /10,272 |4,943,344 /20,976 | Identical |

Final receipt `Artifacts/C018H-DependencyInventory1/AgilityUpstream3.json` SHA256
`0bf862ed083a77f541506b8581a2f0c05b0efb5520cf99304166fb2f41bb34fd`
records actual raw hashes, both comparison hashes/ranges, document hashes,
file-list data, tool/input hashes and `rightsReview=pending_owner`. Actual live
build_review invocation terminal0/3.83s, corresponding .exit.json. Initial
AgilityUpstream1.json is partial/historical (PowerShell diagnostic string polluted
its document array; no file-list body or successful masked comparison). Native
CLI request2 terminal1/input_or_network_error and no output: observed receipt
AgilityUpstream2.exit.json retained. A later bounded HEAD succeeded; request3
completed the inspection. No timeout was treated as completion or used to restart
a live job; all owned jobs are terminal. Reuse3, not the partial1 or absent2.

Exact primary file-list/binary-payload/license-body binding is now resolved for
these two DLLs. Owner compliance still needs review of the installed agreement:
Windows use scope, application functionality, distributor/end-user protections,
indemnification, branding/source-license restrictions and retained notices, plus
any applicable separate third-party terms. Do not treat a redistributable listing
as an unconditional distribution grant or use the MIT header license for DLLs.
No EULA was silently authored/accepted, final notice bundle copied or release
gate approved. The111-byte package notice and existing QA archives are unchanged.

AgilityClosing1.json records repository validation0/0M1warnings, security0/
345candidate texts/no findings or policy failures, diff0,278preserved Git status
paths/empty index/HEAD0726de2/no named jobs, and final receipt/current tool-hash
equality. Earlier tests/LFS/native/archive/font evidence remains preserved; no
new LFS binary or source asset was added. All engineering checks are local,
not clean-candidate/remote CI, trusted player or owner distribution approval.

### XAudio2 primary payload/license-document mapping —2026-10-12

The installed xaudio2_9redist.dll is858,152bytes, SHA256
`0ca010abcabe52493fdb744d79c4c8132ae6756046b68aa7c88bdc03ec1effcd`;
SourceMappings2 already binds it to both QA configurations and the native stage
rule. Its PE FileVersion/ProductVersion1.0.0.1 does **not** identify a NuGet
package version. Narrow read-only inspection found only Build.cs in its module
folder, only the DLL in the selected binary folders, and no XAudio-named document
in the flat installed license folder. The guessed XAudio2_9.tps file was absent;
that inspection error is not a build failure or a global missing-license finding.

Bounded public package probes used the fixed primary NuGet URLs, in-memory ZIP
reads and the unchanged19-test PE comparison helper; no install, DLL loading,
SDK update, external README command, persisted raw binary/license text or engine
edit. All first/current and1.0.x candidate comparisons remained unequal outside
the permitted checksum/certificate fields. The next candidate produced a match:
[Microsoft.XAudio2.Redist1.1.0](https://www.nuget.org/packages/Microsoft.XAudio2.Redist/1.1.0),
`build/native/release/bin/x64/xaudio2_9redist.dll`,847,440bytes/raw SHA256
`0e01f400baa09694cacfd2cfbff0d721c17b56785d2d6e29b6fdd117f70ccc1e`.
All bytes outside checksum4bytes/security-directory8bytes/strict EOF certificate
table are identical to the installed/packaged DLL. The comparison SHA256 is
`30b54b840aee15fff8173ab07b0a83ca139b2cf0e3c8bc4aa64cfd2a279900f9`.
Both certificate tables start at838,144; official length9,296 vs installed20,008.
Full raw bytes/hashes remain different; no Authenticode trust/signature claim.

Matched package:14,263,605bytes/SHA256
`9a9b86a1d87184697a4b3858361d87a9ee4d1f66dfa4a7565febe092cc3750c4`.
The same pinned package's LICENSE.txt is11,462bytes/SHA256
`d64a0de294ac98d5982e7bdf783dc2ce45e6698d6fc101ac67ddf3ca74850a7a`,
specifically titled for Microsoft XAudio2.9 Redistributable. README is2,131bytes/
SHA256 `e07baadc8b24350a8ede530afdf02e08ba00b2e951d192e2b9539fa3191e4905`.
Both documents were read in memory. The README says to deploy release DLLs, not
debug DLLs; no build/rule/config was changed. Lookup stops at this match:1.1.1
was not requested and no unique-version identification among every release is
claimed. This resolves an exact **candidate payload and accompanying document**
mapping, not identity of all raw signing bytes or final contract applicability.

Owner review flag: the examined LICENSE section2(a) names an Image Library,
not an explicit DLL distributable file-list. Do not silently repair its wording
or substitute Agility's separate list. The [Microsoft developer guide](https://learn.microsoft.com/en-us/windows/win32/xaudio2/xaudio2-redistributable)
describes redistribution for the family; that does not by itself reconcile the
exact agreement's scope or grant unconditional distribution rights. Confirm the
applicable distributable-code scope and any Epic/Microsoft agreement, plus
application functionality, distributor/end-user protections, indemnity,
notice/export/branding/source-license obligations and conditional privacy/data
requirements. No claim that this game collects data or that the bundled DLL is
unlawful; no EULA/privacy-policy was authored or accepted automatically.

Final documentary receipt: DependencyInventory1/XAudioSourceReview1.json SHA256
`bd8345836f0fbe88ba0536c00b54422cf065e819b07fcefc243858d061b755ae`.
It binds existing source/staging receipt and probes2/4/5, reader hash, binary
comparison ranges, matched package/doc hashes and open owner decisions.
Probe2 SHA25691f1913e... records1.0.0/1.2.13;4 SHA394fac40... records1.0.1–1.0.4;
5 SHAcbcd946e... records1.1.0 and the actual match. All terminal0; no owned job.
Initial probes1/3 terminal1/no output;1 hit the8MiB cap, smaller than later
packages.3's fixed detail was not retained, so its recorded reason remains
bounded-query failure, not a guessed exception. Failure receipts retained.
Read-only HEADs exposed actual package sizes; the subsequent cap32MiB stayed
bounded. Never restart a live job on timeout, widen PE exclusions to force a
match, or requery these now-resolved candidates when switching clients.

No tool/parser/runtime/native/asset/QA archive change. Prior88Python/19PE tests
and native gates reused appropriately; final notices and owner compliance OPEN.
Keep the111-byte FreeType-only package notice unchanged until a reviewed bundle;
source mapping does not prove DLL loading, player audio recovery or trusted QA.

XAudioClosing1.json: repository0/security0/345candidate texts/no findings/policy
failures/diff0,278preserved status paths/empty index/HEAD0726de2/no named jobs,
reader and existing source-mapping receipt hashes unchanged. Actual closing shell
terminal0. Next unresolved exact dependency: DirectML1.15.4 versus older1.15.2 TPS;
do not repeat XAudio/Agility extraction/probes or native gates on client switch.

### DirectML1.15.4 selected primary package binding —2026-10-12

`Artifacts/C018H-DependencyInventory1/DirectMLUpstream1.json` SHA256
`d28bc6d85eb9bb1a1bdebe2d63ca66b83cf81b9fd21956872754d86ddfd291af`
rechecks the installed DLL and both existing QA DLLs before public requests.
All three are18,538,488bytes/SHA256
`f97b69379ecbdbab7e1dcd2b6170e7f285d367c86ee6718bc401c4d7cef1275c`.
DirectML.Build.cs SHA2569b3b983f... explicitly stages Game x64 into DML/x64;
the debug DLL is Editor-target-only, not Game Development. The older1.15.2 TPS
remains unchanged and is not the delivered version's authority.

The [official Microsoft.AI.DirectML1.15.4 package](https://www.nuget.org/packages/Microsoft.AI.DirectML/1.15.4)
is202,292,617bytes. New `Tools/nuget_range_reader.py` reads only selected public
HTTPS ranges: HEAD and every range pin the same strong ETag0x8DCF76FB90FC55A,
reject redirects/200 fallback/changed metadata before reading bodies, and enforce
size/request/time/ZIP-member limits. The index query found55entries and read4,306
bytes; the payload/documents query used18ranges/9,344,078bytes. No full SDK
download, extraction, installation, DLL execution or raw document persistence.
Whole-package SHA256 is explicitly null; range hashes and ZIP CRCs do not prove
whole-package signatures, trusted players or unique identity across releases.

Selected `bin/x64-win/DirectML.dll` is18,527,776bytes/SHA256
`9c9e6d822561c6c41b90e6994b3e8857cf1d66dbfb1e0c4c799c7c89b4e92da1`.
Raw bytes differ. All bytes outside ONLY checksum4/security-directory8/strict
EOF certificate table match, SHA256
`518c2f3fc40dd6e7ed2087bb21cf9b98296e8f0066e9c74d3d52b76766cdd4d7`.
Certificate offset18,517,504; local/upstream certificate sizes20,984/10,272.
The unchanged19-test PE helper was used with an explicit in-process32MiB binary
size limit (default16MiB is too small here); no exclusion or signature logic
changed. No Authenticode verification, certificate stripping or legal approval.

Same-package README346bytes/SHA25686e8e7c... explicitly assigns LICENSE.txt to
bin/ and LICENSE-CODE.txt to include/build. LICENSE.txt10,439bytes/SHA256
`a05138e3a085ff60a44881eedfa58dccb03ecc1d7b1f6ae888418e8c2fec4b8d`;
LICENSE-CODE.txt1,093bytes/SHA256903df551... is a separate MIT document, not
blanket MIT terms for the runtime DLL. Complete documents were read in memory.
The binary terms condition distribution/use on the agreement and Windows/Xbox
scope, preserve supplier notices and refer to separate third-party notices.
Export, applicable regional and conditional data obligations remain owner review;
the data clause is not evidence this game collects data or a privacy acceptance.

Installed DirectML_v1.15.4.license10,709bytes/SHA25682e90cab... is NOT an exact
raw or whitespace-only body match. `DirectMLLicenseComparison1.json` SHA256
`be5cde45329c909d8941eea0b89540a602f13f648f66aaebf8956ee8e6881f2f`
pins/rechecks the same upstream license and installed hash: after the266-byte
wrapper, official1,605 versus installed1,606 tokens differ only at doubled
cardinal-obligations quotes and a final standalone quote. This is a diagnostic,
not a license repair, semantic-equivalence finding or acceptance. Preserve both
hashes and have the owner review authoritative text; don't force equality.

ThirdPartyNotices.txt4,577bytes/SHA256
`2c95795c13ff48a58b6ed916f37901c23d964b5d9d601af422f17ad2172e7950`
identifies Microsoft/GSL (Microsoft2015, MIT), half (Christian Rau2012–2021,
MIT), and Zstandard (Facebook2016–present, BSD terms). Retain exact copyright,
license/disclaimer and binary-notice obligations in the reviewed final bundle;
do not translate/localize the notice. Its generic LGPL debugging exception is
not proof a particular LGPL library is retained. NuGet's no-dependency declaration
does not mean the DLL incorporates no third-party material. Googletest TPS alone
does not extend this exact notice list or prove retained runtime code.

RangeIndex1 receipt SHA256793393a1...; full hashes/requests/limits/member hashes
are in the receipts.12 synthetic offline range/ZIP tests pass; full Python100
tests:99pass/1existing symlink-capability skip, actual exit0, saved
DirectMLRangeTests1/DirectMLPythonFull1 .log/.exit. No native builds/player/asset
or engine changes justify rerunning previously approved native gates here.
DirectMLClosing1.json at18:44:20UTC: repository0/security0/347candidate texts,
no findings/policy failures/diff0/280statuspaths/indexempty/HEAD0726de2 unchanged,
namedNativeJobs0/tool+inputhashesmatch. No owned process pending. Final Markdown
security/diff checks are repeated after saving the handoff, not native gates.
RightsReview stays pending_owner; final NOTICES.txt and M7 release gates OPEN.
Next unresolved exact dependency: MsQuic2.2.0 versus older2.1.8 EULA metadata.
Reuse these receipts on switch; no repeated DirectML/Agility/XAudio downloads.

### MsQuic2.2.0 OpenSSL binding —2026-10-12; release security HOLD

`Artifacts/C018H-DependencyInventory1/MsQuicSourceReview1.json` SHA256
`726183e7364524e62401f38f696d89dc8a5894677f5de31013e5d947defaef36`
binds installed/BOTH QA DLL hashes checked BEFORE network:2,547,080bytes/SHA256
`5c6b07bf263dd8b85cb146b59468a22eb1fbd8635d4a6c56a8e41b67ce5eca2b`.
Native rule SHA736350ab... stages v220; malformed TPS/installed MIT document
reference2.1.8, retained unchanged rather than repaired or taken as DLL identity.

Bounded indexes of official Schannel/OpenSSL2.2.0 contain x64 DLLs of492,936/
2,536,368bytes. Only OpenSSL bytes were compared, no Schannel equality finding.
Primary [Microsoft.Native.Quic.MsQuic.OpenSSL2.2.0](https://www.nuget.org/packages/Microsoft.Native.Quic.MsQuic.OpenSSL/2.2.0)
has18entries/7,742,637bytes. Selected proof18ranges/1,247,594bytes pins strong
ETag0x8DB4580892EFA7F; wholePackageSha256=null. No SDK persistence/extraction/
install/execution/signature verification or unique-version claim. DLL SHA256
`150963f4a05ffddc9a8498ae0ec4ddf5bed762a83b6ad089d2fd8d499101c7ea`;
raw bytes differ, all bytes outside ONLY checksum4/security-directory8/strict EOF
certificate match, SHA6c604973... (cert offset2,526,208; sizes20,872/10,160).

Same-package nuspec1,010bytes/SHAc34f2ef8... specifies2.2.0+359654, commit
1903eac39ca42c6999d9cd0bde4a96f140a151a3 and MIT AND Apache-2.0.
LICENSE1,093bytes/SHA903df551... complete MIT body is contained in installed
MsQuic_License.txt after whitespace normalization, NOT raw-document equality.
README5,198bytes/SHAf1902e0f... and THIRD-PARTY-NOTICES10,778bytes/SHA256
`c823706397513b6eab16af7bd5ca30f8e98272712c71688ac246d7aec00fa5bb`
read fully in memory. The latter identifies OpenSSL and includes Apache2.0 terms,
not an exact embedded OpenSSL version. Owner-reviewed final bundle must retain
applicable MIT copyright/terms and Apache license/attribution/NOTICE, review
conditional modification/trademark/patent requirements. No acceptance/final copy.

**Release security HOLD, not merely attribution pending.** NuGet marks the
candidate deprecated/critical-warning. Initial MsQuicAdvisories1.json SHAc2dc0b36...
records four publisher advisories (High/CVSS7.5), NOT the complete current list.
The six-record update below supersedes that initial scope; actual game runtime
exploitability is unverified. Version2.2.0 lies within these affected ranges:

| Publisher advisory | CVE | Affected / listed patched versions |
| --- | --- | --- |
| [gvvw-8j96-8g5r](https://github.com/microsoft/msquic/security/advisories/GHSA-gvvw-8j96-8g5r) |2026-32179 |<2.5.7 /2.5.7,2.4.18 |
| [fr44-546p-7xcp](https://github.com/microsoft/msquic/security/advisories/GHSA-fr44-546p-7xcp) |2023-36435 |<2.2.3 /2.2.3 |
| [xh5m-8qqp-c5x7](https://github.com/microsoft/msquic/security/advisories/GHSA-xh5m-8qqp-c5x7) |2023-38171 |<2.2.3 /2.2.3 |
| [2x7m-gf85-3745](https://github.com/microsoft/msquic/security/advisories/GHSA-2x7m-gf85-3745) |2024-26190 |<2.3.5 /2.3.5,2.2.7,2.1.12 |

Patched lists are publisher metadata, not permission to replace Epic DLLs manually,
compatibility proof, complete CVE coverage or assurance an older patch fixes all
advisories. No exploit/network probe/DLL load/upgrade. Passing candidate-text/
explicit plugin-policy scanning does NOT clear engine-transitive findings.

MsQuicUsageScope1.json SHA4d448d78... checks only previously-selected184/179
engine descriptors and exact project Source/Config/uproject identifiers. No
explicit MsQuic/QUIC matches or descriptor dependency references; five descriptors
are non-strict JSON, textual candidates only, not repaired. MsQuic is default-
enabled/Runtime/PreDefault; inspected InitRuntime can load via GetDllHandle,
NOT an automatic StartupModule load claim. Implicit engine use/actual networking/
exploitability are unverified; identifier absence does not prove safe removal.

Owner chose RETAIN QUIC and evaluate a compatible update. Do NOT disable the
plugin. SourceReview1 records the earlier unanswered question; latest decision
and evaluation below supersede it. No config/plugin policy/engine/DLL/archive
changes. Keep security HOLD until reviewed mitigation/new-candidate verification.
Independent ONNX/oneTBB attribution may continue, no repeated SDK fetch/engine scan.
Unchanged range/PE tool hashes checked, prior100Python99pass/1capability skip
reused from DirectMLClosing1.json; no unjustified native/test rerun for documents.

#### Current update evaluation: retain QUIC —2026-10-12

MsQuicUpdateEvaluation1.json SHA256
`f5a1a041083fe1fff71e746d73194e2a898126b4ffef8ef32c20af74c8de6587`
binds OwnerDecision2 (exact accented answer preserved;1 had shell-encoding loss),
UpdateSources1 (six current primary advisories/support/latest release) and
Compatibility1 (bounded in-memory candidate-header comparison). Older receipt
pending-choice/four-advisory fields are historical, not current status.

The complete observed publisher page (6records,<100-page limit) adds:

| Publisher advisory | CVE / severity | Affected / listed patched versions |
| --- | --- | --- |
| [92f5-vc22-8j33](https://github.com/microsoft/msquic/security/advisories/GHSA-92f5-vc22-8j33) |2026-62815 / Critical RCE |<2.5.10 /2.5.10,2.4.19 |
| [w5f4-fx9m-m4q7](https://github.com/microsoft/msquic/security/advisories/GHSA-w5f4-fx9m-m4q7) |2026-105794 / High certificate validation (OpenSSL) |<2.6.1 /2.6.1,2.5.11,2.4.20 |

Thus the initial four High records did not explain all current vulnerabilities;
the publisher DOES now confirm a Critical advisory affecting the2.2.0 candidate.
Runtime exposure/exploitability still unproven. Neither2.4.18 nor2.5.7 resolves
the newer advisories, so those old patched lists are NOT update recommendations.

[Current Microsoft release/support policy](https://raw.githubusercontent.com/microsoft/msquic/main/docs/Release.md)
lists2.5 supported through2027-09-01 and2.6 supported (end TBD);2.4 support ended
2026-09-01. The observed latest non-prerelease is [2.6.2](https://github.com/microsoft/msquic/releases/tag/v2.6.2)
(2026-09-30), with additional OpenSSL fixes.2.6.2 is the primary upstream
evaluation candidate; [2.5.11](https://github.com/microsoft/msquic/releases/tag/v2.5.11)
is a supported alternative, not blanket CVE clearance or an approved drop-in.

Candidate headers retain API2 and the31 parsed installed API-table type/name
entries as the same prefix (2.5.11:37fields;2.6.2:39). This is textual preliminary
evidence ONLY: not full typedef/struct layout/macros/calling-convention/exports,
ABI, import-lib/link compatibility or TLS/runtime proof. No candidate DLL loaded
or installed, no unsafe networking probes/registry/firewall commands executed.

Preferred integration route: an Epic-supported distribution with a verified
patched dependency, preserving QUIC. [Epic's official5.8 announcement](https://forums.unrealengine.com/t/unreal-engine-5-8-released/2729274)
includes5.8.3 hotfix notes; the examined text doesn't identify MsQuic. Availability
is NOT proof5.8.3 replaces2.2.0 or fixes these advisories. Inspect its exact vendor
dependency/patch evidence before selecting it. No unverified engine-upgrade cure
or replacing only the DLL in Launcher-installed5.8.2. If vendor release still
ships old payload, assess explicit supported source integration in an isolated
candidate with matching headers/import library/OpenSSL runtime and rollback.

Before implementing: obtain authority for the concrete install/source-engine
migration, establish compatible toolchain/license/notices, build Editor/Game/IWYU,
verify API2/TLS certificate validation/handshake/connection lifecycle, fresh Dev/
Shipping cook/staging/version/hash receipts and trusted packaged regressions.
Engine changes require proportionate gameplay/input/save/performance checks;
old passing5.8.2 gates don't transfer automatically. Evaluation is completed to
this evidence boundary; compatibility and mitigation NOT achieved, release HOLD.

Owner follow-up—2026-10-12: supplied UE5.8.2/CL56702186 at
`C:\Program Files\Epic Games\UE_5.8`, MsQuic2.2.0.359654-official.
MsQuicOwnerBaselineConfirmation1.json records the targeted Build.version and DLL
identity check: DLL SHA256
`5c6b07bf263dd8b85cb146b59468a22eb1fbd8635d4a6c56a8e41b67ce5eca2b`
is EXACTLY the previously reviewed installed-source hash. This installation is
the existing compatibility baseline, not a new patched candidate or evidence
that the security problem was corrected. Integration-file availability reported
by the owner does not establish a complete engine-source repository. No repeat
source audit/advisory fetch/build/load or engine modification; reuse prior
evidence. Distinct vendor patch/integration evidence still needed, including
OpenSSL/curl; retain QUIC and security HOLD. Independent attribution may continue.

MsQuicClosing1.json at18:58:17UTC: repository0/security347candidate texts/no
findings/policy failures/diff0/280paths/indexempty/HEAD0726de2 unchanged/named
nativeJobs0/tool+inputs match. Validation1 log/exit0 retained; no process pending.
Structural checks pass, but upstream mitigation/release security explicitly HOLD.

### ONNX Runtime candidate source/header and document-role review —2026-10-12

OnnxAttributionReview1.json SHA256
`b34d60985351ebc936f32301b1c668f4013bf58e32880ff55d741ae530af90eb`
binds CommitLookup1/SourceReview1/TextComparison1 under DependencyInventory1.
Installed source and BOTH unchanged QA DLLs rehashed before primary requests:
17,339,384bytes/SHA256
`5df3d744b5cc4421181e9e2c97cadff4e9b1b2b25e80202160e80fc94ff948f2`.
NNEOnnxruntime.build.cs SHA22ad9155... explicitly delay-loads/stages this plugin
Win64 DLL; namespace Ort01241/API24 and Editor-only exceptions are declarations,
not a stable1.24.1 binary identity, runtime provider use or retained-code proof.

Reported version1.24.20260304.11.3a728b7 is distinct from1.24.1 metadata. Its suffix
resolves to public [commit3a728b75062256951b6e19ce718907cf1a1d4cf0](https://github.com/microsoft/onnxruntime/commit/3a728b75062256951b6e19ce718907cf1a1d4cf0),
dated2026-03-05 with1.24.3 release-cherry-pick message. This is candidate
self-reported suffix corroboration, not authenticated/reproducible DLL provenance
or proof the DLL is a final1.24.3 release. Selected installed C API header
382,197bytes/SHA7f03541b... matches the pinned upstream373,908bytes/SHA9ed0d705...
after ONLY CRLF to LF (8,289line endings); raw bytes differ. No header/code changed,
ABI/runtime assertion, comparison to a different release DLL or SDK installed.

Document-role mismatch: installed Onnxruntime_v1.24.1.license11,996bytes/SHA256
cea1491b... is wrapped **ONNX/Open Neural Network Exchange Apache2.0** text,
NOT the primary runtime MIT license. Adjacent onnx_vort.1.24.1.license11,991bytes/
SHAac4dfa87... and two explicitly selected wrapped TPS sources are recorded,
not parsed/repaired into permission. The runtime [pinned LICENSE](https://raw.githubusercontent.com/microsoft/onnxruntime/3a728b75062256951b6e19ce718907cf1a1d4cf0/LICENSE)
is MIT/Microsoft,1,073bytes/SHA256
`2f07c72751aed99790b8a4869cf2311df85a860b22ded05fa22803587a48922c`;
complete MIT body is NOT contained in the misnamed installed core document.
This narrow finding is not global absence of a suitable license or a violation.
Don't relabel the entire runtime Apache, discard ONNX's applicable Apache terms,
replace it with an older sibling runtime document or auto-accept terms.

Pinned [ThirdPartyNotices](https://raw.githubusercontent.com/microsoft/onnxruntime/3a728b75062256951b6e19ce718907cf1a1d4cf0/ThirdPartyNotices.txt)
325,054bytes/SHA256
`0e07b95f3a8d6230037707c5c4a2b554d12c4cb67369669ac255635528ffcee2`
fetched/hash-recorded in bounded memory.68delimiter-split segments including
preamble are NOT68 proven retained dependencies. Only the ONNX section was
examined for the role diagnostic; the other terms/provider applicability are NOT
all reviewed. Installed core body has1,597 versus official section1,596tokens,
21differences consisting only of doubled quotes/trailing standalone quote.
Strict raw/whitespace-only equality false; diagnostic correspondence is not
license repair, semantic-equivalence finding, blanket MIT/Apache coverage or
rights approval. Preserve exact copyright, actual applicable MIT/Apache and
per-component notices/conditions in the owner-reviewed final bundle.

Exact build flags/providers/retained components and final runtime source rights
remain unverified; current source/header candidate does not close that boundary.
No raw source/license/header persisted, DLL loaded/replaced, engine/TPS/config/
Content/QA archives changed. Existing tools/tests/native gates unchanged; prior
100Python99pass/1capability skip reused appropriately. Initial guessed TPS path
absent was a read-only lookup failure; corrected exact TPS used, not hidden.
Next independent attribution: oneTBB exact terms/source. MsQuic security HOLD
and owner RETAIN QUIC/update-evaluation-only decision remain fully in force.

OnnxClosing1.json at19:08:16UTC: repository0/security347candidate texts/no
findings/policy failures/diff0/280paths/indexempty/HEAD0726de2 unchanged/named
nativeJobs0/toolhashesmatch; OnnxValidation1 log/exit0 retained. No job pending.
Structural closing does not clear remaining ONNX rights/retained dependencies or
independent critical MsQuic mitigation/release hold.

## oneTBB2022.3.0 — source/staging verified, upstream binary identity OPEN

OneTbbUpstream1.json SHA256
6966401be451721166f34cdea8b924751c8f02c1a04b859863a9be7a03927598
records rehashed installed source and BOTH local QA DLLs. tbb12.dll342456bytes
SHAde15a5f93bfd334d4fe32343a81d76f9de0824af8e31b2189637d12545af2673;
tbbmalloc.dll117176bytes
SHA74edc04a8f5bc2593a5a555b675c1251d6d90e693ee6a67cfa11dd3b2d17a856.
Native IntelTBB.Build.cs4296bytes/SHA95dbbeb8... stages both from
Deploy/oneTBB-2022.3.0/VS2015/x64/bin; debug suffix requires Debug AND debug CRT,
not Development. Valid oneTBB.tps809bytes/SHA836f039a... declares2022.3.0 and
the exact upstream license; generic license-directory inventory was ambiguous,
not evidence of missing terms.

The [official release](https://github.com/uxlfoundation/oneTBB/releases/tag/v2022.3.0)
publishes SHAe1b2373f25558bf47d16b4c89cf0a31e6689aaf7221400d209e8527afc7c9eee
for its Windows ZIP. Bounded memory-only GET verified that full18457066byte hash
and316safe entries/75170235expanded bytes. Only production intel64/vc14 DLLs
compared: tbb12.dll348008bytes/SHAb24e99b40add5ce3132793c10f7325b8e9fdfb4db03465f5103cfb43146a5a9f;
tbbmalloc.dll139624bytes/SHA673ab4205b7ee7ac45895560a7ff9608031fe4973c8b9878a025de7b0e8e4a1a.
BOTH raw AND strict PE-normalized comparisons are FALSE. Unchanged19-test helper
excluded only checksum4/security-directory8/terminal certificates; no widening
to force equality. Different build origin/flags remain unverified, not a
corruption/malicious-change finding or exact authenticated upstream provenance.
No UWP/ia32/debug binary comparisons retained. Initial overbroad probe hit ia32
architecture guard and terminalexit1 before saving results; OneTbbProbeFailure1
preserves failure, corrected query terminal0. No live job was restarted.

Same-package LICENSE.txt11357bytes/SHA
c71d239df91726fc519c6eb72d318ec65820627232b2f796219e87dcf35d0ab4:
complete Apache2.0 body appears in installed oneTBB_LICENSE.txt11838bytes/SHA
a7bf4bcf31a8ff9d004976f7a1253eeb49ce3169ddaf5f83da12b99dd279b0c3
after whitespace normalization, NOT raw equality. Owner review must retain the
license and applicable notices, mark modified files, and consider patent and
trademark conditions. [Primary license](https://raw.githubusercontent.com/uxlfoundation/oneTBB/v2022.3.0/LICENSE.txt).

Same-package third-party-programs.txt11631bytes/SHA
1634857eb058dce5278c57a832b8974190785145f4be9acdbdb511f3a0c4091c
read completely: ITT Notify, hwloc, gperftools/Tachyon, GCC ABI workaround,
Python thread pool and doctest. These are package-wide candidates, NOT proof all
are retained by the two Windows DLLs. Relevant copyright/conditions/disclaimers
need final attribution review. Conditional GCC Runtime Library Exception text
does not make the whole DLL GPL or establish an LLVM exception. Selected legal
documents contain no LLVM exception. [Primary notice source](https://raw.githubusercontent.com/uxlfoundation/oneTBB/v2022.3.0/third-party-programs.txt).

OneTbbAttributionReview1.json binds findings and next steps. Exact Epic build/
source/flags/modifications and retained-component applicability/owner rights/
final notice bundle remain OPEN. No package/license/DLL raw persistence,
installation/execution/replacement, engine/config/Content/archive/tool/test
changes or signature/trust approval.111byte FreeType-only staged NOTICES stays
unchanged; prior100Python/native gates reused for unchanged code. NEXT independent
Ogg/Vorbis terms or exact vendor integration evidence; no repeat SDK downloads.
Owner RETAIN QUIC/evaluate-only decision and Critical MsQuic release HOLD remain.

OneTbbClosing1.json at19:19:48UTC: repository0/security347candidate texts/no
findings/policy failures/diff0/280paths/indexempty/HEAD0726de2 unchanged/named
nativeJobs0/toolhashesmatch. OneTbbValidation1 log/exit0 retained; no pending job.
Current structural checks do not establish upstream binary identity or clear
owner rights/retained notices/independent Critical MsQuic mitigation.

## Ogg/Vorbis — staging and BSD candidates reviewed, exact builds OPEN

OggVorbisSourceReview1.json SHA256
093cea2128d2b50fe4d6d5034291d165fd5232870d4c4295779da3ea9fd425da
rehashes installed sources and BOTH QA configurations:

| DLL | Bytes | SHA256 |
|---|---:|---|
| libogg_64.dll | 59832 | f23df1213c4bf23b5c666bafa7ec18743847a2e18192ae8e0d096a2fb7c28bf9 |
| libvorbis_64.dll | 1724856 | a56d43a98e197f5e73a1dadf5c1dc7a07ad40327b199faa4ff1247abd6eccd5b |
| libvorbisfile_64.dll | 48568 | d64e67ca918716be8b6056a6f7a1f6256e910754425db8369d952943264b40eb |

Entire UEOgg/Vorbis/VorbisFile.Build.cs read: Windows x64 VS2015 import paths,
delay loads and RuntimeDependencies explain all three staged files. Declared
source directories are libogg1.2.2/libvorbis1.3.2, NOT authenticated DLL source
identity. Selected bounded printable strings in libvorbis include1.3.2 and
20101101(Schaufenugget); strings corroborate a candidate, not patch/security/
runtime proof. No selected aoTuV banner does not exclude incorporated aoTuV code.
The ue-module-build-system skill guided staging-vs-link declaration separation;
no build rules were changed.

Pinned primary candidates:

- [Ogg1.2.2 COPYING](https://raw.githubusercontent.com/xiph/ogg/23d9360b46ae26303e9e544da52dca16938bdeff/COPYING):1466bytes/SHAd2ab5758336489da61c12cc5bb757da5339c4ae9001f9bb0562b4370249af814, copyright2002.
- [Vorbis1.3.2 COPYING](https://raw.githubusercontent.com/xiph/vorbis/a1f20a587a8c97edb1feca565c85c89a29e46d43/COPYING):1470bytes/SHA414bfd5d8ee64395a9231d6386188461500052c10915bee08b18bfa9489dd0b3, copyright2002–2008.

Tag-to-commit resolutions and selected ogg.h/codec.h hashes are in the receipt.
BOTH headers differ raw AND after CRLF-to-LF normalization. Follow-up
OggVorbisHeaderComparison1.json SHA
433623d8226636472e59d3eb11129e41f86c79a0ebc06309c65e78be1a3d5ce7
diagnoses ONLY the expanded historical $Id comment on line14 after splitlines;
no broader semantic/ABI/DLL equivalence or source-tree completeness asserted.
Only21617bytes in first bounded primary GETs, plus two small header diagnostics;
raw upstream files never persisted/installed/executed.

Valid OggVorbis.tps explicitly names installed OggVorbis1.2.0_License.txt;
Vorbis.tps redirects to it. That2998byte/SHA9aa6318d... document is UTF-16 with
BOM and copyright2002–2004. Separate1558byte/SHA40edfd19... quality document
contains aoTuV2003–2008/Aoyumi and libvorbis2002–2008/Xiph. They are NOT one
combined text file; no copying/repair/copyright-year replacement. Full upstream
COPYING texts do NOT appear verbatim after whitespace normalization in the
older combined-product-named document. Their condition/disclaimer bodies,
excluding first copyright line, do match both installed candidates after
whitespace normalization. Preserve exact attribution identities, not just BSD
family names. Candidate terms require retained source notices or reproduced
binary notices/conditions/disclaimers and restrict endorsement; owner rights
and applicable final materials remain OPEN.

Quality TPS says used as part of libvorbis; exact aoTuV version/modifications/
retained code still unverified. The [author's primary site](https://ao-yumi.github.io/aotuv_web/)
documents multiple historical integrations; its modern family license is NOT
substituted for exact shipped terms. Exact Epic source/build/patch provenance
and separate native security review remain OPEN; no security clearance from
old directory/banner/header matches.

Initial UTF-8 read rejected UTF-16 before any public GET; terminalexit1/no
receipt/partial results retained in OggVorbisProbeFailure1. Corrected existing
bounded read_text respects BOM; source review and diagnostic terminal0. No
tool/test/C++/engine/Content/config/context/archive/binary/license change;
unchanged100Python/native evidence reused. Final111byte FreeType-only NOTICES
unchanged. NEXT exact Aftermath terms, then remaining DbgHelp/WinPix/Vulkan
candidate applicability, without repeating SDK downloads. QUIC retained,
evaluate-only owner choice/Critical release security HOLD unchanged.

OggVorbisClosing1.json at19:27:09UTC: repository0/security347candidate texts/no
findings/policy failures/diff0/280paths/indexempty/HEAD0726de2 unchanged/named
nativeJobs0/three toolhashesmatch; Validation1 log/exit0 retained. No pending job.
Structural check does not clear exact build/patch/retained code/rights or Critical
MsQuic mitigation. Goal remains ACTIVE/M7 OPEN.

## Aftermath2025.5 — staged DLL and legal roles reviewed, rights OPEN

AftermathSourceReview1.json SHA256
21f836b0c4ad5384cd2f96f37f3585ee0d49caa486940fe9cc7cfb69203ac026
records rehashed installed and BOTH QA GFSDK_Aftermath_Lib.x64.dll5799016bytes/
SHA82d7a3e50710d660927db2f318cbed471049c962b7c05de12c2b2b3e40a35af9.
Existing byte-bound version receipt reports file2.26.0.0/product2025.5.0.0
(build36881664). Defines header API0x21A/2.26 and declared TPS2025.5.0.251114
corroborate a family, NOT exact authenticated upstream build or public package
identity. No DLL loaded/disassembled/changed or SDK binary requested by the
bounded receipt script; earlier native declarations remain reused, not rebuilt.

Exact version-named installed .license266bytes/SHA
654ea863fd389323dabe19322f1fd96af22872cbc4f76ee50bacccafc2f8e5d2
contains ONLY metadata wrapper/URL, no contract body. TPS878bytes/SHA6806a4e4...
points to a generic license directory with an empty end-user entry. Neither is
proof of redistribution permission or violation. Adjacent Aftermath_NVAPI_License.txt
14704bytes/SHA6eb29635057cff751ccb7135e5751710713a9a26b129b7b25cfbcfff6a6d1328
is a materially different legacy SDK candidate: strict UTF-8 invalid, bounded
cp1252 diagnostic read preserves raw bytes; not an encoding declaration/repair.
Its contract includes derivative/object-code scope, end-user protection,
confidentiality, term and export conditions; exact2025.5 applicability unknown.

[Current primary legal page](https://docs.nvidia.com/nsight-aftermath/CopyrightAndLicenses/index.html)
56534bytes/SHA2434af82b2f7fe86eab568f7935cb36002bb06af9bdcb59f226a25e30bcbe299
includes a2019 SDK agreement plus Aftermath supplement identifying .dll/.so
redistribution under agreement conditions. Owner must verify exact agreement,
application-functionality/access, end-user IP/privacy/security protections,
restrictions, usage guidelines and export compliance. Package-wide JSON/MIT,
Flatbuffers/Apache, DXC/LLVM-NCSA and xxHASH/BSD candidate notices need retained
component review, not unconditional applicability. This current page does NOT
replace the installed legacy agreement or prove accepted rights for exact DLL.
No agreement accepted or final notice text copied.

[Primary2025.5 release notes](https://developer.nvidia.com/nsight-aftermath/getting-started/release-note-v2025.5)
24503bytes/SHA978efbd0bf63798c4f7deb4bfcd45c3834e09f426d94f26d4cf39bbb308f165c
corroborate the GPU crash dump editing API present in installed headers, not
build36881664/251114 hash identity. Current download navigation points to a
different2026.3 package and failed in web tooling; no SDK bytes acquired. Do NOT
retry/persist its signed redirect query or infer a global download/auth failure.
Guessed /2025.5/ legal URL inaccessible via web tooling is not HTTP404/global
absence proof. Two subsequent bounded public HTML GETs succeeded, no raw source
persistence/install/load/login/terms acceptance/engine mutation.

Exact SDK payload/contract binding, pass-through rights, retained notices and
usage guidelines remain OPEN. Crash-reporting role does not prove enabled
telemetry or privacy behavior; no crash/upload experiment performed. Existing
100Python/native gates reused after tool hashes;111byte FreeType-only archive
NOTICES unchanged. NEXT DbgHelp/Dev WinPix/Vulkan applicability and consolidated
owner review; preserve QUIC/evaluate-only and Critical release security HOLD.

AftermathClosing1.json at19:33:03UTC: repository0/security347candidate texts/no
findings/policy failures/diff0/280paths/indexempty/HEAD0726de2 unchanged/named
nativeJobs0/three toolhashesmatch; Validation1 log/exit0 retained. No pending job.
Not exact contract/payload/retained-notice/rights/security/release approval.

## DbgHelp / Development WinPix / Vulkan — candidate terms and scope reviewed

DependencyTailReview1.json SHA256
9c36396de0824145aabd38ccb737fbc59ce7f781bf1be6d40cc9e2b9996e8190
rechecks nine exact installed/Development DLL hashes and DbgHelp in Shipping.
Seven Vulkan JSON manifests also match installed source AND immutable archived
Development manifest hashes; each names its paired DLL. Their API1.4.341 matches
VK_HEADER_VERSION341, but is NOT authenticated SDK/component commit identity.
Profiles implementation1.3.0 and GFXReconstruct description1.0.5 are distinct
manifest fields, not interchangeable with Vulkan API or SDK version. Previously
verified UAT Debug/Development-only staging is reused without reopening/building
rules. Shipping DLL path-set excludes WinPix/seven layers; no claim no retained
static PIX/Vulkan code or no actual runtime usage.

DbgHelp2232688bytes/SHA36795e47... reports10.0.25291.1000; binary-folder TPS
names SDK10.0.19041.0 and EWDK terms, not exact shipped provenance. [Microsoft
distinguishes OS and Debugging Tools distributions](https://learn.microsoft.com/en-us/windows/win32/debug/dbghelp-versions):
OS-shipped DbgHelp is not redistributable; tool installer policies are different.
This does NOT prove our engine-supplied DLL is an OS copy or an approved loose
redistributable. [TPS-linked EWDK candidate](https://learn.microsoft.com/en-us/legal/windows-sdk/license-terms-ewdk)
requires applicable file lists/conditions, not blanket SDK permission; exact
25291 package/list/contract and owner compliance OPEN. No OS copying/replacement,
installer download, agreement acceptance or SDK upgrade. Two primary HTML hashes
are in the receipt; declarations/old-version references preserved.

WinPixUpstream1.json SHA256
115adb19ff6dcbdb7db0d7f554d84bef4bcad87c54cd3297ad887e3432b8d624
inspects fixed [Microsoft package1.0.240308001](https://www.nuget.org/packages/WinPixEventRuntime/1.0.240308001)
173282bytes/SHA726acc93d6968e2146261a1e415521747d50ad69894c2b42b5d0d4c29fd66ec4,
22safe entries. Production bin/x64 DLL58368bytes/SHA81adcfd8... differs from
installed/Development69056bytes/SHAba40d206... raw, but ALL bytes outside narrow
PE checksum/security-directory/terminal certificates match (SHAa81aa7f9...).
Same strict19-test helper, no broadened exclusions/signature/trust verification.
Nuspec explicitly selects1093byte license.txt/SHA903df551...: Microsoft MIT,
not legacy PIX proprietary terms or an unrelated EULA redirect destination.
Keep copyright/permission/disclaimer in applicable distribution materials.
28402byte ThirdPartyNotices/SHA3f06560b... decoded completely by script; tool
rendering truncated. Introductory18-program listing/select sections inspected,
NOT full clause/retained-component review. Broader PIX informational notices do
not establish all18 are incorporated into this runtime DLL. Official devblog
403 and legacy EULA redirect to unrelated web-development page remain explicit;
no access bypass/terms substitution. No raw package/DLL/license persistence,
install/execute or archive change; final rights/notices OPEN.

Vulkan_License.txt is a registry description, NOT umbrella MIT/Apache terms.
Installed Validation Layers_v1.3.250.1.LICENSE header/Apache declaration and AMD
VMA MIT appendix inspected; this older named candidate cannot be applied blindly
to seven manifest1.4.341 layers. [Publisher registry](https://vulkan.lunarg.com/license/)
renders only a SPA shell in current web tooling; actual per-component records
were NOT read. Guessed1.4.341.0 notes inaccessible is not HTTP404/global absence.
Exact SDK package, component commits, modifications and retained notices OPEN;
no large SDK/installer fetch or modern family-license substitution. Absent guessed
Source/ThirdParty/Windows/WinPixEventRuntime lookup was read-only; inventory's
actual WinPix module directory is Windows/PIX, not global missing-source proof.

## Native-family review coverage — dynamic DLL scope only, M7 still OPEN

NativeFamilyCoverage1.json SHA256
d2d10e5d662854da324b9e5885ced204e086b27520210d2fcb778aa3baa6cb65
binds21Development/13Shipping DLL rows across11families to existing exact-hashed
review receipts. Each row has ONE family and a distinct evidence grade; every
rights/final-notice status remains OPEN. Engineering family coverage is complete
for those DLL inventories, NOT full retained/static source/content/rights/security
coverage.87Shipping/94Development unique declared link inputs remain declarations,
not proof all retained components/notices reviewed. Font/provider-specific gaps,
exact vendor contracts/builds, final staged bundle and owner release gates remain.

NEXT bounded static-linked notice/applicability gap review or exact supported
QUIC integration evaluation, reusing existing exports/receipts. Asked owner for
version/path of any other installed candidate Unreal/source tree (no credentials)
to inspect actual patched payload; no answer/installation authority inferred.
QUIC retained/evaluate-only and Critical MsQuic release security HOLD unchanged.
No tools/tests/C++/engine/Content/config/context/native archive/notice changes;
unchanged100Python/native gates reused after tool hashes. No owned live process.

TailClosing1.json at19:42:30UTC: repository0/security347candidate texts/no
findings/policy failures/diff0/280paths/indexempty/HEAD0726de2 unchanged/named
nativeJobs0/three toolhashesmatch; TailValidation1 log/exit0 retained. No pending
process. Dynamic family index does not clear static/rights/security/release gates.

## Declared link/TPS review index — candidates, not retained static code

New offline Tools/link_attribution_index.py SHA256
b158938f698303f7dba3680531f7ff9bfd625e0f0bbb485014d165d37765d8e7
joins existing inventory receipts only. Same-origin longest directory association
provides candidate modules, not actual linking ownership; equal-directory owners
remain ambiguous. TPS redirects/statuses/hash references preserved, never parsed
as permission. Platform import names are not concrete redistributed files;
.lib files remain static-or-import UNDETERMINED, all retained-code/terms/final
notice statuses OPEN. No engine/library scan/load/build/network/license copying.

LinkIndexReview1.json SHA256
201c1edc22a5d8bef9bc57ed0a3b6b38e9c19bc29db4f6d5a79cd57ff9151007
binds LinkIndex-Development1 SHA2d3933bb12eba4db4c840e3184fdb1416baadfaa1f7b8ddb7050c958b7d1cacf
and LinkIndex-Shipping1 SHAe77dc89e6af03b9200bf1d06e36d7a5880abf6e342744f4a1bc3952f06e1859b.

| Declared-input candidate status | Development | Shipping |
|---|---:|---:|
| Has module/TPS candidates (not license approval) | 48 | 45 |
| No TPS in selected source-module inventory | 7 | 3 |
| Outside exported module directories | 4 | 4 |
| Equal-directory module ambiguity | 2 | 2 |
| Platform import name/SDK contract review | 33 | 33 |
| Total | 94 | 87 |

Actual file-backed .lib inputs61/54; system import names33 each. Missing TPS here
does NOT imply missing license: WinPix terms/binary-folder TPS already reviewed.
Vorbis/VorbisFile directory ambiguity is heuristic; earlier native declarations
and explicit BSD candidates remain authoritative. Four unassociated paths are
libpas, RadAudio decoder, BinkAudio static decoder and OodleNetwork SDK2.9.16,
not proof native ownership or rights are missing. Other Shipping gaps include
OodleDataCompression2.9.16, UEJpegComp and Draco; Development adds null-profiler
inputs. Next review uses those exact paths/candidates, not repeated dynamic
SDK/font exports or a broad engine/repository scan.

CLI rejects unsafe/reparse/out-of-Artifacts input/output paths, duplicate records,
invalid hashes/origins/revisions, failed inventories, overlong redirects and
expanded candidate budgets; output create-only after bounded validation. Input
hash checked before/after indexing; output <=32MiB/candidate metadata <=16MiB,
8192association/reference caps. Original helper unchanged. New14synthetic tests
pass; full offline Python114total113pass/1existing symlink-capability skip. Actual
exit0/logs LinkIndexTests1 and LinkIndexPythonFull1 retained. Synthetic tests do
not establish engine retention/legal rights/native runtime or protect against
every filesystem race. No native reruns justified by offline index code.

Reproduction: python Tools/link_attribution_index.py --project . --inventory
Artifacts/C018H-DependencyInventory1/Inventory-Shipping2.json --output
Artifacts/C018H-DependencyInventory1/LinkIndex-Shipping2.json (new path only).
Do not overwrite1 or rerun old SDK/network/native gates when changing clients.
Owner candidate engine/source version+path question remains unanswered; QUIC
retained/evaluate-only/Critical release HOLD and remaining owner gates unchanged.

LinkIndexClosing1.json at19:53:04UTC: repository0/security349candidate texts/no
findings/policy failures/diff0/282paths/indexempty/HEAD0726de2 unchanged/named
nativeJobs0/five toolhashesmatch; Validation1 log/exit0. Two new offline Python
files account for status/text-count increase. No pending job;114Python proof
current. Structural closing does not clear retained-code/rights/Critical HOLD.

## Exact source-gap follow-up — declarations resolved; attribution OPEN

StaticGapSourceReview1.json SHA256
3926cd858bdcb3afe67bbc6b224fddb24431c7bdeba4686bd489f5f9c15098e2
binds both existing native exports/indices, seven unchanged library hashes and
selected installed source notices. Each path has exactly one PublicLibraries
declaring module in BOTH exports; directory heuristics were not native ownership
failures. Original LinkIndex receipts/statuses remain unchanged.

| Exact declared input | Native declaring module |
|---|---|
| libpas.lib | libpas |
| radaudio_decoder_win64.lib | RadAudioDecoder |
| binka_ue_decode_win64_static.lib | BinkAudioDecoder |
| oo2net_win64.lib | OodleNetworkHandlerComponent |
| oo2core_win64.lib | OodleDataCompression |
| uejpeg_w64.lib | UEJpegComp |
| draco.lib | Draco |

libpas parent TPS explicitly describes a modified WebKit fork, not an owner
distribution approval. Complete first-comment notices in22 supplied UE headers
have BSD-2-Clause-style conditions with Epic2023(15),2025(6),2023-2024(1)
copyrights. Original fork/build/retained notices remain unverified; preserve
all copyright years rather than substituting one generic BSD document.

RadAudio decoder copyright is Epic Games Tools LLC; RADA_LIBRARY_VERSION1 is
compatibility numbering, NOT release identity. Bink decoder/UEJpeg public header
are Epic Games Inc copyright notices, not standalone redistribution grants.
Oodle SDK2.9.16 Data/Network headers declare9/16; base copyright1994-2026 differs
from Data1994-2022 and Network2018-2022. Accepted Epic/SDK contracts and exact
binary builds are not established by directory names or these headers.
Draco installed version header reports1.5.6/Apache2.0/2016DracoAuthors; header
notice does not establish binary-source identity or all transitive notices.

UEJpeg Source/uejpeg.zip115000bytes SHAe73b1ffb41c9cf1853aa3a034740bdca3cfdacf401a70c88434ed0062ea3c69e
has6entries/492950expanded bytes. Bounded in-memory inspection hashed5file
members, without extraction/raw copies. uejpeg.c declares STB_IMAGE_IMPLEMENTATION
and includes stb_image.h (lines23/28); its complete license footer offers MIT
(2017SeanBarrett) or Unlicense/public-domain dedication. Owner choice/applicability
OPEN, not proven retained in uejpeg_w64.lib/the game. Bundled Oodle header9/13
and1994-2024 base copyright are distinct from native SDK2.9.16; preserve this
source mismatch, do not label it an actual packaged Oodle version or repair it.

DevProfilerSourceReview1.json SHA256
89791ac128c5f47368a1518c29aa3984c4723d408cb6ab9c2d18cfb93d57f748
binds three additional unchanged Development library hashes and exact declaring
modules ConcurrencyVisualizer/SuperLuminal/VSPerfExternalProfiler. These paths
are absent from selected Shipping index, not proof of absence from every binary.
Targeted filenames in three directories/matching ThirdParty/Licenses yielded
no license/notice/TPS/header candidates; NOT global absence or exemption.
Null filenames are not retained-code proof; Build.cs files were not read/changed.

Ignored one-off ReviewStaticGaps1.py is hash-bound by its receipt and terminal0;
not a new reusable product tool or native gate. Initial inline query failed
KeyError:path on platform-import rows before any receipt write; corrected optional
path lookup succeeded. Earlier PowerShell foreach-pipeline parse error likewise
made no changes. All retained-code/static-import types/rights/final bundles OPEN.
Existing114Python/113pass/1capability skip and native gates reused for unchanged
tools/runtime; no downloads, engine/config/Content/archive/license/security writes.
NEXT consolidate applicable original/transitive notice and retained-object/vendor
evidence from existing indices, NOT redoing these10declarations. Owner-supported
QUIC candidate version/path still pending; retain/evaluate-only/Critical HOLD.

StaticGapClosing1.json at20:06:48UTC records repository0/0M1warnings,
security349candidate texts/no findings or policy failures/diff0/282paths/
indexempty/HEADunchanged/namedNativeJobs0/five product toolhashesmatch.
Completed command observations retained, no standalone validator logs fabricated;
ignored research receipts/scripts do not change product test/runtime scope.
No owned pending process or release approval.

## zlib / libPNG / FreeType source-document binding — binary retention OPEN

CompressionFontCoreReview1.json SHA256
3cc511ccb7cef75284413455f331d8fb96b1021fd22689084f07ed7622f39bfe
binds three unchanged library hashes in BOTH existing indices, selected TPS/
license hashes, seven installed documents and six primary version-tag texts.
Bounded public GETs431620bytes total, no redirects/raw persistence/install/native
execution. Ignored one-off script terminal0, hash-bound in receipt. Version tags
are source candidates, not authenticated immutable library build identities.

Headers report zlib1.3/libPNG1.6.44/FreeType2.14.1 and match their selected primary
headers after ONLY CRLF-to-LF; raw text differs. Installed libPNG source LICENSE
and FreeType FTL likewise match the primary bodies after ONLY CRLF-to-LF.
These selected-file matches do not prove complete source trees, patches, objects
retained in the game, .lib format or accepted distribution rights.

The central zlib notice is a whitespace-only matching prefix of installed/primary
zlib.h before its data-format discussion. Central version-named libPNG license
does NOT match primary/source LICENSE after whitespace:768tokens each, ONLY
two aligned copyright tokens differ (1995-2025/2018-2025 vs1995-2024/2018-2024).
Wrapper/name/year mismatch preserved; do not rewrite copyrights or assume
legal equivalence/current upstream text applies to all compiled code.
[zlib v1.3 header](https://raw.githubusercontent.com/madler/zlib/v1.3/zlib.h),
[libPNG v1.6.44 LICENSE](https://raw.githubusercontent.com/pnggroup/libpng/v1.6.44/LICENSE).

FreeType's pinned root LICENSE requires selecting FTL or GPLv2-or-later, with
separate package-wide candidates for BDF/PCF/fthash X-style, gzip zlib, autofit
HarfBuzz Old MIT and development-debug MD5. Not all are proven retained in this
library/game. Installed FTL is not the entire license-choice/transitive package
document. Under the FTL candidate, binary distribution needs a FreeType credit
in documentation; source/modified-source conditions differ. Owner choice,
applicable exceptions/notices and rights remain OPEN; no GPL mandate inferred.
[FreeType 2.14.1 root LICENSE](https://raw.githubusercontent.com/freetype/freetype/VER-2-14-1/LICENSE.TXT),
[pinned FTL](https://raw.githubusercontent.com/freetype/freetype/VER-2-14-1/docs/FTL.TXT).

Existing Shipping NOTICES111bytes/SHAba49ae5586f817b36a2772269f71f899cb1f54b5f2720fad951319edd5e140b8
matches its recorded manifest and contains the FreeType-only credit without a
year. This is not a complete attribution bundle or owner license selection;
FTL's suggested year wording is distinct from its binary-credit requirement.
Notice/archive untouched. Web docs/LICENSE.TXT tooling errors are not HTTP404
or global absence: the verified choice document lives at ROOT LICENSE.TXT.
NEXT source/build/support evidence for declared OpenSSL1.1.1t/libcurl8.15.0,
independent of QUIC. Source versions do not establish retained binary CVEs.
Existing114Python/native gates reused for unchanged product code; no M7 closure.

## OpenSSL / libcurl native source and publisher review — security HOLD

NetworkSourceReview1.json SHA256
e4aab2cad9a8ef8a7759afd038fe1221f8c1c3f13c47ee4a4ed6ac16c59b20b5
binds three unchanged library hashes in BOTH indices and one exact declaring
module each in BOTH native exports: OpenSSL/libcurl. Libraries contain banners
OpenSSL1.1.1t/7Feb2023 and libcurl/8.15.0, respectively; banners/declarations are
not retained-object, complete build/source/patch identity or .lib-type proof.
Four installed headers/notices and eight primary text/metadata sources hashed;
bounded public195866byte GETs, no redirects/raw persistence/install/execution.
One-off ignored script terminal0/hash-bound; no product tool/native changes.

OpenSSL version header and complete installed license match pinned1.1.1t source
after ONLY CRLF-to-LF. The legacy document applies OpenSSL AND original SSLeay
conditions together, not a choice, ordinary MIT or modern Apache substitution.
Review required copyright/conditions/disclaimers/acknowledgments, conditional
advertising and Windows-app-code clauses against retained code/distribution.
[Pinned OpenSSL1.1.1t license](https://raw.githubusercontent.com/openssl/openssl/OpenSSL_1_1_1t/LICENSE).

curl installed header8.15.0/date2025-07-16 differs from version-tag header ONLY
in two macros: upstream8.15.0-DEV/[unreleased]. Header documents release-time
generation; this difference is NOT evidence that Epic made insecure modifications.
Central curl notice uses1996-2020/plain email vs pinned COPYING1996-2025/angle
email. Entire documents differ; bodies starting All rights reserved match only
after whitespace. Preserve original copyrights/document roles, no repair or
automatic generic MIT replacement; exact applicable binary notice remains OPEN.
[Pinned curl COPYING](https://raw.githubusercontent.com/curl/curl/curl-8_15_0/COPYING).

Current publisher policy lists OpenSSL1.1.1 as no longer supported normally,
with separate extended support available. This is not proof of owner/Epic
extended-support contracts or vendor patches. Do not swap to any3.x DLL/lib:
vendor compatibility and its API/ABI/build/rights need independent evidence.
[OpenSSL release strategy](https://openssl-library.org/policies/releasestrat/).

curl's current8.15.0 publisher JSON contains43 advisories (29Low/14Medium), each
explicitly listing8.15.0; original SEMVER events/branch fixes and metadata retained.
This is source-version candidate coverage, NOT43 verified vulnerabilities in
the game, exhaustive engine CVEs or all feature-conditions reviewed. TLS backend,
compiled protocols/features, vendor backports, retained code and runtime exposure
remain UNVERIFIED. [Publisher version data](https://curl.se/docs/vuln-8.15.0.json).

Two condition sets reviewed: CVE-2025-14819 depends on OpenSSL/fork TLS, reused
handles, changing NO_PARTIALCHAIN and enabled CA-store caching; not all TLS.
CVE-2025-9086 depends on secure-cookie/same-host cleartext-HTTP path replacement;
not all requests or the CLI. Their individual fixes are not a version solving
all43. [TLS advisory](https://curl.se/docs/CVE-2025-14819.html),
[cookie advisory](https://curl.se/docs/CVE-2025-9086.html).

Security/release remains HOLD until exact supplier patches/features/retention and
trusted behavior are evaluated. This is INDEPENDENT of existing MsQuic Critical
HOLD; retaining QUIC does not grant engine migration, library replacement or
network probing authority. Owner candidate Unreal VERSION+PATH still pending;
its evaluation must include these dependencies, not MsQuic alone. No explicit
mitigation was implemented. NEXT independent ICU/HarfBuzz/libWebSockets notice
candidates or exact vendor candidate evidence, not repeating these source GETs.

## ICU / HarfBuzz / libWebSockets notices — candidate mapping, rights OPEN

TextWebSocketSourceReview1.json SHA256
6e60d961470a7db0328fe13f6760f9e3f298ff251f3a4c09c911344d9dae8734
binds three unchanged library hashes/unique declaring modules in BOTH native
exports and twelve selected installed files. Six primary version-tag GETs200,
60821bytes total, no redirects/raw persistence/install/execution. Header versions
ICU64.1/HarfBuzz2.4.0/libWebSockets3.0.0, not complete binary build/retention proof.

ICU64/HarfBuzz2.4 TPS reference //UE4/Main depot-style paths, not valid local paths.
Explicit same-basename local Licenses files found; references were NEVER followed
as UNC/network shares or repaired in inventories. This resolves candidate lookup,
not rights. Newer malformed78.1/12.2 TPS and old51.2 license remain separate.

ICU64 and HarfBuzz2.4 central notices fail strict UTF8/BOM decoding; strict CP1252
is a diagnostic candidate only, original bytes/hash/EOL unchanged. ICU contains
repeated CRs: complete decoded notice equals pinned64.1 after whitespace only,
not raw/CRLF-only; version header matches ONLY CRLF-to-LF. Six package-wide
sections reviewed: legacy ICU, Chinese/Japanese dictionaries, Lao, Burmese,
timezone database, double-conversion. Not six proven retained dependencies.
[ICU64.1 license](https://raw.githubusercontent.com/unicode-org/icu/release-64-1/icu4c/LICENSE).

HarfBuzz selected header copyright comment matches pinned generated-header
template after ONLY CRLF-to-LF. Central Old MIT notice differs from pinned
COPYING: local Google2010–2019 versus primary2010–2012 plus local Facebook2019.
Permission/disclaimer body matches after whitespace, not complete document
equivalence. Preserve all copyrights; individual files/additional COPYING still
need applicability. Source UCDN notice matches primary after CRLF-to-LF; central
notice differs only in trailing whitespace from source, not complete per-file
copyright/retention proof. [HarfBuzz2.4 COPYING](https://raw.githubusercontent.com/harfbuzz/harfbuzz/2.4.0/COPYING).

libWebSockets TPS says1.7.3 while installed header/config indicates3.0.0. Central
205byte notice is UE4 credit/source locator, NOT full license or evidence of
accessible exact modified source. EpicGames/ThirdParty was not accessed.
Pinned3.0.0 license has LGPL2.1 with four exceptions: static-linked program
source/relinking is not automatically required, but modified-library conditions
remain; credit still needed, other source licenses conditional. Relevant
exceptions and sections1/2/4 reviewed, not whole legal-compliance approval.
Exact modifications/source fulfillment/retained components/owner rights OPEN.
[libWebSockets3.0.0 license](https://raw.githubusercontent.com/warmcat/libwebsockets/v3.0.0/LICENSE).

TextWebSocketDiagnostics1.json SHA256
be1423ab45034a815bd5de6cd399dbfc917eb54b73e6e704e887d21f4bd76fc1
hash-checks ONLY two necessary follow-up GETs (HarfBuzz mismatch/WS section4),
records token diagnostics and section4 lines285–297 hash, not raw text copies.
Initial UTF8 queries each exited1 before any receipt/GET; corrected bounded
research terminal0. Web ICU root/LICENSE404 and other cache misses are NOT
global absence; actual icu4c/LICENSE GET200. No engine/notice/archive changes,
original indices unchanged,114Python/native proofs reused. Security HOLD still
unmitigated. NEXT remaining BLAKE3/MikkTSpace/LibTiff/Opus/media/vendor cohorts
or exact owner-provided Unreal candidate, not repeated source/license GETs.

## Tests and reproduction

Original dependency-inventory cohort (before font/SDK additions): targeted21/21
tests pass. Pure Python suite58 total/57 pass/1 existing real-symlink
capability skip; independent synthetic reparse guard passes. Actual native exit0,
`PythonTestsFinal1.log/.exit.json`. PowerShell5.1 wraps native unittest stderr in
a NativeCommandError display record; it is not a failed unittest/native exit.
Negative cases cover hashes/missing/extra files, case duplicates, traversal/UNC/
ADS, wrong configuration/project/main EXE, test module, invalid schemas, module/
link containment, XML entities (UTF8/UTF16), malformed TPS, redirects/cycles,
legacy text encodings, bounds, redacted errors and create-missing-only output.

Closing local checks: repository validation exit0/0M1warnings; redacted candidate
gate exit0/340 texts/no findings or policy failures; Git LFS fsck and diff check
pass. `AcceptedInputsFinal1.json` matches all77 prior accepted source/asset hashes.
HEAD0726de2/main/index empty;273 dirty/untracked status paths preserved. No new
native gameplay/full UE suite/IWYU/cook/package/visual rerun for tool/docs changes.

For a fresh receipt, use the SAME existing inspected archive/inputs and a NEW
output filename under Artifacts. Do not rerun JsonExport/build/cook/player merely
to switch models. Example Shipping (project/engine absolute roots as configured):

```powershell
python Tools/packaged_dependency_inventory.py --project (Get-Location).Path --engine 'C:/Program Files/Epic Games/UE_5.8/Engine' --archive Artifacts/Packages/C018H_LocalQA1/Shipping/Windows --manifest Artifacts/Packages/C018H_LocalQA1/Shipping/archive-sha256-Shipping.json --target Artifacts/C018H-DependencyInventory1/Target-Shipping.json --plugins Artifacts/C018H-PakInspection1/Shipping/RomanEmpireGame/Plugins/RomanEmpireGame.upluginmanifest --response Intermediate/Build/Win64/x64/RomanEmpireGame/Shipping/RomanEmpireGame-Win64-Shipping.exe.rsp --configuration Shipping --output Artifacts/C018H-DependencyInventory1/Inventory-Shipping3.json
python -m unittest discover -s Tools/tests -p test_packaged_dependency_inventory.py
```

For Development use its corresponding paths and `RomanEmpireGame.exe.rsp`.
Verification success means structural inventory only, not release/license CI
promotion. Do not auto-upload metadata or add all installed licenses to a package.

## Remaining boundary

Subsequent [delivered-font review](M7_FONT_ATTRIBUTION_REVIEW.md) separately covers
all15 raw-font entries in the recorded Pak indexes (14distinct hashes), with
fresh scoped extraction/source identity/name metadata. This extends the previous
engine-Content exclusion only for those explicitly selected font paths; it is
not blanket asset/license rights coverage, UI changes or a final notice bundle.

No clean committed release, retained-code/PE import certification, platform/version
applicability resolution, exhaustive asset/plugin/SDK-rights determination or
legal/distribution approval. Selected module subtrees are not all engine Content,
SDK directories, private caches, Git history or every sibling plugin TPS.
The111-byte packaged NOTICES.txt remains unchanged. A reviewed final notice bundle
and exact license/version mapping must be completed for the clean candidate.

Goal remains ACTIVE. Local work here is meaningful progress and supersedes the
latest "no safe local substitute" snapshot; no external choice was supplied.
Clean candidate/isolated CI/trusted player/owner FPS/hardware/version/distribution
and applicable Epic/SDK rights still require owner decisions. No commit, push,
SDK installation, remote settings, credentials or Windows trust change authorized.
Bounded read-only primary public metadata/package requests above are research,
not permission to install/execute, replace dependencies or accept their contracts.

Skills fully read: ue-module-build-system (target/module/link distinctions,
relevant context sections), unreal-packaging (package scope is not player proof).

### BLAKE3 / MikkTSpace / LibTiff source-notice candidates —2026-10-12

HashTangentTiffSourceReview1.json SHA256
`e21cc069213f384d5ac8106fa68b0543392a544a81077aaaf59f26a4ef03f279`
binds three unchanged library hashes and unique declaring modules in BOTH existing
native exports/indexes. BLAKE3.lib172982bytes/SHA5d0c14a4..., MikkTSpace.lib118436/
SHAceb9df77..., tiff.lib1216956/SHA6804b019.... Static/import type, retained code,
exact binary builds, owner rights and final bundle remain OPEN.

- BLAKE3 header1.3.1 matches the [pinned primary header](https://raw.githubusercontent.com/BLAKE3-team/BLAKE3/1.3.1/c/blake3.h)
  only CRLF→LF. Central Blake3_License.txt19019bytes/SHA261eae54... matches the
  [complete pinned notice](https://raw.githubusercontent.com/BLAKE3-team/BLAKE3/1.3.1/LICENSE)
  only after whitespace normalization, NOT raw/CRLF-only identity. Alternatives
  are CC0-1.0 OR Apache-2.0, not MIT. Exact basename maps the depot-style TPS
  candidate without UNC access or index repair; applicable branch remains owner-reviewed.
- MikkTSpace local header describes1.0 and copyright2011 Morten S. Mikkelsen.
  Central2934byte/SHA1e9df955... notice equals the local copyright+author comments
  whitespace-only. Against [author repository pinned header](https://raw.githubusercontent.com/mmikk/MikkTSpace/3e895b49d05ea07e4c2133156cfa94369e19e409/mikktspace.h),
  copyright comment matches CRLF-only but WHOLE header differs. Hash-checked
  Diagnostics1 finds exactly one whitespace-token insertion: local
  `tbool m_bIgnoreDegenerates;` plus explanatory comment. No full-source-revision,
  ABI or binary identity inferred. Restrictions preserve source notices, origin
  and marking of altered source; product acknowledgement is appreciated, not
  required by this notice. Generic TPS license directory and separately selected
  central filename do not constitute unique index resolution. Separate
  ComputingTangentSpace TPS LicenseFolder=None is not a license/exemption;
  referenced technique applicability/retention remains unresolved.
- LibTiff header/version4.2.0/date20201219 and central1167byte/SHA8f859efd...
  notice match [pinned version](https://gitlab.com/libtiff/libtiff/-/raw/v4.2.0/libtiff/tiffvers.h),
  [header](https://gitlab.com/libtiff/libtiff/-/raw/v4.2.0/libtiff/tiffio.h) and
  [COPYRIGHT](https://gitlab.com/libtiff/libtiff/-/raw/v4.2.0/COPYRIGHT) only CRLF→LF.
  Notice/copyright preservation and advertising-name restrictions are not generic
  MIT substitution. TPS directory remains ambiguous; central candidate selected
  explicitly, not repaired. TPS describes static zlib/turbojpeg integration,
  not actual embedded archive versions/retention or transitive license clearance.

Seven bounded public GET200/58978bytes + one8209byte hash-checked diagnostic GET;
no redirects, raw source/license copies, private access, install/execute, native
build, engine/config/library/license/QA mutation. Strict existing text reader
succeeded; no encoding repair. Web cache misses were not actual HTTP failures.
Prior114Python/native gates reused for unchanged product code. Security HOLD
unchanged; NEXT Opus/media/vendor candidates from the existing index, or genuinely
distinct vendor-patched Unreal candidate. No repeat of completed source queries.

HashTangentTiffClosing1.json SHA0c5f67be10659e781773aefb3c5de9e8dff8dae98351efc3b5e854d5ec6faa3c:
repository0/0M1warnings; security0/350texts/no findings or policy failures; diff0;
JSON/script/diagnostic binding checks0;283dirty paths/indexempty/HEAD0726de25...
unchanged. No owned live process. This does not clear dependency advisories,
owner rights, clean revision/CI, trusted player or full M7/release gates.

### Opus / Speex / VPX / WebM source-notice candidates —2026-10-12

AudioVideoSourceReview1.json SHA256
`cbbc0e3d8bf22ba671317eebf57aa29c8273dd3f0e9d269a210cb4ae80e6de9b`
binds FIVE unchanged libraries and unique declaring modules in BOTH exports:
libOpus declares speex_resampler.lib57620bytes/SHAb74752ee..., opus.lib789432/
SHA612c84d9..., opus_sse41.lib29440/SHA0679762a...; LibVpx15660670/SHAec5fb6b3...;
LibWebM477056/SHA9b56dae1.... No static/import/retained-code or exact binary-build
proof. Headers/notice candidates are not a completed release attribution bundle.

- Opus path1.3.1-12, but selected archive banner is `libopus unknown`: exact
  binary version/12-after-tag commit not established. Header matches [v1.3.1](https://raw.githubusercontent.com/xiph/opus/v1.3.1/include/opus.h)
  CRLF-only, central Opus_License.txt1973bytes/SHA3503c933... matches [COPYING](https://raw.githubusercontent.com/xiph/opus/v1.3.1/COPYING)
  whitespace-only, not raw/CRLF-only. BSD3 root notice and separate BSD2-style
  API-header copyright2010–2011/Xiph/Skype retained as candidates. COPYING links
  IETF patent records1524/1914/1526, not examined for owner applicability here.
  Generic TPS2017/Redistribute=false/LicenseFolder LibOpus and old1.0.3 remain
  metadata, not a legal violation or grant. Old exact central filename is absent;
  installed1.4 summary and1.5.2 generic text are not substitutes.
- Speex resampler is a distinct declared dependency inside opus-1.1 directory,
  NOT proof of SpeexDSP version. Old and current-include local headers are raw
  identical15190bytes/SHA5c3afb55.... Copyright2007 Jean-Marc Valin/BSD3-style
  first notice matches [SpeexDSP-1.2.1 notice](https://raw.githubusercontent.com/xiph/speexdsp/SpeexDSP-1.2.1/include/speex/speex_resampler.h)
  whitespace-only; WHOLE header differs, so do not assign that release to the
  installed archive or merge its copyright into the Opus root document.
- LibVpx path/banner1.14.1: BOTH root and nested local codec headers match
  [pinned header](https://raw.githubusercontent.com/webmproject/libvpx/v1.14.1/vpx/vpx_codec.h)
  CRLF-only. Exact TPS central license filename absent, not global absence or
  violation. [Pinned BSD3-style LICENSE](https://raw.githubusercontent.com/webmproject/libvpx/v1.14.1/LICENSE)
  candidate has source/binary notice and endorsement restrictions.
- LibWebM path1.0.0.27: local1526byte/SHA17c029a9... LICENSE.TXT and parser header
  match [pinned license](https://raw.githubusercontent.com/webmproject/libwebm/libwebm-1.0.0.27/LICENSE.TXT)
  and [header](https://raw.githubusercontent.com/webmproject/libwebm/libwebm-1.0.0.27/mkvparser.hpp)
  CRLF-only. Source license2010Google and header2012WebM authors are distinct,
  preserve both. Old central TPS filename absent; Linux TPS wording doesn't
  negate the observed Windows declarations. No retained runtime use inferred.

AudioVideoSupplement1 corrects ONLY the initial WebM PATENTS URL404 interpretation:
actual [PATENTS.TXT](https://raw.githubusercontent.com/webmproject/libwebm/libwebm-1.0.0.27/PATENTS.TXT)
GET200/1436bytes/SHAcc3273e0... is byte-identical to [VPX PATENTS](https://raw.githubusercontent.com/webmproject/libvpx/v1.14.1/PATENTS).
Full grant examined: Google-licensable claims necessary to specified implementations,
not claims introduced by further modification; patent-enforcement termination.
Not universal game/codec/owner patent clearance. Original receipt not overwritten.

Eight initial GET200/112985bytes, one exact PATENTS URL404, one follow-up GET200/
1436bytes; no redirects, raw copies, private access, install/execute or native/
engine/license/QA mutation. Initial inline platform-row KeyError and exact-path
PowerShell missing-reference errors occurred before research receipt/GETs; those
absences are narrow, not license findings. Corrected script terminal0 and all
input/source/library hashes rechecked. Reuse114Python/native gates unchanged.
NEXT nghttp2/LibJpegTurbo/UEOpenAPV/Imath/UEOpenExr and remaining GPU/DirectX
declaration scope; do not repeat already reviewed dynamic families. Security/
owner rights/clean revision/CI/trusted player/final notices remain OPEN.

AudioVideoClosing1.json SHA5fdf9a3634ad57d87fb0fb3e47ae17e9dfe4e0ddfec071713269e8a1c5beb8e8:
repository0/0M1warnings; security0/350texts/no findings or policy failures; diff0;
JSON/script/supplement bindings0;283dirty paths/indexempty/HEAD0726de25...
unchanged, no owned live process. Engineering source-notice progress only,
not release attribution/security rights, trusted player or full M7 completion.

### nghttp2 / JPEG / APV / Imath / OpenEXR / OpenJPH candidates —2026-10-12

ImageHttpSourceReview1.json SHA256
`f787c8d7df321d11bf19f96856c2d51cd71989ada85cb7a6af585cb5b41928ce`
and ImageHttpSupplement1.json SHA95adeed20e248232d20dd1d1d734dfa116c0142cb2bf15ac641753d073504148
bind TEN unchanged libraries and unique declaring modules in BOTH existing native
exports: nghttp2, LibJpegTurbo, UEOpenAPV, Imath and six UEOpenExr inputs. The sixth
is separately licensed OpenJPH, not a sixth OpenEXR-only copyright. Static/import
type, objects retained, exact library builds/patches and release applicability
remain unproven. Original directory-ambiguous/malformed TPS statuses preserved.

- nghttp2 local header1.64.0; legacy central notice equals [pinned COPYING](https://raw.githubusercontent.com/nghttp2/nghttp2/v1.64.0/COPYING)
  CRLF-only. Version-named central notice includes metadata; body starting at
  MIT heading matches whitespace-only, not complete-file identity. Preserve
  Tatsuhiro Tsujikawa/nghttp2 contributor copyrights. TPS static HTTP2/libcurl
  description is not retained/runtime-network exposure proof.
- JPEG Turbo path3.0.0/header matches [pinned TurboJPEG header](https://raw.githubusercontent.com/libjpeg-turbo/libjpeg-turbo/3.0.0/turbojpeg.h)
  CRLF-only. Central notice requires explicit strictCP1252 diagnostic candidate
  decoding, with raw bytes unchanged; WHOLE notice doesn't match [3.0.0 LICENSE.md](https://raw.githubusercontent.com/libjpeg-turbo/libjpeg-turbo/3.0.0/LICENSE.md)
  even whitespace-only. Local D.R.Commander2009–2020 vs primary2009–2023 and
  formatting differences preserved. IJG, ModifiedBSD3 and zlib apply to different
  components, not an OR-choice. Publisher's binary/static distribution guidance
  calls for IJG credit in product documentation; TurboJPEG API use also requires
  its BSD notice. [Pinned README.ijg](https://raw.githubusercontent.com/libjpeg-turbo/libjpeg-turbo/3.0.0/README.ijg)
  is distinct from central JPEG_License.txt, which identifies release6b/1998;
  do not substitute that older document for this build. Actual retained scope,
  per-file copyrights, notices and owner compliance remain OPEN.
- APV TPS labelsv0.2.0.1 but is quoted/doubled-quote malformed metadata; central
  notice also has wrapper/doubled quotes. WHOLE local API header and notice do
  NOT match [pinned header](https://raw.githubusercontent.com/AcademySoftwareFoundation/openapv/v0.2.0.1/inc/oapv.h)
  or [license](https://raw.githubusercontent.com/AcademySoftwareFoundation/openapv/v0.2.0.1/LICENSE).
  Hash-checked follow-up proves only first copyright/license comment CRLF-only
  match (Samsung2022/BSD3-style). No quote repair, whole-source identity,
  library-version proof or unconditional patent rights inferred.
- Imath local config3.2.2 and OpenEXR config3.4.3 are generated field evidence,
  not build identity. Version-named central files include metadata wrappers;
  bodies beginning at copyright match [Imath3.2.2](https://raw.githubusercontent.com/AcademySoftwareFoundation/Imath/v3.2.2/LICENSE.md)
  and [OpenEXR3.4.3](https://raw.githubusercontent.com/AcademySoftwareFoundation/openexr/v3.4.3/LICENSE.md)
  CRLF-only. Preserve their distinct BSD3 notices; Mac2.3.0 and image-sample
  licenses are not substitutes or all-image attribution approval.
- OpenJPH filename0.24, local macros0.24.5 and version header match [pinned header](https://raw.githubusercontent.com/aous72/OpenJPH/0.24.5/src/core/common/ojph_version.h)
  CRLF-only, not exact library build. Separate BSD2 header credits Aous Naman,
  Kakadu and UNSW2019; [root LICENSE](https://raw.githubusercontent.com/aous72/OpenJPH/0.24.5/LICENSE)
  names UNSW Sydney whereas header doesn't. Keep notices/scopes separate,
  don't collapse this dependency into general OpenEXR BSD3 attribution.

Ten primary GET200/151418bytes + two hash-checked diagnostic GETs/31458bytes;
no redirects, raw source/license copies, private access, install/execute, native
build or engine/library/notice/QA mutation. Initial strict-reader failure exit1
before any GET/receipt corrected only for explicitly identified JPEG notice;
shared helper untouched. Exact absent jconfig.h diagnostic isn't global absence
or native failure; existing TurboJPEG header used. Corrected research script and
follow-up exit0; hashes guarded before/after. Reuse114Python/native gates.
NEXT remaining GPU/DirectX declaration-source scope (AMD_AGS, Intel extensions/
VTune, NVAPI and selected DirectX libraries); preserve/reuse prior dynamic-family
receipts. Security/owner rights/clean revision/CI/trusted player/final bundle OPEN.

ImageHttpClosing1.json SHAecfb394040c6efb66f66e5d709f647dbf696f44d7c0de95b38cc94cf7fbafe5d:
repository0/0M1warnings; security0/350texts/no findings or policy failures; diff0;
JSON/script/supplement bindings0;283dirty paths/indexempty/HEAD0726de25...
unchanged, no owned live process. Source/notice engineering progress only,
not complete retained-code attribution, rights/security mitigation or M7 release.

### GPU / DirectX declaration candidates —2026-10-12

GpuDeclarationReview1.json SHAe51761ab024e675e1fd36134523e29a57e34b185181c08d97c40a2419097490a
and Supplement1 SHA94a4f2b31f70878258458fa722e9a89dacaa5e7df2a7ad4a87e712037191658c
bind TWELVE unchanged libraries in BOTH native exports/indexes. Four GPU/ITT
libraries have unique native declarers; DirectX is a DIRECTORY association,
NOT the actual declaring module. Exact native associations, identical in both:

| DirectX library | Actual declaring modules |
| --- | --- |
| dxgi | DX11, DX12 |
| d3d12 | DX12 |
| dxguid | DX11, DX12, DirectSound, DX11Audio |
| d3d11, dinput8 | DX11 |
| xapobase | DX11, DX11Audio |
| XInput | XInput |
| dsound | DirectSound |

Original index/directory-candidate/TPS statuses untouched. Static/import type,
retained objects and general library/whole-source build provenance unverified.

- AGS local header6.0.1 matches [publisher header](https://raw.githubusercontent.com/GPUOpen-LibrariesAndSDKs/AGS_SDK/v6.0.1/ags_lib/inc/amd_ags.h)
  CRLF-only. Published [VS2017 x64 MD library](https://github.com/GPUOpen-LibrariesAndSDKs/AGS_SDK/blob/v6.0.1/ags_lib/lib/amd_ags_x64_2017_MD.lib)
  has EXACT matching raw hash89e1e6ba.../214264bytes, not a PE-normalized match.
  [MIT notice](https://raw.githubusercontent.com/GPUOpen-LibrariesAndSDKs/AGS_SDK/v6.0.1/ags_lib/LICENSE.txt)
  associated with that exact SDK candidate. Referenced local LICENSE.txt absent
  at the exact TPS path, not global absence or a licensing violation.
- Intel Extensions header copyright2025/Intel/SPDX MIT; exact TPS central filename
  absent. No exact public source/library candidate established. Header identifier
  is not complete redistributed notice text. IGCL, other API examples and DXVK
  compatibility shim are not substitutes for this Windows integration.
- ITT header API3.0/copyright2005–2019 explicitly GPL-2.0-only OR BSD-3-Clause;
  VTune-2023 folder doesn't prove SDK/binary release. Central commit-named file
  contains a README, NOT a full alternative license. Public prefix5b8a7d7 resolves
  [full commit](https://github.com/intel/ittapi/tree/5b8a7d7422611c3a0d799fb5fc5dd4abfae35b42);
  selected header, README body and [BSD notice](https://raw.githubusercontent.com/intel/ittapi/5b8a7d7422611c3a0d799fb5fc5dd4abfae35b42/LICENSES/BSD-3-Clause.txt)
  do not wholly/whitespace-only match local counterparts. Preserve local2005–2013
  BSD notice and newer header separately. Branch/effective build/per-file scope
  remain owner-reviewed; not an automatic requirement to GPL the whole game.
- NVAPI guessed R570 ref URLs returned404; bounded tag/branch lookups gave no
  matching ref, not global source absence. First public commit page identified
  [R570 SDK commit ce6d2a18...](https://github.com/NVIDIA/nvapi/commit/ce6d2a183f9559f717e82b80333966d19edb9c8c).
  Header CRLF-only matches and nvapi64.lib978386bytes/SHA3b33fdf3... has EXACT
  raw hash equality to publisher library. [Pinned License.txt](https://raw.githubusercontent.com/NVIDIA/nvapi/ce6d2a183f9559f717e82b80333966d19edb9c8c/License.txt)
  EXPLICITLY licenses nvapi.lib/nvapi64.lib under MIT. Local version-named notice
  has metadata instead of publisher's library-applicability introduction; BOTH
  bodies from SPDX copyright onward match CRLF-only. Complete-file equality is
  false; retain applicability statement and2024 root/2019–2024 header credits.
  Legacy custom NVAPI/Epic/Aftermath contract reviewed earlier remains separately
  scoped; exact current library match doesn't relicense Aftermath or settle all
  owner agreements/whole-game rights. No repeated Aftermath SDK download/access.
- DirectX TPS June2010/LicenseFolder=None isn't permission/exemption; header
  custom/MIT metadata doesn't cover eight library inputs or Agility binaries.
  [Publisher download page](https://www.microsoft.com/en-us/download/details.aspx?id=6812)
  identifies legacy/EOL SDK9.29.1962.1,571.7MB installer, not exact source-package
  ownership/redistribution file-list proof. No installer downloaded or executed,
  no OS DLL copied/replaced. Existing XAudio/Agility/WinPix evidence reused,
  not blanket clearance of these legacy inputs. Source/version/rights pending.

Initial script assumed directory association==native owner, exit1 BEFORE GET or
receipt. Exact DirectX map above fixed only the ignored research guard; corrected
script exit0, all source/library/input hashes checked before/after. Seven initial
GET200/498048bytes + three guessed-ref404; follow-up primary history/metadata and
NVAPI three-file comparison2324994bytes + two1217byte license diagnostics, no
redirects/auth/raw copies/install/execute/private access. SDK library bytes were
read only in bounded memory; no persisted binaries or vendor/Epic mutations.
Existing114Python/native gates reused. NEXT reconcile coverage of61Dev/54Ship
file-backed declarations against existing receipts and resolve actual remaining
Development-only candidates; do not repeat source queries. Rights/retention/final
notices/security/clean revision/CI/trusted player still OPEN, M7 not complete.

GpuDeclarationClosing1.json SHA9225c0780d3c8ca2cbc361fa400718d4710347d6ae10b1d68db6e55afa3fb715:
repository0/0M1warnings; security0/350texts/no findings or policy failures; diff0;
JSON/script/supplement assertions0;283dirty paths/indexempty/HEAD0726de25...
unchanged, no owned live process. Two raw publisher-library matches and corrected
native declaration mapping do not close full M7/rights/security/runtime gates.

### Declaration reconciliation and Development tail —2026-10-12

DeclarationReviewCoverage1.json SHA256
54ce9d2abfd0c518e7b82c651b4658ba4d6eeb63f7a9af831b99aad73511604c
matches exact adjacent path+hash references ONLY inside nine selected existing
source-review bodies. Baseline:49/61 Development and46/54 Shipping;33 platform-name
declarations per configuration excluded separately, not missing file-backed inputs.
Input receipts and every concrete library hash checked before/after. This ledger
does not treat input/index references or a neighboring DLL review as source coverage.

CompanionImportBinding1.json SHA256
eaca76d8b70f1dbf87e0f212d907914dc8a28d2e15187785fd69ef07fc50fada
adds nine exact library identities without repeating their family source review.
Bounded read-only archive parsing found AMD64/version-zero short import records
under the [Microsoft COFF import format](https://learn.microsoft.com/en-us/windows/win32/debug/pe-format#import-library-format).
Actual PublicLibraries declarers verified in both native exports, NOT inferred
from directory candidates or inherited delay-load entries:

| Library | Declaring module | Import-record DLL name |
| --- | --- | --- |
| tbb12.lib, tbbmalloc.lib | IntelTBB | tbb12.dll, tbbmalloc.dll |
| libogg_64.lib | UEOgg | libogg_64.dll |
| libvorbis_64.lib | Vorbis | libvorbis_64.dll |
| libvorbisfile_64.lib | VorbisFile | libvorbisfile_64.dll |
| GFSDK_Aftermath_Lib.x64.lib | NVAftermath | GFSDK_Aftermath_Lib.x64.dll |
| msquic.lib | MsQuic | msquic.dll |
| DirectML.lib | DirectML | DirectML.dll |
| WinPixEventRuntime.lib (Development only) | WinPixEventRuntime | WinPixEventRuntime.dll |

Import names associate libraries with prior hash-bound QA DLL/family receipts;
they do NOT prove exact DLL build/ABI, actual imported or retained objects,
runtime invocation or applicable terms. Three non-short/non-linker members in
each library remain unclassified: no claim these archives contain only imports.
Parser synthetic checks passed: valid short import, BigObj version2 NOT classified
as an import, five malformed bounds/header/string cases rejected. No DLL execution.
Intermediate combined identity references:58/61 Development,54/54 Shipping.

DevTailSourceReview1.json SHA256
27831e36a0ac6748ec194c01a48817ff8f50dd2e8822121debd465d7d4a7d68c
adds the THREE genuinely new Development-only scopes. Library hashes, unique
actual declarers and absence from Shipping PublicLibraries/index checked in both:

- Detours: local header macro0x4c0c1 and TPS4.0.1; full detours.h/detver.h differ
  from [publisher v4.0.1](https://github.com/microsoft/Detours/tree/v4.0.1), even
  whitespace-normalized. Two exact central notice candidates found without
  repairing ambiguous TPS metadata. Permission-through-disclaimer bodies match
  the [pinned MIT notice](https://raw.githubusercontent.com/microsoft/Detours/v4.0.1/LICENSE.md)
  whitespace-only; full files do not. Explicit exclusions are metadata, title,
  copyright punctuation and all-rights-reserved introduction, all preserved.
  Detours_LICENSE.txt labels5.3, NOT proof this library is5.3 or a replacement for
  the4.0.1 candidate. Exact binary source/patches and retained notice scope OPEN.
- DirectShow: installed SDK agreement identifies Windows7/.NET4, not current SDK
  terms or blanket .lib redistribution. Raw12233-byte notice remains unchanged;
  strict UTF8 fails, identified-file strictCP1252 diagnostic candidate recorded,
  shared strict reader unchanged. Public sample commit434f6002bdf9cf9829406c3ff2b33387982d6168:
  [combase.h](https://raw.githubusercontent.com/microsoft/Windows-classic-samples/434f6002bdf9cf9829406c3ff2b33387982d6168/Samples/Win7Samples/multimedia/directshow/baseclasses/combase.h)
  matches CRLF-only; streams.h differs even whitespace-normalized. Headers retain
  Microsoft1992–2001 notices, not exact compiled source proof. Owner must identify
  actual marked-sample/distributable/REDIST scope and accepted agreement. If that
  agreement applies, linked-result distribution, primary functionality, protected
  downstream terms, program copyright and Windows scope are conditional checks;
  Windows Media credit is conditional on those SDK portions, not every DirectShow
  use. No modern sample-license substitution, OS copy or unconditional grant.
- LiveCoding LC_JumpToSelf: supplied212-byte assembly SHA
  f6c23430fbe3db032ff4d3c9a65f34122ccb241806a77b20e8e59c032bce50b0
  identifies Molecular Matters2011–2020/all rights reserved. Neighboring xxHash TPS
  explicitly describes hashing/Live++ update, NOT assembly/library licensing;
  do not assign xxHash BSD terms to JumpToSelf. Exact Molecular Matters/Live++/Epic
  rights require owner evidence. No disassembly/recompile/run/private-source access.

Final identity-reference presence:61/61 Development,54/54 Shipping, zero remaining
file-backed declaration identities without a receipt. This is NOT complete source
attribution, retained-code coverage, all transitive/platform/provider/font notices,
owner rights, final attribution bundle or M7 completion. Baseline indexes and their
unresolved/ambiguous TPS statuses unchanged; NOTICES111-byte FreeType-only unchanged.
Six bounded public GET200/59855 bytes; all public source bytes memory-only, no
redirect/auth/raw copies. All three ignored scripts actual exit0, inputs/library
hashes stable. No product/native/engine/license/QA/trust/remote mutations. Existing
114Python/native gates reused. Security QUIC+OpenSSL+curl HOLD unchanged.

NEXT consolidate outstanding owner/provenance/retained-object/notice obligations
using these receipts, not another declaration/source scan. Evaluate a genuinely
distinct patched compatible engine if supplied; same UE5.8.2 baseline is not one.

DeclarationCoverageClosing1.json SHAc057ffcb538366e9273a97d21dee386f7ea82813e419397461f60d8acbbec6fd:
repository0/0M1warnings, security0/350texts/no findings or policy failures, diff0
(prior EOL warnings only), JSON/script/unique61/54 identity-set assertions0;
283dirty paths/indexempty/HEAD0726de25... unchanged, no owned live process.
First query at assumed validate_security.ps1 failed by filename; actual
security_gate.py found/run0, not an engine/security failure. Goal ACTIVE/M7 OPEN.

### Owner-action consolidation —2026-10-12

[Pending actions ATR-00–14](RELEASE_ATTRIBUTION_ACTIONS.md) separate security,
source/build/contract evidence, retained-object scope and final notices for an
exact clean candidate. Not an approval or another source/license search.

AttributionActionCoverage1.json SHA256
77c46f7e8784b597ddeab17e3b6801d6e3d1be230a5617436036a6d4de2af735
verifies28input receipts before/after, resolves exact JSON pointers rather than
trusting receipt names, and maps61Development/54Shipping library identities into
11disjoint action groups with actual native PublicLibraries owners. Separate
21/13DLLs in11families retain prior graded proof and hash-bound reviews; no sum
of library/DLL counts is asserted as the complete dependency/licensing inventory.
Platform-name33-per-config, fonts/content/providers/transitives and accepted
owner agreements remain explicit additional work. All owner decisions pending.

Ignored consolidation script actual0; no new engine reads/scans, primary GETs,
native rebuilds, binary/license/package edits, private/remote/trust changes or
commit. No final notices copied; NOTICES111-byte QA baseline unchanged. Existing
114Python/native gates reused. Security HOLD and M7 OPEN are unchanged.
NEXT inspect only existing metadata needed to determine availability of actual
retained-object evidence, then address supported actions; do not infer retention
from declarations or restart provenance/license searches already completed.

RetainedMetadataAvailability1.json SHA256
9c1780e54ec18bc8025fa057924d821b0ea95b5ffb6bde04cb57a27f577da154:
exact existing PDB metadata/hash before/after stable, no debug-stream parsing:
Development383250432bytes/SHAddab13eca7d70c6574341222e875b5ecf2b93ce1a4d70d9c66bffab4322427c5;
Shipping237268992bytes/SHA423e089aa719ed65c40c01dabb42955decf8911959c13a041f0141914244332f.
Both MSF7 signatures observed; existing inventory-bound .rsp hashes unchanged,
/DEBUG:FULL, correct PDB basename, no /MAP or /VERBOSE flags. No .map in the
exact response/bin directories, not global absence. llvm-pdbutil absent only
from PATH/three scoped candidates; no install/tool/game execution. Script0.
PDB-to-EXE GUID/age and contributions NOT checked, no retained-object approval.
NEXT bounded documented read-only PDB/PE identity/contribution analysis using
these exact candidates, not a native rebuild or third-party source rescan.

AttributionActionsClosing1.json SHAb980876c0b2d6f22d07400cc89868770bef6bc3953de53cf6b13f2f484f0ab54:
repository0/0M1warnings, security0/351texts/no findings or policy failures,
diff0/prior EOLwarnings only, receipt/script/hash/AST/action/Markdown assertions0;
284dirty paths (+new worksheet)/empty index/HEAD0726de25... intact/no owned job.
Goal ACTIVE/M7 OPEN; no retained-code/owner/final-attribution/security approval.

### PDB / PE metadata binding and contributions —2026-10-12

PdbContributionReview1.json SHA256
4857a6a85625cbf679a38c2cc222f82770c760126373f26edc2431e0acfdbf83
binds exact existing native EXE hashes to prior QA game identities, then matches
PE RSDS GUID/age, PDB info-stream GUID/age and DBI age. No executable launch,
instruction disassembly, symbol/source-line extraction or original path following.

| Metadata observation | Development | Shipping |
| --- | ---: | ---: |
| GUID | 190d9779-d627-46c2-bba6-dea38fec61c0 | b6415375-daad-9d2e-e4e1-03b3bae73ba1 |
| PDB/RSDS/DBI age | 1 | 1 |
| DBI module records | 3,581 | 3,494 |
| Contribution records within current PE section bounds | 2,756,003 | 1,065,743 |
| Library identities with engine-relative archive-name suffix | 54/61 | 46/54 |
| Those with positive contribution intervals | 54/61 | 45/54 |

PDB section-header bytes equal the current PE headers; both OMAP streams absent,
so direct section-interval interpretation is eligible. Record counts are not
unique bytes/functions, executable coverage or a complete notice inventory.
Positive associations are exact engine-relative NAME suffixes, not the library's
byte hash inside PDB; no basename-only fallback was needed for these results.
Import descriptors/wrappers, ICF/LTO/COMDAT and header/transitive origins remain
separate questions; nonzero intervals do not prove every archive member retained.

Both builds have libssl/libcrypto/libcurl associated modules with positive .text,
.rdata and .data contributions. This strengthens the compiled-network footprint
evidence, not runtime invocation, a specific reachable CVE or vendor-patch proof.
Independent QUIC/OpenSSL/curl security HOLD remains unchanged.

Development no library-name association: tbb12, dinput8, xapobase, OpenEXRUtil-3_4,
speex_resampler, websockets_static and msquic. Shipping additionally has no ITT
association; libogg has a named module but no positive contribution interval.
These observations NEVER authorize removal of dependencies/notices or security
clearance: dynamic DLL loading/imports, compiler transformations and metadata
scope can differ. Previously verified staged DLLs and contracts still matter.
Development Detours/DirectShow/JumpToSelf each has positive named contributions,
not a new license grant. Shipping tbbmalloc has only observed .rdata in this scope.

Reader follows [LLVM MSF](https://llvm.org/docs/PDB/MsfFile.html),
[PDB identity](https://llvm.org/docs/PDB/PdbStream.html),
[DBI format](https://llvm.org/docs/PDB/DbiStream.html) and
[Microsoft PE](https://learn.microsoft.com/en-us/windows/win32/debug/pe-format)
metadata layouts; zero-based contribution/module association cross-checked with
[LLVM's linker writer](https://github.com/llvm/llvm-project/blob/main/lld/COFF/PDB.cpp).
Original reader64MiB bound rejected Development DBI78,642,145 bytes, actualexit1
BEFORE receipt creation. Exact header diagnostic found contribution substream
77,168,088 bytes; corrected128MiB bound and aggregated per-module counters/section
names avoid millions of persisted records. Shipping DBI31,266,794 bytes. Corrected
script actual0, all input PDB/EXE/receipt hashes before/after stable; no raw copies.
An ancillary web lookup NativeEnumModules.cpp failed cache fetch; no install or
private lookup, other primary format/writer references suffice, not global absence.

PdbReaderAssertions1.json SHAeb4f685ce4a309d5b0d9bec003212129463d21563f891a972421061191935f88:
actual0, valid discontiguous MSF/module/PE-RSDS synthetic checks, four malformed
MSF/two module/fifteen PE-read bounds cases rejected. This is targeted parser
evidence, not a claim of full generic PDB correctness. Existing114Python/native
product gates reused unchanged; no real binary mutated or executed.
No private module/symbol/source paths persisted, license/engine/QA/remote/trust
changes or owner approval. M7/final notices remain OPEN.
NEXT inspect only PE import/delay-import metadata and existing companion records
needed for remaining associations, then address ATR scope with actual evidence;
do not infer absence, restart source queries or rebuild/run the unsigned player.

PdbContributionClosing1.json SHA7661fcda60ce676ccfab72e974fb586bee69a4316e9795bcf0248478e5cf43a4:
repository0/0M1warnings, security0/351texts/no findings/policy failures, diff0,
JSON/script/hash/AST/count/eligibility assertions0; exact reader AST identity
guard valid match accepted/wrong version+GUID+age rejected, command0.284dirty
paths/indexempty/HEAD0726de25... intact/no owned job. Goal ACTIVE/M7 OPEN.

### Explicit PE normal/delay import metadata —2026-10-12

PeImportReview1.json SHA256
2c32d6f2b3194fa21eed7d6464accbc802c95ddc468d4d50edb7a807e40549ed
reads EVERY native image identified by the existing QA manifests:23Development/
15Shipping =38images (main+bootstrap EXEs and21/13DLLs). All image/manifest/input
hashes verified before/after, no PDB reparse, instructions/disassembly/execute.
Descriptors and bounded named/ordinal lookup counts use the
[Microsoft PE import/delay layouts](https://learn.microsoft.com/en-us/windows/win32/debug/pe-format).
No imported function names, raw source/binary or private paths persisted.

| Metadata scope | Development | Shipping |
| --- | ---: | ---: |
| Main Game normal DLL descriptors | 47 | 41 |
| Main Game delay DLL descriptors | 15 | 13 |
| Descriptor edges to same-basename packaged DLL candidates, all QA images | 13 | 10 |
| Packaged DLLs without any explicit descriptor from these QA images | 13 | 7 |

Name matches are QA-set candidates, not the Windows loader's chosen path/version,
forwarded exports, ABI compatibility, executed calls or complete dependency graph.
Names outside that set are NOT automatically missing required files or licensed
by Windows; system/API-set/driver/other resolution and rights are separate scope.
Delay descriptors indicate a possible deferred load, not proof it occurred.

- Main Development normally imports tbbmalloc and delay-imports PIX/Ogg/Vorbis/
  VorbisFile/Aftermath/DirectML/DbgHelp. Main Shipping delay-imports Vorbis/
  VorbisFile/Aftermath/DirectML/DbgHelp; no normal same-basename QA DLL candidate.
- In BOTH configurations, Vorbis and VorbisFile normally import libogg_64;
  VorbisFile also normally imports libvorbis_64. Thus Shipping's absent direct
  Game Ogg descriptor and zero positive Ogg PDB interval cannot justify dropping
  Ogg notices/dependency. This is explicit transitive name metadata, not execution.
- No explicit descriptor to tbb12 or MsQuic in any of these38images; neither is
  omitted from the QA inventories. Dynamic loading remains unverified, security
  HOLD and QUIC preservation unchanged. No DLL/plugin disabled or replaced.
- Shipping's seven no-explicit-reference candidates: MsQuic, XAudio redist, ONNX
  Runtime, D3D12Core, SDKLayers, tbb12 and tbbmalloc. Development additionally has
  the seven Vulkan layer DLLs and normally references tbbmalloc, leaving13.
  Startup/services/providers/OS-loading mechanisms are not tested by this graph.

Initial synthetic negative fixture expected the wrong first rejection for a
nonzero terminator row, actualexit1 BEFORE native inputs/receipt. Correcting only
that expected code made six malformed cases pass; normal/delay named+ordinal
synthetic fixtures pass. Corrected research script actual0~2.88s, all38images
parsed under strict bounds; no bound-IAT lookup guessing, unsupported attributes
or terminator failures bypassed. Existing114Python/native gates reused unchanged.
No raw binary/license/engine/QA/remote/trust mutation, install or player execution.
Metadata advances ATR scope, NOT final notices/rights/runtime/security clearance.

NEXT use the completed identity/PDB/import evidence to close only supported ATR
items. Exact vendor patch/contracts, owner choices, clean candidate/isolated CI,
trusted-player/performance and final bundle remain independent prerequisites;
do not invent further source scans, remove notices from negative observations,
rebuild the same QA or re-ask for the same already-supplied engine installation.

PeImportClosing1.json SHA07ece33093e4792c4b57ed7a15c635fa550ba4a8d6d6f0be1c06e2801469c084:
repository0/0M1warnings, security0/351texts/no findings or policy failures,
diff0; JSON/script/hash/AST/image-count/graph/Ogg assertions0.284dirty paths/
empty index/HEAD0726de25... unchanged/no owned job. Goal ACTIVE/M7 OPEN.
