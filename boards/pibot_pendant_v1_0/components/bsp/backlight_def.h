// Backlight definitions for PiBot CNC Pendant
#pragma once

#include "board_config.h"
#include "backlight.h"

#ifdef __cplusplus
extern "C" {
#endif

// Backlight configuration using values from board_config.h
backlight_config_t backlight_config = {
    .pin = BACKLIGHT_PIN,
    .active_high = BACKLIGHT_ACTIVE_HIGH,
    .default_level = BACKLIGHT_DEFAULT_LEVEL,
    .pwm_control = BACKLIGHT_PWM_ENABLED,
    .pwm_freq = BACKLIGHT_PWM_FREQ,
    .pwm_resolution = BACKLIGHT_PWM_RESOLUTION,
    .pwm_timer = BACKLIGHT_PWM_TIMER,
    .pwm_channel = BACKLIGHT_PWM_CHANNEL
};

#ifdef __cplusplus
}
#endif
