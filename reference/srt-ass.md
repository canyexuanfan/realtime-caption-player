# 参考：SRT / ASS 字幕处理

- **SRT 导出：** 自实现（见 `src/captions/SrtExporter.h/.cpp`），不依赖第三方库。
- **ASS 转义 / overlay：** 自实现 `src/captions/AssEscaper.h/.cpp`（技术方案 9.3）。
- **libass（参考，非直接依赖）：** https://github.com/libass/libass （Stars 约 1k+，许可证 LGPL-2.1-or-later）—— mpv 内部用于渲染字幕；我们仅参考其转义规则，不直接链接。
- **SubtitleEdit（参考，非依赖）：** https://github.com/SubtitleEdit/subtitleedit （GPL-3.0）—— SRT/ASS 处理参考，仅供格式边界参考。

## 我们怎么用（复用程度：自实现 + 参考格式规则）

### SRT 导出（技术方案 9.6，对应 T 导出任务）
输入仅当前会话最新 final revision，按 `start_ms, segment_id` 排序：
1. 丢弃空文本；
2. 限制在媒体范围；
3. 后段开始早于前段结束 → 前段结束裁剪到后段开始；
4. 每段最短 300ms（不与下一段冲突时延长）；
5. 每行 ≤20 中文字符或 42 拉丁字符，最多 2 行，智能断行；
6. 保留中文标点，不按字节切断；
7. 时间格式 `HH:MM:SS,mmm`（小时可 >99）；
8. 输出 UTF-8 **with BOM**（兼容 Windows 播放器）；
9. `QSaveFile` 原子替换；
10. 文件名冲突默认询问覆盖，自动导出用版本后缀。
- 无 final 片段时禁用导出并解释；识别中允许导出“当前已完成部分”（`_partial` 标记）。

### ASS 转义（技术方案 9.3）
- 过滤控制字符；转义 `{ } \`（ASS 特殊字符）。
- 固定 overlay ID，更新而非不断创建；partial 灰、final 白；黑色描边 + 半透明背景可配置。

### 外部字幕解析（技术方案 1.1.5）
- 支持外挂 SRT/ASS/SSA/VTT；由 libmpv 的 `sub-add` 加载（不自行解析内嵌），仅对导出/缓存路径做 SRT 生成。

## 风险 / 注意

- **乱码**：所有字幕/JSON/IPC/日志 UTF-8；SRT 输出 BOM（技术方案 36）。
- **重叠/不单调**：导出算法必须保证时间段合法、单调、不重叠（质量门槛，技术方案 2）。
- 同一转写重复导出字节级一致（确定性）。

## 合规

- SRT/ASS 为开放格式，无许可证约束；libass 仅参考不链接；SubtitleEdit 仅参考。
