[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)]
    [string]$EngineRoot,
    [ValidateSet('Development', 'Shipping', 'CI')]
    [string]$Configuration = 'CI',
    [string]$OutputDirectory = 'Artifacts/CI'
)
Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
$repositoryRoot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
$artifactRoot = [IO.Path]::GetFullPath((Join-Path $repositoryRoot 'Artifacts'))
$outputRoot = if ([IO.Path]::IsPathRooted($OutputDirectory)) {
    [IO.Path]::GetFullPath($OutputDirectory)
} else { [IO.Path]::GetFullPath((Join-Path $repositoryRoot $OutputDirectory)) }
if (-not $outputRoot.StartsWith($artifactRoot + [IO.Path]::DirectorySeparatorChar,
                                [StringComparison]::OrdinalIgnoreCase)) {
    throw 'Build metadata must be written to a child of this repository Artifacts root'
}
# Reject junction/symlink redirection below the allowed root before any write.
$ancestor = $outputRoot
while ($ancestor.StartsWith($artifactRoot, [StringComparison]::OrdinalIgnoreCase)) {
    if (Test-Path -LiteralPath $ancestor) {
        if ((Get-Item -LiteralPath $ancestor -Force).Attributes -band [IO.FileAttributes]::ReparsePoint) {
            throw "Metadata destination contains a reparse point: $ancestor"
        }
    }
    if ($ancestor -eq $artifactRoot) { break }
    $ancestor = Split-Path -Parent $ancestor
}
$engineVersionFile = Join-Path (Resolve-Path -LiteralPath $EngineRoot).Path 'Engine/Build/Build.version'
$engineVersion = Get-Content -LiteralPath $engineVersionFile -Raw -Encoding UTF8 | ConvertFrom-Json
$project = Get-Content -LiteralPath (Join-Path $repositoryRoot 'RomanEmpireGame.uproject') -Raw -Encoding UTF8 | ConvertFrom-Json
$revision = & git.exe -C $repositoryRoot rev-parse --verify HEAD
if ($LASTEXITCODE -ne 0) { throw 'Cannot identify build revision' }
$status = & git.exe -C $repositoryRoot status --porcelain --untracked-files=normal
if ($LASTEXITCODE -ne 0) { throw 'Cannot identify dirty build state' }
$metadata = [ordered]@{
    schema = 1
    createdUtc = [DateTime]::UtcNow.ToString('o')
    revision = [string]$revision
    dirty = (@($status).Count -ne 0)
    configuration = $Configuration
    projectEngineAssociation = [string]$project.EngineAssociation
    engineVersion = "$($engineVersion.MajorVersion).$($engineVersion.MinorVersion).$($engineVersion.PatchVersion)"
    engineChangelist = $engineVersion.Changelist
    engineCompatibleChangelist = $engineVersion.CompatibleChangelist
    ciRunId = [string]$env:GITHUB_RUN_ID
    ciRunAttempt = [string]$env:GITHUB_RUN_ATTEMPT
    evidence = 'Sidecar source/build identity; not embedded binary identity or a player launch certificate'
}
New-Item -ItemType Directory -Path $outputRoot -Force | Out-Null
$metadataFile = Join-Path $outputRoot "build-metadata-$Configuration.json"
if ((Test-Path -LiteralPath $metadataFile) -and
    ((Get-Item -LiteralPath $metadataFile -Force).Attributes -band [IO.FileAttributes]::ReparsePoint)) {
    throw 'Build metadata file must not redirect outside the allowed output root'
}
$metadata | ConvertTo-Json -Depth 6 | Set-Content -LiteralPath $metadataFile -Encoding UTF8
$readback = Get-Content -LiteralPath $metadataFile -Raw -Encoding UTF8 | ConvertFrom-Json
if ($readback.schema -ne 1 -or $readback.revision -ne $revision -or $readback.configuration -ne $Configuration) {
    throw 'Build metadata readback failed'
}
Write-Host "Build metadata saved: $metadataFile (dirty=$($metadata.dirty))"
