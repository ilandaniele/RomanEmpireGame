[CmdletBinding()]
param([ValidatePattern('^[A-Za-z0-9_]{1,48}$')][string]$Prefix='C018FUI_Guards',
    [string]$EngineRoot='C:\Program Files\Epic Games\UE_5.8')
Set-StrictMode -Version Latest
$ErrorActionPreference='Stop'
$root=[IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../..'))
. (Join-Path $root 'Tools/UIVisualArtifacts.ps1')
if ([Diagnostics.Process]::GetProcessesByName('UnrealEditor-Cmd').Count -or [Diagnostics.Process]::GetProcessesByName('UnrealEditor').Count -or
    @(Get-CimInstance Win32_Process | Where-Object { $_.Name -eq 'UnrealBuildTool.exe' -or ($_.Name -eq 'dotnet.exe' -and $_.CommandLine -match 'UnrealBuildTool\.dll') }).Count) { throw 'Wait for existing Unreal/UBT sessions' }
$output=Assert-VisualPath -Path (Join-Path $root "Artifacts/UIVisualRuns/$Prefix") -Root $root
if (Test-Path -LiteralPath $output) { throw 'Never overwrite negative UI evidence' }
New-Item -ItemType Directory -Path $output | Out-Null
$cases=@(
    @{name='NoCapture';reason='UI regression requires actual responsive raster capture';flags=@('-RomanUIVisualRegression','-RomanUITextScale=1')},
    @{name='ConflictingModes';reason='Exactly one UI visual mode';flags=@('-RomanCaptureResponsive','-RomanUIVisualRegression','-RomanUIVisualCandidates','-RomanUITextScale=1')},
    @{name='MissingTextScale';reason='Explicit canonical text scale1 or2';flags=@('-RomanCaptureResponsive','-RomanUIVisualCandidates')}
)
$test='RomanEmpireGame.UI.ResponsiveShell.1280x720_100'
foreach ($case in $cases) {
    $report=Join-Path $output "$($case.name)-automation"; $log=Join-Path $output "$($case.name).log"; $stderr=Join-Path $output "$($case.name).stderr.log"
    $nativeArguments=@("`"$(Join-Path $root 'RomanEmpireGame.uproject')`"",'-unattended','-nop4','-NullRHI',
        "`"-ExecCmds=Automation RunTest $test;Quit`"","`"-ReportExportPath=$report`"",'-stdout','-FullStdOutLogOutput','-log','-DisablePlugins=Fab')+$case.flags
    $process=Start-Process -FilePath (Join-Path $EngineRoot 'Engine/Binaries/Win64/UnrealEditor-Cmd.exe') -ArgumentList $nativeArguments -WindowStyle Hidden -PassThru -RedirectStandardOutput $log -RedirectStandardError $stderr
    $null=$process.Handle
    [ordered]@{schema=1;pid=$process.Id;log=$log;report=$report;startedUtc=[DateTime]::UtcNow.ToString('o')} |
        ConvertTo-Json | Set-Content -LiteralPath (Join-Path $output "$($case.name).process.json") -Encoding UTF8
    Write-Host "Negative $($case.name) PID$($process.Id): $log"
    $process.WaitForExit(); $exitCode=$process.ExitCode
    [ordered]@{schema=1;pid=$process.Id;exitCode=$exitCode;completedUtc=[DateTime]::UtcNow.ToString('o')} |
        ConvertTo-Json | Set-Content -LiteralPath (Join-Path $output "$($case.name).exit.json") -Encoding UTF8
    if ($null -eq $exitCode -or $exitCode -ne 255 -or (Get-Item -LiteralPath $stderr).Length) { throw 'Expected native Automation failure255 with report, not crash/harness failure' }
    $data=Get-Content -LiteralPath (Join-Path $report 'index.json') -Raw -Encoding UTF8 | ConvertFrom-Json
    if ($data.tests.Count -ne 1 -or $data.tests[0].fullTestPath -cne $test -or $data.tests[0].state -cne 'Fail' -or $data.failed -ne 1 -or
        @($data.tests[0].entries | Where-Object { $_.event.message -like 'UIVisualIncoming=*' }).Count) { throw 'Exact negative UI report without captures required' }
    $errors=@($data.tests[0].entries | Where-Object { $_.event.type -eq 'Error' })
    if ($errors.Count -ne 1 -or $errors[0].event.message -notlike "*$($case.reason)*") { throw 'Negative UI failure must prove the exact intended flag invariant' }
    $rejected=$false
    try { Assert-AutomationReportPassed -ReportDirectory $report -RequiredTests @($test) } catch { $rejected=$true }
    if (-not $rejected) { throw 'Strict report validator accepted a failed native UI guard' }
}
Write-Host 'Three real native UI misuse failures255 and strict-report rejection passed; NOT positive raster evidence.'
