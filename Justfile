zmk_workspace := env('ZMK_WORKSPACE', env('HOME') + '/personal/zmk-config')

default:
    @just --list

# Render the real LVGL view, check text/footers, and generate a review gallery.
preview:
    #!/usr/bin/env bash
    set -euo pipefail
    cmake -S tools/preview -B .preview -DZMK_WORKSPACE="{{ zmk_workspace }}"
    cmake --build .preview --parallel
    ctest --test-dir .preview --output-on-failure
    python3 tools/preview/gallery.py .preview
    python3 tools/preview/gallery.py .preview/catppuccin Catppuccin

# Build this checkout using the config repo's pinned ZMK environment.
firmware layout="vaporwave":
    #!/usr/bin/env bash
    set -euo pipefail
    module_dir="$(pwd -P)"
    case "{{ layout }}" in
        vaporwave) layout_config=VAPORWAVE ;;
        catppuccin) layout_config=CATPPUCCIN ;;
        *) echo 'Layout must be vaporwave or catppuccin' >&2; exit 1 ;;
    esac
    cd "{{ zmk_workspace }}"
    nix develop --command just _build_single xiao_ble//zmk \
        'hillside_d50_dongle prospector_adapter' '' hillside_d50_dongle_{{ layout }} \
        "-DCONFIG_PROSPECTOR_STATUS_SCREEN_$layout_config=y -DCONFIG_ZMK_PM_SOFT_OFF=n -DCONFIG_PM_DEVICE=n -DZMK_EXTRA_MODULES=$module_dir"
