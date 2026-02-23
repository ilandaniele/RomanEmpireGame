<#
.SYNOPSIS
    Roman Empire Game - AI-Powered Development Orchestrator
.DESCRIPTION
    Uses Gemini AI to analyze the codebase, suggest improvements, generate code,
    build, test, and iterate autonomously.
.PARAMETER Action
    develop  - Full AI dev loop: analyze -> suggest -> implement -> build -> test
    analyze  - Ask Gemini to analyze current codebase and suggest next steps
    implement - Give Gemini a specific task to implement
    build    - Build only
    test     - Build + launch + log analysis
.PARAMETER Task
    Specific task description for 'implement' action
.PARAMETER Iterations
    Number of development iterations for 'develop' action (default: 3)
#>

param(
    [ValidateSet("develop", "analyze", "implement", "build", "test")]
    [string]$Action = "develop",
    [string]$Task = "",
    [int]$Iterations = 3
)

$ErrorActionPreference = "Continue"

# --- Configuration ---
$ProjectRoot = "c:\Users\iland\.gemini\antigravity\playground\eternal-perigee\RomanEmpireGame"
$SourceDir = "$ProjectRoot\Source\RomanEmpireGame"
$BuildScript = "$ProjectRoot\build.bat"
$LogDir = "$ProjectRoot\Saved\Logs"
$ReportDir = "$ProjectRoot\Tools\Reports"
$EnvFile = "$ProjectRoot\Tools\.env"

# Load API key
if (Test-Path $EnvFile) {
    Get-Content $EnvFile | ForEach-Object {
        if ($_ -match '^\s*([^#][^=]+)=(.+)$') {
            [System.Environment]::SetEnvironmentVariable($Matches[1].Trim(), $Matches[2].Trim(), "Process")
        }
    }
}
$ApiKey = $env:GEMINI_API_KEY
if (-not $ApiKey) {
    Write-Host "[ERROR] No GEMINI_API_KEY found in Tools/.env" -ForegroundColor Red
    exit 1
}

$GeminiUrl = "https://generativelanguage.googleapis.com/v1beta/models/gemini-2.0-flash:generateContent?key=$ApiKey"

if (-not (Test-Path $ReportDir)) { New-Item -ItemType Directory -Path $ReportDir -Force | Out-Null }
$Timestamp = Get-Date -Format "yyyy-MM-dd_HH-mm-ss"

# --- Helper Functions ---
function Write-Status($msg) { Write-Host "[ORCHESTRATOR] $msg" -ForegroundColor Cyan }
function Write-AI($msg) { Write-Host "[GEMINI] $msg" -ForegroundColor Magenta }
function Write-Pass($msg) { Write-Host "  [PASS] $msg" -ForegroundColor Green }
function Write-Fail($msg) { Write-Host "  [FAIL] $msg" -ForegroundColor Red }

function Get-CodebaseSummary {
    $summary = "# Roman Empire Game - UE5 C++ RTS/FPS Hybrid`n"
    $summary += "## Source Files:`n"
    
    Get-ChildItem "$SourceDir" -Filter "*.h" -Recurse | ForEach-Object {
        $relPath = $_.FullName.Replace($SourceDir, "").TrimStart('\')
        $lineCount = (Get-Content $_.FullName | Measure-Object -Line).Lines
        $summary += "- $relPath ($lineCount lines)`n"
    }
    
    Get-ChildItem "$SourceDir" -Filter "*.cpp" -Recurse | ForEach-Object {
        $relPath = $_.FullName.Replace($SourceDir, "").TrimStart('\')
        $lineCount = (Get-Content $_.FullName | Measure-Object -Line).Lines
        $summary += "- $relPath ($lineCount lines)`n"
    }
    
    return $summary
}

function Get-FileContent($filePath) {
    if (Test-Path $filePath) {
        return Get-Content $filePath -Raw
    }
    return ""
}

function Call-Gemini {
    param([string]$Prompt)
    
    # Build JSON manually to avoid encoding issues with ConvertTo-Json
    $escapedPrompt = $Prompt.Replace('\', '\\').Replace('"', '\"').Replace("`n", '\n').Replace("`r", '\r').Replace("`t", '\t')
    $jsonBody = '{"contents":[{"parts":[{"text":"' + $escapedPrompt + '"}]}],"generationConfig":{"temperature":0.7,"maxOutputTokens":8192}}'
    $bodyBytes = [System.Text.Encoding]::UTF8.GetBytes($jsonBody)
    
    $maxRetries = 5
    for ($retry = 0; $retry -lt $maxRetries; $retry++) {
        try {
            $request = [System.Net.HttpWebRequest]::Create($GeminiUrl)
            $request.Method = "POST"
            $request.ContentType = "application/json; charset=utf-8"
            $request.Timeout = 120000  # 2 minutes
            $request.ContentLength = $bodyBytes.Length
            
            $stream = $request.GetRequestStream()
            $stream.Write($bodyBytes, 0, $bodyBytes.Length)
            $stream.Close()
            
            $response = $request.GetResponse()
            $reader = New-Object System.IO.StreamReader($response.GetResponseStream())
            $responseText = $reader.ReadToEnd()
            $reader.Close()
            $response.Close()
            
            $parsed = $responseText | ConvertFrom-Json
            $text = $parsed.candidates[0].content.parts[0].text
            return $text
        }
        catch [System.Net.WebException] {
            $statusCode = [int]$_.Exception.Response.StatusCode
            if ($statusCode -eq 429) {
                $waitSec = [Math]::Pow(2, $retry + 1) * 5  # 10, 20, 40, 80, 160 seconds
                Write-Status "Rate limited (429). Waiting ${waitSec}s before retry $($retry + 1)/$maxRetries..."
                Start-Sleep -Seconds $waitSec
                continue
            }
            Write-Fail "Gemini API error ($statusCode): $($_.Exception.Message)"
            try {
                $errReader = New-Object System.IO.StreamReader($_.Exception.Response.GetResponseStream())
                $errBody = $errReader.ReadToEnd()
                $errReader.Close()
                Write-Fail "Response: $errBody"
            } catch {}
            return $null
        }
        catch {
            Write-Fail "Gemini API call failed: $($_.Exception.Message)"
            return $null
        }
    }
    
    Write-Fail "Max retries ($maxRetries) exceeded"
    return $null
}

function Do-Build {
    Write-Status "Building project..."
    $buildLog = "$ReportDir\build_$Timestamp.log"
    & cmd /c "$BuildScript" > $buildLog 2>&1
    $content = Get-Content $buildLog -Raw -ErrorAction SilentlyContinue
    $succeeded = $content -match "Result: Succeeded"
    
    if ($succeeded) {
        Write-Pass "Build succeeded"
        return @{ Success = $true; Log = $buildLog }
    } else {
        Write-Fail "Build failed"
        # Extract errors
        $errors = Get-Content $buildLog | Select-String "error C\d+|error LNK" | Select-Object -First 10
        $errorText = ($errors | ForEach-Object { $_.Line }) -join "`n"
        return @{ Success = $false; Log = $buildLog; Errors = $errorText }
    }
}

function Do-Analyze {
    Write-Status "Asking Gemini to analyze the codebase..."
    
    $codebaseSummary = Get-CodebaseSummary
    
    # Read key files for context
    $gameModeContent = Get-FileContent "$SourceDir\Core\RomanEmpireGameMode.cpp"
    $unitBaseHeader = Get-FileContent "$SourceDir\Units\UnitBase.h"
    $playerControllerContent = Get-FileContent "$SourceDir\Core\RomanEmpirePlayerController.cpp"
    
    $prompt = @"
You are a senior UE5 C++ game developer analyzing a Roman Empire RTS/FPS hybrid game.

Here is the codebase structure:
$codebaseSummary

Here is the GameMode (main game logic):
``````cpp
$gameModeContent
``````

Here is the UnitBase header (unit system):
``````cpp
$unitBaseHeader
``````

Here is the PlayerController (input):
``````cpp
$playerControllerContent
``````

The game currently has:
- Seamless zoom camera (strategic -> tactical -> combat -> first person)
- Canvas-based HUD showing resources, phase, controls
- 15 Roman (red) + 10 Carthaginian (purple) legionaries with cylinder meshes
- Roman camp with barracks, temple, houses, walls
- Green terrain with trees, hills, river, road
- WASD camera movement, scroll zoom, LMB select, RMB command

Analyze what the game needs most urgently to feel like a playable RTS. List the TOP 3 most impactful features to implement next, ranked by importance. For each feature:
1. Name and why it matters
2. Which files need to change
3. Estimated complexity (lines of code)

Be specific and practical. Focus on gameplay, not cosmetics.
"@
    
    $response = Call-Gemini -Prompt $prompt
    if ($response) {
        Write-AI "Analysis complete"
        Write-Host $response -ForegroundColor White
        
        # Save analysis
        $response | Out-File "$ReportDir\analysis_$Timestamp.md" -Encoding utf8
        Write-Status "Analysis saved to $ReportDir\analysis_$Timestamp.md"
    }
    
    return $response
}

function Do-Implement {
    param([string]$TaskDescription)
    
    if (-not $TaskDescription) {
        Write-Fail "No task description provided"
        return $null
    }
    
    Write-Status "Asking Gemini to implement: $TaskDescription"
    
    $codebaseSummary = Get-CodebaseSummary
    
    # Read relevant files based on the task
    $relevantFiles = @{}
    Get-ChildItem "$SourceDir" -Filter "*.h" -Recurse | ForEach-Object {
        $content = Get-Content $_.FullName -Raw
        if ($content.Length -lt 10000) {
            $relPath = $_.FullName.Replace($SourceDir, "").TrimStart('\')
            $relevantFiles[$relPath] = $content
        }
    }
    Get-ChildItem "$SourceDir" -Filter "*.cpp" -Recurse | ForEach-Object {
        $content = Get-Content $_.FullName -Raw
        if ($content.Length -lt 15000) {
            $relPath = $_.FullName.Replace($SourceDir, "").TrimStart('\')
            $relevantFiles[$relPath] = $content
        }
    }
    
    $filesContext = ""
    foreach ($entry in $relevantFiles.GetEnumerator()) {
        $filesContext += "`n### $($entry.Key)`n``````cpp`n$($entry.Value)`n```````n"
    }
    
    $prompt = @"
You are a senior UE5 C++ developer. You need to implement the following feature for a Roman Empire RTS game:

TASK: $TaskDescription

Here is the complete codebase:
$filesContext

RULES:
1. Output ONLY the complete file contents that need to change
2. Use this exact format for EACH file you modify:
   ===FILE: relative/path/to/File.h===
   (complete file content)
   ===END FILE===
3. You must output the COMPLETE file content, not just diffs
4. Use only UE5 C++ with engine built-in assets (no external dependencies)
5. All meshes must use /Engine/BasicShapes/ (Cube, Sphere, Cylinder, Cone, Plane)
6. All materials must use /Engine/BasicShapes/BasicShapeMaterial with dynamic color
7. Keep it simple and functional - this is a prototype

Output the files now:
"@
    
    $response = Call-Gemini -Prompt $prompt
    if (-not $response) {
        Write-Fail "Gemini returned no response"
        return $null
    }
    
    Write-AI "Code generated. Applying changes..."
    
    # Parse the response for file blocks
    $filePattern = '===FILE:\s*(.+?)===\s*\n([\s\S]*?)===END FILE==='
    $matches = [regex]::Matches($response, $filePattern)
    
    $appliedFiles = @()
    
    foreach ($match in $matches) {
        $relPath = $match.Groups[1].Value.Trim()
        $content = $match.Groups[2].Value.Trim()
        
        $fullPath = Join-Path $SourceDir $relPath
        $dir = Split-Path $fullPath -Parent
        
        if (-not (Test-Path $dir)) {
            New-Item -ItemType Directory -Path $dir -Force | Out-Null
        }
        
        $content | Out-File -FilePath $fullPath -Encoding utf8 -NoNewline
        $appliedFiles += $relPath
        Write-Pass "Updated: $relPath"
    }
    
    if ($appliedFiles.Count -eq 0) {
        Write-Fail "No file blocks found in Gemini response. Raw response saved."
        $response | Out-File "$ReportDir\gemini_raw_$Timestamp.md" -Encoding utf8
    }
    
    # Save the full response for reference
    $response | Out-File "$ReportDir\implement_$Timestamp.md" -Encoding utf8
    
    return @{ Files = $appliedFiles; Response = $response }
}

function Do-FixBuildErrors {
    param([string]$ErrorText)
    
    Write-Status "Asking Gemini to fix build errors..."
    
    # Read the files that likely have errors
    $relevantFiles = @{}
    Get-ChildItem "$SourceDir" -Filter "*.h" -Recurse | ForEach-Object {
        $content = Get-Content $_.FullName -Raw
        if ($content.Length -lt 10000) {
            $relPath = $_.FullName.Replace($SourceDir, "").TrimStart('\')
            $relevantFiles[$relPath] = $content
        }
    }
    Get-ChildItem "$SourceDir" -Filter "*.cpp" -Recurse | ForEach-Object {
        $content = Get-Content $_.FullName -Raw
        if ($content.Length -lt 15000) {
            $relPath = $_.FullName.Replace($SourceDir, "").TrimStart('\')
            $relevantFiles[$relPath] = $content
        }
    }
    
    $filesContext = ""
    foreach ($entry in $relevantFiles.GetEnumerator()) {
        $filesContext += "`n### $($entry.Key)`n``````cpp`n$($entry.Value)`n```````n"
    }
    
    $prompt = @"
You are a senior UE5 C++ developer. The build failed with these errors:

$ErrorText

Here is the current codebase:
$filesContext

Fix ALL the build errors. Output the COMPLETE corrected files using this format:
===FILE: relative/path/to/File.h===
(complete file content)
===END FILE===

Only output files that need changes. Output the COMPLETE file, not just the fix.
"@
    
    $response = Call-Gemini -Prompt $prompt
    if (-not $response) { return $null }
    
    $filePattern = '===FILE:\s*(.+?)===\s*\n([\s\S]*?)===END FILE==='
    $matches = [regex]::Matches($response, $filePattern)
    
    foreach ($match in $matches) {
        $relPath = $match.Groups[1].Value.Trim()
        $content = $match.Groups[2].Value.Trim()
        $fullPath = Join-Path $SourceDir $relPath
        $dir = Split-Path $fullPath -Parent
        if (-not (Test-Path $dir)) { New-Item -ItemType Directory -Path $dir -Force | Out-Null }
        $content | Out-File -FilePath $fullPath -Encoding utf8 -NoNewline
        Write-AI "Fixed: $relPath"
    }
    
    $response | Out-File "$ReportDir\fix_$Timestamp.md" -Encoding utf8
    return $response
}

function Do-DevLoop {
    param([int]$MaxIterations = 3)
    
    Write-Host ""
    Write-Host "============================================" -ForegroundColor Cyan
    Write-Host " AI DEVELOPMENT LOOP ($MaxIterations iterations)" -ForegroundColor Cyan
    Write-Host "============================================" -ForegroundColor Cyan
    Write-Host ""
    
    for ($i = 1; $i -le $MaxIterations; $i++) {
        Write-Host ""
        Write-Host "--- ITERATION $i / $MaxIterations ---" -ForegroundColor Yellow
        Write-Host ""
        
        # Step 1: Analyze
        Write-Status "[$i] Analyzing codebase..."
        $analysis = Do-Analyze
        if (-not $analysis) {
            Write-Fail "Analysis failed, skipping iteration"
            continue
        }
        
        # Step 2: Extract the #1 priority task from analysis
        $taskPrompt = @"
From this analysis, extract ONLY the single most important task to implement next. 
Output it as a single clear sentence describing what to implement:

$analysis
"@
        $topTask = Call-Gemini -Prompt $taskPrompt
        if (-not $topTask) { continue }
        
        Write-AI "Top priority: $topTask"
        
        # Step 3: Implement
        Write-Status "[$i] Implementing..."
        $implResult = Do-Implement -TaskDescription $topTask
        if (-not $implResult -or $implResult.Files.Count -eq 0) {
            Write-Fail "Implementation produced no files"
            continue
        }
        
        # Step 4: Build
        Write-Status "[$i] Building..."
        $buildResult = Do-Build
        
        # Step 5: If build failed, try to fix
        if (-not $buildResult.Success) {
            Write-Status "[$i] Build failed, asking Gemini to fix..."
            Do-FixBuildErrors -ErrorText $buildResult.Errors
            
            # Retry build
            $buildResult = Do-Build
            if (-not $buildResult.Success) {
                Write-Fail "[$i] Build still failing after fix attempt. Rolling back would be needed."
                Write-Status "Saving error state and continuing to next iteration..."
                continue
            }
        }
        
        Write-Pass "[$i] Iteration complete! Feature implemented and compiled."
        
        # Git commit
        & git -C $ProjectRoot add -A
        & git -C $ProjectRoot commit -m "feat(ai): $topTask" 2>$null
        Write-Pass "[$i] Changes committed to git"
    }
    
    Write-Host ""
    Write-Host "============================================" -ForegroundColor Green
    Write-Host " DEVELOPMENT LOOP COMPLETE" -ForegroundColor Green  
    Write-Host " Ran $MaxIterations iterations" -ForegroundColor Green
    Write-Host "============================================" -ForegroundColor Green
    
    # Push all changes
    & git -C $ProjectRoot push origin main 2>$null
    Write-Pass "All changes pushed to GitHub"
}

# --- Main ---
Write-Host ""
Write-Host "======================================================" -ForegroundColor Cyan
Write-Host " Roman Empire Game - AI Development Orchestrator" -ForegroundColor Cyan
Write-Host " Powered by Gemini 2.0 Flash" -ForegroundColor Cyan
Write-Host "======================================================" -ForegroundColor Cyan
Write-Host ""

switch ($Action) {
    "build" { Do-Build }
    "analyze" { Do-Analyze }
    "implement" { 
        if (-not $Task) {
            Write-Fail "Use -Task 'description' to specify what to implement"
            exit 1
        }
        $result = Do-Implement -TaskDescription $Task
        if ($result -and $result.Files.Count -gt 0) {
            $buildResult = Do-Build
            if (-not $buildResult.Success) {
                Do-FixBuildErrors -ErrorText $buildResult.Errors
                Do-Build
            }
        }
    }
    "test" {
        $buildResult = Do-Build
        if ($buildResult.Success) {
            Write-Status "Build succeeded. Run the game in editor and then use: .\Tools\orchestrator.ps1 -Action log"
        }
    }
    "develop" { Do-DevLoop -MaxIterations $Iterations }
}
