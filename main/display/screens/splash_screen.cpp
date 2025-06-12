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

#include "screens/splash_screen.h"

#include <lvgl.h>

#include "esp3d_log.h"
#include "esp3d_styles.h"
#include "esp3d_tft_ui.h"
#include "screens/main_screen.h"

// Images are stored in the flash memory
extern const lv_image_dsc_t logo;

// namespace for the splash screen
namespace splashScreen {

void create()
{
    esp3dTftui.set_current_screen(ESP3DScreenType::none);
    esp3d_log_d("Create splash screen");
    if (ESP3D_SPLASH_DELAY == 0)
    {
        esp3d_log_d("Splash screen disabled, switching to main screen immediately");
        mainScreen::create();
        return;
    }
    lv_obj_t *current_screen = lv_screen_active();
    lv_obj_t *screen         = lv_obj_create(NULL);
    if (!lv_obj_is_valid(screen))
    {
        esp3d_log_e("Failed to create screen object");
        return;
    }
    // Set the screen as the current screen
    lv_screen_load(screen);
    // delete the previous screen if it exists
    if (lv_obj_is_valid(current_screen))
    {
        esp3d_log_d("already a screen, deleting it");
        lv_obj_del(current_screen);
    }
    // create image on screen
    lv_obj_t *img = lv_image_create(screen);
    // apply styles to the screen
    lv_obj_set_style_bg_color(screen, lv_color_hex(ESP3D_SCREEN_BACKGROUND_COLOR), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_scrollbar_mode(screen, LV_SCROLLBAR_MODE_OFF);
    // load the logo image
    lv_image_set_src(img, &logo);
    lv_obj_align(img, LV_ALIGN_CENTER, 0, 0);

    esp3dTftui.set_current_screen(ESP3DScreenType::splash);
    esp3d_log_d("Splash screen created");

    // set a timer to switch to the circular menu after 2 seconds
    lv_timer_create(
        [](lv_timer_t *timer) {
            lv_timer_delete(timer);  // delete the timer to prevent it from running again
            mainScreen::create();    // Switch to the main menu
        },
        2000,
        screen);
}

}  // namespace splashScreen
