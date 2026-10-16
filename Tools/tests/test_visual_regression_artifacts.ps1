param([Parameter(Mandatory=$true)][string]$InputPng)
Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
$root = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../..'))
. (Join-Path $root 'Tools/VisualRegressionArtifacts.ps1')
$identity = Get-VisualImageIdentity -Path $InputPng
$fixture = Join-Path $root ('Artifacts/VisualArtifactContract/' + [Guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path $fixture | Out-Null
$script:passed = 0
function Assert-Rejected {
    param([string]$Name,[scriptblock]$Action,[string]$Reason)
    $caught=$false
    try { $null = & $Action } catch {
        $caught=$true
        if ($_.Exception.Message -notlike $Reason) { throw "$Name rejected for unexpected reason: $_" }
    }
    if (-not $caught) { throw "$Name unexpectedly accepted" }
    $script:passed++
}
$null = Assert-VisualPath -Path $identity.path -Root $root; $script:passed++
Assert-Rejected 'Outside root' { Assert-VisualPath -Path (Join-Path $root '../outside.png') -Root $root } '*outside expected root*'
Assert-Rejected 'Root itself' { Assert-VisualPath -Path $root -Root $root } '*outside expected root*'
$fake = Join-Path $fixture 'fake.png'
[IO.File]::WriteAllText($fake,('not png '*8))
Assert-Rejected 'Non-PNG' { Get-VisualImageIdentity -Path $fake } '*Not a PNG*'
$short = Join-Path $fixture 'short.png'
[IO.File]::WriteAllText($short,'short')
Assert-Rejected 'Truncated PNG' { Get-VisualImageIdentity -Path $short } '*bounded PNG*'
$bytes = [IO.File]::ReadAllBytes($identity.path)
$wrongSize = $bytes.Clone(); $wrongSize[19] = 1
[IO.File]::WriteAllBytes($fake,$wrongSize)
Assert-Rejected 'No resize acceptance' { Get-VisualImageIdentity -Path $fake } '*Native 1280x720 required*'
[IO.File]::WriteAllBytes($fake,$bytes[0..32])
Assert-Rejected 'Header-only PNG fails actual decode' { Get-VisualImageIdentity -Path $fake } '*PNG decoding failed*'
$folder = Join-Path $fixture 'Tests/VisualBaselines/Win64_D3D12'
New-Item -ItemType Directory -Path $folder | Out-Null
$images = @(foreach ($view in @('CharacterProfiles','CharacterRTS','CharacterPossession')) {
    [IO.File]::Copy($identity.path,(Join-Path $folder "$view.png"),$false)
    [IO.File]::WriteAllText((Join-Path $folder "$view.png.sha1"),$identity.sha1)
    [ordered]@{view=$view;sha256=$identity.sha256;sha1=$identity.sha1}
})
$manifest = [ordered]@{schema=1;engine='5.8.2';platform='Win64_D3D12';width=1280;height=720;images=$images}
function Save-FixtureManifest {
    $manifest | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath (Join-Path $folder 'manifest.json') -Encoding UTF8
}
Save-FixtureManifest
$null = Assert-VisualBaselineSet -RepositoryRoot $fixture; $script:passed++
$manifest.platform='NullRHI'; Save-FixtureManifest
Assert-Rejected 'No NullRHI baseline' { Assert-VisualBaselineSet -RepositoryRoot $fixture } '*contract mismatch*'
$manifest.platform='Win64_D3D12'; $manifest.engine='5.7'; Save-FixtureManifest
Assert-Rejected 'Wrong engine' { Assert-VisualBaselineSet -RepositoryRoot $fixture } '*contract mismatch*'
$manifest.engine='5.8.2'; $manifest.images=$images[0..1]; Save-FixtureManifest
Assert-Rejected 'Incomplete baseline cohort' { Assert-VisualBaselineSet -RepositoryRoot $fixture } '*contract mismatch*'
$manifest.images=$images; $images[1].view='CharacterProfiles'; Save-FixtureManifest
Assert-Rejected 'Duplicate/missing views' { Assert-VisualBaselineSet -RepositoryRoot $fixture } '*Missing/duplicate*'
$images[1].view='CharacterRTS'; $images[0].sha256='wrong'; Save-FixtureManifest
Assert-Rejected 'Manifest SHA256 mismatch' { Assert-VisualBaselineSet -RepositoryRoot $fixture } '*integrity mismatch*'
$images[0].sha256=$identity.sha256; Save-FixtureManifest
[IO.File]::WriteAllText((Join-Path $folder 'CharacterProfiles.png.sha1'),'wrong')
Assert-Rejected 'Native sidecar SHA1 mismatch' { Assert-VisualBaselineSet -RepositoryRoot $fixture } '*integrity mismatch*'
[IO.File]::WriteAllText((Join-Path $folder 'CharacterProfiles.png.sha1'),$identity.sha1)
$null = Assert-VisualBaselineSet -RepositoryRoot $fixture; $script:passed++
Write-Host "$script:passed visual artifact checks passed. Fixtures only: $fixture"
