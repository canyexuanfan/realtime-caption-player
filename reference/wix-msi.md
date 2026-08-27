# 参考：WiX Toolset（MSI 打包，仅工具）

- **仓库/站点：** https://github.com/wixtoolset/wix （WiX Toolset v3/v4/v5）
- **许可证：** MS-RL（Microsoft Reciprocal License）—— 仅作为**打包工具**使用，不随运行时分发，不影响产品 GPL 合规。
- **角色：** 生成 `RealtimeCaptionPlayer-Offline-x64.msi` 与 `RealtimeCaptionPlayer-Lite-x64.msi`（技术方案 11.4）。

## 我们怎么用（复用程度：自编写 .wxs）

- 不捆绑 WiX 源码；仅在本机构建期使用 `candle`/`light`（或 WiX v4+ 的 `wix build`）。
- 打包顺序（技术方案 11.4）：
  1. Release 构建；2. 单测/集成；3. `windeployqt` 收集 Qt DLL/plugins；4. 复制自建 libmpv/FFmpeg/sherpa-onnx 运行库；
  5. 复制并校验模型；6. 生成第三方许可证 + SBOM；7. WiX 生成 MSI；8. 干净机 smoke test；9. 签名；10. SHA-256 + release manifest。
- **per-user 安装**（无需管理员）：ProgID `RealtimeCaptionPlayer.Video.1`；`HKCU\Software\Classes\...`；`HKCU\Software\RegisteredApplications`；`Capabilities\FileAssociations`（技术方案 11.1）。
- 升级：覆盖保留设置/历史/DB/用户模型；卸载默认保留用户数据，可选“同时删除用户数据”（技术方案 11.5）。

## 风险 / 注意

- **本环境 WiX 未安装** → MSI 打包（P8）BLOCKED，待安装。
- **不得修改 `UserChoice`**：默认播放器按钮仅注册能力并打开系统默认应用页（技术方案 11.2，ADR 边界）。
- 发布前必须对干净机做安装/升级/卸载矩阵（技术方案 15.3 门禁）。

## 合规

- WiX 为构建工具，MS-RL 不影响产品 GPL-3.0 分发。
- MSI 内包含完整 GPL 源码获取方式、NOTICE、依赖许可证、SBOM（技术方案 15.2/15.3）。
