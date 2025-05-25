/*
  control_types.h - Type definitions for control devices in PiBot CNC Pendant
  Copyright (c) 2025 Luc LEBOSSE. All rights reserved.
  Licensed under GNU Lesser General Public License v2.1 or later.
*/

#ifndef CONTROL_TYPES_H
#define CONTROL_TYPES_H

typedef enum {
    CONTROL_FAMILY_BUTTONS = 0, // Buttons (3 physical buttons)
    CONTROL_FAMILY_SWITCH,     // 4-position switch
    CONTROL_FAMILY_TOUCH,       // Touchscreen (future)
    CONTROL_FAMILY_ENCODER,     // Rotary encoder (future)
    CONTROL_FAMILY_POTENTIOMETER // Potentiometer (future)
} control_family_t;

#endif // CONTROL_TYPES_H