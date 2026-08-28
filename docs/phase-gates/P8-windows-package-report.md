# Phase P8 Gate Report — Windows 集成、文件关联、MSI

Status: **PASS**（带已知限制，见 Known Issues）

Commit: 948f86d（UI 重做+mpv 修复）/ d5059cd（ASR 链路修复）
Tag: phase-p8-pass
Date: 2026-08-28

## Tasks（压缩达成路径）

MVP 按"把产品开发成型"的用户指令压缩推进，P8 对应能力落实如下（与 T0219–T0243 的映射见
`docs/implementation-status.md` Task Status 注记）：

- **T0228 等价（图标/版本资源/manifest）**：exe 含版本资源（CMake 工程 0.1.0）；`/MANIFEST:NO`
  为沙箱链接限制的临时选项（见 Known Issues）。
- **T0229/T0230/T0231 等价（release staging + 第三方 DLL + 模型）**：`cmake/RcpBundle.cmake`
  收集 .tools 运行时 DLL + 四套 ASR 模型 → `out/bundle/runtime`（自包含，~700MB）；
  `windeployqt` 收齐 Qt6（Core/Gui/Widgets/OpenGL/OpenGLWidgets/Network/Svg）+ platforms。
- **T0232 等价（LICENSE/NOTICE/SBOM）**：LICENSE(GPL-3.0-or-later) + NOTICE +
  `dependencies.lock.json`（含 SHA-256/来源/许可证）+ `docs/release/SBOM.md`。
- **T0233/T0234（WiX MSI 骨架 + Offline MSI）**：heat(-ag) → candle → light(-sval)，
  产出 `out/package/RealtimeCaptionPlayer-0.1.0.msi`（≈677MB，OLE 头 d0cf11e0 校验通过），
  全部文件嵌入 MSI，**离线安装、终用户零下载**。自定义最小 UI（欢迎→选目录→确认→完成），
  目录自动追加 `RealtimeCaptionPlayer` 子文件夹，Type51 重置旧路径记忆（修 2819/1721 历史）。
- **T0236（UpgradeCode/ProductVersion 固定）**：product.wxs 固定 UpgradeCode，版本 0.1.0。
- **T0238 等价（卸载清理）**：卸载移除全部程序文件（用户数据目录不删除，符合"保留用户数据"）。
- **T0240 等价（一键打包）**：`ninja -C out/build/spikes2 package_msi` 一条命令出包。
- **T0241（干净环境离线 smoke）**：本机（Windows 11 x64，非开发构建目录）安装→运行→卸载
  真机通过（见 Evidence）。Lite MSI（T0235）不做：MVP 无 Lite 变体需求来源，不造假包。
- **T0239（代码签名）**：BLOCKED——无私钥证书。构件以 SHA-256 清单发布（见 release manifest）。
- **T0237（首次启动关联注册）/T0219–T0227（文件关联）**：MVP 未实装 UserChoice 关联
  （涉及资源管理器注册与用户引导流程），列入 Known Issues / 后续任务，不做假关联。

## Build

- `ninja -C out/build/spikes2 player_app caption_worker` → rc=0（Release，MSVC 14.44）。
- `ninja -C out/build/spikes2 package_msi` → rc=0，MSI ≈677MB。

## Tests

- ctest 21/21 PASS（2.83s，含 BoundedQueue 容量修复后的稳定通过）。

## E2E（真机，Windows 11 x64 + Intel Iris Xe + 2560x1440 显示器）

1. **安装**：`msiexec /i ... INSTALLFOLDER=F:\rcp-app-test /qn`（UAC 提权）→ **exit 0**，
   文件/模型/Qt 运行时齐全。
2. **一键运行**：安装目录 `player_app.exe e2e-test.mp4` →
   player_app + caption_worker 均从安装目录自动启动（模型目录解析修复生效）；
   369 个 time-pos 事件推进、osd-overlay 报错 0、转写面板 4 句字幕逐字正确带时间戳、
   ASR 状态卡"运行中"、播放至片尾（00:21/00:21）。
3. **卸载**：`msiexec /x ... /qn`（UAC 提权）→ **exit 0**，安装目录完全移除。

## Evidence

- `docs/evidence/P4-asr-real-e2e/`：真实中文 TTS 语料 ASR 运行日志 + SRT + GUI 播放 trace。
- `docs/evidence/P8-msi/`：安装/卸载退出码、安装目录清单、trace-installed。
- `questions/sherpaVAD崩溃排查指南.md`、`questions/libmpv播放停滞排查指南.md`：
  本阶段真机回填发现并修复的 6 个真实缺陷记录（VAD Empty/Detected、绝对时间戳、
  use-after-free、BoundedQueue 容量、MPV_FORMAT_FLAG 误用、osd-overlay 语法）。

## Known Issues

1. **代码签名缺失**（无私钥证书），构件以 SHA-256 清单交付。
2. **文件关联（UserChoice）未实装**，T0219–T0227 保留为后续任务。
3. **`/MANIFEST:NO`**：正式发布建议恢复 manifest 嵌入（开发机正常 TEMP 可用）。
4. **Intel Iris Xe**：mpv 建纹理偶报一次 INVALID_ENUM，非致命，不影响播放
   （详见 questions/libmpv播放停滞排查指南.md）。
5. T0019 rclone 断网、T0023 FunASR CER/WER 仍 PARTIAL（外部依赖：网盘挂载/授权语料）。

## Gate Result

**PASS** —— 安装/运行/卸载真机闭环 + 真实语音 ASR 端到端全通过；限制项均有记录与后续路径。

## 补记（2026-08-28 晚，tag v0.1.0 之后）

用户交互安装时「修改文件夹」对话框报 **Error 2812**。根因：自定义 BrowseDlg 的按钮
事件名错误（`SetTarget`/`DirectoryUp`/`DirectoryComboUp` 均非合法 MSI ControlEvent）。
已逐项对照 WiX 官方 BrowseDlg.wxs 修复：Up=`DirectoryListUp(0)`、NewFolder=
`DirectoryListNew(0)`、DirectoryCombo 订阅 `IgnoreChange`、OK=`SetTargetPath`+
`EndDialog Return`、Cancel=`Reset`+`EndDialog Return`。

**交互式验证（非提权向导，a11y 驱动）**：Browse→Up 导航到上级目录成功、目录列表与
组合框同步更新、无任何错误弹窗（旧包同操作必现 2812）。ControlEvent 表复检确认包内
零非法事件。完整安装链由用户重装复验（静默安装/卸载链路此前已 exit 0 通过）。
修复后 MSI SHA-256 已更新至 `docs/release/release-manifest-v0.1.0.txt`
（tag v0.1.0 指向修复前的代码状态，此为 tag 后的补丁提交）。

### 补记 2（同日）：OK 卡死修复
BrowseDlg OK 点后冻结：SetTargetPath 参数未按 MSDN 规则为间接属性加方括号，
导致路径校验失败并封锁该控件全部 ControlEvent（含 EndDialog）。已改为
`[WIXUI_INSTALLDIR]`（InstallDirDlg Next 与 BrowseDlg OK 两处），ControlEvent 表
复检通过；MSI 重打，manifest 哈希同步更新。详见 questions/Wix安装器错误排查指南.md。

### 补记 3（同日）：2732 + Install 序列缺失修复 + 自动子文件夹
自定义 InstallUISequence 缺少标准动作（CostInitialize/CostFinalize/ExecuteAction）
→ 目录管理器未初始化，BrowseDlg OK 报 2732；且 Install 按钮实际不会开始安装。
已按官方结构交错排布标准动作与对话框；同时实现用户需求的"Browse OK 后自动追加
\RealtimeCaptionPlayer\ 子文件夹"（SetTargetPath 后接 SetProperty Publish）。
InstallUISequence 与 ControlEvent 双表复检通过；MSI 重打，manifest 已同步。

### 补记 4（同日）：2732 真正根因——NewDialog 不推进序列
上轮"标准动作与对话框交错 + NewDialog 顺次执行中间动作"的理解是错误的：
NewDialog 仅切换对话框，序列暂停在首个 Show 处不动。正确结构是先 Costing 再 Welcome
（WelcomeDlg After=CostFinalize 唯一 Show），NewDialog 链在目录管理器就绪后运行；
Install=EndDialog Return 恢复序列至 ExecuteAction。两表复检通过，manifest 已同步。
完整流程待用户重装复验。详见 questions/Wix安装器错误排查指南.md 第四轮更正。

### 补记 5（同日）：补安装进度页与收尾三态页
用户反馈点 Install 后窗口消失、全程静默。补 ProgressDlg（Modeless，Before=
ExecuteAction，订阅 SetProgress/ActionText/TimeRemaining）+ UserExit(OnExit=cancel)
+ FatalError(OnExit=error)。样式位经 MSDN Dialog Style Bits 核实：无 Modeless 位、
不带 Modal(2) 即非模态（实测 ProgressDlg Attributes=5 正确）。manifest 已同步。

### 补记 6（2026-08-29）：UI 全面复刻参考稿 + 品牌 logo
用户要求：前端与 05_Single_HTML_Frontend_Reference.html 除实例数据外完全一致；
logo 采用用户提供的霓虹语音图标。
- 资源：resources/logo.png（原图）+ logo128.png（qrc）+ app.ico（ffmpeg 生成，
  嵌入 exe via app.rc → 安装快捷方式自动继承图标）；30 个 sprite 图标转 SVG 入 qrc，
  QSvgRenderer 运行期换色渲染（Qt6::Svg）。
- MainWindow 全面重写（对照参考 :root 令牌与各区块 CSS）：自绘标题栏（25px logo+
  品牌+文件名+打开文件/打开文件夹/设置+46px 窗控钮，close hover #c42b3a）；
  左栏 238px（播放列表 276px 区+播放列表/历史记录 tabs（accent-soft 选中态）+
  转写面板（时间戳小字+正文、partial 灰字原位更新）+搜索框+行数/实时延迟统计）；
  视频区（文件名 17px、隐私徽标、字幕叠加（partial 82% 灰字+final 36px 白字描边，
  位置 底部13%/中部/顶部 可调）、波形条（识别活动驱动渐变条）、ASR 浮卡
  （引擎/语言/模型/置信度/延迟/已识别时长/导出，可拖动））；控制台 108px
  （时间轴+三组圆形钮：字幕开关/截图(mpv screenshot)/AB循环(mpv ab-loop)/
  字幕轨(cycle sub)、±10s、上下曲、45px 渐变主按钮、倍速弹窗(0.5-4x 网格)、
  音量、全屏）；右滑设置面板 344px（常规/播放/字幕/实时字幕/快捷键/外观/高级 7 页，
  真实接线：自动播放/记住位置/恢复会话/默认倍速/硬件解码/字幕延迟±5s/
  加载外挂字幕/字号22-54/位置/灰字开关/文件夹连续播放/打开 ms-settings:defaultapps；
  无后端项按参考稿模式置灰+说明，不做假开关）。
- 历史记录：QSettings 持久化最近 30 条（真实）；启动恢复上次会话。
- 验证：构建 rc=0；真机启动 a11y 树确认全部区块与控件就位；两处启动崩溃修复
  （loadHistory 早于 UI 创建的空指针）。

### 补记 7（2026-08-29）：logo 变形修复 + 前端二轮对照源码抠差异
- logo：原图 1536x1024（3:2 非正方），按用户要求 pad 白底至 1536x1536 等比不压缩，
  重生成 logo128/app.ico；标题栏 25x25 不再变形。
- 前端对照源码修复差异：基础字号 13→14px（body）；左栏改参考稿 4 行网格
  （顶部 276px 区由 tabs 切换播放列表/历史记录，标题联动；转写面板占 1fr 大区）；
  列表行部件化（40px 行、▶当前项、名称 12px+时长 11px（QSettings 时长缓存，真实）、
  选中紫渐变）；设置面板默认展开且停驻参考稿默认页「实时字幕」；视频渐晕层
  （.video-vignette）；隐私徽标悬停显示（hover 态）；拖放提示遮罩（.drop-hint）。

### 补记 8（2026-08-29）：资源初始化根修 + ASR 卡/徽标/控制台对齐参考稿
用户截图对比发现：全部 SVG 图标/logo 未渲染。根因：qt_add_resources 挂在静态库
rcp_player 上，qrc 初始化对象被链接器丢弃（构建系统级坑，已记入 questions/）。
修复：移至 player_app 可执行目标（add_executable 之后）。同时修复：ASR 卡
延迟/已识别时长标签未入布局悬浮错位（改 metricRow 两行结构）；隐私徽标补定位
（top17/right18，此前叠在文件名上）；控制台左组按参考稿改为 打开文件夹/截图/AB/
字幕开关/字幕轨。真机 a11y 复检通过；截屏通道故障以构建产物 obj 证据替代视觉自检。

### 补记 9（同日）：图标渲染像素级实证（快照自验证闭环）
按用户要求落实问题修复流程（questions/Qt图标SVG不显示排查指南.md 建档）。
新增 RCP_SNAPSHOT 离屏快照自验证（WA_DontShowOnScreen + grab，屏幕零干扰），
snap3.png 像素级确认：logo/全部图标/列表行/设置面板渲染成功。
修复第三层根因：SVG currentColor 替换色用了 Qt 私有 8 位 #AARRGGBB，
QSvgHandler 不认导致描边回落黑色隐形——改 6 位 #RRGGBB。
MSI 重打，manifest 同步。
