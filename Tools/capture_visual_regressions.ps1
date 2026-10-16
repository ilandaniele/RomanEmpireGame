[CmdletBinding()]
param(
    [ValidatePattern('^[A-Za-z0-9_]{1,48}$')][string]$CapturePrefix = 'C018F',
    [ValidateRange(1,3)][int]$Runs = 1,
    [switch]$Candidates,
    [string]$EngineRoot = 'C:\Program Files\Epic Games\UE_5.8'
)
Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
$root = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
. (Join-Path $PSScriptRoot 'AutomationReport.ps1')
. (Join-Path $PSScriptRoot 'VisualRegressionArtifacts.ps1')
if ($Candidates -and $Runs -ne 1) { throw 'Candidate cohorts use one run; comparisons are independent captures' }
$output = Assert-VisualPath -Path (Join-Path $root "Artifacts/VisualRegressionRuns/$CapturePrefix") -Root $root
if (Test-Path -LiteralPath $output) { throw "Never overwrite visual evidence: $output" }
if ([Diagnostics.Process]::GetProcessesByName('UnrealEditor-Cmd').Count -or
    [Diagnostics.Process]::GetProcessesByName('UnrealEditor').Count -or
    @(Get-CimInstance Win32_Process | Where-Object { $_.Name -eq 'UnrealBuildTool.exe' -or
        ($_.Name -eq 'dotnet.exe' -and $_.CommandLine -match 'UnrealBuildTool\.dll') }).Count) {
    throw 'Wait for existing Unreal/UBT sessions before capture'
}
$editor = Join-Path $EngineRoot 'Engine/Binaries/Win64/UnrealEditor-Cmd.exe'
if (-not (Test-Path -LiteralPath $editor -PathType Leaf)) { throw 'Built installed Editor required' }
if (-not $Candidates) { $null = Assert-VisualBaselineSet -RepositoryRoot $root }
New-Item -ItemType Directory -Path $output | Out-Null
$test = 'RomanEmpireGame.Characters.CanonicalPosePIE'
$images = @()
foreach ($run in 1..$Runs) {
    foreach ($view in @('CharacterProfiles','CharacterRTS','CharacterPossession')) {
        $name = "${view}_Run$run"
        $log = Join-Path $output "$name.log"
        $stderr = Join-Path $output "$name.stderr.log"
        $report = Join-Path $output "$name-automation"
        $arguments = @("`"$(Join-Path $root 'RomanEmpireGame.uproject')`"", '-unattended', '-nop4', '-D3D12',
            '-RenderOffscreen', '-ResX=1280', '-ResY=720', '-ForceRes', '-RomanCaptureCharacters',
            "`"-ExecCmds=Automation RunTest $test;Quit`"", "`"-ReportExportPath=$report`"",
            '-stdout', '-FullStdOutLogOutput', '-log', '-DisablePlugins=Fab')
        if ($Candidates) { $arguments += '-RomanVisualCandidates' } else { $arguments += '-RomanVisualRegression' }
        if ($view -eq 'CharacterRTS') { $arguments += '-RomanCharacterRTSView' }
        if ($view -eq 'CharacterPossession') { $arguments += '-RomanCharacterPlayerView' }
        $process = Start-Process -FilePath $editor -ArgumentList $arguments -WindowStyle Hidden -PassThru `
            -RedirectStandardOutput $log -RedirectStandardError $stderr
        $null = $process.Handle
        [ordered]@{schema=1;pid=$process.Id;view=$view;run=$run;log=$log;report=$report;
            candidate=[bool]$Candidates;startedUtc=[DateTime]::UtcNow.ToString('o')} |
            ConvertTo-Json | Set-Content -LiteralPath (Join-Path $output "$name.process.json") -Encoding UTF8
        Write-Host "$name PID$($process.Id): $log"
        $process.WaitForExit()
        $exitCode = $process.ExitCode
        [ordered]@{schema=1;pid=$process.Id;exitCode=$exitCode;completedUtc=[DateTime]::UtcNow.ToString('o')} |
            ConvertTo-Json | Set-Content -LiteralPath (Join-Path $output "$name.exit.json") -Encoding UTF8
        if ($null -eq $exitCode -or $exitCode -ne 0) { throw "Native visual failure exit${exitCode}: $log" }
        Assert-AutomationReportPassed -ReportDirectory $report -RequiredTests @($test)
        if ((Get-Item -LiteralPath $stderr).Length -ne 0) { throw "Unexpected native stderr: $stderr" }
        $result = Get-Content -LiteralPath (Join-Path $report 'index.json') -Raw -Encoding UTF8 | ConvertFrom-Json
        $row = @($result.tests | Where-Object { $_.fullTestPath -ceq $test })[0]
        $incoming = @($row.entries | Where-Object { $_.event.message -like 'VisualIncoming=*' })
        if ($incoming.Count -ne 1) { throw 'Exactly one native raster receipt required' }
        $path = Assert-VisualPath -Path $incoming[0].event.message.Substring(15) -Root (Join-Path $root 'Artifacts/VisualRegression')
        if ([IO.Path]::GetFileName($path) -cne "$view.png") { throw 'Incorrect native view receipt' }
        $identity = Get-VisualImageIdentity -Path $path
        $comparison = $null
        if (-not $Candidates) {
            $comparisonPath = [IO.Path]::Combine([IO.Path]::GetDirectoryName($path), "$view-comparison.json")
            $comparison = Get-Content -LiteralPath $comparisonPath -Raw | ConvertFrom-Json
            if ($comparison.schema -ne 1 -or $comparison.view -cne $view -or $comparison.candidate -ne $false -or
                $comparison.similar -ne $true -or $comparison.localLimit -ne .01 -or $comparison.globalLimit -ne .002 -or
                $comparison.localError -lt 0 -or $comparison.localError -gt .01 -or
                $comparison.globalError -lt 0 -or $comparison.globalError -gt .002) { throw 'Actual comparison receipt rejected' }
            $null = Assert-VisualBaselineSet -RepositoryRoot $root
        } elseif (@($row.entries | Where-Object { $_.event.message -like 'CANDIDATE ONLY:*' }).Count -ne 1) {
            throw 'Candidate must explicitly disclaim regression approval'
        }
        $images += [ordered]@{view=$view;run=$run;path=$identity.path;sha256=$identity.sha256;sha1=$identity.sha1;
            report=$report;reportSha256=(Get-FileHash -LiteralPath (Join-Path $report 'index.json')).Hash.ToLowerInvariant();
            nativeExitCode=$exitCode;comparison=$comparison}
    }
}
[ordered]@{schema=1;engine='5.8.2';platform='Win64_D3D12';width=1280;height=720;
    candidate=[bool]$Candidates;images=$images;completedUtc=[DateTime]::UtcNow.ToString('o')} |
    ConvertTo-Json -Depth 8 | Set-Content -LiteralPath (Join-Path $output 'manifest.json') -Encoding UTF8
if ($Candidates) { Write-Host 'CANDIDATES ONLY. Inspect each raster before explicit create-missing-only review; no regression gate passed.' }
else { Write-Host 'Independent actual rasters compared; baseline integrity unchanged. No packaged-player/temporal-AA claim.' }
