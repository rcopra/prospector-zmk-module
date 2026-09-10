/* SPDX-License-Identifier: MIT */
#include "view.h"

#include <ctype.h>
#include <stdio.h>
#include <zephyr/kernel.h>
#include <zmk/ble.h>
#include <zmk/display.h>
#include <zmk/endpoints.h>
#include <zmk/event_manager.h>
#include <zmk/events/battery_state_changed.h>
#include <zmk/events/ble_active_profile_changed.h>
#include <zmk/events/endpoint_changed.h>
#include <zmk/events/keycode_state_changed.h>
#include <zmk/events/layer_state_changed.h>
#include <zmk/events/split_central_status_changed.h>
#include <zmk/events/usb_conn_state_changed.h>
#include <zmk/events/wpm_state_changed.h>
#include <zmk/hid.h>
#include <zmk/keymap.h>
#include <zmk/usb.h>
#include <zmk/wpm.h>
#ifdef CONFIG_DT_HAS_ZMK_BEHAVIOR_CAPS_WORD_ENABLED
#include <zmk/events/caps_word_state_changed.h>
#endif

static struct vaporwave_state state;
static K_MUTEX_DEFINE(state_mutex);

static void refresh_keyboard_state(void) {
    state.layer_index = zmk_keymap_highest_layer_active();
    state.layer_count = ZMK_KEYMAP_LAYERS_LEN;
    const char *name = zmk_keymap_layer_name(
        zmk_keymap_layer_index_to_id(state.layer_index));
    if (name && *name) {
        snprintf(state.layer, sizeof(state.layer), "%s", name);
        for (char *c = state.layer; *c; c++) {
            *c = toupper((unsigned char)*c);
        }
    } else {
        snprintf(state.layer, sizeof(state.layer), "%u", state.layer_index);
    }
    zmk_mod_flags_t mods = zmk_hid_get_explicit_mods();
    state.mods = (!!(mods & (MOD_LGUI | MOD_RGUI))) |
        ((!!(mods & (MOD_LALT | MOD_RALT))) << 1) |
        ((!!(mods & (MOD_LCTL | MOD_RCTL))) << 2) |
        ((!!(mods & (MOD_LSFT | MOD_RSFT))) << 3);
    state.wpm = zmk_wpm_get_state();
    struct zmk_endpoint_instance endpoint = zmk_endpoint_get_selected();
    state.usb = (endpoint.transport == ZMK_TRANSPORT_NONE ?
        zmk_endpoint_get_preferred_transport() : endpoint.transport) == ZMK_TRANSPORT_USB;
    state.profile = zmk_ble_active_profile_index();
    state.output_connected = state.usb ? zmk_usb_is_hid_ready() :
                                       zmk_ble_active_profile_is_connected();
}

static void update_display(struct k_work *work) {
    k_mutex_lock(&state_mutex, K_FOREVER);
    struct vaporwave_state snapshot = state;
    k_mutex_unlock(&state_mutex);
    vaporwave_update(&snapshot);
}

static K_WORK_DEFINE(display_work, update_display);

static int status_listener(const zmk_event_t *event) {
    k_mutex_lock(&state_mutex, K_FOREVER);
    const struct zmk_peripheral_battery_state_changed *battery =
        as_zmk_peripheral_battery_state_changed(event);
    if (battery && battery->source < ARRAY_SIZE(state.batteries)) {
        state.batteries[battery->source].level = MIN(battery->state_of_charge, 100);
        state.batteries[battery->source].known = true;
        state.batteries[battery->source].connected = true;
    }
    const struct zmk_split_central_status_changed *connection =
        as_zmk_split_central_status_changed(event);
    if (connection && connection->slot < ARRAY_SIZE(state.batteries)) {
        state.batteries[connection->slot].connected = connection->connected;
        if (!connection->connected) {
            state.batteries[connection->slot].known = false;
        }
    }
#ifdef CONFIG_DT_HAS_ZMK_BEHAVIOR_CAPS_WORD_ENABLED
    const struct zmk_caps_word_state_changed *caps = as_zmk_caps_word_state_changed(event);
    if (caps) {
        state.caps_word = caps->active;
    }
#endif
    refresh_keyboard_state();
    k_mutex_unlock(&state_mutex);

    /* Cache both halves even before display init and when work submissions coalesce. */
    if (zmk_display_is_initialized()) {
        k_work_submit_to_queue(zmk_display_work_q(), &display_work);
    }
    return ZMK_EV_EVENT_BUBBLE;
}

ZMK_LISTENER(vaporwave_status, status_listener);
ZMK_SUBSCRIPTION(vaporwave_status, zmk_peripheral_battery_state_changed);
ZMK_SUBSCRIPTION(vaporwave_status, zmk_split_central_status_changed);
ZMK_SUBSCRIPTION(vaporwave_status, zmk_keycode_state_changed);
ZMK_SUBSCRIPTION(vaporwave_status, zmk_layer_state_changed);
ZMK_SUBSCRIPTION(vaporwave_status, zmk_endpoint_changed);
ZMK_SUBSCRIPTION(vaporwave_status, zmk_ble_active_profile_changed);
ZMK_SUBSCRIPTION(vaporwave_status, zmk_usb_conn_state_changed);
ZMK_SUBSCRIPTION(vaporwave_status, zmk_wpm_state_changed);
#ifdef CONFIG_DT_HAS_ZMK_BEHAVIOR_CAPS_WORD_ENABLED
ZMK_SUBSCRIPTION(vaporwave_status, zmk_caps_word_state_changed);
#endif

lv_obj_t *zmk_display_status_screen(void) {
    lv_obj_t *screen = vaporwave_create();
    k_mutex_lock(&state_mutex, K_FOREVER);
    refresh_keyboard_state();
    k_mutex_unlock(&state_mutex);
    update_display(NULL);
    return screen;
}
