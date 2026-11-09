[CmdletBinding()]
param(
    [Parameter(Mandatory)][ValidatePattern('^[A-Za-z0-9_]{1,48}$')][string]$Name,
    [ValidateSet('Camp01','Camp02','Camp03')][string]$Camp='Camp01',
    [switch]$SkipPoseHold
)
Set-StrictMode -Version Latest
$ErrorActionPreference='Stop'
$root=[IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
$preview=Join-Path $root 'Artifacts/VQ01_Semireal1/GamePreview'
$evidence=Join-Path $root 'Artifacts/VQ02_CampSource1'
if($Camp -eq 'Camp02') { $evidence=Join-Path $root 'Artifacts/VQ02_CampRevision2' }
if($Camp -eq 'Camp03') { $evidence=Join-Path $root 'Artifacts/VQ02_CampRevision3' }
$output=Join-Path $preview "Artifacts/$Name"
$report=Join-Path $preview "Reports/$Name"
$log=Join-Path $evidence "$Name.log"
$stderr=Join-Path $evidence "$Name.stderr.log"
$processReceipt=Join-Path $evidence "$Name-process.json"
$exitReceipt=Join-Path $evidence "$Name-exit.json"
foreach($target in @($preview,$evidence)) {
    $ancestor=$target
    while($ancestor) {
        if((Test-Path -LiteralPath $ancestor) -and ((Get-Item -LiteralPath $ancestor -Force).Attributes -band [IO.FileAttributes]::ReparsePoint)) { throw 'Refusing redirected path' }
        $ancestor=Split-Path $ancestor -Parent
    }
}
foreach($target in @($output,$report,$log,$stderr,$processReceipt,$exitReceipt)) {
    if(Test-Path -LiteralPath $target) { throw "Evidence exists; never overwrite: $target" }
}
if(@(Get-Process -Name 'UnrealEditor*' -ErrorAction SilentlyContinue).Count) { throw 'Wait for existing Editor' }
if(@(Get-CimInstance Win32_Process | Where-Object { $_.Name -eq 'UnrealBuildTool.exe' -or ($_.Name -eq 'dotnet.exe' -and $_.CommandLine -match 'UnrealBuildTool\.dll') }).Count) { throw 'Wait for build' }
$source=Join-Path $PSScriptRoot 'ArtProbe/Native/SemirealCampPreview.cpp'
$copy=Join-Path $preview 'Source/RomanEmpireGameTests/Private/SemirealCampPreview.cpp'
$binary=Join-Path $preview 'Binaries/Win64/UnrealEditor-RomanEmpireGameTests.dll'
if((Get-FileHash $source).Hash -ne (Get-FileHash $copy).Hash -or (Get-Item $binary).LastWriteTimeUtc -lt (Get-Item $copy).LastWriteTimeUtc) { throw 'Copy/build exact private fixture first' }
$content=Join-Path $preview 'Content'
$before=@(Get-ChildItem -LiteralPath $content -Recurse -File | ForEach-Object { [ordered]@{path=$_.FullName.Substring($content.Length+1);sha256=(Get-FileHash -LiteralPath $_.FullName).Hash} })
$arguments=@(('"'+(Join-Path $preview 'RomanEmpireGame.uproject')+'"'),'-unattended','-nop4','-nosound','-D3D12','-sm6','-RenderOffscreen','-NoTextureStreaming','-ResX=1280','-ResY=720','-ForceRes',"-RomanCampCapture=$Name",'"-ExecCmds=Automation RunTest RomanPreview.Camp.;Quit"',('"-ReportExportPath='+$report+'"'),'-stdout','-FullStdOutLogOutput','-DisablePlugins=Fab')
$arguments+="-RomanCampSource=$Camp"
if($SkipPoseHold) { $arguments+='-RomanCampSkipPoseHold' }
$p=Start-Process -FilePath 'C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe' -ArgumentList $arguments -WindowStyle Hidden -PassThru -RedirectStandardOutput $log -RedirectStandardError $stderr
$null=$p.Handle
[ordered]@{pid=$p.Id;started_utc=$p.StartTime.ToUniversalTime().ToString('o');log=$log;report=$report;output=$output;camp=$Camp;source_sha256=(Get-FileHash $source).Hash;binary_sha256=(Get-FileHash $binary).Hash;skip_pose_hold=[bool]$SkipPoseHold;content_before=$before} | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath $processReceipt -Encoding UTF8
Write-Host "Owned camp $Name PID=$($p.Id); mutation=$([bool]$SkipPoseHold)"
while(-not $p.HasExited) { $null=$p.WaitForExit(1000) }; $p.WaitForExit()
[ordered]@{pid=$p.Id;exit_code=$p.ExitCode;completed_utc=[DateTime]::UtcNow.ToString('o')} | ConvertTo-Json | Set-Content -LiteralPath $exitReceipt -Encoding UTF8
if((Get-Item $stderr).Length -ne 0) { throw 'Unexpected stderr' }
$r=Get-Content (Join-Path $report 'index.json') -Raw -Encoding UTF8 | ConvertFrom-Json
$tests=@($r.tests)
if($tests.Count -ne 2 -or $r.notRun -ne 0 -or $r.inProcess -ne 0) { throw 'Missing/unfinished tests' }
$readback=@($tests | Where-Object fullTestPath -CEQ 'RomanPreview.Camp.Readback')
$gpu=@($tests | Where-Object fullTestPath -CEQ 'RomanPreview.Camp.GPU')
if($readback.Count -ne 1 -or $gpu.Count -ne 1 -or $readback[0].state -cne 'Success' -or $readback[0].errors -ne 0) { throw 'Wrong/failed native readback' }
if($SkipPoseHold) {
    $errors=@($gpu[0].entries | Where-Object { $_.event.type -ceq 'Error' })
    if($p.ExitCode -notin @(0,-1,255) -or $r.failed -ne 1 -or ($r.succeeded+$r.succeededWithWarnings) -ne 1 -or $gpu[0].state -cne 'Fail' -or $gpu[0].errors -ne 1 -or $errors.Count -ne 1 -or $errors[0].event.message -notmatch 'Owned source actor never falls/moves between art witnesses') { throw 'Mutation failed for an unexpected reason or escaped detection' }
} else {
    if($p.ExitCode -ne 0 -or $r.failed -ne 0 -or ($r.succeeded+$r.succeededWithWarnings) -ne 2 -or $gpu[0].state -cne 'Success' -or $gpu[0].errors -ne 0) { throw 'Actual GPU proof failed' }
    foreach($file in @('RTSAll.png','RTSHiddenGround.png','RTSHiddenWall.png','RTSHiddenBarracks.png','FPSAll.png','FPSHiddenCamp.png','RTSReturn.png','camp-gpu-receipt.txt')) {
        $path=Join-Path $output $file
        if(-not (Test-Path -LiteralPath $path) -or (Get-Item $path).Length -lt 1) { throw 'Missing GPU evidence' }
    }
    $campReceipt=Get-Content -LiteralPath (Join-Path $output 'camp-gpu-receipt.txt') -Raw -Encoding UTF8
    if($campReceipt -cnotmatch ('(?m)^camp='+[regex]::Escape($Camp)+'\r?$')) { throw 'Native rendered camp profile mismatch' }
}
if(@(Get-ChildItem -LiteralPath $content -Recurse -File).Count -ne $before.Count) { throw 'Content count changed' }
foreach($entry in $before) {
    if((Get-FileHash -LiteralPath (Join-Path $content $entry.path)).Hash -ne $entry.sha256) { throw ('Content changed: '+$entry.path) }
}
Write-Host "Camp proof verified; mutation=$([bool]$SkipPoseHold), Content $($before.Count) hash-identical. Inspect PNGs/receipts; not owner/player/performance acceptance."
