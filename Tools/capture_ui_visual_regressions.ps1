[CmdletBinding()]
param(
    [ValidatePattern('^[A-Za-z0-9_]{1,48}$')][string]$CapturePrefix='C018FUI',
    [ValidateRange(0,11)][int]$FirstCase=0,
    [ValidateRange(1,12)][int]$CaseCount=12,
    [switch]$Candidates,
    [string]$EngineRoot='C:\Program Files\Epic Games\UE_5.8'
)
Set-StrictMode -Version Latest
$ErrorActionPreference='Stop'
$root=[IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
. (Join-Path $PSScriptRoot 'UIVisualArtifacts.ps1')
if ($FirstCase+$CaseCount -gt 12) { throw 'UI case range outside0..11' }
$output=Assert-VisualPath -Path (Join-Path $root "Artifacts/UIVisualRuns/$CapturePrefix") -Root $root
if (Test-Path -LiteralPath $output) { throw 'Never overwrite UI capture evidence' }
if ([Diagnostics.Process]::GetProcessesByName('UnrealEditor-Cmd').Count -or [Diagnostics.Process]::GetProcessesByName('UnrealEditor').Count -or
    @(Get-CimInstance Win32_Process | Where-Object { $_.Name -eq 'UnrealBuildTool.exe' -or ($_.Name -eq 'dotnet.exe' -and $_.CommandLine -match 'UnrealBuildTool\.dll') }).Count) {
    throw 'Wait for existing Unreal/UBT sessions'
}
$editor=Join-Path $EngineRoot 'Engine/Binaries/Win64/UnrealEditor-Cmd.exe'
if (-not (Test-Path -LiteralPath $editor -PathType Leaf)) { throw 'Built installed Editor required' }
if (-not $Candidates) { $null=Assert-UIVisualBaselines -RepositoryRoot $root }
New-Item -ItemType Directory -Path $output | Out-Null
$cases=@(Get-UIVisualCases)[$FirstCase..($FirstCase+$CaseCount-1)]
$images=@()
foreach ($case in $cases) {
    $log=Join-Path $output "$($case.name).log"; $stderr=Join-Path $output "$($case.name).stderr.log"
    $report=Join-Path $output "$($case.name)-automation"
    $text=$case.textScale/100
    $arguments=@("`"$(Join-Path $root 'RomanEmpireGame.uproject')`"",'-unattended','-nop4','-D3D12','-RenderOffscreen',
        "-ResX=$($case.width)","-ResY=$($case.height)",'-ForceRes','-RomanCaptureResponsive',"-RomanUITextScale=$text",
        "`"-ExecCmds=Automation RunTest $($case.test);Quit`"","`"-ReportExportPath=$report`"",'-stdout','-FullStdOutLogOutput','-log','-DisablePlugins=Fab')
    if ($Candidates) { $arguments+='-RomanUIVisualCandidates' } else { $arguments+='-RomanUIVisualRegression' }
    $process=Start-Process -FilePath $editor -ArgumentList $arguments -WindowStyle Hidden -PassThru -RedirectStandardOutput $log -RedirectStandardError $stderr
    $null=$process.Handle
    [ordered]@{schema=1;pid=$process.Id;case=$case.name;log=$log;report=$report;candidate=[bool]$Candidates;startedUtc=[DateTime]::UtcNow.ToString('o')} |
        ConvertTo-Json | Set-Content -LiteralPath (Join-Path $output "$($case.name).process.json") -Encoding UTF8
    Write-Host "$($case.name) PID$($process.Id): $log"
    $process.WaitForExit(); $exitCode=$process.ExitCode
    [ordered]@{schema=1;pid=$process.Id;exitCode=$exitCode;completedUtc=[DateTime]::UtcNow.ToString('o')} |
        ConvertTo-Json | Set-Content -LiteralPath (Join-Path $output "$($case.name).exit.json") -Encoding UTF8
    if ($null -eq $exitCode -or $exitCode -ne 0) { throw "UI native failure exit${exitCode}: $log" }
    Assert-AutomationReportPassed -ReportDirectory $report -RequiredTests @($case.test)
    if ((Get-Item -LiteralPath $stderr).Length -gt 0) { throw 'Unexpected UI stderr' }
    $data=Get-Content -LiteralPath (Join-Path $report 'index.json') -Raw -Encoding UTF8 | ConvertFrom-Json
    if ($data.tests.Count -ne 1) { throw 'Exact single native layout required' }
    $entries=@($data.tests[0].entries | Where-Object { $_.event.message -like 'UIVisualIncoming=*' })
    if ($entries.Count -ne 7) { throw 'Seven actual native UI captures required' }
    foreach ($screen in @(Get-UIVisualScreens)) {
        $view="$($case.name)_$screen"
        $matching=@($entries | Where-Object { [IO.Path]::GetFileName($_.event.message.Substring('UIVisualIncoming='.Length)) -ceq "$view.png" })
        if ($matching.Count -ne 1) { throw "Missing/duplicate native UI raster $view" }
        $png=Assert-VisualPath -Path $matching[0].event.message.Substring('UIVisualIncoming='.Length) -Root (Join-Path $root 'Artifacts/UIVisualRegression')
        $identity=Get-VisualImageIdentity -Path $png -ExpectedWidth $case.width -ExpectedHeight $case.height
        $images += [ordered]@{case=$case.name;screen=$screen;view=$view;width=$case.width;height=$case.height;
            path=$identity.path;sha256=$identity.sha256;sha1=$identity.sha1;report=$report;nativeExitCode=$exitCode;
            reportSha256=(Get-FileHash -LiteralPath (Join-Path $report 'index.json')).Hash.ToLowerInvariant()}
    }
    if (-not $Candidates) { $null=Assert-UIVisualBaselines -RepositoryRoot $root }
}
$manifest=Join-Path $output 'manifest.json'
[ordered]@{schema=1;family='UI_v1';engine='5.8.2';platform='Win64_D3D12';candidate=[bool]$Candidates;
    images=$images;completedUtc=[DateTime]::UtcNow.ToString('o')} | ConvertTo-Json -Depth 7 | Set-Content -LiteralPath $manifest -Encoding UTF8
$null=Assert-UIVisualCohort -ManifestPath $manifest -RepositoryRoot $root
if ($Candidates) { Write-Host 'CANDIDATES ONLY; partial sets cannot approve family, no regression success claim.' }
else { Write-Host 'Actual UI comparisons validated; no packaged-player/full-M7 claim.' }
