# Third-Party Notices

Last art-source generation: 2026-10-22. Packaged dependency inventory: 2026-10-12 (C018H); release rights remain an owner gate.

The last committed Git candidate contains no vendored third-party art source. The current uncommitted semi-realistic art work adds the explicitly CC0 anatomy data described below, not MakeHuman application code. This is NOT a claim that packaged Unreal executables contain no third-party software. Ignored local QA archives contain engine-transitive binaries and declared static-library inputs, inventoried below.

Unreal Engine and its engine-provided plugins/content are external development prerequisites and are governed by Epic Games' terms; they are not relicensed by this project. Enhanced Input is enabled; Procedural Mesh Component is retired/explicitly disabled. The project does not vendor their source or binaries. ModelContextProtocol/EditorToolset/AutomationTestToolset are Editor-target-only integrations, not Shipping plugins.

`Content/Maps/L_Testbed.umap` and `Content/Maps/L_VerticalSlice.umap` are project-saved derivatives of UE 5.8's engine-provided `Template_Default` map and reference engine-provided basic shapes/materials. Their use and distribution remain subject to Epic Games' applicable Unreal Engine/content terms in addition to the project's own modifications.

Before adding any third-party material, record its source, exact version, license, redistribution rights, modifications, required attribution, and affected repository paths here and in `ASSET_INVENTORY.md`.

C017D–I original material/audio/character recipes and assets introduce no third-party art source. Existing maps still reference engine/template content. Do not treat project authorship as approval to distribute Epic content; verify the actual candidate and applicable account/distribution rights before release.

C018A GitHub Actions are external CI prerequisites, not vendored/shipped game content: checkout v4.2.2 (`11bd71901bbe5b1630ceea73d27597364c9af683`) and upload-artifact v4.6.2 (`ea165f8d65b6e75b540449e92b4886f43607fa02`). Exact reviewed upstream links and runner/security requirements are in `CI_RUNNER_SETUP.md`; re-review pins before updates. Locally available PyYAML6.0.3 was used only to parse the workflow during development, not installed or added as a production/CI test dependency.

## Semi-realistic anatomy source — 2026-10-19

`SourceArt/ThirdParty/MakeHuman/a8bc2d54/base.obj` is pinned to
`makehumancommunity/makehuman` commit `a8bc2d54ff0ac92e78ff71431b1023eda42bf482`.
Its own header explicitly releases this asset as CC0 (September 2020) and names
Data Collection AB, Joel Palmius and Jonas Hauquier as copyright holders.
Original `LICENSE.md` and `LICENSE.ASSETS.md` are retained beside it in full.
The application is AGPL; no MakeHuman application code is copied or executed.
This asset-specific statement does not license arbitrary community assets.

The same pinned revision additionally supplies three numeric morph-data assets
under `macrodetails/`: `caucasian-male-young.target`, `caucasian-male-old.target`,
and `universal-male-young-maxmuscle-averageweight.target`. Each file has its own
explicit September-2020 CC0 statement, original copyright2014 Manuel Bastioni
and the stated2016 transfer /2020 holders above. The retained root license also
explicitly includes targets/modifiers as bundled CC0 assets. These are numeric
vertex deltas, not downloaded/executed program logic. Filenames retain upstream
terminology; the draft blend is an artistic proposal, not an anthropological or
historically validated model of a Roman population. Exact hashes are inventoried.

The new candidate filters body/helper groups, normalizes scale and derives joints,
then adds original clothing, equipment, materials, skinning and animation through
`Tools/create_semireal_legionary.py`. Exact hashes and upstream URLs are recorded
in `ASSET_INVENTORY.md`. Existing shipped Content has not been replaced. Final
visual approval and any overall distribution review remain separate gates.

Blender 5.2.2 LTS is an external, workspace-local portable development tool under
ignored `Artifacts/Toolchains`, not a vendored or shipped runtime. Official ZIP
checksum and Blender Foundation signature are recorded in its bootstrap receipt.

The camp source architecture/material revision under
`SourceArt/Environment/RomanCamp/semireal_v2` is original GPT Blender geometry
and procedural shader/bake work. It reuses the v1 GPT-generated soil input
byte-for-byte; no new image generation, third-party environment pack, purchase,
external code or tool installation. The existing MakeHuman-derived legionary
appears only as a BLEND/render scale reference, excluded from all four campGLBs;
its exact provenance/retained notices above remain applicable. Generated-content
and tool terms/owner rights review are not replaced by a blanketCC0 claim.
[Revision review](M6_SEMIREAL_CAMP_REVISION_REVIEW.md) records source identities,
verification and boundaries. No mainContent/QAarchive/package notice changed.

## Actual dirty local QA package dependencies — review OPEN

[Packaged dependency review](M7_PACKAGED_DEPENDENCY_REVIEW.md) records the verified
Development/Shipping archives, all21/13 DLL hashes and version resources,
184/179 plugin descriptors,497/455 target modules and94/87 unique declared link
inputs. zlib, ICU, BLAKE3, Oodle, libpas, VTune, OpenSSL, WebM and other declared
inputs appear in the machine-readable inventory; declarations are not proof that
all their code survives linking or executes. Full source-license text is not
reproduced here, and this file is not yet the release notice bundle.

| Packaged native family | Installed license evidence / unresolved review |
| --- | --- |
| DbgHelp | Source/BOTH QA hashes exact; version10.0.25291 differs TPS19041; OS-vs-debugger redistribution distinction reviewed, exact package/list/contract/rights OPEN |
| MsQuic2.2.0 OpenSSL | Primary payload matches outside narrow PE fields; MIT body maps to installed candidate plus Apache2.0 OpenSSL notices. Published-advisory RELEASE HOLD; mitigation/owner notices pending |
| Ogg/Vorbis/VorbisFile | Source/BOTH QA hashes and native staging verified; pinned Ogg1.2.2/Vorbis1.3.2 BSD bodies map installed candidates excluding distinct copyright lines; aoTuV/source-build/retained notices and final rights OPEN |
| oneTBB2022.3.0 | Full candidate Apache body maps installed wrapper; exact staged DLLs differ from official production package even outside narrow PE fields; Epic build provenance/retained notices OPEN |
| NVIDIA Aftermath2025.5 | Exact source/QA hashes; version-named266byte document is wrapper only. Current SDK agreement/dll-so supplement differs from legacy NVAPI candidate; exact payload/contract/retained notices/owner rights OPEN |
| XAudio2 redist | Retail x64 payload matches pinned Microsoft.XAudio2.Redist1.1.0 outside narrow PE signing/checksum fields; same package's license/README hashed and reviewed; exact distributable-code scope and owner compliance pending |
| ONNX Runtime | Reported suffix/pinned C API header corroborate candidate commit after LF normalization; misnamed core document is ONNX Apache, not runtime MIT. Exact retained providers/components/final notices/rights pending |
| DirectML1.15.4 | Pinned official Windows x64 payload matches outside narrow PE fields; package README maps binary license and GSL/half/Zstandard notices; installed license quote differences, owner compliance/final bundle pending |
| D3D12 Core / SDK Layers1.618.5 | Installed staging/source hashes match; pinned official package lists both as distributables, non-certificate bytes match, complete binary-license body maps after whitespace normalization; owner compliance/final notices pending |
| Development-only Vulkan layers | Seven paired DLL/JSON hashes exact; manifests/header API1.4.341, older validation license1.3.250.1 and generic registry are not complete applicable terms; exact SDK/components/notices OPEN |
| Development-only WinPixEventRuntime | Official1.0.240308001 production x64 matches outside narrow PE fields, same-package MIT declared; full retained18-program notice applicability/final rights OPEN |

Installed sources are external under UE5.8's Engine/Source/ThirdParty/Licenses and
the TPS paths recorded in the receipt. Ten explicitly named candidate documents
have SHA256 receipts, not automatic applicability/rights approval. No whole
license directory was copied. TPS variants include unrelated platforms/versions;
non-XML metadata or an unresolved reference is not a license-violation finding.
Supplementary SourceMappings2.json records14 exact installed-source hash/staging
bindings and the additionally found exact-version AgilitySDK license candidate.
Module-export omissions are not missing source provenance; distribution review
remains OPEN and the original inventory receipts are unchanged.

AgilityUpstream3.json now binds the official1.618.5 package, exact distributable
file list and binary license body to the installed candidate and both DLL payloads.
Full DLL hashes differ: only PE checksum/security-directory/terminal-certificate
bytes are excluded in the separately recorded equality comparison. No signature
stripping/verification or raw-byte identity claim. Header MIT terms remain separate
from binary terms; Windows scope, distributor/end-user obligations and all other
applicable requirements still need owner compliance review. No package notices,
EULA, DLL, engine rule or distribution setting changed. See the dependency review
for hashes/tests/partial and failed receipts; reuse final3 rather than rerun GETs.

XAudioSourceReview1.json additionally binds the installed/QA XAudio2 DLL payload
to the primary1.1.0 retail package and its exact LICENSE/README hashes. Full DLL
bytes differ; only the recorded PE checksum/security-directory/EOF certificate
fields are excluded in the equality proof. PE version1.0.0.1 is not NuGet identity,
and no unique package version or runtime loading is claimed. The examined license
section2(a) refers to an Image Library rather than an explicit DLL file-list;
owner applicable-agreement/scope/obligations review remains OPEN. Family-level
developer documentation does not resolve that ambiguity by itself. No DLL update,
engine metadata repair, license rewrite, EULA acceptance or final notice copy.

DirectMLUpstream1.json binds the installed/QA Windows x64 DLL to selected ranges
of the primary1.15.4 package, not a whole-package hash or signature verification.
README explicitly maps LICENSE.txt to bin/, distinct from MIT include/build
terms. ThirdPartyNotices identifies GSL/half/Zstandard and records exact notice
hashes; no-dependency NuGet metadata does not mean no incorporated components.
The installed license has doubled/trailing quotes: strict whitespace-only equality
fails, preserved in DirectMLLicenseComparison1.json without repairs/acceptance.
Platform/applicable terms/owner compliance and reviewed final notices stay OPEN;
no engine DLLs, metadata or package NOTICES.txt changed.

MsQuicSourceReview1.json resolves the2.2.0+359654 OpenSSL payload and MIT/Apache2.0
documentary scope despite older malformed TPS. Six current publisher advisories
affect this candidate, including Critical RCE; release security remains HOLD.
Exact runtime exposure is unverified. Owner chose retain QUIC/evaluate compatible
update, NOT disable/install/replace. MsQuicUpdateEvaluation1 supersedes initial
four-advisory/pending-choice snapshot; no plugin/DLL changed. [Dependency review](M7_PACKAGED_DEPENDENCY_REVIEW.md) records primary
links/hashes/usage scope. Passing repository secret checks does not clear this.

OnnxAttributionReview1.json separates primary runtime MIT from ONNX Apache
despite the installed Onnxruntime_v1.24.1.license name. Exact source/QA hashes
rechecked; reported commit suffix and one LF-normalized C API header corroborate
a candidate, NOT authenticated DLL source or stable1.24.1 release identity.
Pinned notice source325,054bytes hashed, only ONNX section inspected; not all
retained components/provider terms reviewed. Installed doubled/trailing quotes
preserved without repair/legal equivalence. Final applicable MIT/Apache/component
notices and owner rights OPEN; no engine/package/source change.

oneTBB follow-up: OneTbbUpstream1/OneTbbAttributionReview1 record exact source/
BOTH QA hashes and official Windows package checksum. Production x64 DLLs do
NOT match upstream raw or narrow PE-normalized bytes; no forced equivalence or
corruption finding. Same-package Apache license body matches installed wrapper
after whitespace only. Complete package-wide third-party-programs.txt has six
sections, not six proven DLL dependencies; preserve applicable notices after
Epic build/source/flags review. GCC exception is conditional, no LLVM exception
in selected legal documents. No final notice bundle/rights approval or engine/
archive changes. [Detailed proof and primary sources](M7_PACKAGED_DEPENDENCY_REVIEW.md).

NativeFamilyCoverage1.json indexes all21Development/13Shipping DLL rows across
11reviewed families to exact receipt hashes/evidence grades. This is dynamic
DLL engineering coverage only, NOT complete distribution attribution:87/94
declared static link inputs, retained components/providers/fonts, exact vendor
agreements/builds and final bundled notices remain OPEN. WinPix MIT is bound to
the compared package, not proof all broader PIX informational components retained;
Vulkan older/general notices must not blanket-replace per-component terms.
[Tail review and primary sources](M7_PACKAGED_DEPENDENCY_REVIEW.md).

Declared link review now indexed by Tools/link_attribution_index.py from existing
receipts:61/54file-backed .lib inputs plus33platform import names in Development/
Shipping.45Shipping rows have module/TPS candidates,3no selected TPS,4outside
exported directories,2ambiguous; none approved or proven retained. Static vs
import .lib formats are not inferred. Missing WinPix TPS in module-source scope
does not invalidate already reviewed binary-folder TPS/MIT evidence; Vorbis
shared-directory ambiguity remains only a heuristic. Exact libpas/RadAudio/Bink/
OodleNetwork and OodleData/UEJpegComp/Draco candidate gaps are resolved at the
declaration level by the follow-up below, not at retained-code/rights level.
LinkIndexReview1 hashes both generated indices; no license copies/native changes.

StaticGapSourceReview1 binds exact unique native declaring modules for those
seven unchanged libraries in both exports. libpas22UE-header BSD-style notices
have distinct Epic2023/2023-2024/2025 copyrights; original modified WebKit fork
notices remain unverified. RadAudio/Bink/Oodle/UEJpeg copyright/version headers
are not accepted contracts. Draco header1.5.6/Apache2.0 is a source candidate.
UEJpeg bundled ZIP contains stb_image implementation declaration plus complete
MIT2017SeanBarrett/Unlicense alternatives; owner choice and binary retention
OPEN. Bundled Oodle9/13 differs from native SDK9/16, not a packaged-version claim.
DevProfilerSourceReview1 binds three Development-only profiler declarations;
Null names/no matching local notice filenames do not establish exemption.
No final license copies/package changes. Applicable original/transitive notices,
retained components and Epic/SDK distribution rights still require review.

## Delivered engine fonts — exact provenance inventoried, rights OPEN

TextWebSocketSourceReview1/Diagnostics1 resolve local candidate ICU64/HarfBuzz2.4
depot-style TPS references by explicit same-basename license files, not UNC
resolution/rights approval. Raw CP1252 candidate notices preserved: ICU full
notice whitespace matches primary; HarfBuzz copyright years/extra Facebook differ,
permission body matches only after whitespace. Separate UCDN/per-file notices
need applicability. libWebSockets TPS1.7.3 versus header3.0.0 preserved;205byte
UE4 credit/source locator does not prove modified-source fulfillment. Pinned
LGPL2.1/static-link exceptions do not automatically require game-source release;
modified library obligations and exact rights remain OPEN. No notices copied to
package. [Exact source review](M7_PACKAGED_DEPENDENCY_REVIEW.md).

NetworkSourceReview1 binds OpenSSL/libcurl declarations/library banners and
selected source notices, NOT retained binary identity or distribution approval.
OpenSSL1.1.1t license matches primary after ONLY CRLF-to-LF and requires BOTH
legacy OpenSSL/SSLeay conditions, not modern Apache or license choice. curl's
complete old notice differs from pinned COPYING in copyright year/email;
permission/disclaimer body matches only after whitespace. Generated release
header macros differ from upstream DEV template, not an Epic-modification finding.
Publisher support/advisory review adds security HOLD independent of notices;
no license/archive/library changes or final bundle. [Review](M7_PACKAGED_DEPENDENCY_REVIEW.md).

CompressionFontCoreReview1 adds exact selected zlib1.3/libPNG1.6.44/FreeType2.14.1
header matches to primary version-tag sources after ONLY CRLF-to-LF, not binary
build/retention proof. libPNG source LICENSE matches but central version-named
notice differs in two copyright years (2025vs2024), preserved/not repaired.
FreeType FTL body matches its pinned source; root LICENSE additionally contains
FTL/GPLv2-or-later choice and package-wide alternative-license component notices.
Owner choice/applicability/retained exceptions remain OPEN. Existing111byte
FreeType-only package credit is unchanged, not a full bundle. Library source
licensing is distinct from the licensing of the font assets below.
[Exact source-document evidence](M7_PACKAGED_DEPENDENCY_REVIEW.md).

[Font attribution review](M7_FONT_ATTRIBUTION_REVIEW.md) records15 raw-font entries
per native Pak index (14distinct TTF hashes; duplicate Old/Tiles Bold) for each
Development/Shipping Pak, freshly extracted from unchanged archives and matching
installed engine source hashes; both font sets are byte-identical. Eight Roboto
faces are static2011 versions without name IDs13/14, while Light/Medium are2017
with Apache2.0 metadata claims. Droid Fallback, Droid Mono and two Noto UI faces
have distinct embedded terms/versions. Six entries contain explicit license name
records, nine do not; neither result alone proves applicable rights. DroidMono's Ascender/five-PC language needs
specific agreement/provenance resolution, not an automatic violation finding.
No broad family-name licensing, modern Roboto substitution, font removal or
metadata rewriting. These engine-transitive fonts are NOT new project-authored
art and are not covered solely by the packaged FreeType copyright notice.
Full final attribution/rights approval remains open; no production changes.
Subsequent [exact DroidMono source mapping](M7_FONT_ATTRIBUTION_REVIEW.md)
matches all bytes to AOSP commit54b6cfa9 and its same-directory Apache-2.0
README/NOTICE/marker. That resolves upstream source/declaration provenance, not
owner compliance or final bundle approval, and does not rewrite embedded
Ascender metadata. Older Roboto remains a separate exact-version review.

Targeted installed font-directory TPS review additionally found exact declared
ROBOTO_License.txt (Apache2/Google2012 header, legacy encoding flagged) and
Noto_License.txt (OFL1.1), with immutable source/document hashes in
FontTpsSources1.json. These three Content-directory TPS files were outside the
earlier module inventory scope. DroidSans.tps redirects to an absent installed
AndroidOpenSourceProject.tps; independent DroidMono upstream proof remains valid.
Directory declarations do not prove per-face version applicability:2016 TPS dates,
Roboto2011/2017 fonts and its2012 notice header are retained separately. Sixteen
exact old-Roboto blob lookups returned404 in two selected primary repositories,
not a global source-absence or licensing finding. No license text was copied,
re-encoded or substituted; final notices and owner compliance remain OPEN.

The [per-identity notice map](M7_FONT_NOTICE_MAP.md) (2026-10-21) now associates
all14 raw hashes/15 historical QA paths per configuration with verbatim
copyright/nameID0/13/14, versions, candidate-document identities and specific
review gaps. FontNoticeMap02.json pins the existing receipts and rechecks45
installed/extracted raw paths;12 directed mapping tests pass. Absent metadata
is not a license determination, Mono proof does not extend to Fallback, and the
Old/Tiles Bold alias does not extend a Fonts-directory declaration. ATR-13 and
the final owner-reviewed bundle remain PENDING. No QA NOTICES/Pak/font/license
bytes changed; this technical map is not the release notice bundle.

HashTangentTiffSourceReview1/Diagnostics1 adds three source-notice candidates with
unchanged library hashes/unique declaring modules in both exports. BLAKE3 1.3.1:
CC0-1.0 OR Apache-2.0 alternatives; whole local notice matches pinned primary only
whitespace-wise. MikkTSpace1.0 copyright2011 Morten S. Mikkelsen: source-notice,
origin and altered-source restrictions; acknowledgement appreciated, not required.
Local header adds m_bIgnoreDegenerates and comment, so whole upstream-header match
is false although copyright comment matches CRLF-only. LibTiff4.2.0 central
notice/header/version match pinned primary CRLF-only; preserve copyrights/permission
and advertising-name restrictions, not generic MIT replacement. Candidate TPS
basenames/directories are not final license/retention resolution; separate
ComputingTangentSpace metadata and TIFF embedded-zlib/turbojpeg scope remain OPEN.
[Source evidence and primary documents](M7_PACKAGED_DEPENDENCY_REVIEW.md).
No notices copied to the package, library modification, binary/source-revision
approval, owner branch choice, final bundle or security mitigation claimed.

AudioVideoSourceReview1/Supplement1 adds five unchanged declared libraries:
Opus/sse41 plus separately credited Speex resampler, LibVpx and LibWebM.
Opus path1.3.1-12 is not an exact binary version (banner says libopus unknown).
Its root BSD3 candidate and API-header BSD2-style notice are distinct; older TPS
and newer1.4/1.5 notices are not substitutes. Speex copyright2007 Jean-Marc Valin
notice matches a primary candidate, whole header differs/version unproven.
VPX1.14.1/header candidates and WebM1.0.0.27 local license/header map to pinned
primary documents; missing central TPS filenames are narrow path observations.
WebM PATENTS.TXT exists despite PATENTS URL404; grant matches pinned VPX document,
with modification scope and patent-enforcement termination, not whole-game rights.
Opus IETF patent references/applicability, retained objects, all per-file notices
and final bundle remain OPEN. [Evidence and sources](M7_PACKAGED_DEPENDENCY_REVIEW.md).
No licenses repaired/copied, newer version substitution or rights approval.

ImageHttpSourceReview1/Supplement1 adds ten declaration candidates in five modules,
including separately credited OpenJPH. nghttp2 MIT notice/body maps to pinned1.64.0
with scoped metadata exclusion; Imath3.2.2/OpenEXR3.4.3 BSD3 bodies map CRLF-only.
OpenJPH0.24.5 version header/BSD2 is distinct, not covered by OpenEXR root notice.
JPEG Turbo3.0.0 header maps but central notice is older2009–2020 vs primary2023,
strictCP1252 diagnostic candidate only/raw unchanged. IJG/BSD3/zlib component scope
is not an OR-choice; IJG product credit and TurboJPEG binary notice applicability
need resolution. Central legacy IJG6b/1998 isn't the pinned README. APVv0.2.0.1
first header notice matches, WHOLE header/wrapped license does not; malformed
quotes preserved, no source/build/patent approval. [Evidence and sources](M7_PACKAGED_DEPENDENCY_REVIEW.md).
All retained-code, exact-build, copyright and final bundle obligations remain
owner-reviewed; no library/license/package mutation or security mitigation.

GpuDeclarationReview1/Supplement1 records twelve unchanged declaration libraries,
not twelve proven retained components. AGS6.0.1 and NVAPI R570 library hashes
match pinned publisher files RAW; selected headers match CRLF-only. Publisher's
NVAPI notice explicitly covers nvapi.lib/nvapi64.lib under MIT; local notice
body matches after scoped metadata/applicability-introduction exclusion, with
the publisher applicability statement retained as evidence. AGS MIT candidate
identified despite exact TPS local filename absent. Intel extension header MIT
identifier is not a full notice/build mapping. ITT BSD3 OR GPL2 choice and
local/primary mismatches remain open, not a whole-game GPL requirement. DirectX
directory association isn't actual DX11/DX12/audio/XInput declaration ownership;
legacy SDK/header metadata is not blanket rights or an exemption. Existing
Aftermath/Agility/XAudio/WinPix contracts/scopes are not replaced by these matches.
[Evidence, native ownership map and sources](M7_PACKAGED_DEPENDENCY_REVIEW.md).
No raw SDK persisted, copied final notice, licence repair or release approval.

DeclarationReviewCoverage1/CompanionImportBinding1/DevTailSourceReview1 now reference
all61 Development/54 Shipping concrete library identities;33 platform-name inputs
per configuration remain separately excluded. This means identity evidence exists,
NOT that all libraries are retained, their sources/rights settled or notices ready.
Nine companion archives have short import records naming previously reviewed DLL
families; actual native owners verified, other members/ABI/retention unproven.
No repeated DLL source searches or blanket reuse of their licenses for all .lib code.

Development-only Detours4.0.1 header/version candidates differ from publisher source;
two local MIT permission bodies match whitespace-only, full notices do not. Preserve
copyright/introduction differences and the other notice's5.3 label, not a binary
version assertion. DirectShow's installed Windows7/.NET4 SDK notice is an explicit
CP1252 diagnostic candidate, raw unchanged; exact REDIST/marked-sample rights,
linked-result distribution and any Windows Media portion credit remain conditional.
Modern public sample header/terms do not replace that agreement. LC_JumpToSelf
assembly identifies Molecular Matters2011–2020/all rights reserved; neighboring
xxHash TPS does NOT assign BSD terms to that assembly/library. Owner must establish
exact Live++/Molecular Matters/Epic distribution scope before including any required
notice. [Evidence, limitations and primary sources](M7_PACKAGED_DEPENDENCY_REVIEW.md).

The actual Shipping NOTICES.txt remains111 bytes/FreeType-only.
Use [the pending owner-action worksheet](RELEASE_ATTRIBUTION_ACTIONS.md) to record
candidate-specific provenance, retained objects, contracts, notice scope and review.
The worksheet and its hash-bound coverage receipt do not grant rights or replace
notice text. No action is owner-approved, and the current QA bundle is unchanged.

PdbContributionReview1 adds actual GUID/age-linked PDB/PE interval metadata for
the unchanged QA EXEs.54Dev/45Ship libraries have positive name-associated
contributions, not a complete source/per-file/notice inventory. Import wrappers,
compiler folding/LTO, dynamic loading and transitive origins remain separately
scoped; a missing association never justifies removing a notice/dependency.
[Metadata results and limitations](M7_PACKAGED_DEPENDENCY_REVIEW.md).

PeImportReview1 now includes all38exact QA native images' normal/delay descriptors.
Ogg is referenced by Vorbis/VorbisFile in both archives, even without a direct
Shipping Game descriptor. Do not drop its notice based on main-EXE/PDB absence.
MsQuic/tbb12 have no explicit descriptors here but may be dynamically loaded;
this is not exclusion, security mitigation or attribution/rights approval.

Before distribution,
resolve applicable exact versions, all retained/transitive and shipped content
attribution, SDK/Epic/account redistribution rights and a reviewed final notice
bundle for the clean candidate. Inventory exit0/owner-authored assets do not close
this gate. Do not modify old QA archives or publish their ignored metadata by default.
