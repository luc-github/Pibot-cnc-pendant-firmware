/*
  esp3d_buzzer.cpp -  buzzer functions class

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

#if defined(ESP3D_BUZZER_FEATURE) && ESP3D_BUZZER_FEATURE == 1

#include "esp3d_buzzer.h"

#include "buzzer.h"
#include "esp3d_log.h"
#include "esp3d_settings.h"

Buzzer esp3d_buzzer;

Buzzer::Buzzer()
{
    end();
}

Buzzer::~Buzzer()
{
    end();
}

bool Buzzer::enabled(bool fromSettings)
{
    if (fromSettings)
    {
        _enabled = esp3dTftsettings.readByte(ESP3DSettingIndex::esp3d_buzzer_on);
    }
    return _enabled;
}

bool Buzzer::begin()
{
    end();
    enabled(true);
    if (!_enabled)
    {
        esp3d_log_d("Buzzer is disabled in settings");
    }
    esp3d_log_d("Buzzer started successfully");
    return true;
}

void Buzzer::end()
{
    _enabled = false;
    esp3d_log_d("Buzzer ended successfully");
}

esp_err_t Buzzer::set_loud(bool loud)
{
    if (!_enabled)
    {
        esp3d_log_d("Buzzer is disabled, cannot set loudness");
        return ESP_ERR_INVALID_STATE;
    }
    esp_err_t result = buzzer_set_loud(loud);
    if (result != ESP_OK)
    {
        esp3d_log_e("Failed to set buzzer loudness: %s", esp_err_to_name(result));
    }
    return result;
}

esp_err_t Buzzer::bip(uint16_t freq_hz, uint32_t duration_ms)
{
    if (!_enabled)
    {
        esp3d_log_d("Buzzer is disabled, cannot play bip");
        return ESP_ERR_INVALID_STATE;
    }
    esp_err_t result = buzzer_bip(freq_hz, duration_ms);
    if (result != ESP_OK)
    {
        esp3d_log_e("Failed to play bip: %s", esp_err_to_name(result));
    }
    return result;
}

esp_err_t Buzzer::play(const buzzer_tone_t *tones, uint32_t count)
{
    if (!_enabled)
    {
        esp3d_log_d("Buzzer is disabled, cannot play tones");
        return ESP_ERR_INVALID_STATE;
    }
    esp_err_t result = buzzer_play(tones, count);
    if (result != ESP_OK)
    {
        esp3d_log_e("Failed to play tones: %s", esp_err_to_name(result));
    }
    return result;
}

#endif  // ESP3D_BUZZER_FEATURE