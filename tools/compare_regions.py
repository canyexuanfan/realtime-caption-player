#!/usr/bin/env python3
# tools/compare_regions.py — 关键区域逐区并排（ref 左 / ours 右）
import sys
from PIL import Image

REF = r"F:\workbuddy\视频播放器\out\ui-snap\ref_scaled.png"
OURS = r"F:\workbuddy\视频播放器\out\ui-snap\ours_scaled.png"
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
    out = rf"F:\workbuddy\视频播放器\out\ui-snap\cmp_{name}.png"
    canvas.save(out)
    print(out, canvas.size)

# 控制条：ref y507-615 vs ours y692-800
rc = ref.crop((0, 500, 1280, 618)); oc = ours.crop((0, 688, 1280, 806))
canvas = Image.new("RGB", (2560 + sep, 118), (40, 40, 48))
canvas.paste(rc, (0, 0)); canvas.paste(oc, (1280 + sep, 0))
out = r"F:\workbuddy\视频播放器\out\ui-snap\cmp_deck.png"
canvas.save(out)
print(out, canvas.size)
