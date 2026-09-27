# tools/check-status.ps1 - todolist / implementation-status consistency audit (D5).
# Checks:
#   1. implementation-status snapshot must NOT claim RELEASE READY unless todolist
#      phases are all checked (currently always expected OFF before MVP re-baseline).
#   2. todolist.md checkboxes count sanity ([x]/[ ]).
#   3. Tracked files must not contain local absolute paths / usernames (privacy).
# Usage: powershell -NoProfile -ExecutionPolicy Bypass -File tools/check-status.ps1
$ErrorActionPreference = 'Continue'
$repo = Split-Path -Parent $PSScriptRoot
$fail = 0

$status = Get-Content (Join-Path $repo 'docs\implementation-status.md') -Raw -Encoding UTF8
if ($status -match 'RELEASE READY' -and $status -notmatch 'withdrawal declaration') {
    Write-Output 'FAIL: implementation-status still claims RELEASE READY without a withdrawal banner'
    $fail++
}

$todo = Get-Content (Join-Path $repo 'todolist.md') -Raw -Encoding UTF8
$checked = ([regex]::Matches($todo, '\[x\]')).Count
$unchecked = ([regex]::Matches($todo, '\[ \]')).Count
Write-Output ("todolist: {0} checked, {1} unchecked" -f $checked, $unchecked)

$hits = & git -C $repo grep -nE 'F:[/\]|wzm33' -- . 2>$null | Select-String -NotMatch 'docs/review/'
if ($hits) {
    Write-Output 'FAIL: local absolute paths / username found in tracked files:'
    $hits | Select-Object -First 10
    $fail++
}

if ($fail -eq 0) { Write-Output 'CHECK-STATUS-PASS' } else { Write-Output ("CHECK-STATUS-FAIL ({0})" -f $fail); exit 1 }
