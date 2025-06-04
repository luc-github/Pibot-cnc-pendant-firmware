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
#include <esp_timer.h>
#include <math.h>
#include <stdlib.h>

lv_timer_t *screen_on_delay_timer = nullptr;

void screen_on_delay_timer_cb(lv_timer_t *timer)
{
    lv_timer_del(screen_on_delay_timer);
    backlight_set(90);
}

#define CIRCLE_DIAMETER 240
#define BORDER_WIDTH 3
#define MAX_SECTIONS 14   // Maximum number of sections
#define INNER_CIRCLE_DIAMETER 130 // Diamètre du cercle central
#define INNER_ARC_WIDTH 54 // Largeur de l'arc intérieur
#define ICON_ZONE_DIAMETER 40 // Diamètre des zones cliquables
#define BOTTOM_BUTTON_SIZE 50 // Taille des boutons du bas
#define BOTTOM_BUTTON_SPACING 25 // Espacement entre boutons du bas
#define LONG_PRESS_THRESHOLD_MS 800 // Seuil pour un appui long (ms)

// Définitions des couleurs
#define BACKGROUND_COLOR 0x000000 // Noir pour le fond
#define BORDER_COLOR 0x404040     // Gris foncé pour la bordure
#define INNER_COLOR 0x222222      // Gris clair pour le cercle central
#define SELECTOR_COLOR 0x00BFFF   // Bleu clair pour la sélection
#define ICON_COLOR 0xFFFFFF       // Blanc pour les icônes

// Configuration structures
typedef enum {
    MENU_ITEM_SYMBOL,
    MENU_ITEM_IMAGE
} menu_item_type_t;

typedef struct {
    menu_item_type_t type;   // Type: symbol or image
    union {
        const char *symbol;  // Symbol string (e.g., LV_SYMBOL_SETTINGS)
        const char *img_path; // Image path (e.g., "L:/hometp.png")
    };
    const char *center_text; // Texte à afficher au centre
    void (*on_press)(int32_t section_id); // Callback pour l'appui
} menu_section_conf_t;

typedef struct {
    menu_item_type_t type;   // Type: symbol or image
    union {
        const char *icon;    // Symbol string (e.g., LV_SYMBOL_OK)
        const char *img_path; // Image path (e.g., "L:/hometp.png")
    };
    void (*on_press)(int32_t button_idx); // Callback pour l'appui
} bottom_button_conf_t;

typedef struct {
    uint32_t num_sections;           // Nombre de sections
    menu_section_conf_t *sections;   // Tableau des sections
    bottom_button_conf_t bottom_buttons[3]; // Trois boutons du bas
} circular_menu_conf_t;

// Structure pour les données du menu
typedef struct {
    lv_obj_t *screen;                // Écran principal
    lv_obj_t *menu;                  // Conteneur du menu
    lv_obj_t *arc;                   // Arc de sélection
    lv_obj_t *inner_arc;             // Arc intérieur
    lv_obj_t *center_label;          // Label pour le texte central
    lv_obj_t **click_zones;          // Zones cliquables (dynamique)
    lv_obj_t **icons;                // Icônes des sections (dynamique)
    lv_obj_t *bottom_button_labels[3]; // Labels des boutons du bas
    circular_menu_conf_t conf;       // Configuration du menu
    int32_t current_section;         // Section active
    uint32_t allocated_sections;     // Nombre de sections allouées
} circular_menu_data_t;

static circular_menu_data_t menu_data = {
    .screen = nullptr,
    .menu = nullptr,
    .arc = nullptr,
    .inner_arc = nullptr,
    .center_label = nullptr,
    .click_zones = nullptr,
    .icons = nullptr,
    .bottom_button_labels = {nullptr, nullptr, nullptr},
    .conf = {0, nullptr, {{MENU_ITEM_SYMBOL, {nullptr}, nullptr}, {MENU_ITEM_SYMBOL, {nullptr}, nullptr}, {MENU_ITEM_SYMBOL, {nullptr}, nullptr}}},
    .current_section = 0,
    .allocated_sections = 0
};

// Helper function to trigger a short beep
static void trigger_button_beep(void)
{
    buzzer_set_loud(false);
    buzzer_bip(1000, 100); // Beep de 100 ms
}

// Fonction pour mettre à jour les styles des icônes
static void update_icon_styles(void)
{
    for (int i = 0; i < menu_data.conf.num_sections; i++) {
        if (i == menu_data.current_section) {
            if (menu_data.conf.sections[i].type == MENU_ITEM_SYMBOL) {
                lv_obj_set_style_text_color(menu_data.icons[i], lv_color_hex(ICON_COLOR), LV_PART_MAIN);
            } else {
                lv_obj_set_style_img_recolor(menu_data.icons[i], lv_color_hex(ICON_COLOR), LV_PART_MAIN);
                lv_obj_set_style_img_recolor_opa(menu_data.icons[i], LV_OPA_COVER, LV_PART_MAIN);
            }
        } else {
            if (menu_data.conf.sections[i].type == MENU_ITEM_SYMBOL) {
                lv_obj_set_style_text_color(menu_data.icons[i], lv_color_hex(ICON_COLOR), LV_PART_MAIN);
            } else {
                lv_obj_set_style_img_recolor_opa(menu_data.icons[i], LV_OPA_TRANSP, LV_PART_MAIN);
            }
        }
    }
}

// Mettre à jour le texte central
static void update_center_text(void)
{
    if (menu_data.center_label && menu_data.conf.sections) {
        const char *text = menu_data.conf.sections[menu_data.current_section].center_text;
        lv_label_set_text(menu_data.center_label, text ? text : "");
        lv_obj_center(menu_data.center_label);
    }
}

// Function to get the active section ID
int32_t get_active_section_id(void)
{
    return menu_data.current_section;
}

// Helper function to simulate a click on the active section
static void simulate_click_on_active_section(void)
{
    if (menu_data.conf.sections[menu_data.current_section].type == MENU_ITEM_SYMBOL) {
        lv_obj_set_style_text_color(menu_data.icons[menu_data.current_section], lv_color_hex(SELECTOR_COLOR), LV_PART_MAIN);
    } else {
        lv_obj_set_style_img_recolor(menu_data.icons[menu_data.current_section], lv_color_hex(SELECTOR_COLOR), LV_PART_MAIN);
        lv_obj_set_style_img_recolor_opa(menu_data.icons[menu_data.current_section], LV_OPA_COVER, LV_PART_MAIN);
    }
    esp3d_log_d("Click zone pressed: active_section_id=%ld", menu_data.current_section);
    if (menu_data.conf.sections && menu_data.conf.sections[menu_data.current_section].on_press) {
        esp3d_log_d("Calling on_press callback for section %ld", menu_data.current_section);
        menu_data.conf.sections[menu_data.current_section].on_press(menu_data.current_section);
    } else {
        esp3d_log_e("No valid on_press callback for section %ld", menu_data.current_section);
    }
}

// Deletion callback
static void obj_delete_cb(lv_event_t *e)
{
    lv_obj_t *obj = (lv_obj_t *)lv_event_get_target(e);
    esp3d_log("Object deleted: %p", obj);
    if (obj == menu_data.screen) {
        if (menu_data.click_zones) {
            free(menu_data.click_zones);
            menu_data.click_zones = nullptr;
        }
        if (menu_data.icons) {
            free(menu_data.icons);
            menu_data.icons = nullptr;
        }
        memset(&menu_data, 0, sizeof(circular_menu_data_t));
        esp3d_log("Circular menu screen cleared");
    } else {
        for (int i = 0; i < 3; i++) {
            if (menu_data.bottom_button_labels[i] == obj) {
                menu_data.bottom_button_labels[i] = nullptr;
                esp3d_log("Bottom button label %d cleared", i);
            }
        }
    }
}

// Physical Buttons callback 
static void button_event_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    control_event_t *event = (control_event_t *)lv_event_get_param(e);
    if (!event || event->family_id != CONTROL_FAMILY_BUTTONS) {
        return;
    }
    uint32_t btn_id = event->btn_id;

    if (code == LV_EVENT_PRESSED)
    {
        esp3d_log_d("Button %ld pressed", btn_id + 1);
        trigger_button_beep();
        if (btn_id == 0) {
            simulate_click_on_active_section();
        }
    }
    else if (code == LV_EVENT_RELEASED)
    {
        uint32_t duration = event->press_duration;
        if (btn_id == 0) {
            update_icon_styles();
        }
        if (duration < LONG_PRESS_THRESHOLD_MS) {
            esp3d_log_d("Button %ld short press released (duration: %ld ms)", btn_id + 1, duration);
        } else {
            esp3d_log_d("Button %ld long press released (duration: %ld ms)", btn_id + 1, duration);
        }
    }
}

// Callback pour l'encodeur
static void encoder_event_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_KEY)
    {
        control_event_t *event = (control_event_t *)lv_event_get_param(e);
        if (event && event->family_id == CONTROL_FAMILY_ENCODER)
        {
            int32_t steps = event->steps;
            esp3d_log_d("Encoder steps received: %ld", steps);
            // Normalize steps to prevent large jumps
            if (steps > 1) steps = 1;
            if (steps < -1) steps = -1;

            int32_t prev_section = menu_data.current_section;
            // Update current_section with explicit wrapping
            menu_data.current_section += steps;
            if (menu_data.current_section < 0) {
                menu_data.current_section = menu_data.conf.num_sections - 1; // Go to last section
            } else if (menu_data.current_section >= menu_data.conf.num_sections) {
                menu_data.current_section = 0; // Go to first section
            }

            esp3d_log_d("Encoder: steps=%ld, prev_section=%ld, new_section=%ld",
                        steps, prev_section, menu_data.current_section);

            int32_t start_angle = menu_data.current_section * (360 / menu_data.conf.num_sections);
            int32_t end_angle = start_angle + (360 / menu_data.conf.num_sections);
            lv_arc_set_angles(menu_data.arc, start_angle, end_angle);
            lv_arc_set_angles(menu_data.inner_arc, start_angle, end_angle);

            update_icon_styles();
            update_center_text();
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
        trigger_button_beep();
        if (menu_data.conf.sections[section].type == MENU_ITEM_SYMBOL) {
            lv_obj_set_style_text_color(menu_data.icons[section], lv_color_hex(SELECTOR_COLOR), LV_PART_MAIN);
        } else {
            lv_obj_set_style_img_recolor(menu_data.icons[section], lv_color_hex(SELECTOR_COLOR), LV_PART_MAIN);
            lv_obj_set_style_img_recolor_opa(menu_data.icons[section], LV_OPA_COVER, LV_PART_MAIN);
        }
        menu_data.current_section = section;

        int32_t start_angle = menu_data.current_section * (360 / menu_data.conf.num_sections);
        int32_t end_angle = start_angle + (360 / menu_data.conf.num_sections);
        lv_arc_set_angles(menu_data.arc, start_angle, end_angle);
        lv_arc_set_angles(menu_data.inner_arc, start_angle, end_angle);

        esp3d_log_d("Click zone pressed: section=%ld", menu_data.current_section);
        if (menu_data.conf.sections && menu_data.conf.sections[section].on_press) {
            esp3d_log_d("Calling on_press callback for section %ld", section);
            menu_data.conf.sections[section].on_press(section);
        } else {
            esp3d_log_e("No valid on_press callback for section %ld", section);
        }
        update_center_text();
    }
    else if (code == LV_EVENT_RELEASED || code == LV_EVENT_PRESS_LOST)
    {
        update_icon_styles();
    }
}

// Callback for bottom buttons
static void bottom_button_event_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    int32_t button_idx = (int32_t)(intptr_t)lv_event_get_user_data(e);
    static uint32_t press_start_time[3] = {0, 0, 0};

    if (code == LV_EVENT_PRESSED)
    {
        esp3d_log_d("Bottom button pressed: index=%ld", button_idx);
        press_start_time[button_idx] = esp_timer_get_time() / 1000;
        trigger_button_beep();
        if (menu_data.bottom_button_labels[button_idx]) {
            if (menu_data.conf.bottom_buttons[button_idx].type == MENU_ITEM_SYMBOL) {
                lv_obj_set_style_text_color(menu_data.bottom_button_labels[button_idx], lv_color_hex(SELECTOR_COLOR), LV_PART_MAIN);
            } else {
                lv_obj_set_style_img_recolor(menu_data.bottom_button_labels[button_idx], lv_color_hex(SELECTOR_COLOR), LV_PART_MAIN);
                lv_obj_set_style_img_recolor_opa(menu_data.bottom_button_labels[button_idx], LV_OPA_COVER, LV_PART_MAIN);
            }
            esp3d_log_d("Bottom button %ld label set to blue", button_idx);
        }
        if (button_idx == 0) {
            simulate_click_on_active_section();
        } else if (menu_data.conf.bottom_buttons[button_idx].on_press) {
            esp3d_log_d("Calling on_press callback for bottom button %ld", button_idx);
            menu_data.conf.bottom_buttons[button_idx].on_press(button_idx);
        }
    }
    else if (code == LV_EVENT_RELEASED || code == LV_EVENT_PRESS_LOST)
    {
        uint32_t duration = (esp_timer_get_time() / 1000) - press_start_time[button_idx];
        if (menu_data.bottom_button_labels[button_idx]) {
            if (menu_data.conf.bottom_buttons[button_idx].type == MENU_ITEM_SYMBOL) {
                lv_obj_set_style_text_color(menu_data.bottom_button_labels[button_idx], lv_color_hex(ICON_COLOR), LV_PART_MAIN);
            } else {
                lv_obj_set_style_img_recolor_opa(menu_data.bottom_button_labels[button_idx], LV_OPA_TRANSP, LV_PART_MAIN);
            }
            esp3d_log_d("Bottom button %ld label reset to white", button_idx);
        }
        if (button_idx == 0) {
            update_icon_styles();
        }
        if (duration < LONG_PRESS_THRESHOLD_MS) {
            esp3d_log_d("Bottom button %ld short press released (duration: %ld ms)", button_idx, duration);
        } else {
            esp3d_log_d("Bottom button %ld long press released (duration: %ld ms)", button_idx, duration);
        }
    }
}

// Fonction pour créer le menu circulaire
lv_obj_t *create_circular_menu(lv_obj_t *parent, int32_t initial_section_id, circular_menu_conf_t menu_conf)
{
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-enum-enum-conversion"
    // Validate configuration
    if (!menu_conf.sections || menu_conf.num_sections == 0 || menu_conf.num_sections > MAX_SECTIONS) {
        esp3d_log_e("Invalid menu configuration: num_sections=%lu", menu_conf.num_sections);
        return nullptr;
    }

    // Debug configuration
    for (uint32_t i = 0; i < menu_conf.num_sections; i++) {
        if (menu_conf.sections[i].type == MENU_ITEM_SYMBOL) {
            esp3d_log_d("Section %lu: symbol=%s, center_text=%s, on_press=%p", 
                        i, 
                        menu_conf.sections[i].symbol ? menu_conf.sections[i].symbol : "null",
                        menu_conf.sections[i].center_text ? menu_conf.sections[i].center_text : "null",
                        (void*)menu_conf.sections[i].on_press);
        } else {
            esp3d_log_d("Section %lu: img_path=%s, center_text=%s, on_press=%p", 
                        i, 
                        menu_conf.sections[i].img_path ? menu_conf.sections[i].img_path : "null",
                        menu_conf.sections[i].center_text ? menu_conf.sections[i].center_text : "null",
                        (void*)menu_conf.sections[i].on_press);
        }
    }

    // Create screen if parent is NULL
    menu_data.screen = parent ? parent : lv_obj_create(NULL);
    lv_obj_set_style_bg_color(menu_data.screen, lv_color_hex(BACKGROUND_COLOR), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(menu_data.screen, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_scrollbar_mode(menu_data.screen, LV_SCROLLBAR_MODE_OFF);
    lv_obj_add_event_cb(menu_data.screen, obj_delete_cb, LV_EVENT_DELETE, NULL);

    // Store configuration
    menu_data.conf = menu_conf;
    menu_data.current_section = initial_section_id < 0 || initial_section_id >= menu_conf.num_sections ? 0 : initial_section_id;
    menu_data.allocated_sections = menu_conf.num_sections;

    // Allocate dynamic arrays
    menu_data.click_zones = (lv_obj_t **)malloc(menu_conf.num_sections * sizeof(lv_obj_t *));
    menu_data.icons = (lv_obj_t **)malloc(menu_conf.num_sections * sizeof(lv_obj_t *));
    if (!menu_data.click_zones || !menu_data.icons) {
        esp3d_log_e("Failed to allocate memory for click_zones or icons");
        if (menu_data.click_zones) free(menu_data.click_zones);
        if (menu_data.icons) free(menu_data.icons);
        lv_obj_delete(menu_data.screen);
        memset(&menu_data, 0, sizeof(circular_menu_data_t));
        return nullptr;
    }
    memset(menu_data.click_zones, 0, menu_conf.num_sections * sizeof(lv_obj_t *));
    memset(menu_data.icons, 0, menu_conf.num_sections * sizeof(lv_obj_t *));

    // Initialize bottom button labels
    for (int i = 0; i < 3; i++) {
        menu_data.bottom_button_labels[i] = nullptr;
    }

    // Create container for the circular menu
    menu_data.menu = lv_obj_create(menu_data.screen);
    lv_obj_set_style_bg_color(menu_data.menu, lv_color_hex(BACKGROUND_COLOR), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(menu_data.menu, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_size(menu_data.menu, CIRCLE_DIAMETER, CIRCLE_DIAMETER);
    lv_obj_set_style_border_width(menu_data.menu, 0, LV_PART_MAIN);
    lv_obj_set_style_border_opa(menu_data.menu, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_align(menu_data.menu, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_set_scrollbar_mode(menu_data.menu, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_style_pad_all(menu_data.menu, 0, LV_PART_MAIN);
    lv_obj_set_style_border_width(menu_data.menu, 0, LV_PART_MAIN | LV_STATE_ANY);
    lv_obj_set_style_border_opa(menu_data.menu, LV_OPA_TRANSP, LV_PART_MAIN | LV_STATE_ANY);
    lv_obj_add_event_cb(menu_data.menu, obj_delete_cb, LV_EVENT_DELETE, NULL);

    // Créer le cercle de base (bordure gris foncé)
    lv_obj_t *circle = lv_obj_create(menu_data.menu);
    lv_obj_set_size(circle, CIRCLE_DIAMETER, CIRCLE_DIAMETER);
    lv_obj_center(circle);
    lv_obj_set_style_radius(circle, LV_RADIUS_CIRCLE, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(circle, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_color(circle, lv_color_hex(BORDER_COLOR), LV_PART_MAIN);
    lv_obj_set_style_border_width(circle, BORDER_WIDTH, LV_PART_MAIN);
    lv_obj_set_style_border_opa(circle, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_scrollbar_mode(circle, LV_SCROLLBAR_MODE_OFF);

    // Créer le cercle central (gris clair, rempli)
    lv_obj_t *inner_circle = lv_obj_create(menu_data.menu);
    lv_obj_set_size(inner_circle, INNER_CIRCLE_DIAMETER, INNER_CIRCLE_DIAMETER);
    lv_obj_center(inner_circle);
    lv_obj_set_style_radius(inner_circle, LV_RADIUS_CIRCLE, LV_PART_MAIN);
    lv_obj_set_style_bg_color(inner_circle, lv_color_hex(INNER_COLOR), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(inner_circle, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_width(inner_circle, 0, LV_PART_MAIN);
    lv_obj_set_scrollbar_mode(inner_circle, LV_SCROLLBAR_MODE_OFF);

    // Créer le label pour le texte central
    menu_data.center_label = lv_label_create(inner_circle);
    lv_obj_set_style_text_color(menu_data.center_label, lv_color_hex(ICON_COLOR), LV_PART_MAIN);
    lv_obj_set_scrollbar_mode(menu_data.center_label, LV_SCROLLBAR_MODE_OFF);
    update_center_text();

    // Créer l'arc intérieur (gris clair)
    menu_data.inner_arc = lv_arc_create(menu_data.menu);
    lv_obj_set_size(menu_data.inner_arc, CIRCLE_DIAMETER - (2 * BORDER_WIDTH), CIRCLE_DIAMETER - (2 * BORDER_WIDTH));
    lv_obj_center(menu_data.inner_arc);
    lv_arc_set_angles(menu_data.inner_arc, menu_data.current_section * (360 / menu_data.conf.num_sections), menu_data.current_section * (360 / menu_data.conf.num_sections) + (360 / menu_data.conf.num_sections));
    lv_obj_set_style_arc_width(menu_data.inner_arc, INNER_ARC_WIDTH, LV_PART_INDICATOR);
    lv_obj_set_style_arc_color(menu_data.inner_arc, lv_color_hex(INNER_COLOR), LV_PART_INDICATOR);
    lv_obj_set_style_arc_opa(menu_data.inner_arc, LV_OPA_COVER, LV_PART_INDICATOR);
    lv_obj_set_style_arc_rounded(menu_data.inner_arc, false, LV_PART_INDICATOR);
    lv_obj_set_style_bg_opa(menu_data.inner_arc, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_arc_set_bg_angles(menu_data.inner_arc, 0, 0);
    lv_obj_set_style_arc_opa(menu_data.inner_arc, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_opa(menu_data.inner_arc, LV_OPA_TRANSP, LV_PART_KNOB);
    lv_obj_set_style_size(menu_data.inner_arc, 0, 0, LV_PART_KNOB);
    lv_obj_set_style_radius(menu_data.inner_arc, 0, LV_PART_KNOB);
    lv_obj_set_scrollbar_mode(menu_data.inner_arc, LV_SCROLLBAR_MODE_OFF);

    // Créer l'arc de sélection (bleu)
    menu_data.arc = lv_arc_create(menu_data.menu);
    lv_obj_set_size(menu_data.arc, CIRCLE_DIAMETER, CIRCLE_DIAMETER);
    lv_obj_center(menu_data.arc);
    lv_arc_set_angles(menu_data.arc, menu_data.current_section * (360 / menu_data.conf.num_sections), menu_data.current_section * (360 / menu_data.conf.num_sections) + (360 / menu_data.conf.num_sections));
    lv_obj_set_style_arc_width(menu_data.arc, BORDER_WIDTH, LV_PART_INDICATOR);
    lv_obj_set_style_arc_color(menu_data.arc, lv_color_hex(SELECTOR_COLOR), LV_PART_INDICATOR);
    lv_obj_set_style_arc_opa(menu_data.arc, LV_OPA_COVER, LV_PART_INDICATOR);
    lv_obj_set_style_bg_opa(menu_data.arc, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_arc_set_bg_angles(menu_data.arc, 0, 0);
    lv_obj_set_style_arc_opa(menu_data.arc, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_opa(menu_data.arc, LV_OPA_TRANSP, LV_PART_KNOB);
    lv_obj_set_style_size(menu_data.arc, 0, 0, LV_PART_KNOB);
    lv_obj_set_style_radius(menu_data.arc, 0, LV_PART_KNOB);
    lv_obj_set_scrollbar_mode(menu_data.arc, LV_SCROLLBAR_MODE_OFF);

    // Créer les zones cliquables avec icônes ou images
    for (int i = 0; i < menu_conf.num_sections; i++) {
        lv_obj_t *click_zone = lv_obj_create(menu_data.menu);
        menu_data.click_zones[i] = click_zone;
        lv_obj_set_size(click_zone, ICON_ZONE_DIAMETER, ICON_ZONE_DIAMETER);
        lv_obj_set_style_radius(click_zone, LV_RADIUS_CIRCLE, LV_PART_MAIN);

        float mid_angle_rad = (i * (360.0 / menu_conf.num_sections) + (360.0 / menu_conf.num_sections) / 2) * (M_PI / 180.0);
        int32_t radius = (CIRCLE_DIAMETER - INNER_CIRCLE_DIAMETER) / 4 + INNER_CIRCLE_DIAMETER / 2;
        int32_t offset_x = (int32_t)(cos(mid_angle_rad) * radius);
        int32_t offset_y = (int32_t)(sin(mid_angle_rad) * radius);
        lv_obj_align(click_zone, LV_ALIGN_CENTER, offset_x, offset_y);

        lv_obj_set_style_bg_opa(click_zone, LV_OPA_TRANSP, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_border_opa(click_zone, LV_OPA_TRANSP, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_shadow_opa(click_zone, LV_OPA_TRANSP, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_outline_opa(click_zone, LV_OPA_TRANSP, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_border_width(click_zone, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_pad_all(click_zone, 0, LV_PART_MAIN | LV_STATE_DEFAULT);

        lv_obj_set_style_bg_opa(click_zone, LV_OPA_TRANSP, LV_PART_MAIN | LV_STATE_PRESSED);
        lv_obj_set_style_border_opa(click_zone, LV_OPA_TRANSP, LV_PART_MAIN | LV_STATE_PRESSED);
        lv_obj_set_style_shadow_opa(click_zone, LV_OPA_TRANSP, LV_PART_MAIN | LV_STATE_PRESSED);
        lv_obj_set_style_outline_opa(click_zone, LV_OPA_TRANSP, LV_PART_MAIN | LV_STATE_PRESSED);
        lv_obj_set_style_border_width(click_zone, 0, LV_PART_MAIN | LV_STATE_PRESSED);
        lv_obj_set_style_pad_all(click_zone, 0, LV_PART_MAIN | LV_STATE_PRESSED);
        lv_obj_set_style_bg_grad_color(click_zone, lv_color_hex(BACKGROUND_COLOR), LV_PART_MAIN | LV_STATE_PRESSED);
        lv_obj_set_style_bg_main_stop(click_zone, 0, LV_PART_MAIN | LV_STATE_PRESSED);
        lv_obj_set_style_bg_grad_stop(click_zone, 0, LV_PART_MAIN | LV_STATE_PRESSED);

        lv_obj_add_flag(click_zone, LV_OBJ_FLAG_CLICKABLE);

        lv_obj_t *icon;
        if (menu_conf.sections[i].type == MENU_ITEM_SYMBOL) {
            icon = lv_label_create(click_zone);
            lv_label_set_text(icon, menu_conf.sections[i].symbol ? menu_conf.sections[i].symbol : "");
            lv_obj_set_style_text_color(icon, lv_color_hex(ICON_COLOR), LV_PART_MAIN);
        } else {
            icon = lv_img_create(click_zone);
            lv_img_set_src(icon, menu_conf.sections[i].img_path ? menu_conf.sections[i].img_path : "L:/default.png");
            lv_obj_set_style_img_recolor_opa(icon, LV_OPA_TRANSP, LV_PART_MAIN);
        }
        menu_data.icons[i] = icon;
        lv_obj_center(icon);
        lv_obj_set_style_bg_opa(icon, LV_OPA_TRANSP, LV_PART_MAIN | LV_STATE_DEFAULT | LV_STATE_PRESSED);
        lv_obj_set_style_border_opa(icon, LV_OPA_TRANSP, LV_PART_MAIN | LV_STATE_DEFAULT | LV_STATE_PRESSED);
        lv_obj_set_style_shadow_opa(icon, LV_OPA_TRANSP, LV_PART_MAIN | LV_STATE_DEFAULT | LV_STATE_PRESSED);
        lv_obj_set_style_outline_opa(icon, LV_OPA_TRANSP, LV_PART_MAIN | LV_STATE_DEFAULT | LV_STATE_PRESSED);
        lv_obj_set_style_border_width(icon, 0, LV_PART_MAIN | LV_STATE_DEFAULT | LV_STATE_PRESSED);
        lv_obj_set_style_pad_all(icon, 0, LV_PART_MAIN | LV_STATE_DEFAULT | LV_STATE_PRESSED);
        lv_obj_set_scrollbar_mode(icon, LV_SCROLLBAR_MODE_OFF);

        lv_obj_add_event_cb(click_zone, click_zone_event_cb, LV_EVENT_PRESSED, (void *)(intptr_t)i);
        lv_obj_add_event_cb(click_zone, click_zone_event_cb, LV_EVENT_RELEASED, (void *)(intptr_t)i);
        lv_obj_add_event_cb(click_zone, click_zone_event_cb, LV_EVENT_PRESS_LOST, (void *)(intptr_t)i);
        lv_obj_add_event_cb(click_zone, obj_delete_cb, LV_EVENT_DELETE, NULL);
        lv_obj_add_event_cb(icon, obj_delete_cb, LV_EVENT_DELETE, NULL);
    }

    // Create the three bottom buttons
    for (int i = 0; i < 3; i++) {
        if (!menu_conf.bottom_buttons[i].on_press) {
            esp3d_log_d("Bottom button %d hidden (no callback)", i);
            continue;
        }

        lv_obj_t *button = lv_btn_create(menu_data.screen);
        lv_obj_set_size(button, BOTTOM_BUTTON_SIZE, BOTTOM_BUTTON_SIZE);
        lv_obj_set_style_radius(button, LV_RADIUS_CIRCLE, LV_PART_MAIN);
        lv_obj_set_style_bg_color(button, lv_color_hex(BACKGROUND_COLOR), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_opa(button, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_border_color(button, lv_color_hex(BORDER_COLOR), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_border_width(button, BORDER_WIDTH, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_border_opa(button, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_shadow_opa(button, LV_OPA_TRANSP, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_color(button, lv_color_hex(BACKGROUND_COLOR), LV_PART_MAIN | LV_STATE_PRESSED);
        lv_obj_set_style_bg_opa(button, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_PRESSED);
        lv_obj_set_style_border_color(button, lv_color_hex(SELECTOR_COLOR), LV_PART_MAIN | LV_STATE_PRESSED);
        lv_obj_set_style_border_width(button, BORDER_WIDTH, LV_PART_MAIN | LV_STATE_PRESSED);
        lv_obj_set_style_border_opa(button, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_PRESSED);
        lv_obj_set_style_shadow_opa(button, LV_OPA_TRANSP, LV_PART_MAIN | LV_STATE_PRESSED);
        lv_obj_set_scrollbar_mode(button, LV_SCROLLBAR_MODE_OFF);

        int32_t offset_x = (i - 1) * (BOTTOM_BUTTON_SIZE + BOTTOM_BUTTON_SPACING);
        lv_obj_align(button, LV_ALIGN_BOTTOM_MID, offset_x, -10);

        lv_obj_t *icon;
        if (menu_conf.bottom_buttons[i].type == MENU_ITEM_SYMBOL) {
            icon = lv_label_create(button);
            lv_label_set_text(icon, menu_conf.bottom_buttons[i].icon ? menu_conf.bottom_buttons[i].icon : "");
            lv_obj_set_style_text_color(icon, lv_color_hex(ICON_COLOR), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_color(icon, lv_color_hex(SELECTOR_COLOR), LV_PART_MAIN | LV_STATE_PRESSED);
        } else {
            icon = lv_img_create(button);
            lv_img_set_src(icon, menu_conf.bottom_buttons[i].img_path ? menu_conf.bottom_buttons[i].img_path : "L:/default.png");
            lv_obj_set_style_img_recolor_opa(icon, LV_OPA_TRANSP, LV_PART_MAIN);
        }
        menu_data.bottom_button_labels[i] = icon;
        lv_obj_center(icon);
        lv_obj_set_style_text_opa(icon, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT | LV_STATE_PRESSED);
        lv_obj_set_style_bg_opa(icon, LV_OPA_TRANSP, LV_PART_MAIN);
        lv_obj_set_style_border_opa(icon, LV_OPA_TRANSP, LV_PART_MAIN);
        lv_obj_set_style_shadow_opa(icon, LV_OPA_TRANSP, LV_PART_MAIN);
        lv_obj_set_style_outline_opa(icon, LV_OPA_TRANSP, LV_PART_MAIN);
        lv_obj_set_style_border_width(icon, 0, LV_PART_MAIN);
        lv_obj_set_style_pad_all(icon, 0, LV_PART_MAIN);
        lv_obj_set_scrollbar_mode(icon, LV_SCROLLBAR_MODE_OFF);

        lv_obj_add_event_cb(button, bottom_button_event_cb, LV_EVENT_PRESSED, (void *)(intptr_t)i);
        lv_obj_add_event_cb(button, bottom_button_event_cb, LV_EVENT_RELEASED, (void *)(intptr_t)i);
        lv_obj_add_event_cb(button, bottom_button_event_cb, LV_EVENT_PRESS_LOST, (void *)(intptr_t)i);
        lv_obj_add_event_cb(button, obj_delete_cb, LV_EVENT_DELETE, NULL);
        lv_obj_add_event_cb(icon, obj_delete_cb, LV_EVENT_DELETE, NULL);

        esp3d_log_d("Bottom button %d created with label reference", i);
    }

    update_icon_styles();
    lv_obj_add_event_cb(menu_data.screen, encoder_event_cb, LV_EVENT_KEY, NULL);
    lv_obj_add_event_cb(menu_data.screen, button_event_cb, LV_EVENT_PRESSED, NULL);
    lv_obj_add_event_cb(menu_data.screen, button_event_cb, LV_EVENT_RELEASED, NULL);

    return menu_data.screen;
#pragma GCC diagnostic pop
}

// Example configuration and callbacks
static void section_press_cb(int32_t section_id)
{
    esp3d_log_d("Menu section %ld pressed", section_id);
}

static void bottom_button_press_cb(int32_t button_idx)
{
    esp3d_log_d("Bottom button %ld pressed", button_idx);
}

static menu_section_conf_t main_menu_sections[] = {
    {MENU_ITEM_SYMBOL, {.symbol = LV_SYMBOL_SETTINGS}, "Settings", section_press_cb},
    {MENU_ITEM_SYMBOL, {.symbol = LV_SYMBOL_LIST}, "List", section_press_cb},
    {MENU_ITEM_SYMBOL, {.symbol = LV_SYMBOL_HOME}, "Home", section_press_cb},
    {MENU_ITEM_SYMBOL, {.symbol = LV_SYMBOL_EJECT}, "Eject", section_press_cb},
    {MENU_ITEM_SYMBOL, {.symbol = LV_SYMBOL_PLAY}, "Play", section_press_cb},
    {MENU_ITEM_SYMBOL, {.symbol = LV_SYMBOL_FILE}, "File", section_press_cb},
    {MENU_ITEM_IMAGE, {.img_path = "L:/poo.png"}, "Poo", section_press_cb},
};

static circular_menu_conf_t main_menu_conf = {
    .num_sections = 7,
    .sections = main_menu_sections,
    .bottom_buttons = {
        {MENU_ITEM_SYMBOL, {.icon = LV_SYMBOL_OK}, bottom_button_press_cb}, // Button 0 visible
        {MENU_ITEM_SYMBOL, {.icon = LV_SYMBOL_CLOSE}, bottom_button_press_cb}, // Button 1 hidden
        {MENU_ITEM_IMAGE, {.img_path = "L:/poo.png"}, bottom_button_press_cb} // Button 2 visible
    }
};

// Créer l'interface utilisateur
void create_application(void)
{
    esp3d_log_d("Creating LVGL application UI");

    lv_display_t *display = get_lvgl_display();
    if (!display)
    {
        esp3d_log_e("LVGL display not initialized");
        return;
    }

    lv_obj_t *screen = create_circular_menu(NULL, 0, main_menu_conf);
    if (screen) {
        lv_screen_load(screen);
        esp3d_log_d("Circular menu screen loaded");
    }

    esp3d_log_d("LVGL application UI created");
    screen_on_delay_timer = lv_timer_create(screen_on_delay_timer_cb, 50, NULL);
}