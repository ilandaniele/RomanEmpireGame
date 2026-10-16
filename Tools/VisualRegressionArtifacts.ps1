# Pure visual-artifact checks. Dot-sourcing does not write files or launch Unreal.
function Assert-VisualPath {
    param([Parameter(Mandatory=$true)][string]$Path, [Parameter(Mandatory=$true)][string]$Root)
    $full = [IO.Path]::GetFullPath($Path)
    $base = [IO.Path]::GetFullPath($Root).TrimEnd('\','/')
    if (-not $full.StartsWith($base + [IO.Path]::DirectorySeparatorChar, [StringComparison]::OrdinalIgnoreCase)) {
        throw "Visual path outside expected root: $full"
    }
    $current = $full
    while ($current.Length -ge $base.Length) {
        if ((Test-Path -LiteralPath $current) -and
            ((Get-Item -LiteralPath $current -Force).Attributes -band [IO.FileAttributes]::ReparsePoint)) {
            throw "Redirected visual path: $current"
        }
        $current = [IO.Path]::GetDirectoryName($current)
        if (-not $current) { break }
    }
    return $full
}
function Get-VisualImageIdentity {
    param([Parameter(Mandatory=$true)][string]$Path, [int]$ExpectedWidth=1280, [int]$ExpectedHeight=720)
    $item = Get-Item -LiteralPath $Path -ErrorAction Stop
    if ($item.PSIsContainer -or $item.Length -lt 33 -or $item.Length -gt 16MB) { throw 'Invalid/bounded PNG required' }
    $bytes = [IO.File]::ReadAllBytes($item.FullName)
    $signature = @(137,80,78,71,13,10,26,10)
    for ($index=0; $index -lt 8; $index++) {
        if ($bytes[$index] -ne $signature[$index]) { throw 'Not a PNG raster' }
    }
    if ([Text.Encoding]::ASCII.GetString($bytes,12,4) -cne 'IHDR') { throw 'PNG IHDR required' }
    $width = [long]$bytes[16]*16777216 + [long]$bytes[17]*65536 + [long]$bytes[18]*256 + $bytes[19]
    $height = [long]$bytes[20]*16777216 + [long]$bytes[21]*65536 + [long]$bytes[22]*256 + $bytes[23]
    if ($width -ne $ExpectedWidth -or $height -ne $ExpectedHeight) { throw "Native ${ExpectedWidth}x${ExpectedHeight} required, not resized: ${width}x${height}" }
    Add-Type -AssemblyName System.Drawing
    $stream = [IO.MemoryStream]::new($bytes, $false)
    $decoded = $null
    try {
        $decoded = [Drawing.Bitmap]::new($stream)
        if ($decoded.Width -ne $ExpectedWidth -or $decoded.Height -ne $ExpectedHeight -or
            $decoded.RawFormat.Guid -ne [Drawing.Imaging.ImageFormat]::Png.Guid) { throw 'Decoded PNG contract mismatch' }
        $null = $decoded.GetPixel($ExpectedWidth-1,$ExpectedHeight-1)
    } catch { throw "PNG decoding failed: $_" }
    finally { if ($null -ne $decoded) { $decoded.Dispose() }; $stream.Dispose() }
    [PSCustomObject]@{path=$item.FullName;width=$width;height=$height;
        sha256=(Get-FileHash -LiteralPath $item.FullName -Algorithm SHA256).Hash.ToLowerInvariant();
        sha1=(Get-FileHash -LiteralPath $item.FullName -Algorithm SHA1).Hash.ToLowerInvariant()}
}
function Assert-VisualBaselineSet {
    param([Parameter(Mandatory=$true)][string]$RepositoryRoot)
    $folder = Assert-VisualPath -Path (Join-Path $RepositoryRoot 'Tests/VisualBaselines/Win64_D3D12') -Root $RepositoryRoot
    $manifestPath = Assert-VisualPath -Path (Join-Path $folder 'manifest.json') -Root $RepositoryRoot
    if ((Get-Item -LiteralPath $manifestPath).Length -gt 64KB) { throw 'Baseline manifest too large' }
    $manifest = Get-Content -LiteralPath $manifestPath -Raw -Encoding UTF8 | ConvertFrom-Json
    if ($manifest.schema -ne 1 -or $manifest.engine -cne '5.8.2' -or $manifest.platform -cne 'Win64_D3D12' -or
        $manifest.width -ne 1280 -or $manifest.height -ne 720 -or $manifest.images.Count -ne 3) { throw 'Baseline contract mismatch' }
    foreach ($view in @('CharacterProfiles','CharacterRTS','CharacterPossession')) {
        $rows = @($manifest.images | Where-Object { $_.view -ceq $view })
        if ($rows.Count -ne 1) { throw "Missing/duplicate reviewed view: $view" }
        $path = Assert-VisualPath -Path (Join-Path $folder "$view.png") -Root $RepositoryRoot
        $identity = Get-VisualImageIdentity -Path $path
        $sidecar = Assert-VisualPath -Path ($path + '.sha1') -Root $RepositoryRoot
        if ((Get-Item -LiteralPath $sidecar).Length -gt 128 -or
            (Get-Content -LiteralPath $sidecar -Raw).Trim() -cne $identity.sha1 -or
            $rows[0].sha256 -cne $identity.sha256 -or $rows[0].sha1 -cne $identity.sha1) {
            throw "Baseline integrity mismatch: $view"
        }
    }
    return $manifest
}
