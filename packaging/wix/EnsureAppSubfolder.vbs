' EnsureAppSubfolder.vbs — WiX CustomAction
' 用户在 InstallDirDlg 点 Change 选目录后，自动追加 \RealtimeCaptionPlayer 子文件夹。
' 由 product.wxs 的 Publish(InstallDirDlg/Next → DoAction EnsureAppSubfolder) 触发。
Function EnsureAppSubfolder()
  On Error Resume Next

  Dim sPath
  sPath = Session.Property("WIXUI_INSTALLDIR")
  If sPath = "" Then
    sPath = Session.Property("INSTALLFOLDER")
  End If

  ' 去掉尾部反斜杠
  If Right(sPath, 1) = "\" Then
    sPath = Left(sPath, Len(sPath) - 1)
  End If

  ' 如果路径不是以 \RealtimeCaptionPlayer 结尾，则追加
  Dim sAppName
  sAppName = "RealtimeCaptionPlayer"
  If LCase(Right(sPath, Len(sAppName) + 1)) <> LCase("\" & sAppName) Then
    sPath = sPath & "\" & sAppName
  End If

  ' 确保尾部有反斜杠（Windows Installer 目录属性约定）
  If Right(sPath, 1) <> "\" Then
    sPath = sPath & "\"
  End If

  Session.Property("INSTALLFOLDER") = sPath

  EnsureAppSubfolder = 1   ' 成功，继续安装
  If Err.Number <> 0 Then
    EnsureAppSubfileder = 3  ' 错误，跳过（不应发生）
  End If
End Function
