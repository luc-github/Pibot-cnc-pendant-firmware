/*
  esp3d_tft_main_ui.cpp
  Copyright (c) 2022 Luc Lebosse. All rights reserved.
  Licensed under GNU Lesser General Public License v2.1 or later.
*/

#include <stdlib.h>

#include "disp_backlight.h"
#include "board_init.h"
#include "esp3d_hal.h"
#include "esp3d_log.h"
#include <lvgl.h>
#include "screens/splash_screen.h"


// Create the LVGL application UI
// This function initializes the LVGL UI and creates the application UI
void create_application(void)
{
    esp3d_log_d("Creating LVGL application UI");

    lv_display_t *display = get_lvgl_display();
    if (!display)
    {
        esp3d_log_e("LVGL display not initialized");
        return;
    }

    // Display the splash screen
    splashScreen::create();

    esp3d_log_d("LVGL application UI created");
    // Set up a timer to turn on the screen after a delay
    lv_timer_t *screen_on_delay_timer = lv_timer_create(
        [](lv_timer_t *timer) {
            lv_timer_delete(timer);  // delete the timer to prevent it from running again
            backlight_set(90);       // Set backlight to 90% brightness
        },
        50,
        NULL);
    if (!screen_on_delay_timer)
    {
        esp3d_log_e("Failed to create screen on delay timer");
    }
}
