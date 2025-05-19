/*
  board_init.c - PiBot CNC Pendant hardware initialization

  Copyright (c) 2025 Luc LEBOSSE. All rights reserved.

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

#include "board_init.h"
#include "board_config.h"
#include "esp3d_log.h"

// Include drivers and libraries

#if ESP3D_DISPLAY_FEATURE
#include "driver/gpio.h"
#include "driver/spi_common.h"
#include "disp_backlight.h"
#include "backlight_def.h"
#include "disp_ili9341_spi.h"
#include "ili9341_def.h"
//#include "lvgl.h"


// LVGL display handle
///static lv_disp_t* disp = NULL;

// Forward declarations for internal functions
//static esp_err_t init_lvgl(void);
#endif  // ESP3D_DISPLAY_FEATURE

// Initialize board hardware and subsystems
esp_err_t board_init(void)
{
    esp_err_t ret = ESP_OK;
    
    esp3d_log("Initializing %s %s", BOARD_NAME_STR, BOARD_VERSION_STR);
 
#if ESP3D_DISPLAY_FEATURE   

    // Initialize display
    // Initialize backlight
    ret = backlight_configure(&backlight_cfg);
    if (ret != ESP_OK) {
        esp3d_log_e("Backlight initialization failed");
        return ret;
    }
    backlight_set(0);
    // Initialize display
    ret = ili9341_spi_configure(&ili9341_default_config);
    

    backlight_set(100);
#endif  // ESP3D_DISPLAY_FEATURE
    
    esp3d_log("Board initialization completed successfully");
    return ret;
}

#if ESP3D_DISPLAY_FEATURE

// Initialize LVGL
/*static esp_err_t init_lvgl(void)
{
    esp3d_log("Initializing LVGL");
 
    esp3d_log("LVGL initialized successfully");
    return ESP_OK;
}*/
#endif // ESP3D_DISPLAY_FEATURE

// Get board name
const char* board_get_name(void)
{
    return BOARD_NAME_STR;
}

// Get board version
const char* board_get_version(void)
{
    return BOARD_VERSION_STR;
}
