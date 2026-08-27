<#
.SYNOPSIS
  Idempotent bootstrap for Realtime Caption Player build environment.
  Verifies/installs cmake, ninja and Qt 6, and confirms the MSVC toolchain.
  Fails (non-zero) with a clear list of missing items; never corrupts a working env.
.NOTES
  Qt is installed with aqtinstall. IMPORTANT: qtbase is the *default* package and
  must NOT be passed via `-m qtbase` (that triggers "packages not found").
  The correct invocation uses the full arch token `win64_msvc2022_64` and omits `-m`.
#>
param(
    [string]$QtVersion   = "6.8.1",
    [string]$QtArch      = "win64_msvc2022_64",
    [string]$QtOutputDir = "$PSScriptRoot/../.tools/Qt",
    [int]   $TimeoutSec  = 300,
    [int]   $MaxRetry    = 12
)

$ErrorActionPreference = 'Stop'
$root = Resolve-Path "$PSScriptRoot/.."

function Require($name, $test) {
    if (-not $test) { Write-Error "MISSING: $name" ; return $false }
    Write-Host "OK   $name"
    return $true
}

Write-Host "== RealtimeCaptionPlayer bootstrap =="

# 1. cmake / ninja (prefer a managed venv; fall back to pip install)
$cmake = Get-Command cmake -ErrorAction SilentlyContinue
$ninja = Get-Command ninja -ErrorAction SilentlyContinue
if (-not ($cmake -and $ninja)) {
    Write-Host "Attempting pip install cmake ninja ..."
    python -m pip install --quiet cmake ninja
}

# 2. Qt 6 (qtbase-only; qtbase is the default package)
$qtDir = Join-Path $QtOutputDir "$QtVersion/msvc2022_64"
if (-not (Test-Path "$qtDir/bin/Qt6Core.dll")) {
    Write-Host "Installing Qt $QtVersion ($QtArch, qtbase) to $QtOutputDir ..."
    $attempt = 0
    $ok = $false
    while ($attempt -lt $MaxRetry -and -not $ok) {
        $attempt++
        try {
            python -m aqt install-qt windows desktop $QtVersion $QtArch `
                --outputdir $QtOutputDir --timeout $TimeoutSec `
                --accept-licenses --accept-obligations
            $ok = $true
        }
        catch {
            Write-Host "aqt attempt $attempt failed: $_"
            Start-Sleep -Seconds 5
        }
    }
    if (-not $ok) { Write-Error "Qt install failed after $MaxRetry attempts" }
}
Require "Qt6Core.dll" (Test-Path "$qtDir/bin/Qt6Core.dll") | Out-Null
Require "Qt6 cmake dir" (Test-Path "$qtDir/lib/cmake/Qt6") | Out-Null

# 3. compiler
Require "MSVC cl.exe" (Get-Command cl -ErrorAction SilentlyContinue) | Out-Null

Write-Host "Bootstrap complete. Next:"
Write-Host "  $qtDir  -> set as CMAKE_PREFIX_PATH"
Write-Host "  cmake --preset windows-msvc-debug"
Write-Host "  cmake --build --preset windows-msvc-debug --parallel"
Write-Host "  ctest --preset windows-msvc-debug --output-on-failure"
