/* SPDX-License-Identifier: MIT */
#include "view.h"
#include <stdio.h>
#include <string.h>

LV_FONT_DECLARE(vaporwave_pixel_40);
LV_FONT_DECLARE(vaporwave_status_16);

/* Catppuccin Mocha: https://github.com/catppuccin/catppuccin */
#define BG 0x1E1E2E
#define SURFACE 0x45475A
#define TEXT 0xCDD6F4
#define DIM 0xA6ADC8
#define MAUVE 0xCBA6F7
#define GREEN 0xA6E3A1
#define PEACH 0xFAB387
#define BLUE 0x89B4FA
#define ROSE 0xF5E0DC
#define PINK 0xF5C2E7

static lv_obj_t *screen;
static lv_obj_t *layer_label;
static lv_obj_t *output_label;
static lv_obj_t *wpm_label;
static lv_obj_t *mod_boxes[4];
static lv_obj_t *mod_labels[4];
static lv_obj_t *battery_labels[2];
static struct catppuccin_state displayed;
static int cursor_x;

static void rect(lv_layer_t *layer, int x, int y, int w, int h, uint32_t color) {
    lv_draw_rect_dsc_t dsc;
    lv_draw_rect_dsc_init(&dsc);
    dsc.bg_color = lv_color_hex(color);
    lv_area_t area = {x, y, x + w - 1, y + h - 1};
    lv_draw_rect(layer, &dsc, &area);
}

static void draw_cat(lv_layer_t *layer) {
    /* Stepped silhouette and face, kept in flash as geometry rather than a framebuffer. */
    rect(layer, 110, 111, 8, 8, MAUVE);
    rect(layer, 162, 111, 8, 8, MAUVE);
    rect(layer, 108, 119, 18, 8, MAUVE);
    rect(layer, 154, 119, 18, 8, MAUVE);
    rect(layer, 104, 127, 72, 10, MAUVE);
    rect(layer, 100, 137, 80, 16, MAUVE);
    rect(layer, 112, 117, 4, 10, PINK);
    rect(layer, 164, 117, 4, 10, PINK);
    rect(layer, 116, 123, 6, 4, ROSE);
    rect(layer, 158, 123, 6, 4, ROSE);
    rect(layer, 136, 127, 8, 4, ROSE);
    rect(layer, 128, 131, 24, 4, ROSE);
    rect(layer, 120, 135, 40, 4, ROSE);
    rect(layer, 108, 139, 64, 14, ROSE);
    rect(layer, 120, 138, 4, 6, BG);
    rect(layer, 156, 138, 4, 6, BG);
    rect(layer, 112, 146, 10, 3, PINK);
    rect(layer, 158, 146, 10, 3, PINK);
    rect(layer, 132, 145, 3, 3, BG);
    rect(layer, 138, 145, 3, 3, BG);
    rect(layer, 144, 145, 3, 3, BG);
    rect(layer, 135, 148, 9, 2, BG);

    rect(layer, 88, 155, 104, 10, SURFACE);
    rect(layer, 92, 153, 96, 2, DIM);
    for (int i = 0; i < 12; i++) {
        bool hit = displayed.wpm > 0 && i == (displayed.right_paw ? 9 : 2);
        rect(layer, 93 + i * 8, 157, 5, 3, hit ? MAUVE : DIM);
    }
    for (int i = 0; i < 2; i++) {
        bool raised = displayed.wpm > 0 && i != displayed.right_paw;
        int x = 99 + i * 64;
        int y = raised ? 141 : 149;
        rect(layer, x, y, 18, 10, BG);
        rect(layer, x + 2, y - 2, 14, 14, BG);
        rect(layer, x + 2, y + 1, 14, 8, ROSE);
        rect(layer, x + 4, y, 10, 10, ROSE);
        if (raised) {
            rect(layer, x - 6 + i * 28, y - 3, 3, 5, MAUVE);
        }
    }
}

static void draw_background(lv_event_t *event) {
    lv_layer_t *layer = lv_event_get_layer(event);
    rect(layer, 24, 34, 232, 1, SURFACE);
    rect(layer, 24, 18, 5, 5, PINK);
    rect(layer, 33, 18, 5, 5, PEACH);
    rect(layer, 42, 18, 5, 5, GREEN);
    if (cursor_x <= 248) {
        rect(layer, cursor_x, 97, 8, 3, GREEN);
    }
    draw_cat(layer);
    rect(layer, 24, 196, 232, 1, SURFACE);
    for (int i = 0; i < 2; i++) {
        const struct catppuccin_battery *battery = &displayed.batteries[i];
        uint32_t color = !battery->connected ? DIM :
            battery->known && battery->level < 20 ? PEACH : GREEN;
        int x = 24 + i * 144;
        rect(layer, x, 205, 26, 12, color);
        rect(layer, x + 26, 209, 3, 4, color);
        rect(layer, x + 2, 207, 22, 8, BG);
        if (battery->connected && battery->known) {
            for (int segment = 0; segment < 4; segment++) {
                if (battery->level > segment * 25) {
                    rect(layer, x + 3 + segment * 5, 208, 4, 6, color);
                }
            }
        }
    }
}

static lv_obj_t *label(lv_obj_t *parent, const char *text, int x, int y,
                       uint32_t color) {
    lv_obj_t *obj = lv_label_create(parent);
    lv_obj_set_style_text_font(obj, &vaporwave_status_16, 0);
    lv_obj_set_style_text_color(obj, lv_color_hex(color), 0);
    lv_label_set_text(obj, text);
    lv_obj_set_pos(obj, x, y);
    return obj;
}

lv_obj_t *catppuccin_create(void) {
    memset(&displayed, 0, sizeof(displayed));
    cursor_x = 208;
    screen = lv_obj_create(NULL);
    lv_obj_remove_style_all(screen);
    lv_obj_set_size(screen, 280, 240);
    lv_obj_set_style_bg_color(screen, lv_color_hex(BG), 0);
    lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, 0);
    lv_obj_remove_flag(screen, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_event_cb(screen, draw_background, LV_EVENT_DRAW_MAIN, NULL);

    output_label = label(screen, "USB", 56, 12, TEXT);
    wpm_label = label(screen, "0", 224, 12, BLUE);
    lv_obj_set_width(wpm_label, 32);
    lv_obj_set_style_text_align(wpm_label, LV_TEXT_ALIGN_RIGHT, 0);
    lv_label_set_long_mode(wpm_label, LV_LABEL_LONG_CLIP);
    label(screen, "WPM", 192, 12, DIM);
    label(screen, "user@prospector:~", 24, 42, MAUVE);
    lv_obj_t *prompt = label(screen, ">", 24, 77, GREEN);
    lv_obj_set_style_text_font(prompt, &lv_font_unscii_16, 0);
    layer_label = label(screen, "BASE", 48, 61, GREEN);
    lv_obj_set_style_text_font(layer_label, &vaporwave_pixel_40, 0);
    lv_obj_set_width(layer_label, 208);
    lv_label_set_long_mode(layer_label, LV_LABEL_LONG_CLIP);

    static const char *names[] = {"CMD", "OPT", "CTRL", "SHIFT"};
    for (int i = 0; i < 4; i++) {
        mod_boxes[i] = lv_obj_create(screen);
        lv_obj_remove_style_all(mod_boxes[i]);
        lv_obj_remove_flag(mod_boxes[i], LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_pos(mod_boxes[i], 24 + i * 60, 172);
        lv_obj_set_size(mod_boxes[i], 52, 22);
        lv_obj_set_style_bg_opa(mod_boxes[i], LV_OPA_COVER, 0);
        mod_labels[i] = label(mod_boxes[i], names[i], 0, 0, DIM);
        lv_obj_center(mod_labels[i]);
    }
    for (int i = 0; i < 2; i++) {
        battery_labels[i] = label(screen, i ? "R OFF" : "L OFF", 64 + i * 144, 202, DIM);
    }
    return screen;
}

void catppuccin_update(const struct catppuccin_state *state) {
    displayed = *state;
    size_t length = strlen(state->layer);
    bool compact = length > 5;
    char name[27];
    /* All configured layers fit at 40 px. Custom names use 8 px cells, then ellipsis. */
    if (length > 26) {
        snprintf(name, sizeof(name), "%.23s...", state->layer);
    } else {
        snprintf(name, sizeof(name), "%.26s", state->layer);
    }
    lv_label_set_text(layer_label, name);
    lv_obj_set_style_text_font(layer_label,
        compact ? &vaporwave_status_16 : &vaporwave_pixel_40, 0);
    lv_obj_set_y(layer_label, compact ? 77 : 61);
    cursor_x = 48 + strlen(name) * (compact ? 8 : 40);
    if (state->usb) {
        lv_label_set_text(output_label, state->output_connected ? "USB" : "USB --");
    } else {
        lv_label_set_text_fmt(output_label, "BT %u%s", state->profile + 1,
                              state->output_connected ? "" : " --");
    }
    lv_obj_set_style_text_color(output_label,
        lv_color_hex(state->output_connected ? TEXT : PEACH), 0);
    lv_label_set_text_fmt(wpm_label, "%d", state->wpm);
    for (int i = 0; i < 4; i++) {
        bool active = (state->mods & (1 << i)) || (i == 3 && state->caps_word);
        uint32_t accent = i == 3 && state->caps_word ? PEACH : MAUVE;
        lv_obj_set_style_bg_color(mod_boxes[i], lv_color_hex(active ? accent : BG), 0);
        lv_obj_set_style_text_color(mod_labels[i], lv_color_hex(active ? BG : DIM), 0);
    }
    lv_label_set_text(mod_labels[3], state->caps_word ? "CAPS" : "SHIFT");
    lv_obj_center(mod_labels[3]);
    for (int i = 0; i < 2; i++) {
        const struct catppuccin_battery *battery = &state->batteries[i];
        char side = i ? 'R' : 'L';
        uint32_t color = DIM;
        if (!battery->connected) {
            lv_label_set_text_fmt(battery_labels[i], "%c OFF", side);
        } else if (!battery->known) {
            lv_label_set_text_fmt(battery_labels[i], "%c --", side);
            color = TEXT;
        } else {
            lv_label_set_text_fmt(battery_labels[i], "%c %u%%", side, battery->level);
            color = battery->level < 20 ? PEACH : GREEN;
        }
        lv_obj_set_style_text_color(battery_labels[i], lv_color_hex(color), 0);
    }
    lv_obj_invalidate(screen);
}
