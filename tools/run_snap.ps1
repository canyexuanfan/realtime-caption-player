# tools/run_snap.ps1  (ASCII only, BOM)
# Run player_app in RCP_DEMO_ONLY + RCP_SNAPSHOT mode to grab 1280x800 UI screenshot.
param(
    [string]$OutPng = "out/ui-snap/snap.png",
    [int]$T1 = 3000,
    [int]$T2 = 4500
)
$ErrorActionPreference = "Stop"
$root = Split-Path -Parent $PSScriptRoot
$binDir  = Join-Path $root "out\build\ui\bin"
$qtBin   = Join-Path $root ".qt6\6.8.1\msvc2022_64\bin"
$mpvBin  = Join-Path $root ".tools\mpv\bin"
$env:PATH = "$binDir;$qtBin;$mpvBin;$env:PATH"
$env:QT_QPA_PLATFORM_PLUGIN_PATH = Join-Path $root ".qt6\6.8.1\msvc2022_64\plugins\platforms"
$env:QT_PLUGIN_PATH = Join-Path $root ".qt6\6.8.1\msvc2022_64\plugins"

$outAbs = Join-Path $root $OutPng
$outDir = Split-Path -Parent $outAbs
if (!(Test-Path $outDir)) { New-Item -ItemType Directory -Force -Path $outDir | Out-Null }
Remove-Item $outAbs -ErrorAction SilentlyContinue

$env:RCP_DEMO_ONLY = "1"
$env:RCP_SNAPSHOT  = $outAbs
$env:RCP_SNAP_T1   = [string]$T1
$env:RCP_SNAP_T2   = [string]$T2
$env:RCP_NO_CAPTION = "1"

Write-Host "RUN $binDir\player_app.exe -> $outAbs (t1=$T1 t2=$T2)"
& (Join-Path $binDir "player_app.exe")
Write-Host "RUN_EXIT=$LASTEXITCODE"
if (Test-Path $outAbs) { Write-Host "SNAP_OK $outAbs" } else { Write-Host "SNAP_MISSING" }
