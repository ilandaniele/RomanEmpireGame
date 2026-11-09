[CmdletBinding()]
param(
    [Parameter(Mandatory)][ValidateSet('Author','Verify','Diagnostic')][string]$Mode,
    [Parameter(Mandatory)][ValidatePattern('^[A-Za-z0-9_]+$')][string]$Run,
    [Parameter(Mandatory)][ValidatePattern('^[A-Za-z0-9_]+$')][string]$Output,
    [ValidatePattern('^Attempt[0-9]+$')][string]$Source='Attempt02',
    [ValidateSet(2,3)][int]$Revision=2
)
Set-StrictMode -Version Latest
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
$root=Join-Path $repo "Artifacts\VQ02_CampRevision$Revision"
$exe=Join-Path $repo 'Artifacts\Toolchains\blender-5.2.2-windows-x64\blender.exe'
$scripts=@{Author=(Join-Path $PSScriptRoot 'create_semireal_camp_v2.py');Verify=(Join-Path $PSScriptRoot 'verify_semireal_camp_source_v2.py');Diagnostic=(Join-Path $root ($Run+'.py'))}
if($Revision -eq 3) { $scripts.Author=Join-Path $PSScriptRoot 'create_semireal_camp_v3.py' }
$script=$scripts[$Mode]; $outputDir=Join-Path $root $Output
$stdout=Join-Path $root ($Run+'.stdout.log'); $stderr=Join-Path $root ($Run+'.stderr.log')
$pidFile=Join-Path $root ($Run+'.pid.json'); $exitFile=Join-Path $root ($Run+'.exit.json')
foreach($path in @($root,$exe,$script)){
    for($ancestor=$path;$ancestor;$ancestor=Split-Path $ancestor -Parent){
        if((Test-Path -LiteralPath $ancestor) -and ((Get-Item -LiteralPath $ancestor -Force).Attributes -band [IO.FileAttributes]::ReparsePoint)){throw 'Redirected input path refused'}
    }
}
foreach($fresh in @($outputDir,$stdout,$stderr,$pidFile,$exitFile)){if(Test-Path -LiteralPath $fresh){throw 'Evidence/output exists; inspect same job instead of restarting'}}
if(Get-Process -Name blender,UnrealEditor,UnrealEditor-Cmd,UnrealBuildTool -ErrorAction SilentlyContinue){throw 'Competing DCC/editor/build active'}
if($Mode -eq 'Author' -and $Output -notmatch '^Attempt[0-9]+$'){throw 'Author output must be a fresh Attempt'}
$arguments=@('--background','--disable-autoexec','--threads','8','--python-exit-code','73','--python',('"'+$script+'"'),'--','--output',('"'+$outputDir+'"'))
if($Mode -ne 'Author'){$arguments+=@('--source',('"'+(Join-Path $root $Source)+'"'))}
if($Mode -eq 'Verify'){$arguments+=@('--revision',"$Revision")}
function Write-FreshJson($path,$data){
    $stream=[IO.File]::Open($path,[IO.FileMode]::CreateNew,[IO.FileAccess]::Write)
    $writer=[IO.StreamWriter]::new($stream,[Text.UTF8Encoding]::new($false))
    try{$writer.Write(($data|ConvertTo-Json -Depth 6))}finally{$writer.Dispose()}
}
$job=Start-Process -FilePath $exe -ArgumentList $arguments -WindowStyle Hidden -RedirectStandardOutput $stdout -RedirectStandardError $stderr -PassThru
$null=$job.Handle
Write-FreshJson $pidFile @{pid=$job.Id;startedUtc=$job.StartTime.ToUniversalTime().ToString('o');mode=$Mode;source=$Source;output=$outputDir;script=$script;scriptSha256=(Get-FileHash -LiteralPath $script -Algorithm SHA256).Hash;stdout=$stdout;stderr=$stderr}
Write-Output ('Owned '+$Mode+' Blender PID='+$job.Id)
$job.WaitForExit(); $nativeExit=$job.ExitCode
$required=@{Author='source-manifest.json';Verify='readback.json';Diagnostic='diagnostic.json'}[$Mode]
$receiptPresent=Test-Path -LiteralPath (Join-Path $outputDir $required)
$stderrBytes=(Get-Item -LiteralPath $stderr).Length
Write-FreshJson $exitFile @{pid=$job.Id;mode=$Mode;exitCode=$nativeExit;completedUtc=[DateTime]::UtcNow.ToString('o');requiredReceiptPresent=$receiptPresent;stderrBytes=$stderrBytes}
Write-Output ('Native exit='+$nativeExit+'; receipt='+$receiptPresent+'; stderrBytes='+$stderrBytes)
if($null -eq $nativeExit -or $nativeExit -ne 0 -or -not $receiptPresent -or $stderrBytes -ne 0){throw 'Native exit/required receipt/stderr gate failed; preserve diagnostics'}
