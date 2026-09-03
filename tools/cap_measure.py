# compare captions precisely: locate text rows, measure glyph height/color/stroke
from PIL import Image
import sys

def find_caption_rows(im, x0, x1, y0, y1, min_bright=120):
    """Scan a region, return list of (row, height) clusters where bright text pixels exist.
    Bright = luminance high AND near-white (text is white #fff / light gray)."""
    region = im.crop((x0, y0, x1, y1))
    w, h = region.size
    rows = []
    for y in range(h):
        cnt = 0
        for x in range(0, w, 2):
            r, g, b = region.getpixel((x, y))
            if r > min_bright and g > min_bright and b > min_bright:
                cnt += 1
        rows.append(cnt)
    # cluster consecutive rows with count > threshold
    clusters = []
    in_cluster = False
    start = 0
    for y, c in enumerate(rows):
        if c > 2 and not in_cluster:
            in_cluster = True; start = y
        elif c <= 2 and in_cluster:
            in_cluster = False
            clusters.append((start, y))
    if in_cluster:
        clusters.append((start, h))
    return clusters

def measure_text_bbox(im, x0, x1, y0, y1):
    """Exact bbox of bright (near-white) pixels in region, with per-row histogram of x extent."""
    region = im.crop((x0, y0, x1, y1))
    w, h = region.size
    ys, xs = [], []
    for y in range(h):
        for x in range(w):
            r, g, b = region.getpixel((x, y))
            if r > 120 and g > 120 and b > 120:
                ys.append(y); xs.append(x)
    if not ys:
        return None
    return (min(xs), min(ys), max(xs), max(ys))

# ---- reference 06_HTML_Preview.png (1600x1000) ----
ref = Image.open('06_HTML_Preview.png').convert('RGB')
print('ref size', ref.size)
# The app window: caption over the video. Scan lower-middle of the page for caption text.
# Try to locate caption rows across the full width in the video area.
# From prior: app-window width 1280 starting at left. Video area approx x 238..~958 within app? 
# Actually the caption overlay spans min(86%,920px) centered. Let's scan a wide band.
R = ref
# locate bright text in region y 300..700 full width, cluster
cl = find_caption_rows(R, 0, R.size[0], 200, 750, min_bright=100)
print('ref clusters (y in region 200..750):', cl)
# print a downscaled view to inspect
import numpy as np
# Let's find the widest white text band: the caption final line
best = None
for (s, e) in cl:
    bb = measure_text_bbox(R, 0, R.size[0], 200 + s, 200 + e)
    if bb:
        bw = bb[2]-bb[0]; bh = bb[3]-bb[1]
        if best is None or bw > best[0]:
            best = (bw, bh, bb)
print('ref best text bbox (in absolute 1600x1000):', best)

# ---- ours capdemo2.png (1920x1200 physical, 1280x800 logical @150%) ----
ours = Image.open('out/ui-snap/capdemo2.png').convert('RGB')
print('ours size', ours.size)
# video area logical x218..958 -> physical x327..1437; captions bottom of video area
# scan y 700..1040 physical
cl2 = find_caption_rows(ours, 320, 1440, 560, 1040, min_bright=100)
print('ours clusters (y in region 560..1040):', cl2)
best2 = None
for (s, e) in cl2:
    bb = measure_text_bbox(ours, 320, 1440, 560 + s, 560 + e)
    if bb:
        bw = bb[2]-bb[0]; bh = bb[3]-bb[1]
        if best2 is None or bw > best2[0]:
            best2 = (bw, bh, bb)
print('ours best text bbox (in absolute 1920x1200):', best2)
