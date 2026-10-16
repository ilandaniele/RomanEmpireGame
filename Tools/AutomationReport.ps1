# Pure report validation. Dot-sourcing defines the function; no engine/process/file writes.
function Assert-AutomationReportPassed {
    [CmdletBinding()]
    param(
        [Parameter(Mandatory = $true)]
        [string]$ReportDirectory,
        [string[]]$RequiredTests = @()
    )

    $reportPath = Join-Path $ReportDirectory 'index.json'
    if (-not (Test-Path -LiteralPath $reportPath -PathType Leaf)) {
        throw "Automation completed without the expected report: $reportPath"
    }
    if ((Get-Item -LiteralPath $reportPath).Length -gt 64MB) {
        throw "Automation report exceeds the 64 MiB bound: $reportPath"
    }
    try {
        $report = Get-Content -LiteralPath $reportPath -Raw -Encoding UTF8 | ConvertFrom-Json
    }
    catch {
        throw "Automation report is not valid JSON: $reportPath"
    }

    function Read-ReportCount {
        param($Object, [string]$Name)
        if ($null -eq $Object) { throw "Missing report object for $Name" }
        $property = $Object.PSObject.Properties[$Name]
        if ($null -eq $property -or
            ($property.Value -isnot [int] -and $property.Value -isnot [long]) -or
            $property.Value -lt 0 -or $property.Value -gt [int]::MaxValue) {
            throw "Unsupported automation count: $Name must be a nonnegative integer"
        }
        return [long]$property.Value
    }

    $counts = @{}
    foreach ($name in @('succeeded', 'succeededWithWarnings', 'failed', 'notRun', 'inProcess')) {
        $counts[$name] = Read-ReportCount -Object $report -Name $name
    }
    if ($counts.failed -ne 0 -or $counts.notRun -ne 0 -or $counts.inProcess -ne 0) {
        throw "Automation contains failed or unfinished tests: $reportPath"
    }
    $testsProperty = $report.PSObject.Properties['tests']
    if ($null -eq $testsProperty -or $testsProperty.Value -isnot [array] -or
        $testsProperty.Value.Count -lt 1) {
        throw "Automation must contain a nonempty test array: $reportPath"
    }

    $paths = [Collections.Generic.HashSet[string]]::new([StringComparer]::OrdinalIgnoreCase)
    $clean = 0L
    $warned = 0L
    foreach ($test in $testsProperty.Value) {
        if ($null -eq $test) { throw 'Automation contains a null test' }
        $pathProperty = $test.PSObject.Properties['fullTestPath']
        $stateProperty = $test.PSObject.Properties['state']
        if ($null -eq $pathProperty -or $pathProperty.Value -isnot [string] -or
            -not $pathProperty.Value.StartsWith('RomanEmpireGame.', [StringComparison]::Ordinal) -or
            $pathProperty.Value.Length -le 'RomanEmpireGame.'.Length -or
            -not $paths.Add($pathProperty.Value)) {
            throw 'Automation contains an invalid, foreign or duplicate test path'
        }
        if ($null -eq $stateProperty -or $stateProperty.Value -cne 'Success') {
            throw "Automation test did not succeed: $($pathProperty.Value)"
        }
        $errors = Read-ReportCount -Object $test -Name 'errors'
        $warnings = Read-ReportCount -Object $test -Name 'warnings'
        if ($errors -ne 0) { throw "Successful test contains errors: $($pathProperty.Value)" }
        if ($warnings -gt 0) { $warned++ } else { $clean++ }
    }
    if ($clean -ne $counts.succeeded -or $warned -ne $counts.succeededWithWarnings) {
        throw "Automation summary disagrees with actual per-test results: $reportPath"
    }
    foreach ($required in $RequiredTests) {
        if ([string]::IsNullOrWhiteSpace($required) -or -not $paths.Contains($required)) {
            throw "Required automation test was not executed: $required"
        }
    }
    Write-Host "Automation report passed: $($clean + $warned) succeeded ($warned with warnings), 0 failed." -ForegroundColor Green
}
