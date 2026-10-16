param([Parameter(Mandatory=$true)][string]$InputManifest)
Set-StrictMode -Version Latest
$ErrorActionPreference='Stop'
$root=[IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../..'))
. (Join-Path $root 'Tools/UIVisualArtifacts.ps1')
$native=Assert-UIVisualCohort -ManifestPath $InputManifest -RepositoryRoot $root
if (-not $native.candidate -or $native.images.Count -ne 7) { throw 'Use one actual seven-screen native candidate set' }
$fixture=Join-Path $root ('Artifacts/UIArtifactContract/'+[Guid]::NewGuid().ToString('N'))
$cohort=Join-Path $fixture 'Artifacts/UIVisualRuns/Synthetic'
$raster=Join-Path $fixture 'Artifacts/UIVisualRegression/Synthetic'
$report=Join-Path $cohort 'automation'
New-Item -ItemType Directory -Path $report,$raster | Out-Null
$reportData=Get-Content -LiteralPath (Join-Path $native.images[0].report 'index.json') -Raw -Encoding UTF8 | ConvertFrom-Json
$data=$native | ConvertTo-Json -Depth 20 | ConvertFrom-Json
foreach ($row in $data.images) {
    $old=$row.path
    $row.path=Join-Path $raster "$($row.view).png"
    [IO.File]::Copy($old,$row.path,$false)
    foreach ($entry in $reportData.tests[0].entries) {
        if ($entry.event.message -like 'UIVisualIncoming=*' -and
            [IO.Path]::GetFullPath($entry.event.message.Substring('UIVisualIncoming='.Length)).Equals($old,[StringComparison]::OrdinalIgnoreCase)) {
            $entry.event.message="UIVisualIncoming=$($row.path)"
        }
    }
    $row.report=$report
}
$reportPath=Join-Path $report 'index.json'
$reportData | ConvertTo-Json -Depth 40 | Set-Content -LiteralPath $reportPath -Encoding UTF8
$reportHash=(Get-FileHash -LiteralPath $reportPath).Hash.ToLowerInvariant()
foreach ($row in $data.images) { $row.reportSha256=$reportHash }
$manifestPath=Join-Path $cohort 'manifest.json'
$original=$data | ConvertTo-Json -Depth 20
$originalReport=$reportData | ConvertTo-Json -Depth 40
$originalReportBytes=[IO.File]::ReadAllBytes($reportPath)
$script:passed=0
function Reset-Fixture {
    $script:data=$original | ConvertFrom-Json
    [IO.File]::WriteAllBytes($reportPath,$originalReportBytes)
}
function Save-Fixture {
    $data | ConvertTo-Json -Depth 20 | Set-Content -LiteralPath $manifestPath -Encoding UTF8
}
function Check-Fixture { $null=Assert-UIVisualCohort -RepositoryRoot $fixture -ManifestPath $manifestPath }
function Reject {
    param([string]$Name,[scriptblock]$Action,[string]$Reason)
    $caught=$false
    try { $null=& $Action } catch { $caught=$true; if ($_.Exception.Message -notlike $Reason) { throw "$Name unexpected rejection: $_" } }
    if (-not $caught) { throw "$Name unexpectedly accepted" }
    $script:passed++
}
if (@(Get-UIVisualCases).Count -ne 12 -or @(Get-UIVisualScreens).Count -ne 7) { throw 'Matrix identity mismatch' }; $script:passed++
Save-Fixture; Check-Fixture; $script:passed++
Reject 'Partial cannot approve' { Assert-UIVisualCohort -RepositoryRoot $fixture -ManifestPath $manifestPath -Complete } '*All84*'
foreach ($field in @('schema','family','engine','platform','candidate')) {
    Reset-Fixture
    if ($field -eq 'schema') { $data.$field=2 } else { $data.$field='wrong' }
    Save-Fixture; Reject "Invalid$field" { Check-Fixture } '*contract mismatch*'
}
Reset-Fixture; $data.images=@($data.images[0..5]); Save-Fixture
Reject 'Incomplete screens' { Check-Fixture } '*contract mismatch*'
foreach ($field in @('view','case','screen','width','height','nativeExitCode')) {
    Reset-Fixture
    if ($field -in @('width','height','nativeExitCode')) { $data.images[0].$field=-1 } else { $data.images[0].$field='../wrong' }
    Save-Fixture; Reject "Invalid$field" { Check-Fixture } '*identity/dimensions/exit mismatch*'
}
Reset-Fixture; $data.images[1]=$data.images[0]; Save-Fixture
Reject 'Duplicate ID' { Check-Fixture } '*identity/dimensions/exit mismatch*'
foreach ($field in @('width','height','nativeExitCode')) {
    Reset-Fixture; $data.images[0].$field=[string]$data.images[0].$field; Save-Fixture
    Reject "Numeric string$field" { Check-Fixture } '*identity/dimensions/exit mismatch*'
}
Reset-Fixture; $data.schema=$true; Save-Fixture
Reject 'Boolean schema not version1' { Check-Fixture } '*contract mismatch*'
Reset-Fixture; $data.images[0].nativeExitCode=$false; Save-Fixture
Reject 'Boolean native exit not exit0' { Check-Fixture } '*identity/dimensions/exit mismatch*'
Reset-Fixture; $data.images[0].path=Join-Path $fixture 'outside.png'; Save-Fixture
Reject 'Raster escapes owned root' { Check-Fixture } '*outside expected root*'
foreach ($field in @('sha256','sha1')) {
    Reset-Fixture; $data.images[0].$field='wrong'; Save-Fixture
    Reject "Tampered$field" { Check-Fixture } '*raster changed*'
}
Reset-Fixture; $data.images[0].reportSha256='wrong'; Save-Fixture
Reject 'Tampered report hash' { Check-Fixture } '*report changed*'
Reset-Fixture; $data.images[0].report=Join-Path $fixture 'outside-report'; Save-Fixture
Reject 'Report escapes cohort' { Check-Fixture } '*outside expected root*'
# Deliberately edited copies below are synthetic contract witnesses, never native evidence.
function Save-EditedReport {
    param($Edited)
    $Edited | ConvertTo-Json -Depth 40 | Set-Content -LiteralPath $reportPath -Encoding UTF8
    $hash=(Get-FileHash -LiteralPath $reportPath).Hash.ToLowerInvariant()
    foreach ($row in $data.images) { $row.reportSha256=$hash }
    Save-Fixture
}
Reset-Fixture; $edited=$originalReport | ConvertFrom-Json
$edited.tests[0].state='Fail'; Save-EditedReport $edited
Reject 'Native failure' { Check-Fixture } '*Automation test did not succeed*'
Reset-Fixture; $edited=$originalReport | ConvertFrom-Json
$edited.tests[0].entries=@($edited.tests[0].entries | Where-Object { $_.event.message -notlike 'UI CANDIDATE ONLY:*' }); Save-EditedReport $edited
Reject 'No false candidate approval' { Check-Fixture } '*disclaimer missing*'
Reset-Fixture; $edited=$originalReport | ConvertFrom-Json
$incoming=@($edited.tests[0].entries | Where-Object { $_.event.message -like 'UIVisualIncoming=*' })
$incoming[0].event.message="UIVisualIncoming=$(Join-Path $raster 'detached.png')"; Save-EditedReport $edited
Reject 'PNG detached from native report' { Check-Fixture } '*binding missing*'
Reset-Fixture; $data.candidate=$false
Save-Fixture
Reject 'Cannot relabel candidates as comparisons' { Check-Fixture } '*not relabelled candidates*'
$edited=$originalReport | ConvertFrom-Json
$prototype=@($edited.tests[0].entries | Where-Object { $_.event.message -like 'UI CANDIDATE ONLY:*' })[0]
$edited.tests[0].entries=@($edited.tests[0].entries | Where-Object { $_.event.message -notlike 'UI CANDIDATE ONLY:*' })
foreach ($row in $data.images) {
    $entry=$prototype | ConvertTo-Json -Depth 10 | ConvertFrom-Json
    $entry.event.message="UIVisualComparison $($row.view) local=0.000000000 global=0.000000000 error="
    $edited.tests[0].entries+=@($entry)
}
Save-EditedReport $edited
foreach ($row in $data.images) {
    [ordered]@{schema=1;family='UI_v1';view=$row.view;width=$row.width;height=$row.height;
        candidate=$false;similar=$true;localError=0;globalError=0;localLimit=.01;globalLimit=.002} |
        ConvertTo-Json | Set-Content -LiteralPath (Join-Path $raster "$($row.view)-comparison.json") -Encoding UTF8
}
Save-Fixture; Check-Fixture; $script:passed++
$comparisonPath=Join-Path $raster "$($data.images[0].view)-comparison.json"
$comparisonOriginal=Get-Content -LiteralPath $comparisonPath -Raw -Encoding UTF8
foreach ($field in @('schema','family','view','width','height','candidate','similar','localLimit','globalLimit','localError','globalError')) {
    $comparison=$comparisonOriginal | ConvertFrom-Json
    switch ($field) {
        'candidate' {$comparison.$field=$true}
        'similar' {$comparison.$field=$false}
        'family' {$comparison.$field='wrong'}
        'view' {$comparison.$field='wrong'}
        default {$comparison.$field=-1}
    }
    $comparison | ConvertTo-Json | Set-Content -LiteralPath $comparisonPath -Encoding UTF8
    Reject "Invalid comparison$field" { Check-Fixture } '*comparison rejected*'
}
foreach ($value in @($null,'0',$false,'NaN')) {
    $comparison=$comparisonOriginal | ConvertFrom-Json; $comparison.localError=$value
    $comparison | ConvertTo-Json | Set-Content -LiteralPath $comparisonPath -Encoding UTF8
    Reject 'Invalid numeric comparison receipt' { Check-Fixture } '*comparison rejected*'
}
$comparison=$comparisonOriginal | ConvertFrom-Json; $comparison.localError=.001
$comparison | ConvertTo-Json | Set-Content -LiteralPath $comparisonPath -Encoding UTF8
Reject 'Metric/receipt binding mismatch' { Check-Fixture } '*metrics/receipt mismatch*'
Reset-Fixture; Save-Fixture; Check-Fixture; $script:passed++
Write-Host "$script:passed UI artifact checks passed. Synthetic fixtures only: $fixture"
