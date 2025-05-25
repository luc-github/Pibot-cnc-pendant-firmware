/*
  esp3d_tft_main_ui.cpp
  Copyright (c) 2022 Luc Lebosse. All rights reserved.
  Licensed under GNU Lesser General Public License v2.1 or later.
*/

#include "esp3d_log.h"
#include "esp3d_version.h"
#include "board_config.h"
#include "board_init.h"
#include "control_event.h"
#include "lvgl.h"
#include "disp_backlight.h"
#include "phy_switch.h"

lv_timer_t *screen_on_delay_timer = nullptr;

void screen_on_delay_timer_cb(lv_timer_t *timer) {
    lv_timer_del(screen_on_delay_timer);
    backlight_set(100);
}

// Widgets pour l'UI
static lv_obj_t *touch_label = NULL;      // Label pour les coordonnées du touch
static lv_obj_t *button_label = NULL;     // Label pour le dernier bouton pressé et l'encodeur
static lv_obj_t *switch_label = NULL;     // Label pour l'état actuel du switch
static lv_obj_t *encoder_slider = NULL;   // Slider pour l'encodeur
static lv_group_t *encoder_group = NULL;  // Groupe pour l'encodeur

// Callback pour les boutons physiques
static void button_event_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *label = button_label;
    if (code == LV_EVENT_PRESSED) {
        control_event_t *event = (control_event_t *)lv_event_get_param(e);
        if (event && event->family_id == CONTROL_FAMILY_BUTTONS) { // Family ID pour boutons
            uint32_t btn_id = event->btn_id;
            esp3d_log_d("Button key: %ld, family_id: %d", btn_id, event->family_id);
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
    lv_obj_t *label = switch_label;
    if (code == LV_EVENT_PRESSED) {
        control_event_t *event = (control_event_t *)lv_event_get_param(e);
        if (event && event->family_id == CONTROL_FAMILY_SWITCH) { // Family ID pour switch
            uint32_t btn_id = event->btn_id;
            esp3d_log_d("Switch button key: %ld, family_id: %d", btn_id, event->family_id);
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
    if (!screen) {
        esp3d_log_e("Active screen is NULL");
        return;
    }

    // Définir le fond noir
    lv_obj_set_style_bg_color(screen, lv_color_black(), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, LV_PART_MAIN);

    // Créer un conteneur principal pour organiser les widgets
    lv_obj_t *main_container = lv_obj_create(screen);
    lv_obj_set_size(main_container, 240, 320);
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

    // Créer un label pour le dernier bouton pressé et l'encodeur
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

    // Créer un slider pour l'encodeur
    encoder_slider = lv_slider_create(main_container);
    lv_obj_set_size(encoder_slider, 200, 20);
    lv_obj_align(encoder_slider, LV_ALIGN_TOP_MID, 0, 60);
    lv_slider_set_range(encoder_slider, 0, 100);
    lv_slider_set_value(encoder_slider, 50, LV_ANIM_OFF);
    esp3d_log_d("Encoder slider created: %p", encoder_slider);

    // Créer le groupe pour l'encodeur (kept for future re-enabling)
    encoder_group = lv_group_create();
    esp3d_log_d("Encoder group created: %p", encoder_group);

    // Associer l'indev encodeur au groupe (kept for future re-enabling)
    lv_indev_t *encoder_indev = get_encoder_indev();
    if (encoder_indev) {
        lv_indev_set_group(encoder_indev, encoder_group);
        esp3d_log_d("Encoder indev %p assigned to group %p", encoder_indev, encoder_group);
    } else {
        esp3d_log_e("Encoder indev is NULL");
    }

    // Ajouter le slider à encoder_group (kept for future re-enabling)
    lv_group_add_obj(encoder_group, encoder_slider);
    esp3d_log_d("Slider added to encoder group");

    // Activer le mode édition pour le slider (kept for future re-enabling)
    lv_group_set_editing(encoder_group, true);

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

    // Attacher les gestionnaires d'événements à l'écran actif
    lv_obj_add_event_cb(screen, button_event_cb, LV_EVENT_PRESSED, NULL);
    lv_obj_add_event_cb(screen, switch_event_cb, LV_EVENT_PRESSED, NULL);

    esp3d_log("LVGL application UI created");
    screen_on_delay_timer = lv_timer_create(screen_on_delay_timer_cb, 50, NULL);
}