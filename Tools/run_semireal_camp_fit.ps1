[CmdletBinding()]
param([Parameter(Mandatory)][ValidatePattern('^[A-Za-z0-9_]{1,48}$')][string]$Name)
Set-StrictMode -Version Latest
$ErrorActionPreference='Stop'
$root=[IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
$preview=Join-Path $root 'Artifacts/VQ01_Semireal1/GamePreview'
$source=Join-Path $root 'SourceArt/Environment/RomanCamp/semireal_v2'
$output=Join-Path $root "Artifacts/VQ02_CampRevision2/Fit/$Name"
foreach($target in @($preview,$source,$output)) {
    $ancestor=$target
    while($ancestor) {
        if((Test-Path -LiteralPath $ancestor) -and ((Get-Item -LiteralPath $ancestor -Force).Attributes -band [IO.FileAttributes]::ReparsePoint)) { throw 'Linked fit input/output refused' }
        $ancestor=Split-Path $ancestor -Parent
    }
}
if(Test-Path -LiteralPath $output) { throw 'Fit evidence exists; never overwrite it' }
if(Get-Process -Name 'UnrealEditor*','UnrealBuildTool' -ErrorAction SilentlyContinue) { throw 'Close existing UE/UBT first' }
if(Get-CimInstance Win32_Process -Filter "Name='dotnet.exe'" | Where-Object CommandLine -Match 'UnrealBuildTool') { throw 'UBT is active' }
function Snapshot([string]$Base) {
    @(Get-ChildItem -LiteralPath $Base -Force -Recurse | ForEach-Object {
        if($_.Attributes -band [IO.FileAttributes]::ReparsePoint) { throw 'Linked protected input refused' }
        if(-not $_.PSIsContainer) { [ordered]@{path=$_.FullName.Substring($Base.Length+1).Replace('\','/');sha256=(Get-FileHash -LiteralPath $_.FullName).Hash} }
    } | Sort-Object { $_.path })
}
$trees=[ordered]@{content=Join-Path $preview 'Content';source=$source;config=Join-Path $preview 'Config'}
$before=[ordered]@{}
foreach($key in $trees.Keys) { $before[$key]=@(Snapshot $trees[$key]) }
if($before.content.Count -ne 181 -or $before.source.Count -ne 14) { throw 'Reconcile current protected camp cohort first' }
$project=Join-Path $preview 'RomanEmpireGame.uproject'
$descriptor=(Get-FileHash -LiteralPath $project).Hash
$script=Join-Path $PSScriptRoot 'inspect_semireal_camp_fit.py'
$scriptHash=(Get-FileHash -LiteralPath $script).Hash
$helperHash=(Get-FileHash -LiteralPath (Join-Path $PSScriptRoot 'semireal_camp_fit.py')).Hash
[void](New-Item -ItemType Directory -Path $output)
$log=Join-Path $output 'native.log'
$stderr=Join-Path $output 'native.stderr.log'
$scriptCommand=$script.Replace('\','/')+" --name=$Name"
$arguments=@(('"'+$project+'"'),'-run=pythonscript',('-script="'+$scriptCommand+'"'),'-unattended','-nop4','-NullRHI','-nosound','-stdout','-FullStdOutLogOutput','-DisablePlugins=Fab')
$process=Start-Process -FilePath 'C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe' -ArgumentList $arguments -WindowStyle Hidden -PassThru -RedirectStandardOutput $log -RedirectStandardError $stderr
$null=$process.Handle
[ordered]@{pid=$process.Id;started_utc=$process.StartTime.ToUniversalTime().ToString('o');arguments=$arguments;log=$log;script_sha256=$scriptHash;helper_sha256=$helperHash;descriptor_sha256=$descriptor;protected_before=$before} | ConvertTo-Json -Depth 6 | Set-Content -LiteralPath (Join-Path $output 'process.json') -Encoding UTF8
Write-Output "Owned read-only fit PID=$($process.Id); log=$log"
while(-not $process.HasExited) { $null=$process.WaitForExit(1000) }
$process.WaitForExit()
$unchanged=$descriptor -eq (Get-FileHash -LiteralPath $project).Hash
foreach($key in $trees.Keys) {
    $after=@(Snapshot $trees[$key])
    if((ConvertTo-Json -InputObject $before[$key] -Depth 4 -Compress) -cne (ConvertTo-Json -InputObject $after -Depth 4 -Compress)) { $unchanged=$false }
}
if($scriptHash -ne (Get-FileHash -LiteralPath $script).Hash -or $helperHash -ne (Get-FileHash -LiteralPath (Join-Path $PSScriptRoot 'semireal_camp_fit.py')).Hash) { $unchanged=$false }
[ordered]@{pid=$process.Id;exit_code=$process.ExitCode;completed_utc=[DateTime]::UtcNow.ToString('o');protected_unchanged=$unchanged} | ConvertTo-Json | Set-Content -LiteralPath (Join-Path $output 'exit.json') -Encoding UTF8
if($null -eq $process.ExitCode -or $process.ExitCode -ne 0 -or -not $unchanged -or (Get-Item -LiteralPath $stderr).Length -ne 0) { throw 'Read-only fit process/protection failed; inspect preserved evidence' }
$report=Get-Content -LiteralPath (Join-Path $output 'readback.json') -Raw -Encoding UTF8 | ConvertFrom-Json
if($report.status -ne 'pass') { throw 'Native fit readback failed; process success alone is not a pass' }
Write-Output 'Native fit measurement passed; protected Content/source/config/descriptor unchanged. This is not placement/nav/spawn or owner approval.'
