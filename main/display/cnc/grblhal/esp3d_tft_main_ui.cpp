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

lv_timer_t *screen_on_delay_timer = nullptr;

void screen_on_delay_timer_cb(lv_timer_t *timer) {
lv_timer_del(screen_on_delay_timer);
backlight_set(100);
}

// Widgets pour l'UI
static lv_obj_t *event_label = NULL;
static lv_obj_t *encoder_slider = NULL;
static lv_obj_t *switch_btn_container = NULL;

// Callback pour les événements de touch
static void screen_touch_event_cb(lv_event_t *e)
{
lv_event_code_t code = lv_event_get_code(e);
lv_obj_t *label = (lv_obj_t *)lv_event_get_user_data(e);

if (code == LV_EVENT_PRESSED || code == LV_EVENT_PRESSING) {
lv_point_t point;
lv_indev_t *indev = lv_indev_get_act();
if (indev != NULL) {
lv_indev_get_point(indev, &point);
lv_label_set_text_fmt(label, "Touch: %ld,%ld", (long)point.x, (long)point.y);
}
} else if (code == LV_EVENT_RELEASED) {
lv_label_set_text(label, "Touch: ---,---");
}
}

// Callback pour les boutons
static void button_event_cb(lv_event_t *e)
{
lv_event_code_t code = lv_event_get_code(e);
lv_indev_t *indev = lv_event_get_indev(e);
lv_indev_type_t type = lv_indev_get_type(indev);
lv_obj_t *label = (lv_obj_t *)lv_event_get_user_data(e);
if (code == LV_EVENT_PRESSED && type == LV_INDEV_TYPE_BUTTON) {
lv_point_t point;
lv_indev_get_point(indev, &point);
uint32_t btn_id = point.x;
esp3d_log_d("Button key: %ld", btn_id);
switch (btn_id) {
case 0: lv_label_set_text(label, "Button: 1"); break;
case 1: lv_label_set_text(label, "Button: 2"); break;
case 2: lv_label_set_text(label, "Button: 3"); break;
default: lv_label_set_text(label, "Button: Unknown"); break;
}
}
}

// Callback pour l'encodeur
static void encoder_event_cb(lv_event_t *e)
{
lv_event_code_t code = lv_event_get_code(e);
lv_indev_t *indev = lv_event_get_indev(e);
lv_obj_t *slider = (lv_obj_t *)lv_event_get_target(e);
lv_obj_t *label = (lv_obj_t *)lv_event_get_user_data(e);
if (code == LV_EVENT_KEY && lv_indev_get_type(indev) == LV_INDEV_TYPE_ENCODER) {
uint32_t key = lv_indev_get_key(indev);
esp3d_log_d("Encoder key: %ld", key);
int32_t value = lv_slider_get_value(slider);
if (key == LV_KEY_RIGHT) {
value += 5; // Incrément de 5
if (value > 100) value = 100;
lv_slider_set_value(slider, value, LV_ANIM_OFF);
lv_label_set_text_fmt(label, "Encoder: %ld", value);
} else if (key == LV_KEY_LEFT) {
value -= 5; // Décrément de 5
if (value < 0) value = 0;
lv_slider_set_value(slider, value, LV_ANIM_OFF);
lv_label_set_text_fmt(label, "Encoder: %ld", value);
}
}
}

// Callback pour le switch
static void switch_event_cb(lv_event_t *e)
{
lv_event_code_t code = lv_event_get_code(e);
lv_indev_t *indev = lv_event_get_indev(e);
lv_obj_t *container = (lv_obj_t *)lv_event_get_user_data(e);
if (code == LV_EVENT_PRESSED && lv_indev_get_type(indev) == LV_INDEV_TYPE_KEYPAD) {
uint32_t key = lv_indev_get_key(indev);
esp3d_log_d("Switch key: %ld", key);
lv_obj_t *label = lv_obj_get_child(container, 4); // Label est le 5e enfant
int btn_index = -1;
switch (key) {
case 0x31: btn_index = 0; lv_label_set_text(label, "Switch: Position 1"); break;
case 0x32: btn_index = 1; lv_label_set_text(label, "Switch: Position 2"); break;
case 0x33: btn_index = 2; lv_label_set_text(label, "Switch: Position 3"); break;
case 0x34: btn_index = 3; lv_label_set_text(label, "Switch: Position 4"); break;
default: lv_label_set_text(label, "Switch: Unknown"); break;
}
// Mettre à jour le focus sur le bouton correspondant
if (btn_index >= 0) {
for (int i = 0; i < 4; i++) {
lv_obj_t *btn = lv_obj_get_child(container, i);
if (i == btn_index) {
lv_obj_add_state(btn, LV_STATE_FOCUSED);
} else {
lv_obj_clear_state(btn, LV_STATE_FOCUSED);
}
}
}
}
}

// Créer l'interface utilisateur
void create_application(void) {
esp3d_log("Creating LVGL application UI");

// Récupérer les groupes
lv_group_t *encoder_group = get_encoder_group();
lv_group_t *switch_group = get_switch_group();
if (!encoder_group || !switch_group) {
esp3d_log_e("Failed to get input groups");
return;
}
esp3d_log_d("Encoder group: %p", encoder_group);
esp3d_log_d("Switch group: %p", switch_group);

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
lv_obj_set_size(main_container, 240, 320);
lv_obj_align(main_container, LV_ALIGN_CENTER, 0, 0);
lv_obj_set_flex_flow(main_container, LV_FLEX_FLOW_COLUMN);
lv_obj_set_style_pad_all(main_container, 10, LV_PART_MAIN);
lv_obj_set_style_bg_opa(main_container, LV_OPA_TRANSP, LV_PART_MAIN);

// Créer un label pour afficher les événements
event_label = lv_label_create(main_container);
lv_label_set_text(event_label, "Waiting for input...");
lv_obj_align(event_label, LV_ALIGN_TOP_MID, 0, 0);
lv_obj_set_style_text_color(event_label, lv_color_white(), LV_PART_MAIN);

// Créer un slider pour l'encodeur
encoder_slider = lv_slider_create(main_container);
lv_obj_set_size(encoder_slider, 200, 20);
lv_obj_align(encoder_slider, LV_ALIGN_TOP_MID, 0, 40);
lv_slider_set_range(encoder_slider, 0, 100);
lv_slider_set_value(encoder_slider, 50, LV_ANIM_OFF);
lv_obj_add_flag(encoder_slider, LV_OBJ_FLAG_EVENT_BUBBLE); // Permet aux événements de remonter

// Créer un conteneur pour les boutons du switch
switch_btn_container = lv_obj_create(main_container);
lv_obj_set_size(switch_btn_container, 200, 80);
lv_obj_align(switch_btn_container, LV_ALIGN_TOP_MID, 0, 80);
lv_obj_set_flex_flow(switch_btn_container, LV_FLEX_FLOW_ROW);
lv_obj_set_style_pad_all(switch_btn_container, 5, LV_PART_MAIN);

// Ajouter 4 boutons pour les positions du switch
const char *positions[] = {"Pos 1", "Pos 2", "Pos 3", "Pos 4"};
for (int i = 0; i < 4; i++) {
lv_obj_t *btn = lv_button_create(switch_btn_container);
lv_obj_t *btn_label = lv_label_create(btn);
lv_label_set_text(btn_label, positions[i]);
lv_obj_center(btn_label);
lv_obj_set_size(btn, 45, 30);
}

// Ajouter le label comme enfant du conteneur pour les mises à jour
lv_obj_t *switch_label = lv_label_create(switch_btn_container);
lv_label_set_text(switch_label, "Switch: ---");
lv_obj_align(switch_label, LV_ALIGN_TOP_MID, 0, 40);
lv_obj_set_style_text_color(switch_label, lv_color_white(), LV_PART_MAIN);

// Ajouter les widgets aux groupes
lv_group_add_obj(encoder_group, encoder_slider);
esp3d_log_d("Slider added to encoder_group");
for (int i = 0; i < 4; i++) {
lv_obj_t *btn = lv_obj_get_child(switch_btn_container, i);
lv_group_add_obj(switch_group, btn);
}
esp3d_log_d("Buttons added to switch_group");

// Attacher les gestionnaires d'événements
lv_obj_add_event_cb(encoder_slider, encoder_event_cb, LV_EVENT_KEY, event_label);
lv_obj_add_event_cb(switch_btn_container, switch_event_cb, LV_EVENT_PRESSED, switch_btn_container);
lv_obj_add_event_cb(screen, button_event_cb, LV_EVENT_PRESSED, event_label);
lv_obj_add_event_cb(screen, screen_touch_event_cb, LV_EVENT_ALL, event_label);

// Créer un bouton de test
lv_obj_t *test_btn = lv_button_create(main_container);
lv_obj_t *test_label = lv_label_create(test_btn);
lv_label_set_text_static(test_label, "Test");
lv_obj_center(test_label);
lv_obj_align(test_btn, LV_ALIGN_TOP_MID, 0, 180);
lv_obj_set_size(test_btn, 80, 30);
lv_obj_add_event_cb(test_btn, [](lv_event_t *e) {
lv_obj_t *btn = (lv_obj_t *)lv_event_get_target(e);
static uint32_t counter = 0;
counter++;
lv_obj_t *label = lv_obj_get_child(btn, 0);
lv_label_set_text_fmt(label, "Test %lu", counter);
esp3d_log("Test button pressed %lu times", counter);
}, LV_EVENT_CLICKED, NULL);

// Activer le focus sur le slider
lv_group_focus_obj(encoder_slider);
lv_group_set_default(encoder_group);

// Désactiver la navigation par focus pour éviter les changements indésirables
lv_group_set_editing(encoder_group, true); // Mode édition pour le slider

esp3d_log("LVGL application UI created");
screen_on_delay_timer = lv_timer_create(screen_on_delay_timer_cb, 50, NULL);
}