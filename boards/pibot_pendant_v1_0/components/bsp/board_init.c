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
#include "driver/gpio.h"
#include "driver/spi_common.h"
#include "disp_backlight.h"
#include "backlight_def.h"
#include "disp_ili9341_spi.h"
#include "ili9341_def.h"
#include "touch_ft6336u.h"
#include "touch_ft6336u_def.h"

// Required includes for LVGL
#include "lvgl.h"
#include "esp_timer.h"
#include "esp_heap_caps.h"
#include <sys/lock.h>

#if ESP3D_DISPLAY_FEATURE

// Global variables for LVGL
static _lock_t lvgl_api_lock;
static lv_display_t *lvgl_display = NULL;
static lv_color16_t *lvgl_buf1 = NULL;
static lv_color16_t *lvgl_buf2 = NULL;
static esp_timer_handle_t lvgl_tick_timer = NULL;
static lv_indev_t *touch_indev = NULL;

// Callback function to notify LVGL when flush is complete
static bool notify_lvgl_flush_ready(esp_lcd_panel_io_handle_t panel_io, esp_lcd_panel_io_event_data_t *edata, void *user_ctx)
{
    lv_display_t *disp = (lv_display_t *)user_ctx;
    lv_display_flush_ready(disp);
    return false;
}

// LVGL flush callback function
static void lvgl_flush_cb(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map)
{
    esp_lcd_panel_handle_t panel_handle = lv_display_get_user_data(disp);
    int offsetx1 = area->x1;
    int offsetx2 = area->x2;
    int offsety1 = area->y1;
    int offsety2 = area->y2;
    
    // Swap R and B bytes if needed
    if (DISPLAY_SWAP_COLOR_FLAG) {
        lv_draw_sw_rgb565_swap(px_map, (offsetx2 + 1 - offsetx1) * (offsety2 + 1 - offsety1));
    }
    
    // Send data to the display
    esp_lcd_panel_draw_bitmap(panel_handle, offsetx1, offsety1, offsetx2 + 1, offsety2 + 1, px_map);
}

// LVGL touch input read callback
static void touch_read_cb(lv_indev_t *indev, lv_indev_data_t *data)
{
    touch_ft6336u_data_t touch_data = touch_ft6336u_read();
    
    if (touch_data.is_pressed) {
        data->state = LV_INDEV_STATE_PRESSED;
        data->point.x = touch_data.x;
        data->point.y = touch_data.y;
        esp3d_log("Touch detected at (%d, %d)", touch_data.x, touch_data.y);
    } else {
        data->state = LV_INDEV_STATE_RELEASED;
    }
}

// LVGL tick timer callback function
static void increase_lvgl_tick(void *arg)
{
    lv_tick_inc(LVGL_TICK_PERIOD_MS);
}

// Initialize touch controller
static esp_err_t init_touch_controller(void)
{
    esp3d_log("Initializing touch controller");
    
    // Initialize the touch controller
    esp_err_t ret = touch_ft6336u_configure(&touch_ft6336u_default_config);
    if (ret != ESP_OK) {
        esp3d_log_e("Touch controller initialization failed: %d", ret);
        return ret;
    }
    
    esp3d_log("Touch controller initialized successfully");
    return ESP_OK;
}

// Initialize LVGL
static esp_err_t init_lvgl(void)
{
    esp_err_t ret = ESP_OK;
    
    esp3d_log("Initializing LVGL v%d.%d.%d", lv_version_major(), lv_version_minor(), lv_version_patch());
    
    // Initialize LVGL library
    lv_init();
    
    // Get panel and IO handles
    esp_lcd_panel_handle_t panel_handle = ili9341_spi_get_panel_handle();
    if (!panel_handle) {
        esp3d_log_e("ILI9341 panel not initialized");
        return ESP_FAIL;
    }
    
    esp_lcd_panel_io_handle_t io_handle = ili9341_spi_get_io_handle();
    if (!io_handle) {
        esp3d_log_e("ILI9341 IO not initialized");
        return ESP_FAIL;
    }
    
    // Create LVGL display
    lvgl_display = lv_display_create(DISPLAY_WIDTH_PX, DISPLAY_HEIGHT_PX);
    if (!lvgl_display) {
        esp3d_log_e("LVGL display creation failed");
        return ESP_FAIL;
    }
    
    // Allocate drawing buffers
    size_t draw_buf_size = DISPLAY_WIDTH_PX * DISPLAY_BUFFER_LINES_NB * sizeof(lv_color16_t);
    
    lvgl_buf1 = heap_caps_malloc(draw_buf_size, MALLOC_CAP_DMA);
    if (!lvgl_buf1) {
        esp3d_log_e("Failed to allocate draw buffer 1");
        return ESP_ERR_NO_MEM;
    }
    
#if DISPLAY_USE_DOUBLE_BUFFER_FLAG
    lvgl_buf2 = heap_caps_malloc(draw_buf_size, MALLOC_CAP_DMA);
    if (!lvgl_buf2) {
        esp3d_log_e("Failed to allocate draw buffer 2");
        free(lvgl_buf1);
        return ESP_ERR_NO_MEM;
    }
    esp3d_log("LVGL configured with double buffer");
#else
    lvgl_buf2 = NULL;
    esp3d_log("LVGL configured with single buffer");
#endif
    
    // Configure LVGL drawing buffers
    lv_display_set_buffers(lvgl_display, lvgl_buf1, lvgl_buf2, draw_buf_size, LV_DISPLAY_RENDER_MODE_PARTIAL);
    
    // Associate the panel with the display
    lv_display_set_user_data(lvgl_display, panel_handle);
    
    // Set color format
    lv_display_set_color_format(lvgl_display, LV_COLOR_FORMAT_RGB565);
    
    // Set flush callback
    lv_display_set_flush_cb(lvgl_display, lvgl_flush_cb);
    
    // Configure LVGL tick timer
    const esp_timer_create_args_t lvgl_tick_timer_args = {
        .callback = &increase_lvgl_tick,
        .name = "lvgl_tick"
    };
    
    ret = esp_timer_create(&lvgl_tick_timer_args, &lvgl_tick_timer);
    if (ret != ESP_OK) {
        esp3d_log_e("LVGL tick timer creation failed");
        return ret;
    }
    
    ret = esp_timer_start_periodic(lvgl_tick_timer, LVGL_TICK_PERIOD_MS * 1000);
    if (ret != ESP_OK) {
        esp3d_log_e("LVGL tick timer start failed");
        return ret;
    }
    
    // Register callbacks for color transfer completion notification
    const esp_lcd_panel_io_callbacks_t cbs = {
        .on_color_trans_done = notify_lvgl_flush_ready,
    };
    
    // Register the callbacks
    ret = esp_lcd_panel_io_register_event_callbacks(io_handle, &cbs, lvgl_display);
    if (ret != ESP_OK) {
        esp3d_log_e("Register IO callbacks failed");
        return ret;
    }
    
    // Initialize touch input device for LVGL
    touch_indev = lv_indev_create();
    lv_indev_set_type(touch_indev, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(touch_indev, touch_read_cb);
    lv_indev_set_display(touch_indev, lvgl_display);
    
    esp3d_log("LVGL initialized successfully");
    return ESP_OK;
}
#endif // ESP3D_DISPLAY_FEATURE

// Access functions for ESP3DTftUi
lv_display_t* get_lvgl_display(void)
{
#if ESP3D_DISPLAY_FEATURE
    return lvgl_display;
#else
    return NULL;
#endif
}

_lock_t* get_lvgl_lock(void)
{
#if ESP3D_DISPLAY_FEATURE
    return &lvgl_api_lock;
#else
    return NULL;
#endif
}

// Initialize board hardware and subsystems
esp_err_t board_init(void)
{
    esp_err_t ret = ESP_OK;
    
    esp3d_log("Initializing %s %s", BOARD_NAME_STR, BOARD_VERSION_STR);
 
#if ESP3D_DISPLAY_FEATURE   
    // Initialize backlight
    ret = backlight_configure(&backlight_cfg);
    if (ret != ESP_OK) {
        esp3d_log_e("Backlight initialization failed");
        return ret;
    }
    backlight_set(0);
    
    // Initialize display
    ret = ili9341_spi_configure(&ili9341_default_config);
    if (ret != ESP_OK) {
        esp3d_log_e("ILI9341 display initialization failed");
        return ret;
    }
    
    // Initialize touch controller
    ret = init_touch_controller();
    if (ret != ESP_OK) {
        esp3d_log_e("Touch controller initialization failed");
        // Continue even if touch controller fails
    }
    
    // Initialize LVGL
    ret = init_lvgl();
    if (ret != ESP_OK) {
        esp3d_log_e("LVGL initialization failed");
        return ret;
    }
    
    //backlight_set(100);
#endif  // ESP3D_DISPLAY_FEATURE
    
    esp3d_log("Board initialization completed successfully");
    return ret;
}

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
