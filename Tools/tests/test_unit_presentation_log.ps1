[CmdletBinding()]
param()

# Pure parser regression checks. All game/controller/unit identities below are
# synthetic. This script never launches a game, builds, or certifies pixels.
Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
$helperPath = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../verify_unit_presentation.ps1'))
$parseTokens = $null
$parseErrors = $null
$ast = [Management.Automation.Language.Parser]::ParseFile($helperPath, [ref]$parseTokens, [ref]$parseErrors)
if ($parseErrors.Count) { throw ($parseErrors | Out-String) }
$definitions = @($ast.FindAll({
    param($node)
    $node -is [Management.Automation.Language.FunctionDefinitionAst] -and $node.Name -eq 'Assert-PresentationLog'
}, $true))
if ($definitions.Count -ne 1) { throw 'Expected exactly one Assert-PresentationLog function.' }
# Evaluate only the function definition, never the helper's player-launch body.
. ([scriptblock]::Create($definitions[0].Extent.Text))

$controller = '/Game/Synthetic.Synthetic:PersistentLevel.Controller_1'
$unit = '/Game/Synthetic.Synthetic:PersistentLevel.Unit_1'
$otherUnit = '/Game/Synthetic.Synthetic:PersistentLevel.Unit_2'
$unitRef = "/Script/RomanEmpireGame.Legionary'$unit'"
$cameraRef = "/Script/RomanEmpireGame.SeamlessZoomCamera'/Game/Synthetic.Synthetic:PersistentLevel.Camera_1'"
$cameraLine = "0) RomanEmpirePlayerController $controller.Pawn = $cameraRef"
$selectionHeader = "0) RomanEmpirePlayerController $controller.SelectedUnits ="
$memberLine = "    0: $unitRef"
$unitFlag = "0) Legionary $unit.bIsPossessedByPlayer"
$otherFlag = "1) Legionary $otherUnit.bIsPossessedByPlayer"
$fixture = @"
$cameraLine
LogRomanEmpire: Display: Presentation selected $unit at X=0 Y=0 Z=0 (selected=1)
$selectionHeader
$memberLine
LogRomanEmpire: Entered FPS mode with unit: Unit_1
0) RomanEmpirePlayerController $controller.PossessedUnit = $unitRef
0) RomanEmpirePlayerController $controller.Pawn = $unitRef
$unitFlag = True
$otherFlag = False
LogRomanEmpire: Exited FPS mode - camera restored
0) RomanEmpirePlayerController $controller.PossessedUnit = None
$selectionHeader
$cameraLine
$unitFlag = False
$otherFlag = False
"@
$fixture = $fixture.Replace("`r`n", "`n")
$fixture += "`n"
Assert-PresentationLog $fixture
Assert-PresentationLog $fixture.Replace("`n", "`r`n")
$prefixed = (($fixture.TrimEnd() -split '\r?\n') | ForEach-Object { '[2026.01.01-00.00.00:000][  1]' + $_ }) -join "`n"
Assert-PresentationLog ($prefixed + "`n")

$lastCamera = $fixture.LastIndexOf($cameraLine)
$lastSelection = $fixture.LastIndexOf($selectionHeader)
$cases = @(
    @('wrong possessed identity',
        $fixture.Replace(".PossessedUnit = $unitRef", ".PossessedUnit = /Script/RomanEmpireGame.Legionary'$otherUnit'"),
        'PossessedUnit differs from selection.'),
    @('wrong actual Pawn',
        $fixture.Replace(".Pawn = $unitRef", ".Pawn = /Script/RomanEmpireGame.Legionary'$otherUnit'"),
        'actual Pawn differs from selection.'),
    @('uncleared possessed reference',
        $fixture.Replace('.PossessedUnit = None', ".PossessedUnit = $unitRef"),
        'return must clear possession and selection.'),
    @('wrong restored camera',
        $fixture.Remove($lastCamera, $cameraLine.Length).Insert($lastCamera, $cameraLine.Replace('Camera_1', 'Camera_2')),
        'return must restore the original camera Pawn.'),
    @('lingering unit possession flag',
        ($fixture.Substring(0, $lastCamera) + $fixture.Substring($lastCamera).Replace("$unitFlag = False", "$unitFlag = True")),
        'a unit remains flagged as possessed after return.'),
    @('multiple possessed units',
        $fixture.Replace("$otherFlag = False", "$otherFlag = True"),
        'exactly the selected unit must report possession.'),
    @('changed controller identity',
        $fixture.Replace("$controller.PossessedUnit", 'WRONG_CONTROLLER.PossessedUnit'),
        'samples refer to different controllers.'),
    @('failure signature',
        ($fixture + "LogRomanEmpire: Error: synthetic injected error`n"),
        'failure signature in Player.log.'),
    @('historical incomplete camera samples',
        $fixture.Replace($cameraLine + "`n", ''),
        'incomplete state samples:'),
    @('wrong entry unit',
        $fixture.Replace('Entered FPS mode with unit: Unit_1', 'Entered FPS mode with unit: Unit_2'),
        'entered unit differs from selected unit.'),
    @('uncleared selection array',
        $fixture.Insert($lastSelection + $selectionHeader.Length + 1, "$memberLine`n"),
        'return must clear possession and selection.'),
    @('duplicate selection member',
        $fixture.Replace($memberLine, "$memberLine`n$memberLine"),
        'selection must contain exactly one unit.'),
    @('missing possession flag samples',
        [regex]::Replace($fixture, '(?m)^\d+\) Legionary [^\r\n]+\.bIsPossessedByPlayer = (?:True|False)\r?\n', ''),
        'missing or out-of-phase possession flag samples.'),
    @('duplicate final unit sample',
        ($fixture + "$otherFlag = False`n"),
        'missing or out-of-phase possession flag samples.'),
    @('reordered lifecycle',
        $fixture.Replace('Entered FPS mode with unit: Unit_1', 'Exited FPS mode').Replace('Exited FPS mode - camera restored', 'Entered FPS mode with unit: Unit_1'),
        'selection/possession/exit order.'),
    @('changed final unit identity',
        ($fixture.Substring(0, $lastCamera) + $fixture.Substring($lastCamera).Replace('Unit_2.bIsPossessedByPlayer', 'Unit_3.bIsPossessedByPlayer')),
        'unit sample identities changed or contain duplicates.'),
    @('camera sample in wrong phase',
        $fixture.Substring($cameraLine.Length + 1).Insert($fixture.IndexOf($selectionHeader) - $cameraLine.Length - 1, $cameraLine + "`n"),
        'state samples are out of phase.')
)

foreach ($case in $cases) {
    $failure = $null
    try { Assert-PresentationLog $case[1] } catch { $failure = $_.Exception.Message }
    if ($null -eq $failure) { throw "Mutation survived: $($case[0])" }
    if (-not $failure.Contains($case[2])) {
        throw "Mutation '$($case[0])' failed for the wrong reason. Expected '$($case[2])'; got '$failure'."
    }
    Write-Output "Rejected as expected: $($case[0])"
}
Write-Output "Presentation log parser passed: 3 synthetic positive fixtures and $($cases.Count) expected rejection cases. No game/build launched."

$ownerLine = "0) Legionary $unit.OwnerFaction = Carthage`n"
Assert-PresentationLog ($fixture + $ownerLine) $true
foreach ($ownerFixture in @($fixture, ($fixture + $ownerLine.Replace('Carthage', 'Rome')), ($ownerLine + $fixture))) {
    $failure = $null
    try { Assert-PresentationLog $ownerFixture $true } catch { $failure = $_.Exception.Message }
    if ($null -eq $failure -or -not $failure.Contains('ownership-loss scenario must record')) {
        throw 'Ownership-loss parser mutation survived or failed for an unrelated reason.'
    }
}
Write-Output 'Ownership-loss parser passed: 1 synthetic positive and 3 expected rejection cases. No game/build launched.'
