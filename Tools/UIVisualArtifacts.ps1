# Pure UI family contract; reuse bounded PNG decoding and strict Automation validation.
. (Join-Path $PSScriptRoot 'VisualRegressionArtifacts.ps1')
. (Join-Path $PSScriptRoot 'AutomationReport.ps1')
function Get-UIVisualCases {
    $index=0
    foreach ($size in @(@(1280,720),@(1920,1080),@(2560,1440))) {
        foreach ($dpi in @(100,125)) { foreach ($text in @(100,200)) {
            $name="$($size[0])x$($size[1])_${dpi}_Text${text}"
            [PSCustomObject]@{index=$index;width=$size[0];height=$size[1];dpi=$dpi;textScale=$text;name=$name;
                test="RomanEmpireGame.UI.ResponsiveShell.$($size[0])x$($size[1])_$dpi"}
            $index++
        } }
    }
}
function Get-UIVisualScreens { @('Settings','BindingCapture','Tactical','Construction','Strategic','Possession','Outcome') }
function Assert-UIVisualCohort {
    param([Parameter(Mandatory=$true)][string]$ManifestPath,[Parameter(Mandatory=$true)][string]$RepositoryRoot,[switch]$Complete)
    $path=Assert-VisualPath -Path $ManifestPath -Root (Join-Path $RepositoryRoot 'Artifacts/UIVisualRuns')
    if ((Get-Item -LiteralPath $path).Length -gt 256KB) { throw 'UI manifest too large' }
    $manifest=Get-Content -LiteralPath $path -Raw -Encoding UTF8 | ConvertFrom-Json
    if (($manifest.schema -isnot [int] -and $manifest.schema -isnot [long]) -or $manifest.schema -ne 1 -or $manifest.family -cne 'UI_v1' -or $manifest.engine -cne '5.8.2' -or
        $manifest.platform -cne 'Win64_D3D12' -or $manifest.candidate -isnot [bool] -or
        $manifest.images -isnot [array] -or $manifest.images.Count -lt 7 -or $manifest.images.Count -gt 84 -or $manifest.images.Count%7) {
        throw 'UI cohort contract mismatch'
    }
    if ($Complete -and $manifest.images.Count -ne 84) { throw 'All84 native UI cases required, not partial approval' }
    $cases=@(Get-UIVisualCases); $screens=@(Get-UIVisualScreens)
    $seen=[Collections.Generic.HashSet[string]]::new([StringComparer]::Ordinal)
    $reports=[Collections.Generic.HashSet[string]]::new([StringComparer]::OrdinalIgnoreCase)
    foreach ($row in $manifest.images) {
        $known=@($cases | Where-Object { $_.name -ceq $row.case })
        if ($known.Count -ne 1 -or $screens -cnotcontains $row.screen -or
            ($row.width -isnot [int] -and $row.width -isnot [long]) -or ($row.height -isnot [int] -and $row.height -isnot [long]) -or
            ($row.nativeExitCode -isnot [int] -and $row.nativeExitCode -isnot [long]) -or
            $row.view -cne "$($row.case)_$($row.screen)" -or -not $seen.Add($row.view) -or
            $row.width -ne $known[0].width -or $row.height -ne $known[0].height -or $row.nativeExitCode -ne 0) {
            throw 'UI identity/dimensions/exit mismatch'
        }
        $imagePath=Assert-VisualPath -Path $row.path -Root (Join-Path $RepositoryRoot 'Artifacts/UIVisualRegression')
        if ([IO.Path]::GetFileName($imagePath) -cne "$($row.view).png") { throw 'Native UI image name mismatch' }
        $identity=Get-VisualImageIdentity -Path $imagePath -ExpectedWidth $known[0].width -ExpectedHeight $known[0].height
        if ($identity.sha256 -cne $row.sha256 -or $identity.sha1 -cne $row.sha1) { throw 'UI raster changed after receipt' }
        $report=Assert-VisualPath -Path $row.report -Root (Split-Path -Parent $path)
        if ((Get-FileHash -LiteralPath (Join-Path $report 'index.json')).Hash.ToLowerInvariant() -cne $row.reportSha256) { throw 'UI report changed after receipt' }
        if ($reports.Add($report)) {
            Assert-AutomationReportPassed -ReportDirectory $report -RequiredTests @($known[0].test)
            $data=Get-Content -LiteralPath (Join-Path $report 'index.json') -Raw -Encoding UTF8 | ConvertFrom-Json
            if ($data.tests.Count -ne 1) { throw 'One exact UI layout per independent process required' }
            $incoming=@($data.tests[0].entries | Where-Object { $_.event.message -like 'UIVisualIncoming=*' })
            if ($incoming.Count -ne 7) { throw 'Seven native UI raster receipts required' }
            $disclaimers=@($data.tests[0].entries | Where-Object { $_.event.message -like 'UI CANDIDATE ONLY:*' })
            $metrics=@($data.tests[0].entries | Where-Object { $_.event.message -like 'UIVisualComparison *' })
            if ($manifest.candidate -and ($disclaimers.Count -ne 7 -or $metrics.Count)) { throw 'Candidate comparison disclaimer missing or contradictory' }
            if (-not $manifest.candidate -and ($disclaimers.Count -or $metrics.Count -ne 7)) { throw 'Actual native comparison metrics required, not relabelled candidates' }
        }
        # Binding each image to its native report prevents detached/relabelled PNG evidence.
        $data=Get-Content -LiteralPath (Join-Path $report 'index.json') -Raw -Encoding UTF8 | ConvertFrom-Json
        $matching=@($data.tests[0].entries | Where-Object {
            $_.event.message -like 'UIVisualIncoming=*' -and
            [IO.Path]::GetFullPath($_.event.message.Substring('UIVisualIncoming='.Length)).Equals($imagePath,[StringComparison]::OrdinalIgnoreCase)
        })
        if ($matching.Count -ne 1) { throw 'Actual UI PNG/report binding missing' }
        if (-not $manifest.candidate) {
            $metrics=@($data.tests[0].entries | Where-Object { $_.event.message.StartsWith("UIVisualComparison $($row.view) ",[StringComparison]::Ordinal) })
            if ($metrics.Count -ne 1) { throw 'Exact native UI comparison/report binding missing' }
            $comparisonPath=Assert-VisualPath -Path (Join-Path ([IO.Path]::GetDirectoryName($imagePath)) "$($row.view)-comparison.json") -Root (Join-Path $RepositoryRoot 'Artifacts/UIVisualRegression')
            if ((Get-Item -LiteralPath $comparisonPath).Length -gt 64KB) { throw 'UI comparison receipt too large' }
            $comparison=Get-Content -LiteralPath $comparisonPath -Raw -Encoding UTF8 | ConvertFrom-Json
            if ($comparison.schema -ne 1 -or $comparison.family -cne 'UI_v1' -or $comparison.view -cne $row.view -or
                $comparison.candidate -isnot [bool] -or $comparison.similar -isnot [bool] -or
                $comparison.candidate -ne $false -or $comparison.similar -ne $true -or
                $comparison.width -ne $row.width -or $comparison.height -ne $row.height -or
                $comparison.localLimit -ne .01 -or $comparison.globalLimit -ne .002 -or
                $comparison.localError -isnot [ValueType] -or $comparison.globalError -isnot [ValueType] -or
                $comparison.localError -is [bool] -or $comparison.globalError -is [bool] -or
                [double]::IsNaN($comparison.localError) -or [double]::IsNaN($comparison.globalError) -or
                $comparison.localError -lt 0 -or $comparison.localError -gt .01 -or
                $comparison.globalError -lt 0 -or $comparison.globalError -gt .002) { throw 'Native UI comparison rejected' }
            if ($metrics[0].event.message -notmatch '^UIVisualComparison \S+ local=([0-9]+\.[0-9]+) global=([0-9]+\.[0-9]+) error=$') { throw 'Native comparison metrics malformed' }
            $culture=[Globalization.CultureInfo]::InvariantCulture
            if ([Math]::Abs([double]::Parse($Matches[1],$culture)-$comparison.localError) -gt .000000001 -or
                [Math]::Abs([double]::Parse($Matches[2],$culture)-$comparison.globalError) -gt .000000001) { throw 'Native comparison metrics/receipt mismatch' }
        }
    }
    foreach ($group in @($manifest.images | Group-Object -Property case)) {
        if ($group.Count -ne 7) { throw 'Incomplete screen set for UI layout' }
    }
    return $manifest
}
function Assert-UIVisualBaselines {
    param([Parameter(Mandatory=$true)][string]$RepositoryRoot)
    $folder=Assert-VisualPath -Path (Join-Path $RepositoryRoot 'Tests/VisualBaselines/UI_v1/Win64_D3D12') -Root $RepositoryRoot
    $path=Join-Path $folder 'manifest.json'
    if ((Get-Item -LiteralPath $path).Length -gt 256KB) { throw 'UI baseline manifest too large' }
    $manifest=Get-Content -LiteralPath $path -Raw -Encoding UTF8 | ConvertFrom-Json
    if (($manifest.schema -isnot [int] -and $manifest.schema -isnot [long]) -or $manifest.schema -ne 1 -or $manifest.family -cne 'UI_v1' -or $manifest.engine -cne '5.8.2' -or
        $manifest.platform -cne 'Win64_D3D12' -or $manifest.images -isnot [array] -or $manifest.images.Count -ne 84) { throw 'Complete reviewed UI family required' }
    foreach ($case in @(Get-UIVisualCases)) { foreach ($screen in @(Get-UIVisualScreens)) {
        $view="$($case.name)_$screen"; $rows=@($manifest.images | Where-Object { $_.view -ceq $view })
        if ($rows.Count -ne 1) { throw 'Missing/duplicate reviewed UI case' }
        $png=Assert-VisualPath -Path (Join-Path $folder "$view.png") -Root $RepositoryRoot
        $identity=Get-VisualImageIdentity -Path $png -ExpectedWidth $case.width -ExpectedHeight $case.height
        $sidecar=Assert-VisualPath -Path ($png+'.sha1') -Root $RepositoryRoot
        if ((Get-Item -LiteralPath $sidecar).Length -gt 128 -or (Get-Content -LiteralPath $sidecar -Raw).Trim() -cne $identity.sha1 -or
            $rows[0].sha256 -cne $identity.sha256 -or $rows[0].sha1 -cne $identity.sha1) { throw "UI baseline integrity mismatch: $view" }
    } }
    return $manifest
}
