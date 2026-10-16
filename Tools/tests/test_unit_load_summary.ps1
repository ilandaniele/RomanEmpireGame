[CmdletBinding()]
param()
Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
$summaryScript = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../summarize_unit_load.ps1'))
$tokens = $null; $parseErrors = $null
$tree = [Management.Automation.Language.Parser]::ParseFile($summaryScript, [ref]$tokens, [ref]$parseErrors)
if ($parseErrors.Count) { throw ($parseErrors | Out-String) }
$functions = @($tree.FindAll({
    param($node)
    $node -is [Management.Automation.Language.FunctionDefinitionAst] -and
    $node.Name -in @('Summarize-Values','Parse-Metric','Read-EncounterCounters','New-ProfileHeaders')
}, $true))
if ($functions.Count -ne 4) { throw 'Expected four real pure telemetry functions' }
foreach ($definition in $functions) { . ([scriptblock]::Create($definition.Extent.Text)) }

$checks = 0
$values = [double[]](0..239)
$quantiles = Summarize-Values -Values $values
if ($quantiles.samples -ne 240 -or $quantiles.p50 -ne 119 -or
    $quantiles.p95 -ne 227 -or $quantiles.max -ne 239) {
    throw 'Actual lower-index CSV quantiles are incorrect'
}
$checks++
foreach ($shortCount in @(0,1,239)) {
    $short = if ($shortCount -eq 0) { [double[]]@() } else { [double[]](0..($shortCount - 1)) }
    $rejected = $false
    try { Summarize-Values -Values $short | Out-Null } catch { $rejected = $true }
    if (-not $rejected) { throw 'Insufficient CSV samples accepted' }
    $checks++
}
foreach ($invalid in @('', 'NaN', 'Infinity', '-Infinity', '-0.1', 'text', '1,25', $null)) {
    $rejected = $false
    try { Parse-Metric -Row ([PSCustomObject]@{Metric=$invalid}) -Name 'Metric' | Out-Null }
    catch { $rejected = $true }
    if (-not $rejected) { throw 'Invalid/nonfinite/negative/locale-dependent CSV metric accepted' }
    $checks++
}
$missingRejected = $false
try { Parse-Metric -Row ([PSCustomObject]@{}) -Name 'Absent' | Out-Null }
catch { $missingRejected = $true }
if (-not $missingRejected) { throw 'Missing CSV column accepted' }
$checks++
$previousCulture = [Threading.Thread]::CurrentThread.CurrentCulture
try {
    [Threading.Thread]::CurrentThread.CurrentCulture = [Globalization.CultureInfo]::GetCultureInfo('es-UY')
    foreach ($expected in @(@('0',0.0), @('1.25',1.25), @('1e3',1000.0))) {
        $actual = Parse-Metric -Row ([PSCustomObject]@{Metric=$expected[0]}) -Name 'Metric'
        if ($actual -ne $expected[1]) { throw 'Invariant metric parsing changed with workstation locale' }
        $checks++
    }
}
finally { [Threading.Thread]::CurrentThread.CurrentCulture = $previousCulture }
$shortPhase = Summarize-Values -MinimumSamples 1 -Values ([double[]]@(2,4))
if ($shortPhase.samples -ne 2 -or $shortPhase.p95 -ne 2) { throw 'Short phase quantiles changed' }
$checks++
foreach ($invalidMinimum in @(0,241)) {
    $rejected = $false
    try { Summarize-Values -MinimumSamples $invalidMinimum -Values $values | Out-Null } catch { $rejected = $true }
    if (-not $rejected) { throw 'Invalid minimum sample count accepted' }
    $checks++
}
$goodCounters = @{
    'RomanEncounter/Phase'='2'; 'RomanEncounter/AliveUnits'='50'; 'RomanEncounter/TrainingActive'='1'
    'RomanEncounter/Casualties'='0'; 'RomanEncounter/CombatDuringTraining'='1'
}
$parsed = Read-EncounterCounters ([PSCustomObject]$goodCounters) 50
if ($parsed.Phase -ne 2 -or $parsed.AliveUnits -ne 50 -or $parsed.CombatDuringTraining -ne 1) { throw 'Real encounter counters changed' }
$checks++
foreach ($case in @(@('Phase','1'), @('Phase','5'), @('Phase','2.5'), @('AliveUnits','52'),
    @('AliveUnits','0'), @('AliveUnits','-1'), @('AliveUnits','NaN'), @('TrainingActive','2'),
    @('Casualties','52'), @('CombatDuringTraining','2'))) {
    $broken = $goodCounters.Clone(); $broken["RomanEncounter/$($case[0])"] = $case[1]
    $rejected = $false
    try { Read-EncounterCounters ([PSCustomObject]$broken) 50 | Out-Null } catch { $rejected = $true }
    if (-not $rejected) { throw "Invalid encounter $($case[0]) counter accepted" }
    $checks++
}
$headers = @(New-ProfileHeaders @('FrameTime','Streaming/Update','Streaming/Update','Streaming/Update#Duplicate2'))
if ($headers.Count -ne 4 -or $headers[0] -ne 'FrameTime' -or $headers[2] -ne 'Streaming/Update#Duplicate2_') { throw 'Unrelated duplicate CSV columns not preserved' }
$checks++
foreach ($critical in @('FrameTime','GPUTime','DrawCall/Slate','RomanEncounter/Phase')) {
    $rejected = $false
    try { New-ProfileHeaders @($critical,$critical) | Out-Null } catch { $rejected = $true }
    if (-not $rejected) { throw 'Ambiguous measured column accepted' }
    $checks++
}
foreach ($badHeader in @('', 'quoted"Header')) {
    $rejected = $false
    try { New-ProfileHeaders @('FrameTime',$badHeader) | Out-Null } catch { $rejected = $true }
    if (-not $rejected) { throw 'Unsupported/empty header accepted' }
    $checks++
}
foreach ($case in @(@('DrawCall/Slate','drawcall/slate'), @('RomanEncounter/Phase','romanencounter/phase'))) {
    $rejected = $false
    try { New-ProfileHeaders $case | Out-Null } catch { $rejected = $true }
    if (-not $rejected) { throw 'Case-insensitive measured duplicate accepted' }
    $checks++
}
Write-Host "Unit-load/encounter summary: $checks pure parser checks pass; no files/processes/engine touched."
