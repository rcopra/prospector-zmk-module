---
name: prospector-display
description: Design Prospector ZMK dongle layouts or debug display clipping, text wrapping, color, and rendering on the Waveshare 1.69-inch screen.
---

# Prospector displays

## Establish the workspace

Use the personal module checkout at `~/personal/prospector-zmk-module` for
authoring, and `~/personal/zmk-config` for keyboard configuration and the pinned
west toolchain. If the checkouts have moved, locate them through their remotes
and `config/west.yml` before editing. Read the module's `AGENTS.md` and
[design workflow](/Users/rick/personal/prospector-zmk-module/design/README.md).
Finish this step with an identified layout, cleanly understood git state, and
the intended module path for the build.

## Establish the visible contract

Before positioning content, read the
[display rules and evidence](/Users/rick/personal/prospector-zmk-module/design/display-rules.md).
They distinguish controller RAM, logical viewport, rounded glass and case
masking, and point to the authoritative driver/configuration files.

Inspect available device photos with an image-capable tool. Label measurements,
estimates and unknowns separately. Preserve the user's orientation and visual
style unless the task calls for changing them. For new hardware or changed
drivers, verify the resolved viewport, rotation, offsets, color format and
memory limits before carrying over the existing safe-area assumptions.
Finish with explicit content envelopes and worst-case label strings.

## Render the actual view

Keep ZMK event collection separate from the LVGL view, using the existing
layout conventions. Render the same C view and fonts in the native preview;
use mockups for visual exploration, then validate against firmware code.

Exercise changing digit counts in both directions, both batteries at 100%,
zero/unknown/disconnected states, the longest configured layer/output text,
modifiers and Caps Word. Measure glyph advances and line height against parent
content bounds. Include icons in clearance checks. For a reported bug, capture
a failing scenario before editing and rerun it after the fix.

Finish when text is legible and complete, dynamic updates fit their envelopes
without stale pixels, and the raw and masked renders have been visually
inspected. A conservative mask proves clearance against that mask; it does
not measure the user's case.

## Build and hand off

Run the module's preview and firmware recipes. Confirm the selected module
path and report FLASH/RAM use. Keep desktop capture buffers out of firmware
and check resource use when adding fonts, bitmaps or animation.

Follow the workflow document's local-override and release paths. Publish and
pin a reachable fork commit when authorized; otherwise leave a reviewable
local change with a tested development build. State when a cached local
override differs from the manifest pin.

Finish with the workspace, preview and UF2 paths, validation results, and
physical acceptance status. When device access is unavailable, provide the
specific on-device checks still needed. A successful build establishes
firmware validity; final bezel clearance needs device evidence.
