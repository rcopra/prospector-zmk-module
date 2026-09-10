/* SPDX-License-Identifier: MIT */
#pragma once

#include <lvgl.h>

struct vaporwave_battery {
    bool connected;
    bool known;
    uint8_t level;
};

struct vaporwave_state {
    char layer[32];
    uint8_t layer_index;
    uint8_t layer_count;
    uint8_t mods; /* Cmd, Opt, Ctrl, Shift, from least significant bit. */
    bool caps_word;
    int wpm;
    bool usb;
    bool output_connected;
    uint8_t profile;
    struct vaporwave_battery batteries[2];
};

lv_obj_t *vaporwave_create(void);
void vaporwave_update(const struct vaporwave_state *state);
