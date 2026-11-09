# C018H packaged font attribution — LOCAL INVENTORY VERIFIED; rights OPEN

2026-10-12 UTC. The previous module/TPS inventory deliberately excluded engine
Content. Final native Pak indexes contain15 raw-font entries:14 under
Engine/Content/Slate/Fonts and one under Engine/Content/Slate/Old/Tiles.
These are third-party engine-transitive assets, not
new project-authored/imported fonts. This review extends M7.5 coverage without
changing the correct UI, engine defaults, assets, font metadata or packages.

## Evidence and guarded extraction

Initial filtered extraction from BOTH existing QA Paks terminated0,14 fonts each
(PARTIAL/HISTORICAL, preserved):

- Shipping: Artifacts/C018H-FontShippingExtract2.log/.exit.json; native0.027235s.
- Development: Artifacts/C018H-FontDevelopmentExtract1.log/.exit.json; native0.016605s.
- Fresh roots: Artifacts/C018H-FontAttribution1/{Shipping,Development}.

Original Pak SHA256 matched its immutable archive manifest before/after extraction.
Fresh extraction only selected `*Content/Slate/Fonts/*.ttf`; no other content,
FontFace import, renderer/font interpreter, build, cook or player execution.
Those14 files matched sources but did not exhaust the native font index. The
exact full-index check found the additional Old/Tiles/Roboto-Bold.ttf, prompting
a fresh expanded cohort (not automatic scope closure from a subset):

- Final Artifacts/C018H-Font{Shipping,Development}AllExtract1.log/.exit.json:
  native exit0,15 files each, filter `*Content/Slate/*.ttf`, Pak hashes unchanged.
- Final roots: Artifacts/C018H-FontAttribution2/{Shipping,Development}.
- PakFontScopeFinal1.json: all15 raw-font index paths exactly equal each final
  inventory's paths, not just counts; no other TTF/OTF/TTC/WOFF entries in the
  recorded native Pak indexes. Unnamed IO chunks/FontFace assets are not covered.
- All15 freshly extracted files match installed source SHA256; all15 paths/bytes
  are identical across configs.14distinct hashes: Old/Tiles/Roboto-Bold.ttf is
  byte-identical to Fonts/Roboto-Bold.ttf, not a newly authored typeface.

Original native Pak payload verify
and archive evidence remain in M7_LOCAL_QA_PACKAGE_REVIEW.md.

The initial Shipping invocation used `-Extract=<path>`, which installed5.8.2 did
not recognize as extraction. It entered creation mode but refused the existing
Pak, exit1/no destination/no package mutation. Hash rechecked unchanged, failure
log/receipt1 retained. Installed PakFileUtilities.cpp6008–6051 was inspected:
use **`-Extract <PakFile> <OutputPath>`**, never the '=' form. The corrected call
and actual receipt2 above prove success. Pak index SHA1 for compressed entries
is not raw font SHA1; no false equality was used (observed Roboto-Regular).

## Tool and integrity receipts

`Tools/packaged_font_inventory.py` uses the existing dependency inventory's
contained-path/reparse/hash helpers and Python's standard library. It reads only
bounded SFNT name tables, validating headers/table offsets/record storage,
language-tag bounds, UTF16/MacRoman encodings and allowed extracted paths.
No glyph execution, URL fetch, font replacement or assumed license conversion.
Names, versions, copyright and explicit name IDs13/14 are recorded as untrusted
data. Missing IDs13/14 mean only those fields are absent, NOT that no contract
exists elsewhere or that copyright/license data cannot occur in other fields.

- Final Fonts-Shipping1.json in FontAttribution2 SHA256:
  `c2e9338472626e10ccfa4bc12c879dd3349f5c3b750baad28ef70dc32133dd71`.
- Final Fonts-Development1.json in FontAttribution2 SHA256:
  `943d18007990e010a5ecbb7b9b18665485d1e9290d851c6c459972f13f4b7a84`.
- Each receipt binds the extraction exit receipt, archive manifest, actual Pak
  SHA256, current parser hash and each exact font/source SHA256.
-15font entries/config,14distinct hashes,6with explicit license name records,
  9without them (eight faces plus the duplicate Bold),0encoding warnings;
  `rightsReview=pending_owner` for every font and the full receipt.
- Initial FontAttribution1 receipts retained as partial/historical evidence.
  No automatic publication/approval, overwrite or broad engine Content scan.

## Actual metadata, not a rights decision

| Delivered faces | Embedded versions | Explicit name IDs13/14 observations |
| --- | --- | --- |
|8Roboto faces: Black, BlackItalic, Bold, BoldCondensed, BoldCondensedItalic, BoldItalic, Italic, Regular |1.00000;2011 | Absent; exact source/version/attribution evidence pending |
| Roboto Light / Medium |2.137;2017 | Apache2.0 URL/description claims, not blanket approval for all Roboto |
| Droid Sans Fallback |2.55b | Apache2.0 URL/description claims |
| Droid Sans Mono |1.00 /1.00build107 | Ascender personal/business five-PC language and old EULA URL; resolve applicable agreement/provenance |
| Noto Naskh Arabic UI |1.06uh | SIL OFL1.1 description/URL claims |
| Noto Sans Thai |2.000;GOOG;noto-source:20170915:90ef993387c0 | SIL OFL description/URL claims |

The ninth older-Roboto entry at Old/Tiles/Roboto-Bold.ttf is the same2011 Bold
bytes, SHA25624571503140760240924dcd1238f77e7cd0454c0d8b0793990cefa2fad71471f.

DroidSansMono's embedded language is an actual review flag, **not** a finding
that Epic's bundled font is unlawful: embedded historical text may differ from
the applicable external agreement. Do not silently ignore it, reinterpret it as
Apache/OFL solely from the family name, remove/replace the font, or claim rights
are resolved. Local Git-blob SHA1 `4546611d4d55347c95827d96d41a771d7ba283ca`
for its78,296bytes was initially only a source-comparison hint. The subsequent
exact upstream verification below supersedes that earlier limitation.

### DroidMono exact upstream source and directory declaration — verified

2026-10-12. Public unauthenticated GitHub GETs (no installs, credentials, persisted
font/NOTICE downloads or font execution) found the matching original AOSP blob.
Pinned commit `54b6cfa9a9e5b861a9930af873580d6dc20f773c` contains
`data/fonts/DroidSansMono.ttf`, Git blob `4546611d4d55347c95827d96d41a771d7ba283ca`.
Decoded upstream bytes and the actual packaged/source file are exactly78,296bytes,
SHA256 `793a63429da92cb9be35ca3fca080a66b3c74915fc24b778dcc35d78f465e403`.
This is byte-level provenance, not a family-name or embedded-version guess.

That SAME pinned font directory contains MODULE_LICENSE_APACHE2 plus README.txt
explicitly declaring its fonts under Apache2, and NOTICE with the corresponding
license/copyright text. References: [pinned font source](https://github.com/aosp-mirror/platform_frameworks_base/blob/54b6cfa9a9e5b861a9930af873580d6dc20f773c/data/fonts/DroidSansMono.ttf),
[directory README](https://raw.githubusercontent.com/aosp-mirror/platform_frameworks_base/54b6cfa9a9e5b861a9930af873580d6dc20f773c/data/fonts/README.txt),
[directory NOTICE](https://raw.githubusercontent.com/aosp-mirror/platform_frameworks_base/54b6cfa9a9e5b861a9930af873580d6dc20f773c/data/fonts/NOTICE).
The exact source/upstream Apache-2.0 declaration mapping is now resolved. This
does not assert owner compliance, erase Ascender's embedded historical text or
approve distribution; final applicable-agreement/notice review remains open.

Receipt: Artifacts/C018H-FontAttribution2/DroidMonoUpstream1.json SHA256
`daa1b70a46ff15b603cd8984ede5a6d226575feda3b650badacee9e71a71aa56`.
It binds exact source/hash/commit, marker and both document blob/SHA256 values,
not raw binary/license text. NOTICE SHA256
`38751245389e1e23f73e6f5384b5cbe7fa972cc4410c5adc9c04b082a0b9561a`.
The seven latest path revisions were checked narrowly; current/2011/2014/2015
fonts differed, and one historical endpoint was unavailable. Only the original
matching commit is used, not newer versions with the same filename.

Older Roboto has not received the same proof: Android4.0.1_r1's corresponding
files differ in size/blob identity, and eight exact local Roboto blob lookups
were unavailable in the selected AOSP repository. These are limited lookup
results, NOT proof of unlicensed content or absence from all upstream sources.
No copying a current OFL/other font's terms, renderer change or source mutation.

### Installed font-directory TPS declarations — verified, applicability open

2026-10-12. A targeted inspection found three adjacent engine TPS files omitted
by the earlier **module-directory** inventory scope. The existing bounded TPS
reader inspected only these files and their declared targets; no module rescan,
engine repair, metadata rewrite, font import or packaging run was performed.

| Installed TPS | Declared target / actual observation |
| --- | --- |
| Content/Slate/Fonts/Roboto.tps | Exact ROBOTO_License.txt exists:11,329bytes, Apache2 text with Google2012 header; legacy text encoding flagged |
| Content/Slate/Fonts/Noto.tps | Exact Noto_License.txt exists:4,391bytes, SIL OFL1.1 text |
| Content/Slate/Fonts/DroidSans.tps | Declared ../../../Build/Android/Java/AndroidOpenSourceProject.tps target is absent in this installed engine |

Both license targets are under Engine/Source/ThirdParty/Licenses. Their exact
SHA256 values, in table order, are:

- ROBOTO_License.txt: `813aec82b71fe6fc927f1f9f5c08c5a098785b0a89e3a2e76697aa4662e5f666`.
- Noto_License.txt: `1beea52b7cf6b8f5fa3120f3b6fc4bd5828037bdbe513624ea55100876654c18`.

The Roboto/Noto declarations date from2016 and name the font directory, not a
hash-pinned list of every face. Roboto is described as an editor font, yet actual
QA Pak indexes prove its delivery here. Do not infer omission or player use from
the TPS function label. The Roboto document's2012 copyright header and delivered
2011/2017 versions remain explicitly different observations, not a proved license
conflict or an exact-version match. Retain original bytes; do not silently recode
the legacy text or replace it with current OFL terms. Noto's installed document
does not supply exact per-face copyright/version provenance by itself. The absent
Droid redirect is a local metadata gap, not a violation finding and does not
invalidate the independent exact DroidMono AOSP proof above.

`Artifacts/C018H-FontAttribution2/FontTpsSources1.json` records source/target hashes,
selected declarations, reader hash and final font/lookup receipt inputs. SHA256:
`b6da23e66a4600ae65301a0e1c173c28e8c54b387f23c1812ebce2c943cd9379`.
Result:2 exact declared documents found,1 unresolved redirect,
`rightsReview=pending_owner`. No raw license text persisted or notice bundle changed.

Separately, all16 exact old-Roboto Git-blob GETs in the two selected primary
repositories (`google/fonts`, `googlefonts/roboto-2`) returned404. Receipt
`RobotoPrimaryLookups1.json` SHA256
`e5d56e9fa172a3322605e990382d000bec9d36d60073dfbab1bf3c5a5e560b7d`.
This proves only those repository-specific lookup results, not absence from all
history/sources or a legal conclusion. No unbounded fork/download search is queued.

Before assembling release notices, review the installed declarations together
with each face's retained copyright/version and any exact upstream evidence.
[Apache2 section4](https://www.apache.org/licenses/LICENSE-2.0) addresses license
delivery, modification markings, retained source attribution and relevant NOTICE
content. [OFL1.1 condition2](https://openfontlicense.org/open-font-license-official-text/)
requires accompanying copyright and license information in an accessible form
when bundling fonts. These primary terms guide the review; they are not an owner
compliance approval or proof that this111-byte FreeType-only package notice is complete.

Primary references checked2026-10-12:

- Current [Google Fonts Roboto metadata](https://github.com/google/fonts/blob/main/ofl/roboto/METADATA.pb)
  describes an OFL variable-font release; it is **not** exact evidence for these
  older static2011/2017 files. No modern license was substituted.
- [Noto's own app-bundling documentation](https://notofonts.github.io/noto-docs/website/use/)
  points to OFL and additional requirements; exact applicable terms/copyrights
  and notices still require a reviewed candidate.
- AOSP android10 browser endpoints initially failed. The later bounded public
  mirror lookup above obtained exact pinned DroidMono source/document evidence;
  do not repeat failed endpoints or inherit failure as a global blocker.

## SDK primary-source clarification from this same block

The exact [Microsoft.Direct3D.D3D12 1.618.5 package page](https://www.nuget.org/packages/Microsoft.Direct3D.D3D12/1.618.5)
assigns LICENSE.txt to binary-directory contents and LICENSE-CODE.txt to headers.
This resolves the binary-versus-header scope ambiguity, not owner compliance or
byte equality with the online license: its license endpoint was inaccessible.
[Microsoft's D3D12 redistributable specification](https://microsoft.github.io/DirectX-Specs/d3d/D3D12Redistributable.html)
lists Core and SDKLayers as SDK components. A debug-oriented filename alone is
not evidence to strip the matched pair from correct native staging.
[Microsoft's XAudio redist guide](https://learn.microsoft.com/en-us/windows/win32/xaudio2/xaudio2-redistributable)
documents the app-local DLL family, not the exact applicable contract for the
installed source/version. Existing SourceMappings2.json bindings remain intact.

## Verification and next action

11/11 targeted tests pass: malformed/truncated SFNT/strings/languages, unknown
encodings, missing explicit license fields, changed Pak/source hashes, wrong
configuration/native exit/counts, unrelated extracted files, size limits,
redacted errors, create-missing-only output and explicit expanded-Slate directory
allowlist (reject arbitrary engine Content). Full pure Python suite69total/
68pass/1existing actual-symlink capability skip; actual native exit0 in
PythonTestsFinal1.log/.exit.json under the new cohort. Independent synthetic
reparse/path guards from the reused helper suite pass. PowerShell5.1 wraps native
unittest stderr in a display error record; tests/native exit did not fail.

Final repository validation exit0/0M1warnings; redacted candidate gate exit0/
343texts/no findings/policyfailures; diff check0, same-block LFSfsck0. Final
ClosingFinal1.json in FontAttribution2 records no named UE/Python jobs, empty
index, HEAD0726de2 and276preserved dirty/untracked status paths. No runtime,
rendering, Content/config or source asset change justified another native suite.

Reproduce parsing only with the SAME saved extraction receipt/manifest and NEW
output filename; do not reextract or rerun UE/build/cook/rendering merely on switch:

```powershell
python -m unittest discover -s Tools/tests -p test_packaged_font_inventory.py
python Tools/packaged_font_inventory.py --project (Get-Location).Path --engine 'C:/Program Files/Epic Games/UE_5.8/Engine' --extraction-receipt Artifacts/C018H-FontShippingAllExtract1.exit.json --manifest Artifacts/Packages/C018H_LocalQA1/Shipping/archive-sha256-Shipping.json --output Artifacts/C018H-FontAttribution2/Fonts-Shipping2.json
```

Before final notice bundling, resolve exact older-font agreements/attribution,
especially DroidMono and mixed Roboto, alongside native library/plugin/SDK rights.
Current package NOTICES.txt remains unchanged. Inventory != owner legal approval,
complete all-engine-asset audit, actual player behavior, clean release or CI.
No new production asset/import/config/dependency, commit/install/trust change.
Packaging skill informed separation of extraction/provenance/rights/player proof.

## Per-identity notice map — 2026-10-21

[M7_FONT_NOTICE_MAP.md](M7_FONT_NOTICE_MAP.md) now binds all14unique font hashes
to exact copyright/nameID0/13/14/version observations, documentary tiers and
specific review gaps. FontNoticeMap02.json reuses five pinned receipts, verifies
Pak/extraction/manifest/TPS/license identities and45raw installed/extracted paths.
No new extraction, font parsing, download or native run. Old/Tiles Bold is an
alias outside the declared TPS directory, not another face or extended scope.
Exact Mono proof is retained separately from historical Ascender metadata and
is not applied to Fallback. Eight oldRoboto identities have no13/14 and do not
inherit the2017 siblings' claims.12new directed mapping tests pass; this is not
a rerun or supersession of the original parser suite/fullPython69 result above.
ATR-13 refined, rights/notice review still pending_owner; no final notices copied
or package/font/engine/license bytes rewritten. See map for current hashes/limits.
