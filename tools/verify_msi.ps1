# tools/verify_msi.ps1 - minimal MSI verification via WindowsInstaller COM (E).
# Usage: powershell -File tools/verify_msi.ps1 <path-to-msi>
param([string]$MsiPath = "out\package\RealtimeCaptionPlayer-0.1.0.msi")
$ErrorActionPreference = 'Stop'
$installer = New-Object -ComObject WindowsInstaller.Installer
$db = $installer.GetType().InvokeMember('OpenDatabase','InvokeMethod',$null,$installer,@((Resolve-Path $MsiPath).Path, 0))

function QueryAll($db, $sql) {
    $view = $db.GetType().InvokeMember('OpenView','InvokeMethod',$null,$db,@($sql))
    $view.GetType().InvokeMember('Execute','InvokeMethod',$null,$view,@($null)) | Out-Null
    $rows = @()
    while ($true) {
        $rec = $view.GetType().InvokeMember('Fetch','InvokeMethod',$null,$view,$null)
        if ($null -eq $rec) { break }
        $n = $rec.GetType().InvokeMember('FieldCount','GetProperty',$null,$rec,$null)
        $row = @()
        for ($i = 1; $i -le $n; $i++) {
            $row += $rec.GetType().InvokeMember('StringData','GetProperty',$null,$rec,@($i))
        }
        $rows += ,($row -join ' | ')
    }
    return $rows
}

$pv = QueryAll $db "SELECT Value FROM Property WHERE Property = 'ProductVersion'"
Write-Output ("ProductVersion = {0}" -f ($pv -join ','))
$reg = QueryAll $db "SELECT Registry FROM Registry WHERE Registry LIKE '%Rcp.VideoFiles%'"
Write-Output ("Rcp.VideoFiles rows = {0}" -f $reg.Count)
if ($reg.Count -ge 8) { Write-Output 'MSI-VERIFY-PASS' } else { Write-Output 'MSI-VERIFY-FAIL'; exit 1 }
