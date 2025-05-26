/*
  esp3d_tft_main_ui.cpp
  Copyright (c) 2022 Luc Lebosse. All rights reserved.
  Licensed under GNU Lesser General Public License v2.1 or later.
*/

#include "board_config.h"
#include "board_init.h"
#include "control_event.h"
#include "disp_backlight.h"
#include "esp3d_log.h"
#include "esp3d_version.h"
#include "esp3d_hal.h"
#include "lvgl.h"
#include "phy_potentiometer.h"
#include "phy_switch.h"
#include "buzzer.h"

lv_timer_t *screen_on_delay_timer = nullptr;



// Widgets pour l'UI
static lv_obj_t *touch_label = NULL;  // Label pour les coordonnées du touch
static lv_obj_t *button_label = NULL;  // Label pour le dernier bouton pressé, l'encodeur et le potentiomètre
static lv_obj_t *switch_label = NULL;  // Label pour l'état du switch
static lv_obj_t *backlight_slider = NULL;  // Slider pour le backlight
static lv_obj_t *encoder_slider = NULL;  // Slider pour l'encodeur et potentiomètre
static lv_obj_t *touch_button = NULL;  // Bouton pour tester le touch
static lv_obj_t *touch_counter_label = NULL;  // Label pour le compteur de touch
static uint32_t touch_press_count = 0;     // Compteur de pressions sur le bouton


void screen_on_delay_timer_cb(lv_timer_t *timer)
{
    lv_timer_del(screen_on_delay_timer);
    backlight_set(100);
   lv_slider_set_value(backlight_slider, 100, LV_ANIM_OFF);
}

// Callback pour les boutons physiques
static void button_event_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *label = button_label;
    if (code == LV_EVENT_PRESSED)
    {
        control_event_t *event = (control_event_t *)lv_event_get_param(e);
        if (event && event->family_id == CONTROL_FAMILY_BUTTONS)
        {
            uint32_t btn_id = event->btn_id;
            esp3d_log_d("Button key: %ld, family_id: %d", btn_id, event->family_id);
            buzzer_tone_t tones[] = {
                {1000, 500},
                {523, 500},
                {1000, 500}
            };
            const int tone_count = sizeof(tones) / sizeof(tones[0]);

            switch (btn_id)
            {
                case 0:
                    lv_label_set_text(label, "Dernier bouton: 1");
                    buzzer_set_loud(true);
                    buzzer_bip(1000, 500);
                    break;
                case 1:
                    lv_label_set_text(label, "Dernier bouton: 2");
                    buzzer_set_loud(false);
                    buzzer_play(tones, tone_count);
                    break;
                case 2:
                    lv_label_set_text(label, "Dernier bouton: 3");
                    buzzer_set_loud(true);
                    buzzer_play(tones, tone_count);
                    break;
                default:
                    lv_label_set_text(label, "Dernier bouton: Inconnu");
                    break;
            }
        }
    }
}

// Callback pour le switch
static void switch_event_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *label = switch_label;
    if (code == LV_EVENT_PRESSED)
    {
        control_event_t *event = (control_event_t *)lv_event_get_param(e);
        if (event && event->family_id == CONTROL_FAMILY_SWITCH)
        {
            uint32_t btn_id = event->btn_id;
            esp3d_log_d("Switch button key: %ld, family_id: %d", btn_id, event->family_id);
            switch (btn_id)
            {
                case 0:
                    lv_label_set_text(label, "Axe: X");
                    break;
                case 1:
                    lv_label_set_text(label, "Axe: Y");
                    break;
                case 2:
                    lv_label_set_text(label, "Axe: Z");
                    break;
                case 3:
                    lv_label_set_text(label, "Axe: C");
                    break;
                default:
                    lv_label_set_text(label, "Axe: Inconnu");
                    break;
            }
        }
    }
}

// Callback pour l'encodeur
static void encoder_event_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *label = button_label;
    if (code == LV_EVENT_KEY)
    {
        control_event_t *event = (control_event_t *)lv_event_get_param(e);
        if (event && event->family_id == CONTROL_FAMILY_ENCODER)
        {
            int32_t steps = event->steps;
            lv_obj_t *slider = encoder_slider;
            int32_t value = lv_slider_get_value(slider);
            value += steps * 5;
            if (value < 0)
                value = 0;
            if (value > 100)
                value = 100;
            lv_slider_set_value(slider, value, LV_ANIM_OFF);
            lv_label_set_text_fmt(label, "Encoder: %ld", value);
            esp3d_log_d("Encoder: steps=%ld, value=%ld", steps, value);
        }
    }
}

// Callback pour le potentiomètre
static void potentiometer_event_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *label = button_label;
    if (code == LV_EVENT_VALUE_CHANGED)
    {
        control_event_t *event = (control_event_t *)lv_event_get_param(e);
        if (event && event->family_id == CONTROL_FAMILY_POTENTIOMETER)
        {
            int32_t value = event->steps;
            lv_obj_t *slider = encoder_slider;
            lv_slider_set_value(slider, value, LV_ANIM_OFF);
            lv_label_set_text_fmt(label, "Potentiometer: %ld", value);
            esp3d_log_d("Potentiometer: value=%ld", value);
        }
    }
}

// Callback pour le touch
static void touch_event_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *label = touch_counter_label;
    if (code == LV_EVENT_PRESSED)
    {
        touch_press_count++;
        lv_label_set_text_fmt(label, "Touch presses: %ld", touch_press_count);
        esp3d_log_d("Touch presses: %ld", touch_press_count);
    }
}

// Callback pour le slider de backlight
static void backlight_slider_event_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_VALUE_CHANGED) {
        lv_obj_t *slider = (lv_obj_t *)lv_event_get_target(e);
        int32_t value = lv_slider_get_value(slider);
        esp_err_t ret = backlight_set(value);
        if (ret != ESP_OK) {
            esp3d_log_e("Failed to set backlight to %ld%%", value);
        } else {
            esp3d_log_d("Backlight set to %ld%%", value);
        }
    }
}

// Créer l'interface utilisateur
void create_application(void)
{
    esp3d_log("Creating LVGL application UI");

    // Get the LVGL display
    lv_display_t *display = get_lvgl_display();
    if (!display)
    {
        esp3d_log_e("LVGL display not initialized");
        return;
    }

    // Get the active screen
    lv_obj_t *screen = lv_display_get_screen_active(display);
    if (!screen)
    {
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

    // Créer un label pour le dernier bouton pressé, l'encodeur et le potentiomètre
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

    // Créer un slider pour l'encodeur et le potentiomètre
    encoder_slider = lv_slider_create(main_container);
    lv_obj_set_size(encoder_slider, 200, 20);
    lv_obj_align(encoder_slider, LV_ALIGN_TOP_MID, 0, 60);
    lv_slider_set_range(encoder_slider, 0, 100);
    lv_slider_set_value(encoder_slider, 50, LV_ANIM_OFF);
    esp3d_log_d("Encoder slider created: %p", encoder_slider);

    // Créer un slider pour le backlight
    backlight_slider = lv_slider_create(main_container);
    lv_obj_set_size(backlight_slider, 200, 20);
    lv_obj_align(backlight_slider, LV_ALIGN_TOP_MID, 0, 90);
    lv_slider_set_range(backlight_slider, 0, 100);
    int initial_brightness = backlight_get_current();
    if (initial_brightness >= 0) {
        lv_slider_set_value(backlight_slider, initial_brightness, LV_ANIM_OFF);
        esp3d_log_d("Backlight slider initialized to %d%%", initial_brightness);
    } else {
        lv_slider_set_value(backlight_slider, 100, LV_ANIM_OFF);
        esp3d_log_w("Backlight not initialized, defaulting to 100%%");
    }
    lv_obj_add_event_cb(backlight_slider, backlight_slider_event_cb, LV_EVENT_VALUE_CHANGED, NULL);
    esp3d_log_d("Backlight slider created: %p", backlight_slider);

    // Créer un bouton pour tester le touch
    touch_button = lv_btn_create(main_container);
    lv_obj_set_size(touch_button, 100, 40);
    lv_obj_align(touch_button, LV_ALIGN_TOP_MID, 0, 120);
    lv_obj_t *btn_label = lv_label_create(touch_button);
    lv_label_set_text(btn_label, "Touch Me");
    lv_obj_center(btn_label);
    esp3d_log_d("Touch button created: %p", touch_button);

    // Créer un label pour le compteur de touch
    touch_counter_label = lv_label_create(main_container);
    lv_label_set_text(touch_counter_label, "Touch presses: 0");
    lv_obj_align(touch_counter_label, LV_ALIGN_TOP_MID, 0, 170);
    lv_obj_set_style_text_color(touch_counter_label, lv_color_white(), LV_PART_MAIN);
    esp3d_log_d("Touch counter label created: %p", touch_counter_label);



    // Initialiser l'UI avec l'état actuel du switch
    uint32_t initial_key_code;
    if (phy_switch_get_state(&initial_key_code) == ESP_OK)
    {
        switch (initial_key_code)
        {
            case 0:
                lv_label_set_text(switch_label, "Axe: X");
                break;
            case 1:
                lv_label_set_text(switch_label, "Axe: Y");
                break;
            case 2:
                lv_label_set_text(switch_label, "Axe: Z");
                break;
            case 3:
                lv_label_set_text(switch_label, "Axe: C");
                break;
            default:
                lv_label_set_text(switch_label, "Axe: Inconnu");
                break;
        }
        esp3d_log_d("Switch initialized with key code: %ld", initial_key_code);
    }
    else
    {
        esp3d_log_e("Failed to get initial switch state");
    }

    // Initialiser l'UI avec l'état actuel du potentiomètre
    uint32_t initial_adc_value;
    if (phy_potentiometer_read(&initial_adc_value) == ESP_OK)
    {
        int32_t initial_value = (initial_adc_value * 100) / 4095;
        lv_slider_set_value(encoder_slider, initial_value, LV_ANIM_OFF);
        lv_label_set_text_fmt(button_label, "Potentiometer: %ld", initial_value);
        esp3d_log_d("Potentiometer initialized with value: %ld", initial_value);
    }
    else
    {
        esp3d_log_e("Failed to get initial potentiometer state");
    }

    // Attacher les gestionnaires d'événements à l'écran actif
    lv_obj_add_event_cb(screen, button_event_cb, LV_EVENT_PRESSED, NULL);
    lv_obj_add_event_cb(screen, switch_event_cb, LV_EVENT_PRESSED, NULL);
    lv_obj_add_event_cb(screen, encoder_event_cb, LV_EVENT_KEY, NULL);
    lv_obj_add_event_cb(screen, potentiometer_event_cb, LV_EVENT_VALUE_CHANGED, NULL);
    lv_obj_add_event_cb(touch_button, touch_event_cb, LV_EVENT_PRESSED, NULL);

    esp3d_log("LVGL application UI created");
    screen_on_delay_timer = lv_timer_create(screen_on_delay_timer_cb, 50, NULL);
}