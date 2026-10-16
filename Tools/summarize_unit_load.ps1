[CmdletBinding()]
param(
    [ValidatePattern('^[A-Za-z0-9_]{1,48}$')]
    [string]$CapturePrefix = 'C018B',
    [ValidateRange(1, 10)]
    [int]$Runs = 3,
    [switch]$Encounter
)
Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
$repositoryRoot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
$artifactRoot = Join-Path $repositoryRoot 'Artifacts'
$outputRoot = Join-Path $artifactRoot "Profiling/$CapturePrefix"
$csvRoot = Join-Path $repositoryRoot 'Saved/Profiling/CSV'
. (Join-Path $PSScriptRoot 'AutomationReport.ps1')
if (-not (Test-Path -LiteralPath $outputRoot -PathType Container)) {
    throw 'Capture directory must already exist; run the owned capture first'
}
foreach ($checkedRoot in @($artifactRoot, (Join-Path $artifactRoot 'Profiling'), $outputRoot,
                          (Join-Path $repositoryRoot 'Saved'), (Join-Path $repositoryRoot 'Saved/Profiling'), $csvRoot)) {
    if ((Get-Item -LiteralPath $checkedRoot -Force).Attributes -band [IO.FileAttributes]::ReparsePoint) {
        throw "Capture root is redirected: $checkedRoot"
    }
}
$summaryFile = Join-Path $outputRoot 'summary.json'
if (Test-Path -LiteralPath $summaryFile) { throw 'Refusing to overwrite an existing capture summary' }

function Summarize-Values {
    param([double[]]$Values, [ValidateRange(1, 240)][int]$MinimumSamples = 240)
    $sorted = @($Values | Sort-Object)
    if ($sorted.Count -lt $MinimumSamples) { throw "Need at least $MinimumSamples numeric samples per metric" }
    return [ordered]@{
        samples = $sorted.Count
        p50 = $sorted[[int][Math]::Floor(($sorted.Count - 1) * .50)]
        p95 = $sorted[[int][Math]::Floor(($sorted.Count - 1) * .95)]
        max = $sorted[-1]
    }
}
function Parse-Metric {
    param($Row, [string]$Name)
    $property = $Row.PSObject.Properties[$Name]
    $value = 0.0
    if ($null -eq $property -or
        -not [double]::TryParse($property.Value, [Globalization.NumberStyles]::Float,
            [Globalization.CultureInfo]::InvariantCulture, [ref]$value) -or
        [double]::IsNaN($value) -or [double]::IsInfinity($value) -or $value -lt 0) {
        throw "Missing/invalid CSV metric: $Name"
    }
    return $value
}

function New-ProfileHeaders {
    param([string[]]$Headers)
    $seen = [Collections.Generic.HashSet[string]]::new([StringComparer]::OrdinalIgnoreCase)
    $reserved = [Collections.Generic.HashSet[string]]::new([StringComparer]::OrdinalIgnoreCase)
    foreach ($header in $Headers) { $null = $reserved.Add($header) }
    for ($index = 0; $index -lt $Headers.Count; $index++) {
        $header = $Headers[$index]
        if ([string]::IsNullOrWhiteSpace($header) -or $header.Contains('"')) { throw 'Unsupported/empty Unreal CSV header' }
        if ($seen.Add($header)) { $header; continue }
        if ($header -in @('FrameTime','GameThreadTime','RenderThreadTime','GPUTime','Exclusive/GameThread/CharacterMovement') -or
            $header.StartsWith('DrawCall/', [StringComparison]::OrdinalIgnoreCase) -or
            $header.StartsWith('RomanEncounter/', [StringComparison]::OrdinalIgnoreCase)) { throw "Ambiguous measured CSV column: $header" }
        # UE can repeat unrelated streaming stats; retain both positions, never silently drop a measured column.
        $alias = "${header}#Duplicate$index"
        while ($reserved.Contains($alias) -or -not $seen.Add($alias)) { $alias += '_' }
        $alias
    }
}

function Read-EncounterCounters {
    param($Row, [ValidateSet(50,100)][int]$Total)
    $result = [ordered]@{}
    foreach ($name in @('Phase','AliveUnits','TrainingActive','Casualties','CombatDuringTraining')) {
        $value = Parse-Metric $Row "RomanEncounter/$name"
        if ($value -ne [Math]::Floor($value)) { throw 'Encounter counters must be integers' }
        $result[$name] = [int]$value
    }
    if ($result.Phase -notin @(0,2,3,4) -or $result.TrainingActive -notin @(0,1) -or
        $result.CombatDuringTraining -notin @(0,1) -or $result.AliveUnits -gt $Total + 1 -or
        $result.Casualties -gt $Total + 1 -or ($result.Phase -gt 0 -and $result.AliveUnits -lt 1)) {
        throw 'Invalid encounter telemetry counters'
    }
    return [PSCustomObject]$result
}

$captures = @()
$kind = if ($Encounter) { 'Encounter' } else { 'Load' }
foreach ($run in 1..$Runs) {
    foreach ($count in @(50,100)) {
        $name = "${CapturePrefix}_${kind}${count}_Run${run}"
        Assert-AutomationReportPassed -ReportDirectory (Join-Path $artifactRoot "Automation/$name") `
            -RequiredTests @("RomanEmpireGame.Performance.${kind}${count}PIE")
        $logPath = Join-Path $artifactRoot "$name.log"
        $log = Get-Content -LiteralPath $logPath -Raw -Encoding UTF8
        $matches = [regex]::Matches($log, 'Writing CSV to file\s*:\s*[^\r\n]*?(Profile\([0-9]{8}_[0-9]{6}\)\.csv)')
        if ($matches.Count -ne 1) { throw "Expected one actual CSV receipt for $name" }
        $sourceCsv = Join-Path $csvRoot $matches[0].Groups[1].Value
        if ((Get-Item -LiteralPath $sourceCsv -Force).Attributes -band [IO.FileAttributes]::ReparsePoint) {
            throw 'Source CSV must not be a redirected file'
        }
        $trace = Join-Path $outputRoot "$name.utrace"
        if ((Get-Item -LiteralPath $trace).Length -lt 1) { throw "Missing/empty actual trace: $trace" }
        $workload = $null
        if ($Encounter) {
            if (-not [regex]::IsMatch($log, "EncounterViewport${count}: 1920x1080; projected Roman centers$($count - 10)/$($count - 10)")) {
                throw 'Gameplay viewport/full initial Roman-center projection was not verified'
            }
            $receipts = [regex]::Matches($log, "Encounter${count}: initial TOTAL${count}; added([0-9]+); peak sampled live([0-9]+); final live([0-9]+); casualties([0-9]+); natural recruit1; combat during training; FPS realtime; physical victory; ([0-9]+) measured PIE frames; ([0-9.]+) wall seconds; projected Roman centers([0-9]+)/([0-9]+)")
            if ($receipts.Count -ne 1) { throw "Missing/ambiguous real encounter completion receipt for $name" }
            $receipt = $receipts[0]
            $workload = [ordered]@{
                addedUnits = [int]$receipt.Groups[1].Value; peakSampledLive = [int]$receipt.Groups[2].Value
                finalLive = [int]$receipt.Groups[3].Value; casualties = [int]$receipt.Groups[4].Value
                measuredPIEFrames = [int]$receipt.Groups[5].Value
                wallSeconds = [double]::Parse($receipt.Groups[6].Value, [Globalization.CultureInfo]::InvariantCulture)
                projectedRomanCenters = [int]$receipt.Groups[7].Value; initialRomans = [int]$receipt.Groups[8].Value
            }
            if ($workload.addedUnits -ne $count - 25 -or $workload.initialRomans -ne $count - 10 -or
                $workload.projectedRomanCenters -gt $workload.initialRomans -or
                $workload.peakSampledLive -lt $count -or $workload.peakSampledLive -gt $count + 1 -or
                $workload.finalLive -lt 1 -or $workload.finalLive -gt $count + 1 -or
                $workload.casualties -lt 10 -or $workload.finalLive + $workload.casualties -ne $count + 1) {
                throw "Inconsistent canonical encounter receipt for $name"
            }
        } else {
            $view = [regex]::Match($log, "M3Load${count}: 240 sampled PIE frames,.*? ([0-9]+)/${count} units initially inside viewport")
            if (-not $view.Success -or [int]$view.Groups[1].Value -ne $count) {
                throw "Added units were not all inside the real viewport for $name"
            }
        }
        if ((Get-Item -LiteralPath $sourceCsv).Length -gt 512MB) { throw 'CSV exceeds bounded parser size (512MiB)' }
        $headers = @(New-ProfileHeaders -Headers (Get-Content -LiteralPath $sourceCsv -TotalCount 1).Split(','))
        $selectedColumns = @($headers | Where-Object {
            $_ -in @('FrameTime','GameThreadTime','RenderThreadTime','GPUTime','Exclusive/GameThread/CharacterMovement') -or
            $_.StartsWith('DrawCall/', [StringComparison]::OrdinalIgnoreCase) -or
            $_.StartsWith('RomanEncounter/', [StringComparison]::OrdinalIgnoreCase)
        })
        $rows = @(Get-Content -LiteralPath $sourceCsv | Select-Object -Skip 1 | ConvertFrom-Csv -Header $headers | Where-Object {
            # UE appends a repeated header and metadata. Neither is a numeric frame.
            $frame = 0.0
            [double]::TryParse($_.FrameTime, [Globalization.NumberStyles]::Float,
                [Globalization.CultureInfo]::InvariantCulture, [ref]$frame)
        } | Select-Object -Property $selectedColumns)
        $metrics = [ordered]@{}
        foreach ($metric in @('FrameTime','GameThreadTime','RenderThreadTime','GPUTime',
                              'Exclusive/GameThread/CharacterMovement')) {
            $values = @($rows | ForEach-Object { Parse-Metric -Row $_ -Name $metric })
            if ($metric -eq 'GPUTime' -and @($values | Where-Object { $_ -gt 0 }).Count -eq 0) {
                throw "No measured GPU timings for $name; not a valid renderer capture"
            }
            $metrics[$metric] = Summarize-Values -Values $values
        }
        $drawColumns = @($rows[0].PSObject.Properties.Name | Where-Object { $_.StartsWith('DrawCall/', [StringComparison]::OrdinalIgnoreCase) })
        if ($drawColumns.Count -lt 1) { throw 'Missing actual draw-call counters' }
        $drawValues = @(foreach ($row in $rows) {
            $draws = 0.0
            foreach ($column in $drawColumns) { $draws += Parse-Metric -Row $row -Name $column }
            $draws
        })
        $metrics['DrawCallSum'] = Summarize-Values -Values $drawValues
        $phases = [ordered]@{}
        if ($Encounter) {
            if ([Math]::Abs($rows.Count - $workload.measuredPIEFrames) -gt 8) { throw 'CSV/PIE frame counts differ beyond start/stop latency allowance (8)' }
            $byPhase = @{2=@();3=@();4=@()}
            $concurrentFrames = 0
            foreach ($row in $rows) {
                $counters = Read-EncounterCounters $row $count
                if ($counters.Phase -gt 0) { $byPhase[$counters.Phase] += $row }
                if ($counters.TrainingActive -eq 1 -and $counters.CombatDuringTraining -eq 1) { $concurrentFrames++ }
            }
            if ($concurrentFrames -lt 1) { throw 'CSV does not prove actual damage while natural training is active' }
            $workload.concurrentDamageTrainingFrames = $concurrentFrames
            foreach ($phase in @(2,3,4)) {
                if ($byPhase[$phase].Count -lt 1) { throw "Missing measured encounter phase $phase" }
                $phaseMetrics = [ordered]@{}
                foreach ($metric in @('FrameTime','GameThreadTime','RenderThreadTime','GPUTime')) {
                    $phaseMetrics[$metric] = Summarize-Values -MinimumSamples 1 -Values @($byPhase[$phase] | ForEach-Object { Parse-Metric $_ $metric })
                }
                $phases["$phase"] = $phaseMetrics
            }
        }
        $copy = Join-Path $outputRoot "$name.csv"
        if (Test-Path -LiteralPath $copy) { throw 'Refusing to overwrite copied capture' }
        Copy-Item -LiteralPath $sourceCsv -Destination $copy
        $sourceHash = (Get-FileHash -LiteralPath $sourceCsv -Algorithm SHA256).Hash
        if ((Get-FileHash -LiteralPath $copy -Algorithm SHA256).Hash -ne $sourceHash) {
            throw 'Capture copy readback failed'
        }
        $captures += [ordered]@{
            name = $name; addedUnits = $(if ($Encounter) { $count - 25 } else { $count })
            totalUnits = $(if ($Encounter) { $count } else { $count + 25 })
            csv = $copy; csvSHA256 = $sourceHash; trace = $trace
            traceSHA256 = (Get-FileHash -LiteralPath $trace -Algorithm SHA256).Hash
            log = $logPath; metrics = $metrics; workload = $workload; phases = $phases
        }
    }
}
[ordered]@{
    schema = 1
    scope = $(if ($Encounter) { 'Provisional Editor D3D12 natural combat/training/FPS/physical occupation; not packaged-player gate' } else { 'Provisional Editor D3D12 moving/held units, not complete combat or packaged-player gate' })
    viewport = '1920x1080'; sampledFramesRequested = $(if ($Encounter) { 'Until physical victory; variable count' } else { 240 })
    percentile = 'Lower indexed quantile over all finite numeric CSV frames; count recorded per metric'
    warning = 'Frame cap and whole-Editor memory/cold startup affect interpretation; target not yet agreed'
    phaseLegend = $(if ($Encounter) { '2=RTS combat/cleanup/training wait;3=realtime FPS;4=physical objective navigation;0=CSV start/stop latency. Phase quantiles may have <240 samples; fixture queries contribute overhead. Actor centers are not mesh pixel visibility.' } else { $null })
    captures = $captures
} | ConvertTo-Json -Depth 12 | Set-Content -LiteralPath $summaryFile -Encoding UTF8
$readback = Get-Content -LiteralPath $summaryFile -Raw -Encoding UTF8 | ConvertFrom-Json
if ($readback.schema -ne 1 -or $readback.captures.Count -ne 2 * $Runs) { throw 'Summary readback failed' }
Write-Host "Capture summary saved: $summaryFile; $($captures.Count) actual rendered captures."
