/*
  phy_switch.c - 4-position switch driver for PiBot CNC Pendant
  Copyright (c) 2025 Luc Lebosse. All rights reserved.
  Licensed under GNU Lesser General Public License v2.1 or later.
*/

#include "phy_switch.h"
#include "esp3d_log.h"
#include "driver/gpio.h"
#include "esp_timer.h"

static phy_switch_config_t switch_config;
static bool is_initialized = false;

// Table de correspondance pour les 4 états (basée sur 000, 100, 010, 001)
static const uint32_t state_to_key_code[8] = {
    0, // 000: Position 0 (aucune broche active)
    3, // 001: Position 3 (GPIO39 à 0)
    2, // 010: Position 2 (GPIO35 à 0)
    0, // 011: Non utilisé
    1, // 100: Position 1 (GPIO34 à 0)
    0, // 101: Non utilisé
    0, // 110: Non utilisé
    0  // 111: Non utilisé
};

/**
 * @brief Configure the 4-position switch
 */
esp_err_t phy_switch_configure(const phy_switch_config_t *config)
{
    if (config == NULL) {
        esp3d_log_e("Invalid switch configuration");
        return ESP_ERR_INVALID_ARG;
    }

    memcpy(&switch_config, config, sizeof(phy_switch_config_t));

    // Validate GPIO pins
    for (int i = 0; i < 3; i++) {
        if (!GPIO_IS_VALID_GPIO(switch_config.pins[i])) {
            esp3d_log_e("Invalid GPIO number for switch pin %d", i + 1);
            return ESP_ERR_INVALID_ARG;
        }
    }

    // Configure GPIO pins
    for (int i = 0; i < 3; i++) {
        gpio_config_t io_conf = {
            .pin_bit_mask = (1ULL << switch_config.pins[i]),
            .mode = GPIO_MODE_INPUT,
            .pull_up_en = switch_config.pullups_enabled ? GPIO_PULLUP_ENABLE : GPIO_PULLUP_DISABLE,
            .pull_down_en = GPIO_PULLDOWN_DISABLE,
            .intr_type = GPIO_INTR_DISABLE
        };
        if (gpio_config(&io_conf) != ESP_OK) {
            esp3d_log_e("Failed to configure GPIO for switch pin %d", i + 1);
            return ESP_ERR_INVALID_ARG;
        }
    }

    is_initialized = true;
    esp3d_log("4-position switch configured successfully");
    return ESP_OK;
}

/**
 * @brief Read the state of the switch as buttons
 */
esp_err_t phy_switch_read(bool *states)
{
    if (!is_initialized) {
        esp3d_log_e("Switch not configured");
        return ESP_ERR_INVALID_STATE;
    }

    if (states == NULL) {
        esp3d_log_e("Invalid states pointer");
        return ESP_ERR_INVALID_ARG;
    }

    static uint32_t last_key_code = UINT32_MAX;
    static uint64_t last_change_time = 0;
    static uint32_t last_state = UINT32_MAX;
    static uint32_t stable_count = 0;
    uint64_t current_time = esp_timer_get_time() / 1000; // Temps en ms
    const uint64_t transient_key0_duration = 1000; // 1000 ms pour key_code 0
    const uint32_t stable_threshold = 3; // Nombre de lectures consécutives pour stabilité

    // Lire les niveaux des 3 pins (inversés pour correspondre à Arduino)
    uint32_t state = 0;
    bool changed = false;
    static uint8_t prevstate[] = {0, 0, 0};
    for (int i = 0; i < 3; i++) {
        uint8_t current_level = !gpio_get_level(switch_config.pins[i]); // Inversé pour correspondre à Arduino
        if (current_level != prevstate[i]) {
            changed = true; // Un changement a été détecté
            prevstate[i] = current_level; // Mettre à jour l'état précédent
        }
        state |= (current_level << i);
    }

    // Loguer les niveaux bruts des pins
   // esp3d_log_d("Switch pins (39,35,34): %d%d%d", 
   //             !gpio_get_level(switch_config.pins[2]), 
   //             !gpio_get_level(switch_config.pins[1]), 
   //             !gpio_get_level(switch_config.pins[0]));
   #if TFT_LOG_LEVEL >= ESP_LOG_DEBUG
   if (changed){
    esp3d_log_d("Switch pins (39,35,34): %d%d%d", 
                prevstate[2], 
                prevstate[1], 
                prevstate[0]);
   }

   #endif // TFT_LOG_LEVEL >= ESP_LOG_DEBUG

    // Mapper l'état à un code de touche
    uint32_t new_key_code = state_to_key_code[state];

    // Vérifier la stabilité de l'état
    if (state == last_state) {
        stable_count++;
    } else {
        stable_count = 0;
        last_state = state;
    }

    // Vérification du débounce et de la stabilité
    if (new_key_code != last_key_code && stable_count >= stable_threshold) {
        uint64_t required_duration = (new_key_code == 0) ? transient_key0_duration : switch_config.debounce_ms;
        if ((current_time - last_change_time) >= required_duration) {
            last_key_code = new_key_code;
            last_change_time = current_time;
            esp3d_log_d("Switch state changed to key code %ld", new_key_code);
        }
    }

    // Mettre à jour les états des boutons virtuels
    for (int i = 0; i < 4; i++) {
        states[i] = (i == last_key_code);
    }

    return ESP_OK;
}

/**
 * @brief Get the current state of the switch
 */
esp_err_t phy_switch_get_state(uint32_t *key_code)
{
    if (!is_initialized) {
        esp3d_log_e("Switch not configured");
        return ESP_ERR_INVALID_STATE;
    }

    if (key_code == NULL) {
        esp3d_log_e("Invalid key code pointer");
        return ESP_ERR_INVALID_ARG;
    }

    // Lire les niveaux des 3 pins
    uint32_t state = 0;
    for (int i = 0; i < 3; i++) {
        state |= (!gpio_get_level(switch_config.pins[i]) << i);
    }

    // Mapper l'état à un code de touche
    *key_code = state_to_key_code[state];
    return ESP_OK;
}