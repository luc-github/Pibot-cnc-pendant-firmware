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

void screen_on_delay_timer_cb(lv_timer_t *timer)
{
    lv_timer_del(screen_on_delay_timer);
    backlight_set(90);
}

#define CIRCLE_DIAMETER 240
#define BORDER_WIDTH 3
#define SELECTION_ARC 45 // 360/8 = 45 degrés pour 1/8
#define NUM_SECTIONS 8   // Nombre de sections (8 pour 1/8 de cercle)
#define INNER_CIRCLE_DIAMETER 130 // Diamètre du cercle central
#define INNER_ARC_WIDTH 54 // Largeur de l'arc intérieur (approximation de (237-130)/2)
#define ICON_ZONE_DIAMETER 40 // Diamètre des zones cliquables
#define BOTTOM_BUTTON_SIZE 50 // Size of bottom buttons

// Définitions des couleurs
#define BACKGROUND_COLOR 0x000000 // Noir pour le fond
#define BORDER_COLOR 0x404040     // Gris foncé pour la bordure extérieure
#define INNER_COLOR 0x222222      // Gris clair pour le cercle central
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

// Array of icons for bottom buttons
static const char *bottom_button_icons[3] = {
    LV_SYMBOL_OK,
    LV_SYMBOL_CLOSE,
    LV_SYMBOL_REFRESH
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

// Function to get the active section ID
int32_t get_active_section_id(void)
{
    return current_section;
}

// Helper function to simulate a click on the active section
static void simulate_click_on_active_section(void)
{
    // Icône pressée : bleue (SELECTOR_COLOR)
    lv_obj_set_style_text_color(icons[current_section], lv_color_hex(SELECTOR_COLOR), LV_PART_MAIN);

    // Log the simulated click
    esp3d_log_d("Click zone pressed: active_section_id=%ld", current_section);
}

// Physical Buttons callback 
static void button_event_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_PRESSED)
    {
        control_event_t *event = (control_event_t *)lv_event_get_param(e);
        if (event && event->family_id == CONTROL_FAMILY_BUTTONS)
        {
            uint32_t btn_id = event->btn_id;
            esp3d_log_d("Button key: %ld, family_id: %d", btn_id, event->family_id);
            switch (btn_id)
            {
                case 0:
                    esp3d_log_d("Button 1 pressed");
                    buzzer_set_loud(false);
                    buzzer_bip(1000, 500);
                    simulate_click_on_active_section();
                    update_icon_styles(); // Reset icon styles immediately
                    break;
                case 1:
                    esp3d_log_d("Button 2 pressed");
                    break;
                case 2:
                    esp3d_log_d("Button 3 pressed");
                    break;
                default:
                    esp3d_log_e("Button unknown pressed");
                    break;
            }
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

// Callback for bottom buttons
static void bottom_button_event_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    int32_t button_idx = (int32_t)(intptr_t)lv_event_get_user_data(e);

    if (code == LV_EVENT_PRESSED)
    {
        esp3d_log_d("Bottom button pressed: index=%ld", button_idx);
        if (button_idx == 0) {
            simulate_click_on_active_section();
        }
    }
    else if (code == LV_EVENT_RELEASED || code == LV_EVENT_PRESS_LOST)
    {
        if (button_idx == 0) {
            // Après relâchement ou perte de pression, revenir à l'état normal
            update_icon_styles();
            buzzer_set_loud(false);
            buzzer_bip(1000, 500);
        }
    }
}

// Fonction pour créer le menu circulaire
void create_circular_menu(lv_obj_t *screen, int32_t initial_section_id, int32_t bottom_button_spacing) {
    #pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-enum-enum-conversion"
    // Validate initial section ID
    if (initial_section_id < 0 || initial_section_id >= NUM_SECTIONS) {
        initial_section_id = 0; // Default to section 0 if invalid
        esp3d_log_w("Invalid initial_section_id, defaulting to 0");
    }
    current_section = initial_section_id;

    // Create container for the circular menu
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
    lv_obj_set_style_border_opa(circle, LV_OPA_COVER, LV_PART_MAIN);

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
    lv_arc_set_angles(inner_arc, initial_section_id * SELECTION_ARC, initial_section_id * SELECTION_ARC + SELECTION_ARC);
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

    // Create the three bottom buttons
    for (int i = 0; i < 3; i++) {
        // Create a button
        lv_obj_t *button = lv_btn_create(screen);
        lv_obj_set_size(button, BOTTOM_BUTTON_SIZE, BOTTOM_BUTTON_SIZE);
        lv_obj_set_style_radius(button, LV_RADIUS_CIRCLE, LV_PART_MAIN);
        // Default state: gray background, gray border
        lv_obj_set_style_bg_color(button, lv_color_hex(BORDER_COLOR), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_opa(button, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_border_color(button, lv_color_hex(BORDER_COLOR), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_border_width(button, BORDER_WIDTH, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_border_opa(button, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_shadow_opa(button, LV_OPA_TRANSP, LV_PART_MAIN | LV_STATE_DEFAULT);
        // Pressed state: gray background, blue border
        lv_obj_set_style_bg_color(button, lv_color_hex(BORDER_COLOR), LV_PART_MAIN | LV_STATE_PRESSED);
        lv_obj_set_style_bg_opa(button, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_PRESSED);
        lv_obj_set_style_border_color(button, lv_color_hex(SELECTOR_COLOR), LV_PART_MAIN | LV_STATE_PRESSED);
        lv_obj_set_style_border_width(button, BORDER_WIDTH, LV_PART_MAIN | LV_STATE_PRESSED);
        lv_obj_set_style_border_opa(button, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_PRESSED);
        lv_obj_set_style_shadow_opa(button, LV_OPA_TRANSP, LV_PART_MAIN | LV_STATE_PRESSED);

        // Position buttons horizontally at the bottom
        int32_t offset_x = (i - 1) * (BOTTOM_BUTTON_SIZE + bottom_button_spacing);
        lv_obj_align(button, LV_ALIGN_BOTTOM_MID, offset_x, -10);

        // Add a label for the icon
        lv_obj_t *icon = lv_label_create(button);
        lv_label_set_text(icon, bottom_button_icons[i]);
        lv_obj_center(icon);
        // Default state: white icon
        lv_obj_set_style_text_color(icon, lv_color_hex(ICON_COLOR), LV_PART_MAIN | LV_STATE_DEFAULT);
        // Pressed state: blue icon
        lv_obj_set_style_text_color(icon, lv_color_hex(SELECTOR_COLOR), LV_PART_MAIN | LV_STATE_PRESSED);
        lv_obj_set_style_bg_opa(icon, LV_OPA_TRANSP, LV_PART_MAIN);
        lv_obj_set_style_border_opa(icon, LV_OPA_TRANSP, LV_PART_MAIN);
        lv_obj_set_style_shadow_opa(icon, LV_OPA_TRANSP, LV_PART_MAIN);
        lv_obj_set_style_outline_opa(icon, LV_OPA_TRANSP, LV_PART_MAIN);
        lv_obj_set_style_border_width(icon, 0, LV_PART_MAIN);
        lv_obj_set_style_pad_all(icon, 0, LV_PART_MAIN);

        // Associate the press and release events
        lv_obj_add_event_cb(button, bottom_button_event_cb, LV_EVENT_PRESSED, (void *)(intptr_t)i);
        lv_obj_add_event_cb(button, bottom_button_event_cb, LV_EVENT_RELEASED, (void *)(intptr_t)i);
        lv_obj_add_event_cb(button, bottom_button_event_cb, LV_EVENT_PRESS_LOST, (void *)(intptr_t)i);
    }

    // Mettre à jour les styles des icônes pour refléter la section initiale (current_section = 0)
    update_icon_styles();

    // Associer l'événement de l'encodeur au screen
    lv_obj_add_event_cb(screen, encoder_event_cb, LV_EVENT_KEY, NULL);

    // Associate the physical button event to the screen
    lv_obj_add_event_cb(screen, button_event_cb, LV_EVENT_PRESSED, NULL);
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

    // Create circular menu with initial section ID and bottom button spacing
    create_circular_menu(screen, 0, 25); // Initial section ID = 0, button spacing = 25 pixels

    esp3d_log("LVGL application UI created");
    screen_on_delay_timer = lv_timer_create(screen_on_delay_timer_cb, 50, NULL);
}