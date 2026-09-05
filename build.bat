@echo off
setlocal

rem Compatibility entry point. BuildProject.ps1 owns discovery and diagnostics.
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0BuildProject.ps1" -Action Editor %*
set EXIT_CODE=%ERRORLEVEL%

endlocal & exit /b %EXIT_CODE%
