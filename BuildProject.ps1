[CmdletBinding()]
param(
    [ValidateSet('Validate', 'Generate', 'Editor', 'IWYU', 'Game', 'Test', 'Cook', 'Package')]
    [string]$Action = 'Validate',

    [string]$EngineRoot,

    [ValidateSet('DebugGame', 'Development', 'Test', 'Shipping')]
    [string]$Configuration = 'Development',

    [ValidateSet('VisualStudioCode', 'VisualStudio')]
    [string]$IDE = 'VisualStudioCode',

    [string]$ArchiveDirectory,

    [switch]$UseBuildAcceleration,

    # Process-only opt-in when the optional engine Fab browser cannot load on this host.
    [switch]$DisableFab
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

if ($DisableFab -and $Action -notin @('Test', 'Cook', 'Package')) {
    throw '-DisableFab applies only to Test, Cook, and Package editor processes.'
}
$editorStartupArguments = @()
$cookerStartupArguments = @()
if ($DisableFab) {
    $editorStartupArguments = @('-DisablePlugins=Fab')
    $cookerStartupArguments = @('-AdditionalCookerOptions=-DisablePlugins=Fab')
    Write-Host 'Fab disabled for this automation process only; project, engine and Windows security settings are unchanged.'
}

$projectRoot = $PSScriptRoot
$projectFile = Join-Path $projectRoot 'RomanEmpireGame.uproject'
$project = Get-Content -LiteralPath $projectFile -Raw | ConvertFrom-Json
$requiredEngineVersion = [string]$project.EngineAssociation

function Add-EngineCandidate {
    param(
        [System.Collections.Generic.List[string]]$Candidates,
        [string]$Path
    )

    if (-not [string]::IsNullOrWhiteSpace($Path) -and -not $Candidates.Contains($Path)) {
        $Candidates.Add($Path)
    }
}

function Find-EngineRoot {
    param(
        [string]$ExplicitRoot,
        [string]$RequiredVersion
    )

    $candidates = [System.Collections.Generic.List[string]]::new()
    Add-EngineCandidate -Candidates $candidates -Path $ExplicitRoot
    Add-EngineCandidate -Candidates $candidates -Path $env:ROMANEMPIREGAME_ENGINE_ROOT

    if (-not [string]::IsNullOrWhiteSpace($env:ProgramFiles)) {
        Add-EngineCandidate -Candidates $candidates -Path (Join-Path $env:ProgramFiles "Epic Games\UE_$RequiredVersion")
    }

    $manifestRoot = Join-Path $env:ProgramData 'Epic\EpicGamesLauncher\Data\Manifests'
    if (Test-Path -LiteralPath $manifestRoot) {
        Get-ChildItem -LiteralPath $manifestRoot -Filter '*.item' -File -ErrorAction SilentlyContinue | ForEach-Object {
            try {
                $manifest = Get-Content -LiteralPath $_.FullName -Raw | ConvertFrom-Json
                if ($manifest.DisplayName -like "Unreal Engine $RequiredVersion*") {
                    Add-EngineCandidate -Candidates $candidates -Path ([string]$manifest.InstallLocation)
                }
            }
            catch {
                Write-Verbose "Skipping unreadable Epic manifest $($_.FullName): $($_.Exception.Message)"
            }
        }
    }

    $registeredBuildsKey = 'HKCU:\Software\Epic Games\Unreal Engine\Builds'
    if (Test-Path -LiteralPath $registeredBuildsKey) {
        $registeredBuilds = Get-ItemProperty -LiteralPath $registeredBuildsKey
        $registeredBuilds.PSObject.Properties |
            Where-Object { $_.Name -notlike 'PS*' } |
            ForEach-Object { Add-EngineCandidate -Candidates $candidates -Path ([string]$_.Value) }
    }

    foreach ($candidate in $candidates) {
        if (-not (Test-Path -LiteralPath $candidate -PathType Container)) {
            continue
        }

        $resolved = (Resolve-Path -LiteralPath $candidate).Path
        if ((Split-Path -Leaf $resolved) -eq 'Engine') {
            $resolved = Split-Path -Parent $resolved
        }

        $buildBatch = Join-Path $resolved 'Engine\Build\BatchFiles\Build.bat'
        $buildVersionFile = Join-Path $resolved 'Engine\Build\Build.version'
        if (-not (Test-Path -LiteralPath $buildBatch -PathType Leaf) -or
            -not (Test-Path -LiteralPath $buildVersionFile -PathType Leaf)) {
            continue
        }

        $buildVersion = Get-Content -LiteralPath $buildVersionFile -Raw | ConvertFrom-Json
        $actualVersion = "$($buildVersion.MajorVersion).$($buildVersion.MinorVersion)"
        if ($actualVersion -eq $RequiredVersion) {
            return $resolved
        }
    }

    $searched = if ($candidates.Count -gt 0) { $candidates -join "`n  - " } else { '(no candidates)' }
    throw @"
Unreal Engine $RequiredVersion was not found.

Install the Unreal Engine $RequiredVersion Launcher build, then either:
  - rerun this command and allow Epic Launcher discovery;
  - pass -EngineRoot 'C:\Program Files\Epic Games\UE_$RequiredVersion'; or
  - set ROMANEMPIREGAME_ENGINE_ROOT to that directory.

Candidates checked:
  - $searched
"@
}

function Assert-VisualStudioToolchain {
    $vsWhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
    if (-not (Test-Path -LiteralPath $vsWhere -PathType Leaf)) {
        throw 'The Microsoft C++ build toolchain was not found. VS Code can be used as the editor, but UE 5.8 still requires Visual Studio 2022 Build Tools 17.14+ (or a supported full Visual Studio installation), MSVC, and a Windows SDK. Use the repository .vsconfig component list.'
    }

    $installationPath = & $vsWhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
    if ($LASTEXITCODE -ne 0 -or [string]::IsNullOrWhiteSpace(($installationPath -join ''))) {
        throw 'A Visual Studio C++ toolchain was not found. Install Visual Studio 2022 Build Tools 17.14+ (or a supported full Visual Studio installation) with the MSVC x64/x86 tools and Windows SDK from .vsconfig.'
    }

    return ($installationPath | Select-Object -First 1)
}

function Assert-WindowsSdk {
    param([string]$ResolvedEngineRoot)

    $sdkPolicyPath = Join-Path $ResolvedEngineRoot 'Engine\Config\Windows\Windows_SDK.json'
    if (-not (Test-Path -LiteralPath $sdkPolicyPath -PathType Leaf)) {
        throw "UE Windows SDK policy was not found: $sdkPolicyPath"
    }

    $sdkPolicy = Get-Content -LiteralPath $sdkPolicyPath -Raw | ConvertFrom-Json
    $requiredVersion = [version]$sdkPolicy.MainVersion
    $installedRootsKeys = @(
        'HKLM:\SOFTWARE\WOW6432Node\Microsoft\Windows Kits\Installed Roots'
        'HKLM:\SOFTWARE\Microsoft\Windows Kits\Installed Roots'
    )
    $sdkRootCandidates = @(
        foreach ($installedRootsKey in $installedRootsKeys) {
            if (Test-Path -LiteralPath $installedRootsKey) {
                [string](Get-ItemProperty -LiteralPath $installedRootsKey).KitsRoot10
            }
        }
        (Join-Path ${env:ProgramFiles(x86)} 'Windows Kits\10')
    ) | Where-Object {
        -not [string]::IsNullOrWhiteSpace($_) -and
        (Test-Path -LiteralPath (Join-Path $_ 'Lib') -PathType Container) -and
        (Test-Path -LiteralPath (Join-Path $_ 'Include') -PathType Container)
    } | Select-Object -Unique

    $sdkRoot = $sdkRootCandidates | Select-Object -First 1
    if ([string]::IsNullOrWhiteSpace($sdkRoot)) {
        throw "Windows SDK $requiredVersion or newer was not found. Install Microsoft.VisualStudio.Component.Windows11SDK.22621 from .vsconfig."
    }

    $validSdks = foreach ($directory in Get-ChildItem -LiteralPath (Join-Path $sdkRoot 'Lib') -Directory -ErrorAction SilentlyContinue) {
        try {
            $candidateVersion = [version]$directory.Name
        }
        catch {
            continue
        }

        $windowsHeader = Join-Path $sdkRoot "Include\$($directory.Name)\um\Windows.h"
        $kernelLibrary = Join-Path $sdkRoot "Lib\$($directory.Name)\um\x64\Kernel32.Lib"
        if ($candidateVersion -ge $requiredVersion -and
            (Test-Path -LiteralPath $windowsHeader -PathType Leaf) -and
            (Test-Path -LiteralPath $kernelLibrary -PathType Leaf)) {
            [pscustomobject]@{ Version = $candidateVersion; Name = $directory.Name }
        }
    }

    $selectedSdk = $validSdks | Sort-Object Version -Descending | Select-Object -First 1
    if (-not $selectedSdk) {
        throw "Windows SDK $requiredVersion or newer is incomplete or unavailable under $sdkRoot. Install Microsoft.VisualStudio.Component.Windows11SDK.22621 from .vsconfig."
    }

    return $selectedSdk.Name
}

function Assert-GitLfs {
    $git = Get-Command git.exe -ErrorAction SilentlyContinue
    if (-not $git) {
        throw 'Git was not found on PATH.'
    }

    & $git.Source lfs version *> $null
    if ($LASTEXITCODE -ne 0) {
        throw 'Git LFS is required. Install it, run git lfs install, and fetch the repository LFS objects.'
    }

    $trackedLfsFiles = & $git.Source lfs ls-files --name-only
    if ($LASTEXITCODE -ne 0) {
        throw 'Unable to inspect Git LFS state.'
    }
    if ($trackedLfsFiles) {
        & $git.Source lfs fsck
        if ($LASTEXITCODE -ne 0) {
            throw 'One or more required Git LFS objects are missing or corrupt. Run git lfs pull and retry.'
        }
    }
}

function Invoke-Checked {
    param(
        [string]$FilePath,
        [string[]]$Arguments
    )

    Write-Host "> $FilePath $($Arguments -join ' ')" -ForegroundColor Cyan
    & $FilePath @Arguments
    if ($LASTEXITCODE -ne 0) {
        throw "Command failed with exit code ${LASTEXITCODE}: $FilePath"
    }
}

function Assert-PackagingInputs {
    $requiredMaps = @(
        (Join-Path $projectRoot 'Content\Maps\L_Testbed.umap'),
        (Join-Path $projectRoot 'Content\Maps\L_VerticalSlice.umap')
    )
    $missingMaps = $requiredMaps | Where-Object { -not (Test-Path -LiteralPath $_ -PathType Leaf) }
    if ($missingMaps) {
        throw "M1 bootstrap maps are missing. Create and commit L_Testbed and L_VerticalSlice through Unreal Editor before cooking or packaging.`n  - $($missingMaps -join "`n  - ")"
    }

    $defaultEngine = Get-Content -LiteralPath (Join-Path $projectRoot 'Config\DefaultEngine.ini') -Raw
    if ($defaultEngine -notmatch '(?m)^GameDefaultMap=/Game/Maps/L_VerticalSlice\s*$') {
        throw 'GameDefaultMap must be /Game/Maps/L_VerticalSlice before cooking or packaging.'
    }
}

function Assert-AutomationReportPassed {
    param([string]$ReportDirectory)

    $reportPath = Join-Path $ReportDirectory 'index.json'
    if (-not (Test-Path -LiteralPath $reportPath -PathType Leaf)) {
        throw "Automation completed without creating the expected report: $reportPath"
    }

    try {
        $report = Get-Content -LiteralPath $reportPath -Raw -Encoding UTF8 | ConvertFrom-Json
    }
    catch {
        throw "Automation report is not valid JSON: $reportPath ($($_.Exception.Message))"
    }

    $failedProperty = $report.PSObject.Properties | Where-Object { $_.Name -ieq 'failed' } | Select-Object -First 1
    $succeededProperty = $report.PSObject.Properties | Where-Object { $_.Name -ieq 'succeeded' } | Select-Object -First 1
    if (-not $failedProperty -or -not $succeededProperty) {
        throw "Automation report schema is unsupported; expected succeeded/failed counts in $reportPath"
    }

    $failed = [int]$failedProperty.Value
    $succeeded = [int]$succeededProperty.Value
    $warningProperty = $report.PSObject.Properties | Where-Object { $_.Name -ieq 'succeededWithWarnings' } | Select-Object -First 1
    $succeededWithWarnings = if ($warningProperty) { [int]$warningProperty.Value } else { 0 }
    foreach ($unfinishedState in @('notRun', 'inProcess')) {
        $unfinishedProperty = $report.PSObject.Properties | Where-Object { $_.Name -ieq $unfinishedState } | Select-Object -First 1
        if (-not $unfinishedProperty -or [int]$unfinishedProperty.Value -ne 0) {
            throw "Automation report must contain zero $unfinishedState tests. Inspect $reportPath"
        }
    }
    if ($failed -gt 0) {
        throw "Automation reported $failed failed test(s). Inspect $reportPath"
    }
    if (($succeeded + $succeededWithWarnings) -lt 1) {
        throw "Automation did not execute any successful RomanEmpireGame tests. Inspect $reportPath"
    }

    Write-Host "Automation report passed: $($succeeded + $succeededWithWarnings) succeeded ($succeededWithWarnings with warnings), $failed failed." -ForegroundColor Green
}

$repositoryValidator = Join-Path $projectRoot 'Tools\validate_repository.ps1'
$powerShell = (Get-Command powershell.exe -ErrorAction Stop).Source
Invoke-Checked -FilePath $powerShell -Arguments @(
    '-NoProfile', '-ExecutionPolicy', 'Bypass',
    '-File', $repositoryValidator
)

$resolvedEngineRoot = Find-EngineRoot -ExplicitRoot $EngineRoot -RequiredVersion $requiredEngineVersion
$visualStudioRoot = Assert-VisualStudioToolchain
$windowsSdkVersion = Assert-WindowsSdk -ResolvedEngineRoot $resolvedEngineRoot
Assert-GitLfs

$buildBatch = Join-Path $resolvedEngineRoot 'Engine\Build\BatchFiles\Build.bat'
$runUat = Join-Path $resolvedEngineRoot 'Engine\Build\BatchFiles\RunUAT.bat'
$editorCommand = Join-Path $resolvedEngineRoot 'Engine\Binaries\Win64\UnrealEditor-Cmd.exe'
$buildExecutorArguments = if ($UseBuildAcceleration) { @() } else { @('-NoUBA') }
$uatBuildArguments = if ($UseBuildAcceleration) { @() } else { @('-UbtArgs=-NoUBA') }

Write-Host 'Roman Empire Game prerequisites are valid.' -ForegroundColor Green
Write-Host "  Project:       $projectFile"
Write-Host "  Engine:        $resolvedEngineRoot (UE $requiredEngineVersion)"
Write-Host "  Visual Studio: $visualStudioRoot"
Write-Host "  Windows SDK:   $windowsSdkVersion"
Write-Host "  Action:        $Action"
Write-Host "  IDE:           $IDE"
Write-Host "  UBA:           $(if ($UseBuildAcceleration) { 'Enabled (explicit opt-in)' } else { 'Disabled (safe default)' })"

switch ($Action) {
    'Validate' {
        return
    }
    'Generate' {
        $projectFileArguments = @(
            '-ProjectFiles',
            "-Project=$projectFile",
            '-Game',
            '-Engine',
            '-Progress'
        )
        if ($IDE -eq 'VisualStudioCode') {
            $projectFileArguments += '-VSCode'
        }
        Invoke-Checked -FilePath $buildBatch -Arguments $projectFileArguments

        $generatedSearchIgnore = Join-Path $projectRoot '.ignore'
        if (Test-Path -LiteralPath $generatedSearchIgnore -PathType Leaf) {
            Remove-Item -LiteralPath $generatedSearchIgnore -Force
            Write-Host 'Removed UBT-generated .ignore so repository audits can inspect Content, Tools, scripts, and agent context.' -ForegroundColor Green
        }
    }
    'Editor' {
        Invoke-Checked -FilePath $buildBatch -Arguments (@(
            'RomanEmpireGameEditor', 'Win64', $Configuration,
            "-Project=$projectFile", '-WaitMutex', '-Progress'
        ) + $buildExecutorArguments)
    }
    'IWYU' {
        Invoke-Checked -FilePath $buildBatch -Arguments (@(
            'RomanEmpireGameEditor', 'Win64', 'Development',
            "-Project=$projectFile", '-WaitMutex', '-Progress',
            '-DisableUnity', '-NoPCH'
        ) + $buildExecutorArguments)
    }
    'Game' {
        Invoke-Checked -FilePath $buildBatch -Arguments (@(
            'RomanEmpireGame', 'Win64', $Configuration,
            "-Project=$projectFile", '-WaitMutex', '-Progress'
        ) + $buildExecutorArguments)
    }
    'Test' {
        if (-not (Test-Path -LiteralPath $editorCommand -PathType Leaf)) {
            throw "UnrealEditor-Cmd.exe was not found at $editorCommand"
        }

        Invoke-Checked -FilePath $buildBatch -Arguments (@(
            'RomanEmpireGameEditor', 'Win64', 'Development',
            "-Project=$projectFile", '-WaitMutex', '-Progress'
        ) + $buildExecutorArguments)

        $runStamp = Get-Date -Format 'yyyyMMdd_HHmmss_fff'
        $reportDirectory = Join-Path $projectRoot "Artifacts\Automation\$runStamp"
        New-Item -ItemType Directory -Path $reportDirectory -Force | Out-Null
        Invoke-Checked -FilePath $editorCommand -Arguments (@(
            $projectFile,
            '-unattended', '-nop4', '-NullRHI',
            '-ExecCmds=Automation RunTest RomanEmpireGame;Quit',
            "-ReportExportPath=$reportDirectory",
            '-stdout', '-FullStdOutLogOutput', '-log'
        ) + $editorStartupArguments)
        Assert-AutomationReportPassed -ReportDirectory $reportDirectory
    }
    'Cook' {
        Assert-PackagingInputs
        Invoke-Checked -FilePath $runUat -Arguments (@(
            'BuildCookRun',
            "-project=$projectFile",
            '-nop4', '-build', '-cook', '-skipstage',
            '-platform=Win64',
            "-clientconfig=$Configuration",
            '-utf8output'
        ) + $uatBuildArguments + $cookerStartupArguments)
    }
    'Package' {
        Assert-PackagingInputs
        if ([string]::IsNullOrWhiteSpace($ArchiveDirectory)) {
            $ArchiveDirectory = Join-Path $projectRoot "Artifacts\Packages\$Configuration"
        }
        $resolvedArchiveDirectory = [System.IO.Path]::GetFullPath($ArchiveDirectory)
        New-Item -ItemType Directory -Path $resolvedArchiveDirectory -Force | Out-Null

        Invoke-Checked -FilePath $runUat -Arguments (@(
            'BuildCookRun',
            "-project=$projectFile",
            '-nop4', '-build', '-cook', '-stage', '-package', '-pak', '-archive',
            '-platform=Win64',
            "-clientconfig=$Configuration",
            "-archivedirectory=$resolvedArchiveDirectory",
            '-utf8output'
        ) + $uatBuildArguments + $cookerStartupArguments)
    }
}
