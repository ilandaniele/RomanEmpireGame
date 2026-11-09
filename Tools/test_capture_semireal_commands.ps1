# Pure command construction regression. Never launches UE or creates evidence.
$ErrorActionPreference = 'Stop'
$commandErrors=$null
$commandTree=[System.Management.Automation.Language.Parser]::ParseFile((Join-Path $PSScriptRoot 'capture_semireal_encounter.ps1'),[ref]$null,[ref]$commandErrors)
if ($commandErrors) { throw $commandErrors }
$commandFunction=$commandTree.FindAll({param($node) $node -is [System.Management.Automation.Language.FunctionDefinitionAst] -and $node.Name -eq 'New-SemirealExecCommands'},$true)
if ($commandFunction.Count -ne 1) { throw 'Expected exactly one command factory' }
. ([scriptblock]::Create($commandFunction[0].Extent.Text))
$commandCases=0
foreach ($unitTotal in @(50,100)) {
    foreach ($nativeResolution in @($false,$true)) {
        foreach ($disableCap in @($false,$true)) {
            $actual=New-SemirealExecCommands $unitTotal $nativeResolution $disableCap
            $parts=$actual.Split(',')
            if ($parts.Count -ne (1+[int]$nativeResolution+[int]$disableCap)) { throw 'Startup commands must be comma-separated' }
            if ($parts[-1] -ne "Automation RunTest RomanEmpireGame.Performance.Encounter${unitTotal}PIE;Quit") { throw 'Lost Automation internal queue' }
            if ($actual.Contains(';r.')) { throw 'A CVar would consume subsequent commands' }
            if ($nativeResolution -and $parts[0] -ne 'r.ScreenPercentage 100') { throw 'Native resolution command missing or reduced' }
            if ($disableCap -and 'r.DontLimitOnBattery 1' -notin $parts) { throw 'Battery diagnostic missing' }
            ++$commandCases
        }
    }
}
Write-Output "Command factory PASS: $commandCases cases; no process launched"
