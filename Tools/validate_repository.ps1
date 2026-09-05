[CmdletBinding()]
param()

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

$repositoryRoot = Split-Path -Parent $PSScriptRoot
$failures = [System.Collections.Generic.List[string]]::new()
$warnings = [System.Collections.Generic.List[string]]::new()

function Add-Failure {
    param([string]$Message)
    $script:failures.Add($Message)
}

function Assert-True {
    param(
        [bool]$Condition,
        [string]$Message
    )
    if (-not $Condition) {
        Add-Failure -Message $Message
    }
}

function Read-Utf8 {
    param([string]$Path)
    return Get-Content -LiteralPath $Path -Raw -Encoding UTF8
}

$requiredFiles = @(
    '.editorconfig',
    '.gitattributes',
    'ASSET_INVENTORY.md',
    'BuildProject.ps1',
    'IMPLEMENTATION_PLAN.md',
    'LICENSE.txt',
    'M1_REVIEW.md',
    'PRODUCT_BRIEF.md',
    'THIRD_PARTY_NOTICES.md',
    '.agents\ue-project-context.md',
    'Source\RomanEmpireGame\Public\Faction\FactionData.h',
    'Source\RomanEmpireGameTests\RomanEmpireGameTests.Build.cs',
    'Source\RomanEmpireGameTests\Private\ContentPathValidationTests.cpp',
    'Source\RomanEmpireGameTests\Private\FactionResourceRulesTests.cpp'
)

foreach ($relativePath in $requiredFiles) {
    Assert-True -Condition (Test-Path -LiteralPath (Join-Path $repositoryRoot $relativePath) -PathType Leaf) `
        -Message "Required repository file is missing: $relativePath"
}

$projectPath = Join-Path $repositoryRoot 'RomanEmpireGame.uproject'
try {
    $project = Read-Utf8 -Path $projectPath | ConvertFrom-Json
}
catch {
    Add-Failure -Message "RomanEmpireGame.uproject is invalid JSON: $($_.Exception.Message)"
    $project = $null
}

if ($project) {
    Assert-True -Condition ($project.EngineAssociation -eq '5.8') `
        -Message 'RomanEmpireGame.uproject must remain associated with UE 5.8 during M1 recovery.'

    $runtimeModule = $project.Modules | Where-Object { $_.Name -eq 'RomanEmpireGame' -and $_.Type -eq 'Runtime' }
    $testModule = $project.Modules | Where-Object { $_.Name -eq 'RomanEmpireGameTests' -and $_.Type -eq 'Editor' }
    Assert-True -Condition ($null -ne $runtimeModule) -Message 'The RomanEmpireGame runtime module declaration is missing.'
    Assert-True -Condition ($null -ne $testModule) -Message 'RomanEmpireGameTests must remain an Editor-only module.'

    foreach ($pluginName in @('RemoteControl', 'RemoteControlWebInterface', 'AutomationDriver', 'Landmass', 'AISupport')) {
        $plugin = $project.Plugins | Where-Object { $_.Name -eq $pluginName }
        Assert-True -Condition ($null -ne $plugin -and $plugin.Enabled -eq $false) `
            -Message "$pluginName must be explicitly disabled in normal project defaults."
    }

    foreach ($pluginName in @('ModelContextProtocol', 'EditorToolset', 'AutomationTestToolset')) {
        $plugin = $project.Plugins | Where-Object { $_.Name -eq $pluginName }
        Assert-True -Condition ($null -ne $plugin -and $plugin.Enabled -eq $true) `
            -Message "$pluginName must be enabled for the Unreal Editor implementation workflow."
        Assert-True -Condition ($null -ne $plugin -and 'Editor' -in $plugin.TargetAllowList) `
            -Message "$pluginName must be restricted to the Editor target."
    }

    $allToolsets = $project.Plugins | Where-Object { $_.Name -eq 'AllToolsets' }
    Assert-True -Condition ($null -ne $allToolsets -and $allToolsets.Enabled -eq $false) `
        -Message 'AllToolsets must remain disabled; enable only the narrowly required editor toolsets.'
}

$defaultEngine = Read-Utf8 -Path (Join-Path $repositoryRoot 'Config\DefaultEngine.ini')
Assert-True -Condition ($defaultEngine -match '(?m)^bAutoStartServer=False\s*$') `
    -Message 'Remote Control auto-start must remain disabled.'
Assert-True -Condition ($defaultEngine -match '(?m)^bAllowNetworkConnection=False\s*$') `
    -Message 'Android File Server network access must remain disabled for the Windows-only slice.'
Assert-True -Condition ($defaultEngine -match '(?m)^GameDefaultMap=/Game/Maps/L_VerticalSlice\s*$') `
    -Message 'GameDefaultMap must point to the packaged vertical-slice map.'
Assert-True -Condition ($defaultEngine -match '(?m)^EditorStartupMap=/Game/Maps/L_Testbed\s*$') `
    -Message 'EditorStartupMap must point to the deterministic testbed map.'
Assert-True -Condition ($defaultEngine -match '(?m)^GlobalDefaultGameMode=/Script/RomanEmpireGame\.RomanEmpireGameMode\s*$') `
    -Message 'GlobalDefaultGameMode must explicitly use ARomanEmpireGameMode.'

$defaultGame = Read-Utf8 -Path (Join-Path $repositoryRoot 'Config\DefaultGame.ini')
Assert-True -Condition ($defaultGame -match '(?m)^bEnabled=False\s*$') `
    -Message 'Supabase must remain explicitly disabled for the offline slice.'
Assert-True -Condition ($defaultGame -notmatch 'YOUR_PROJECT|YOUR_ANON|YOUR_KEY') `
    -Message 'Placeholder online credentials must not masquerade as configured values.'

$gitIgnore = Read-Utf8 -Path (Join-Path $repositoryRoot '.gitignore')
Assert-True -Condition ($gitIgnore -notmatch '(?m)^\s*\*\.uasset\s*$') `
    -Message '.gitignore must not exclude Unreal .uasset source files.'
Assert-True -Condition ($gitIgnore -notmatch '(?m)^\s*\*\.umap\s*$') `
    -Message '.gitignore must not exclude Unreal .umap source files.'
Assert-True -Condition ($gitIgnore -match '(?m)^\s*\*\.sln\s*$') `
    -Message '.gitignore must exclude generated Visual Studio solution files.'

$trackedSolutionsPresent = @(
    & git -C $repositoryRoot ls-files '*.sln' 2>$null |
        Where-Object { Test-Path -LiteralPath (Join-Path $repositoryRoot $_) -PathType Leaf }
)
Assert-True -Condition ($trackedSolutionsPresent.Count -eq 0) `
    -Message "Tracked generated solution files remain in the working tree: $($trackedSolutionsPresent -join ', ')"
Assert-True -Condition (-not (Test-Path -LiteralPath (Join-Path $repositoryRoot '.ignore') -PathType Leaf)) `
    -Message 'A root .ignore file must not hide source, Content, Tools, or agent context from repository audits.'

$gitAttributes = Read-Utf8 -Path (Join-Path $repositoryRoot '.gitattributes')
foreach ($pattern in @('*.uasset', '*.umap')) {
    $escapedPattern = [regex]::Escape($pattern)
    Assert-True -Condition ($gitAttributes -match "(?m)^$escapedPattern\s+filter=lfs\s+diff=lfs\s+merge=lfs\s+-text\s*$") `
        -Message ".gitattributes is missing the required Git LFS rule for $pattern."
}

$vsConfigPath = Join-Path $repositoryRoot '.vsconfig'
try {
    $vsConfig = Read-Utf8 -Path $vsConfigPath | ConvertFrom-Json
    foreach ($component in @(
        'Microsoft.VisualStudio.Component.VC.Tools.x86.x64',
        'Microsoft.VisualStudio.Component.VC.14.44.17.14.x86.x64',
        'Microsoft.VisualStudio.Component.VC.14.44.17.14.ATL',
        'Microsoft.Net.Component.4.8.SDK',
        'Microsoft.VisualStudio.Component.Windows11SDK.22621'
    )) {
        Assert-True -Condition ($component -in $vsConfig.components) `
            -Message ".vsconfig is missing required component: $component"
    }
}
catch {
    Add-Failure -Message ".vsconfig is invalid JSON: $($_.Exception.Message)"
}

$runtimeRoot = Join-Path $repositoryRoot 'Source\RomanEmpireGame'
$runtimeBuildRules = Read-Utf8 -Path (Join-Path $runtimeRoot 'RomanEmpireGame.Build.cs')
Assert-True -Condition ($runtimeBuildRules -match 'IWYUSupport\s*=\s*IWYUSupport\.Full') `
    -Message 'The runtime module must enforce IWYU during M1 recovery.'
Assert-True -Condition ($runtimeBuildRules -notmatch 'IWYUSupport\.None') `
    -Message 'The runtime module must not disable IWYU support.'
Assert-True -Condition ($runtimeBuildRules -match 'PrivateIncludePaths\.Add\(ModuleDirectory\)') `
    -Message 'The legacy module-root include path must remain private to the runtime module.'
Assert-True -Condition ($runtimeBuildRules -notmatch 'PublicIncludePaths\.Add\(ModuleDirectory\)') `
    -Message 'The runtime module root must not be exported as a public include path.'

$testBuildRules = Read-Utf8 -Path (Join-Path $repositoryRoot 'Source\RomanEmpireGameTests\RomanEmpireGameTests.Build.cs')
Assert-True -Condition ($testBuildRules -match 'IWYUSupport\s*=\s*IWYUSupport\.Full') `
    -Message 'The test module must enforce IWYU during M1 recovery.'
Assert-True -Condition ($testBuildRules -notmatch 'IWYUSupport\.None') `
    -Message 'The test module must not disable IWYU support.'

$testSource = Get-ChildItem -LiteralPath (Join-Path $repositoryRoot 'Source\RomanEmpireGameTests') -Recurse -File -Include '*.h', '*.cpp'
foreach ($file in $testSource) {
    $text = Read-Utf8 -Path $file.FullName
    if ($text -match '(?m)^#include\s+"(?:Assets|Audio|Building|Camera|Core|Online|UI|Units|World)/') {
        Add-Failure -Message "The test module may not import private runtime headers: $($file.FullName)"
    }
}

$factionDataHeader = Read-Utf8 -Path (Join-Path $runtimeRoot 'Public\Faction\FactionData.h')
foreach ($resourceType in @(
    'FFactionResourceAmounts',
    'FFactionResourceDelta',
    'FFactionResourceTransactionResult'
)) {
    Assert-True -Condition ($factionDataHeader -match "struct\s+ROMANEMPIREGAME_API\s+$resourceType") `
        -Message "$resourceType must remain exported from the runtime module for native tests."
}
Assert-True -Condition ($factionDataHeader -notmatch 'struct\s+ROMANEMPIREGAME_API\s+FFactionResources\b') `
    -Message 'The overloaded FFactionResources type must not be reintroduced.'

$runtimeSource = Get-ChildItem -LiteralPath $runtimeRoot -Recurse -File -Include '*.h', '*.cpp', '*.Build.cs'
foreach ($file in $runtimeSource) {
    $text = Read-Utf8 -Path $file.FullName
    if ($text -match 'AutomationDriver|WebRemoteControl|ModelContextProtocol|AllToolsets') {
        Add-Failure -Message "Editor/test tooling leaked into the runtime module: $($file.FullName)"
    }

    if ($file.Extension -in @('.h', '.cpp') -and $text -match '(?m)^#include\s+"\.\./') {
        Add-Failure -Message "Use module-root include paths instead of parent-relative includes: $($file.FullName)"
    }

    if ($file.Extension -eq '.cpp' -and $text -match '\bGEngine\b' -and
        $text -notmatch '(?m)^#include\s+"Engine/Engine\.h"') {
        Add-Failure -Message "GEngine requires an explicit Engine/Engine.h include: $($file.FullName)"
    }

    if ($file.Extension -eq '.cpp' -and $text -match '(GetWorld\(\)->|\bUWorld\s*\*|SpawnActor\s*<)' -and
        $text -notmatch '(?m)^#include\s+"Engine/World\.h"') {
        Add-Failure -Message "World access requires an explicit Engine/World.h include: $($file.FullName)"
    }

    if ($file.Extension -eq '.cpp' -and $text -match '\bFCollisionQueryParams\b' -and
        $text -notmatch '(?m)^#include\s+"CollisionQueryParams\.h"') {
        Add-Failure -Message "FCollisionQueryParams requires its documented direct include: $($file.FullName)"
    }

    if ($file.Extension -eq '.cpp' -and $text -match '\bEKeys::' -and
        $text -notmatch '(?m)^#include\s+"InputCoreTypes\.h"') {
        Add-Failure -Message "EKeys requires its documented direct include: $($file.FullName)"
    }
}

$unitHeader = Read-Utf8 -Path (Join-Path $runtimeRoot 'Units\UnitBase.h')
foreach ($accessor in @('GetCurrentHealth', 'GetMaxHealth', 'GetCurrentMorale')) {
    $count = [regex]::Matches($unitHeader, "\b$accessor\s*\(\s*\)\s*const").Count
    Assert-True -Condition ($count -eq 1) -Message "UnitBase.h must declare $accessor exactly once; found $count."
}

$buildingHeader = Read-Utf8 -Path (Join-Path $runtimeRoot 'Building\BuildingBase.h')
$buildingSetterCount = [regex]::Matches($buildingHeader, '\bSetBuildingTypeData\s*\(').Count
Assert-True -Condition ($buildingSetterCount -eq 1) `
    -Message "BuildingBase.h must declare SetBuildingTypeData exactly once; found $buildingSetterCount."

$hudSource = Read-Utf8 -Path (Join-Path $runtimeRoot 'Core\RomanEmpireHUD.cpp')
Assert-True -Condition ($hudSource -match 'void\s+ARomanEmpireHUD::ShowFPSOverlay\s*\(') `
    -Message 'ARomanEmpireHUD::ShowFPSOverlay is declared but not implemented.'

$gameModeSource = Read-Utf8 -Path (Join-Path $runtimeRoot 'Core\RomanEmpireGameMode.cpp')
Assert-True -Condition ($gameModeSource -notmatch 'HUD->DisplayGold') `
    -Message 'GameMode may not mutate protected HUD resource presentation state directly.'
Assert-True -Condition ($gameModeSource -notmatch '\b(?:GetPlayerGold|SubtractGold|UpdateHUDResources)\b') `
    -Message 'GameMode may not reintroduce a parallel player-resource authority.'
foreach ($assignment in @(
    'DefaultPawnClass\s*=\s*ASeamlessZoomCamera::StaticClass\(\)',
    'PlayerControllerClass\s*=\s*ARomanEmpirePlayerController::StaticClass\(\)',
    'GameStateClass\s*=\s*AGameStateBase::StaticClass\(\)',
    'HUDClass\s*=\s*ARomanEmpireHUD::StaticClass\(\)'
)) {
    Assert-True -Condition ($gameModeSource -match $assignment) `
        -Message "RomanEmpireGameMode must explicitly assign its gameplay framework classes: $assignment"
}

$controllerSource = Read-Utf8 -Path (Join-Path $runtimeRoot 'Core\RomanEmpirePlayerController.cpp')
Assert-True -Condition ($controllerSource -notmatch 'REHUD->HUDSelectedBuilding') `
    -Message 'PlayerController must use the HUD public building-selection API.'
Assert-True -Condition ($controllerSource -notmatch '\b(?:TrainingProgress|TrainingTimeRemaining|UnitBeingTrained)\b') `
    -Message 'PlayerController may not own duplicate unit-training state.'
Assert-True -Condition ($controllerSource -match 'if\s*\(\s*BuildingPlacementComponent->ConfirmPlacement\(\)\s*\)\s*\{\s*CurrentSelectionMode\s*=\s*ERomanSelectionMode::None') `
    -Message 'Building placement mode must clear only after ConfirmPlacement succeeds.'

$placementSource = Read-Utf8 -Path (Join-Path $runtimeRoot 'Building\BuildingPlacementComponent.cpp')
$placementSpendCount = [regex]::Matches($placementSource, '\bTrySpendFactionResources\s*\(').Count
Assert-True -Condition ($placementSpendCount -eq 1) `
    -Message "Building placement must perform exactly one canonical spend call; found $placementSpendCount."

$barracksSource = Read-Utf8 -Path (Join-Path $runtimeRoot 'Building\Barracks.cpp')
$trainingSpendCount = [regex]::Matches($barracksSource, '\bTrySpendFactionResources\s*\(').Count
Assert-True -Condition ($trainingSpendCount -eq 1) `
    -Message "Barracks training acceptance must perform exactly one canonical spend call; found $trainingSpendCount."

$supabaseSource = Read-Utf8 -Path (Join-Path $runtimeRoot 'Online\SupabaseSubsystem.cpp')
Assert-True -Condition ($supabaseSource -notmatch 'LogRomanEmpireGame') `
    -Message 'SupabaseSubsystem uses a nonexistent log category.'

Get-ChildItem -LiteralPath (Join-Path $repositoryRoot 'Source') -Recurse -File -Filter '*.h' | ForEach-Object {
    $includeLines = Select-String -LiteralPath $_.FullName -Pattern '^#include' -Encoding UTF8
    $generatedInclude = $includeLines | Where-Object { $_.Line -match '\.generated\.h"' } | Select-Object -Last 1
    if ($generatedInclude -and $includeLines[-1].LineNumber -ne $generatedInclude.LineNumber) {
        Add-Failure -Message "Generated header is not the final include: $($_.FullName):$($generatedInclude.LineNumber)"
    }
}

Get-ChildItem -LiteralPath $repositoryRoot -Recurse -File -Filter '*.json' |
    Where-Object { $_.FullName -notmatch '\\(Binaries|DerivedDataCache|Intermediate|Saved|Artifacts)\\' } |
    ForEach-Object {
        try {
            Read-Utf8 -Path $_.FullName | ConvertFrom-Json | Out-Null
        }
        catch {
            Add-Failure -Message "Invalid JSON file $($_.FullName): $($_.Exception.Message)"
        }
    }

foreach ($scriptPath in @(
    (Join-Path $repositoryRoot 'BuildProject.ps1'),
    (Join-Path $repositoryRoot 'Tools\validate_repository.ps1')
)) {
    $tokens = $null
    $parseErrors = $null
    [void][System.Management.Automation.Language.Parser]::ParseFile($scriptPath, [ref]$tokens, [ref]$parseErrors)
    foreach ($parseError in $parseErrors) {
        Add-Failure -Message "PowerShell parse error in ${scriptPath}: $($parseError.Message)"
    }
}

$buildEntrypoint = Read-Utf8 -Path (Join-Path $repositoryRoot 'BuildProject.ps1')
Assert-True -Condition ($buildEntrypoint -match '\[switch\]\$UseBuildAcceleration') `
    -Message 'Build acceleration must remain an explicit opt-in for workstation policy compatibility.'
Assert-True -Condition ($buildEntrypoint -match "@\('-NoUBA'\)") `
    -Message 'Direct UBT actions must disable UBA by default.'
Assert-True -Condition ($buildEntrypoint -match "@\('-UbtArgs=-NoUBA'\)") `
    -Message 'UAT cook/package actions must pass the safe UBT executor default.'
Assert-True -Condition ($buildEntrypoint -match 'Remove-Item\s+-LiteralPath\s+\$generatedSearchIgnore') `
    -Message 'VS Code generation must remove UBT''s repository-hiding .ignore artifact.'

foreach ($requiredMap in @(
    'Content\Maps\L_Testbed.umap',
    'Content\Maps\L_VerticalSlice.umap'
)) {
    if (-not (Test-Path -LiteralPath (Join-Path $repositoryRoot $requiredMap) -PathType Leaf)) {
        $warnings.Add("M1 bootstrap asset remains open: $requiredMap")
    }
}

if ($warnings.Count -gt 0) {
    Write-Host 'Repository validation warnings:' -ForegroundColor Yellow
    foreach ($warning in $warnings) {
        Write-Host "  - $warning" -ForegroundColor Yellow
    }
}

if ($failures.Count -gt 0) {
    Write-Host 'Repository validation failed:' -ForegroundColor Red
    foreach ($failure in $failures) {
        Write-Host "  - $failure" -ForegroundColor Red
    }
    exit 1
}

Write-Host "Repository validation passed with $($warnings.Count) open M1 warning(s)." -ForegroundColor Green
exit 0
