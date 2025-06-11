/*
  esp3d_tft

  Copyright (c) 2022 Luc Lebosse. All rights reserved.

  This code is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This code is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include "screens/main_screen.h"

#include <esp_timer.h>
#include <lvgl.h>
#include <math.h>
#include <stdlib.h>

#include "board_config.h"
#include "board_init.h"
#include "buzzer/esp3d_buzzer.h"
#include "control_event.h"
#include "disp_backlight.h"
#include "esp3d_hal.h"
#include "esp3d_log.h"
#include "esp3d_lvgl.h"
#include "esp3d_string.h"
#include "esp3d_styles.h"
#include "esp3d_tft_ui.h"
#include "phy_potentiometer.h"
#include "phy_switch.h"

// #include "screens/positions_screen.h"
#include "translations/esp3d_translation_service.h"

extern const lv_image_dsc_t workspaces_m;
extern const lv_image_dsc_t ok_b;
extern const lv_image_dsc_t lock_b;
extern const lv_image_dsc_t unlock_b;
extern const lv_image_dsc_t reset_m;
extern const lv_image_dsc_t settings_m;
extern const lv_image_dsc_t information_m;
extern const lv_image_dsc_t positions_m;
extern const lv_image_dsc_t jog_m;
extern const lv_image_dsc_t files_m;
extern const lv_image_dsc_t macros_m;
extern const lv_image_dsc_t probe_m;
extern const lv_image_dsc_t changetool_m;
extern const lv_image_dsc_t alarm_s;
extern const lv_image_dsc_t run_s;
extern const lv_image_dsc_t idle_s;
extern const lv_image_dsc_t status_s;

/**********************
 *  Namespace
 **********************/
namespace mainScreen {
bool is_locked = false;  // Global variable to track lock state
 //Enum for firmware states
// This enum defines the possible states of the firmware
  typedef enum {
    FIRMWARE_IDLE,
    FIRMWARE_RUN,
    FIRMWARE_ALARM,
    FIRMWARE_OTHER
  } firmware_state_t;

  // Variables for firmware and connection status
  static lv_obj_t *firmware_status_img = nullptr;  // Image for firmware status
  static lv_obj_t *connection_status_img = nullptr;  // Image for connection status
  static firmware_state_t current_firmware_state = FIRMWARE_IDLE;  // Current firmware state
    static bool is_connection_ok = false;  // Status of the connection

// Configuration structures

// Types for menu items
typedef enum
{
    MENU_ITEM_SYMBOL,
    MENU_ITEM_IMAGE
} menu_item_type_t;

// Configuration for each section of the circular menu
typedef struct
{
    menu_item_type_t type;  // Type: symbol or image
    union
    {
        const char *symbol;    // Symbol string (e.g., LV_SYMBOL_SETTINGS)
        const void *img_path;  // Image path (e.g., "L:/hometp.png" or pointer to lv_image_dsc_t)
    } data;
    std::string center_text;               // Text to display in the center of the section
    void (*on_press)(int32_t section_id);  // Callback for section press
} menu_section_conf_t;

// Configuration for bottom buttons
typedef struct
{
    menu_item_type_t type;  // Type: symbol or image
    union
    {
        const char *symbol;    // Symbol string (e.g., LV_SYMBOL_OK)
        const void *img_path;  // Image path (e.g., "L:/hometp.png" or pointer to lv_image_dsc_t)
    } data;
    void (*on_press)(int32_t button_idx);  // Callback for button press
} bottom_button_conf_t;

typedef struct
{
    uint32_t num_sections;                   // Section count
    menu_section_conf_t *sections;           // Arrays of menu sections
    bottom_button_conf_t bottom_buttons[3];  // Bottom buttons configuration (3 buttons)
} circular_menu_conf_t;

// Data structure for the circular menu
typedef struct
{
    lv_obj_t *screen;                   // screen object
    lv_obj_t *menu;                     // menu container
    lv_obj_t *arc;                      // Selection arc
    lv_obj_t *inner_arc;                // Inner arc for the center circle
    lv_obj_t *center_label;             // Label in the center of the menu
    lv_obj_t **click_zones;             // Clickable zones for each section (dynamic)
    lv_obj_t **icons;                   // Icons for each section (dynamic)
    lv_obj_t *bottom_button_labels[3];  // Labels for bottom buttons (dynamic)
    circular_menu_conf_t conf;          // Menu configuration
    int32_t current_section;            // Currently selected section
    uint32_t allocated_sections;        // Number of allocated sections
} circular_menu_data_t;

// Create a static instance of the circular menu data
static circular_menu_data_t menu_data = {.screen               = nullptr,
                                         .menu                 = nullptr,
                                         .arc                  = nullptr,
                                         .inner_arc            = nullptr,
                                         .center_label         = nullptr,
                                         .click_zones          = nullptr,
                                         .icons                = nullptr,
                                         .bottom_button_labels = {nullptr, nullptr, nullptr},
                                         .conf                 = {0,
                                                                  nullptr,
                                                                  {{MENU_ITEM_SYMBOL, {nullptr}, nullptr},
                                                                   {MENU_ITEM_SYMBOL, {nullptr}, nullptr},
                                                                   {MENU_ITEM_SYMBOL, {nullptr}, nullptr}}},
                                         .current_section      = 0,
                                         .allocated_sections   = 0};

// Forward declarations of callback functions

static void section_press_cb(int32_t section_id);
static void bottom_button_press_cb(int32_t button_idx);
static void bottom_button_lock_press_cb(int32_t button_idx);

// Structure for the main menu sections
// This structure defines the sections of the main menu
// Each section can be an image or a symbol, and has a callback for press events
static menu_section_conf_t main_menu_sections[] = {
    {MENU_ITEM_IMAGE, {.img_path = &settings_m}, "Settings", section_press_cb},
    {MENU_ITEM_IMAGE, {.img_path = &information_m}, "Information", section_press_cb},
    {MENU_ITEM_IMAGE, {.img_path = &positions_m}, "Positions", section_press_cb},
    {MENU_ITEM_IMAGE, {.img_path = &jog_m}, "jog", section_press_cb},
    {MENU_ITEM_IMAGE, {.img_path = &files_m}, "Files", section_press_cb},
    {MENU_ITEM_IMAGE, {.img_path = &macros_m}, "Macros", section_press_cb},
    {MENU_ITEM_IMAGE, {.img_path = &probe_m}, "Probe", section_press_cb},
    {MENU_ITEM_IMAGE, {.img_path = &workspaces_m}, "Workspaces", section_press_cb},
    {MENU_ITEM_IMAGE, {.img_path = &changetool_m}, "Change tool", section_press_cb},
};

#define ESP3D_MAIN_MENU_INITIAL_SECTION_ID 0

// Configuration for the bottom buttons
// This structure defines the bottom buttons of the main menu
// Each button can be an image or a symbol, and has a callback for press events
static circular_menu_conf_t menu_conf =
    {.num_sections   = 9,
     .sections       = main_menu_sections,
     .bottom_buttons = {
         {MENU_ITEM_IMAGE, {.img_path = &ok_b}, bottom_button_press_cb},           // Button 0
                                                                                   // visible
         {MENU_ITEM_IMAGE, {.img_path = &unlock_b}, bottom_button_lock_press_cb},  // Button 1
                                                                                   // visible
         {MENU_ITEM_IMAGE, {.img_path = &reset_m}, bottom_button_press_cb}  // Button 2 visible
     }};

// Helper functions

// Function to update the firmware status image

static void update_firmware_status_image(firmware_state_t state)
  {
      if (!firmware_status_img) {
          esp3d_log_e("Firmware status image object is null");
          return;
      }

      current_firmware_state = state;
      const lv_image_dsc_t *img_src = nullptr;
      switch (state) {
          case FIRMWARE_IDLE:
              img_src = &idle_s;
              break;
          case FIRMWARE_RUN:
              img_src = &run_s;
              break;
          case FIRMWARE_ALARM:
              img_src = &alarm_s;
              break;
          case FIRMWARE_OTHER:
          default:
              img_src = &status_s;
              break;
      }

      lv_img_set_src(firmware_status_img, img_src);
      lv_obj_invalidate(firmware_status_img); // Invalidate the object to force redraw
      esp3d_log_d("Firmware status updated to %d", state);
  }

  // Function to update the connection status image
  static void update_connection_status_image(bool connection_ok)
  {
      if (!connection_status_img) {
          esp3d_log_e("Connection status image object is null");
          return;
      }

      is_connection_ok = connection_ok;
      lv_obj_set_style_img_recolor(connection_status_img,
                                   connection_ok ? lv_color_hex(0x00FF00) : lv_color_hex(0xFF0000),
                                   LV_PART_MAIN);
      lv_obj_set_style_img_recolor_opa(connection_status_img, LV_OPA_COVER, LV_PART_MAIN);
      lv_obj_invalidate(connection_status_img); // Invalidate the object to force redraw
      esp3d_log_d("Connection status updated to %s", connection_ok ? "OK" : "Fail");
  }

// Helper function to trigger a short beep
static void trigger_button_beep(void)
{
    esp3d_buzzer.set_loud(false);
    esp3d_buzzer.bip(1000, 100);  // Beep of 100 ms
}

// Function to update the icons styles based on the current section
static void update_icon_styles(void)
{
    for (int i = 0; i < menu_data.conf.num_sections; i++)
    {
        if (i == menu_data.current_section)
        {
            if (menu_data.conf.sections[i].type == MENU_ITEM_SYMBOL)
            {
                lv_obj_set_style_text_color(menu_data.icons[i],
                                            lv_color_hex(ESP3D_MENU_ICON_COLOR),
                                            LV_PART_MAIN);
            }
            else
            {
                lv_obj_set_style_img_recolor(menu_data.icons[i],
                                             lv_color_hex(ESP3D_MENU_ICON_COLOR),
                                             LV_PART_MAIN);
                lv_obj_set_style_img_recolor_opa(menu_data.icons[i], LV_OPA_COVER, LV_PART_MAIN);
            }
        }
        else
        {
            if (menu_data.conf.sections[i].type == MENU_ITEM_SYMBOL)
            {
                lv_obj_set_style_text_color(menu_data.icons[i],
                                            lv_color_hex(ESP3D_MENU_ICON_COLOR),
                                            LV_PART_MAIN);
            }
            else
            {
                lv_obj_set_style_img_recolor_opa(menu_data.icons[i], LV_OPA_TRANSP, LV_PART_MAIN);
            }
        }
    }
}

// Function to update center text
static void update_center_text(void)
{
    if (menu_data.center_label && menu_data.conf.sections)
    {
        const char *text = menu_data.conf.sections[menu_data.current_section].center_text.c_str();
        lv_label_set_text(menu_data.center_label, text ? text : "");
        lv_obj_center(menu_data.center_label);
    }
}

static void simulate_click_on_button(uint32_t btn_id)
{
    if (btn_id >= 3 || !menu_data.conf.bottom_buttons[btn_id].on_press)
    {
        esp3d_log_d("Invalid button ID %ld or no callback", btn_id);
        return;
    }

    // Simuler l'appui visuel sur le bouton virtuel
    if (menu_data.bottom_button_labels[btn_id])
    {
        if (menu_data.conf.bottom_buttons[btn_id].type == MENU_ITEM_SYMBOL)
        {
            lv_obj_set_style_text_color(menu_data.bottom_button_labels[btn_id],
                                        lv_color_hex(ESP3D_MENU_SELECTOR_COLOR),
                                        LV_PART_MAIN);
        }
        else
        {
            lv_obj_set_style_img_recolor(menu_data.bottom_button_labels[btn_id],
                                         lv_color_hex(ESP3D_MENU_SELECTOR_COLOR),
                                         LV_PART_MAIN);
            lv_obj_set_style_img_recolor_opa(menu_data.bottom_button_labels[btn_id],
                                             LV_OPA_COVER,
                                             LV_PART_MAIN);
        }
        lv_obj_invalidate(menu_data.bottom_button_labels[btn_id]);  // Forcer le redessin
    }

    // Appeler le callback du bouton virtuel
    esp3d_log_d("Simulating click on bottom button %ld", btn_id);
    menu_data.conf.bottom_buttons[btn_id].on_press(btn_id);
}

// Helper function to simulate a click on the active section from button or touch
static void simulate_click_on_active_section(void)
{
    if (menu_data.conf.sections[menu_data.current_section].type == MENU_ITEM_SYMBOL)
    {
        lv_obj_set_style_text_color(menu_data.icons[menu_data.current_section],
                                    lv_color_hex(ESP3D_MENU_SELECTOR_COLOR),
                                    LV_PART_MAIN);
    }
    else
    {
        lv_obj_set_style_img_recolor(menu_data.icons[menu_data.current_section],
                                     lv_color_hex(ESP3D_MENU_SELECTOR_COLOR),
                                     LV_PART_MAIN);
        lv_obj_set_style_img_recolor_opa(menu_data.icons[menu_data.current_section],
                                         LV_OPA_COVER,
                                         LV_PART_MAIN);
    }
    esp3d_log_d("Click zone pressed: active_section_id=%ld", menu_data.current_section);
    if (menu_data.conf.sections && menu_data.conf.sections[menu_data.current_section].on_press)
    {
        esp3d_log_d("Calling on_press callback for section %ld", menu_data.current_section);
        menu_data.conf.sections[menu_data.current_section].on_press(menu_data.current_section);
    }
    else
    {
        esp3d_log_e("No valid on_press callback for section %ld", menu_data.current_section);
    }
}

// Callback functions
// Deletion callback, to clean up when screen is deleted
static void obj_delete_cb(lv_event_t *e)
{
    lv_obj_t *obj = (lv_obj_t *)lv_event_get_target(e);
    esp3d_log("Object deleted: %p", obj);
    if (obj == menu_data.screen)
    {
        if (menu_data.click_zones)
        {
            free(menu_data.click_zones);
            menu_data.click_zones = nullptr;
        }
        if (menu_data.icons)
        {
            free(menu_data.icons);
            menu_data.icons = nullptr;
        }
        memset(&menu_data, 0, sizeof(circular_menu_data_t));
        esp3d_log("Circular menu screen cleared");
    }
    else
    {
        for (int i = 0; i < 3; i++)
        {
            if (menu_data.bottom_button_labels[i] == obj)
            {
                menu_data.bottom_button_labels[i] = nullptr;
                esp3d_log("Bottom button label %d cleared", i);
            }
        }
    }
}

// Physical Buttons callback
static void button_event_cb(lv_event_t *e)
{
    lv_event_code_t code   = lv_event_get_code(e);
    control_event_t *event = (control_event_t *)lv_event_get_param(e);
    if (!event || event->family_id != CONTROL_FAMILY_BUTTONS)
    {
        return;
    }
    uint32_t btn_id = event->btn_id;
    if (btn_id!=1 && is_locked){
        esp3d_log_d("Button %ld ignored, system is locked", btn_id + 1);
        return;  // Ignore button presses when locked
    }
    if (code == LV_EVENT_PRESSED)
    {
        esp3d_log_d("Button %ld pressed", btn_id + 1);
        trigger_button_beep();
        simulate_click_on_button(btn_id);
        if (btn_id == 0)
        {
            simulate_click_on_active_section();
        }
    }
    else if (code == LV_EVENT_RELEASED)
    {
        uint32_t duration = event->press_duration;

        // Reset the button style
        if (btn_id < 3 && menu_data.bottom_button_labels[btn_id])
        {
            if (menu_data.conf.bottom_buttons[btn_id].type == MENU_ITEM_SYMBOL)
            {
                lv_obj_set_style_text_color(menu_data.bottom_button_labels[btn_id],
                                            lv_color_hex(ESP3D_MENU_ICON_COLOR),
                                            LV_PART_MAIN);
            }
            else
            {
                lv_obj_set_style_img_recolor_opa(menu_data.bottom_button_labels[btn_id],
                                                 LV_OPA_TRANSP,
                                                 LV_PART_MAIN);
            }
            lv_obj_invalidate(menu_data.bottom_button_labels[btn_id]);  // Force redraw
            if (btn_id == 0)
            {
                update_icon_styles();
            }
        }

        if (duration < ESP3D_LONG_PRESS_THRESHOLD_MS)
        {
            esp3d_log_d("Button %ld short press released (duration: %ld ms)", btn_id + 1, duration);
        }
        else
        {
            esp3d_log_d("Button %ld long press released (duration: %ld ms)", btn_id + 1, duration);
        }
    }
}

// Callback for encoder events
static void encoder_event_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    if (is_locked){
        esp3d_log_d("Event is ignored, system is locked");
        return;  // Ignore event when locked
    }
    if (code == LV_EVENT_KEY)
    {
        control_event_t *event = (control_event_t *)lv_event_get_param(e);
        if (event && event->family_id == CONTROL_FAMILY_ENCODER)
        {
            int32_t steps = event->steps;
            esp3d_log_d("Encoder steps received: %ld", steps);
            // Normalize steps to prevent large jumps
            if (steps > 1)
                steps = 1;
            if (steps < -1)
                steps = -1;

            int32_t prev_section = menu_data.current_section;
            (void)prev_section;  // Suppress unused variable warning
            // Update current_section with explicit wrapping
            menu_data.current_section += steps;
            if (menu_data.current_section < 0)
            {
                menu_data.current_section = menu_data.conf.num_sections - 1;  // Go to last section
            }
            else if (menu_data.current_section >= menu_data.conf.num_sections)
            {
                menu_data.current_section = 0;  // Go to first section
            }

            esp3d_log_d("Encoder: steps=%ld, prev_section=%ld, new_section=%ld",
                        steps,
                        prev_section,
                        menu_data.current_section);

            int32_t start_angle =
                menu_data.current_section * (360 / menu_data.conf.num_sections) + 90;
            int32_t end_angle = start_angle + (360 / menu_data.conf.num_sections);
            lv_arc_set_angles(menu_data.arc, start_angle, end_angle);
            lv_arc_set_angles(menu_data.inner_arc, start_angle, end_angle);

            update_icon_styles();
            update_center_text();
        }
    }
}

// Callback for click areas
static void click_zone_event_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    int32_t section      = (int32_t)(intptr_t)lv_event_get_user_data(e);
    if ( is_locked){
        esp3d_log_d("Button %ld ignored, system is locked", section);
        return;  // Ignore button presses when locked
    }
    if (code == LV_EVENT_PRESSED)
    {
        trigger_button_beep();
        if (menu_data.conf.sections[section].type == MENU_ITEM_SYMBOL)
        {
            lv_obj_set_style_text_color(menu_data.icons[section],
                                        lv_color_hex(ESP3D_MENU_SELECTOR_COLOR),
                                        LV_PART_MAIN);
        }
        else
        {
            lv_obj_set_style_img_recolor(menu_data.icons[section],
                                         lv_color_hex(ESP3D_MENU_SELECTOR_COLOR),
                                         LV_PART_MAIN);
            lv_obj_set_style_img_recolor_opa(menu_data.icons[section], LV_OPA_COVER, LV_PART_MAIN);
        }
        menu_data.current_section = section;

        int32_t start_angle = menu_data.current_section * (360 / menu_data.conf.num_sections) + 90;
        int32_t end_angle   = start_angle + (360 / menu_data.conf.num_sections);
        lv_arc_set_angles(menu_data.arc, start_angle, end_angle);
        lv_arc_set_angles(menu_data.inner_arc, start_angle, end_angle);

        esp3d_log_d("Click zone pressed: section=%ld", menu_data.current_section);
        if (menu_data.conf.sections && menu_data.conf.sections[section].on_press)
        {
            esp3d_log_d("Calling on_press callback for section %ld", section);
            menu_data.conf.sections[section].on_press(section);
        }
        else
        {
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
    lv_event_code_t code                = lv_event_get_code(e);
    int32_t button_idx                  = (int32_t)(intptr_t)lv_event_get_user_data(e);
    static uint32_t press_start_time[3] = {0, 0, 0};
     if (button_idx!=1 && is_locked){
        esp3d_log_d("Button %ld ignored, system is locked", button_idx );
        return;  // Ignore button presses when locked
    }
    if (code == LV_EVENT_PRESSED)
    {
        esp3d_log_d("Bottom button pressed: index=%ld", button_idx);
        press_start_time[button_idx] = esp_timer_get_time() / 1000;
        trigger_button_beep();
        if (menu_data.bottom_button_labels[button_idx])
        {
            if (menu_data.conf.bottom_buttons[button_idx].type == MENU_ITEM_SYMBOL)
            {
                lv_obj_set_style_text_color(menu_data.bottom_button_labels[button_idx],
                                            lv_color_hex(ESP3D_MENU_SELECTOR_COLOR),
                                            LV_PART_MAIN);
            }
            else
            {
                lv_obj_set_style_img_recolor(menu_data.bottom_button_labels[button_idx],
                                             lv_color_hex(ESP3D_MENU_SELECTOR_COLOR),
                                             LV_PART_MAIN);
                lv_obj_set_style_img_recolor_opa(menu_data.bottom_button_labels[button_idx],
                                                 LV_OPA_COVER,
                                                 LV_PART_MAIN);
            }
            esp3d_log_d("Bottom button %ld label set to blue", button_idx);
        }
        if (button_idx == 0)
        {
            simulate_click_on_active_section();
        }
        else if (menu_data.conf.bottom_buttons[button_idx].on_press)
        {
            esp3d_log_d("Calling on_press callback for bottom button %ld", button_idx);
            menu_data.conf.bottom_buttons[button_idx].on_press(button_idx);
        }
    }
    else if (code == LV_EVENT_RELEASED || code == LV_EVENT_PRESS_LOST)
    {
        uint32_t duration = (esp_timer_get_time() / 1000) - press_start_time[button_idx];
        if (menu_data.bottom_button_labels[button_idx])
        {
            if (menu_data.conf.bottom_buttons[button_idx].type == MENU_ITEM_SYMBOL)
            {
                lv_obj_set_style_text_color(menu_data.bottom_button_labels[button_idx],
                                            lv_color_hex(ESP3D_MENU_ICON_COLOR),
                                            LV_PART_MAIN);
            }
            else
            {
                lv_obj_set_style_img_recolor_opa(menu_data.bottom_button_labels[button_idx],
                                                 LV_OPA_TRANSP,
                                                 LV_PART_MAIN);
            }
            esp3d_log_d("Bottom button %ld label reset to white", button_idx);
        }
        if (button_idx == 0)
        {
            update_icon_styles();
        }
        if (duration < ESP3D_LONG_PRESS_THRESHOLD_MS)
        {
            esp3d_log_d("Bottom button %ld short press released (duration: %ld ms)",
                        button_idx,
                        duration);
        }
        else
        {
            esp3d_log_d("Bottom button %ld long press released (duration: %ld ms)",
                        button_idx,
                        duration);
        }
    }
}

static void section_press_cb(int32_t section_id)
{
    esp3d_log_d("Menu section %ld pressed", section_id);
}

static void bottom_button_lock_press_cb(int32_t button_idx)
{
    esp3d_log_d("Bottom button %ld pressed", button_idx);
    is_locked = !is_locked;  // Toggle lock state
    if (is_locked)
    {
        esp3d_log_d("Lock button %ld pressed, lock UI", button_idx);
        // Set lock image
        if (menu_data.bottom_button_labels[1])
        {
            lv_img_set_src(menu_data.bottom_button_labels[1], &lock_b);
        }
    }
    else
    {
        esp3d_log_d("Lock button %ld pressed, unlock UI", button_idx);
        // Set unlock image
        if (menu_data.bottom_button_labels[1])
        {
            lv_img_set_src(menu_data.bottom_button_labels[1], &unlock_b);
        }
    }
    // lv_obj_invalidate(menu_data.bottom_button_labels[1]);
}

static void bottom_button_press_cb(int32_t button_idx)
{
    esp3d_log_d("Bottom button %ld pressed", button_idx);
}

void create()
{
    esp3dTftui.set_current_screen(ESP3DScreenType::none);
    // Screen creation
    esp3d_log("Main screen creation");
    lv_obj_t *current_screen = lv_screen_active();
    lv_obj_t *screen         = lv_obj_create(NULL);
    if (!lv_obj_is_valid(screen))
    {
        esp3d_log_e("Failed to create screen object");
        return;
    }
    // Set the screen as the current screen
    lv_screen_load(screen);
    // Delete the previous screen if it exists
    if (lv_obj_is_valid(current_screen))
    {
        esp3d_log_d("Already a screen, deleting it");
        lv_obj_del(current_screen);
    }
    // Apply styles to the screen
    lv_obj_set_style_bg_color(screen, lv_color_hex(ESP3D_SCREEN_BACKGROUND_COLOR), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_scrollbar_mode(screen, LV_SCROLLBAR_MODE_OFF);
    // Create the main menu container
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-enum-enum-conversion"
    // Validate configuration
    if (!menu_conf.sections || menu_conf.num_sections == 0
        || menu_conf.num_sections > ESP3D_MAIN_MENU_MAX_SECTIONS)
    {
        esp3d_log_e("Invalid menu configuration: num_sections=%lu", menu_conf.num_sections);
        return;
    }

    // Create screen if parent is NULL
    menu_data.screen = screen;
    lv_obj_set_style_bg_color(menu_data.screen,
                              lv_color_hex(ESP3D_SCREEN_BACKGROUND_COLOR),
                              LV_PART_MAIN);
    lv_obj_set_style_bg_opa(menu_data.screen, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_scrollbar_mode(menu_data.screen, LV_SCROLLBAR_MODE_OFF);

    lv_obj_add_event_cb(menu_data.screen, obj_delete_cb, LV_EVENT_DELETE, NULL);

    // Store configuration
    menu_data.conf               = menu_conf;
    menu_data.current_section    = ESP3D_MAIN_MENU_INITIAL_SECTION_ID;
    menu_data.allocated_sections = menu_conf.num_sections;

    // Allocate dynamic arrays
    menu_data.click_zones = (lv_obj_t **)malloc(menu_conf.num_sections * sizeof(lv_obj_t *));
    menu_data.icons       = (lv_obj_t **)malloc(menu_conf.num_sections * sizeof(lv_obj_t *));
    if (!menu_data.click_zones || !menu_data.icons)
    {
        esp3d_log_e("Failed to allocate memory for click_zones or icons");
        if (menu_data.click_zones)
            free(menu_data.click_zones);
        if (menu_data.icons)
            free(menu_data.icons);
        lv_obj_delete(menu_data.screen);
        memset(&menu_data, 0, sizeof(circular_menu_data_t));
        return;
    }
    memset(menu_data.click_zones, 0, menu_conf.num_sections * sizeof(lv_obj_t *));
    memset(menu_data.icons, 0, menu_conf.num_sections * sizeof(lv_obj_t *));

    // Initialize bottom button labels
    for (int i = 0; i < 3; i++)
    {
        menu_data.bottom_button_labels[i] = nullptr;
    }

    // Create container for the circular menu
    menu_data.menu = lv_obj_create(menu_data.screen);
    lv_obj_set_style_bg_color(menu_data.menu,
                              lv_color_hex(ESP3D_SCREEN_BACKGROUND_COLOR),
                              LV_PART_MAIN);
    lv_obj_set_style_bg_opa(menu_data.menu, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_size(menu_data.menu, ESP3D_MAIN_MENU_DIAMETER, ESP3D_MAIN_MENU_DIAMETER);
    lv_obj_set_style_border_width(menu_data.menu, 0, LV_PART_MAIN);
    lv_obj_set_style_border_opa(menu_data.menu, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_align(menu_data.menu, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_set_scrollbar_mode(menu_data.menu, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_style_pad_all(menu_data.menu, 0, LV_PART_MAIN);
    lv_obj_set_style_border_width(menu_data.menu, 0, LV_PART_MAIN | LV_STATE_ANY);
    lv_obj_set_style_border_opa(menu_data.menu, LV_OPA_TRANSP, LV_PART_MAIN | LV_STATE_ANY);
    lv_obj_add_event_cb(menu_data.menu, obj_delete_cb, LV_EVENT_DELETE, NULL);


    // Create image for the firmware status (top left)
      // This image will display the current firmware state (Idle, Run, Alarm, etc.)
      firmware_status_img = lv_img_create(menu_data.menu);
      lv_img_set_src(firmware_status_img, &idle_s); // Initial state: Idle
      lv_obj_align(firmware_status_img, LV_ALIGN_TOP_LEFT, 10, 10);
      lv_obj_set_style_img_recolor_opa(firmware_status_img, LV_OPA_TRANSP, LV_PART_MAIN);
      lv_obj_set_scrollbar_mode(firmware_status_img, LV_SCROLLBAR_MODE_OFF);

      // Create image for the connection status (top right)
      // This image will display the current connection status (OK or Fail)
      connection_status_img = lv_img_create(menu_data.menu);
      lv_img_set_src(connection_status_img, &status_s);
      lv_obj_align(connection_status_img, LV_ALIGN_TOP_RIGHT, -10, 10);
      lv_obj_set_style_img_recolor(connection_status_img, lv_color_hex(0xFF0000), LV_PART_MAIN); // Initial color: red (Fail)
      lv_obj_set_style_img_recolor_opa(connection_status_img, LV_OPA_COVER, LV_PART_MAIN);
      lv_obj_set_scrollbar_mode(connection_status_img, LV_SCROLLBAR_MODE_OFF);

    // Create the outer circle (border)
    lv_obj_t *circle = lv_obj_create(menu_data.menu);
    lv_obj_set_size(circle, ESP3D_MAIN_MENU_DIAMETER, ESP3D_MAIN_MENU_DIAMETER);
    lv_obj_center(circle);
    lv_obj_set_style_radius(circle, LV_RADIUS_CIRCLE, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(circle, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_color(circle, lv_color_hex(ESP3D_MENU_BORDER_COLOR), LV_PART_MAIN);
    lv_obj_set_style_border_width(circle, ESP3D_MAIN_MENU_BORDER_WIDTH, LV_PART_MAIN);
    lv_obj_set_style_border_opa(circle, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_scrollbar_mode(circle, LV_SCROLLBAR_MODE_OFF);

    // Create the inner circle (white background)
    lv_obj_t *inner_circle = lv_obj_create(menu_data.menu);
    lv_obj_set_size(inner_circle, ESP3D_MAIN_MENU_INNER_DIAMETER, ESP3D_MAIN_MENU_INNER_DIAMETER);
    lv_obj_center(inner_circle);
    lv_obj_set_style_radius(inner_circle, LV_RADIUS_CIRCLE, LV_PART_MAIN);
    lv_obj_set_style_bg_color(inner_circle, lv_color_hex(ESP3D_MENU_INNER_COLOR), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(inner_circle, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_width(inner_circle, 0, LV_PART_MAIN);
    lv_obj_set_scrollbar_mode(inner_circle, LV_SCROLLBAR_MODE_OFF);

    // Create the center label
    // This label will display the text in the center of the menu
    menu_data.center_label = lv_label_create(inner_circle);
    lv_obj_set_style_text_color(menu_data.center_label,
                                lv_color_hex(ESP3D_MENU_ICON_COLOR),
                                LV_PART_MAIN);
    lv_obj_set_scrollbar_mode(menu_data.center_label, LV_SCROLLBAR_MODE_OFF);
    update_center_text();

    // Create the inner arc (white background)
    // This arc will be used to indicate the current section
    menu_data.inner_arc = lv_arc_create(menu_data.menu);
    lv_obj_set_size(menu_data.inner_arc,
                    ESP3D_MAIN_MENU_DIAMETER - (2 * ESP3D_MAIN_MENU_BORDER_WIDTH),
                    ESP3D_MAIN_MENU_DIAMETER - (2 * ESP3D_MAIN_MENU_BORDER_WIDTH));
    lv_obj_center(menu_data.inner_arc);
    int32_t start_angle = menu_data.current_section * (360 / menu_data.conf.num_sections) + 90;
    int32_t end_angle   = start_angle + (360 / menu_data.conf.num_sections);
    lv_arc_set_angles(menu_data.inner_arc, start_angle, end_angle);
    lv_obj_set_style_arc_width(menu_data.inner_arc,
                               ESP3D_MAIN_MENU_INNER_ARC_WIDTH,
                               LV_PART_INDICATOR);
    lv_obj_set_style_arc_color(menu_data.inner_arc,
                               lv_color_hex(ESP3D_MENU_INNER_COLOR),
                               LV_PART_INDICATOR);
    lv_obj_set_style_arc_opa(menu_data.inner_arc, LV_OPA_COVER, LV_PART_INDICATOR);
    lv_obj_set_style_arc_rounded(menu_data.inner_arc, false, LV_PART_INDICATOR);
    lv_obj_set_style_bg_opa(menu_data.inner_arc, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_arc_set_bg_angles(menu_data.inner_arc, 0, 0);
    lv_obj_set_style_arc_opa(menu_data.inner_arc, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_opa(menu_data.inner_arc, LV_OPA_TRANSP, LV_PART_KNOB);
    lv_obj_set_style_size(menu_data.inner_arc, 0, 0, LV_PART_KNOB);
    lv_obj_set_style_radius(menu_data.inner_arc, 0, LV_PART_KNOB);
    lv_obj_set_scrollbar_mode(menu_data.inner_arc, LV_SCROLLBAR_MODE_OFF);

    // Create the selection arc
    // This arc will be used to indicate the selected section
    menu_data.arc = lv_arc_create(menu_data.menu);
    lv_obj_set_size(menu_data.arc, ESP3D_MAIN_MENU_DIAMETER, ESP3D_MAIN_MENU_DIAMETER);
    lv_obj_center(menu_data.arc);
    lv_arc_set_angles(menu_data.arc, start_angle, end_angle);
    lv_obj_set_style_arc_width(menu_data.arc, ESP3D_MAIN_MENU_BORDER_WIDTH, LV_PART_INDICATOR);
    lv_obj_set_style_arc_color(menu_data.arc,
                               lv_color_hex(ESP3D_MENU_SELECTOR_COLOR),
                               LV_PART_INDICATOR);
    lv_obj_set_style_arc_opa(menu_data.arc, LV_OPA_COVER, LV_PART_INDICATOR);
    lv_obj_set_style_bg_opa(menu_data.arc, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_arc_set_bg_angles(menu_data.arc, 0, 0);
    lv_obj_set_style_arc_opa(menu_data.arc, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_opa(menu_data.arc, LV_OPA_TRANSP, LV_PART_KNOB);
    lv_obj_set_style_size(menu_data.arc, 0, 0, LV_PART_KNOB);
    lv_obj_set_style_radius(menu_data.arc, 0, LV_PART_KNOB);
    lv_obj_set_scrollbar_mode(menu_data.arc, LV_SCROLLBAR_MODE_OFF);

    // Create the clickable zones for each section
    for (int i = 0; i < menu_conf.num_sections; i++)
    {
        lv_obj_t *click_zone     = lv_obj_create(menu_data.menu);
        menu_data.click_zones[i] = click_zone;
        lv_obj_set_size(click_zone,
                        ESP3D_MAIN_MENU_ICON_ZONE_DIAMETER,
                        ESP3D_MAIN_MENU_ICON_ZONE_DIAMETER);
        lv_obj_set_style_radius(click_zone, LV_RADIUS_CIRCLE, LV_PART_MAIN);

        float mid_angle_rad =
            (i * (360.0 / menu_conf.num_sections) + (360.0 / menu_conf.num_sections) / 2 + 90.0)
            * (M_PI / 180.0);
        int32_t radius = (ESP3D_MAIN_MENU_DIAMETER - ESP3D_MAIN_MENU_INNER_DIAMETER) / 4
                         + ESP3D_MAIN_MENU_INNER_DIAMETER / 2;
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
        lv_obj_set_style_bg_grad_color(click_zone,
                                       lv_color_hex(ESP3D_SCREEN_BACKGROUND_COLOR),
                                       LV_PART_MAIN | LV_STATE_PRESSED);
        lv_obj_set_style_bg_main_stop(click_zone, 0, LV_PART_MAIN | LV_STATE_PRESSED);
        lv_obj_set_style_bg_grad_stop(click_zone, 0, LV_PART_MAIN | LV_STATE_PRESSED);

        lv_obj_add_flag(click_zone, LV_OBJ_FLAG_CLICKABLE);

        lv_obj_t *icon;
        if (menu_conf.sections[i].type == MENU_ITEM_SYMBOL)
        {
            icon = lv_label_create(click_zone);
            lv_label_set_text(icon,
                              menu_conf.sections[i].data.symbol ? menu_conf.sections[i].data.symbol
                                                                : "");
            lv_obj_set_style_text_color(icon, lv_color_hex(ESP3D_MENU_ICON_COLOR), LV_PART_MAIN);
        }
        else
        {
            icon = lv_img_create(click_zone);
            lv_img_set_src(icon, menu_conf.sections[i].data.img_path);
            lv_obj_set_style_img_recolor_opa(icon, LV_OPA_TRANSP, LV_PART_MAIN);
        }
        menu_data.icons[i] = icon;
        lv_obj_center(icon);
        lv_obj_set_style_bg_opa(icon,
                                LV_OPA_TRANSP,
                                LV_PART_MAIN | LV_STATE_DEFAULT | LV_STATE_PRESSED);
        lv_obj_set_style_border_opa(icon,
                                    LV_OPA_TRANSP,
                                    LV_PART_MAIN | LV_STATE_DEFAULT | LV_STATE_PRESSED);
        lv_obj_set_style_shadow_opa(icon,
                                    LV_OPA_TRANSP,
                                    LV_PART_MAIN | LV_STATE_DEFAULT | LV_STATE_PRESSED);
        lv_obj_set_style_outline_opa(icon,
                                     LV_OPA_TRANSP,
                                     LV_PART_MAIN | LV_STATE_DEFAULT | LV_STATE_PRESSED);
        lv_obj_set_style_border_width(icon, 0, LV_PART_MAIN | LV_STATE_DEFAULT | LV_STATE_PRESSED);
        lv_obj_set_style_pad_all(icon, 0, LV_PART_MAIN | LV_STATE_DEFAULT | LV_STATE_PRESSED);
        lv_obj_set_scrollbar_mode(icon, LV_SCROLLBAR_MODE_OFF);

        lv_obj_add_event_cb(click_zone, click_zone_event_cb, LV_EVENT_PRESSED, (void *)(intptr_t)i);
        lv_obj_add_event_cb(click_zone,
                            click_zone_event_cb,
                            LV_EVENT_RELEASED,
                            (void *)(intptr_t)i);
        lv_obj_add_event_cb(click_zone,
                            click_zone_event_cb,
                            LV_EVENT_PRESS_LOST,
                            (void *)(intptr_t)i);
        lv_obj_add_event_cb(click_zone, obj_delete_cb, LV_EVENT_DELETE, NULL);
        lv_obj_add_event_cb(icon, obj_delete_cb, LV_EVENT_DELETE, NULL);
    }

    // Create the three bottom buttons
    for (int i = 0; i < 3; i++)
    {
        if (!menu_conf.bottom_buttons[i].on_press)
        {
            esp3d_log_d("Bottom button %d hidden (no callback)", i);
            continue;
        }

        lv_obj_t *button = lv_btn_create(menu_data.screen);
        lv_obj_set_size(button, ESP3D_BOTTOM_BUTTON_SIZE, ESP3D_BOTTOM_BUTTON_SIZE);
        lv_obj_set_style_radius(button, LV_RADIUS_CIRCLE, LV_PART_MAIN);
        lv_obj_set_style_bg_color(button,
                                  lv_color_hex(ESP3D_SCREEN_BACKGROUND_COLOR),
                                  LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_opa(button, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_border_color(button,
                                      lv_color_hex(ESP3D_MENU_BORDER_COLOR),
                                      LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_border_width(button,
                                      ESP3D_MAIN_MENU_BORDER_WIDTH,
                                      LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_border_opa(button, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_shadow_opa(button, LV_OPA_TRANSP, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_color(button,
                                  lv_color_hex(ESP3D_SCREEN_BACKGROUND_COLOR),
                                  LV_PART_MAIN | LV_STATE_PRESSED);
        lv_obj_set_style_bg_opa(button, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_PRESSED);
        lv_obj_set_style_border_color(button,
                                      lv_color_hex(ESP3D_MENU_SELECTOR_COLOR),
                                      LV_PART_MAIN | LV_STATE_PRESSED);
        lv_obj_set_style_border_width(button,
                                      ESP3D_MAIN_MENU_BORDER_WIDTH,
                                      LV_PART_MAIN | LV_STATE_PRESSED);
        lv_obj_set_style_border_opa(button, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_PRESSED);
        lv_obj_set_style_shadow_opa(button, LV_OPA_TRANSP, LV_PART_MAIN | LV_STATE_PRESSED);
        lv_obj_set_scrollbar_mode(button, LV_SCROLLBAR_MODE_OFF);

        int32_t offset_x = (i - 1) * (ESP3D_BOTTOM_BUTTON_SIZE + ESP3D_BOTTOM_BUTTON_SPACING);
        lv_obj_align(button, LV_ALIGN_BOTTOM_MID, offset_x, -10);

        lv_obj_t *icon;
        if (menu_conf.bottom_buttons[i].type == MENU_ITEM_SYMBOL)
        {
            icon = lv_label_create(button);
            lv_label_set_text(icon,
                              menu_conf.bottom_buttons[i].data.symbol
                                  ? menu_conf.bottom_buttons[i].data.symbol
                                  : "");
            lv_obj_set_style_text_color(icon,
                                        lv_color_hex(ESP3D_MENU_ICON_COLOR),
                                        LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_color(icon,
                                        lv_color_hex(ESP3D_MENU_SELECTOR_COLOR),
                                        LV_PART_MAIN | LV_STATE_PRESSED);
        }
        else
        {
            icon = lv_img_create(button);
            lv_img_set_src(icon, menu_conf.bottom_buttons[i].data.img_path);
            lv_obj_set_style_img_recolor_opa(icon, LV_OPA_TRANSP, LV_PART_MAIN);
        }
        menu_data.bottom_button_labels[i] = icon;
        lv_obj_center(icon);
        lv_obj_set_style_text_opa(icon,
                                  LV_OPA_COVER,
                                  LV_PART_MAIN | LV_STATE_DEFAULT | LV_STATE_PRESSED);
        lv_obj_set_style_bg_opa(icon, LV_OPA_TRANSP, LV_PART_MAIN);
        lv_obj_set_style_border_opa(icon, LV_OPA_TRANSP, LV_PART_MAIN);
        lv_obj_set_style_shadow_opa(icon, LV_OPA_TRANSP, LV_PART_MAIN);
        lv_obj_set_style_outline_opa(icon, LV_OPA_TRANSP, LV_PART_MAIN);
        lv_obj_set_style_border_width(icon, 0, LV_PART_MAIN);
        lv_obj_set_style_pad_all(icon, 0, LV_PART_MAIN);
        lv_obj_set_scrollbar_mode(icon, LV_SCROLLBAR_MODE_OFF);

        lv_obj_add_event_cb(button, bottom_button_event_cb, LV_EVENT_PRESSED, (void *)(intptr_t)i);
        lv_obj_add_event_cb(button, bottom_button_event_cb, LV_EVENT_RELEASED, (void *)(intptr_t)i);
        lv_obj_add_event_cb(button,
                            bottom_button_event_cb,
                            LV_EVENT_PRESS_LOST,
                            (void *)(intptr_t)i);
        lv_obj_add_event_cb(button, obj_delete_cb, LV_EVENT_DELETE, NULL);
        lv_obj_add_event_cb(icon, obj_delete_cb, LV_EVENT_DELETE, NULL);

        esp3d_log_d("Bottom button %d created with label reference", i);
    }
    update_firmware_status_image(FIRMWARE_IDLE); // Initial state: Idle
    update_connection_status_image(false); // Initial state: Not connected
    update_icon_styles();
    lv_obj_add_event_cb(menu_data.screen, encoder_event_cb, LV_EVENT_KEY, NULL);
    lv_obj_add_event_cb(menu_data.screen, button_event_cb, LV_EVENT_PRESSED, NULL);
    lv_obj_add_event_cb(menu_data.screen, button_event_cb, LV_EVENT_RELEASED, NULL);

#pragma GCC diagnostic pop
    // Create the main menu
    esp3dTftui.set_current_screen(ESP3DScreenType::main);
}
}  // namespace mainScreen
