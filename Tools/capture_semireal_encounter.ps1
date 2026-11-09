[CmdletBinding()]
param(
    [Parameter(Mandatory)][ValidatePattern('^[A-Za-z0-9_]{1,48}$')][string]$Name,
    [Parameter(Mandatory)][ValidateSet(50,100)][int]$Total,
    [switch]$NativeResolutionDiagnostic,
    [switch]$DisableEditorBatteryCapDiagnostic
)
Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
function New-SemirealExecCommands {
    param([int]$UnitTotal,[bool]$NativeResolution,[bool]$DisableBatteryCap)
    if ($UnitTotal -notin @(50,100)) { throw 'Expected50/100 units' }
    $commands = @()
    if ($NativeResolution) { $commands += 'r.ScreenPercentage 100' }
    if ($DisableBatteryCap) { $commands += 'r.DontLimitOnBattery 1' }
    $commands += "Automation RunTest RomanEmpireGame.Performance.Encounter${UnitTotal}PIE;Quit"
    return ($commands -join ',')
}
$repoRoot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
$previewRoot = Join-Path $repoRoot 'Artifacts/VQ01_Semireal1/GamePreview'
$profileRoot = Join-Path $previewRoot 'Artifacts/SemirealPerformance'
$project = Join-Path $previewRoot 'RomanEmpireGame.uproject'
$editor = 'C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe'
foreach ($target in @($previewRoot,$profileRoot)) {
    $ancestor = $target
    while ($ancestor) {
        if ((Test-Path -LiteralPath $ancestor) -and
            ((Get-Item -LiteralPath $ancestor -Force).Attributes -band [IO.FileAttributes]::ReparsePoint)) {
            throw "Refusing redirected profiler path: $ancestor"
        }
        $ancestor = [IO.Path]::GetDirectoryName($ancestor)
    }
}
if (-not (Test-Path $project -PathType Leaf) -or -not (Test-Path $editor -PathType Leaf)) { throw 'Existing private preview/installed editor required' }
if ([Diagnostics.Process]::GetProcessesByName('UnrealEditor').Count -or [Diagnostics.Process]::GetProcessesByName('UnrealEditor-Cmd').Count) { throw 'Existing Unreal session: do not interrupt/relaunch' }
if (@(Get-CimInstance Win32_Process | Where-Object {
    $_.Name -eq 'UnrealBuildTool.exe' -or ($_.Name -eq 'dotnet.exe' -and $_.CommandLine -match 'UnrealBuildTool\.dll')
}).Count) { throw 'Wait for the current build' }
$nativeSource = Join-Path $PSScriptRoot 'ArtProbe/Native/SemirealEncounterProfile.cpp'
$privateSource = Join-Path $previewRoot 'Source/RomanEmpireGameTests/Private/SemirealEncounterProfile.cpp'
$testBinary = Join-Path $previewRoot 'Binaries/Win64/UnrealEditor-RomanEmpireGameTests.dll'
if ((Get-FileHash $nativeSource).Hash -ne (Get-FileHash $privateSource).Hash -or
    (Get-Item $testBinary).LastWriteTimeUtc -lt (Get-Item $privateSource).LastWriteTimeUtc) { throw 'Copy and compile exact private observer first' }
$log = Join-Path $profileRoot "$Name.log"
$stderr = Join-Path $profileRoot "$Name.stderr.log"
$trace = Join-Path $profileRoot "$Name.utrace"
$report = Join-Path $previewRoot "Reports/$Name"
$processReceipt = Join-Path $profileRoot "$Name.process.json"
$exitReceipt = Join-Path $profileRoot "$Name.exit.json"
$csvCopy = Join-Path $profileRoot "$Name.csv"
$nativeOutput = Join-Path $profileRoot $Name
foreach ($target in @($log,$stderr,$trace,$report,$processReceipt,$exitReceipt,$csvCopy,$nativeOutput)) {
    if (Test-Path -LiteralPath $target) { throw "Evidence exists; never overwrite: $target" }
}
if (-not (Test-Path $profileRoot)) { New-Item -ItemType Directory -Path $profileRoot | Out-Null }
$contentRoot = Join-Path $previewRoot 'Content'
$contentBefore = @(Get-ChildItem -LiteralPath $contentRoot -Recurse -File | ForEach-Object {
    [ordered]@{path=$_.FullName.Substring($contentRoot.Length+1);sha256=(Get-FileHash -LiteralPath $_.FullName).Hash}
})
$power = (& powercfg.exe /GETACTIVESCHEME | Out-String).Trim()
$battery = @(Get-CimInstance Win32_Battery | Select-Object BatteryStatus,EstimatedChargeRemaining)
# Engine startup ExecCmds splits console commands on commas. The Automation
# command has its OWN semicolon queue for RunTest;Quit. Never join CVar commands
# with semicolons: a CVar consumes the rest and the test never starts.
$profileExec = New-SemirealExecCommands -UnitTotal $Total -NativeResolution ([bool]$NativeResolutionDiagnostic) -DisableBatteryCap ([bool]$DisableEditorBatteryCapDiagnostic)
$arguments = @("`"$project`"",'-unattended','-nop4','-nosound','-D3D12','-sm6','-RenderOffscreen',
    '-ResX=1920','-ResY=1080','-ForceRes','-RomanCaptureEncounter',"-RomanSemirealProfile=$Name", "-RomanSemirealProfileUnits=$Total",'-llm',
    '-trace=cpu,frame,gpu,stats,bookmark,region,counters,memtag',"`"-tracefile=$trace`"",
    "`"-ExecCmds=$profileExec`"",
    "`"-ReportExportPath=$report`"",'-stdout','-FullStdOutLogOutput','-log','-DisablePlugins=Fab')
$process = Start-Process -FilePath $editor -ArgumentList $arguments -WindowStyle Hidden -PassThru -RedirectStandardOutput $log -RedirectStandardError $stderr
$null = $process.Handle
[ordered]@{schema=1;name=$Name;total=$Total;pid=$process.Id;startedUtc=[DateTime]::UtcNow.ToString('o');log=$log;trace=$trace;report=$report;
    observer_sha256=(Get-FileHash $nativeSource).Hash;test_binary_sha256=(Get-FileHash $testBinary).Hash;power_scheme=$power;battery=$battery;
    process_only_diagnostics=[ordered]@{native_resolution=[bool]$NativeResolutionDiagnostic;disable_editor_battery_cap=[bool]$DisableEditorBatteryCapDiagnostic};
    private_content_before=$contentBefore} |
    ConvertTo-Json -Depth 6 | Set-Content -LiteralPath $processReceipt -Encoding UTF8
Write-Host "Owned capture $Name PID=$($process.Id); $log"
while (-not $process.HasExited) { $null = $process.WaitForExit(1000) }
$process.WaitForExit()
$exitCode = $process.ExitCode
[ordered]@{schema=1;name=$Name;pid=$process.Id;exitCode=$exitCode;completedUtc=[DateTime]::UtcNow.ToString('o')} |
    ConvertTo-Json | Set-Content -LiteralPath $exitReceipt -Encoding UTF8
if ($null -eq $exitCode -or $exitCode -ne 0) { throw "Process exit failed/missing: $exitCode. Preserve logs/receipt." }
if ((Get-Item -LiteralPath $stderr).Length -gt 0) { throw 'Inspect unexpected native stderr; preserve evidence' }
. (Join-Path $PSScriptRoot 'AutomationReport.ps1')
Assert-AutomationReportPassed -ReportDirectory $report -RequiredTests @("RomanEmpireGame.Performance.Encounter${Total}PIE")
foreach ($file in @($trace,(Join-Path $nativeOutput 'profile-receipt.txt'),(Join-Path $nativeOutput 'population.csv'),(Join-Path $nativeOutput 'PreCapture.png'))) {
    if (-not (Test-Path -LiteralPath $file -PathType Leaf) -or (Get-Item -LiteralPath $file).Length -lt 1) { throw "Missing actual evidence: $file" }
}
$profileCsvMatches = [regex]::Matches((Get-Content -LiteralPath $log -Raw -Encoding UTF8),'Writing CSV to file\s*:\s*[^\r\n]*?(Profile\([0-9]{8}_[0-9]{6}\)\.csv)')
if ($profileCsvMatches.Count -ne 1) { throw 'Expected one actual profiler CSV receipt' }
$sourceCsv = Join-Path $previewRoot ('Saved/Profiling/CSV/'+$profileCsvMatches[0].Groups[1].Value)
Copy-Item -LiteralPath $sourceCsv -Destination $csvCopy
$contentAfter = @(Get-ChildItem -LiteralPath $contentRoot -Recurse -File)
if ($contentBefore.Count -ne $contentAfter.Count) { throw 'Private Content count changed' }
foreach ($entry in $contentBefore) {
    if ((Get-FileHash -LiteralPath (Join-Path $contentRoot $entry.path)).Hash -ne $entry.sha256) { throw ('Private asset changed: '+$entry.path) }
}
Write-Host "Capture passed; Content hash-identical; CSV=$csvCopy. Analyze actual full-population RTS frames; no player60FPS claim."
