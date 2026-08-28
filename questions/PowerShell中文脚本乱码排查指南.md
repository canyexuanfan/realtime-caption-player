# Windows PowerShell 中文脚本乱码排查指南

## 问题描述

用 Write 工具生成的含中文的 `.ps1` 脚本，用 `powershell -File` 执行时报
解析错误，中文字符串显示为 `浠婂ぉ鎴戜滑...`（UTF-8 字节被按 GBK 解释的典型乱码）。

## 原因

Windows PowerShell 5.1 对**无 BOM** 的 UTF-8 脚本按系统 ANSI 代码页（中文系统 = GBK/936）
读取。Write 工具输出的是无 BOM UTF-8，中文字符串全部乱码甚至破坏语法（引号被吞）。

## ✅ 成功解决方法

给脚本加 UTF-8 BOM 后再执行：

```bash
printf '\xEF\xBB\xBF' > script_bom.ps1 && cat script.ps1 >> script_bom.ps1 && mv -f script_bom.ps1 script.ps1
```

或在 PowerShell 内重存：`Get-Content -Encoding UTF8 | Set-Content -Encoding UTF8BOM`（PS7）。

## 注意事项

- PowerShell 7（pwsh）默认按 UTF-8 读无 BOM 脚本，无此问题；`powershell`（5.1）有。
- 本项目编码铁律：全部源文件 UTF-8；`.ps1`/`.bat` 若含中文必须带 BOM（bat 是 ANSI/CJK 兼容问题，优先用 ps1+BOM）。
- `cl.exe` 编译含中文注释的源文件需 `/utf-8`，否则 CP936 下报 C4819 甚至语法错误。

## 更新记录

- 2026-08-28 首次记录（TTS 语料生成脚本踩坑）。
