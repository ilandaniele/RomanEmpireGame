[CmdletBinding()]
param(
    [Parameter(Mandatory)][ValidatePattern('^[A-Za-z0-9_]{1,48}$')][string]$Name,
    [ValidateSet('Camp01','Camp02','Camp03')][string]$Camp='Camp01'
)
Set-StrictMode -Version Latest
$ErrorActionPreference='Stop'
$root=[IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
$preview=Join-Path $root 'Artifacts/VQ01_Semireal1/GamePreview'
$source=Join-Path $root 'SourceArt/Environment/RomanCamp/semireal_v1'
$revision='semireal_v1'
if($Camp -eq 'Camp02') { $revision='semireal_v2'; $source=Join-Path $root 'SourceArt/Environment/RomanCamp/semireal_v2' }
if($Camp -eq 'Camp03') { $revision='semireal_v3'; $source=Join-Path $root 'SourceArt/Environment/RomanCamp/semireal_v3' }
$content=Join-Path $preview 'Content'
$candidate=Join-Path $content "ArtCandidates/$Camp"
$output=Join-Path $root "Artifacts/VQ02_CampCook1/$Name"
if($Camp -eq 'Camp02') { $output=Join-Path $root "Artifacts/VQ02_CampRevision2/Cook/$Name" }
if($Camp -eq 'Camp03') { $output=Join-Path $root "Artifacts/VQ02_CampRevision3/Cook/$Name" }
$project=Join-Path $preview 'RomanEmpireGame.uproject'
foreach($target in @($preview,$source,$output)) {
    $ancestor=$target
    while($ancestor) {
        if((Test-Path -LiteralPath $ancestor) -and ((Get-Item -LiteralPath $ancestor -Force).Attributes -band [IO.FileAttributes]::ReparsePoint)) { throw "Refusing redirected path: $ancestor" }
        $ancestor=Split-Path $ancestor -Parent
    }
}
if(Test-Path -LiteralPath $output) { throw 'Fresh cook namespace required; never overwrite evidence' }
if(@(Get-Process -Name 'UnrealEditor*' -ErrorAction SilentlyContinue).Count) { throw 'Wait for existing Editor/cooker' }
if(@(Get-CimInstance Win32_Process | Where-Object { $_.Name -eq 'UnrealBuildTool.exe' -or ($_.Name -eq 'dotnet.exe' -and $_.CommandLine -match 'UnrealBuildTool\.dll') }).Count) { throw 'Wait for existing build' }
if(@(Get-ChildItem -LiteralPath $candidate -File).Count -ne 12) { throw 'Reconcile the selected verified camp namespace before cooking' }
function Snapshot-Tree([string]$Base) {
    @(Get-ChildItem -LiteralPath $Base -Recurse -File | ForEach-Object {
        if($_.Attributes -band [IO.FileAttributes]::ReparsePoint) { throw 'Refusing redirected file' }
        [ordered]@{path=$_.FullName.Substring($Base.Length+1);sha256=(Get-FileHash -LiteralPath $_.FullName).Hash}
    })
}
$contentBefore=Snapshot-Tree $content
$sourceBefore=Snapshot-Tree $source
$configBefore=Snapshot-Tree (Join-Path $preview 'Config')
if(($Camp -eq 'Camp03' -and $contentBefore.Count -ne 193) -or ($Camp -eq 'Camp02' -and $contentBefore.Count -notin @(181,193)) -or ($Camp -eq 'Camp01' -and $contentBefore.Count -notin @(169,181,193)) -or $sourceBefore.Count -ne 14) { throw 'Unexpected source/Content state; reconcile first' }
$sourceManifestHash=(Get-FileHash -LiteralPath (Join-Path $source 'source-manifest.json')).Hash
$expectedManifestHash='0669bef5066f8204730935da2276bcf4cac08e5cc78e5d7f4d6158bdfa905912'
if($Camp -eq 'Camp02') { $expectedManifestHash='aa9364411a1de6c413041ce3075c2c0a0b3457f4d8260de43d4f0a41a9164546' }
if($Camp -eq 'Camp03') { $expectedManifestHash='b1403c940209438f18a7addd6c4afbb7e2f725471f68e66a1e2c49f82f8e83e5' }
if($sourceManifestHash -ne $expectedManifestHash) { throw 'Selected source manifest identity changed' }
$descriptorHash=(Get-FileHash -LiteralPath $project).Hash
New-Item -ItemType Directory -Path $output | Out-Null
$log=Join-Path $output 'cook.log'
$stderr=Join-Path $output 'cook.stderr.log'
# Targeted diagnostic content cook only. Process flags exclude default maps and
# configured AlwaysCook directories, NOT hard/soft dependencies or global shaders.
# This does not establish normal production reachability or a runnable package.
# Use the supported loose writer for inspectable package/bulk bytes; do not
# confuse default Zen storage with missing assets. Add SM6 without removing SM5
# or changing project/Windows settings. Cook01 retained the default Zen/SM5 path.
$cookOutput=Join-Path $output 'Cooked/[Platform]'
$arguments=@(('"'+$project+'"'),'-run=cook','-TargetPlatform=Windows',('-CookDir="'+$candidate+'"'),('-OutputDir="'+$cookOutput+'"'),'-NoDefaultMaps','-NoGameAlwaysCook','-unattended','-nop4','-nosound','-NullRHI','-stdout','-FullStdOutLogOutput','-DisablePlugins=Fab')
$arguments+='-SkipZenStore'
$arguments+='-ini:Engine:[/Script/WindowsTargetPlatform.WindowsTargetSettings]:+D3D12TargetedShaderFormats=PCD3D_SM6'
$p=Start-Process -FilePath 'C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe' -ArgumentList $arguments -WindowStyle Hidden -PassThru -RedirectStandardOutput $log -RedirectStandardError $stderr
$null=$p.Handle
[ordered]@{schema=1;pid=$p.Id;started_utc=$p.StartTime.ToUniversalTime().ToString('o');project=$project;output=$output;log=$log;arguments=$arguments;camp=$Camp;source_revision=$revision;source_manifest_sha256=$sourceManifestHash;diagnostic_content_cook_only=$true;standalone_configuration_claimed=$false;loose_package_writer=$true;additional_shader_format='PCD3D_SM6';descriptor_sha256=$descriptorHash;content_before=$contentBefore;source_before=$sourceBefore;config_before=$configBefore;editor_tests_binary_sha256=(Get-FileHash (Join-Path $preview 'Binaries/Win64/UnrealEditor-RomanEmpireGameTests.dll')).Hash} | ConvertTo-Json -Depth 6 | Set-Content (Join-Path $output 'process.json') -Encoding UTF8
Write-Host "Owned cook $Name PID=$($p.Id); log=$log"
while(-not $p.HasExited) { $null=$p.WaitForExit(1000) }; $p.WaitForExit()
[ordered]@{pid=$p.Id;exit_code=$p.ExitCode;completed_utc=[DateTime]::UtcNow.ToString('o')} | ConvertTo-Json | Set-Content (Join-Path $output 'exit.json') -Encoding UTF8
foreach($scope in @(@($content,$contentBefore),@($source,$sourceBefore),@((Join-Path $preview 'Config'),$configBefore))) {
    $base=$scope[0]; $before=$scope[1]
    if(@(Get-ChildItem -LiteralPath $base -Recurse -File).Count -ne $before.Count) { throw 'Protected file count changed' }
    foreach($file in $before) { if((Get-FileHash -LiteralPath (Join-Path $base $file.path)).Hash -ne $file.sha256) { throw "Protected file changed: $($file.path)" } }
}
if((Get-FileHash -LiteralPath $project).Hash -ne $descriptorHash) { throw 'Project descriptor changed' }
if($null -eq $p.ExitCode -or $p.ExitCode -ne 0 -or (Get-Item $stderr).Length -ne 0) { throw 'Cook process failed/unexpected stderr; inspect preserved evidence' }
$cooked=Join-Path $output 'Cooked/Windows/RomanEmpireGame'
$packages=Join-Path $cooked "Content/ArtCandidates/$Camp"
foreach($asset in Get-ChildItem -LiteralPath $candidate -Filter '*.uasset' -File) {
    $path=Join-Path $packages $asset.Name
    if(-not (Test-Path -LiteralPath $path) -or (Get-Item $path).Length -lt 1) { throw "Candidate not present in actual cooked output: $($asset.Name)" }
}
$registry=Join-Path $cooked 'AssetRegistry.bin'
if(-not (Test-Path -LiteralPath $registry) -or (Get-Item $registry).Length -lt 1) { throw 'Actual cooked registry missing' }
Write-Host 'Targeted cook process/files passed; protected Content/source/config unchanged. Registry/dependency/shader inspection remains; NOT a package/player/release approval.'
