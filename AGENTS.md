# Personal Prospector layouts

This fork is the authoring workspace for Rick's Prospector screens. The sibling
`zmk-config` repo owns keyboard configuration and the pinned west dependencies.

Before designing or debugging a screen, read [design/README.md](design/README.md)
for the build/preview workflow and [design/display-rules.md](design/display-rules.md)
for the physical viewport, text measurement, and hardware acceptance rules.

Keep layouts under `boards/shields/prospector_adapter/src/layouts/` and preserve
the upstream registration conventions. Vaporwave separates ZMK events in
`status_screen.c` from an LVGL-only `view.c`; the native preview compiles that
same view. Extend preview scenarios when changing visible state or geometry.

Run `just preview` and `just firmware` for Vaporwave changes. Inspect the rendered
images as well as the checks. Report desktop validation and physical-device
validation separately. Release pinning is described in `design/README.md`.
