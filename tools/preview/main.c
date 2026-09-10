/* SPDX-License-Identifier: MIT */
#include "view.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef PREVIEW_CATPPUCCIN
#define view_state catppuccin_state
#define view_battery catppuccin_battery
#define view_create catppuccin_create
#define view_update catppuccin_update
#else
#define view_state vaporwave_state
#define view_battery vaporwave_battery
#define view_create vaporwave_create
#define view_update vaporwave_update
#endif

static uint16_t buffer[280 * 60];
static uint16_t frame[280 * 240];
static unsigned failures;
static uint16_t initial_frame[280 * 240];

static void flush(lv_display_t *display, const lv_area_t *area, uint8_t *pixels) {
    uint16_t *src = (uint16_t *)pixels;
    for (int y = area->y1; y <= area->y2; y++) {
        for (int x = area->x1; x <= area->x2; x++) {
            frame[y * 280 + x] = *src++;
        }
    }
    lv_display_flush_ready(display);
}

static void check_labels(lv_obj_t *obj, const char *scenario) {
    if (lv_obj_check_type(obj, &lv_label_class)) {
        const char *text = lv_label_get_text(obj);
        lv_area_t area, parent;
        lv_obj_get_coords(obj, &area);
        lv_obj_get_content_coords(lv_obj_get_parent(obj), &parent);
        lv_point_t size;
        lv_text_get_size(&size, text, lv_obj_get_style_text_font(obj, 0),
                         lv_obj_get_style_text_letter_space(obj, 0),
                         lv_obj_get_style_text_line_space(obj, 0),
                         LV_COORD_MAX, LV_TEXT_FLAG_NONE);
        if (size.x > lv_obj_get_content_width(obj) ||
            area.x1 < parent.x1 || area.x2 > parent.x2 ||
            area.y1 < parent.y1 || area.y2 > parent.y2 ||
            lv_obj_get_height(obj) > size.y) {
            fprintf(stderr, "%s: clipped/wrapped '%s': text %dx%d, box (%d,%d)-(%d,%d)\n",
                    scenario, text, (int)size.x, (int)size.y,
                    (int)area.x1, (int)area.y1, (int)area.x2, (int)area.y2);
            failures++;
        }
        /* Conservative content envelope for corner-adjacent rows. */
        if ((area.y1 < 32 || area.y1 > 200) &&
            (area.x1 < 24 || area.x2 > 255 || area.y1 < 12 || area.y2 > 219)) {
            fprintf(stderr, "%s: corner clearance '%s': (%d,%d)-(%d,%d)\n",
                    scenario, text, (int)area.x1, (int)area.y1, (int)area.x2, (int)area.y2);
            failures++;
        }
    }
    for (unsigned i = 0; i < lv_obj_get_child_count(obj); i++) {
        check_labels(lv_obj_get_child(obj, i), scenario);
    }
}

static void save(const char *name, struct view_state *state) {
    view_update(state);
    lv_refr_now(NULL);
    check_labels(lv_screen_active(), name);
    /* Actual draw commands (including battery icons) must fit the footer. */
    uint16_t background = frame[200 * 280];
    bool footer_clips = false;
    for (int y = 200; y < 240; y++) {
        for (int x = 0; x < 280; x++) {
            if (frame[y * 280 + x] != background &&
                (x < 24 || x > 255 || y > 219)) {
                footer_clips = true;
            }
        }
    }
    if (footer_clips) {
        fprintf(stderr, "%s: footer pixels outside safe envelope\n", name);
        failures++;
    }
    char path[128];
    snprintf(path, sizeof(path), "%s.ppm", name);
    FILE *file = fopen(path, "wb");
    if (!file) { perror(path); exit(2); }
    fprintf(file, "P6\n280 240\n255\n");
    for (int i = 0; i < 280 * 240; i++) {
        unsigned char rgb[] = {
            ((frame[i] >> 11) & 31) * 255 / 31,
            ((frame[i] >> 5) & 63) * 255 / 63,
            (frame[i] & 31) * 255 / 31,
        };
        fwrite(rgb, 1, 3, file);
    }
    fclose(file);
}

int main(void) {
    lv_init();
    lv_display_t *display = lv_display_create(280, 240);
    lv_display_set_color_format(display, LV_COLOR_FORMAT_RGB565);
    lv_display_set_buffers(display, buffer, NULL, sizeof(buffer), LV_DISPLAY_RENDER_MODE_PARTIAL);
    lv_display_set_flush_cb(display, flush);
    lv_screen_load(view_create());
    struct view_state state = {
        .layer = "BASE", .layer_count = 7, .usb = true, .output_connected = true,
        .batteries = {{true, true, 100}, {true, true, 100}},
    };
    static const int speeds[] = {0, 9, 10, 99, 100, 999, 0};
    for (unsigned i = 0; i < sizeof(speeds) / sizeof(speeds[0]); i++) {
        state.wpm = speeds[i];
        char name[32];
        snprintf(name, sizeof(name), "wpm-%d", speeds[i]);
        save(name, &state);
        lv_obj_t *wpm = lv_obj_get_child(lv_screen_active(), 1);
        char expected[16];
        snprintf(expected, sizeof(expected), "%d", state.wpm);
        lv_area_t area;
        lv_obj_get_coords(wpm, &area);
        if (strcmp(lv_label_get_text(wpm), expected) || area.x2 != 255) {
            fprintf(stderr, "%s: expected digits anchored at x=255\n", name);
            failures++;
        }
        if (i == 0) {
            memcpy(initial_frame, frame, sizeof(frame));
        } else if (i == 6 && memcmp(initial_frame, frame, sizeof(frame))) {
            fprintf(stderr, "WPM returning to zero left stale pixels\n");
            failures++;
        }
    }
    static const char *layers[] = {"BASE", "GAME", "NAV", "FN", "NUM", "SYS", "MOUSE"};
    for (int i = 0; i < 7; i++) {
        strcpy(state.layer, layers[i]);
        state.layer_index = i;
        state.mods = i;
        save(layers[i], &state);
    }
    state.usb = false;
    state.profile = 4;
    state.output_connected = false;
    state.caps_word = true;
    state.batteries[0].level = 9;
    state.batteries[1].connected = false;
    save("disconnected", &state);
    state.usb = true;
    state.batteries[0] = (struct view_battery){true, true, 0};
    state.batteries[1] = (struct view_battery){true, false, 0};
    save("unknown", &state);
#ifdef PREVIEW_CATPPUCCIN
    state = (struct view_state){
        .layer = "BASE", .layer_count = 7, .usb = true, .output_connected = true,
        .batteries = {{true, true, 100}, {true, true, 100}},
    };
    save("resting", &state);
    memcpy(initial_frame, frame, sizeof(frame));
    state.wpm = 42;
    state.mods = 2;
    save("typing-left", &state);
    uint16_t left_frame[280 * 240];
    memcpy(left_frame, frame, sizeof(frame));
    state.right_paw = true;
    save("typing-right", &state);
    if (!memcmp(left_frame, frame, sizeof(frame))) {
        fprintf(stderr, "Typing paw did not alternate\n");
        failures++;
    }
    state.wpm = 0;
    state.mods = 0;
    save("resting-again", &state);
    if (memcmp(initial_frame, frame, sizeof(frame))) {
        fprintf(stderr, "Returning to rest left stale pixels\n");
        failures++;
    }
    state.usb = false;
    state.profile = 255;
    state.output_connected = false;
    state.mods = 15;
    state.caps_word = true;
    strcpy(state.layer, "ABCDEFGHIJKLMNOPQRSTUVWXYZ12345");
    save("long-name", &state);
    strcpy(state.layer, "CUSTOM");
    save("compact-name", &state);
    strcpy(state.layer, "BASE");
    state.usb = true;
    state.output_connected = true;
    state.mods = 0;
    state.caps_word = false;
    save("restored", &state);
    if (memcmp(initial_frame, frame, sizeof(frame))) {
        fprintf(stderr, "Restoring labels left stale pixels\n");
        failures++;
    }
#endif
    for (int i = 0; i < 1000; i++) {
        state.mods = i % 16;
        state.wpm = i % 150;
        view_update(&state);
        lv_refr_now(display);
    }
    printf("%u layout failures; rendered layers, 0/9/10/99/100/999 WPM, battery states, Caps Word, 1000 updates.\n", failures);
    return failures ? 1 : 0;
}
