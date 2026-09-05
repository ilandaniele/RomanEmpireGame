<#
.SYNOPSIS
    Compatibility wrapper for repository validation and Unreal automation.
.DESCRIPTION
    The former log-string orchestrator was retired during M1 recovery. This
    wrapper delegates only to the authoritative repository/build entry points.
#>

[CmdletBinding()]
param(
    [ValidateSet('validate', 'build', 'test')]
    [string]$Action = 'validate',

    [string]$EngineRoot
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

$repositoryRoot = Split-Path -Parent $PSScriptRoot
$buildProject = Join-Path $repositoryRoot 'BuildProject.ps1'

if ($Action -eq 'validate') {
    & $buildProject -Action Validate -EngineRoot $EngineRoot
}
elseif ($Action -eq 'build') {
    & $buildProject -Action Editor -EngineRoot $EngineRoot
}
elseif ($Action -eq 'test') {
    & $buildProject -Action Test -EngineRoot $EngineRoot
}

exit $LASTEXITCODE
