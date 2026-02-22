@echo off
setlocal

set PROJECT_PATH=%~dp0RomanEmpireGame.uproject
set UBT_PATH=C:\Program Files\Epic Games\UE_5.7\Engine\Binaries\DotNET\UnrealBuildTool\UnrealBuildTool.dll
set DOTNET_PATH=C:\Program Files\Epic Games\UE_5.7\Engine\Binaries\ThirdParty\DotNet\8.0.412\win-x64\dotnet.exe

echo Building RomanEmpireGameEditor...

"%DOTNET_PATH%" "%UBT_PATH%" RomanEmpireGameEditor Win64 Development -Project="%PROJECT_PATH%" -progress

echo EXIT_CODE=%ERRORLEVEL%

endlocal
