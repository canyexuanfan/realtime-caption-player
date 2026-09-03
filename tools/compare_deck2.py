#!/usr/bin/env python3
# tools/compare_deck2.py — 控制条/字幕/波形精确对齐比对（1280 断点布局：rail218 / stage218-958 / settings958-1280）
from PIL import Image

REF = r"F:\workbuddy\视频播放器\out\ui-snap\ref_scaled.png"
OURS = r"F:\workbuddy\视频播放器\out\ui-snap\ours_scaled.png"
ref = Image.open(REF).convert("RGB")
ours = Image.open(OURS).convert("RGB")
sep = 12

def pair(name, rb, ob, zoom=2):
    rc = ref.crop(rb); oc = ours.crop(ob)
    rc = rc.resize((rc.width*zoom, rc.height*zoom), Image.NEAREST)
    oc = oc.resize((oc.width*zoom, oc.height*zoom), Image.NEAREST)
    canvas = Image.new("RGB", (rc.width + oc.width + sep*zoom, max(rc.height, oc.height)), (40,40,48))
    canvas.paste(rc, (0,0)); canvas.paste(oc, (rc.width+sep*zoom,0))
    out = rf"F:\workbuddy\视频播放器\out\ui-snap\cmp2_{name}.png"
    canvas.save(out); print(out, canvas.size)

# 控制条（108px）：ref y507-615 vs ours y692-800，x=218-958
pair("deck", (218,500,958,618), (218,688,958,806), 2)
# 字幕浮层区：ref y330-460 vs ours y330-460（视频区上，标题下）
pair("caption", (218,320,958,470), (218,320,958,470), 2)
# 波形区：ref 底部字幕上方 y440-500 vs ours 对应相对位置——先整段视频中部对比
pair("vmid", (218,210,958,520), (218,210,958,520), 1)
