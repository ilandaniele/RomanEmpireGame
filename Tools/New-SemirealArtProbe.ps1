[CmdletBinding()]
param()
$ErrorActionPreference = 'Stop'
$taskRepoRoot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
$probeRoot = Join-Path $taskRepoRoot 'Artifacts\VQ01_Semireal1\UEProbe'
if (Test-Path -LiteralPath $probeRoot) {
    throw 'Art probe exists: inspect/reuse it, never overwrite or restart its jobs.'
}
foreach ($relativeParent in @('Artifacts', 'Artifacts\VQ01_Semireal1')) {
    $parent = Join-Path $taskRepoRoot $relativeParent
    if ((Test-Path -LiteralPath $parent) -and ((Get-Item -LiteralPath $parent).Attributes -band [IO.FileAttributes]::ReparsePoint)) {
        throw 'Refusing an art-probe parent reparse point.'
    }
}
$template = Join-Path $PSScriptRoot 'ArtProbe'
New-Item -ItemType Directory -Path (Join-Path $probeRoot 'Config') -Force | Out-Null
Copy-Item -LiteralPath (Join-Path $template 'ArtProbe.uproject') -Destination (Join-Path $probeRoot 'ArtProbe.uproject')
Copy-Item -LiteralPath (Join-Path $template 'Config\DefaultEngine.ini') -Destination (Join-Path $probeRoot 'Config\DefaultEngine.ini')
Write-Output "Created isolated source-art project: $probeRoot"
Write-Output 'No game module, MCP, runtime asset replacement, VM or Windows security changes.'
