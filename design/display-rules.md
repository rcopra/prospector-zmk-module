# Prospector screen design

## Establish the viewport

Keep controller memory, addressable pixels, glass outline and case opening
distinct. Their dimensions answer different questions.

Waveshare specifies 240 × 280 addressable pixels and an active area of about
27.97 × 32.63 mm. Its drawing separately marks the viewing area, polarizer, LCD
and touch-panel outlines. The outer touch-panel radius callout is not the
active-pixel corner radius. Sources: [module specifications](https://www.waveshare.com/product/displays/lcd-oled/lcd-oled-3/1.69inch-touch-lcd-module.htm)
and [dimension drawing](https://files.waveshare.com/wiki/1.69inch-Touch-LCD-Module/1.69inch_Touch_LCD_Module_2D_Drawing.pdf).

For the running firmware, inspect these sources rather than inferring rotation
or color format from a screenshot:

- `boards/shields/prospector_adapter/boards/xiao_ble_zmk.overlay`: dimensions,
  offsets and panel initialization.
- `drivers/display/display_st7789v.c`: the module overrides Zephyr's driver;
  inspect its rotation, margins and reported resolution.
- `boards/shields/prospector_adapter/src/display_rotate_init.c`: orientation
  selection.
- The target's generated `zephyr/.config` and `zephyr.dts` in `zmk-config/.build`:
  resolved orientation, RGB format, buffers, heap and brightness.

At the September 2026 investigation, the logical canvas is 280 × 240 landscape,
RGB565, with native offsets (0,20). The existing target sets
`PROSPECTOR_ROTATE_DISPLAY_180=y`, selecting the driver's 90° orientation; the
alternative selects 270°. The driver swaps the offset axes for landscape.
Waveshare describes 240 × 320 controller RAM behind the 240 × 280 panel in its
[module wiki](https://www.waveshare.net/wiki/1.69inch_Touch_LCD_Module).
Unused RAM rows do not establish a visible black stripe: the offset maps the
addressable window into controller memory.

## Safe area

The user's `IMG_1819.jpeg` shows USB near the upper-left curve, the left battery
icon near the lower-left curve, and the right percentage near the lower-right
curve. It shows black surround on all sides. Perspective, glare and the fitted
case prevent a reliable per-corner radius or a unique offset-stripe diagnosis.
Earlier STL aperture/radius estimates remain unverified and are not design
dimensions. The upstream hero image demonstrates generous spacing and dark
surrounds; it is a visual reference rather than a calibration target.

Vaporwave's conservative content envelopes use inclusive coordinates:

| Content | Envelope |
| --- | --- |
| Header | x=24…255, y=12…28 |
| Battery row, including icons and every label state | x=24…255, y=202…219 |
| Mid-screen content | Keep full glyph bounds within the canvas and parent boxes |

Decorative backgrounds and separator lines may bleed to the edge. Essential
glyphs and icons need clearance, including at maximum values. Other layouts
may choose different regions if previews and device evidence support them.
The gallery's 4 px inset with radius 24–40 px is a stress mask, not hardware
truth. Keep labels clear across that range; use a frontal device photo and
pixel-marked calibration screen if closer-to-edge content becomes necessary.

## Typography and dynamic state

Measure actual font advances, line height and parent content bounds in LVGL.
Unscii 16 has a 16 px advance and a 17 px line height: `10 WPM` is 96 px wide,
`R 100%` is 96 px, `CTRL` is 64 px and `SHIFT` is 80 px. This reproduced WPM
wrapping in its 88 px box, battery text beyond x=279, and modifier clipping in
59 px boxes. Setting clip mode alone can hide the symptom while losing text.

Use fixed-width, right-aligned boxes for changing numbers and verify growth
and shrinkage (0, 9, 10, 99, 100 and back to 0). Give status labels a deliberate
single-line policy. Keep disconnected/unknown states distinct from zero.
Vaporwave retains full footer/modifier labels with an Unscii-derived 8 px
advance, 16 px tall glyph design (18 px line height). Regenerate it with:

```sh
python3 scripts/vaporwave_font.py "$ZMK_WORKSPACE/modules/lib/gui/lvgl/src/font/lv_font_unscii_8.c" --status
```

The large layer font remains separate. Long layer names need an explicit
policy: this view falls back to Unscii 16 above six characters and clips text
beyond its 260 px width. Review real configured names; the stock seven fit.

## Color and memory

Review the RGB565 output, not only CSS hex swatches. Quantize R/B to 5 bits and
G to 6 bits, expand them back to RGB, then evaluate contrast against the actual
background, including plates and active fills. Arbitrary input hex values are
valid; they need not sit on an invented RGB888 grid. RGB565 still changes the
displayed result. A practical target here is at least 4:1 for small status text
and higher contrast for active information; this is a design target, not an
accessibility certification. Classic Outrun's quantized DIM/BG is about 4.22:1,
PINK/BG 5.65:1, CYAN/BG 15.32:1. Physical brightness, viewing angle and glare
still require device review. Keep bright art behind text under a dark plate.

A 280 × 240 RGB565 framebuffer costs 134,400 bytes before overhead. Retain
partial rendering and lightweight draw commands for this MCU. Inspect the
resolved buffer count/size, LVGL pool and linker RAM usage when adding fonts,
images or animation. The host renderer's full capture buffer is desktop-only;
it is not evidence that an equally large firmware allocation will fit.

## Evidence for completion

Run the native preview, inspect raw and masked captures, run the firmware
build, and verify that its module path points to the intended checkout. Report
text/geometry checks, linker memory usage, and physical acceptance separately.
This investigation's desktop checks prove the software clipping and wrapping
fixes; final clearance against the user's fitted case remains a device check.
