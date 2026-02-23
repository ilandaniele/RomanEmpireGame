<#
.SYNOPSIS
    Roman Empire Game - Automated Test Orchestrator
.DESCRIPTION
    Builds, launches, tests, and analyzes the Roman Empire Game.
.PARAMETER Action
    test    - Full pipeline: build + launch + analyze
    build   - Build only 
    launch  - Launch game in standalone mode
    log     - Analyze latest log file
#>

param(
    [ValidateSet("test", "build", "launch", "log")]
    [string]$Action = "test"
)

$ErrorActionPreference = "Continue"

# --- Configuration ---
$ProjectRoot = "c:\Users\iland\.gemini\antigravity\playground\eternal-perigee\RomanEmpireGame"
$UProjectFile = "$ProjectRoot\RomanEmpireGame.uproject"
$BuildScript = "$ProjectRoot\build.bat"
$LogDir = "$ProjectRoot\Saved\Logs"
$ReportDir = "$ProjectRoot\Tools\Reports"
$UE5Editor = "C:\Program Files\Epic Games\UE_5.7\Engine\Binaries\Win64\UnrealEditor.exe"

if (-not (Test-Path $ReportDir)) {
    New-Item -ItemType Directory -Path $ReportDir -Force | Out-Null
}

$Timestamp = Get-Date -Format "yyyy-MM-dd_HH-mm-ss"

function Write-Status($msg) { Write-Host "[ORCHESTRATOR] $msg" -ForegroundColor Cyan }
function Write-Pass($msg) { Write-Host "  [PASS] $msg" -ForegroundColor Green }
function Write-Fail($msg) { Write-Host "  [FAIL] $msg" -ForegroundColor Red }
function Write-Warn($msg) { Write-Host "  [WARN] $msg" -ForegroundColor Yellow }

function Do-Build {
    Write-Status "Building project..."
    $buildLog = "$ReportDir\build_$Timestamp.log"
    & cmd /c "$BuildScript" > $buildLog 2>&1
    $exitCode = $LASTEXITCODE
    $content = Get-Content $buildLog -Raw -ErrorAction SilentlyContinue
    $succeeded = $content -match "Result: Succeeded"
    if ($exitCode -eq 0 -and $succeeded) {
        Write-Pass "Build succeeded"
        return $true
    } else {
        Write-Fail "Build failed (Exit: $exitCode)"
        return $false
    }
}

function Do-Launch {
    Write-Status "Launching game in standalone mode..."
    Stop-Process -Name "UnrealEditor*" -Force -ErrorAction SilentlyContinue
    Start-Sleep -Seconds 2
    
    $argStr = "`"$UProjectFile`" /Game/NewMap -game -windowed -resx=1280 -resy=720 -log -nosplash -nosound"
    $process = Start-Process -FilePath $UE5Editor -ArgumentList $argStr -PassThru -ErrorAction SilentlyContinue
    
    if ($process) {
        Write-Status "Game launched (PID: $($process.Id)). Waiting 40s..."
        Start-Sleep -Seconds 40
        if (-not $process.HasExited) {
            Write-Pass "Game is running"
            Start-Sleep -Seconds 10
            Stop-Process -Id $process.Id -Force -ErrorAction SilentlyContinue
            Start-Sleep -Seconds 3
            return $true
        } else {
            Write-Fail "Game exited prematurely (Code: $($process.ExitCode))"
            return $false
        }
    } else {
        Write-Fail "Failed to start game"
        return $false
    }
}

function Do-LogAnalysis {
    Write-Status "Analyzing game log..."
    
    $latestLog = Get-ChildItem "$LogDir\*.log" -ErrorAction SilentlyContinue | Sort-Object LastWriteTime -Descending | Select-Object -First 1
    if (-not $latestLog) {
        Write-Fail "No log files found"
        return
    }
    
    Write-Status "Reading: $($latestLog.FullName)"
    $content = Get-Content $latestLog.FullName -ErrorAction SilentlyContinue
    if (-not $content) {
        Write-Fail "Log empty"
        return
    }
    
    $passed = 0
    $total = 0
    
    # Check: GameMode
    $total++
    if ($content | Select-String "Roman Empire Game Mode started") {
        Write-Pass "GameMode initialized"
        $passed++
    } else { Write-Fail "GameMode NOT initialized" }
    
    # Check: Camera
    $total++
    if ($content | Select-String "Seamless Zoom Camera initialized") {
        Write-Pass "Camera initialized"
        $passed++
    } else { Write-Fail "Camera NOT initialized" }
    
    # Check: Input
    $total++
    if ($content | Select-String "Input actions and mappings created") {
        Write-Pass "Input system created"
        $passed++
    } else { Write-Fail "Input NOT created" }
    
    # Check: Player Controller
    $total++
    if ($content | Select-String "Player Controller initialized") {
        Write-Pass "PlayerController initialized"
        $passed++
    } else { Write-Fail "PlayerController NOT initialized" }
    
    # Check: HUD
    $total++
    if ($content | Select-String "HUD initialized") {
        Write-Pass "HUD initialized"
        $passed++
    } else { Write-Fail "HUD NOT initialized" }
    
    # Check: Units
    $total++
    $units = $content | Select-String "Spawned Legionary"
    if ($units) {
        Write-Pass "Units spawned ($($units.Count) legionaries)"
        $passed++
    } else { Write-Fail "No units spawned" }
    
    # Check: Managers
    $total++
    $fm = $content | Select-String "Faction Manager initialized"
    $wm = $content | Select-String "World Map Manager initialized"
    $cm = $content | Select-String "Campaign Manager initialized"
    if ($fm -and $wm -and $cm) {
        Write-Pass "All managers initialized"
        $passed++
    } else { Write-Fail "Some managers missing" }
    
    # Check: No Crashes
    $total++
    $crashes = $content | Select-String "Assertion failed|Fatal error|appError"
    if ($crashes) {
        Write-Fail "CRASHES DETECTED"
        $crashes | ForEach-Object { Write-Host "    $($_.Line)" -ForegroundColor Red }
    } else {
        Write-Pass "No crashes"
        $passed++
    }
    
    # Check: Enhanced Input registered
    $total++
    if ($content | Select-String "Enhanced Input mapping context registered") {
        Write-Pass "Enhanced Input registered"
        $passed++
    } else { Write-Fail "Enhanced Input NOT registered" }
    
    Write-Host ""
    if ($passed -eq $total) {
        Write-Host "========================================" -ForegroundColor Green
        Write-Host " ALL TESTS PASSED ($passed/$total)" -ForegroundColor Green
        Write-Host "========================================" -ForegroundColor Green
    } else {
        Write-Host "========================================" -ForegroundColor Red
        Write-Host " TESTS: $passed/$total passed" -ForegroundColor Red
        Write-Host "========================================" -ForegroundColor Red
    }
    
    # Save report
    $reportLines = @(
        "# Test Report - $(Get-Date -Format 'yyyy-MM-dd HH:mm:ss')"
        "Result: $passed/$total passed"
        "Log: $($latestLog.FullName)"
    )
    $reportLines | Out-File "$ReportDir\report_$Timestamp.md" -Encoding utf8
    Write-Status "Report saved to $ReportDir\report_$Timestamp.md"
}

# --- Main ---
Write-Host ""
Write-Host "======================================" -ForegroundColor Cyan
Write-Host " Roman Empire Game Test Orchestrator" -ForegroundColor Cyan
Write-Host "======================================" -ForegroundColor Cyan
Write-Host ""

switch ($Action) {
    "build" { Do-Build }
    "launch" { Do-Launch }
    "log" { Do-LogAnalysis }
    "test" {
        Write-Status "Starting full test pipeline..."
        Write-Host ""
        $buildOK = Do-Build
        if (-not $buildOK) { Write-Fail "Build failed - aborting"; exit 1 }
        Write-Host ""
        $launchOK = Do-Launch
        Write-Host ""
        Do-LogAnalysis
    }
}
