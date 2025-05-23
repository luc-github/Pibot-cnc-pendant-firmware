/*
  phy_encoder.c - Rotary encoder driver for PiBot CNC Pendant
  Copyright (c) 2025 Luc Lebosse. All rights reserved.
  Licensed under GNU Lesser General Public License v2.1 or later.
*/

#include "phy_encoder.h"
#include "esp3d_log.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "esp_timer.h"

static phy_encoder_config_t encoder_config;
static bool is_initialized = false;
static volatile int32_t encoder_steps = 0;
static QueueHandle_t encoder_queue = NULL;

// Transition table for quadrature decoding (based on A and B states)
static const int8_t transition_table[16] = {
    0,  // 00 -> 00: No change
    -1, // 00 -> 01: Counterclockwise
    1,  // 00 -> 10: Clockwise
    0,  // 00 -> 11: Invalid
    1,  // 01 -> 00: Clockwise
    0,  // 01 -> 01: No change
    0,  // 01 -> 10: Invalid
    -1, // 01 -> 11: Counterclockwise
    -1, // 10 -> 00: Counterclockwise
    0,  // 10 -> 01: Invalid
    0,  // 10 -> 10: No change
    1,  // 10 -> 11: Clockwise
    0,  // 11 -> 00: Invalid
    1,  // 11 -> 01: Clockwise
    -1, // 11 -> 10: Counterclockwise
    0   // 11 -> 11: No change
};

// GPIO interrupt handler for encoder signals
static void IRAM_ATTR encoder_isr_handler(void *arg)
{
    static uint8_t prev_state = 0;
    uint8_t curr_state = (gpio_get_level(encoder_config.pin_a) << 1) | gpio_get_level(encoder_config.pin_b);
    uint8_t transition = (prev_state << 2) | curr_state;
    int8_t step = transition_table[transition];
    
    if (step != 0) {
        encoder_steps += step;
        BaseType_t higher_priority_task_woken = pdFALSE;
        xQueueSendFromISR(encoder_queue, &step, &higher_priority_task_woken);
        portYIELD_FROM_ISR(higher_priority_task_woken);
    }
    prev_state = curr_state;
}

/**
 * @brief Configure the rotary encoder
 *
 * This function initializes the GPIO pins for the encoder based on the provided configuration.
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

    // Validate configuration values
    if (encoder_config.debounce_us == 0 || encoder_config.min_step_interval_us == 0) {
        esp3d_log_e("Invalid debounce or step interval configuration");
        return ESP_ERR_INVALID_ARG;
    }

    // Configure GPIO pins
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << encoder_config.pin_a) | (1ULL << encoder_config.pin_b),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = encoder_config.pullups_enabled ? GPIO_PULLUP_ENABLE : GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_ANYEDGE
    };
    
    if (gpio_config(&io_conf) != ESP_OK) {
        esp3d_log_e("Failed to configure GPIO for encoder");
        return ESP_ERR_INVALID_ARG;
    }

    // Create queue for interrupt events
    encoder_queue = xQueueCreate(20, sizeof(int8_t)); // Increased for high resolution encoder
    if (encoder_queue == NULL) {
        esp3d_log_e("Failed to create encoder queue");
        return ESP_ERR_NO_MEM;
    }

    // Install interrupt handler service
    if (gpio_install_isr_service(0) != ESP_OK) {
        esp3d_log("ISR service already installed");
    }
    
    if (gpio_isr_handler_add(encoder_config.pin_a, encoder_isr_handler, NULL) != ESP_OK ||
        gpio_isr_handler_add(encoder_config.pin_b, encoder_isr_handler, NULL) != ESP_OK) {
        esp3d_log_e("Failed to add ISR handler");
        vQueueDelete(encoder_queue);
        return ESP_ERR_INVALID_STATE;
    }

    // Reset variables
    encoder_steps = 0;

    is_initialized = true;
    esp3d_log("Rotary encoder configured successfully (pins A:%d, B:%d, debounce:%luµs, min_interval:%luµs, steps_per_rev:%lu)", 
              encoder_config.pin_a, encoder_config.pin_b, 
              encoder_config.debounce_us, encoder_config.min_step_interval_us, 
              encoder_config.steps_per_rev);
    return ESP_OK;
}

/**
 * @brief Read the encoder state with advanced filtering
 *
 * This function reads the current encoder steps with debounce and step interval filtering.
 *
 * @param steps Pointer to store the filtered step count
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

    int32_t total_steps = 0;
    int8_t step;
    uint64_t current_time = esp_timer_get_time();
    static uint64_t last_debounce_time = 0;
    static uint64_t last_valid_step_time = 0;

    // Read events from queue with dual filtering
    while (xQueueReceive(encoder_queue, &step, 0)) {
        // First filter: hardware debounce
        if ((current_time - last_debounce_time) >= encoder_config.debounce_us) {
            // Second filter: minimum step interval
            if ((current_time - last_valid_step_time) >= encoder_config.min_step_interval_us) {
                total_steps += step;
                last_valid_step_time = current_time;
                esp3d_log("Encoder step: %d (total: %ld)", step, total_steps);
            } else {
                esp3d_log("Encoder step filtered out (too fast): %d", step);
            }
            last_debounce_time = current_time;
        } else {
            esp3d_log("Encoder step debounced: %d", step);
        }
    }

    *steps = total_steps;
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
    esp3d_log("Encoder steps counter reset");
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

    // Remove interrupt handlers
    gpio_isr_handler_remove(encoder_config.pin_a);
    gpio_isr_handler_remove(encoder_config.pin_b);

    // Delete queue
    if (encoder_queue != NULL) {
        vQueueDelete(encoder_queue);
        encoder_queue = NULL;
    }

    // Reset variables
    encoder_steps = 0;
    is_initialized = false;

    esp3d_log("Rotary encoder deinitialized");
    return ESP_OK;
}