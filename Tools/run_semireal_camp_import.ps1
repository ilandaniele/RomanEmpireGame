[CmdletBinding()]
param(
    [Parameter(Mandatory)][ValidatePattern('^Camp[0-9]+$')][string]$Attempt,
    [Parameter(Mandatory)][ValidatePattern('^[a-zA-Z0-9_-]+\.json$')][string]$Report,
    [ValidateSet('semireal_v1','semireal_v2','semireal_v3')][string]$SourceRevision='semireal_v1',
    [switch]$ReadOnly
)
Set-StrictMode -Version Latest
$ErrorActionPreference='Stop'
$root=[IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
$preview=Join-Path $root 'Artifacts/VQ01_Semireal1/GamePreview'
$output=Join-Path $root 'Artifacts/VQ02_CampSource1'
if($SourceRevision -eq 'semireal_v2') { $output=Join-Path $root 'Artifacts/VQ02_CampRevision2' }
if($SourceRevision -eq 'semireal_v3') { $output=Join-Path $root 'Artifacts/VQ02_CampRevision3' }
if(($SourceRevision -eq 'semireal_v3' -or $Attempt -eq 'Camp03') -and -not ($SourceRevision -eq 'semireal_v3' -and $Attempt -eq 'Camp03')) { throw 'Camp03 requires exactly semireal_v3' }
$stem=[IO.Path]::GetFileNameWithoutExtension($Report)
$log=Join-Path $output "$stem.log"
$stderr=Join-Path $output "$stem.stderr.log"
$processReceipt=Join-Path $output "$stem-process.json"
$exitReceipt=Join-Path $output "$stem-exit.json"
$reportPath=Join-Path $preview $Report
foreach($target in @($preview,$output)) {
    $ancestor=$target
    while($ancestor) {
        if((Test-Path -LiteralPath $ancestor) -and ((Get-Item -LiteralPath $ancestor -Force).Attributes -band [IO.FileAttributes]::ReparsePoint)) { throw 'Refusing redirected path' }
        $ancestor=Split-Path $ancestor -Parent
    }
}
foreach($target in @($log,$stderr,$processReceipt,$exitReceipt,$reportPath)) {
    if(Test-Path -LiteralPath $target) { throw "Evidence exists; do not overwrite: $target" }
}
if(@(Get-Process -Name 'UnrealEditor*' -ErrorAction SilentlyContinue).Count) { throw 'Wait for existing Editor' }
if(@(Get-CimInstance Win32_Process | Where-Object { $_.Name -eq 'UnrealBuildTool.exe' -or ($_.Name -eq 'dotnet.exe' -and $_.CommandLine -match 'UnrealBuildTool\.dll') }).Count) { throw 'Wait for existing build' }
$project=Join-Path $preview 'RomanEmpireGame.uproject'
$script=(Join-Path $PSScriptRoot 'import_semireal_camp.py').Replace('\','/')
$scriptCommand="$script --attempt=$Attempt --report=$Report --source-revision=$SourceRevision"
if($ReadOnly) { $scriptCommand+=' --read-only' }
$arguments=@(('"'+$project+'"'),'-run=pythonscript',('-script="'+$scriptCommand+'"'),'-unattended','-nop4','-NullRHI','-nosound','-stdout','-FullStdOutLogOutput','-DisablePlugins=Fab')
$process=Start-Process -FilePath 'C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe' -ArgumentList $arguments -WindowStyle Hidden -PassThru -RedirectStandardOutput $log -RedirectStandardError $stderr
$null=$process.Handle
[ordered]@{pid=$process.Id;started_utc=$process.StartTime.ToUniversalTime().ToString('o');log=$log;stderr=$stderr;report=$reportPath;script_sha256=(Get-FileHash $script).Hash;source_revision=$SourceRevision;read_only=[bool]$ReadOnly} | ConvertTo-Json | Set-Content -LiteralPath $processReceipt -Encoding UTF8
Write-Host "Owned $Attempt/$Report PID=$($process.Id)"
while(-not $process.HasExited) { $null=$process.WaitForExit(1000) }
$process.WaitForExit()
[ordered]@{pid=$process.Id;exit_code=$process.ExitCode;completed_utc=[DateTime]::UtcNow.ToString('o')} | ConvertTo-Json | Set-Content -LiteralPath $exitReceipt -Encoding UTF8
if($null -eq $process.ExitCode -or $process.ExitCode -ne 0) { throw 'Process failed; inspect preserved log/report' }
if((Get-Item -LiteralPath $stderr).Length -ne 0) { throw 'Unexpected stderr; inspect before proceeding' }
$receipt=Get-Content -LiteralPath $reportPath -Raw -Encoding UTF8 | ConvertFrom-Json
if($receipt.status -ne 'pass' -or -not $receipt.existing_content_unchanged -or $receipt.source_revision -cne $SourceRevision) { throw 'Asset readback/content/source guard failed' }
Write-Host "Camp readback PASS; protected $($receipt.existing_content_count) Content files. Separate native collision/GPU/player/owner gates remain."
