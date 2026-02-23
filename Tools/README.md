# Roman Empire Game — Orchestrator

## Overview
Automated test and iteration system for the Roman Empire Game.
Launches the game, collects logs, identifies issues, and creates reports.

## Usage
```powershell
# From project root:
.\Tools\orchestrator.ps1 -Action test      # Run full test suite
.\Tools\orchestrator.ps1 -Action launch     # Launch in game mode
.\Tools\orchestrator.ps1 -Action log        # Analyze latest log
.\Tools\orchestrator.ps1 -Action build      # Build and test
```

## What it Tests
1. **Build** — Compiles and checks for errors
2. **Launch** — Runs game in standalone mode with logging
3. **Log Analysis** — Parses logs for our custom `LogRomanEmpire` messages
4. **Verification** — Checks that GameMode, Camera, HUD, Units all initialized
5. **Report** — Generates test report with pass/fail status
