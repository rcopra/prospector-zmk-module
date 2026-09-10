#!/usr/bin/env python3
"""Package the native LVGL RGB565 captures into a standalone review gallery."""
import base64
import html
import struct
import sys
import zlib
from pathlib import Path


def png(ppm):
    magic, dimensions, maximum, pixels = ppm.read_bytes().split(b"\n", 3)
    width, height = map(int, dimensions.split())
    assert magic == b"P6" and maximum == b"255" and len(pixels) == width * height * 3

    def chunk(kind, data):
        return (struct.pack(">I", len(data)) + kind + data
                + struct.pack(">I", zlib.crc32(kind + data)))

    rows = b"".join(b"\0" + pixels[y * width * 3:(y + 1) * width * 3] for y in range(height))
    return (b"\x89PNG\r\n\x1a\n"
            + chunk(b"IHDR", struct.pack(">IIBBBBB", width, height, 8, 2, 0, 0, 0))
            + chunk(b"IDAT", zlib.compress(rows)) + chunk(b"IEND", b""))


directory = Path(sys.argv[1] if len(sys.argv) > 1 else ".preview")
cards = []
for ppm in sorted(directory.glob("*.ppm")):
    data = png(ppm)
    ppm.with_suffix(".png").write_bytes(data)
    encoded = base64.b64encode(data).decode()
    cards.append(f'<figure><div class="panel"><img alt="{html.escape(ppm.stem)}" '
                 f'src="data:image/png;base64,{encoded}"><div class="safe"></div></div>'
                 f'<figcaption>{html.escape(ppm.stem)}</figcaption></figure>')
if not cards:
    raise SystemExit("Run the native preview first; no PPM captures found.")
theme = sys.argv[2] if len(sys.argv) > 2 else "Vaporwave"
page = '''<!doctype html><html lang="en"><meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>Prospector · Vaporwave review</title>
<style>
*{box-sizing:border-box}body{margin:32px;background:#eeeef2;color:#242432;font:16px system-ui}
h1{font-size:26px;margin-bottom:8px}p{max-width:850px;line-height:1.5}
nav{display:flex;gap:24px;flex-wrap:wrap;margin:24px 0}
main{display:grid;grid-template-columns:repeat(auto-fit,280px);gap:28px}
figure{margin:0}.panel{width:280px;height:240px;position:relative;background:#000}
img{width:280px;height:240px;image-rendering:pixelated;display:block}
body.mask img{clip-path:inset(4px round var(--radius,32px))}
figcaption{margin-top:8px;font-size:14px}.safe{display:none;position:absolute;inset:0;pointer-events:none}
body.guides .safe{display:block}.safe:before,.safe:after{content:"";position:absolute;left:24px;
width:232px;border:1px dashed #fff;background:#fff1}
.safe:before{top:12px;height:17px}.safe:after{top:202px;height:18px}
</style><h1>Prospector · Vaporwave</h1>
<p>Actual LVGL view, rendered at 280 × 240 in RGB565. The corner mask is a conservative
design check, not a measured case outline. Use raw pixels to distinguish layout overflow
from masking; confirm the fitted case on the device.</p>
<nav><label><input id="mask" type="checkbox"> Corner mask</label>
<label>Mask radius <input id="radius" type="range" min="24" max="40" value="32"> <output>32 px</output></label>
<label><input id="guides" type="checkbox"> Content guides</label></nav>
<main>''' + "\n".join(cards) + '''</main><script>
for(const id of ['mask','guides'])document.getElementById(id).onchange=e=>document.body.classList.toggle(id,e.target.checked);
document.getElementById('radius').oninput=e=>{document.body.style.setProperty('--radius',e.target.value+'px');document.querySelector('output').value=e.target.value+' px';};
</script></html>'''
(directory / "index.html").write_text(page.replace("Vaporwave", html.escape(theme)))
print(directory / "index.html")
