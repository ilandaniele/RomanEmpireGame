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
    'Source\RomanEmpireGame\Public\Camera\ZoomConfiguration.h',
    'Source\RomanEmpireGame\Public\Campaign\CampaignTurn.h',
    'Source\RomanEmpireGame\Public\Campaign\TerritoryData.h',
    'Source\RomanEmpireGame\Public\Building\BuildingTypes.h',
    'Source\RomanEmpireGame\Public\Building\ProductionEvent.h',
    'Source\RomanEmpireGame\Public\Campaign\CampaignObjectiveState.h',
    'Source\RomanEmpireGame\Public\Content\RomanContentDefinitions.h',
    'Source\RomanEmpireGame\Public\Content\RomanContentRegistry.h',
    'Source\RomanEmpireGame\Public\Core\GameplayMode.h',
    'Source\RomanEmpireGame\Public\Faction\DiplomacyState.h',
    'Source\RomanEmpireGame\Public\Faction\FactionData.h',
    'Source\RomanEmpireGame\Public\Units\UnitTypes.h',
    'Source\RomanEmpireGameTests\RomanEmpireGameTests.Build.cs',
    'Source\RomanEmpireGameTests\Private\CampaignStateRulesTests.cpp',
    'Source\RomanEmpireGameTests\Private\CanonicalUIIntegrationTests.cpp',
    'Source\RomanEmpireGameTests\Private\ContentDefinitionValidationTests.cpp',
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
foreach ($assetType in @('RomanFaction', 'RomanUnit', 'RomanBuilding', 'RomanTerritory')) {
    $assetTypePattern = 'PrimaryAssetType="' + [regex]::Escape($assetType) + '"'
    Assert-True -Condition ($defaultGame -match $assetTypePattern) `
        -Message "DefaultGame.ini must scan the $assetType Primary Data Asset category."
}

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
    foreach ($includeMatch in [regex]::Matches($text, '(?m)^#include\s+"([^"]+)"')) {
        $includePath = $includeMatch.Groups[1].Value.Replace('/', '\')
        $privateCandidate = Join-Path $runtimeRoot $includePath
        $publicCandidate = Join-Path (Join-Path $runtimeRoot 'Public') $includePath
        if ((Test-Path -LiteralPath $privateCandidate -PathType Leaf) -and
            -not (Test-Path -LiteralPath $publicCandidate -PathType Leaf)) {
            Add-Failure -Message "The test module may not import private runtime header '$includePath': $($file.FullName)"
        }
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
$buildingInitializerCount = [regex]::Matches($buildingHeader, '\bInitializeBuilding\s*\(').Count
Assert-True -Condition ($buildingInitializerCount -eq 1) `
    -Message "BuildingBase.h must declare InitializeBuilding exactly once; found $buildingInitializerCount."
Assert-True -Condition ($buildingHeader -notmatch '\bSetBuildingTypeData\s*\(') `
    -Message 'Building types must be initialized from stable content IDs, not mutable enum-only setters.'

$hudSource = Read-Utf8 -Path (Join-Path $runtimeRoot 'Core\RomanEmpireHUD.cpp')
Assert-True -Condition ($hudSource -notmatch '\bCurrentZoomLevel\b') `
    -Message 'HUD gameplay-mode presentation may not infer authority from a local zoom value.'

$gameModeSource = Read-Utf8 -Path (Join-Path $runtimeRoot 'Core\RomanEmpireGameMode.cpp')
Assert-True -Condition ($gameModeSource -notmatch 'HUD->DisplayGold') `
    -Message 'GameMode may not mutate protected HUD resource presentation state directly.'
Assert-True -Condition ($gameModeSource -notmatch '\b(?:GetPlayerGold|SubtractGold|UpdateHUDResources)\b') `
    -Message 'GameMode may not reintroduce a parallel player-resource authority.'
foreach ($assignment in @(
    'DefaultPawnClass\s*=\s*ASeamlessZoomCamera::StaticClass\(\)',
    'PlayerControllerClass\s*=\s*ARomanEmpirePlayerController::StaticClass\(\)',
    'GameStateClass\s*=\s*ARomanEmpireGameState::StaticClass\(\)',
    'HUDClass\s*=\s*ARomanEmpireHUD::StaticClass\(\)'
)) {
    Assert-True -Condition ($gameModeSource -match $assignment) `
        -Message "RomanEmpireGameMode must explicitly assign its gameplay framework classes: $assignment"
}

$controllerSource = Read-Utf8 -Path (Join-Path $runtimeRoot 'Core\RomanEmpirePlayerController.cpp')
Assert-True -Condition ($controllerSource -notmatch 'REHUD|TrainButtonMin|TrainButtonMax') `
    -Message 'PlayerController may not route production UI actions through Canvas or manual screen bounds.'
Assert-True -Condition ($controllerSource -match 'CreateWidget<URomanEmpireMainWidget>') `
    -Message 'PlayerController must create the production UMG root.'
Assert-True -Condition ($controllerSource -notmatch '\b(?:TrainingProgress|TrainingTimeRemaining|UnitBeingTrained)\b') `
    -Message 'PlayerController may not own duplicate unit-training state.'
Assert-True -Condition ($controllerSource -notmatch '\b(?:CurrentZoomLevel|TargetZoomLevel|bIsInFirstPersonMode)\b') `
    -Message 'PlayerController may not own duplicate zoom or gameplay-mode state.'
Assert-True -Condition ($controllerSource -match 'if\s*\(\s*BuildingPlacementComponent->ConfirmPlacement\(\)\s*\)\s*\{\s*CurrentSelectionMode\s*=\s*ERomanSelectionMode::None') `
    -Message 'Building placement mode must clear only after ConfirmPlacement succeeds.'
Assert-True -Condition ($controllerSource -notmatch 'Cost\.Gold\s*=|TEXT\("Building:\s*(?:Barracks|Farm|Mine|Lumber Camp|Wall|Temple)') `
    -Message 'PlayerController may not duplicate building costs or presentation strings from Primary Data Assets.'
Assert-True -Condition ($controllerSource -match 'FindBuildingByQuickSlot\s*\(') `
    -Message 'PlayerController building hotkeys must resolve stable definitions through the content registry.'

$hudHeader = Read-Utf8 -Path (Join-Path $runtimeRoot 'Core\RomanEmpireHUD.h')
Assert-True -Condition (($hudSource + $hudHeader) -notmatch 'EnemiesKilled|bVictory|bDefeated|DrawResourceBar|DrawBuildingPanel|CheckVictoryCondition|TrainButton') `
    -Message 'Canvas HUD must remain diagnostic-only, without gameplay panels or outcome counters.'
Assert-True -Condition ($hudSource -match 'bShowDebugOverlay\s*=\s*false') `
    -Message 'Canvas diagnostics must be disabled by default.'

$mainWidgetSource = Read-Utf8 -Path (Join-Path $runtimeRoot 'UI\RomanEmpireMainWidget.cpp')
Assert-True -Condition ($mainWidgetSource -notmatch 'NativeTick|GetAllActorsOfClass|TrySpendFactionResources') `
    -Message 'Production UMG must observe canonical events and issue commands without polling or spending locally.'
Assert-True -Condition ($mainWidgetSource -match 'GetGameState<ARomanEmpireGameState>' -and
    $mainWidgetSource -notmatch 'GetGameMode|GetFactionManager|GetCampaignManager|GetWorldMapManager') `
    -Message 'Production UMG must consume the public GameState read/event contract instead of resolving private managers.'
foreach ($eventName in @('OnSelectionChanged', 'OnPossessedUnitChanged', 'OnFactionResourcesChanged', 'OnGameplayModeChanged', 'OnTurnProcessed', 'OnProductionChanged', 'OnCampaignObjectivesChanged', 'OnVictory')) {
    Assert-True -Condition ($mainWidgetSource -match ([regex]::Escape($eventName) + '\.AddUniqueDynamic')) `
        -Message "UMG is missing a canonical event binding: $eventName"
    Assert-True -Condition ($mainWidgetSource -match ([regex]::Escape($eventName) + '\.RemoveDynamic')) `
        -Message "UMG must release the canonical event binding at teardown: $eventName"
}
$menuSource = Read-Utf8 -Path (Join-Path $runtimeRoot 'UI\BuildingMenuWidget.cpp')
Assert-True -Condition ($menuSource -match 'GetAllBuildings\s*\(' -and $menuSource -match 'OnBuildingSelected\.Broadcast') `
    -Message 'UMG building buttons must enumerate the content registry and send stable-ID commands.'

$placementSource = Read-Utf8 -Path (Join-Path $runtimeRoot 'Building\BuildingPlacementComponent.cpp')
$placementSpendCount = [regex]::Matches($placementSource, '\bTrySpendFactionResources\s*\(').Count
# This constrains call sites, not execution count: synchronous event reentry is
# covered by the live PIE regression, whose passing report must also be inspected.
Assert-True -Condition ($placementSpendCount -eq 1) `
    -Message "Building placement must contain one canonical spend call site; found $placementSpendCount."
$placementIntegrationTests = Read-Utf8 -Path (Join-Path $repositoryRoot 'Source\RomanEmpireGameTests\Private\CanonicalUIIntegrationTests.cpp')
Assert-True -Condition ($placementIntegrationTests -match 'RomanEmpireGame\.Content\.DefinitionPropagationAndAtomicPlacementPIE') `
    -Message 'The definition-propagation and placement-reentry PIE regression must remain registered.'

$barracksSource = Read-Utf8 -Path (Join-Path $runtimeRoot 'Building\Barracks.cpp')
$trainingSpendCount = [regex]::Matches($barracksSource, '\bTrySpendFactionResources\s*\(').Count
Assert-True -Condition ($trainingSpendCount -eq 1) `
    -Message "Barracks training acceptance must contain one canonical spend call site; found $trainingSpendCount. Live PIE checks execution cost."

$runtimeHeaderText = ($runtimeSource | Where-Object { $_.Extension -eq '.h' } | ForEach-Object {
    Read-Utf8 -Path $_.FullName
}) -join "`n"
$turnCounterCount = [regex]::Matches($runtimeHeaderText, '\bint32\s+CurrentTurn\s*;').Count
Assert-True -Condition ($turnCounterCount -eq 1) `
    -Message "Runtime code must declare exactly one canonical CurrentTurn field; found $turnCounterCount."
Assert-True -Condition ($runtimeHeaderText -notmatch '\bEGamePhase\b|\bbIsInFirstPersonMode\b') `
    -Message 'Legacy zoom-derived phase and duplicate possession-mode state must not be reintroduced.'

$campaignSource = Read-Utf8 -Path (Join-Path $runtimeRoot 'World\CampaignManager.cpp')
Assert-True -Condition ($campaignSource -match 'RomanEmpireCampaign::GetCanonicalTurnPipeline\s*\(') `
    -Message 'CampaignManager must execute the canonical turn pipeline definition.'
foreach ($turnStep in @(
    'ValidateRequest',
    'Production',
    'Upkeep',
    'AIActions',
    'ConflictResolution',
    'VictoryEvaluation',
    'IncrementTurn',
    'BroadcastTurnCompleted',
    'Autosave'
)) {
    $stepCount = [regex]::Matches($campaignSource, "case\s+ECampaignTurnStep::$turnStep\s*:").Count
    Assert-True -Condition ($stepCount -eq 1) `
        -Message "CampaignManager must execute $turnStep exactly once in its step dispatcher; found $stepCount."
}

$zoomCameraSource = Read-Utf8 -Path (Join-Path $runtimeRoot 'Camera\SeamlessZoomCamera.cpp')
Assert-True -Condition ($zoomCameraSource -match 'GetDefaultZoomConfiguration\(\)\.Classify\(CurrentZoomLevel\)') `
    -Message 'SeamlessZoomCamera must classify zoom through the canonical configuration.'
Assert-True -Condition ($runtimeHeaderText -notmatch '\bZOOM_(?:WORLD|TERRITORY|CITY|GROUND|FPS)') `
    -Message 'Legacy parallel zoom thresholds must not be reintroduced.'

$factionManagerHeader = Read-Utf8 -Path (Join-Path $runtimeRoot 'Faction\FactionManager.h')
Assert-True -Condition ($factionManagerHeader -match '\bFDiplomacyState\s+DiplomacyState\s*;') `
    -Message 'FactionManager must own the normalized canonical diplomacy state.'
Assert-True -Condition ($factionManagerHeader -notmatch '\bDiplomacyMatrix\b|\bTerritoryOwnership\b|\bAssignTerritoryToFaction\b|\bGetFactionTerritoryCount\b') `
    -Message 'FactionManager may not reintroduce asymmetric diplomacy or a second territory-ownership store.'

$diplomacyHeader = Read-Utf8 -Path (Join-Path $runtimeRoot 'Public\Faction\DiplomacyState.h')
Assert-True -Condition ($diplomacyHeader -match 'TMap<int32,\s*FDiplomaticRelation>\s+RelationsByPair\s*;') `
    -Message 'Diplomacy state must store each normalized unordered faction pair exactly once.'
Assert-True -Condition ($diplomacyHeader -match '\bNormalizePair\s*\(') `
    -Message 'Diplomacy state must expose one normalized faction-pair contract.'
Assert-True -Condition ($runtimeHeaderText -notmatch 'EDiplomaticStatus::(?:Hostile|Friendly)') `
    -Message 'Diplomacy must use only Invalid, War, Neutral, and Allied canonical statuses.'

$territoryHeader = Read-Utf8 -Path (Join-Path $runtimeRoot 'World\TerritoryRegion.h')
Assert-True -Condition ($territoryHeader -notmatch '\bEFactionID\s+OwnerFaction\s*;|\bSetOwnerFaction\s*\(') `
    -Message 'Territory actors are presentation objects and may not own or directly set campaign ownership.'
$territoryOwnershipStoreCount = [regex]::Matches(
    $runtimeHeaderText,
    'TMap<FName,\s*EFactionID>\s+TerritoryOwnership\s*;').Count
Assert-True -Condition ($territoryOwnershipStoreCount -eq 1) `
    -Message "Runtime headers must contain exactly one canonical territory-ownership store; found $territoryOwnershipStoreCount."

$worldMapSource = Read-Utf8 -Path (Join-Path $runtimeRoot 'World\WorldMapManager.cpp')
Assert-True -Condition ($worldMapSource -match '(?s)SpawnActorDeferred<ATerritoryRegion>.*InitializeTerritory\s*\(.*FinishSpawningActor') `
    -Message 'WorldMapManager must fully initialize territory actors through deferred spawning before they become observable.'
Assert-True -Condition ($worldMapSource -notmatch '\bMakePrototypeTerritoryDefinition\b') `
    -Message 'WorldMapManager may not rebuild Primary Data Asset territory definitions in C++.'
Assert-True -Condition ($worldMapSource -match 'GetAllTerritories\s*\(') `
    -Message 'WorldMapManager must generate the prototype campaign from authored territory assets.'
Assert-True -Condition ($campaignSource -match 'WorldMapManager->GetFactionTerritoryCount\s*\(') `
    -Message 'Campaign victory/defeat evaluation must read canonical territory ownership from WorldMapManager.'

$campaignRulesTests = Read-Utf8 -Path (Join-Path $repositoryRoot 'Source\RomanEmpireGameTests\Private\CampaignStateRulesTests.cpp')
foreach ($requiredTest in @(
    'CanonicalTerritoryOwnership',
    'DiplomacySymmetryAndTransitions'
)) {
    Assert-True -Condition ($campaignRulesTests -match [regex]::Escape($requiredTest)) `
        -Message "Campaign state automation coverage is missing: $requiredTest"
}

$contentDefinitionTests = Read-Utf8 -Path (Join-Path $repositoryRoot 'Source\RomanEmpireGameTests\Private\ContentDefinitionValidationTests.cpp')
foreach ($requiredTest in @(
    'ValidationFailures',
    'PrimaryAssetDiscovery',
    'DuplicateStableID',
    'InvalidFactionReference',
    'MissingAssetType'
)) {
    Assert-True -Condition ($contentDefinitionTests -match [regex]::Escape($requiredTest)) `
        -Message "Content-definition automation coverage is missing: $requiredTest"
}

foreach ($removedDefinitionFactory in @(
    'GetPrototypeStartingResources',
    'MakePrototypeTerritoryDefinition',
    'SetBuildingTypeData'
)) {
    $definitionFactoryMatches = @(
        $runtimeSource.FullName |
            ForEach-Object { Read-Utf8 -Path $_ } |
            Where-Object { $_ -match "\b$removedDefinitionFactory\b" }
    )
    Assert-True -Condition ($definitionFactoryMatches.Count -eq 0) `
        -Message "Legacy hard-coded definition factory remains in runtime source: $removedDefinitionFactory"
}

$requiredDefinitionAssets = @(
    'Content\Data\Factions\PDA_Faction_Rome.uasset',
    'Content\Data\Factions\PDA_Faction_Carthage.uasset',
    'Content\Data\Factions\PDA_Faction_Gaul.uasset',
    'Content\Data\Units\PDA_Unit_Legionary.uasset',
    'Content\Data\Buildings\PDA_Building_Barracks.uasset',
    'Content\Data\Buildings\PDA_Building_Farm.uasset',
    'Content\Data\Buildings\PDA_Building_Mine.uasset',
    'Content\Data\Buildings\PDA_Building_Lumbercamp.uasset',
    'Content\Data\Buildings\PDA_Building_Wall.uasset',
    'Content\Data\Buildings\PDA_Building_Temple.uasset',
    'Content\Data\Territories\PDA_Territory_Gaul.uasset',
    'Content\Data\Territories\PDA_Territory_Alps.uasset',
    'Content\Data\Territories\PDA_Territory_Greece.uasset',
    'Content\Data\Territories\PDA_Territory_Hispania.uasset',
    'Content\Data\Territories\PDA_Territory_Rome.uasset',
    'Content\Data\Territories\PDA_Territory_Macedonia.uasset',
    'Content\Data\Territories\PDA_Territory_Africa.uasset',
    'Content\Data\Territories\PDA_Territory_Sicily.uasset',
    'Content\Data\Territories\PDA_Territory_Egypt.uasset'
)
foreach ($relativePath in $requiredDefinitionAssets) {
    Assert-True -Condition (Test-Path -LiteralPath (Join-Path $repositoryRoot $relativePath) -PathType Leaf) `
        -Message "Required M2.4 Primary Data Asset is missing: $relativePath"
}

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
