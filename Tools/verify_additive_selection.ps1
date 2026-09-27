[CmdletBinding(DefaultParameterSetName = 'Player')]
param(
    [Parameter(Mandatory, ParameterSetName = 'Player')][string]$ArchiveDirectory,
    [Parameter(Mandatory, ParameterSetName = 'Player')]
    [Parameter(Mandatory, ParameterSetName = 'EditorPreview')][string]$OutputDirectory,
    [Parameter(Mandatory, ParameterSetName = 'EditorPreview')][switch]$EditorPreview,
    [Parameter(Mandatory, ParameterSetName = 'SelfTest')][switch]$SelfTest,
    [ValidateRange(1, 600)][int]$TimeoutSeconds = 120
)
Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

function Assert-AdditiveSelectionLog([string]$Log) {
    $lines = [regex]::Replace($Log, '(?m)^\[[^\]\r\n]+\]\[\s*\d+\]', '')
    function Check([bool]$Condition, [string]$Message) {
        if (-not $Condition) { throw "Additive selection check failed: $Message" }
    }
    Check ($lines -notmatch '(?im)(Fatal error:|Assertion failed:|Ensure condition failed:|\bError:|call failed|Failed to find an object|Bad or missing property|Command not recognized|Unrecognized command)') 'failure signature in Player.log'
    $selected = [regex]::Matches($lines, '(?m)^LogRomanEmpire: Display: Presentation selected (?<path>.+) at .+ \(selected=1\)\r?$')
    Check ($selected.Count -eq 1) 'require one canonical initial selection'
    $samples = [regex]::Matches($lines, '(?m)^\d+\) \S+ (?<controller>[^\r\n]+)\.SelectedUnits =[ \t]*\r?\n(?<members>(?:[ \t]+\d+: [^\r\n]+\r?\n)*)')
    Check ($samples.Count -eq 5) 'require initial/Add/repeated Add/empty Add/empty Replace samples'
    Check ($selected[0].Index -lt $samples[0].Index) 'initial state precedes canonical selection'
    $sets = [Collections.Generic.List[object]]::new()
    foreach ($sample in $samples) {
        Check ($sample.Groups['controller'].Value -eq $samples[0].Groups['controller'].Value) 'controller changed between phases'
        $members = [regex]::Matches($sample.Groups['members'].Value, '(?m)^[ \t]+(?<index>\d+): (?<value>[^\r\n]+)\r?$')
        $paths = @()
        for ($i = 0; $i -lt $members.Count; $i++) {
            Check ([int]$members[$i].Groups['index'].Value -eq $i) 'noncontiguous array indices'
            $value = $members[$i].Groups['value'].Value
            Check ($value -match "^\S+'([^']+)'$") 'invalid reflected unit reference'
            $paths += $Matches[1]
        }
        Check (@($paths | Sort-Object -Unique).Count -eq $paths.Count) 'duplicate unit identity'
        $sets.Add($paths)
    }
    Check ($sets[0].Count -eq 1 -and $sets[0][0] -eq $selected[0].Groups['path'].Value) 'initial selection identity/count'
    Check ($sets[1].Count -gt 1 -and $sets[1][0] -eq $sets[0][0]) 'Add must grow selection and preserve initial first member'
    Check (($sets[1] -join "`n") -eq ($sets[2] -join "`n")) 'repeated Add changed identities/order'
    Check (($sets[1] -join "`n") -eq ($sets[3] -join "`n")) 'empty Add changed identities/order'
    Check ($sets[4].Count -eq 0) 'empty Replace must clear selection'
    # Native TextBlock.Text is reflected, but getall includes every live text block.
    # The stable widget name identifies only the feedback text; do not infer pixels.
    $feedback = [regex]::Matches($lines, '(?m)^\d+\) \S+ [^\r\n]+\.InteractionFeedbackText\.Text = (?<text>[^\r\n]+)\r?$')
    $visibility = [regex]::Matches($lines, '(?m)^\d+\) \S+ [^\r\n]+\.InteractionFeedbackText\.Visibility = (?<value>[^\r\n]+)\r?$')
    if ($feedback.Count -eq 1 -and $visibility.Count -eq 1) {
        Check ($feedback[0].Index -gt $samples[4].Index) 'feedback sample out of phase'
        Check ($feedback[0].Groups['text'].Value.Contains('Choose a living enemy unit to attack.')) 'unexpected feedback text'
        Check ($visibility[0].Index -gt $samples[4].Index -and $visibility[0].Groups['value'].Value -match '^(?:ESlateVisibility::)?HitTestInvisible$') 'feedback text not visible'
        return 'Selection state passed; reflected feedback text passed. Inspect Added.png and Feedback.png for rendered UI/indicators.'
    }
    return 'Selection state passed. Feedback reflection was not uniquely identified; visually inspect Feedback.png for the invalid-target message and Added.png for count/indicators before claiming rendered UI success.'
}

if ($SelfTest) {
    $controller = '/Game/Test.Test:PersistentLevel.RomanEmpirePlayerController_0'
    $unit = '/Game/Test.Test:PersistentLevel.Legionary_0'
    $fixture = "LogRomanEmpire: Display: Presentation selected $unit at X=0 (selected=1)`n"
    foreach ($members in @(@($unit), @($unit, 'Unit_1'), @($unit, 'Unit_1'), @($unit, 'Unit_1'), @())) {
        $fixture += "0) RomanEmpirePlayerController $controller.SelectedUnits = `n"
        for ($i = 0; $i -lt $members.Count; $i++) { $fixture += "    ${i}: /Script/RomanEmpireGame.Legionary'$($members[$i])'`n" }
    }
    Assert-AdditiveSelectionLog $fixture | Out-Null
    $feedbackFixture = "0) TextBlock /Game/Test.Widget.WidgetTree.InteractionFeedbackText.Text = Choose a living enemy unit to attack.`n0) TextBlock /Game/Test.Widget.WidgetTree.InteractionFeedbackText.Visibility = ESlateVisibility::HitTestInvisible`n"
    $feedbackResult = Assert-AdditiveSelectionLog ($fixture + $feedbackFixture)
    if (-not $feedbackResult.Contains('reflected feedback text passed')) { throw 'Feedback success fixture was not verified' }
    $badFixtures = @(
        ($fixture + "LogTemp: Error: synthetic failure`n")
        $fixture.Replace('Unit_1', $unit)
        $fixture.Replace("$controller.SelectedUnits", '/Other.Controller.SelectedUnits')
        $fixture.Substring(0, $fixture.LastIndexOf('0) RomanEmpirePlayerController'))
        $fixture.Replace("    1: /Script/RomanEmpireGame.Legionary'Unit_1'", "    2: /Script/RomanEmpireGame.Legionary'Unit_1'")
        # First Add differs from repeated Add, even though counts remain equal.
        ([regex]::new('Unit_1')).Replace($fixture, 'DifferentUnit', 1)
        ($fixture + "    0: /Script/RomanEmpireGame.Legionary'$unit'`n")
        ($fixture + $feedbackFixture.Replace('Choose a living enemy unit to attack.', 'Wrong feedback'))
        ($fixture + $feedbackFixture.Replace('HitTestInvisible', 'Collapsed'))
        ($feedbackFixture + $fixture)
        ($fixture.Substring($fixture.IndexOf("`n") + 1) + $fixture.Substring(0, $fixture.IndexOf("`n") + 1))
    )
    # Changing every controller consistently is valid; make only the first phase differ.
    $badFixtures[2] = ([regex]::new([regex]::Escape("$controller.SelectedUnits"))).Replace($fixture, '/Other.Controller.SelectedUnits', 1)
    foreach ($bad in $badFixtures) {
        $rejected = $false
        try { Assert-AdditiveSelectionLog $bad | Out-Null } catch { $rejected = $true }
        if (-not $rejected) { throw 'Synthetic invalid fixture was accepted' }
    }
    Write-Output "Synthetic parser fixtures passed (selection and feedback success; $($badFixtures.Count) rejection cases). No process launched; this is not runtime evidence."
    return
}

$projectRoot = Split-Path -Parent $PSScriptRoot
$outputPath = [IO.Path]::GetFullPath((Join-Path $projectRoot $OutputDirectory))
if ($outputPath.IndexOfAny([char[]]' ,"''') -ge 0) { throw 'Output path must contain no spaces, commas or quotes for console screenshots.' }
if ($EditorPreview) {
    $playerPath = 'C:/Program Files/Epic Games/UE_5.8/Engine/Binaries/Win64/UnrealEditor.exe'
    $evidenceLabel = 'EDITOR GAME PREVIEW ONLY: not packaged-player evidence; packaged gate remains open'
} else {
    $archivePath = [IO.Path]::GetFullPath((Join-Path $projectRoot $ArchiveDirectory))
    $playerPath = Join-Path $archivePath 'Windows/RomanEmpireGame/Binaries/Win64/RomanEmpireGame.exe'
    $evidenceLabel = 'PACKAGED DEVELOPMENT PLAYER'
}
if (-not (Test-Path -LiteralPath $playerPath -PathType Leaf)) { throw "Missing executable: $playerPath" }
if (Test-Path -LiteralPath $outputPath) { throw "Output already exists; choose a fresh -OutputDirectory: $outputPath" }
New-Item -ItemType Directory -Path $outputPath | Out-Null
$logPath = Join-Path $outputPath $(if ($EditorPreview) { 'EditorPreview.log' } else { 'Player.log' })
$addedPath = (Join-Path $outputPath 'Added.png').Replace('\', '/')
$feedbackPath = (Join-Path $outputPath 'Feedback.png').Replace('\', '/')
$steps = [Collections.Generic.List[string]]::new()
function Add-Step([int]$Frame, [string]$Command) { $steps.Add(('defer ' * $Frame) + $Command) }
Add-Step 0 'r.MotionBlurQuality 0'
Add-Step 5 'ke Legionary CommandHold'
Add-Step 15 'roman.Debug.SelectPlayerUnit'
Add-Step 25 'getall RomanEmpirePlayerController SelectedUnits'
# UE 5.8 ParseExecCommands.cpp protects commas in single quotes and converts
# quotes to double quotes. ScriptCore.cpp parses each quoted token and imports
# it via FStructProperty/UScriptStruct ImportText. Keep these struct args quoted.
Add-Step 35 "ke RomanEmpirePlayerController SelectUnitsInScreenRect '(X=0,Y=0)' '(X=1920,Y=1080)' True"
Add-Step 45 'getall RomanEmpirePlayerController SelectedUnits'
Add-Step 55 "shot showui filename=$addedPath -nosuffix"
Add-Step 65 "ke RomanEmpirePlayerController SelectUnitsInScreenRect '(X=0,Y=0)' '(X=1920,Y=1080)' True"
Add-Step 75 'getall RomanEmpirePlayerController SelectedUnits'
Add-Step 85 "ke RomanEmpirePlayerController SelectUnitsInScreenRect '(X=-10000,Y=-10000)' '(X=-9000,Y=-9000)' True"
Add-Step 95 'getall RomanEmpirePlayerController SelectedUnits'
Add-Step 105 "ke RomanEmpirePlayerController SelectUnitsInScreenRect '(X=-10000,Y=-10000)' '(X=-9000,Y=-9000)' False"
Add-Step 115 'getall RomanEmpirePlayerController SelectedUnits'
Add-Step 125 'ke RomanEmpirePlayerController CommandSelectedUnitsAttack None'
Add-Step 130 'getall TextBlock Text'
Add-Step 130 'getall TextBlock Visibility'
Add-Step 140 "shot showui filename=$feedbackPath -nosuffix"
$commands = $steps -join ','
$arguments = @('-windowed', '-ResX=1920', '-ResY=1080', '-forceres', '-nosplash', '-unattended', '-benchmark', '-seconds=8', '-log', ('-abslog="' + $logPath + '"'), ('-ExecCmds="' + $commands + '"'))
# Editor-only Python toolsets cannot initialize their editor APIs in -game.
# This process needs no scripting bridge; leave project/plugin defaults unchanged.
if ($EditorPreview) { $arguments = @(('"' + (Join-Path $projectRoot 'RomanEmpireGame.uproject') + '"'), '-game', '-DisablePlugins=Fab', '-DisablePython') + $arguments }
Write-Output $evidenceLabel
Write-Output ("Executable SHA256: " + (Get-FileHash -LiteralPath $playerPath -Algorithm SHA256).Hash)
$process = Start-Process -FilePath $playerPath -ArgumentList $arguments -WindowStyle Hidden -PassThru
Write-Output "Additive selection player PID: $($process.Id)"
$deadline = [DateTime]::UtcNow.AddSeconds($TimeoutSeconds)
while (-not $process.WaitForExit(1000)) {
    if ([DateTime]::UtcNow -ge $deadline) { throw "Player exceeded timeout. PID $($process.Id) was not killed or restarted; inspect it and $logPath before another run." }
}
if ($process.ExitCode -ne 0) { throw "Player exited $($process.ExitCode); inspect $logPath" }
foreach ($path in @($addedPath, $feedbackPath)) {
    if (-not (Test-Path -LiteralPath $path -PathType Leaf)) { throw "Capture missing: $path" }
}
Assert-AdditiveSelectionLog (Get-Content -LiteralPath $logPath -Raw -Encoding UTF8)
Write-Output "$evidenceLabel. Process exited 0 and both captures exist. This uses reflected gameplay functions; physical input is not exercised."
