实时字幕播放器：完整资料包

编码与兼容性说明
================
1. 压缩包内部所有目录名、文件名均只使用英文 ASCII 字符，避免旧版 Windows、旧版 WinRAR 或跨系统解压时出现中文文件名乱码。
2. 所有 Markdown、HTML、TXT 文本均统一为 UTF-8 with BOM，可直接使用 Windows 记事本、VS Code、Trae、Cursor、Notepad++ 等工具打开。
3. HTML 已内嵌 CSS、JavaScript、SVG 图标和演示图片，不依赖外部资源；双击 05_Single_HTML_Frontend_Reference.html 即可预览。
4. PNG 文件保持原始二进制内容，不做二次压缩或转码。
5. SHA256SUMS.txt 用于校验文件是否完整。

文件清单与中文对应关系
======================
01_PRD.md
  原文件：PRD(1).md
  内容：实时字幕播放器产品需求文档。

02_Technical_Implementation_Plan.md
  原文件：实时字幕播放器_技术实现方案.md
  内容：详细技术选型、系统架构、模块设计、IPC、ASR、字幕、安装、测试和发布方案。

03_Detailed_Development_TODO.md
  原文件：实时字幕播放器_详细开发TODO.md
  内容：可逐项执行、验收和回填的详细开发任务清单。

04_UI_Design_Mockup.png
  原文件：AI 生成的播放器前端 UI 设计图。
  尺寸：1536 × 1024。

05_Single_HTML_Frontend_Reference.html
  原文件：实时字幕播放器_单HTML前端参考.html
  内容：带交互的单文件前端参考原型。

06_HTML_Preview.png
  原文件：实时字幕播放器_HTML预览.png
  内容：单 HTML 原型的静态预览图。
  尺寸：1600 × 1000。

推荐使用方式
============
- 先阅读 01_PRD.md，确认需求边界。
- 再阅读 02_Technical_Implementation_Plan.md，理解整体架构和实现约束。
- 开发 Agent 按 03_Detailed_Development_TODO.md 的顺序执行、测试、回填并提交 Git。
- 以 04_UI_Design_Mockup.png 和 05_Single_HTML_Frontend_Reference.html 作为前端视觉及交互参考。
- 通过 06_HTML_Preview.png 快速核对整体页面效果。

注意
====
本资料包没有重复包含旧的“方案与 TODO”二次压缩包，因为其中内容已分别以 02、03 两个文件完整收录；这样可以减少冗余，也避免嵌套压缩包带来的编码问题。
