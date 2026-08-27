<#
.SYNOPSIS
  Validate dependencies.lock.json schema and hash completeness.

  Returns 0 when the lock is internally consistent; returns non-zero and names
  the offending dependency when a required field is missing/empty.

  Contract (T0008):
   - Every entry needs name / version / license / status.
   - Entries whose status is "locked" MUST carry source_archive_sha256 and
     binary_artifact_sha256 (enforced once native libs are self-built in T0010-T0013).
   - Any hash field that exists but is empty is invalid.
#>
$ErrorActionPreference = 'Stop'
$root = Resolve-Path "$PSScriptRoot/.."
$lockPath = Join-Path $root "dependencies.lock.json"
if (-not (Test-Path $lockPath)) { Write-Error "MISSING: $lockPath"; exit 1 }

$lock = Get-Content $lockPath -Raw | ConvertFrom-Json
$errors = @()

if (-not $lock.schema_version) { $errors += "lock: schema_version 缺失" }

foreach ($dep in $lock.dependencies) {
    $name = $dep.name
    if (-not $name) { $errors += "dependency 缺少 name"; continue }
    foreach ($field in @('version', 'license', 'status')) {
        if (-not $dep.$field) { $errors += "$name`: 必填字段 $field 缺失/空" }
    }
    # hash enforcement only for fully-pinned (locked) dependencies
    if ($dep.status -eq 'locked') {
        foreach ($h in @('source_archive_sha256', 'binary_artifact_sha256')) {
            if (-not $dep.$h) { $errors += "$name`: 状态为 locked 但缺少 $h" }
        }
    }
    # any present-but-empty hash is invalid
    foreach ($h in @('source_archive_sha256', 'binary_artifact_sha256')) {
        if (($dep.PSObject.Properties.Name -contains $h) -and -not $dep.$h) {
            $errors += "$name`: $h 存在但为空"
        }
    }
}

if ($errors.Count -gt 0) {
    Write-Host "依赖锁校验失败：" -ForegroundColor Red
    foreach ($e in $errors) { Write-Host "  - $e" }
    exit 1
}
$locked = @($lock.dependencies | Where-Object { $_.status -eq 'locked' }).Count
Write-Host "依赖锁校验通过（$(@($lock.dependencies).Count) 个依赖，locked=$locked）。" -ForegroundColor Green
exit 0
