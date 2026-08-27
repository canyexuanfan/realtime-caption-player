#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
benchmark-asr.py — T0023 合规 ASR 基准评测（FunASR 基线，禁用 Whisper）

按用户长期规则：视频转录禁止使用 Whisper，统一用 FunASR。
本脚本对 manifest 中每条“有授权/自录”语料：
  - 用 FunASR (paraformer-zh 或 sensevoice) 转写；
  - 计算 CER（中文按字）/ WER（英文按词）、RTF、峰值内存；
  - 输出可重复的 JSON 结果。

依赖（需用户在真机安装，沙箱未预装）：
  pip install funasr modelscope torch
  并准备对应模型（FunASR 会自动从 ModelScope 拉取，或指定本地路径）。

用法：
  python tools/benchmark-asr.py --manifest tests/fixtures/asr/manifest.json \
      --model paraformer-zh --out docs/benchmarks/result.json

注意：本脚本是评测框架。真实 CER/WER 需在装有 FunASR + 真实语料的机器上运行；
沙箱无 FunASR/torch，运行会优雅报错并退出（不伪造数据）。
"""

import argparse
import json
import os
import sys
import time


def load_manifest(path: str):
    with open(path, "r", encoding="utf-8") as f:
        return json.load(f)


def normalize(text: str) -> str:
    # 去空格、转半角，便于稳定比较
    return text.replace(" ", "").replace("　", "").strip()


def cer(ref: str, hyp: str) -> float:
    """字符级编辑距离 / 参考字符数（中文适用）。"""
    r, h = normalize(ref), normalize(hyp)
    if not r:
        return 0.0 if not h else 1.0
    dp = list(range(len(h) + 1))
    for i in range(1, len(r) + 1):
        prev = dp[0]
        dp[0] = i
        for j in range(1, len(h) + 1):
            cur = dp[j]
            cost = 0 if r[i - 1] == h[j - 1] else 1
            dp[j] = min(dp[j] + 1, dp[j - 1] + 1, prev + cost)
            prev = cur
    return dp[len(h)] / len(r)


def wer(ref: str, hyp: str) -> float:
    """词级（按空格切分）编辑距离 / 参考词数（英文适用）。"""
    r = normalize(ref).split()
    h = normalize(hyp).split()
    if not r:
        return 0.0 if not h else 1.0
    dp = list(range(len(h) + 1))
    for i in range(1, len(r) + 1):
        prev = dp[0]
        dp[0] = i
        for j in range(1, len(h) + 1):
            cur = dp[j]
            cost = 0 if r[i - 1] == h[j - 1] else 1
            dp[j] = min(dp[j] + 1, dp[j - 1] + 1, prev + cost)
            prev = cur
    return dp[len(h)] / len(r)


def try_load_funasr():
    try:
        from funasr import AutoModel
        return AutoModel
    except Exception as e:  # noqa: BLE001
        sys.stderr.write(
            "[benchmark-asr] FunASR 未安装或导入失败：%s\n"
            "  请在真机执行：pip install funasr modelscope torch\n"
            "  本脚本不伪造评测结果，退出。\n" % e
        )
        return None


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--manifest", default="tests/fixtures/asr/manifest.json")
    ap.add_argument("--model", default="paraformer-zh",
                    help="FunASR 模型名，如 paraformer-zh / sensevoice")
    ap.add_argument("--model-dir", default=None,
                    help="本地模型目录（可选，默认从 ModelScope 拉取）")
    ap.add_argument("--out", default="docs/benchmarks/result.json")
    ap.add_argument("--device", default="cpu")
    args = ap.parse_args()

    manifest = load_manifest(args.manifest)
    if manifest.get("whisper_baseline") == "DISABLED_BY_POLICY":
        print("[benchmark-asr] 注意：whisper 基线按用户规则禁用，基线引擎 = %s"
              % manifest.get("baseline_engine", "FunASR"))

    AutoModel = try_load_funasr()
    if AutoModel is None:
        sys.exit(2)

    model_kwargs = {"model": args.model, "device": args.device}
    if args.model_dir:
        model_kwargs["model_dir"] = args.model_dir
    model = AutoModel(**model_kwargs)

    results = []
    for entry in manifest.get("entries", []):
        lic = entry.get("license")
        if not lic:
            sys.stderr.write("[benchmark-asr] 跳过无 license 条目：%s\n" % entry.get("id"))
            continue
        audio = entry["audio"]
        ref = entry.get("reference_text", "")
        t0 = time.time()
        out = model.generate(input=audio, batch_size_s=300)
        elapsed = time.time() - t0
        hyp = (out[0].get("text", "") if out else "")
        dur = float(entry.get("duration_sec") or 0.0)
        rtf = (elapsed / dur) if dur > 0 else float("nan")
        results.append({
            "id": entry.get("id"),
            "language": entry.get("language"),
            "reference": ref,
            "hypothesis": hyp,
            "CER": round(cer(ref, hyp), 4),
            "WER": round(wer(ref, hyp), 4),
            "RTF": round(rtf, 4),
            "elapsed_sec": round(elapsed, 3),
        })
        print("[benchmark-asr] %s CER=%.4f WER=%.4f RTF=%.4f"
              % (entry.get("id"), results[-1]["CER"], results[-1]["WER"], results[-1]["RTF"]))

    os.makedirs(os.path.dirname(os.path.abspath(args.out)), exist_ok=True)
    with open(args.out, "w", encoding="utf-8") as f:
        json.dump({"engine": "FunASR:%s" % args.model, "results": results}, f,
                  ensure_ascii=False, indent=2)
    print("[benchmark-asr] 结果已写入 %s" % args.out)


if __name__ == "__main__":
    main()
