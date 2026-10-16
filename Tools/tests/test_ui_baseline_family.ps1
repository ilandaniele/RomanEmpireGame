param([Parameter(Mandatory=$true)][string]$InputManifest)
Set-StrictMode -Version Latest
$ErrorActionPreference='Stop'
$root=[IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../..'))
. (Join-Path $root 'Tools/UIVisualArtifacts.ps1')
$inputCohort=Assert-UIVisualCohort -ManifestPath $InputManifest -RepositoryRoot $root -Complete
if (-not $inputCohort.candidate) { throw 'Use a full native candidate set as codec input only; no review claim' }
$fixture=Join-Path $root ('Artifacts/UIBaselineContract/'+[Guid]::NewGuid().ToString('N'))
$folder=Join-Path $fixture 'Tests/VisualBaselines/UI_v1/Win64_D3D12'
New-Item -ItemType Directory -Path $folder | Out-Null
$images=@(foreach ($row in $inputCohort.images) {
    $png=Join-Path $folder "$($row.view).png"
    [IO.File]::Copy($row.path,$png,$false)
    [IO.File]::WriteAllText($png+'.sha1',$row.sha1)
    [ordered]@{view=$row.view;sha256=$row.sha256;sha1=$row.sha1}
})
$data=[ordered]@{schema=1;family='UI_v1';engine='5.8.2';platform='Win64_D3D12';images=$images}
$manifest=Join-Path $folder 'manifest.json'
$original=$data | ConvertTo-Json -Depth 8
$script:passed=0
function Save-Fixture { $data | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath $manifest -Encoding UTF8 }
function Reset-Fixture { $script:data=$original | ConvertFrom-Json }
function Check-Fixture { $null=Assert-UIVisualBaselines -RepositoryRoot $fixture }
function Reject {
    param([string]$Name,[string]$Reason)
    $caught=$false
    try { Check-Fixture } catch { $caught=$true; if ($_.Exception.Message -notlike $Reason) { throw "$Name unexpected rejection: $_" } }
    if (-not $caught) { throw "$Name unexpectedly accepted" }; $script:passed++
}
Save-Fixture; Check-Fixture; $script:passed++
foreach ($field in @('schema','family','engine','platform')) {
    Reset-Fixture; if($field -eq 'schema'){$data.$field=2}else{$data.$field='wrong'}
    Save-Fixture; Reject "Invalid$field" '*Complete reviewed UI family required*'
}
Reset-Fixture; $data.images=@($data.images[0..82]); Save-Fixture; Reject 'Partial84' '*Complete reviewed UI family required*'
Reset-Fixture; $data.schema=$true; Save-Fixture; Reject 'Boolean schema not version1' '*Complete reviewed UI family required*'
Reset-Fixture; $data.images[1].view=$data.images[0].view; Save-Fixture; Reject 'Duplicate identities' '*Missing/duplicate*'
foreach ($field in @('sha256','sha1')) {
    Reset-Fixture; $data.images[0].$field='wrong'; Save-Fixture; Reject "Wrong$field" '*integrity mismatch*'
}
Reset-Fixture; Save-Fixture
$sidecar=Join-Path $folder "$($data.images[0].view).png.sha1"
[IO.File]::WriteAllText($sidecar,'wrong'); Reject 'SHA1 sidecar mismatch' '*integrity mismatch*'
[IO.File]::WriteAllText($sidecar,$data.images[0].sha1)
Check-Fixture; $script:passed++
Write-Host "$script:passed UI baseline FAMILY checks passed. Synthetic copies/manifest only: $fixture. No production baseline authored/reviewed."
