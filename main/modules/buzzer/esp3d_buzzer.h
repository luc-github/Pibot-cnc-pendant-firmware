/*
  esp3d_buzzer.h -  buzzer functions class

  Copyright (c) 2014 Luc Lebosse. All rights reserved.

  This code is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This code is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with This code; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/
#pragma once
#include <esp_err.h>
#include "stdio.h"
#include "buzzer_tone.h"

class Buzzer final {
 public:
  Buzzer();
  ~Buzzer();
  bool begin();
  void end();
  bool enabled( bool fromSettings = false);
  esp_err_t set_loud(bool loud);
  esp_err_t bip(uint16_t freq_hz, uint32_t duration_ms);
  esp_err_t play(const buzzer_tone_t *tones, uint32_t count);
 private:
  bool _enabled;
};

extern Buzzer esp3d_buzzer;