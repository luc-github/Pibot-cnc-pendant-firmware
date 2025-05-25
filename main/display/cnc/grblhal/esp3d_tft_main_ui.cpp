/*
  esp3d_tft_main_ui.cpp
  Copyright (c) 2022 Luc Lebosse. All rights reserved.
  Licensed under GNU Lesser General Public License v2.1 or later.
*/

#include "esp3d_log.h"
#include "esp3d_version.h"
#include "board_config.h"
#include "board_init.h"
#include "lvgl.h"
#include "disp_backlight.h"
#include "phy_switch.h"

lv_timer_t *screen_on_delay_timer = nullptr;

void screen_on_delay_timer_cb(lv_timer_t *timer) {
    lv_timer_del(screen_on_delay_timer);
    backlight_set(100);
}

// Widgets pour l'UI
static lv_obj_t *touch_label = NULL;  // Label pour les coordonnées du touch
static lv_obj_t *button_label = NULL; // Label pour le dernier bouton pressé
static lv_obj_t *switch_label = NULL; // Label pour l'état actuel du switch

// Callback de débogage global pour intercepter tous les événements
static void debug_event_cb(lv_event_t *e)
{
    esp3d_log_d("Event: code=%d, target=%p", lv_event_get_code(e), lv_event_get_target(e));
}

// Callback pour les événements de touch
static void touch_event_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *label = touch_label;
    if (code == LV_EVENT_PRESSED || code == LV_EVENT_PRESSING) {
        lv_indev_t *indev = lv_event_get_indev(e);
        if (indev && lv_indev_get_type(indev) == LV_INDEV_TYPE_POINTER) {
            lv_point_t point;
            lv_indev_get_point(indev, &point);
            lv_label_set_text_fmt(label, "Touch: %ld,%ld", (long)point.x, (long)point.y);
            esp3d_log_d("Touch event: x=%ld, y=%ld", (long)point.x, (long)point.y);
        }
    } else if (code == LV_EVENT_RELEASED) {
        lv_label_set_text(label, "Touch: Aucun");
        esp3d_log_d("Touch released");
    }
}

// Callback pour les boutons physiques
static void button_event_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_indev_t *indev = lv_event_get_indev(e);
    lv_obj_t *label = button_label;
    if (code == LV_EVENT_PRESSED && lv_indev_get_type(indev) == LV_INDEV_TYPE_BUTTON) {
        lv_point_t point;
        lv_indev_get_point(indev, &point);
        if (point.x == 0) { // Vérifier que x=0 pour les boutons physiques
            uint32_t btn_id = point.y;
            esp3d_log_d("Button key: %ld, point: %ld,%ld", btn_id, point.x, point.y);
            switch (btn_id) {
                case 0: lv_label_set_text(label, "Dernier bouton: 1"); break;
                case 1: lv_label_set_text(label, "Dernier bouton: 2"); break;
                case 2: lv_label_set_text(label, "Dernier bouton: 3"); break;
                default: lv_label_set_text(label, "Dernier bouton: Inconnu"); break;
            }
        }
    }
}

// Callback pour le switch
static void switch_event_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_indev_t *indev = lv_event_get_indev(e);
    lv_obj_t *label = switch_label;
    if (code == LV_EVENT_PRESSED && lv_indev_get_type(indev) == LV_INDEV_TYPE_BUTTON) {
        lv_point_t point;
        lv_indev_get_point(indev, &point);
        if (point.x == 100) { // Vérifier que x=1 pour le switch
            uint32_t btn_id = point.y-50;
            esp3d_log_d("Switch button key: %ld, point: %ld,%ld", btn_id, point.x, point.y);
            switch (btn_id) {
                case 0: lv_label_set_text(label, "Axe: X"); break;
                case 1: lv_label_set_text(label, "Axe: Y"); break;
                case 2: lv_label_set_text(label, "Axe: Z"); break;
                case 3: lv_label_set_text(label, "Axe: C"); break;
                default: lv_label_set_text(label, "Axe: Inconnu"); break;
            }
        }
    }
}

// Créer l'interface utilisateur
void create_application(void) {
    esp3d_log("Creating LVGL application UI");

    // Get the LVGL display
    lv_display_t *display = get_lvgl_display();
    if (!display) {
        esp3d_log_e("LVGL display not initialized");
        return;
    }

    // Get the active screen
    lv_obj_t *screen = lv_display_get_screen_active(display);

    // Définir le fond noir
    lv_obj_set_style_bg_color(screen, lv_color_black(), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, LV_PART_MAIN);

    // Créer un conteneur principal pour organiser les widgets
    lv_obj_t *main_container = lv_obj_create(screen);
    lv_obj_set_size(main_container, 220, 300);
    lv_obj_align(main_container, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_flex_flow(main_container, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_all(main_container, 10, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(main_container, LV_OPA_TRANSP, LV_PART_MAIN);
    esp3d_log_d("Main container created: %p", main_container);

    // Créer un label pour les coordonnées du touch
    touch_label = lv_label_create(main_container);
    lv_label_set_text(touch_label, "Touch: Aucun");
    lv_obj_align(touch_label, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_set_style_text_color(touch_label, lv_color_white(), LV_PART_MAIN);
    esp3d_log_d("Touch label created: %p", touch_label);

    // Créer un label pour le dernier bouton pressé
    button_label = lv_label_create(main_container);
    lv_label_set_text(button_label, "Dernier bouton: Aucun");
    lv_obj_align(button_label, LV_ALIGN_TOP_MID, 0, 20);
    lv_obj_set_style_text_color(button_label, lv_color_white(), LV_PART_MAIN);
    esp3d_log_d("Button label created: %p", button_label);

    // Créer un label pour l'état du switch
    switch_label = lv_label_create(main_container);
    lv_label_set_text(switch_label, "Axe: ---");
    lv_obj_align(switch_label, LV_ALIGN_TOP_MID, 0, 40);
    lv_obj_set_style_text_color(switch_label, lv_color_white(), LV_PART_MAIN);
    esp3d_log_d("Switch label created: %p", switch_label);

    // Initialiser l'UI avec l'état actuel du switch
    uint32_t initial_key_code;
    if (phy_switch_get_state(&initial_key_code) == ESP_OK) {
        switch (initial_key_code) {
            case 0: lv_label_set_text(switch_label, "Axe: X"); break;
            case 1: lv_label_set_text(switch_label, "Axe: Y"); break;
            case 2: lv_label_set_text(switch_label, "Axe: Z"); break;
            case 3: lv_label_set_text(switch_label, "Axe: C"); break;
            default: lv_label_set_text(switch_label, "Axe: Inconnu"); break;
        }
        esp3d_log_d("Switch initialized with key code: %ld", initial_key_code);
    } else {
        esp3d_log_e("Failed to get initial switch state");
    }

    // Attacher les gestionnaires d'événements au main_container
    lv_obj_add_event_cb(main_container, touch_event_cb, LV_EVENT_PRESSED, NULL);
    lv_obj_add_event_cb(main_container, touch_event_cb, LV_EVENT_PRESSING, NULL);
    lv_obj_add_event_cb(main_container, touch_event_cb, LV_EVENT_RELEASED, NULL);
    lv_obj_add_event_cb(screen, button_event_cb, LV_EVENT_PRESSED, NULL);
    lv_obj_add_event_cb(main_container, switch_event_cb, LV_EVENT_PRESSED, NULL);
    //lv_obj_add_event_cb(main_container, debug_event_cb, LV_EVENT_ALL, NULL);

    esp3d_log("LVGL application UI created");
    screen_on_delay_timer = lv_timer_create(screen_on_delay_timer_cb, 50, NULL);
}