[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)]
    [string]$ReportDirectory,
    [string[]]$RequiredTests = @()
)
Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot 'AutomationReport.ps1')
Assert-AutomationReportPassed -ReportDirectory $ReportDirectory -RequiredTests $RequiredTests
