"""檢查原始碼裡的非 ASCII 字元，字型裡是不是都有。

raylib 遇到字型裡沒有的字會畫成空白（不會報錯），所以新增說明文字後跑一下：
    python tools/check_glyphs.py [字型路徑]
需要 fontTools（pip install fonttools）。
"""
import glob
import os
import sys

from fontTools.ttLib import TTCollection, TTFont

root = os.path.join(os.path.dirname(__file__), "..")
font_path = sys.argv[1] if len(sys.argv) > 1 else "C:/Windows/Fonts/msjh.ttc"
font = TTCollection(font_path).fonts[0] if font_path.lower().endswith(".ttc") else TTFont(font_path)
cmap = font.getBestCmap()

missing = {}
for path in glob.glob(os.path.join(root, "src", "**", "*.[ch]pp"), recursive=True):
    with open(path, encoding="utf-8") as f:
        for lineno, line in enumerate(f, 1):
            for c in line:
                if ord(c) > 127 and ord(c) not in cmap:
                    missing.setdefault(c, []).append(f"{os.path.relpath(path, root)}:{lineno}")

sys.stdout.reconfigure(encoding="utf-8")
if not missing:
    print("OK：所有字元字型裡都有")
    sys.exit(0)
for c, places in sorted(missing.items()):
    print(f"缺字 {c!r} (U+{ord(c):04X})：{', '.join(places[:5])}")
sys.exit(1)
