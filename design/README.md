# Personal display workshop

Author layouts here; use the sibling `zmk-config` checkout for its pinned LVGL,
Zephyr, ZMK and firmware toolchain. `origin` is Rick's fork; `upstream` is
Carrefinho's module. Personal screens remain in the module's existing layout
directory so they can be selected by ordinary ZMK configurations.

| Area | Purpose |
| --- | --- |
| `boards/shields/prospector_adapter/src/layouts/` | Firmware layouts, views and assets |
| `tools/preview/` | Native LVGL renderer and regression checks for Vaporwave and Catppuccin |
| `scripts/vaporwave_font.py` | Reproducible pixel fonts from pinned LVGL Unscii 8 |
| `design/display-rules.md` | Hardware evidence and UI design constraints |
| `skills/prospector-display/` | Agent skill, linked from `~/.agents/skills/prospector-display` |
| `.preview/` | Ignored build output, RGB565 captures and standalone HTML gallery |

## Design loop

Host prerequisites: C compiler, CMake, Python 3 and just. Firmware also uses Nix.
The existing west workspace supplies LVGL; this checkout does not download a
second copy. Set `ZMK_WORKSPACE` if it is somewhere other than
`~/personal/zmk-config`.

```sh
just preview
just firmware
```

Catppuccin uses `just firmware catppuccin`; `just preview` renders both themes.
Its gallery is `.preview/catppuccin/index.html`, and its firmware artifact is
`firmware/hillside_d50_dongle_catppuccin.uf2` in the config workspace.
See [the Catppuccin design notes](catppuccin/README.md) for the concept,
typing-cat behavior and validation evidence.

Open `.preview/index.html` to inspect every captured state at native size. PNG
captures are alongside it. The optional rounded mask and content guides make
corner clearance visible; the mask is an estimate, not measured case geometry.
The automated checks cover WPM digit transitions and stale pixels, all seven
Hillside layers, USB/Bluetooth disconnected states, battery 0/9/100/unknown/off,
modifier labels, Caps Word and 1000 view updates. They exercise the actual view,
but do not simulate BLE, SPI, the case or the panel's optical behavior.

`just firmware` overrides the west-managed Prospector module with this checkout
using `ZMK_EXTRA_MODULES`. It retains the existing dongle orientation and board
options and writes `firmware/hillside_d50_dongle_vaporwave.uf2` in the config
workspace. The override persists in that target's CMake cache. To return that
build directory to the manifest module, run from `zmk-config`:

```sh
nix develop --command just _build_single xiao_ble//zmk \
  'hillside_d50_dongle prospector_adapter' '' hillside_d50_dongle_vaporwave \
  '-DCONFIG_PROSPECTOR_STATUS_SCREEN_VAPORWAVE=y -DCONFIG_ZMK_PM_SOFT_OFF=n -DCONFIG_PM_DEVICE=n -DZMK_EXTRA_MODULES='
```

For a new layout, follow a neighboring layout's Kconfig, CMake and screen-factory
registration. Keep event collection separate from rendering so a native runner
can compile the same view. Add a preview entry for that layout when it exists;
the current runner exercises Vaporwave and Catppuccin.

## Release

After reviewing and testing the layout, commit and push it to the personal fork
when publication is authorized. Then pin that reachable commit's full SHA in
`zmk-config/config/west.yml`, sync west, clear the local module override, and
rebuild from the pin. A development override is local; it does not change CI.
Record the module SHA, config SHA, build result and physical validation status.

The first hardware acceptance pass should check 0→9→10→100 WPM, both battery
corners at 100%, USB and disconnected Bluetooth text, every layer, and Caps Word.
Use the usual case, viewing angle, orientation and brightness. Copy the UF2 to
the dongle's bootloader volume when flashing is requested; a desktop render
cannot establish the final case clearance.
