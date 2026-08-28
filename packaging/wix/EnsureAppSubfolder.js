// EnsureAppSubfolder.js — WiX CustomAction (JScript)
// 用户选目录后自动追加 \RealtimeCaptionPlayer 子文件夹。
// 用 JScript 而非 VBScript：VBScript 在 perMachine(UAC提权) 安装中易触发 Error 1721。
function EnsureAppSubfolder() {
    var sPath = Session.Property("WIXUI_INSTALLDIR");
    if (!sPath) sPath = Session.Property("INSTALLFOLDER");
    if (!sPath) { return 1; }

    // 去掉尾部反斜杠
    if (sPath.charAt(sPath.length - 1) === "\\") {
        sPath = sPath.substring(0, sPath.length - 1);
    }

    var appName = "RealtimeCaptionPlayer";
    var suffix = "\\" + appName;
    if (sPath.toLowerCase().indexOf(suffix.toLowerCase()) !== sPath.length - suffix.length) {
        sPath = sPath + suffix;
    }

    // 确保尾部反斜杠
    if (sPath.charAt(sPath.length - 1) !== "\\") {
        sPath = sPath + "\\";
    }

    // 双写回：WIXUI_INSTALLDIR 控制文本框，INSTALLFOLDER 控制实际安装路径
    Session.Property("WIXUI_INSTALLDIR") = sPath;
    Session.Property("INSTALLFOLDER") = sPath;

    return 1;
}
