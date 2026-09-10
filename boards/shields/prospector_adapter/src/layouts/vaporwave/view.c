/* SPDX-License-Identifier: MIT */
#include "view.h"
#include <string.h>

LV_FONT_DECLARE(vaporwave_pixel_40);

#define BG 0x0C041B
#define GRID 0x34104F
#define CYAN 0x14EAF0
#define PINK 0xFF29AC
#define DIM 0x76509E
#define WHITE 0xF5ECFF
#define PEACH 0xFF927E

static lv_obj_t *screen;
static lv_obj_t *layer_label;
static lv_obj_t *output_label;
static lv_obj_t *wpm_label;
static lv_obj_t *mod_boxes[4];
static lv_obj_t *mod_labels[4];
static lv_obj_t *battery_labels[2];
static struct vaporwave_state displayed;

static void rect(lv_layer_t *layer, int x, int y, int w, int h, uint32_t color) {
    lv_draw_rect_dsc_t dsc;
    lv_draw_rect_dsc_init(&dsc);
    dsc.bg_color = lv_color_hex(color);
    lv_area_t area = {x, y, x + w - 1, y + h - 1};
    lv_draw_rect(layer, &dsc, &area);
}

static void line(lv_layer_t *layer, int x1, int y1, int x2, int y2, uint32_t color) {
    lv_draw_line_dsc_t dsc;
    lv_draw_line_dsc_init(&dsc);
    dsc.p1 = (lv_point_precise_t){x1, y1};
    dsc.p2 = (lv_point_precise_t){x2, y2};
    dsc.color = lv_color_hex(color);
    dsc.width = 1;
    lv_draw_line(layer, &dsc);
}

static void draw_background(lv_event_t *event) {
    lv_layer_t *layer = lv_event_get_layer(event);
    rect(layer, 0, 27, 280, 1, GRID);

    /* Striped sunset and perspective grid use draw commands, not a framebuffer. */
    static const struct { uint8_t x, y, w, h; uint32_t color; } stripes[] = {
        {130, 39, 20, 3, 0xFF20B3}, {116, 42, 48, 6, 0xFF24AD},
        {109, 50, 62, 6, 0xFF2BA6}, {104, 59, 72, 6, 0xFF389F},
        {102, 68, 76, 6, 0xFF5796}, {101, 78, 78, 5, 0xFF7489},
        {103, 88, 74, 4, PEACH}, {106, 97, 68, 3, 0xF573A9},
    };
    for (unsigned i = 0; i < sizeof(stripes) / sizeof(stripes[0]); i++) {
        rect(layer, stripes[i].x, stripes[i].y, stripes[i].w, stripes[i].h,
             stripes[i].color);
    }
    for (int x = -280; x <= 560; x += 70) {
        line(layer, 140 + (x - 140) / 12, 102, x, 158, GRID);
    }
    static const uint8_t rows[] = {103, 106, 111, 119, 132, 152};
    for (unsigned i = 0; i < sizeof(rows) / sizeof(rows[0]); i++) {
        line(layer, 0, rows[i], 279, rows[i], GRID);
    }

    int count = displayed.layer_count;
    int step = count ? LV_MIN(18, 252 / count) : 18;
    int size = LV_MIN(8, step - 2);
    for (int i = 0; i < count; i++) {
        rect(layer, (280 - count * step + step - size) / 2 + i * step,
             145, size, 8, i == displayed.layer_index ? CYAN : DIM);
    }

    rect(layer, 0, 203, 280, 1, PINK);
    for (int i = 0; i < 2; i++) {
        const struct vaporwave_battery *battery = &displayed.batteries[i];
        uint32_t color = !battery->connected ? DIM :
            battery->known && battery->level < 20 ? PEACH : CYAN;
        int x = 10 + i * 142;
        rect(layer, x, 214, 29, 15, color);
        rect(layer, x + 2, 216, 25, 11, BG);
        rect(layer, x + 29, 219, 3, 5, color);
        if (battery->connected && battery->known) {
            for (int segment = 0; segment < 4; segment++) {
                if (battery->level > segment * 25) {
                    rect(layer, x + 4 + segment * 6, 218, 4, 7, color);
                }
            }
        }
    }
}

static lv_obj_t *label(lv_obj_t *parent, const char *text, int x, int y,
                       uint32_t color) {
    lv_obj_t *obj = lv_label_create(parent);
    lv_obj_set_style_text_font(obj, &lv_font_unscii_16, 0);
    lv_obj_set_style_text_color(obj, lv_color_hex(color), 0);
    lv_label_set_text(obj, text);
    lv_obj_set_pos(obj, x, y);
    return obj;
}

lv_obj_t *vaporwave_create(void) {
    screen = lv_obj_create(NULL);
    lv_obj_remove_style_all(screen);
    lv_obj_set_size(screen, 280, 240);
    lv_obj_set_style_bg_color(screen, lv_color_hex(BG), 0);
    lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, 0);
    lv_obj_remove_flag(screen, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_event_cb(screen, draw_background, LV_EVENT_DRAW_MAIN, NULL);

    output_label = label(screen, "USB", 12, 7, CYAN);
    wpm_label = label(screen, "0 WPM", 180, 7, DIM);
    lv_obj_set_width(wpm_label, 88);
    lv_obj_set_style_text_align(wpm_label, LV_TEXT_ALIGN_RIGHT, 0);

    layer_label = label(screen, "BASE", 10, 84, WHITE);
    lv_obj_set_style_text_font(layer_label, &vaporwave_pixel_40, 0);
    lv_obj_set_width(layer_label, 260);
    lv_obj_set_style_text_align(layer_label, LV_TEXT_ALIGN_CENTER, 0);
    lv_label_set_long_mode(layer_label, LV_LABEL_LONG_CLIP);

    static const char *names[] = {"CMD", "OPT", "CTRL", "SHIFT"};
    for (int i = 0; i < 4; i++) {
        mod_boxes[i] = lv_obj_create(screen);
        lv_obj_remove_style_all(mod_boxes[i]);
        lv_obj_set_pos(mod_boxes[i], 10 + i * 67, 167);
        lv_obj_set_size(mod_boxes[i], 59, 27);
        lv_obj_set_style_bg_color(mod_boxes[i], lv_color_hex(BG), 0);
        lv_obj_set_style_bg_opa(mod_boxes[i], LV_OPA_COVER, 0);
        lv_obj_set_style_border_width(mod_boxes[i], 1, 0);
        mod_labels[i] = label(mod_boxes[i], names[i], 0, 0, DIM);
        lv_obj_center(mod_labels[i]);
    }
    battery_labels[0] = label(screen, "L OFF", 49, 213, DIM);
    battery_labels[1] = label(screen, "R OFF", 191, 213, DIM);
    return screen;
}

void vaporwave_update(const struct vaporwave_state *state) {
    if (memcmp(&displayed, state, sizeof(displayed)) == 0) {
        return;
    }
    bool redraw_background = displayed.layer_index != state->layer_index ||
        displayed.layer_count != state->layer_count ||
        memcmp(displayed.batteries, state->batteries, sizeof(state->batteries)) != 0;
    displayed = *state;
    lv_label_set_text(layer_label, state->layer);
    const lv_font_t *font = strlen(state->layer) > 6 ? &lv_font_unscii_16 :
                                                    &vaporwave_pixel_40;
    lv_obj_set_style_text_font(layer_label, font, 0);
    lv_obj_set_y(layer_label, font == &vaporwave_pixel_40 ? 84 : 99);
    if (state->usb) {
        lv_label_set_text(output_label, state->output_connected ? "USB" : "USB --");
    } else {
        lv_label_set_text_fmt(output_label, "BT %u%s", state->profile + 1,
                              state->output_connected ? "" : " --");
    }
    lv_obj_set_style_text_color(output_label,
        lv_color_hex(state->output_connected ? CYAN : DIM), 0);
    lv_label_set_text_fmt(wpm_label, "%d WPM", state->wpm);
    lv_obj_set_style_text_color(wpm_label, lv_color_hex(0xBC95E9), 0);

    for (int i = 0; i < 4; i++) {
        bool active = (state->mods & (1 << i)) || (i == 3 && state->caps_word);
        uint32_t color = active ? (i == 3 ? PINK : CYAN) : DIM;
        lv_obj_set_style_border_color(mod_boxes[i], lv_color_hex(color), 0);
        lv_obj_set_style_text_color(mod_labels[i], lv_color_hex(color), 0);
        lv_obj_set_style_bg_color(mod_boxes[i],
            lv_color_hex(active ? (i == 3 ? 0x300823 : 0x04262E) : BG), 0);
    }
    lv_label_set_text(mod_labels[3], state->caps_word ? "CAPS" : "SHIFT");
    lv_obj_center(mod_labels[3]);
    for (int i = 0; i < 2; i++) {
        const struct vaporwave_battery *battery = &state->batteries[i];
        char side = i ? 'R' : 'L';
        uint32_t color = DIM;
        if (!battery->connected) {
            lv_label_set_text_fmt(battery_labels[i], "%c OFF", side);
        } else if (!battery->known) {
            lv_label_set_text_fmt(battery_labels[i], "%c --", side);
            color = CYAN;
        } else {
            lv_label_set_text_fmt(battery_labels[i], "%c %u%%", side, battery->level);
            color = battery->level < 20 ? PEACH : CYAN;
        }
        lv_obj_set_style_text_color(battery_labels[i], lv_color_hex(color), 0);
    }
    if (redraw_background) {
        lv_obj_invalidate(screen);
    }
}
