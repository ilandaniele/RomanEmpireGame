[CmdletBinding()]
param(
    [Parameter(Mandatory=$true)][ValidatePattern('^[A-Za-z0-9_]{1,48}$')][string]$CandidatePrefix,
    [Parameter(Mandatory=$true)][string]$ReviewReference,
    [switch]$Reviewed
)
Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
if (-not $Reviewed) { throw 'Inspect all three actual images and pass -Reviewed; never automatic approval' }
if ($ReviewReference.Length -lt 8 -or $ReviewReference.Length -gt 256) { throw 'Bounded nonempty review reference required' }
$root = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
. (Join-Path $PSScriptRoot 'AutomationReport.ps1')
. (Join-Path $PSScriptRoot 'VisualRegressionArtifacts.ps1')
$candidateFolder = Assert-VisualPath -Path (Join-Path $root "Artifacts/VisualRegressionRuns/$CandidatePrefix") -Root $root
$manifestPath = Join-Path $candidateFolder 'manifest.json'
if ((Get-Item -LiteralPath $manifestPath).Length -gt 64KB) { throw 'Candidate manifest too large' }
$candidate = Get-Content -LiteralPath $manifestPath -Raw -Encoding UTF8 | ConvertFrom-Json
if ($candidate.schema -ne 1 -or $candidate.candidate -ne $true -or $candidate.images.Count -ne 3 -or
    $candidate.platform -cne 'Win64_D3D12' -or $candidate.engine -cne '5.8.2' -or
    $candidate.width -ne 1280 -or $candidate.height -ne 720) { throw 'Completed candidate cohort required' }
$folder = Assert-VisualPath -Path (Join-Path $root 'Tests/VisualBaselines/Win64_D3D12') -Root $root
if (Test-Path -LiteralPath (Join-Path $folder 'manifest.json')) { throw 'Never replace an existing reviewed manifest' }
# Preflight every source/report/destination before creating anything. Existing baselines are never updated.
$jobs = @(foreach ($view in @('CharacterProfiles','CharacterRTS','CharacterPossession')) {
    $rows = @($candidate.images | Where-Object { $_.view -ceq $view })
    if ($rows.Count -ne 1 -or $rows[0].run -ne 1 -or $rows[0].nativeExitCode -ne 0) { throw "Candidate receipt mismatch: $view" }
    $source = Assert-VisualPath -Path $rows[0].path -Root (Join-Path $root 'Artifacts/VisualRegression')
    $identity = Get-VisualImageIdentity -Path $source
    $report = Assert-VisualPath -Path $rows[0].report -Root $candidateFolder
    Assert-AutomationReportPassed -ReportDirectory $report -RequiredTests 'RomanEmpireGame.Characters.CanonicalPosePIE'
    if ($identity.sha256 -cne $rows[0].sha256 -or $identity.sha1 -cne $rows[0].sha1 -or
        (Get-FileHash -LiteralPath (Join-Path $report 'index.json')).Hash.ToLowerInvariant() -cne $rows[0].reportSha256) {
        throw "Candidate/report changed after capture: $view"
    }
    $target = Join-Path $folder "$view.png"
    foreach ($file in @($target,($target+'.sha1'))) {
        if (Test-Path -LiteralPath $file) { throw "Never overwrite reviewed baseline: $file" }
    }
    [PSCustomObject]@{view=$view;source=$source;target=$target;sha256=$identity.sha256;sha1=$identity.sha1}
})
if (-not (Test-Path -LiteralPath $folder)) { New-Item -ItemType Directory -Path $folder | Out-Null }
foreach ($job in $jobs) {
    [IO.File]::Copy($job.source,$job.target,$false)
    # CreateNew also protects against a concurrent/accidental replacement; no image transformation.
    $stream = [IO.File]::Open($job.target+'.sha1',[IO.FileMode]::CreateNew)
    try { $bytes = [Text.Encoding]::ASCII.GetBytes($job.sha1+"`n"); $stream.Write($bytes,0,$bytes.Length) }
    finally { $stream.Dispose() }
}
[ordered]@{schema=1;engine='5.8.2';platform='Win64_D3D12';width=1280;height=720;
    reviewedUtc=[DateTime]::UtcNow.ToString('o');reviewReference=$ReviewReference;candidatePrefix=$CandidatePrefix;
    sourceManifestSha256=(Get-FileHash -LiteralPath $manifestPath).Hash.ToLowerInvariant();
    scope='Static canonical character profiles, RTS and possession. Motion blur/exposure/temporal AA disabled in owned PIE view only.';
    images=@($jobs | ForEach-Object { [ordered]@{view=$_.view;sha256=$_.sha256;sha1=$_.sha1} })} |
    ConvertTo-Json -Depth 5 | Set-Content -LiteralPath (Join-Path $folder 'manifest.json') -Encoding UTF8
$null = Assert-VisualBaselineSet -RepositoryRoot $root
Write-Host 'Reviewed baseline files created without transforming or replacing images. No automatic commit; owner approval/commit remains separate.'
