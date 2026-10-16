[CmdletBinding()]
param(
    [Parameter(Mandatory=$true)][ValidatePattern('^[A-Za-z0-9_]{1,48}$')][string]$CandidatePrefix,
    [Parameter(Mandatory=$true)][string]$ReviewReference,
    [switch]$Reviewed
)
Set-StrictMode -Version Latest
$ErrorActionPreference='Stop'
if (-not $Reviewed) { throw 'Inspect all84 actual native images and pass -Reviewed; never automatic approval' }
if ($ReviewReference.Trim().Length -lt 8 -or $ReviewReference.Length -gt 256) { throw 'Bounded nonempty review reference required' }
$root=[IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
. (Join-Path $PSScriptRoot 'UIVisualArtifacts.ps1')
$sourceManifest=Join-Path $root "Artifacts/UIVisualRuns/$CandidatePrefix/manifest.json"
if (Test-Path -LiteralPath (Join-Path (Split-Path -Parent $sourceManifest) 'review-rejection.json')) { throw 'Actual image review rejected this cohort; never author its baselines' }
$candidate=Assert-UIVisualCohort -ManifestPath $sourceManifest -RepositoryRoot $root -Complete
if (-not $candidate.candidate) { throw 'Reviewed authoring requires a candidate, not comparison evidence' }
$folder=Assert-VisualPath -Path (Join-Path $root 'Tests/VisualBaselines/UI_v1/Win64_D3D12') -Root $root
$manifestPath=Join-Path $folder 'manifest.json'
if (Test-Path -LiteralPath $manifestPath) { throw 'Never replace an existing reviewed UI manifest' }
# Check every native/report receipt and every destination before writing anything.
$jobs=@(foreach ($row in $candidate.images) {
    $target=Join-Path $folder "$($row.view).png"
    foreach ($path in @($target,($target+'.sha1'))) {
        if (Test-Path -LiteralPath $path) { throw "Never overwrite reviewed UI baseline: $path" }
    }
    [PSCustomObject]@{view=$row.view;source=$row.path;target=$target;sha256=$row.sha256;sha1=$row.sha1}
})
if (-not (Test-Path -LiteralPath $folder)) { New-Item -ItemType Directory -Path $folder | Out-Null }
foreach ($job in $jobs) {
    [IO.File]::Copy($job.source,$job.target,$false)
    $stream=[IO.File]::Open($job.target+'.sha1',[IO.FileMode]::CreateNew)
    try { $bytes=[Text.Encoding]::ASCII.GetBytes($job.sha1+"`n"); $stream.Write($bytes,0,$bytes.Length) }
    finally { $stream.Dispose() }
}
$receipt=[ordered]@{schema=1;family='UI_v1';engine='5.8.2';platform='Win64_D3D12';
    reviewedUtc=[DateTime]::UtcNow.ToString('o');reviewReference=$ReviewReference;candidatePrefix=$CandidatePrefix;
    sourceManifestSha256=(Get-FileHash -LiteralPath $sourceManifest).Hash.ToLowerInvariant();
    scope='Native responsive UMG:12 resolution/DPI/text layouts x7 screens. Owned PIE static scene; no resizing, masking, automatic approval, player or performance claim.';
    images=@($jobs | ForEach-Object { [ordered]@{view=$_.view;sha256=$_.sha256;sha1=$_.sha1} })}
$stream=[IO.File]::Open($manifestPath,[IO.FileMode]::CreateNew)
try { $bytes=[Text.UTF8Encoding]::new($false).GetBytes(($receipt | ConvertTo-Json -Depth 6)); $stream.Write($bytes,0,$bytes.Length) }
finally { $stream.Dispose() }
$null=Assert-UIVisualBaselines -RepositoryRoot $root
Write-Host 'Complete84 reviewed UI family created byte-identically, no replacements. Owner approval/commit remains separate.'
