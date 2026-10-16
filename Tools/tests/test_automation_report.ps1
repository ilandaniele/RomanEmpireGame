[CmdletBinding()]
param()
Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot '../AutomationReport.ps1')

$repositoryRoot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../..'))
$fixtureParent = [IO.Path]::GetFullPath((Join-Path $repositoryRoot 'Saved/Automation'))
$fixtureRoot = [IO.Path]::GetFullPath((Join-Path $fixtureParent ('ReportContract_' + [Guid]::NewGuid().ToString('N'))))
if (-not $fixtureRoot.StartsWith($fixtureParent + [IO.Path]::DirectorySeparatorChar,
                                [StringComparison]::OrdinalIgnoreCase)) {
    throw 'Fixture escaped the allowed automation root'
}
New-Item -ItemType Directory -Path $fixtureRoot -Force | Out-Null
$fixtureFile = Join-Path $fixtureRoot 'index.json'
$checks = 0

function New-ValidReport {
    return [PSCustomObject]@{
        succeeded = 1; succeededWithWarnings = 0; failed = 0; notRun = 0; inProcess = 0
        tests = @([PSCustomObject]@{
            fullTestPath = 'RomanEmpireGame.Synthetic.Invariant'; state = 'Success'; errors = 0; warnings = 0
        })
    }
}
function Write-Fixture {
    param($Report)
    $Report | ConvertTo-Json -Depth 12 | Set-Content -LiteralPath $fixtureFile -Encoding UTF8
}
function Assert-Rejected {
    param([string]$Name, [scriptblock]$Mutation)
    $case = New-ValidReport
    & $Mutation $case
    Write-Fixture $case
    $rejected = $false
    try { Assert-AutomationReportPassed -ReportDirectory $fixtureRoot } catch { $rejected = $true }
    if (-not $rejected) { throw "Invalid automation result accepted: $Name" }
    $script:checks++
}

try {
    Write-Fixture (New-ValidReport)
    Assert-AutomationReportPassed -ReportDirectory $fixtureRoot -RequiredTests @('RomanEmpireGame.Synthetic.Invariant')
    $checks++
    $warned = New-ValidReport
    $warned.succeeded = 0; $warned.succeededWithWarnings = 1; $warned.tests[0].warnings = 2
    Write-Fixture $warned
    Assert-AutomationReportPassed -ReportDirectory $fixtureRoot
    $checks++

    Assert-Rejected 'failed assertion' { param($r) $r.failed = 1; $r.succeeded = 0; $r.tests[0].state = 'Fail'; $r.tests[0].errors = 1 }
    Assert-Rejected 'hidden failed test' { param($r) $r.tests[0].state = 'Fail' }
    Assert-Rejected 'error despite success state' { param($r) $r.tests[0].errors = 1 }
    Assert-Rejected 'pending test' { param($r) $r.inProcess = 1 }
    Assert-Rejected 'not run' { param($r) $r.notRun = 1 }
    Assert-Rejected 'missing pending count' { param($r) $r.PSObject.Properties.Remove('inProcess') }
    Assert-Rejected 'negative summary' { param($r) $r.failed = -1 }
    Assert-Rejected 'numeric string' { param($r) $r.succeeded = '1' }
    Assert-Rejected 'bool count' { param($r) $r.failed = $false }
    Assert-Rejected 'fractional count' { param($r) $r.failed = 0.5 }
    Assert-Rejected 'overflow count' { param($r) $r.succeeded = [long][int]::MaxValue + 1 }
    Assert-Rejected 'counter does not match tests' { param($r) $r.succeeded = 2 }
    Assert-Rejected 'warning counter mismatch' { param($r) $r.tests[0].warnings = 1 }
    Assert-Rejected 'empty suite' { param($r) $r.tests = @(); $r.succeeded = 0 }
    Assert-Rejected 'missing tests' { param($r) $r.PSObject.Properties.Remove('tests') }
    Assert-Rejected 'null row' { param($r) $r.tests = @($null) }
    Assert-Rejected 'duplicate row' { param($r) $r.tests = @($r.tests[0], $r.tests[0]); $r.succeeded = 2 }
    Assert-Rejected 'foreign suite' { param($r) $r.tests[0].fullTestPath = 'OtherGame.Rule' }
    Assert-Rejected 'missing error count' { param($r) $r.tests[0].PSObject.Properties.Remove('errors') }

    Write-Fixture (New-ValidReport)
    $missingRequiredRejected = $false
    try { Assert-AutomationReportPassed -ReportDirectory $fixtureRoot -RequiredTests @('RomanEmpireGame.Missing.Required') }
    catch { $missingRequiredRejected = $true }
    if (-not $missingRequiredRejected) { throw 'Required coverage was not enforced' }
    $checks++

    Set-Content -LiteralPath $fixtureFile -Value '{not-json' -Encoding UTF8
    $invalidJsonRejected = $false
    try { Assert-AutomationReportPassed -ReportDirectory $fixtureRoot } catch { $invalidJsonRejected = $true }
    if (-not $invalidJsonRejected) { throw 'Malformed JSON accepted' }
    $checks++

    # Real process boundary: the same CLI used by tooling must exit nonzero for
    # an actual failed assertion, not merely print an error while returning zero.
    $broken = New-ValidReport
    $broken.tests[0].errors = 1
    Write-Fixture $broken
    $validator = Join-Path $repositoryRoot 'Tools/validate_automation_report.ps1'
    $nativeErrorPreference = $ErrorActionPreference
    $previousNativeExit = 0
    $nativeExitVariable = Get-Variable -Name LASTEXITCODE -Scope Global -ErrorAction SilentlyContinue
    if ($null -ne $nativeExitVariable) { $previousNativeExit = $nativeExitVariable.Value }
    try {
        # Windows PowerShell wraps expected child stderr in NativeCommandError.
        # Capture it without interrupting this negative fixture before exit readback.
        $ErrorActionPreference = 'Continue'
        $childOutput = & powershell.exe -NoProfile -File $validator -ReportDirectory $fixtureRoot 2>&1
        $childExit = $LASTEXITCODE
    }
    finally {
        $ErrorActionPreference = $nativeErrorPreference
        # The expected exit1 must not leak into GitHub's shell exit-code epilogue.
        $global:LASTEXITCODE = $previousNativeExit
    }
    if ($childExit -eq 0) { throw 'Broken invariant did not fail the CLI process' }
    if (($childOutput | Out-String) -notmatch 'Successful test contains errors') {
        throw 'Child failed for an unrelated reason rather than the broken assertion'
    }
    $checks++

    Remove-Item -LiteralPath $fixtureFile
    $missingRejected = $false
    try { Assert-AutomationReportPassed -ReportDirectory $fixtureRoot } catch { $missingRejected = $true }
    if (-not $missingRejected) { throw 'Missing report accepted' }
    $checks++
    Write-Host "Automation report contract: $checks checks passed; broken invariant child exit=$childExit."
}
finally {
    # Only two exact owned paths; no recursive deletion, globs, or external files.
    if (Test-Path -LiteralPath $fixtureFile) { Remove-Item -LiteralPath $fixtureFile }
    Remove-Item -LiteralPath $fixtureRoot
}
