[CmdletBinding()]
param([switch]$PlanOnly)
$ErrorActionPreference = 'Stop'
$repoRoot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
$previewRoot = Join-Path $repoRoot 'Artifacts\VQ01_Semireal1\GamePreview'
$probeRoot = Join-Path $repoRoot 'Artifacts\VQ01_Semireal1\UEProbe'

function Assert-PlainPath([string]$Path) {
    $checked = Get-Item -LiteralPath $Path -Force
    while ($null -ne $checked) {
        if ($checked.Attributes -band [IO.FileAttributes]::ReparsePoint) {
            throw "Refusing reparse point: $($checked.FullName)"
        }
        if ($checked -is [IO.FileInfo]) { $checked = $checked.Directory }
        else { $checked = $checked.Parent }
    }
}
if (Test-Path -LiteralPath $previewRoot) {
    throw 'Private game preview exists: inspect/reuse it; never overwrite or restart its jobs.'
}
# Check every existing ancestor, including workspace ancestors, before creating anything.
$parent = [IO.DirectoryInfo](Split-Path $previewRoot -Parent)
while ($null -ne $parent) {
    if (Test-Path -LiteralPath $parent.FullName) { Assert-PlainPath $parent.FullName }
    $parent = $parent.Parent
}
$reportPath = Join-Path $probeRoot 'import10-1-report.json'
Assert-PlainPath $reportPath
$report = Get-Content -LiteralPath $reportPath -Raw -Encoding UTF8 | ConvertFrom-Json
$expectedHash = '3157ac079a631d6fa2a317d550b0b3aeb9a56be9d752d423c478f7d50e0211f6'
if ($report.status -ne 'pass' -or $report.folder -ne '/Game/ArtCandidates/Attempt10_1' -or $report.source_sha256 -ne $expectedHash) {
    throw 'Not the selected verified attempt10 source/import.'
}
$sourcePath = Join-Path $repoRoot 'SourceArt\Characters\RomanSoldier\semireal_v2\SK_Roman_Rome_Semireal01.glb'
Assert-PlainPath $sourcePath
if ((Get-FileHash -LiteralPath $sourcePath -Algorithm SHA256).Hash.ToLowerInvariant() -ne $expectedHash) {
    throw 'Selected source GLB changed; review before making a preview.'
}
$descriptorPath = Join-Path $repoRoot 'RomanEmpireGame.uproject'
Assert-PlainPath $descriptorPath
$descriptor = Get-Content -LiteralPath $descriptorPath -Raw -Encoding UTF8 | ConvertFrom-Json
$entries = [Collections.Generic.List[object]]::new()
function Add-Copy([string]$Source, [string]$Relative) {
    Assert-PlainPath $Source
    $entries.Add([pscustomobject]@{ source=$Source; relative=$Relative; sha256=(Get-FileHash -LiteralPath $Source -Algorithm SHA256).Hash.ToLowerInvariant() })
}
foreach ($tree in @('Source','Config','Content')) {
    $treeRoot = Join-Path $repoRoot $tree
    Assert-PlainPath $treeRoot
    # Manual traversal rejects junctions BEFORE walking through them.
    $pending = [Collections.Generic.Stack[string]]::new(); $pending.Push($treeRoot)
    while ($pending.Count) {
        $directory = $pending.Pop()
        foreach ($item in Get-ChildItem -LiteralPath $directory -Force) {
            Assert-PlainPath $item.FullName
            if ($item.PSIsContainer) { $pending.Push($item.FullName) }
            else { Add-Copy $item.FullName $item.FullName.Substring($repoRoot.Length + 1) }
        }
    }
}
foreach ($binary in @('UnrealEditor-RomanEmpireGame.dll','UnrealEditor-RomanEmpireGameTests.dll','UnrealEditor.modules','tbb12.dll','tbbmalloc.dll')) {
    Add-Copy (Join-Path $repoRoot "Binaries\Win64\$binary") "Binaries\Win64\$binary"
}
Add-Copy (Join-Path $PSScriptRoot 'ArtProbe\Native\SemirealPreviewAuthoring.cpp') 'Source\RomanEmpireGameTests\Private\SemirealPreviewAuthoring.cpp'
Add-Copy (Join-Path $PSScriptRoot 'ArtProbe\Native\SemirealCameraPreview.cpp') 'Source\RomanEmpireGameTests\Private\SemirealCameraPreview.cpp'
Add-Copy (Join-Path $PSScriptRoot 'ArtProbe\Native\SemirealLODReadback.cpp') 'Source\RomanEmpireGameTests\Private\SemirealLODReadback.cpp'
Add-Copy (Join-Path $PSScriptRoot 'ArtProbe\Native\SemirealLODAnimation.cpp') 'Source\RomanEmpireGameTests\Private\SemirealLODAnimation.cpp'
Add-Copy (Join-Path $PSScriptRoot 'ArtProbe\Native\SemirealLODGPU.cpp') 'Source\RomanEmpireGameTests\Private\SemirealLODGPU.cpp'
Add-Copy (Join-Path $PSScriptRoot 'ArtProbe\Native\SemirealEncounterProfile.cpp') 'Source\RomanEmpireGameTests\Private\SemirealEncounterProfile.cpp'
Add-Copy (Join-Path $PSScriptRoot 'ArtProbe\Native\SemirealCampPreview.cpp') 'Source\RomanEmpireGameTests\Private\SemirealCampPreview.cpp'
$candidateRoot = Join-Path $probeRoot 'Content\ArtCandidates\Attempt10_1'
Assert-PlainPath $candidateRoot
$pending = [Collections.Generic.Stack[string]]::new(); $pending.Push($candidateRoot)
while ($pending.Count) {
    foreach ($item in Get-ChildItem -LiteralPath $pending.Pop() -Force) {
        Assert-PlainPath $item.FullName
        if ($item.PSIsContainer) { $pending.Push($item.FullName) }
        else { Add-Copy $item.FullName ('Content\ArtCandidates\Attempt10_1\' + $item.FullName.Substring($candidateRoot.Length + 1)) }
    }
}
$destinations = @($entries | ForEach-Object { $_.relative.ToLowerInvariant() })
if (@($destinations | Select-Object -Unique).Count -ne $entries.Count) { throw 'Copy destinations collide.' }
if ($PlanOnly) {
    Write-Output "PLAN ONLY: $($entries.Count) exact hashed files; private target $previewRoot; no writes."
    return
}
New-Item -ItemType Directory -Path $previewRoot | Out-Null
foreach ($entry in $entries) {
    $destination = Join-Path $previewRoot $entry.relative
    New-Item -ItemType Directory -Path (Split-Path $destination -Parent) -Force | Out-Null
    Copy-Item -LiteralPath $entry.source -Destination $destination
    if ((Get-FileHash -LiteralPath $destination -Algorithm SHA256).Hash.ToLowerInvariant() -ne $entry.sha256) {
        throw "Copied bytes differ: $($entry.relative). Preserve failed preview for diagnosis."
    }
}
# Local descriptor only. No engine/user plugin config, startup services or remote changes.
$blocked = @('Fab','ModelContextProtocol','AllToolsets','EditorToolset','AutomationTestToolset')
foreach ($plugin in $descriptor.Plugins) { if ($plugin.Name -in $blocked) { $plugin.Enabled = $false } }
$plugins = @($descriptor.Plugins)
foreach ($name in @('PythonScriptPlugin','EditorScriptingUtilities')) {
    if ($name -notin @($plugins.Name)) { $plugins += [pscustomobject]@{Name=$name;Enabled=$true} }
    else { ($plugins | Where-Object Name -EQ $name).Enabled=$true }
}
if ('Fab' -notin @($plugins.Name)) { $plugins += [pscustomobject]@{Name='Fab';Enabled=$false} }
$descriptor.Plugins = $plugins
$utf8 = [Text.UTF8Encoding]::new($false)
[IO.File]::WriteAllText((Join-Path $previewRoot 'RomanEmpireGame.uproject'), ($descriptor | ConvertTo-Json -Depth 12), $utf8)
$receipt = [ordered]@{
    schema=1; status='copied_verified'; created_utc=[DateTime]::UtcNow.ToString('o'); project_root=$previewRoot
    original_root=$repoRoot; source_sha256=$expectedHash; candidate_folder=$report.folder
    source_import_report_sha256=(Get-FileHash -LiteralPath $reportPath -Algorithm SHA256).Hash.ToLowerInvariant()
    original_descriptor_sha256=(Get-FileHash -LiteralPath $descriptorPath -Algorithm SHA256).Hash.ToLowerInvariant()
    private_descriptor_sha256=(Get-FileHash -LiteralPath (Join-Path $previewRoot 'RomanEmpireGame.uproject') -Algorithm SHA256).Hash.ToLowerInvariant()
    files=@($entries.ToArray()); game_assets_replaced=$false; graph_integrated=$false; cameras_verified=$false
}
[IO.File]::WriteAllText((Join-Path $previewRoot 'preview-copy-receipt.json'), ($receipt | ConvertTo-Json -Depth 6), $utf8)
Write-Output "Private game context created, $($entries.Count) copied files SHA256 verified."
Write-Output 'Candidate keeps its new namespace; original game, probe and clean evidence untouched.'
