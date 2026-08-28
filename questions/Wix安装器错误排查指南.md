# WiX 安装器错误排查指南（2819 / 1721 / 2812）

## 问题描述

自定义 WiX UI 的 MSI 在交互式安装中点「修改文件夹 / Change Folder」对话框的
Up / New Folder / OK 按钮即弹 **Error 2812**（"The installer has encountered an
unexpected error installing this package... The error code is 2812."）。
此前同一条自定义 UI 线上还有 2819（对话框控制指针损坏）与 1721（脚本 CustomAction）。

## 已尝试的修复方法及失败原因（重要：全部实测）

- ❌ `Publish Event="SetTarget"`：**根本没有这个事件**。运行时发布未知事件 → 2812。
  这是最初版本按钮报错的直接原因。
- ❌ `Publish Event="DirectoryUp" Value="1"`：也不是合法事件名，点击 Up 仍 2812
  （带不带 DirectoryCombo 控件都崩）。
- ❌ `Publish Event="DirectoryComboUp" Value="1" + SelectionNoItems`：依旧 2812。
  （事件名靠记忆猜的三连败——每猜错一次用户就多弹一次错。）
- ✅ **最终正解（逐项对照 WiX 官方源码 BrowseDlg.wxs，交互实测 Up 通过、无 2812）**：

```xml
<!-- DirectoryCombo 必须存在，且必须订阅 IgnoreChange -->
<Control Id="DirectoryCombo" Type="DirectoryCombo" Property="WIXUI_INSTALLDIR"
         Indirect="yes" Fixed="yes" Remote="yes">
  <Subscribe Event="IgnoreChange" Attribute="IgnoreChange" />
</Control>

<!-- Up / New Folder 的官方事件名（参数是字符串 "0"） -->
<Control Id="Up" Type="PushButton">
  <Publish Event="DirectoryListUp" Value="0">1</Publish>
</Control>
<Control Id="NewFolder" Type="PushButton">
  <Publish Event="DirectoryListNew" Value="0">1</Publish>
</Control>

<Control Id="DirectoryList" Type="DirectoryList" Property="WIXUI_INSTALLDIR"
         Indirect="yes" Sunken="yes" />

<!-- OK：SetTargetPath 把编辑路径写回目录属性；子对话框必须 EndDialog Return -->
<Control Id="OK" Type="PushButton">
  <Publish Event="SetTargetPath" Value="WIXUI_INSTALLDIR" Order="1">1</Publish>
  <Publish Event="EndDialog" Value="Return" Order="2">1</Publish>
</Control>
<!-- Cancel：Reset 恢复原值再关闭 -->
<Control Id="Cancel" Type="PushButton">
  <Publish Event="Reset" Value="0" Order="1">1</Publish>
  <Publish Event="EndDialog" Value="Return" Order="2">1</Publish>
</Control>
```

## 深层问题分析

1. **MSI ControlEvent 表是一个封闭的名字集合**（DirectoryListUp/DirectoryListNew/
   SetTargetPath/Reset/EndDialog/NewDialog/SpawnDialog/DoAction/SetProperty 等），
   light **不做**事件名静态校验——写错名字编译照过，只在运行时点按钮才 2812。
   因此**必须对照官方 wixlib 源码**，不能凭记忆拼事件名。
2. 目录浏览三件套（DirectoryCombo + DirectoryList + PathEdit）共享同一
   `Property + Indirect="yes"`（属性值 = 目录属性名，如 WIXUI_INSTALLDIR →
   值 "INSTALLFOLDER"），Combo 必须订阅 IgnoreChange，否则目录枚举期间的变化
   处理会异常。
3. SpawnDialog 弹出的子对话框只能 `EndDialog Return` 关闭；用 NewDialog 会破坏
   对话框栈（这就是"修改文件夹对话框怪怪的"的手感来源之一）。
4. 本包用 `light -sval` 跳过了 ICE 验证——很多 UI 静态错误（包括部分事件问题）
   本可被 ICE 在构建期拦下。**建议正式发布时去掉 -sval**（本沙箱 rc.exe 受限才加）。

## 调试工具

- 读 MSI 数据库 ControlEvent 表（只读，不安装）：
  PowerShell `WindowsInstaller.Installer` COM → OpenDatabase(msi,0) →
  `SELECT Dialog_, Control_, Event, Argument FROM ControlEvent`（脚本见
  `out/e2e-verify/dump-events.ps1`）。
- 交互测试建议自动化驱动（本会话用 a11y 树逐按钮点击复现），注意：
  提权后的 msiexec 向导 UIPI 拦截非提权自动化——用**非提权启动向导**
  （UAC 在最后点 Install 才弹）即可驱动 Welcome/Dir/Browse 全流程。
- 静默安装（`/qn`）不执行 UI 序列，**测不出 2812**——交互类缺陷必须交互测。

## 注意事项

- 日志（/L*v）与向导窗口可能分属不同进程（服务接管 UI），日志尾部会停在启动段；
  不要依赖它追 UI 事件错误。
- "Change Folder" 对话框的朴素外观是 Windows Installer 原生控件风格
  （DirectoryCombo/DirectoryList），修复后行为与经典安装器一致；
  要现代化外观需 External UI 或完整自绘 UI，属后续可选工作。

## 更新记录

- 2026-08-28 首次记录：SetTarget/DirectoryUp/DirectoryComboUp 三次失败试错 →
  官方源码对照修复（DirectoryListUp/DirectoryListNew + Subscribe IgnoreChange +
  SetTargetPath + Reset），交互实测 Up 导航通过、无 2812；OK/整链由用户重装复验。
