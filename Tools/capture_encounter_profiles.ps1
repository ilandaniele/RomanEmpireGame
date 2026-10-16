[CmdletBinding()]
param(
    [ValidatePattern('^[A-Za-z0-9_]{1,48}$')][string]$CapturePrefix = 'C018E',
    [ValidateRange(1,10)][int]$FirstRun = 1,
    [ValidateRange(1,10)][int]$Runs = 3,
    [string]$EngineRoot = 'C:\Program Files\Epic Games\UE_5.8'
)
Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
if ($FirstRun + $Runs - 1 -gt 10) { throw 'Run numbers must be within 1..10' }
$repositoryRoot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
$artifactRoot = Join-Path $repositoryRoot 'Artifacts'
$outputRoot = Join-Path $artifactRoot "Profiling/$CapturePrefix"
$editor = Join-Path $EngineRoot 'Engine/Binaries/Win64/UnrealEditor-Cmd.exe'
$project = Join-Path $repositoryRoot 'RomanEmpireGame.uproject'
if (-not (Test-Path -LiteralPath $editor -PathType Leaf)) { throw 'Installed editor required; build first' }
if ([Diagnostics.Process]::GetProcessesByName('UnrealEditor-Cmd').Count -or
    [Diagnostics.Process]::GetProcessesByName('UnrealEditor').Count) { throw 'Close other Unreal sessions before owned captures' }
if (@(Get-CimInstance Win32_Process | Where-Object {
    $_.Name -eq 'UnrealBuildTool.exe' -or ($_.Name -eq 'dotnet.exe' -and $_.CommandLine -match 'UnrealBuildTool\.dll')
}).Count) { throw 'Wait for the existing Unreal build to finish before captures' }
. (Join-Path $PSScriptRoot 'AutomationReport.ps1')
foreach ($directory in @($artifactRoot, (Join-Path $artifactRoot 'Profiling'), $outputRoot,
                        (Join-Path $artifactRoot 'Automation'))) {
    if (-not (Test-Path -LiteralPath $directory)) { New-Item -ItemType Directory -Path $directory | Out-Null }
    if ((Get-Item -LiteralPath $directory -Force).Attributes -band [IO.FileAttributes]::ReparsePoint) {
        throw "Capture path redirected: $directory"
    }
}
# Preflight the complete requested batch before starting any process; never overwrite evidence.
$jobs = @(foreach ($run in $FirstRun..($FirstRun + $Runs - 1)) {
    foreach ($total in @(50,100)) {
        $name = "${CapturePrefix}_Encounter${total}_Run${run}"
        $job = [PSCustomObject]@{
            Name=$name; Total=$total; Log=(Join-Path $artifactRoot "$name.log")
            Stderr=(Join-Path $artifactRoot "$name.stderr.log"); Trace=(Join-Path $outputRoot "$name.utrace")
            Report=(Join-Path $artifactRoot "Automation/$name"); ProcessReceipt=(Join-Path $outputRoot "$name.process.json")
        }
        foreach ($file in @($job.Log,$job.Stderr,$job.Trace,$job.Report,$job.ProcessReceipt,(Join-Path $outputRoot "$name.exit.json"))) {
            if (Test-Path -LiteralPath $file) { throw "Refusing to overwrite capture evidence: $file" }
        }
        $job
    }
})
foreach ($job in $jobs) {
    $arguments = @("`"$project`"", '-unattended', '-nop4', '-D3D12', '-RenderOffscreen',
        '-ResX=1920', '-ResY=1080', '-ForceRes', '-RomanCaptureEncounter', '-llm',
        '-trace=cpu,frame,gpu,stats,bookmark,region,counters,memtag', "`"-tracefile=$($job.Trace)`"",
        "`"-ExecCmds=Automation RunTest RomanEmpireGame.Performance.Encounter$($job.Total)PIE;Quit`"",
        "`"-ReportExportPath=$($job.Report)`"", '-stdout', '-FullStdOutLogOutput', '-log', '-DisablePlugins=Fab')
    $process = Start-Process -FilePath $editor -ArgumentList $arguments -WindowStyle Hidden -PassThru `
        -RedirectStandardOutput $job.Log -RedirectStandardError $job.Stderr
    # PS5.1 must retain a native handle before the process exits; otherwise ExitCode can be null.
    $null = $process.Handle
    [ordered]@{schema=1;pid=$process.Id;name=$job.Name;startedUtc=[DateTime]::UtcNow.ToString('o');
        log=$job.Log;trace=$job.Trace;report=$job.Report} | ConvertTo-Json | Set-Content -LiteralPath $job.ProcessReceipt -Encoding UTF8
    Write-Host "Capture $($job.Name): PID$($process.Id), log $($job.Log)"
    $process.WaitForExit()
    $exitCode = $process.ExitCode
    if ($null -eq $exitCode -or $exitCode -ne 0) { throw "Capture failed/missing exit${exitCode}: $($job.Log)" }
    [ordered]@{schema=1;pid=$process.Id;name=$job.Name;exitCode=$exitCode;completedUtc=[DateTime]::UtcNow.ToString('o')} |
        ConvertTo-Json | Set-Content -LiteralPath (Join-Path $outputRoot "$($job.Name).exit.json") -Encoding UTF8
    Assert-AutomationReportPassed -ReportDirectory $job.Report `
        -RequiredTests @("RomanEmpireGame.Performance.Encounter$($job.Total)PIE")
    if ((Get-Item -LiteralPath $job.Trace).Length -lt 1) { throw 'Missing actual trace' }
    if ((Get-Item -LiteralPath $job.Stderr).Length -gt 0) { throw "Inspect unexpected native stderr: $($job.Stderr)" }
    Write-Host "Capture $($job.Name) finished exit0"
}
Write-Host 'Batch complete. Validate CSV/phase/GPU evidence with summarize_unit_load.ps1 -Encounter; no target FPS claimed.'
