#!/usr/bin/env python3
# tools/compare_regions.py — 关键区域逐区并排（ref 左 / ours 右）
# 用法: python tools/compare_regions.py [ref.png ours.png]
#       缺省读 out/ui-snap/{ref_scaled,ours_scaled}.png（仓库相对路径，禁止硬编码本机绝对路径）
import sys
from pathlib import Path
from PIL import Image

SNAP = Path(__file__).resolve().parents[1] / "out" / "ui-snap"
REF = sys.argv[1] if len(sys.argv) > 1 else str(SNAP / "ref_scaled.png")
OURS = sys.argv[2] if len(sys.argv) > 2 else str(SNAP / "ours_scaled.png")
ref = Image.open(REF).convert("RGB")
ours = Image.open(OURS).convert("RGB")

# (name, ref_box, ours_box)
regions = [
    ("titlebar",        (0, 0, 1280, 50),       (0, 0, 1280, 50)),
    ("leftrail_top",    (0, 48, 238, 330),      (0, 48, 238, 330)),
    ("leftrail_tabs",   (0, 320, 238, 372),     (0, 320, 238, 372)),
    ("leftrail_tran",   (0, 366, 238, 540),     (0, 366, 238, 540)),
    ("leftrail_foot",   (0, 536, 238, 618),     (0, 536, 238, 618)),
    ("video_top",       (238, 48, 936, 210),    (238, 48, 936, 210)),
    ("settings",        (936, 48, 1280, 618),   (936, 48, 1280, 618)),
]
sep = 12
for name, rb, ob in regions:
    rc = ref.crop(rb)
    oc = ours.crop(ob)
    w = max(rc.width, oc.width); h = max(rc.height, oc.height)
    canvas = Image.new("RGB", (w * 2 + sep, h), (40, 40, 48))
    canvas.paste(rc, (0, 0))
    canvas.paste(oc, (w + sep, 0))
    out = str(SNAP / f"cmp_{name}.png")
    canvas.save(out)
    print(out, canvas.size)

# 控制条：ref y507-615 vs ours y692-800
rc = ref.crop((0, 500, 1280, 618)); oc = ours.crop((0, 688, 1280, 806))
canvas = Image.new("RGB", (2560 + sep, 118), (40, 40, 48))
canvas.paste(rc, (0, 0)); canvas.paste(oc, (1280 + sep, 0))
out = str(SNAP / "cmp_deck.png")
canvas.save(out)
print(out, canvas.size)
