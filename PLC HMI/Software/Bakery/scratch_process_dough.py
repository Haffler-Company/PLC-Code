import sys
from pathlib import Path
from PIL import Image, ImageFilter

sys.stdout.reconfigure(encoding='utf-8')

img_path = Path(r"C:\Users\Haffler\.gemini\antigravity\brain\b833ee1e-c67e-4bb2-8c2e-2f3001002111\bakery_dough_ball_1788951507486.jpg")
if not img_path.exists():
    print("ERROR: Image not found at", img_path)
    sys.exit(1)

im = Image.open(img_path).convert("RGBA")
print("Original size:", im.size)

# The background is solid black (0,0,0). Let's extract the dough ball with smooth alpha.
# Find bounding box where brightness > 15
gray = im.convert("L")
threshold = 20
mask = gray.point(lambda p: 255 if p > threshold else 0)
bbox = mask.getbbox()
print("Dough bbox:", bbox)

# Crop to dough ball with a tiny margin
crop_im = im.crop(bbox)

# Create soft alpha mask based on dark background
# Smooth edges: pixels close to black fade to 0 alpha
pixels = crop_im.load()
w, h = crop_im.size
for y in range(h):
    for x in range(w):
        r, g, b, a = pixels[x, y]
        # Luminance
        lum = 0.299 * r + 0.587 * g + 0.114 * b
        if lum < 15:
            pixels[x, y] = (0, 0, 0, 0)
        elif lum < 40:
            alpha = int(((lum - 15) / 25.0) * 255)
            pixels[x, y] = (r, g, b, alpha)

# Target size for UI: 84 x 80 (well-proportioned for 480x320 main screen next to counter)
target_w, target_h = 84, 80
resized_im = crop_im.resize((target_w, target_h), Image.Resampling.LANCZOS)
print("Resized to:", resized_im.size)

# Preview save
preview_path = Path(r"c:\Users\Haffler\Documents\Haffler\PLC\PLC HMI\Software\Bakery\tools\dough_preview.png")
resized_im.save(preview_path)
print("Saved preview to:", preview_path)

# Export to LVGL C array
out_c = Path(r"c:\Users\Haffler\Documents\Haffler\PLC\PLC HMI\Software\Bakery\V.1.1.0\ChoneProject\ui_img_chone_gir.c")

w, h = resized_im.size
data_bytes = []
for y in range(h):
    for x in range(w):
        r, g, b, a = resized_im.getpixel((x, y))
        # Convert RGB888 to RGB565 (LVGL little-endian)
        r5 = (r >> 3) & 0x1F
        g6 = (g >> 2) & 0x3F
        b5 = (b >> 3) & 0x1F
        rgb565 = (r5 << 11) | (g6 << 5) | b5
        low_byte = rgb565 & 0xFF
        high_byte = (rgb565 >> 8) & 0xFF
        data_bytes.extend([low_byte, high_byte, a])

header = f"""// Generated Bakery Dough Ball (Choneh) image asset for LVGL 8.3
// Size: {w}x{h}, format: LV_IMG_CF_TRUE_COLOR_ALPHA (RGB565 + Alpha)

#include "ui.h"

#ifndef LV_ATTRIBUTE_MEM_ALIGN
#define LV_ATTRIBUTE_MEM_ALIGN
#endif

const LV_ATTRIBUTE_MEM_ALIGN uint8_t ui_img_chone_gir_data[] = {{
"""

body = []
line = []
for idx, b in enumerate(data_bytes):
    line.append(f"0x{b:02X}")
    if len(line) == 16 or idx == len(data_bytes) - 1:
        body.append("    " + ",".join(line) + ",")
        line = []

footer = f"""}};

const lv_img_dsc_t ui_img_chone_gir = {{
    .header.always_zero = 0,
    .header.w = {w},
    .header.h = {h},
    .data_size = sizeof(ui_img_chone_gir_data),
    .header.cf = LV_IMG_CF_TRUE_COLOR_ALPHA,
    .data = ui_img_chone_gir_data
}};
"""

out_c.write_text(header + "\n".join(body) + "\n" + footer, encoding="utf-8")
print(f"[SUCCESS] Exported {out_c.name}: {w}x{h}, {len(data_bytes)} bytes")
