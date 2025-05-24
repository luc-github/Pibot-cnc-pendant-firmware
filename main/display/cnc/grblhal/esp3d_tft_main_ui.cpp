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
#include "disp_backlight.h"
lv_timer_t *screen_on_delay_timer = nullptr;

void screen_on_delay_timer_cb(lv_timer_t *timer) {
 
      lv_timer_del(screen_on_delay_timer);
      backlight_set(100);
  }

// Label to display touch coordinates
static lv_obj_t *touch_coord_label = NULL;


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

static void button_event_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_indev_t *indev = lv_event_get_indev(e);
    lv_indev_type_t type = lv_indev_get_type( indev);

    if (code == LV_EVENT_PRESSED && type == LV_INDEV_TYPE_BUTTON) {
        uint32_t lkey = lv_indev_get_key(indev);
       esp3d_log_d("button key: %ld", lkey);
         lv_point_t point;
        lv_indev_get_point(indev, &point);
        uint32_t btn_id =point.x;
            switch (btn_id) {
                case 0: esp3d_log_d("Button 1"); break;
                case 1: esp3d_log_d("Button 2"); break;
                case 2: esp3d_log_d("Button 3"); break;
            }
        
    }
}


static void encoder_event_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_indev_t *indev = lv_event_get_indev(e);
    if (code == LV_EVENT_KEY && lv_indev_get_type(indev) == LV_INDEV_TYPE_ENCODER) {
      /*  int32_t steps;
        if (phy_encoder_read(&steps) == ESP_OK && steps != 0) {
            esp3d_log_d("Encoder event: %ld steps", steps);
            lv_obj_t *label = lv_event_get_user_data(e);
            static int value = 0;
            value += steps;
            lv_label_set_text_fmt(label, "Value: %d", value);
        }*/
       esp3d_log_d("Encoder event");
        
    }
}

static void switch_event_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_indev_t *indev = lv_event_get_indev(e);
    if (code == LV_EVENT_PRESSED && lv_indev_get_type(indev) == LV_INDEV_TYPE_KEYPAD) {
        uint32_t key = lv_indev_get_key(indev);
        esp3d_log_d("Switch key: %ld", key);
         lv_obj_t *label = (lv_obj_t *)lv_event_get_user_data(e);
        switch (key) {
            case 0x31: lv_label_set_text(label, "Switch: Position 1"); break;
            case 0x32: lv_label_set_text(label, "Switch: Position 2"); break;
            case 0x33: lv_label_set_text(label, "Switch: Position 3"); break;
            case 0x34: lv_label_set_text(label, "Switch: Position 4"); break;
            default: lv_label_set_text(label, "Switch: Unknown"); break;
        }
    }
}

// Create the user interface
void create_application(void) {
    esp3d_log("Creating LVGL application UI");

  

    //lvgl_fs_init(); // Initialize the filesystem driver for LVGL
    lv_fs_posix_init(); // Initialize the filesystem driver for LVGL
    
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

    // Test screen display
    lv_obj_t *test_label = lv_label_create(screen);
    lv_label_set_text(test_label, "Test Screen");
    lv_obj_align(test_label, LV_ALIGN_TOP_LEFT, 10, 10);
    lv_obj_set_style_bg_color(screen, lv_color_black(), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, LV_PART_MAIN);

     // Définir le texte blanc pour le label
    lv_obj_set_style_text_color(test_label, lv_color_white(), LV_PART_MAIN);

    // Create three 40x40 colored squares (Red, Green, Blue)
    // Red square
    lv_obj_t *red_square = lv_obj_create(screen);
    lv_obj_set_size(red_square, 40, 40);
    lv_obj_align(red_square, LV_ALIGN_CENTER, -50, 0); // Position to the left
    lv_obj_set_style_bg_color(red_square, lv_color_make(255, 0, 0), LV_PART_MAIN); // Pure red
    lv_obj_set_style_bg_opa(red_square, LV_OPA_COVER, LV_PART_MAIN);
    esp3d_log("Red square created");

    // Green square
    lv_obj_t *green_square = lv_obj_create(screen);
    lv_obj_set_size(green_square, 40, 40);
    lv_obj_align(green_square, LV_ALIGN_CENTER, 0, 0); // Center
    lv_obj_set_style_bg_color(green_square, lv_color_make(0, 255, 0), LV_PART_MAIN); // Pure green
    lv_obj_set_style_bg_opa(green_square, LV_OPA_COVER, LV_PART_MAIN);
    esp3d_log("Green square created");

    // Blue square
    lv_obj_t *blue_square = lv_obj_create(screen);
    lv_obj_set_size(blue_square, 40, 40);
    lv_obj_align(blue_square, LV_ALIGN_CENTER, 50, 0); // Position to the right
    lv_obj_set_style_bg_color(blue_square, lv_color_make(0, 0, 255), LV_PART_MAIN); // Pure blue
    lv_obj_set_style_bg_opa(blue_square, LV_OPA_COVER, LV_PART_MAIN);
    esp3d_log("Blue square created");
    
    // Create splash screen
    // splashScreen::create();

    

        
    lv_fs_file_t file;
     const char* img_path = "L:/hometp.png";
    lv_fs_res_t res = lv_fs_open(&file, img_path, LV_FS_MODE_RD);

    if(res != LV_FS_RES_OK) {
        esp3d_log_e("Impossible d'ouvrir l'image: %d", res);
    } else {
        esp3d_log("Image trouvée");
        lv_fs_close(&file);
    }

    // Create a title label
    lv_obj_t *title_label = lv_label_create(screen);
    lv_label_set_text_fmt(title_label, "%s %s\nESP3D v%s", 
                      BOARD_NAME_STR, BOARD_VERSION_STR, 
                      ESP3D_TFT_VERSION);
    lv_obj_align(title_label, LV_ALIGN_TOP_MID, 0, 20);
    lv_obj_set_style_text_color(title_label, lv_color_white(), LV_PART_MAIN);

    
    // Create a label to display touch coordinates
    touch_coord_label = lv_label_create(screen);
    lv_label_set_text(touch_coord_label, "Touch: ---,---");
    lv_obj_align(touch_coord_label, LV_ALIGN_TOP_MID, 0, 60);
    lv_obj_set_style_text_color(touch_coord_label, lv_color_white(), LV_PART_MAIN);

    lv_group_t *group = lv_group_get_default();
    if (!group) {
        group = lv_group_create();
        lv_group_set_default(group);
    }
    lv_group_add_obj(group, touch_coord_label);
    
    // Add touch event handler to the screen
    lv_obj_add_event_cb(screen, screen_touch_event_cb, LV_EVENT_ALL, touch_coord_label);
    lv_obj_add_event_cb(screen, button_event_cb, LV_EVENT_PRESSED, NULL);
    lv_obj_add_event_cb(screen, encoder_event_cb, LV_EVENT_KEY, touch_coord_label);
    lv_obj_add_event_cb(screen, switch_event_cb, LV_EVENT_PRESSED, touch_coord_label);
    
  
    
    // Création de l'objet image
    lv_obj_t* img = lv_img_create(screen);
    
    // Chargement de l'image depuis LittleFS
    lv_img_set_src(img, img_path);
if (lv_img_get_src(img) == NULL) {
    esp3d_log_e("Failed to load image: %s", img_path);
} else {
    esp3d_log("Image loaded: %s", img_path);
}
    // Positionnement de l'image au centre de l'écran
    lv_obj_align(img, LV_ALIGN_CENTER, 0, 0);
    
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
        backlight_set(100); 
    }, LV_EVENT_CLICKED, NULL);
    
    esp3d_log("LVGL application UI created");
    screen_on_delay_timer =
        lv_timer_create(screen_on_delay_timer_cb, 50, NULL);
}