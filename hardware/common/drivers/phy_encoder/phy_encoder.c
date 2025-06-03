/*
  phy_encoder.c - Rotary encoder driver for PiBot CNC Pendant
  Copyright (c) 2025 Luc Lebosse. All rights reserved.
  Licensed under GNU Lesser General Public License v2.1 or later.
*/

#include "phy_encoder.h"
#include "esp3d_log.h"
#include "driver/pulse_cnt.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "esp_timer.h"

static phy_encoder_config_t encoder_config;
static bool is_initialized = false;
static volatile int32_t encoder_steps = 0;
static QueueHandle_t encoder_queue = NULL;
static pcnt_unit_handle_t pcnt_unit = NULL;
static int32_t accumulated_pulses = 0; // Accumulate all pulses from PCNT

// Callback function for PCNT watch events (limit reached)
static bool encoder_pcnt_on_reach(pcnt_unit_handle_t unit, const pcnt_watch_event_data_t *edata, void *user_ctx)
{
    BaseType_t high_task_wakeup;
    QueueHandle_t queue = (QueueHandle_t)user_ctx;
    int watch_point_value = edata->watch_point_value;
    // Send the watch point value to the queue
    xQueueSendFromISR(queue, &watch_point_value, &high_task_wakeup);
    return (high_task_wakeup == pdTRUE);
}

/**
 * @brief Configure the rotary encoder using PCNT
 *
 * @param config Pointer to the encoder configuration
 * @return ESP_OK on success, or an error code on failure
 */
esp_err_t phy_encoder_configure(const phy_encoder_config_t *config)
{
    if (config == NULL) {
        esp3d_log_e("Invalid encoder configuration");
        return ESP_ERR_INVALID_ARG;
    }

    memcpy(&encoder_config, config, sizeof(phy_encoder_config_t));

    // Validate GPIO pins
    if (!GPIO_IS_VALID_GPIO(encoder_config.pin_a) || !GPIO_IS_VALID_GPIO(encoder_config.pin_b)) {
        esp3d_log_e("Invalid GPIO number for encoder pins");
        return ESP_ERR_INVALID_ARG;
    }

    // Initialize PCNT unit
    esp3d_log("Install PCNT unit");
    pcnt_unit_config_t unit_config = {
        .high_limit = encoder_config.pcnt_high_limit,
        .low_limit = encoder_config.pcnt_low_limit,
        .intr_priority = 1, // Set a low interrupt priority to avoid interfering with other peripherals
    };
    ESP_ERROR_CHECK(pcnt_new_unit(&unit_config, &pcnt_unit));

    // Set glitch filter
    esp3d_log("Set glitch filter");
    pcnt_glitch_filter_config_t filter_config = {
        .max_glitch_ns = 500, // Reduced from 1000 to 500 ns to improve sensitivity
    };
    ESP_ERROR_CHECK(pcnt_unit_set_glitch_filter(pcnt_unit, &filter_config));

    // Install PCNT channels
    esp3d_log("Install PCNT channels");
    pcnt_chan_config_t chan_a_config = {
        .edge_gpio_num = encoder_config.pin_a,
        .level_gpio_num = encoder_config.pin_b,
    };
    pcnt_channel_handle_t pcnt_chan_a = NULL;
    ESP_ERROR_CHECK(pcnt_new_channel(pcnt_unit, &chan_a_config, &pcnt_chan_a));

    pcnt_chan_config_t chan_b_config = {
        .edge_gpio_num = encoder_config.pin_b,
        .level_gpio_num = encoder_config.pin_a,
    };
    pcnt_channel_handle_t pcnt_chan_b = NULL;
    ESP_ERROR_CHECK(pcnt_new_channel(pcnt_unit, &chan_b_config, &pcnt_chan_b));

    // Set edge and level actions for PCNT channels (quadrature decoding)
    esp3d_log("Set edge and level actions for PCNT channels");
    ESP_ERROR_CHECK(pcnt_channel_set_edge_action(pcnt_chan_a, PCNT_CHANNEL_EDGE_ACTION_DECREASE, PCNT_CHANNEL_EDGE_ACTION_INCREASE));
    ESP_ERROR_CHECK(pcnt_channel_set_level_action(pcnt_chan_a, PCNT_CHANNEL_LEVEL_ACTION_KEEP, PCNT_CHANNEL_LEVEL_ACTION_INVERSE));
    ESP_ERROR_CHECK(pcnt_channel_set_edge_action(pcnt_chan_b, PCNT_CHANNEL_EDGE_ACTION_INCREASE, PCNT_CHANNEL_EDGE_ACTION_DECREASE));
    ESP_ERROR_CHECK(pcnt_channel_set_level_action(pcnt_chan_b, PCNT_CHANNEL_LEVEL_ACTION_KEEP, PCNT_CHANNEL_LEVEL_ACTION_INVERSE));

    // Add watch points for limits (only high and low limits)
    esp3d_log("Add watch points and register callbacks");
    int watch_points[] = {encoder_config.pcnt_low_limit, encoder_config.pcnt_high_limit};
    for (size_t i = 0; i < sizeof(watch_points) / sizeof(watch_points[0]); i++) {
        ESP_ERROR_CHECK(pcnt_unit_add_watch_point(pcnt_unit, watch_points[i]));
    }

    // Create a queue for watch events
    encoder_queue = xQueueCreate(10, sizeof(int));
    if (encoder_queue == NULL) {
        esp3d_log_e("Failed to create encoder queue");
        return ESP_ERR_NO_MEM;
    }

    // Register callback for watch events
    pcnt_event_callbacks_t cbs = {
        .on_reach = encoder_pcnt_on_reach,
    };
    ESP_ERROR_CHECK(pcnt_unit_register_event_callbacks(pcnt_unit, &cbs, encoder_queue));

    // Enable and start PCNT unit
    esp3d_log("Enable PCNT unit");
    ESP_ERROR_CHECK(pcnt_unit_enable(pcnt_unit));
    esp3d_log("Clear PCNT unit");
    ESP_ERROR_CHECK(pcnt_unit_clear_count(pcnt_unit));
    esp3d_log("Start PCNT unit");
    ESP_ERROR_CHECK(pcnt_unit_start(pcnt_unit));

    // Reset global step counter and accumulated pulses
    encoder_steps = 0;
    accumulated_pulses = 0;

    is_initialized = true;
    esp3d_log_d("Rotary encoder configured successfully (pins A:%d, B:%d, steps_per_rev:%lu)",
              encoder_config.pin_a, encoder_config.pin_b, encoder_config.steps_per_rev);
    return ESP_OK;
}

/**
 * @brief Read the encoder state using PCNT
 *
 * This function reads the accumulated steps since the last call.
 *
 * @param steps Pointer to store the step count
 * @return ESP_OK on success, or an error code on failure
 */
esp_err_t phy_encoder_read(int32_t *steps)
{
    if (!is_initialized) {
        esp3d_log_e("Encoder not configured");
        return ESP_ERR_INVALID_STATE;
    }

    if (steps == NULL) {
        esp3d_log_e("Invalid steps pointer");
        return ESP_ERR_INVALID_ARG;
    }

    // Get the current count from PCNT
    int pulse_count = 0;
    ESP_ERROR_CHECK(pcnt_unit_get_count(pcnt_unit, &pulse_count));
    //esp3d_log_d("Raw pulse count: %d", pulse_count);

    // Check for watch events (limits reached)
    int event_count = 0;
    while (xQueueReceive(encoder_queue, &event_count, 0)) {
        esp3d_log_d("Watch point event, count: %d", event_count);
        // Reset the counter if a limit is reached
        if (event_count == encoder_config.pcnt_high_limit || event_count == encoder_config.pcnt_low_limit) {
            ESP_ERROR_CHECK(pcnt_unit_clear_count(pcnt_unit));
            accumulated_pulses = 0; // Reset accumulated pulses
            pulse_count = 0;
        }
    }

    // Accumulate pulses
    accumulated_pulses += pulse_count;

    // Calculate clicks based on accumulated pulses
    int32_t new_clicks = accumulated_pulses / 4; // 4 pulses per click in 4X mode
    int32_t clicks = new_clicks - encoder_steps; // Difference since last read

    // Update global steps
    encoder_steps = new_clicks;

    // Clear the PCNT counter after reading
    ESP_ERROR_CHECK(pcnt_unit_clear_count(pcnt_unit));

    *steps = clicks;
    //esp3d_log_d("Encoder clicks: %ld (total steps: %ld, accumulated pulses: %ld)", clicks, encoder_steps, accumulated_pulses);

    return ESP_OK;
}

/**
 * @brief Get the total accumulated steps since initialization
 *
 * @param total_steps Pointer to store the total accumulated steps
 * @return ESP_OK on success, or an error code on failure
 */
esp_err_t phy_encoder_get_total_steps(int32_t *total_steps)
{
    if (!is_initialized) {
        esp3d_log_e("Encoder not configured");
        return ESP_ERR_INVALID_STATE;
    }

    if (total_steps == NULL) {
        esp3d_log_e("Invalid total_steps pointer");
        return ESP_ERR_INVALID_ARG;
    }

    *total_steps = encoder_steps;
    return ESP_OK;
}

/**
 * @brief Reset the total accumulated steps counter
 *
 * @return ESP_OK on success, or an error code on failure
 */
esp_err_t phy_encoder_reset_steps(void)
{
    if (!is_initialized) {
        esp3d_log_e("Encoder not configured");
        return ESP_ERR_INVALID_STATE;
    }

    encoder_steps = 0;
    accumulated_pulses = 0;
    ESP_ERROR_CHECK(pcnt_unit_clear_count(pcnt_unit));
    esp3d_log_d("Encoder steps counter reset");
    return ESP_OK;
}

/**
 * @brief Get encoder configuration information
 *
 * @param config Pointer to store the current configuration
 * @return ESP_OK on success, or an error code on failure
 */
esp_err_t phy_encoder_get_config(phy_encoder_config_t *config)
{
    if (!is_initialized) {
        esp3d_log_e("Encoder not configured");
        return ESP_ERR_INVALID_STATE;
    }

    if (config == NULL) {
        esp3d_log_e("Invalid config pointer");
        return ESP_ERR_INVALID_ARG;
    }

    memcpy(config, &encoder_config, sizeof(phy_encoder_config_t));
    return ESP_OK;
}

/**
 * @brief Deinitialize the encoder
 *
 * @return ESP_OK on success, or an error code on failure
 */
esp_err_t phy_encoder_deinit(void)
{
    if (!is_initialized) {
        return ESP_OK; // Already deinitialized
    }

    // Stop and disable PCNT unit
    ESP_ERROR_CHECK(pcnt_unit_stop(pcnt_unit));
    ESP_ERROR_CHECK(pcnt_unit_disable(pcnt_unit));
    ESP_ERROR_CHECK(pcnt_unit_clear_count(pcnt_unit));

    // Delete PCNT unit
    ESP_ERROR_CHECK(pcnt_del_unit(pcnt_unit));
    pcnt_unit = NULL;

    // Delete queue
    if (encoder_queue != NULL) {
        vQueueDelete(encoder_queue);
        encoder_queue = NULL;
    }

    // Reset variables
    encoder_steps = 0;
    accumulated_pulses = 0;
    is_initialized = false;

    esp3d_log_d("Rotary encoder deinitialized");
    return ESP_OK;
}