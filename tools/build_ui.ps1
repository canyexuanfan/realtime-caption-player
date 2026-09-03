# tools/build_ui.ps1  (ASCII only, BOM)
# Build player_app with Ninja + MSVC14.44 + Qt6.8.1 for pixel-replication screenshots.
param(
    [string]$Action = "build",
    [string]$BuildDir = "out/build/ui"
)
$ErrorActionPreference = "Stop"
$root = Split-Path -Parent $PSScriptRoot
$vsBase   = "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools"
$msvc     = Join-Path $vsBase "VC\Tools\MSVC\14.44.35207"
$sdk      = "C:\Program Files (x86)\Windows Kits\10"
$sdkVer   = "10.0.26100.0"
$cmakeExe = Join-Path $vsBase "Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"
$ninjaExe = Join-Path $vsBase "Common7\IDE\CommonExtensions\Microsoft\CMake\Ninja\ninja.exe"
$buildDir = Join-Path $root $BuildDir

$env:INCLUDE = "$msvc\include;$sdk\Include\$sdkVer\ucrt;$sdk\Include\$sdkVer\um;$sdk\Include\$sdkVer\shared"
$env:LIB     = "$msvc\lib\x64;$sdk\Lib\$sdkVer\ucrt\x64;$sdk\Lib\$sdkVer\um\x64"
$env:PATH    = "$msvc\bin\Hostx64\x64;$sdk\bin\$sdkVer\x64;$env:PATH"

if ($Action -eq "configure" -or !(Test-Path "$buildDir\CMakeCache.txt")) {
    Write-Host "== configure =="
    & $cmakeExe -S $root -B $buildDir -G Ninja `
        -DCMAKE_MAKE_PROGRAM="$ninjaExe" `
        -DCMAKE_BUILD_TYPE=Release `
        -DBUILD_TESTS=OFF `
        -DBUILD_PLAYER=ON `
        -DBUILD_WORKER=OFF `
        -DBUILD_SPIKES=OFF `
        -DBUILD_DEPS_SMOKE=OFF
    if ($LASTEXITCODE -ne 0) { throw "cmake configure failed" }
}

if ($Action -eq "configure") { exit 0 }

Write-Host "== build player_app =="
& $cmakeExe --build $buildDir --target player_app --parallel 8
if ($LASTEXITCODE -ne 0) { throw "build player_app failed" }

$bin = Join-Path $buildDir "bin\player_app.exe"
Write-Host "BUILD_OK $bin"
