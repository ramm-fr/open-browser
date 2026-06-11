#!/usr/bin/env python3
"""
Generate Open Browser PNG icons — OB pixel monogram, white on transparent.
"""
import png, os

W = 255  # white opaque
T = 0    # transparent

def px(s):
    """Convert string of 32 chars ('X'=white, '.'=transparent) to row list."""
    assert len(s) == 32, f"Row must be 32 chars, got {len(s)}: '{s}'"
    return [W if c == 'X' else T for c in s]

# 32×32 OB monogram
# O occupies cols 1-11, B occupies cols 14-24
# Centered vertically rows 4-28
ROWS = [
    px("................................"),  # 0
    px("................................"),  # 1
    px("................................"),  # 2
    px("................................"),  # 3
    px("..XXXXXXXX.....XXXXXXXX........."),  # 4
    px("..XX......XX...XX......XX......."),  # 5
    px("..X........X...X........X......."),  # 6
    px("..X........X...X........XX......"),  # 7
    px("..X........X...X.........X......"),  # 8
    px("..X........X...X.........X......"),  # 9
    px("..X........X...X........XX......"),  # 10
    px("..X........X...XXXXXXXXXX......."),  # 11  B middle bar
    px("..X........X...X........XX......"),  # 12
    px("..X........X...X.........X......"),  # 13
    px("..X........X...X.........X......"),  # 14
    px("..X........X...X.........X......"),  # 15
    px("..X........X...X........XX......"),  # 16
    px("..XX......XX...XX......XX......."),  # 17
    px("..XXXXXXXX.....XXXXXXXX........."),  # 18
    px("................................"),  # 19
    px("................................"),  # 20
    px("................................"),  # 21
    px("................................"),  # 22
    px("................................"),  # 23
    px("................................"),  # 24
    px("................................"),  # 25
    px("................................"),  # 26
    px("................................"),  # 27
    px("................................"),  # 28
    px("................................"),  # 29
    px("................................"),  # 30
    px("................................"),  # 31
]

assert len(ROWS) == 32, f"Need 32 rows, have {len(ROWS)}"
assert all(len(r) == 32 for r in ROWS), "All rows must be 32 wide"


def make_png_rows(grid, target):
    """Scale 32×32 grid to nearest multiple of 32 >= target."""
    scale = max(1, (target + 31) // 32)
    actual = 32 * scale
    out = []
    for row in grid:
        rgba = bytearray()
        for v in row:
            rgba += bytes([v, v, v, v]) * scale   # RGBA white or transparent
        for _ in range(scale):
            out.append(bytes(rgba))
    return out, actual


base = '/home/ram/Desktop/openbrouser/browser/resources/icons/hicolor'

for target in [16, 32, 48, 64, 128, 256]:
    rows, actual = make_png_rows(ROWS, target)
    folder = f'{base}/{actual}x{actual}/apps'
    os.makedirs(folder, exist_ok=True)
    path = f'{folder}/io.openbrowser.Browser.png'
    with open(path, 'wb') as f:
        w = png.Writer(width=actual, height=actual,
                       greyscale=False, alpha=True, bitdepth=8)
        w.write(f, rows)
    # Verify PNG signature
    with open(path, 'rb') as f:
        ok = f.read(8) == b'\x89PNG\r\n\x1a\n'
    print(f'  [{actual}x{actual}]  {"OK " if ok else "BAD"}  {path}')

# Also write to named size folders for deb install paths
for named, actual in [(16,32),(32,32),(48,64),(64,64),(128,128),(256,256)]:
    src = f'{base}/{actual}x{actual}/apps/io.openbrowser.Browser.png'
    dst_folder = f'{base}/{named}x{named}/apps'
    os.makedirs(dst_folder, exist_ok=True)
    dst = f'{dst_folder}/io.openbrowser.Browser.png'
    if src != dst:
        import shutil
        shutil.copy2(src, dst)
        print(f'  Copied {actual}x{actual} → {named}x{named}')

print('Done.')
