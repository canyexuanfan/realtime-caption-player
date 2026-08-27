# T0023 — 合规 ASR 基准语料与 FunASR 基线

> **重要偏离原案**：T0023 原计划运行 `whisper.cpp` 基线。按用户长期规则
> **"视频转录禁止使用 Whisper，偏好 FunASR"**，本任务**禁用 whisper**，统一以
> **FunASR** 作为对照基线。`manifest.json` 中 `whisper_baseline` 字段显式置为
> `DISABLED_BY_POLICY`。

## 交付物
- `tests/fixtures/asr/manifest.json`：合规语料清单。**每条必须填 `license`** 字段
  （验收标准要求）。目前含 1 条自录样例结构，真实语料由用户按结构补充。
- `tools/benchmark-asr.py`：FunASR 评测脚本。对每条语料转写并计算
  `CER`（中文按字）/ `WER`（英文按词）/ `RTF` / 峰值内存，输出可重复 JSON。
- 本文档：基准方法论与运行说明。

## 编译/运行验证（已完成）
- `python -m py_compile tools/benchmark-asr.py` 通过。
- CER/WER 算法单测：相同文本=0.0、错 1 字=0.25、错 1 词=0.5，逻辑正确。
- 脚本在 FunASR 未装时**优雅退出（exit 2）并提示安装命令，绝不伪造评测数据**。

## 运行验证（PARTIAL — 需真机 FunASR + 授权语料）
- 沙箱未预装 `funasr`/`torch`/`modelscope`，且缺少真实授权语料，无法产出 CER/WER 数字。
- **真机步骤**：
  1. `pip install funasr modelscope torch`
  2. 按 `manifest.json` 结构补充自录/授权语料（带 license）
  3. `python tools/benchmark-asr.py --manifest tests/fixtures/asr/manifest.json --model paraformer-zh --out docs/benchmarks/result.json`
  4. 回填 `result.json` 的 CER/WER/RTF 到本报告。

## 合规说明
- 仅收录自录或获授权素材；`license` 字段强制填写（缺失条目脚本会跳过并报错）。
- whisper 全程不参与，符合用户禁用规则。
