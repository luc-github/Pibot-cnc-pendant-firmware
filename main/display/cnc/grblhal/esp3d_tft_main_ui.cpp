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
#include "esp3d_log.h"
//#include "esp3d_styles.h"
#include "esp3d_version.h"
#include "board_config.h"
#include "board_init.h"
//#include "screens/splash_screen.h"
#include "lvgl.h"

// Rotation for the display
static lv_display_rotation_t current_rotation = LV_DISPLAY_ROTATION_0;

// Label to display touch coordinates
static lv_obj_t *touch_coord_label = NULL;

// Callback for the rotation button
static void btn_rotate_event_cb(lv_event_t *e)
{
    lv_display_t *disp = (lv_display_t *)lv_event_get_user_data(e);
    current_rotation = (lv_display_rotation_t)((current_rotation + 1) % 4);
    lv_display_set_rotation(disp, current_rotation);
}

// Function to set the angle of an arc (for animation)
static void set_arc_angle(void *obj, int32_t v)
{
    // Cast void* to lv_obj_t* to fix the type error
    lv_arc_set_value((lv_obj_t *)obj, v);
}

// Touch event callback to update coordinates display
static void screen_touch_event_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *label = (lv_obj_t *)lv_event_get_user_data(e);
    
    if (code == LV_EVENT_PRESSED || code == LV_EVENT_PRESSING) {
        lv_point_t point;
        lv_indev_t *indev = lv_indev_get_act();
        if (indev != NULL) {
            lv_indev_get_point(indev, &point);
            // Correction: utilisez %ld au lieu de %d pour int32_t
            lv_label_set_text_fmt(label, "Touch: %ld,%ld", (long)point.x, (long)point.y);
        }
    } else if (code == LV_EVENT_RELEASED) {
        lv_label_set_text(label, "Touch: ---,---");
    }
}

// Create the user interface
void create_application(void) {
    esp3d_log("Creating LVGL application UI");
    
    // Initialize styles if you have them
    // ESP3DStyle::init();
    
    // Get the LVGL display
    lv_display_t *display = get_lvgl_display();
    if (!display) {
        esp3d_log_e("LVGL display not initialized");
        return;
    }
    
    // Get the active screen
    lv_obj_t *screen = lv_display_get_screen_active(display);
    
    // Create splash screen
    // splashScreen::create();
    
    // Create a title label
    lv_obj_t *title_label = lv_label_create(screen);
    lv_label_set_text_fmt(title_label, "%s %s\nESP3D v%s", 
                      BOARD_NAME_STR, BOARD_VERSION_STR, 
                      ESP3D_TFT_VERSION);
    lv_obj_align(title_label, LV_ALIGN_TOP_MID, 0, 20);
    

    
    // Create a label to display touch coordinates
    touch_coord_label = lv_label_create(screen);
    lv_label_set_text(touch_coord_label, "Touch: ---,---");
    lv_obj_align(touch_coord_label, LV_ALIGN_TOP_MID, 0, 60);
    
    // Add touch event handler to the screen
    lv_obj_add_event_cb(screen, screen_touch_event_cb, LV_EVENT_ALL, touch_coord_label);
    
    
    // Create an animated arc
    lv_obj_t *arc = lv_arc_create(screen);
    lv_arc_set_rotation(arc, 270);
    lv_arc_set_bg_angles(arc, 0, 360);
    lv_obj_remove_style(arc, NULL, LV_PART_KNOB);
    lv_obj_remove_flag(arc, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_center(arc);
    
    // Create animation
    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, arc);
    lv_anim_set_exec_cb(&a, set_arc_angle);
    lv_anim_set_duration(&a, 1000);
    lv_anim_set_repeat_count(&a, LV_ANIM_REPEAT_INFINITE);
    lv_anim_set_repeat_delay(&a, 500);
    lv_anim_set_values(&a, 0, 100);
    lv_anim_start(&a);
    
    // Create a calibration button
    lv_obj_t *calib_btn = lv_button_create(screen);
    lv_obj_t *calib_label = lv_label_create(calib_btn);
    lv_label_set_text_static(calib_label, "Touch Test");
    lv_obj_center(calib_label);
    lv_obj_align(calib_btn, LV_ALIGN_BOTTOM_RIGHT, -30, -30);
    
    // Add a simple event demonstration for the calibration button
    lv_obj_add_event_cb(calib_btn, [](lv_event_t *e) {
        // Correction: Cast explicite de void* à lv_obj_t*
        lv_obj_t *btn = (lv_obj_t *)lv_event_get_target(e);
        static uint32_t counter = 0;
        counter++;
        
        lv_obj_t *label = lv_obj_get_child(btn, 0);
        lv_label_set_text_fmt(label, "Pressed %lu", counter);
        
        esp3d_log("Calibration button pressed %lu times", counter);
    }, LV_EVENT_CLICKED, NULL);
    
    esp3d_log("LVGL application UI created");
}