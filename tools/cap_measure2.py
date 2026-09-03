# precise white-core row measurement of caption in ref(1600x1000) and ours(1920x1200)
from PIL import Image

def white_rows(im, x0, x1, y0, y1, thr=235):
    """per-row count of near-pure-white pixels (caption core), return list."""
    region = im.crop((x0, y0, x1, y1))
    w, h = region.size
    rows = []
    for y in range(h):
        cnt = 0
        for x in range(0, w, 2):
            r, g, b = region.getpixel((x, y))
            if r > thr and g > thr and b > thr:
                cnt += 1
        rows.append(cnt)
    return rows

def clusters(rows, min_cnt=3):
    out = []
    ins = False; s = 0
    for y, c in enumerate(rows):
        if c > min_cnt and not ins:
            ins = True; s = y
        elif c <= min_cnt and ins:
            ins = False; out.append((s, y))
    if ins: out.append((s, len(rows)))
    return out

# ---- reference 1600x1000: caption around y 470-690, x 260-1340 ----
ref = Image.open('06_HTML_Preview.png').convert('RGB')
rows = white_rows(ref, 260, 1340, 470, 700, thr=230)
cl = clusters(rows, 4)
print('REF caption white-core clusters (y+470):', [(a, b, b-a) for a, b in cl])
for (a, b) in cl:
    # find x extent at the densest row
    besty = max(range(a, b), key=lambda y: rows[y])
    xs = [x for x in range(260, 1340, 2) if all(v > 230 for v in ref.getpixel((x, 470+besty)))]
    if xs:
        print(f'   cluster y {470+a}-{470+b} (h={b-a}px) densest row {470+besty}, x {min(xs)}-{max(xs)} (w={max(xs)-min(xs)})')

# ---- ours 1920x1200 physical (1280x800 logical @150%): video area x327-1437 ----
ours = Image.open('out/ui-snap/capdemo2.png').convert('RGB')
rows2 = white_rows(ours, 327, 1437, 560, 1040, thr=230)
cl2 = clusters(rows2, 4)
print('OURS caption white-core clusters (y+560):', [(a, b, b-a) for a, b in cl2])
for (a, b) in cl2:
    besty = max(range(a, b), key=lambda y: rows2[y])
    xs = [x for x in range(327, 1437, 2) if all(v > 230 for v in ours.getpixel((x, 560+besty)))]
    if xs:
        print(f'   cluster y {560+a}-{560+b} (h={b-a}px) densest row {560+besty}, x {min(xs)}-{max(xs)} (w={max(xs)-min(xs)})')
