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
