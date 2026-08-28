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

## 追加（第二轮用户反馈）：点 OK 卡死（无 2812，但对话框冻结）

### ❌ 第四次失败：SetTargetPath 参数不带方括号
`SetTargetPath Value="WIXUI_INSTALLDIR"`（不带括号）+ 控件 `Indirect="yes"` 的组合：
MSDN 明确规则——"**Argument = 含路径的属性名；若属性是间接的，属性名必须用方括号**"。
不带括号时 MSI 把 WIXUI_INSTALLDIR **本身**当作含路径的属性，而它的值是字面字符串
"INSTALLFOLDER"（不是路径）→ 路径校验失败 → 触发文档中的另一条规则：
"**若路径不可写/无效，安装器封锁该控件后续的所有 ControlEvent**"——连同按钮上的
EndDialog Return 一起失效 → 表现为点 OK 后对话框死住（用户实测复现）。

### ✅ 正解
间接（Indirect=yes）控件场景下，SetTargetPath 参数必须写：
`Value="[WIXUI_INSTALLDIR]"`（方括号解析后得到真实含路径的属性名 INSTALLFOLDER）。
官方 BrowseDlg.wxs 里 `SetTargetPath Value="[_BrowseProperty]"` 带括号正是此规则。

### 验证状态
- ControlEvent 表复检：`BrowseDlg | OK | SetTargetPath | [WIXUI_INSTALLDIR]` ✓
- Up 导航（DirectoryListUp）上一轮已交互实测通过。
- OK 全链由用户重装复验（本机不再代操作）。

### 附注（行为说明，非缺陷）
若用户选择了不可写的路径，MSDN 规定 MSI 会封锁 OK 控件后续事件（防错误安装），
此时 Cancel 按钮仍可用（封锁按控件粒度）。此为 Windows Installer 原生行为，
标准安装器同样如此。

## 追加（第三轮用户反馈）：Error 2732 "Directory Manager not supplied"

### ❌ 根因：自定义 InstallUISequence 只排了对话框、没排标准动作
原序列只有四条 Show（Welcome/InstallDir/VerifyReady/Exit 相对次序），
**CostInitialize / FileCost / CostFinalize / ExecuteAction 全部缺席**。
目录管理器在 CostInitialize 才初始化；BrowseDlg OK 的 SetTargetPath 在没有
目录管理器的情况下执行 → Error 2732。
（连带隐患：VerifyReadyDlg 的 Install = EndDialog Return 后序列无 ExecuteAction，
点 Install 可能根本不会开始安装。）

### ✅ 修复：对话框与标准动作交错排布（官方 WiX 同构）
```xml
<InstallUISequence>
  <Show Dialog="ExitDialog" OnExit="success" />
  <Show Dialog="WelcomeDlg" Before="CostInitialize" />
  <Show Dialog="InstallDirDlg" After="CostFinalize" />
  <Show Dialog="VerifyReadyDlg" Before="ExecuteAction" />
</InstallUISequence>
```
NewDialog 导航会顺次执行对话框之间的序列动作：Welcome→Next 触发
CostInitialize/FileCost/CostFinalize → 目录页出现（目录管理器就绪）；Install →
EndDialog Return 后执行 ExecuteAction 真正安装 → 成功后 OnExit=success 弹 ExitDialog。

### ✅ 同时落实"选择目录后自动追加程序名子文件夹"
BrowseDlg OK 三连（ControlEvent 表实测）：
```
OK | SetTargetPath | [WIXUI_INSTALLDIR]      | Order 1  # 所选路径写入 INSTALLFOLDER
OK | [INSTALLFOLDER] | [INSTALLFOLDER]RealtimeCaptionPlayer\ | Order 2  # 追加子文件夹
OK | EndDialog | Return                      | Order 3  # 回到目录页
```
WiX3 语法注意：设属性的 Publish 用 `Property=` + `Value=` 属性（无 Event 属性，
元素文本为条件）——写成 `Argument=` 会报 CNDL0004。

### 验证
InstallUISequence 实测表：SetINSTALLFOLDER(798) → WelcomeDlg(799) → CostInitialize(800)
→ FileCost(900) → CostFinalize(1000) → InstallDirDlg(1001) → VerifyReadyDlg(1299)
→ ExecuteAction(1300)；ControlEvent 表如上。静默安装链路（/qn 跳过 UI 序列）不受影响。
完整交互链由用户重装复验。

## 追加（第四轮用户反馈）：2732 依旧——上轮修复思路本身是错的（❌→✅ 更正）

### ❌ 上轮修复思路错误（必须诚实更正）
上轮声称"NewDialog 导航会顺次执行对话框之间的序列动作（CostInitialize 等）"——
**这个理解是错的**。MSI 真实机制：
- `NewDialog` 只切换当前显示的对话框，**序列仍暂停在第一个 Show 动作处**，中间排的
  标准动作（CostInitialize/FileCost/CostFinalize）**不会执行**；
- 只有 `EndDialog Return` 才恢复序列、继续执行后续动作。

因此上一版把 Welcome 排在 CostInitialize 之前（Before=CostInitialize）、想靠
NewDialog 链"穿过"标准动作的做法必然失败：用户进到目录页/Browse 时序列还停在
Welcome 的 Show，目录管理器从未初始化 → Browse OK 依旧 2732。

### ✅ 正确结构：先 Costing，再 Welcome（一次 Show + 纯 NewDialog 链）
```xml
<InstallUISequence>
  <Show Dialog="ExitDialog" OnExit="success" />
  <Show Dialog="WelcomeDlg" After="CostFinalize">1</Show>
</InstallUISequence>
```
- 序列执行：SetINSTALLFOLDER(799) → CostInitialize(800) → FileCost(900) →
  CostFinalize(1000) → WelcomeDlg 显示(1001)；
- 之后整条链（Welcome→目录页→Browse→VerifyReady）全部 NewDialog 切换，
  目录管理器始终就绪 → Browse OK / SetTargetPath / 追加子文件夹均正常；
- VerifyReady 的 Install = EndDialog Return → 序列恢复 → MigrateFeatureStates(1200)
  → ExecuteAction(1300) 真正开始安装 → 成功 → OnExit=success 弹 ExitDialog。
- 目录页/VerifyReady **不要**再单独排 Show（它们由 NewDialog 到达；单独排会在
  Return 恢复序列后被再次求值、二次弹出）。

### 复检（两表实测，2026-08-28）
InstallUISequence：CostInitialize(800)/FileCost(900)/CostFinalize(1000)/
WelcomeDlg(1001)/MigrateFeatureStates(1200)/ExecuteAction(1300)。
ControlEvent：BrowseDlg OK = SetTargetPath([WIXUI_INSTALLDIR]) →
追加 \RealtimeCaptionPlayer\ → EndDialog Return。

## 追加（第五轮用户反馈）：点 Install 后窗口消失、静默安装无反馈

### 根因：没有进度页
VerifyReady 点 Install = EndDialog Return → 序列恢复后直接进 ExecuteAction。
包里没有 ProgressDlg ⇒ 确认页关闭后、整个安装期间**没有任何窗口**
（677MB 包静默解压数十秒），看起来像安装器闪退；装完才弹完成页（或因间隔太长被误以为没有）。

### ✅ 修复：补标准三件
1. **ProgressDlg（Modeless）**：`<Show Dialog="ProgressDlg" Before="ExecuteAction" />`。
   非模态是关键——Show 动作不阻塞序列，继续进 ExecuteAction，引擎把
   SetProgress/ActionText/TimeRemaining 实时推送到对话框订阅控件
   （ProgressBar/ActionText/TimeRemaining 三订阅）。
   **样式位知识（MSDN Dialog Style Bits 权威值，此前凭记忆的"Modeless=bit8"是错的）**：
   Visible=1 / Modal=2 / Minimize=4 / SysModal=8 / KeepModeless=16。
   **没有 Modeless 位——不带 Modal(2) 位即为非模态**。实测本包
   ProgressDlg Attributes=5（Visible+Minimize，无 Modal）＝正确非模态；
   向导页=7（含 Modal）＝模态。
2. **UserExit（OnExit=cancel）**：进度页取消 → 回滚 → 显示"安装被中断"。
3. **FatalError（OnExit=error）**：失败 → 显示出错收尾页。

### 复检
InstallUISequence：FatalError(-3)/UserExit(-2)/ExitDialog(-1)/WelcomeDlg(1001)/
ProgressDlg(1299)/ExecuteAction(1300)；Dialog 表 ProgressDlg Attributes=5（非模态）；
ControlEvent：ProgressDlg|Cancel|SpawnDialog|CancelDlg 接通。

## 附：Qt 静态库资源丢失（图标/logo 全空白）——构建系统级坑

**现象**：qt_add_resources 挂在 STATIC 库目标上时，qrc 初始化对象（qrc_*_init.cpp.obj）
没有外部符号引用，MSVC 链接器将其丢弃 → 运行时 QFile(":/...") 全部失败：
SVG 图标不渲染、按钮空圆、logo 不显示、窗口图标缺失。布局 a11y 检查完全发现不了。

**修复**：qt_add_resources 必须挂在**最终可执行目标**上（player_app），
且调用必须在 add_executable 之后（否则 CMake 报目标不存在）。
验证：out/build/<cfg>/src/player/CMakeFiles/player_app.dir/.qt/rcc/qrc_*.obj 存在。

## 附 2（2026-08-29）：图标仍空白第二轮——qt_add_resources 两个隐藏坑 + 运行时自检

第一轮修复（qrc 移到 exe 目标）后用户仍报"没有变化"。深挖出两层叠加问题：
1. **qt_add_resources 的 FILES 不支持 alias**：资源落在 :/logo128.png，
   而代码读 :/logo.png → logo 必然失败（与静态库问题无关的独立 bug）。
2. 初始化时序仍不完全可靠。
**最终方案（运行时验证通过）**：改用经典 **AUTORCC + 手写 rcp.qrc**（alias 正确、
机制久经考验），qrc 作为 player_app 源文件编译（AUTORCC ON 属性）。
**启动资源自检**：main.cpp 启动时把 :/logo.png、:/icons/*.svg 可达性 +
QSvgRenderer 渲染有效性写入 exe 旁 resource-check.txt（不可写则落 AppData）。
实测：4 资源全 OK、svg-render(play)=OK（227 字节解析成功）。
排查教训：加自检时 QFile(...).readAll() 在未 open 的临时对象上返回空字节，
会让"渲染级自检"假阴性——必须先 open。
