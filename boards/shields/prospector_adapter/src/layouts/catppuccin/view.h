/* SPDX-License-Identifier: MIT */
#pragma once

#include <lvgl.h>

struct catppuccin_battery {
    bool connected;
    bool known;
    uint8_t level;
};

struct catppuccin_state {
    char layer[32];
    uint8_t layer_index;
    uint8_t layer_count;
    uint8_t mods; /* Cmd, Opt, Ctrl, Shift, from least significant bit. */
    bool caps_word;
    int wpm;
    bool right_paw; /* Alternates on key presses; rests when WPM reaches zero. */
    bool usb;
    bool output_connected;
    uint8_t profile;
    struct catppuccin_battery batteries[2];
};

lv_obj_t *catppuccin_create(void);
void catppuccin_update(const struct catppuccin_state *state);
