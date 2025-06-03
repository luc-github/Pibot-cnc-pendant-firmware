/*
  control_event.h - Structure for control events in PiBot CNC Pendant
  Copyright (c) 2025 Luc LEBOSSE. All rights reserved.
  Licensed under GNU Lesser General Public License v2.1 or later.
*/

#ifndef CONTROL_EVENT_H
#define CONTROL_EVENT_H

#include "lvgl.h"
#include "control_types.h"

typedef struct {
    lv_indev_t *indev;
    uint32_t btn_id;
    lv_indev_type_t type;
    control_family_t family_id;
    int32_t steps;
    uint32_t press_duration; // Duration of press in milliseconds
} control_event_t;

#endif // CONTROL_EVENT_H