[CmdletBinding()]
param(
    [string]$ArchiveDirectory = 'Artifacts/Packages/M3_PresentationFinal',
    [string]$OutputDirectory = 'Artifacts/UnitPresentation',
    [switch]$LoseOwnership,
    [ValidateRange(1, 600)]
    [int]$TimeoutSeconds = 120
)

# Drives only the real Development player's reflected gameplay facade. No remote
# service, editor/test module, OS keyboard/cursor injection, or saved asset changes.
Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

function Assert-PresentationLog([string]$Log, [bool]$OwnershipLoss = $false) {
    # Strip only UE's timestamp/frame prefix; preserve array indentation and
    # object paths. These checks compare observed state, not KE dispatch success.
    $lines = [regex]::Replace($Log, '(?m)^\[[^\]\r\n]+\]\[\s*\d+\]', '')
    function Assert-Check([bool]$Condition, [string]$Message) {
        if (-not $Condition) { throw "Presentation state check failed: $Message" }
    }
    function Get-ObjectPath([string]$Value) {
        if ($Value -match "^\S+'([^']+)'$") { return $Matches[1] }
        throw "Expected a reflected object reference, got: $Value"
    }
    Assert-Check ($lines -notmatch '(?im)(Fatal error:|Assertion failed:|Ensure condition failed:|\bError:|call failed|Failed to find an object|Bad or missing property|Command not recognized|Unrecognized command)') 'failure signature in Player.log.'

    $selected = [regex]::Matches($lines, '(?m)^LogRomanEmpire: Display: Presentation selected (?<path>.+) at .+ \(selected=1\)\r?$')
    $entered = [regex]::Matches($lines, '(?m)^LogRomanEmpire: Entered FPS mode with unit: (?<name>\S+)\r?$')
    $exited = [regex]::Matches($lines, '(?m)^LogRomanEmpire: Exited FPS mode[^\r\n]*\r?$')
    Assert-Check ($selected.Count -eq 1 -and $entered.Count -eq 1 -and $exited.Count -eq 1) 'expected one successful selection, possession and exit.'
    $unitPath = $selected[0].Groups['path'].Value
    Assert-Check ($unitPath.EndsWith('.' + $entered[0].Groups['name'].Value)) 'entered unit differs from selected unit.'
    Assert-Check ($selected[0].Index -lt $entered[0].Index -and $entered[0].Index -lt $exited[0].Index) 'selection/possession/exit order.'

    $selection = [regex]::Matches($lines, '(?m)^\d+\) \S+ (?<controller>[^\r\n]+)\.SelectedUnits =[ \t]*\r?\n(?<members>(?:[ \t]+\d+: [^\r\n]+\r?\n)*)')
    $possession = [regex]::Matches($lines, '(?m)^\d+\) \S+ (?<controller>[^\r\n]+)\.PossessedUnit = (?<value>[^\r\n]+)\r?$')
    $pawns = [regex]::Matches($lines, '(?m)^\d+\) \S+ (?<controller>[^\r\n]+)\.Pawn = (?<value>[^\r\n]+)\r?$')
    Assert-Check ($selection.Count -eq 2 -and $possession.Count -eq 2 -and $pawns.Count -eq 3) 'incomplete state samples: require selected/returned arrays, possessed/returned references and initial/possessed/returned Pawn.'
    $controllerPath = $selection[0].Groups['controller'].Value
    foreach ($sample in @($selection) + @($possession) + @($pawns)) {
        Assert-Check ($sample.Groups['controller'].Value -eq $controllerPath) 'samples refer to different controllers.'
    }
    $members = [regex]::Matches($selection[0].Groups['members'].Value, '(?m)^[ \t]+\d+: (?<value>[^\r\n]+)\r?$')
    Assert-Check ($members.Count -eq 1) 'selection must contain exactly one unit.'
    Assert-Check ((Get-ObjectPath $members[0].Groups['value'].Value) -eq $unitPath) 'selected array does not contain the selected unit.'
    Assert-Check ((Get-ObjectPath $possession[0].Groups['value'].Value) -eq $unitPath) 'PossessedUnit differs from selection.'
    Assert-Check ((Get-ObjectPath $pawns[1].Groups['value'].Value) -eq $unitPath) 'actual Pawn differs from selection.'
    Assert-Check ($possession[1].Groups['value'].Value -eq 'None' -and $selection[1].Groups['members'].Value.Length -eq 0) 'return must clear possession and selection.'
    $cameraPath = Get-ObjectPath $pawns[0].Groups['value'].Value
    Assert-Check ($pawns[0].Groups['value'].Value.StartsWith("/Script/RomanEmpireGame.SeamlessZoomCamera'") -and
        (Get-ObjectPath $pawns[2].Groups['value'].Value) -eq $cameraPath -and $cameraPath -ne $unitPath) 'return must restore the original camera Pawn.'
    Assert-Check ($pawns[0].Index -lt $selected[0].Index -and $selected[0].Index -lt $selection[0].Index -and
        $selection[0].Index -lt $entered[0].Index -and $entered[0].Index -lt $possession[0].Index -and
        $possession[0].Index -lt $pawns[1].Index -and $pawns[1].Index -lt $exited[0].Index -and
        $exited[0].Index -lt $possession[1].Index -and $possession[1].Index -lt $selection[1].Index -and
        $selection[1].Index -lt $pawns[2].Index) 'state samples are out of phase.'

    $flags = [regex]::Matches($lines, '(?m)^\d+\) \S+ (?<unit>[^\r\n]+)\.bIsPossessedByPlayer = (?<value>True|False)\r?$')
    $during = @($flags | Where-Object { $_.Index -gt $pawns[1].Index -and $_.Index -lt $exited[0].Index })
    $after = @($flags | Where-Object { $_.Index -gt $pawns[2].Index })
    Assert-Check ($during.Count -gt 0 -and $during.Count -eq $after.Count -and $flags.Count -eq ($during.Count + $after.Count)) 'missing or out-of-phase possession flag samples.'
    $duringPaths = @($during | ForEach-Object { $_.Groups['unit'].Value } | Sort-Object -Unique)
    $afterPaths = @($after | ForEach-Object { $_.Groups['unit'].Value } | Sort-Object -Unique)
    Assert-Check ($duringPaths.Count -eq $during.Count -and $afterPaths.Count -eq $after.Count -and
        ($duringPaths -join "`n") -eq ($afterPaths -join "`n")) 'unit sample identities changed or contain duplicates.'
    $active = @($during | Where-Object { $_.Groups['value'].Value -eq 'True' })
    Assert-Check ($active.Count -eq 1 -and $active[0].Groups['unit'].Value -eq $unitPath) 'exactly the selected unit must report possession.'
    Assert-Check (@($after | Where-Object { $_.Groups['value'].Value -eq 'True' }).Count -eq 0) 'a unit remains flagged as possessed after return.'
    if ($OwnershipLoss) {
        $owner = [regex]::Matches($lines, '(?m)^\d+\) \S+ ' + [regex]::Escape($unitPath) + '\.OwnerFaction = (?:EFactionID::)?Carthage\r?$')
        Assert-Check ($owner.Count -eq 1 -and $owner[0].Index -gt $exited[0].Index) 'ownership-loss scenario must record the former selected unit as Carthage after automatic exit.'
    }
}

$projectRoot = Split-Path -Parent $PSScriptRoot
$archivePath = [IO.Path]::GetFullPath((Join-Path $projectRoot $ArchiveDirectory))
$outputPath = [IO.Path]::GetFullPath((Join-Path $projectRoot $OutputDirectory))
$invalidCommandPath = $outputPath.IndexOfAny([char[]]' ,"') -ge 0
if ($invalidCommandPath) { throw 'Choose an output path without spaces, commas or quotes for Unreal console capture commands.' }
$playerPath = Join-Path $archivePath 'Windows/RomanEmpireGame/Binaries/Win64/RomanEmpireGame.exe'
if (-not (Test-Path -LiteralPath $playerPath -PathType Leaf)) { throw "Missing Development player: $playerPath" }
# Refuse to overwrite prior evidence; use a fresh run directory.
if (Test-Path -LiteralPath $outputPath) { throw "Output already exists; choose a fresh -OutputDirectory: $outputPath" }
New-Item -ItemType Directory -Path $outputPath | Out-Null
$logPath = Join-Path $outputPath 'Player.log'
$selectedPath = (Join-Path $outputPath 'Selected.png').Replace('\', '/')
$possessedPath = (Join-Path $outputPath 'Possessed.png').Replace('\', '/')
$returnedPath = (Join-Path $outputPath 'Returned.png').Replace('\', '/')
$steps = [Collections.Generic.List[string]]::new()
function Add-Step([int]$Frame, [string]$Command) {
    # UE's built-in DEFER executes on the next frame. Nesting schedules this
    # finite scenario without new runtime code or dependence on wall-clock FPS.
    $steps.Add(('defer ' * $Frame) + $Command)
}
Add-Step 0 'r.MotionBlurQuality 0'
Add-Step 5 'ke Legionary CommandHold'
Add-Step 10 'getall RomanEmpirePlayerController Pawn'
Add-Step 15 'roman.Debug.SelectPlayerUnit'
Add-Step 25 'getall RomanEmpirePlayerController SelectedUnits'
Add-Step 40 "shot showui filename=$selectedPath -nosuffix"
Add-Step 60 'roman.Debug.PossessSelectedUnit'
Add-Step 85 'getall RomanEmpirePlayerController PossessedUnit'
Add-Step 86 'getall RomanEmpirePlayerController Pawn'
Add-Step 86 'getall UnitBase bIsPossessedByPlayer'
Add-Step 90 "shot showui filename=$possessedPath -nosuffix"
if ($LoseOwnership) {
    # Controlled transient fixture: transfer all initial legionaries, including
    # the selected/possessed unit. Do not explicitly exit or clear selection.
    Add-Step 120 'ke Legionary SetOwnerFaction Carthage'
    Add-Step 149 'getall UnitBase OwnerFaction'
} else {
    Add-Step 120 'ke RomanEmpirePlayerController ExitFirstPersonMode'
    Add-Step 125 'ke RomanEmpirePlayerController ClearSelection'
}
Add-Step 145 'getall RomanEmpirePlayerController PossessedUnit'
Add-Step 146 'getall RomanEmpirePlayerController SelectedUnits'
Add-Step 147 'getall RomanEmpirePlayerController Pawn'
Add-Step 148 'getall UnitBase bIsPossessedByPlayer'
Add-Step 160 "shot showui filename=$returnedPath -nosuffix"
$commands = $steps -join ','
$arguments = @('-windowed', '-ResX=1920', '-ResY=1080', '-forceres', '-nosplash',
    '-unattended', '-benchmark', '-seconds=8', '-log',
    ('-abslog="' + $logPath + '"'), ('-ExecCmds="' + $commands + '"'))
Write-Output ("Player SHA256: " + (Get-FileHash -LiteralPath $playerPath -Algorithm SHA256).Hash)
Write-Output ("Scenario: " + $(if ($LoseOwnership) { 'ownership loss with automatic exit/selection cleanup' } else { 'explicit exit and selection clear' }))
$process = Start-Process -FilePath $playerPath -ArgumentList $arguments -WindowStyle Hidden -PassThru
Write-Output "Presentation player PID: $($process.Id)"
# The command runner can yield/poll this script. Never automatically relaunch
# just because observing the existing process takes longer than expected.
$deadline = [DateTime]::UtcNow.AddSeconds($TimeoutSeconds)
while (-not $process.WaitForExit(1000)) {
    if ([DateTime]::UtcNow -ge $deadline) {
        throw "Player exceeded $TimeoutSeconds seconds. PID $($process.Id) was not killed or restarted; inspect it and $logPath before another run."
    }
}
if ($process.ExitCode -ne 0) { throw "Player exited $($process.ExitCode); inspect $logPath" }
foreach ($path in @($selectedPath, $possessedPath, $returnedPath)) {
    if (-not (Test-Path -LiteralPath $path -PathType Leaf)) { throw "Capture missing: $path" }
}
$log = Get-Content -LiteralPath $logPath -Raw -Encoding UTF8
Assert-PresentationLog $log $LoseOwnership.IsPresent
Write-Output 'Player exited 0; selected/Pawn identity, restored camera, cleared selection and possession flags passed. All three captures exist; inspect their pixels before claiming the rendered gate passed. This does not exercise physical input.'
