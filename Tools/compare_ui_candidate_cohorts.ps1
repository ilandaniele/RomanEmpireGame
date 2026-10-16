[CmdletBinding()]
param(
    [Parameter(Mandatory=$true)][ValidatePattern('^[A-Za-z0-9_]{1,48}$')][string]$CohortA,
    [Parameter(Mandatory=$true)][ValidatePattern('^[A-Za-z0-9_]{1,48}$')][string]$CohortB,
    [Parameter(Mandatory=$true)][ValidatePattern('^[A-Za-z0-9_]{1,48}$')][string]$OutputPrefix,
    [string]$EngineRoot='C:\Program Files\Epic Games\UE_5.8'
)
Set-StrictMode -Version Latest
$ErrorActionPreference='Stop'
$root=[IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
. (Join-Path $PSScriptRoot 'UIVisualArtifacts.ps1')
$a=Assert-UIVisualCohort -RepositoryRoot $root -ManifestPath (Join-Path $root "Artifacts/UIVisualRuns/$CohortA/manifest.json")
$b=Assert-UIVisualCohort -RepositoryRoot $root -ManifestPath (Join-Path $root "Artifacts/UIVisualRuns/$CohortB/manifest.json")
if (-not $a.candidate -or -not $b.candidate -or $a.images.Count -ne 7 -or $b.images.Count -ne 7 -or $a.images[0].case -cne $b.images[0].case) {
    throw 'Two independent complete7screen candidate sets of the same case required'
}
if ([Diagnostics.Process]::GetProcessesByName('UnrealEditor-Cmd').Count -or [Diagnostics.Process]::GetProcessesByName('UnrealEditor').Count -or
    @(Get-CimInstance Win32_Process | Where-Object { $_.Name -eq 'UnrealBuildTool.exe' -or ($_.Name -eq 'dotnet.exe' -and $_.CommandLine -match 'UnrealBuildTool\.dll') }).Count) {
    throw 'Wait for existing Unreal/UBT sessions'
}
$output=Assert-VisualPath -Path (Join-Path $root "Artifacts/UIVisualRuns/$OutputPrefix") -Root $root
if (Test-Path -LiteralPath $output) { throw 'Never overwrite UI probe evidence' }
$test='RomanEmpireGame.VisualRegression.FileComparisonContract'
$report=Join-Path $output 'automation'; $log=Join-Path $output 'native.log'; $stderr=Join-Path $output 'stderr.log'
$arguments=@("`"$(Join-Path $root 'RomanEmpireGame.uproject')`"",'-unattended','-nop4','-NullRHI','-RomanUIProbe',
    "`"-ExecCmds=Automation RunTest $test;Quit`"","`"-ReportExportPath=$report`"",'-stdout','-FullStdOutLogOutput','-log','-DisablePlugins=Fab')
$index=0
foreach ($screen in @(Get-UIVisualScreens)) {
    $left=@($a.images | Where-Object { $_.screen -ceq $screen })[0]
    $right=@($b.images | Where-Object { $_.screen -ceq $screen })[0]
    if ($left.path -ieq $right.path -or $left.view -cne $right.view) { throw 'Independent matching native pair required' }
    $arguments += "`"-RomanUIProbeA${index}=$($left.path)`""
    $arguments += "`"-RomanUIProbeB${index}=$($right.path)`""
    $index++
}
New-Item -ItemType Directory -Path $output | Out-Null
$editor=Join-Path $EngineRoot 'Engine/Binaries/Win64/UnrealEditor-Cmd.exe'
$process=Start-Process -FilePath $editor -ArgumentList $arguments -WindowStyle Hidden -PassThru -RedirectStandardOutput $log -RedirectStandardError $stderr
$null=$process.Handle
[ordered]@{schema=1;pid=$process.Id;log=$log;report=$report;cohortA=$CohortA;cohortB=$CohortB;startedUtc=[DateTime]::UtcNow.ToString('o')} |
    ConvertTo-Json | Set-Content -LiteralPath (Join-Path $output 'process.json') -Encoding UTF8
Write-Host "Native UI reproducibility probe PID$($process.Id): $log"
$process.WaitForExit(); $exitCode=$process.ExitCode
[ordered]@{schema=1;pid=$process.Id;exitCode=$exitCode;completedUtc=[DateTime]::UtcNow.ToString('o')} |
    ConvertTo-Json | Set-Content -LiteralPath (Join-Path $output 'exit.json') -Encoding UTF8
if ($null -eq $exitCode -or $exitCode -ne 0) { throw "Native UI reproducibility failed exit${exitCode}: $log" }
Assert-AutomationReportPassed -ReportDirectory $report -RequiredTests @($test)
$data=Get-Content -LiteralPath (Join-Path $report 'index.json') -Raw -Encoding UTF8 | ConvertFrom-Json
$metrics=@($data.tests[0].entries | Where-Object { $_.event.message -like 'UIReproducibility *' })
if ($data.tests.Count -ne 1 -or $metrics.Count -ne 7 -or (Get-Item -LiteralPath $stderr).Length) { throw 'Complete native UI probe receipts required' }
$metrics | ForEach-Object { Write-Host $_.event.message }
Write-Host 'Actual reproducibility diagnosis only; NO reviewed-baseline/regression approval.'
