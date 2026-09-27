#!/usr/bin/env python3
# tools/make_compare.py — 同尺寸并排比对（像素复刻比对法）
# 用法: python tools/make_compare.py <ours_1920x1200.png> <out_png> [ref.png]
# 说明:
#   ref   = 参考 HTML 的 1280x800 无头渲染，应用窗口区为顶部 ~615px（下方为 showcase 特性卡区，复刻范围外）
#           缺省取 out/ui-snap/ref.png（仓库相对路径，禁止硬编码本机绝对路径）
#   ours  = 应用快照 1920x1200（150% DPI），缩放到 1280x800 后应用窗口即整幅
#   输出左右并排（ref 左 / ours 右），应用窗口顶部区域对齐；另存一张全高并排便于纵览。
import sys
from pathlib import Path
from PIL import Image

SNAP = Path(__file__).resolve().parents[1] / "out" / "ui-snap"

def main():
    if len(sys.argv) < 3:
        print("用法: python tools/make_compare.py <ours_1920x1200.png> <out_png> [ref.png]")
        return 2
    ours_src, out_png = sys.argv[1], sys.argv[2]
    ref_src = sys.argv[3] if len(sys.argv) > 3 else str(SNAP / "ref.png")
    ref = Image.open(ref_src).convert("RGB")          # 1280x800
    ours = Image.open(ours_src).convert("RGB")         # 1920x1200 -> 1280x800
    ours = ours.resize((1280, 800), Image.LANCZOS)

    ref.save(str(SNAP / "ref_scaled.png"))
    ours.save(str(SNAP / "ours_scaled.png"))

    # 应用窗口高度：参考稿 .showcase 顶行占 78fr(≈615px@800)。逐区比对顶部 615px。
    APP_H = 615
    ref_crop = ref.crop((0, 0, 1280, APP_H))
    ours_crop = ours.crop((0, 0, 1280, APP_H))

    sep = 24
    band_h = APP_H
    canvas = Image.new("RGB", (1280 * 2 + sep, band_h), (40, 40, 48))
    canvas.paste(ref_crop, (0, 0))
    canvas.paste(ours_crop, (1280 + sep, 0))
    canvas.save(out_png)

    # 控制条区：ref 位于 y560-615，ours 位于 y742-800。单独并排。
    ref_bar = ref.crop((0, 552, 1280, 618))
    ours_bar = ours.crop((0, 740, 1280, 806))
    bh = 66
    bc = Image.new("RGB", (1280 * 2 + sep, bh), (40, 40, 48))
    bc.paste(ref_bar, (0, 0))
    bc.paste(ours_bar, (1280 + sep, 0))
    bc.save(out_png.replace(".png", "_bar.png"))

    print("WROTE", out_png, out_png.replace(".png", "_bar.png"))
    return 0

if __name__ == "__main__":
    raise SystemExit(main())
