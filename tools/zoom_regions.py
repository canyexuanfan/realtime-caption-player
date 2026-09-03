#!/usr/bin/env python3
# tools/zoom_regions.py — 高放大核对指定区域
import sys
from PIL import Image

REF = r"F:\workbuddy\视频播放器\out\ui-snap\ref_scaled.png"
OURS = r"F:\workbuddy\视频播放器\out\ui-snap\ours_scaled.png"
ref = Image.open(REF).convert("RGB")
ours = Image.open(OURS).convert("RGB")

# 放大倍数
def side(ref_box, ours_box, name, zoom=2):
    rc = ref.crop(ref_box); oc = ours.crop(ours_box)
    w = max(rc.width, oc.width); h = max(rc.height, oc.height)
    rc = rc.resize((rc.width*zoom, rc.height*zoom), Image.NEAREST)
    oc = oc.resize((oc.width*zoom, oc.height*zoom), Image.NEAREST)
    sep = 12*zoom
    canvas = Image.new("RGB", (rc.width + oc.width + sep, max(rc.height, oc.height)), (40,40,48))
    canvas.paste(rc, (0,0)); canvas.paste(oc, (rc.width+sep,0))
    out = rf"F:\workbuddy\视频播放器\out\ui-snap\zoom_{name}.png"
    canvas.save(out); print(out, canvas.size)

# 标题栏右侧按钮（ref/ours 同 x 范围 820-1280）
side((820,0,1280,48), (820,0,1280,48), "titlebar_right", 3)
# 左轨选项卡（ref/ours 同 0-238, 320-370）
side((0,320,238,370), (0,320,238,370), "tabs", 3)
# 视频区标题（ref/ours 同 238-700, 60-115）
side((238,60,700,115), (238,60,700,115), "videotitle", 3)
# 视频区隐私徽标（右侧）
side((700,48,936,105), (700,48,936,105), "privacy", 3)
