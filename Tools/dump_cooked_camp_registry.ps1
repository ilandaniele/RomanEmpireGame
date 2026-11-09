[CmdletBinding()]
param(
    [Parameter(Mandatory)][ValidatePattern('^[A-Za-z0-9_]{1,48}$')][string]$Name,
    [Parameter(Mandatory)][ValidateSet('Runtime','Development')][string]$Kind,
    [ValidateSet('Camp01','Camp02','Camp03')][string]$Camp='Camp01'
)
# Read actual cooked registries; never recook, overwrite evidence, or launch a player.
Set-StrictMode -Version Latest
$ErrorActionPreference='Stop'
$root=[IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
$preview=Join-Path $root 'Artifacts/VQ01_Semireal1/GamePreview'
$attempt=Join-Path $root "Artifacts/VQ02_CampCook1/$Name"
if($Camp -eq 'Camp02') { $attempt=Join-Path $root "Artifacts/VQ02_CampRevision2/Cook/$Name" }
if($Camp -eq 'Camp03') { $attempt=Join-Path $root "Artifacts/VQ02_CampRevision3/Cook/$Name" }
foreach($path in @($preview,$attempt)) {
    $item=Get-Item -LiteralPath $path -Force
    while($null -ne $item) {
        if($item.Attributes -band [IO.FileAttributes]::ReparsePoint) { throw 'Linked cook/project path refused' }
        $item=$item.Parent
    }
}
if(Get-Process -Name 'UnrealEditor*','UnrealBuildTool' -ErrorAction SilentlyContinue) { throw 'Close UE/UBT before registry export' }
if(Get-CimInstance Win32_Process -Filter "Name='dotnet.exe'" | Where-Object CommandLine -Match 'UnrealBuildTool') { throw 'UBT is active' }
$cook=Get-Content -LiteralPath (Join-Path $attempt 'process.json') -Raw | ConvertFrom-Json
$exitInfo=Get-Content -LiteralPath (Join-Path $attempt 'exit.json') -Raw | ConvertFrom-Json
$actualCamp='Camp01'
if($cook.PSObject.Properties.Name -contains 'camp') { $actualCamp=$cook.camp }
$project=Join-Path $preview 'RomanEmpireGame.uproject'
if($actualCamp -ne $Camp -or $exitInfo.exit_code -ne 0 -or $cook.project -ne $project -or -not $cook.loose_package_writer) { throw 'Not the selected successful private loose cook' }
$filename='AssetRegistry.bin'
if($Kind -eq 'Development') { $filename='Metadata/DevelopmentAssetRegistry.bin' }
$inputPath=Join-Path $attempt "Cooked/Windows/RomanEmpireGame/$filename"
$inputItem=Get-Item -LiteralPath $inputPath -Force
if($inputItem.Attributes -band [IO.FileAttributes]::ReparsePoint) { throw 'Linked cooked registry refused' }
$inputItem=$inputItem.Directory
while($null -ne $inputItem) {
    if($inputItem.Attributes -band [IO.FileAttributes]::ReparsePoint) { throw 'Linked cooked registry refused' }
    $inputItem=$inputItem.Parent
}
$output=Join-Path $attempt "Registry${Kind}01"
$receiptPrefix=Join-Path $attempt "registry-${Kind}01"
foreach($path in @($output,"$receiptPrefix-process.json","$receiptPrefix-exit.json","$receiptPrefix.log","$receiptPrefix.stderr.log")) {
    if(Test-Path -LiteralPath $path) { throw 'Registry evidence exists; never overwrite or rerun it blindly' }
}
$inputHash=(Get-FileHash -LiteralPath $inputPath).Hash
[void](New-Item -ItemType Directory -Path $output)
$arguments=@(('"'+$project+'"'),'-run=DumpAssetRegistry',('-Path="'+$inputPath+'"'),('-OutDir="'+$output+'"'),'-All','-LinesPerPage=100000','-NullRHI','-unattended','-nop4','-nosound','-stdout','-FullStdOutLogOutput','-DisablePlugins=Fab')
$executable='C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe'
$process=Start-Process -FilePath $executable -ArgumentList $arguments -WindowStyle Hidden -PassThru -RedirectStandardOutput "$receiptPrefix.log" -RedirectStandardError "$receiptPrefix.stderr.log"
$null=$process.Handle
[ordered]@{pid=$process.Id;kind=$Kind;camp=$Camp;started_utc=$process.StartTime.ToUniversalTime().ToString('o');input_path=$inputPath;input_sha256=$inputHash;log="$receiptPrefix.log";output=$output;arguments=$arguments} | ConvertTo-Json -Depth 4 | Set-Content -LiteralPath "$receiptPrefix-process.json" -Encoding UTF8
Write-Output "Owned $Kind registry dump PID=$($process.Id); log=$receiptPrefix.log"
while(-not $process.HasExited) { Start-Sleep -Milliseconds 1000; $process.Refresh() }
$process.WaitForExit()
$unchanged=(Get-FileHash -LiteralPath $inputPath).Hash -eq $inputHash
[ordered]@{pid=$process.Id;exit_code=$process.ExitCode;completed_utc=[DateTime]::UtcNow.ToString('o');input_unchanged=$unchanged;input_sha256=$inputHash} | ConvertTo-Json | Set-Content -LiteralPath "$receiptPrefix-exit.json" -Encoding UTF8
if($process.ExitCode -ne 0 -or -not $unchanged -or (Get-Item -LiteralPath "$receiptPrefix.stderr.log").Length -ne 0) { throw 'Native registry export failed; inspect this job, do not overwrite its evidence' }
if(@(Get-ChildItem -LiteralPath $output -Filter 'Page_*.txt' -File).Count -eq 0) { throw 'Native registry dump pages missing' }
Write-Output "$Kind native registry export passed; input unchanged. Dependency/shader verifier still required."
