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
#include <math.h>

lv_timer_t *screen_on_delay_timer = nullptr;


/*
// Widgets pour l'UI
static lv_obj_t *touch_label = NULL;  // Label pour les coordonnées du touch
static lv_obj_t *button_label = NULL;  // Label pour le dernier bouton pressé, l'encodeur et le potentiomètre
static lv_obj_t *switch_label = NULL;  // Label pour l'état du switch
static lv_obj_t *backlight_slider = NULL;  // Slider pour le backlight
static lv_obj_t *encoder_slider = NULL;  // Slider pour l'encodeur et potentiomètre
static lv_obj_t *touch_button = NULL;  // Bouton pour tester le touch
static lv_obj_t *touch_counter_label = NULL;  // Label pour le compteur de touch
static uint32_t touch_press_count = 0;     // Compteur de pressions sur le bouton
*/

void screen_on_delay_timer_cb(lv_timer_t *timer)
{
    lv_timer_del(screen_on_delay_timer);
    backlight_set(90);
  // lv_slider_set_value(backlight_slider, 100, LV_ANIM_OFF);
}

/*

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
}*/

#define CIRCLE_DIAMETER 240
#define BORDER_WIDTH 3
#define SELECTION_ARC 45 // 360/8 = 45 degrés pour 1/8
#define NUM_SECTIONS 8   // Nombre de sections (8 pour 1/8 de cercle)
#define INNER_CIRCLE_DIAMETER 130 // Diamètre du cercle central
#define INNER_ARC_WIDTH 54 // Largeur de l'arc intérieur (approximation de (237-130)/2)
#define ICON_ZONE_DIAMETER 40 // Diamètre des zones cliquables (même taille que les boutons précédents)

// Définitions des couleurs
#define BACKGROUND_COLOR 0x000000 // Noir pour le fond
#define BORDER_COLOR 0x404040     // Gris foncé pour la bordure extérieure
#define INNER_COLOR 0x222222      // Gris clair pour le cercle central et l'arc intérieur
#define SELECTOR_COLOR 0x00BFFF   // Bleu clair pour l'arc de sélection
#define ICON_COLOR 0xFFFFFF       // Blanc pour les icônes par défaut

static lv_obj_t *arc; // Arc bleu extérieur
static lv_obj_t *inner_arc; // Arc intérieur gris clair
static lv_obj_t *click_zones[NUM_SECTIONS]; // Tableau pour stocker les zones cliquables
static lv_obj_t *icons[NUM_SECTIONS]; // Tableau pour stocker les références des icônes
static int32_t current_section = 0; // Suivre la section actuelle (0 à 7)

// Tableau des icônes pour chaque zone
static const char *button_icons[NUM_SECTIONS] = {
    LV_SYMBOL_SETTINGS,
    LV_SYMBOL_LIST,
    LV_SYMBOL_HOME,
    LV_SYMBOL_EJECT,
    LV_SYMBOL_PLAY,
    LV_SYMBOL_FILE,
    LV_SYMBOL_BLUETOOTH,
    LV_SYMBOL_STOP
};

// Fonction pour mettre à jour les styles des icônes
static void update_icon_styles(void)
{
    for (int i = 0; i < NUM_SECTIONS; i++) {
        if (i == current_section) {
            // Icône sélectionnée : blanche
            lv_obj_set_style_text_color(icons[i], lv_color_hex(ICON_COLOR), LV_PART_MAIN);
        } else {
            // Icône inactif : blanche
            lv_obj_set_style_text_color(icons[i], lv_color_hex(ICON_COLOR), LV_PART_MAIN);
        }
    }
}

// Callback pour l'encodeur
static void encoder_event_cb(lv_event_t *e)
{
    static int32_t menuId = 0;
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_KEY)
    {
        control_event_t *event = (control_event_t *)lv_event_get_param(e);
        if (event && event->family_id == CONTROL_FAMILY_ENCODER)
        {
            int32_t steps = event->steps;
            menuId += steps;

            // Mettre à jour la section actuelle (0 à 7)
            current_section = (current_section + steps) % NUM_SECTIONS;
            if (current_section < 0) {
                current_section += NUM_SECTIONS;
            }

            // Mettre à jour les angles des deux arcs
            int32_t start_angle = current_section * SELECTION_ARC;
            int32_t end_angle = start_angle + SELECTION_ARC;
            lv_arc_set_angles(arc, start_angle, end_angle);
            lv_arc_set_angles(inner_arc, start_angle, end_angle);

            // Mettre à jour les styles des icônes
            update_icon_styles();

            esp3d_log_d("Encoder: steps=%ld, menuId=%ld, section=%ld", steps, menuId, current_section);
        }
    }
}

// Callback pour les zones cliquables
static void click_zone_event_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    int32_t section = (int32_t)(intptr_t)lv_event_get_user_data(e);

    if (code == LV_EVENT_PRESSED)
    {
        // Icône pressée : bleue (SELECTOR_COLOR)
        lv_obj_set_style_text_color(icons[section], lv_color_hex(SELECTOR_COLOR), LV_PART_MAIN);

        // Mettre à jour la section actuelle
        current_section = section;

        // Mettre à jour les angles des deux arcs
        int32_t start_angle = current_section * SELECTION_ARC;
        int32_t end_angle = start_angle + SELECTION_ARC;
        lv_arc_set_angles(arc, start_angle, end_angle);
        lv_arc_set_angles(inner_arc, start_angle, end_angle);

        esp3d_log_d("Click zone pressed: section=%ld", current_section);
    }
    else if (code == LV_EVENT_RELEASED || code == LV_EVENT_PRESS_LOST)
    {
        // Après relâchement ou perte de pression, revenir à l'état normal
        update_icon_styles();
    }
}

// Fonction pour créer le menu circulaire
void create_circular_menu(lv_obj_t *screen) {
    #pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-enum-enum-conversion"

    lv_obj_t *menu = lv_obj_create(screen);
        // Fond noir
    lv_obj_set_style_bg_color(menu, lv_color_hex(BACKGROUND_COLOR), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(menu, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_size(menu, CIRCLE_DIAMETER, CIRCLE_DIAMETER);
    lv_obj_set_style_border_width(menu, 0, LV_PART_MAIN);
    lv_obj_set_style_border_opa(menu, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_align(menu, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_set_scrollbar_mode(menu, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_style_pad_all(menu, 0, LV_PART_MAIN);
    lv_obj_set_style_border_width(menu, 0, LV_PART_MAIN | LV_STATE_ANY);
    lv_obj_set_style_border_opa(menu, LV_OPA_TRANSP, LV_PART_MAIN | LV_STATE_ANY);
    // Créer le cercle de base (bordure gris foncé)
    lv_obj_t *circle = lv_obj_create(menu);
    lv_obj_set_size(circle, CIRCLE_DIAMETER, CIRCLE_DIAMETER);
    lv_obj_center(circle);
    lv_obj_set_style_radius(circle, LV_RADIUS_CIRCLE, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(circle, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_color(circle, lv_color_hex(BORDER_COLOR), LV_PART_MAIN);
    lv_obj_set_style_border_width(circle, BORDER_WIDTH, LV_PART_MAIN);
    lv_obj_set_style_border_opa(circle, LV_PART_MAIN, LV_OPA_COVER);

    // Créer le cercle central (gris clair, rempli)
    lv_obj_t *inner_circle = lv_obj_create(menu);
    lv_obj_set_size(inner_circle, INNER_CIRCLE_DIAMETER, INNER_CIRCLE_DIAMETER);
    lv_obj_center(inner_circle);
    lv_obj_set_style_radius(inner_circle, LV_RADIUS_CIRCLE, LV_PART_MAIN);
    lv_obj_set_style_bg_color(inner_circle, lv_color_hex(INNER_COLOR), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(inner_circle, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_width(inner_circle, 0, LV_PART_MAIN); // Pas de bordure

    // Créer l'arc intérieur (gris clair) entre le cercle central et l'arc bleu
    inner_arc = lv_arc_create(menu);
    lv_obj_set_size(inner_arc, CIRCLE_DIAMETER - (2 * BORDER_WIDTH), CIRCLE_DIAMETER - (2 * BORDER_WIDTH)); // 237 px de diamètre
    lv_obj_center(inner_arc);
    lv_arc_set_angles(inner_arc, 0, SELECTION_ARC);
    lv_obj_set_style_arc_width(inner_arc, INNER_ARC_WIDTH, LV_PART_INDICATOR); // Largeur pour couvrir l'espace
    lv_obj_set_style_arc_color(inner_arc, lv_color_hex(INNER_COLOR), LV_PART_INDICATOR);
    lv_obj_set_style_arc_opa(inner_arc, LV_OPA_COVER, LV_PART_INDICATOR);
    lv_obj_set_style_arc_rounded(inner_arc, false, LV_PART_INDICATOR); // Extrémités droites (non arrondies)
    lv_obj_set_style_bg_opa(inner_arc, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_arc_set_bg_angles(inner_arc, 0, 0);
    lv_obj_set_style_arc_opa(inner_arc, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_opa(inner_arc, LV_OPA_TRANSP, LV_PART_KNOB);
    lv_obj_set_style_size(inner_arc, 0, 0, LV_PART_KNOB);
    lv_obj_set_style_radius(inner_arc, 0, LV_PART_KNOB);

    // Créer l'arc de sélection (bleu, 45°)
    arc = lv_arc_create(menu);
    lv_obj_set_size(arc, CIRCLE_DIAMETER, CIRCLE_DIAMETER);
    lv_obj_center(arc);
    lv_arc_set_angles(arc, 0, SELECTION_ARC);
    lv_obj_set_style_arc_width(arc, BORDER_WIDTH, LV_PART_INDICATOR);
    lv_obj_set_style_arc_color(arc, lv_color_hex(SELECTOR_COLOR), LV_PART_INDICATOR);
    lv_obj_set_style_arc_opa(arc, LV_OPA_COVER, LV_PART_INDICATOR);
    lv_obj_set_style_bg_opa(arc, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_arc_set_bg_angles(arc, 0, 0);
    lv_obj_set_style_arc_opa(arc, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_opa(arc, LV_OPA_TRANSP, LV_PART_KNOB);
    lv_obj_set_style_size(arc, 0, 0, LV_PART_KNOB);
    lv_obj_set_style_radius(arc, 0, LV_PART_KNOB);

    // Créer les zones cliquables avec icônes pour chaque section
    for (int i = 0; i < NUM_SECTIONS; i++) {
        // Créer une zone cliquable (objet de base)
        lv_obj_t *click_zone = lv_obj_create(menu);
        click_zones[i] = click_zone;
        lv_obj_set_size(click_zone, ICON_ZONE_DIAMETER, ICON_ZONE_DIAMETER);
        lv_obj_set_style_radius(click_zone, LV_RADIUS_CIRCLE, LV_PART_MAIN);

        // Positionner la zone au centre de la section
        float mid_angle_rad = (i * SELECTION_ARC + SELECTION_ARC / 2) * (M_PI / 180.0); // Angle moyen de la section
        int32_t radius = (CIRCLE_DIAMETER - INNER_CIRCLE_DIAMETER) / 4 + INNER_CIRCLE_DIAMETER / 2; // Position à mi-chemin entre les cercles
        int32_t offset_x = (int32_t)(cos(mid_angle_rad) * radius);
        int32_t offset_y = (int32_t)(sin(mid_angle_rad) * radius);
        lv_obj_align(click_zone, LV_ALIGN_CENTER, offset_x, offset_y);

        // Rendre la zone complètement transparente pour tous les états
        lv_obj_set_style_bg_opa(click_zone, LV_OPA_TRANSP, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_border_opa(click_zone, LV_OPA_TRANSP, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_shadow_opa(click_zone, LV_OPA_TRANSP, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_outline_opa(click_zone, LV_OPA_TRANSP, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_border_width(click_zone, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_pad_all(click_zone, 0, LV_PART_MAIN | LV_STATE_DEFAULT);

        // S'assurer qu'aucun style par défaut n'est appliqué pour l'état pressé
        lv_obj_set_style_bg_opa(click_zone, LV_OPA_TRANSP, LV_PART_MAIN | LV_STATE_PRESSED);
        lv_obj_set_style_border_opa(click_zone, LV_OPA_TRANSP, LV_PART_MAIN | LV_STATE_PRESSED);
        lv_obj_set_style_shadow_opa(click_zone, LV_OPA_TRANSP, LV_PART_MAIN | LV_STATE_PRESSED);
        lv_obj_set_style_outline_opa(click_zone, LV_OPA_TRANSP, LV_PART_MAIN | LV_STATE_PRESSED);
        lv_obj_set_style_border_width(click_zone, 0, LV_PART_MAIN | LV_STATE_PRESSED);
        lv_obj_set_style_pad_all(click_zone, 0, LV_PART_MAIN | LV_STATE_PRESSED);
        lv_obj_set_style_bg_grad_color(click_zone, lv_color_hex(BACKGROUND_COLOR), LV_PART_MAIN | LV_STATE_PRESSED);
        lv_obj_set_style_bg_main_stop(click_zone, 0, LV_PART_MAIN | LV_STATE_PRESSED);
        lv_obj_set_style_bg_grad_stop(click_zone, 0, LV_PART_MAIN | LV_STATE_PRESSED);

        // S'assurer que la zone capture les événements
        lv_obj_add_flag(click_zone, LV_OBJ_FLAG_CLICKABLE);

        // Ajouter une étiquette pour l'icône
        lv_obj_t *icon = lv_label_create(click_zone);
        icons[i] = icon; // Stocker la référence de l'icône
        lv_label_set_text(icon, button_icons[i]);
        lv_obj_center(icon);
        lv_obj_set_style_text_color(icon, lv_color_hex(ICON_COLOR), LV_PART_MAIN);
        lv_obj_set_style_bg_opa(icon, LV_OPA_TRANSP, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_border_opa(icon, LV_OPA_TRANSP, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_shadow_opa(icon, LV_OPA_TRANSP, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_outline_opa(icon, LV_PART_MAIN | LV_STATE_DEFAULT, LV_OPA_TRANSP);
        lv_obj_set_style_border_width(icon, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_pad_all(icon, 0, LV_PART_MAIN | LV_STATE_DEFAULT);

        // Associer les événements de pression et de relâchement
        lv_obj_add_event_cb(click_zone, click_zone_event_cb, LV_EVENT_PRESSED, (void *)(intptr_t)i);
        lv_obj_add_event_cb(click_zone, click_zone_event_cb, LV_EVENT_RELEASED, (void *)(intptr_t)i);
        lv_obj_add_event_cb(click_zone, click_zone_event_cb, LV_EVENT_PRESS_LOST, (void *)(intptr_t)i);
    }

    // Mettre à jour les styles des icônes pour refléter la section initiale (current_section = 0)
    update_icon_styles();

    // Associer l'événement de l'encodeur au screen
    lv_obj_add_event_cb(screen, encoder_event_cb, LV_EVENT_KEY, NULL);
    #pragma GCC diagnostic pop
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

    create_circular_menu(screen);


    esp3d_log("LVGL application UI created");
    screen_on_delay_timer = lv_timer_create(screen_on_delay_timer_cb, 50, NULL);
}


/*

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
    */

    /*
    void screen_on_delay_timer_cb(lv_timer_t *timer)
{
    lv_timer_del(screen_on_delay_timer);
    backlight_set(90);
  // lv_slider_set_value(backlight_slider, 100, LV_ANIM_OFF);
}

// Constant definitions
#define CIRCLE_DIAMETER 240
#define BORDER_WIDTH 3
#define SELECTION_ARC 45 // 360/8 = 45 degrees for 1/8
#define NUM_SECTIONS 8   // Number of sections (8 for 1/8 of circle)
#define INNER_CIRCLE_DIAMETER 130 // Diameter of the central circle
#define INNER_ARC_WIDTH 54 // Width of the inner arc (approximation of (237-130)/2)
#define ICON_ZONE_DIAMETER 40 // Diameter of clickable zones
#define BOTTOM_BUTTON_SIZE 50 // Size of bottom buttons
#define BOTTOM_BUTTON_SPACING 20 // Increased spacing between bottom buttons
#define TOP_OFFSET 10 // Offset of circular menu from the top

// Color definitions
#define BACKGROUND_COLOR 0x000000 // Black for background
#define BORDER_COLOR 0x404040     // Dark gray for outer border
#define INNER_COLOR 0x222222      // Light gray for central circle and inner arc
#define SELECTOR_COLOR 0x00BFFF   // Light blue for selection arc
#define ICON_COLOR 0xFFFFFF       // White for default icons

static lv_obj_t *arc; // Outer blue arc
static lv_obj_t *inner_arc; // Inner light gray arc
static lv_obj_t *click_zones[NUM_SECTIONS]; // Array to store clickable zones
static lv_obj_t *icons[NUM_SECTIONS]; // Array to store icon references
static int32_t current_section = 0; // Track the current section (0 to 7)

// Array of icons for each circular menu section
static const char *button_icons[NUM_SECTIONS] = {
    LV_SYMBOL_SETTINGS,
    LV_SYMBOL_LIST,
    LV_SYMBOL_HOME,
    LV_SYMBOL_EJECT,
    LV_SYMBOL_PLAY,
    LV_SYMBOL_FILE,
    LV_SYMBOL_BLUETOOTH,
    LV_SYMBOL_STOP
};

// Array of icons for bottom buttons (replace with your symbols)
static const char *bottom_button_icons[3] = {
    LV_SYMBOL_OK,
    LV_SYMBOL_CLOSE,
    LV_SYMBOL_POWER
};

// Function to update icon styles
static void update_icon_styles(void)
{
    for (int i = 0; i < NUM_SECTIONS; i++) {
        if (i == current_section) {
            // Selected icon: white
            lv_obj_set_style_text_color(icons[i], lv_color_hex(ICON_COLOR), LV_PART_MAIN);
        } else {
            // Inactive icon: white
            lv_obj_set_style_text_color(icons[i], lv_color_hex(ICON_COLOR), LV_PART_MAIN);
        }
    }
}

// Function to get the active section ID
int32_t get_active_section_id(void)
{
    return current_section;
}

// Callback for encoder
static void encoder_event_cb(lv_event_t *e)
{
    static int32_t menuId = 0;
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_KEY)
    {
        control_event_t *event = (control_event_t *)lv_event_get_param(e);
        if (event && event->family_id == CONTROL_FAMILY_ENCODER)
        {
            int32_t steps = event->steps;
            menuId += steps;

            // Update the current section (0 to 7)
            current_section = (current_section + steps) % NUM_SECTIONS;
            if (current_section < 0) {
                current_section += NUM_SECTIONS;
            }

            // Update the angles of both arcs
            int32_t start_angle = current_section * SELECTION_ARC;
            int32_t end_angle = start_angle + SELECTION_ARC;
            lv_arc_set_angles(arc, start_angle, end_angle);
            lv_arc_set_angles(inner_arc, start_angle, end_angle);

            // Update icon styles
            update_icon_styles();

            esp3d_log_d("Encoder: steps=%ld, menuId=%ld, active_section_id=%ld", steps, menuId, current_section);
        }
    }
}

// Callback for clickable zones
static void click_zone_event_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    int32_t section = (int32_t)(intptr_t)lv_event_get_user_data(e);

    if (code == LV_EVENT_PRESSED)
    {
        // Pressed icon: blue (SELECTOR_COLOR)
        lv_obj_set_style_text_color(icons[section], lv_color_hex(SELECTOR_COLOR), LV_PART_MAIN);

        // Update the current section
        current_section = section;

        // Update the angles of both arcs
        int32_t start_angle = current_section * SELECTION_ARC;
        int32_t end_angle = start_angle + SELECTION_ARC;
        lv_arc_set_angles(arc, start_angle, end_angle);
        lv_arc_set_angles(inner_arc, start_angle, end_angle);

        esp3d_log_d("Click zone pressed: active_section_id=%ld", current_section);
    }
    else if (code == LV_EVENT_RELEASED || code == LV_EVENT_PRESS_LOST)
    {
        // After release or press lost, return to normal state
        update_icon_styles();
    }
}

// Callback for bottom buttons
static void bottom_button_event_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    int32_t button_idx = (int32_t)(intptr_t)lv_event_get_user_data(e);

    if (code == LV_EVENT_PRESSED)
    {
        esp3d_log_d("Bottom button pressed: index=%ld", button_idx);
        // Add specific logic for each button here
    }
}

// Function to create the circular menu
void create_circular_menu(lv_obj_t *screen, int32_t initial_section_id) {
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-enum-enum-conversion"
    // Validate initial section ID
    if (initial_section_id < 0 || initial_section_id >= NUM_SECTIONS) {
        initial_section_id = 0; // Default to section 0 if invalid
        esp3d_log_w("Invalid initial_section_id, defaulting to 0");
    }
    current_section = initial_section_id;

    // Black background
    lv_obj_set_style_bg_color(screen, lv_color_hex(BACKGROUND_COLOR), LV_PART_MAIN);

    // Create the base circle (dark gray border)
    lv_obj_t *circle = lv_obj_create(screen);
    lv_obj_set_size(circle, CIRCLE_DIAMETER, CIRCLE_DIAMETER);
    lv_obj_align(circle, LV_ALIGN_TOP_MID, 0, TOP_OFFSET);
    lv_obj_set_style_radius(circle, LV_RADIUS_CIRCLE, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(circle, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_color(circle, lv_color_hex(BORDER_COLOR), LV_PART_MAIN);
    lv_obj_set_style_border_width(circle, BORDER_WIDTH, LV_PART_MAIN);
    lv_obj_set_style_border_opa(circle, LV_OPA_COVER, LV_PART_MAIN);

    // Create the central circle (light gray, filled)
    lv_obj_t *inner_circle = lv_obj_create(screen);
    lv_obj_set_size(inner_circle, INNER_CIRCLE_DIAMETER, INNER_CIRCLE_DIAMETER);
    lv_obj_align(inner_circle, LV_ALIGN_TOP_MID, 0, TOP_OFFSET);
    lv_obj_set_style_radius(inner_circle, LV_RADIUS_CIRCLE, LV_PART_MAIN);
    lv_obj_set_style_bg_color(inner_circle, lv_color_hex(INNER_COLOR), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(inner_circle, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_width(inner_circle, 0, LV_PART_MAIN);

    // Create the inner arc (light gray) between the central circle and blue arc
    inner_arc = lv_arc_create(screen);
    lv_obj_set_size(inner_arc, CIRCLE_DIAMETER - (2 * BORDER_WIDTH), CIRCLE_DIAMETER - (2 * BORDER_WIDTH));
    lv_obj_align(inner_arc, LV_ALIGN_TOP_MID, 0, TOP_OFFSET);
    lv_arc_set_angles(inner_arc, initial_section_id * SELECTION_ARC, initial_section_id * SELECTION_ARC + SELECTION_ARC);
    lv_obj_set_style_arc_width(inner_arc, INNER_ARC_WIDTH, LV_PART_INDICATOR);
    lv_obj_set_style_arc_color(inner_arc, lv_color_hex(INNER_COLOR), LV_PART_INDICATOR);
    lv_obj_set_style_arc_opa(inner_arc, LV_OPA_COVER, LV_PART_INDICATOR);
    lv_obj_set_style_arc_rounded(inner_arc, false, LV_PART_INDICATOR);
    lv_obj_set_style_bg_opa(inner_arc, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_arc_set_bg_angles(inner_arc, 0, 0);
    lv_obj_set_style_arc_opa(inner_arc, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_opa(inner_arc, LV_OPA_TRANSP, LV_PART_KNOB);
    lv_obj_set_style_size(inner_arc, 0, 0, LV_PART_KNOB);
    lv_obj_set_style_radius(inner_arc, 0, LV_PART_KNOB);

    // Create the selection arc (blue, 45°)
    arc = lv_arc_create(screen);
    lv_obj_set_size(arc, CIRCLE_DIAMETER, CIRCLE_DIAMETER);
    lv_obj_align(arc, LV_ALIGN_TOP_MID, 0, TOP_OFFSET);
    lv_arc_set_angles(arc, initial_section_id * SELECTION_ARC, initial_section_id * SELECTION_ARC + SELECTION_ARC);
    lv_obj_set_style_arc_width(arc, BORDER_WIDTH, LV_PART_INDICATOR);
    lv_obj_set_style_arc_color(arc, lv_color_hex(SELECTOR_COLOR), LV_PART_INDICATOR);
    lv_obj_set_style_arc_opa(arc, LV_OPA_COVER, LV_PART_INDICATOR);
    lv_obj_set_style_bg_opa(arc, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_arc_set_bg_angles(arc, 0, 0);
    lv_obj_set_style_arc_opa(arc, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_opa(arc, LV_OPA_TRANSP, LV_PART_KNOB);
    lv_obj_set_style_size(arc, 0, 0, LV_PART_KNOB);
    lv_obj_set_style_radius(arc, 0, LV_PART_KNOB);

    // Create clickable zones with icons for each section
    for (int i = 0; i < NUM_SECTIONS; i++) {
        // Create a clickable zone (base object)
        lv_obj_t *click_zone = lv_obj_create(screen);
        click_zones[i] = click_zone;
        lv_obj_set_size(click_zone, ICON_ZONE_DIAMETER, ICON_ZONE_DIAMETER);
        lv_obj_set_style_radius(click_zone, LV_RADIUS_CIRCLE, LV_PART_MAIN);

        // Position the zone at the center of the section
        float mid_angle_rad = (i * SELECTION_ARC + SELECTION_ARC / 2) * (M_PI / 180.0);
        int32_t radius = (CIRCLE_DIAMETER - INNER_CIRCLE_DIAMETER) / 4 + INNER_CIRCLE_DIAMETER / 2;
        int32_t offset_x = (int32_t)(cos(mid_angle_rad) * radius);
        int32_t offset_y = (int32_t)(sin(mid_angle_rad) * radius);
        lv_obj_align(click_zone, LV_ALIGN_TOP_MID, offset_x, offset_y); // Removed TOP_OFFSET

        // Make the zone completely transparent for all states
        lv_obj_set_style_bg_opa(click_zone, LV_OPA_TRANSP, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_border_opa(click_zone, LV_OPA_TRANSP, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_shadow_opa(click_zone, LV_OPA_TRANSP, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_outline_opa(click_zone, LV_OPA_TRANSP, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_border_width(click_zone, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_pad_all(click_zone, 0, LV_PART_MAIN | LV_STATE_DEFAULT);

        // Ensure no default styles are applied for pressed state
        lv_obj_set_style_bg_opa(click_zone, LV_OPA_TRANSP, LV_PART_MAIN | LV_STATE_PRESSED);
        lv_obj_set_style_border_opa(click_zone, LV_OPA_TRANSP, LV_PART_MAIN | LV_STATE_PRESSED);
        lv_obj_set_style_shadow_opa(click_zone, LV_OPA_TRANSP, LV_PART_MAIN | LV_STATE_PRESSED);
        lv_obj_set_style_outline_opa(click_zone, LV_OPA_TRANSP, LV_PART_MAIN | LV_STATE_PRESSED);
        lv_obj_set_style_border_width(click_zone, 0, LV_PART_MAIN | LV_STATE_PRESSED);
        lv_obj_set_style_pad_all(click_zone, 0, LV_PART_MAIN | LV_STATE_PRESSED);
        lv_obj_set_style_bg_grad_color(click_zone, lv_color_hex(BACKGROUND_COLOR), LV_PART_MAIN | LV_STATE_PRESSED);
        lv_obj_set_style_bg_main_stop(click_zone, 0, LV_PART_MAIN | LV_STATE_PRESSED);
        lv_obj_set_style_bg_grad_stop(click_zone, 0, LV_PART_MAIN | LV_STATE_PRESSED);

        // Ensure the zone captures events
        lv_obj_add_flag(click_zone, LV_OBJ_FLAG_CLICKABLE);

        // Add a label for the icon
        lv_obj_t *icon = lv_label_create(click_zone);
        icons[i] = icon;
        lv_label_set_text(icon, button_icons[i]);
        lv_obj_center(icon);
        lv_obj_set_style_text_color(icon, lv_color_hex(ICON_COLOR), LV_PART_MAIN);
        lv_obj_set_style_bg_opa(icon, LV_OPA_TRANSP, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_border_opa(icon, LV_OPA_TRANSP, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_shadow_opa(icon, LV_OPA_TRANSP, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_outline_opa(icon, LV_OPA_TRANSP, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_border_width(icon, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_pad_all(icon, 0, LV_PART_MAIN | LV_STATE_DEFAULT);

        // Associate press and release events
        lv_obj_add_event_cb(click_zone, click_zone_event_cb, LV_EVENT_PRESSED, (void *)(intptr_t)i);
        lv_obj_add_event_cb(click_zone, click_zone_event_cb, LV_EVENT_RELEASED, (void *)(intptr_t)i);
        lv_obj_add_event_cb(click_zone, click_zone_event_cb, LV_EVENT_PRESS_LOST, (void *)(intptr_t)i);
    }

    // Create the three bottom buttons
    for (int i = 0; i < 3; i++) {
        // Create a button
        lv_obj_t *button = lv_btn_create(screen);
        lv_obj_set_size(button, BOTTOM_BUTTON_SIZE, BOTTOM_BUTTON_SIZE);
        lv_obj_set_style_radius(button, LV_RADIUS_CIRCLE, LV_PART_MAIN);
        lv_obj_set_style_bg_color(button, lv_color_hex(BORDER_COLOR), LV_PART_MAIN);
        lv_obj_set_style_bg_opa(button, LV_OPA_COVER, LV_PART_MAIN);
        lv_obj_set_style_border_width(button, 0, LV_PART_MAIN);
        lv_obj_set_style_shadow_opa(button, LV_OPA_TRANSP, LV_PART_MAIN);

        // Position buttons horizontally at the bottom
        int32_t offset_x = (i - 1) * (BOTTOM_BUTTON_SIZE + BOTTOM_BUTTON_SPACING);
        lv_obj_align(button, LV_ALIGN_BOTTOM_MID, offset_x, -10);

        // Add a label for the icon
        lv_obj_t *icon = lv_label_create(button);
        lv_label_set_text(icon, bottom_button_icons[i]);
        lv_obj_center(icon);
        lv_obj_set_style_text_color(icon, lv_color_hex(ICON_COLOR), LV_PART_MAIN);
        lv_obj_set_style_bg_opa(icon, LV_OPA_TRANSP, LV_PART_MAIN);
        lv_obj_set_style_border_opa(icon, LV_OPA_TRANSP, LV_PART_MAIN);
        lv_obj_set_style_shadow_opa(icon, LV_OPA_TRANSP, LV_PART_MAIN);
        lv_obj_set_style_outline_opa(icon, LV_OPA_TRANSP, LV_PART_MAIN);
        lv_obj_set_style_border_width(icon, 0, LV_PART_MAIN);
        lv_obj_set_style_pad_all(icon, 0, LV_PART_MAIN);

        // Associate the press event
        lv_obj_add_event_cb(button, bottom_button_event_cb, LV_EVENT_PRESSED, (void *)(intptr_t)i);
    }

    // Update icon styles to reflect the initial section
    update_icon_styles();

    // Associate the encoder event to the screen
    lv_obj_add_event_cb(screen, encoder_event_cb, LV_EVENT_KEY, NULL);
#pragma GCC diagnostic pop
}

// Create the user interface
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

    // Set black background
    lv_obj_set_style_bg_color(screen, lv_color_black(), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, LV_PART_MAIN);

    // Create circular menu with initial section ID (e.g., 2)
    create_circular_menu(screen, 2); // Change 2 to your desired initial section ID (0 to 7)

    esp3d_log("LVGL application UI created");
    screen_on_delay_timer = lv_timer_create(screen_on_delay_timer_cb, 50, NULL);
}
    */