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

#include "esp3d_tft_ui.h"

#include <string>
#include <sys/lock.h>
#include <algorithm> // Pour std::max

#include "esp3d_hal.h"
#include "esp3d_log.h"
#include "esp3d_values.h"
#include "esp3d_version.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "lvgl.h"
#include "board_init.h"
#include "rendering/esp3d_rendering_client.h"
#include "board_config.h"
#include "phy_buttons.h"

// Declaration of the external UI creation function
extern void create_application(void);

ESP3DTftUi esp3dTftui;

// UI task to handle LVGL
static void tft_ui_task(void *arg)
{
    ESP3DTftUi *ui = (ESP3DTftUi *)arg;
    (void)ui;  // Avoid unused variable warning
    _lock_t *lvgl_lock = get_lvgl_lock();
    
    esp3d_log("Starting UI task");
    
    // Call the function that creates the UI
    _lock_acquire(lvgl_lock);
    create_application();
    _lock_release(lvgl_lock);
    
    uint32_t time_till_next_ms = 0;
    uint32_t min_delay_ms = LVGL_TICK_PERIOD_MS;  // Minimum delay to avoid CPU overload
    
    while (1) {
        // Pause respecting LVGL delays - using std::max au lieu de MAX
        vTaskDelay(pdMS_TO_TICKS(std::max(time_till_next_ms, min_delay_ms)));
        
        // Take the lock to access LVGL
        _lock_acquire(lvgl_lock);
        // Handle values to update for UI
        esp3dTftValues.handle();
        
        // Call LVGL task handler and get time until next action
        time_till_next_ms = lv_timer_handler();
        
        // Release the lock
        _lock_release(lvgl_lock);
    }
    
    // Should never reach here
    vTaskDelete(NULL);
}

ESP3DTftUi::ESP3DTftUi() : _started(false), _ui_task_handle(NULL) {}

ESP3DTftUi::~ESP3DTftUi() {
    end();
}

bool ESP3DTftUi::begin() {
    if (_started) {
        return true;
    }
    
    // Start the rendering client
    if (!renderingClient.begin()) {
        esp3d_log_e("Rendering client not started");
        return false;
    }
    
    // Create the UI task
    BaseType_t res = xTaskCreatePinnedToCore(tft_ui_task, "tftUI", LVGL_TASK_STACK_SIZE, this,
                                            LVGL_TASK_PRIORITY, &_ui_task_handle, LVGL_TASK_CORE);
    if (res == pdPASS && _ui_task_handle) {
        esp3d_log("Created UI Task");
        _started = true;
        return true;
    } else {
        esp3d_log_e("UI Task creation failed");
        renderingClient.end();
        return false;
    }
}

void ESP3DTftUi::handle() {
    // This function can remain empty as management is done in the UI task
}

bool ESP3DTftUi::end() {
    if (!_started) {
        return true;
    }
    
    // Stop the rendering client
    renderingClient.end();
    
    // Stop and delete the UI task
    if (_ui_task_handle != NULL) {
        vTaskDelete(_ui_task_handle);
        _ui_task_handle = NULL;
    }
    
    _started = false;
    return true;
}