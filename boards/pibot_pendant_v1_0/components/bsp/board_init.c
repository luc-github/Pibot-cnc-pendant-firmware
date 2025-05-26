/*
  board_init.c - PiBot CNC Pendant hardware initialization
  Copyright (c) 2025 Luc LEBOSSE. All rights reserved.
  Licensed under GNU Lesser General Public License v2.1 or later.
*/

#include "board_init.h"
#include "board_config.h"
#include "esp3d_log.h"
#include "control_event.h"

// Include drivers and libraries
#include "disp_backlight.h"
#include "disp_backlight_def.h"
#include "disp_ili9341_spi.h"
#include "disp_ili9341_spi_def.h"
#include "buzzer.h"
#include "buzzer_def.h"
#include "driver/gpio.h"
#include "driver/spi_common.h"
#include "phy_buttons.h"
#include "phy_buttons_def.h"
#include "touch_ft6336u.h"
#include "touch_ft6336u_def.h"
#include "phy_encoder.h"
#include "phy_encoder_def.h"
#include "phy_switch.h"
#include "phy_switch_def.h"
#include "phy_potentiometer.h"
#include "phy_potentiometer_def.h"

// Required includes for LVGL
#include <sys/lock.h>
#include "esp_heap_caps.h"
#include "esp_timer.h"
#include "lvgl.h"

#if ESP3D_DISPLAY_FEATURE

// Global variables for LVGL
static _lock_t lvgl_api_lock;
static lv_display_t *lvgl_display = NULL;
static lv_color16_t *lvgl_buf1 = NULL;
static lv_color16_t *lvgl_buf2 = NULL;
static esp_timer_handle_t lvgl_tick_timer = NULL;
static lv_indev_t *touch_indev = NULL;
static lv_indev_t *button_indev = NULL;
static lv_indev_t *encoder_indev = NULL;
static lv_indev_t *switch_indev = NULL;
static lv_indev_t *potentiometer_indev = NULL;

// Callback function to notify LVGL when flush is complete
static bool notify_lvgl_flush_ready(esp_lcd_panel_io_handle_t panel_io,
                                    esp_lcd_panel_io_event_data_t *edata,
                                    void *user_ctx)
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

    if (DISPLAY_SWAP_COLOR_FLAG) {
        lv_draw_sw_rgb565_swap(px_map, (offsetx2 + 1 - offsetx1) * (offsety2 + 1 - offsety1));
    }

    esp_lcd_panel_draw_bitmap(panel_handle, offsetx1, offsety1, offsetx2 + 1, offsety2 + 1, px_map);
}

// LVGL touch input read callback
static void touch_read_cb(lv_indev_t *indev, lv_indev_data_t *data)
{
    touch_ft6336u_data_t touch_data = touch_ft6336u_read();

    if (touch_data.is_pressed)
    {
        data->state   = LV_INDEV_STATE_PRESSED;
        data->point.x = touch_data.x;
        data->point.y = touch_data.y;
        esp3d_log("Touch detected at (%d, %d)", touch_data.x, touch_data.y);
    }
    else
    {
        data->state = LV_INDEV_STATE_RELEASED;
    }
}

// LVGL button input read callback
void button_read_cb(lv_indev_t *indev, lv_indev_data_t *data)
{
    static bool last_states[3] = {0, 0, 0};
    static control_event_t button_events[3] = {
        {NULL, 0, LV_INDEV_TYPE_BUTTON, CONTROL_FAMILY_BUTTONS, 0},
        {NULL, 1, LV_INDEV_TYPE_BUTTON, CONTROL_FAMILY_BUTTONS, 0},
        {NULL, 2, LV_INDEV_TYPE_BUTTON, CONTROL_FAMILY_BUTTONS, 0}
    };
    bool states[3];
    phy_buttons_read(states);
    lv_obj_t *active_screen = lv_screen_active();
    if (!active_screen) {
        esp3d_log_e("Active screen is NULL in button_read_cb");
        data->state = LV_INDEV_STATE_RELEASED;
        return;
    }
    // Initialize indev handles
    for (int i = 0; i < 3; i++) {
        button_events[i].indev = indev;
    }
    for (int i = 0; i < 3; i++) {
        if (states[i] && !last_states[i]) {
            data->state = LV_INDEV_STATE_PRESSED;
            lv_obj_send_event(active_screen, LV_EVENT_PRESSED, &button_events[i]);
            esp3d_log_d("Button %d pressed (btn_id: %d)", i + 1, i);
            last_states[i] = states[i];
            return;
        } else if (!states[i] && last_states[i]) {
            data->state = LV_INDEV_STATE_RELEASED;
            lv_obj_send_event(active_screen, LV_EVENT_RELEASED, &button_events[i]);
            esp3d_log_d("Button %d released (btn_id: %d)", i + 1, i);
            last_states[i] = states[i];
            return;
        }
    }
    data->state = LV_INDEV_STATE_RELEASED;
}

// LVGL encoder input read callback
void encoder_read_cb(lv_indev_t *indev, lv_indev_data_t *data)
{
    static control_event_t encoder_event = {
        NULL, 0, LV_INDEV_TYPE_ENCODER, CONTROL_FAMILY_ENCODER, 0
    };
    static uint32_t last_output_time = 0;
    uint32_t current_time = esp_timer_get_time() / 1000; // Temps en ms
    lv_obj_t *active_screen = lv_screen_active();
    if (!active_screen) {
        esp3d_log_e("Active screen is NULL in encoder_read_cb");
        data->key = 0;
        data->state = LV_INDEV_STATE_RELEASED;
        return;
    }
    // Initialize indev handle
    encoder_event.indev = indev;
    int32_t steps;
    if (phy_encoder_read(&steps) == ESP_OK && steps != 0) {
        uint32_t time_since_last = current_time - last_output_time;
        if (time_since_last == 40) {
            data->key = 0;
            data->state = LV_INDEV_STATE_RELEASED;
            return;
        }
        uint32_t min_interval;
        if (time_since_last < 200) {
            min_interval = 60;
        } else {
            min_interval = 120;
        }
        if (time_since_last >= min_interval) {
            // Adjust steps based on ENCODER_INVERT_ROTATION
            int32_t adjusted_steps = ENCODER_INVERT_ROTATION ? -steps : steps;
            data->key = (adjusted_steps > 0) ? LV_KEY_RIGHT : LV_KEY_LEFT;
            data->state = LV_INDEV_STATE_PRESSED;
            encoder_event.steps = adjusted_steps;
            lv_obj_send_event(active_screen, LV_EVENT_KEY, &encoder_event);
            esp3d_log_d("LVGL encoder: key=%ld (steps: %ld, adjusted_steps: %ld, interval: %lu)",
                       data->key, steps, adjusted_steps, time_since_last);
            last_output_time = current_time;
        } else {
            data->key = 0;
            data->state = LV_INDEV_STATE_RELEASED;
        }
    } else {
        data->key = 0;
        data->state = LV_INDEV_STATE_RELEASED;
    }
}

// LVGL switch input read callback
void switch_read_cb(lv_indev_t *indev, lv_indev_data_t *data)
{
    static bool last_states[4] = {0, 0, 0, 0};
    static control_event_t switch_events[4] = {
        {NULL, 0, LV_INDEV_TYPE_BUTTON, CONTROL_FAMILY_SWITCH, 0},
        {NULL, 1, LV_INDEV_TYPE_BUTTON, CONTROL_FAMILY_SWITCH, 0},
        {NULL, 2, LV_INDEV_TYPE_BUTTON, CONTROL_FAMILY_SWITCH, 0},
        {NULL, 3, LV_INDEV_TYPE_BUTTON, CONTROL_FAMILY_SWITCH, 0}
    };
    bool states[4];
    lv_obj_t *active_screen = lv_screen_active();
    if (!active_screen) {
        esp3d_log_e("Active screen is NULL in switch_read_cb");
        data->state = LV_INDEV_STATE_RELEASED;
        return;
    }
    // Initialize indev handles
    for (int i = 0; i < 4; i++) {
        switch_events[i].indev = indev;
    }
    if (phy_switch_read(states) == ESP_OK) {
        for (int i = 0; i < 4; i++) {
            if (states[i] && !last_states[i]) {
                data->state = LV_INDEV_STATE_PRESSED;
                lv_obj_send_event(active_screen, LV_EVENT_PRESSED, &switch_events[i]);
                esp3d_log_d("Switch button %d pressed (btn_id: %d)", i, i);
                last_states[i] = states[i];
                return;
            } else if (!states[i] && last_states[i]) {
                data->state = LV_INDEV_STATE_RELEASED;
                lv_obj_send_event(active_screen, LV_EVENT_RELEASED, &switch_events[i]);
                esp3d_log_d("Switch button %d released (btn_id: %d)", i, i);
                last_states[i] = states[i];
                return;
            }
        }
        data->state = LV_INDEV_STATE_RELEASED;
    } else {
        esp3d_log_e("Failed to read switch");
        data->state = LV_INDEV_STATE_RELEASED;
    }
}

// LVGL potentiometer input read callback
void potentiometer_read_cb(lv_indev_t *indev, lv_indev_data_t *data)
{
    static control_event_t potentiometer_event = {
        NULL, 0, LV_INDEV_TYPE_POINTER, CONTROL_FAMILY_POTENTIOMETER, 0
    };
    static uint32_t last_value = UINT32_MAX;
    lv_obj_t *active_screen = lv_screen_active();
    if (!active_screen) {
        esp3d_log_e("Active screen is NULL in potentiometer_read_cb");
        data->state = LV_INDEV_STATE_RELEASED;
        return;
    }
    // Initialize indev handle
    potentiometer_event.indev = indev;
    uint32_t adc_value;
    if (phy_potentiometer_read(&adc_value) == ESP_OK) {
        // Map ADC value (0–4095) to 0–100
        int32_t mapped_value = (adc_value * 100) / 4095;
        if (last_value == UINT32_MAX) {
            last_value = mapped_value;
        }
        int32_t delta = mapped_value - last_value;
        // Only send event if change is significant (e.g., ±5)
        if (abs(delta) >= 5) {
            potentiometer_event.steps = mapped_value;
            data->state = LV_INDEV_STATE_PRESSED;
            lv_obj_send_event(active_screen, LV_EVENT_VALUE_CHANGED, &potentiometer_event);
            esp3d_log_d("Potentiometer: adc_value=%ld, mapped_value=%ld, delta=%ld", 
                       adc_value, mapped_value, delta);
            last_value = mapped_value;
        } else {
            data->state = LV_INDEV_STATE_RELEASED;
        }
    } else {
        esp3d_log_e("Failed to read potentiometer");
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

    esp3d_log("Initializing LVGL v%d.%d.%d",
              lv_version_major(),
              lv_version_minor(),
              lv_version_patch());

    lv_init();

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

    lvgl_display = lv_display_create(DISPLAY_WIDTH_PX, DISPLAY_HEIGHT_PX);
    if (!lvgl_display) {
        esp3d_log_e("LVGL display creation failed");
        return ESP_FAIL;
    }

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

    lv_display_set_buffers(lvgl_display,
                           lvgl_buf1,
                           lvgl_buf2,
                           draw_buf_size,
                           LV_DISPLAY_RENDER_MODE_PARTIAL);

    lv_display_set_user_data(lvgl_display, panel_handle);

    lv_display_set_color_format(lvgl_display, LV_COLOR_FORMAT_RGB565);

    lv_display_set_flush_cb(lvgl_display, lvgl_flush_cb);

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

    const esp_lcd_panel_io_callbacks_t cbs = {
        .on_color_trans_done = notify_lvgl_flush_ready,
    };

    ret = esp_lcd_panel_io_register_event_callbacks(io_handle, &cbs, lvgl_display);
    if (ret != ESP_OK) {
        esp3d_log_e("Register IO callbacks failed");
        return ret;
    }

    touch_indev = lv_indev_create();
    lv_indev_set_type(touch_indev, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(touch_indev, touch_read_cb);
    lv_indev_set_display(touch_indev, lvgl_display);

    button_indev = lv_indev_create();
    lv_indev_set_type(button_indev, LV_INDEV_TYPE_BUTTON);
    lv_indev_set_read_cb(button_indev, button_read_cb);
    lv_indev_set_display(button_indev, lvgl_display);
    static lv_point_t button_points[3] = {{0, 0}, {0, 0}, {0, 0}};
    lv_indev_set_button_points(button_indev, button_points);

    encoder_indev = lv_indev_create();
    lv_indev_set_type(encoder_indev, LV_INDEV_TYPE_ENCODER);
    lv_indev_set_read_cb(encoder_indev, encoder_read_cb);
    lv_indev_set_display(encoder_indev, lvgl_display);

    switch_indev = lv_indev_create();
    lv_indev_set_type(switch_indev, LV_INDEV_TYPE_BUTTON);
    lv_indev_set_read_cb(switch_indev, switch_read_cb);
    lv_indev_set_display(switch_indev, lvgl_display);
    static lv_point_t switch_points[4] = {{0, 0}, {0, 0}, {0, 0}, {0, 0}};
    lv_indev_set_button_points(switch_indev, switch_points);

    potentiometer_indev = lv_indev_create();
    lv_indev_set_type(potentiometer_indev, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(potentiometer_indev, potentiometer_read_cb);
    lv_indev_set_display(potentiometer_indev, lvgl_display);

    esp3d_log("LVGL initialized successfully");
    return ESP_OK;
}
#endif  // ESP3D_DISPLAY_FEATURE

// Access functions for ESP3DTftUi
lv_display_t *get_lvgl_display(void)
{
#if ESP3D_DISPLAY_FEATURE
    return lvgl_display;
#else
    return NULL;
#endif
}

_lock_t *get_lvgl_lock(void)
{
#if ESP3D_DISPLAY_FEATURE
    return &lvgl_api_lock;
#else
    return NULL;
#endif
}

// Access functions for indev
lv_indev_t *get_touch_indev(void)
{
#if ESP3D_DISPLAY_FEATURE
    return touch_indev;
#else
    return NULL;
#endif
}

lv_indev_t *get_button_indev(void)
{
#if ESP3D_DISPLAY_FEATURE
    return button_indev;
#else
    return NULL;
#endif
}

lv_indev_t *get_encoder_indev(void)
{
#if ESP3D_DISPLAY_FEATURE
    return encoder_indev;
#else
    return NULL;
#endif
}

lv_indev_t *get_switch_indev(void)
{
#if ESP3D_DISPLAY_FEATURE
    return switch_indev;
#else
    return NULL;
#endif
}

lv_indev_t *get_potentiometer_indev(void)
{
#if ESP3D_DISPLAY_FEATURE
    return potentiometer_indev;
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
    ret = backlight_configure(&backlight_cfg);
    if (ret != ESP_OK) {
        esp3d_log_e("Backlight initialization failed");
        return ret;
    }
    backlight_set(0);

    ret = ili9341_spi_configure(&ili9341_default_config);
    if (ret != ESP_OK) {
        esp3d_log_e("ILI9341 display initialization failed");
        return ret;
    }

    ret = init_touch_controller();
    if (ret != ESP_OK) {
        esp3d_log_e("Touch controller initialization failed");
        return ret;
    }

    ret = phy_buttons_configure(&phy_buttons_cfg);
    if (ret != ESP_OK) {
        esp3d_log_e("Physical buttons initialization failed");
        return ret;
    }

    ret = phy_encoder_configure(&phy_encoder_cfg);
    if (ret != ESP_OK) {
        esp3d_log_e("Rotary encoder initialization failed");
        return ret;
    }

    ret = phy_switch_configure(&phy_switch_cfg);
    if (ret != ESP_OK) {
        esp3d_log_e("4-position switch initialization failed");
        return ret;
    }

    ret = phy_potentiometer_configure(&phy_potentiometer_cfg);
    if (ret != ESP_OK) {
        esp3d_log_e("Potentiometer initialization failed");
        return ret;
    }

    ret = buzzer_configure(&buzzer_cfg);
    if (ret != ESP_OK) {
        esp3d_log_e("Buzzer initialization failed");
        return ret;
    }

    ret = init_lvgl();
    if (ret != ESP_OK) {
        esp3d_log_e("LVGL initialization failed");
        return ret;
    }

#endif  // ESP3D_DISPLAY_FEATURE

    esp3d_log("Board initialization completed successfully");
    return ESP_OK;
}

// Get board name
const char *board_get_name(void)
{
    return BOARD_NAME_STR;
}

// Get board version
const char *board_get_version(void)
{
    return BOARD_VERSION_STR;
}