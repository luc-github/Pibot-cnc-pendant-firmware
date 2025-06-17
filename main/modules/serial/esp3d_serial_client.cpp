/*
  esp3d_serial_client

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
#include "esp3d_serial_client.h"

#include <stdio.h>

#include "driver/gpio.h"
#include "driver/uart.h"
#include "esp3d_commands.h"
#include "esp3d_hal.h"
#include "esp3d_log.h"
#include "esp3d_settings.h"
#include "esp_bt.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "serial_def.h"

#if ESP3D_BT_FEATURE
#include "network/esp3d_network.h"
#endif  // ESP3D_BT_FEATURE

ESP3DSerialClient serialClient;

bool ESP3DSerialClient::configure(esp3d_serial_config_t *config)
{
    esp3d_log("Configure Serial Client");
    if (config)
    {
        _config = config;
        return true;
    }
    return false;
}

void ESP3DSerialClient::readSerial()
{
    static uint64_t startTimeout = 0;  // milliseconds
    // Use buffer size from configuration
    int len = uart_read_bytes(_config->port,
                              _data,
                              (_config->rx_buffer_size / 2 - 1),
                              10 / portTICK_PERIOD_MS);
    if (len)
    {
        // parse data
        startTimeout = esp3d_hal::millis();
        esp3d_log("Read %d bytes", len);
        for (size_t i = 0; i < len; i++)
        {
            if (_bufferPos < (_config->rx_buffer_size))
            {
                _buffer[_bufferPos] = _data[i];
                _bufferPos++;
            }
            // if end of char or buffer is full
            if (serialClient.isEndChar(_data[i]) || _bufferPos == (_config->rx_buffer_size))
            {
                // create message and push
                if (!serialClient.pushMsgToRxQueue(_buffer, _bufferPos))
                {
                    // send error
                    esp3d_log_e("Push Message to rx queue failed");
                }
                _bufferPos = 0;
            }
        }
    }
    // if no data during a while then send them
    if (esp3d_hal::millis() - startTimeout > (_config->rx_flush_timeout) && _bufferPos > 0)
    {
        if (!serialClient.pushMsgToRxQueue(_buffer, _bufferPos))
        {
            // send error
            esp3d_log_e("Push Message to rx queue failed");
        }
        _bufferPos = 0;
    }
}

// this task only collecting serial RX data and push them to Rx Queue
static void esp3d_serial_rx_task(void *pvParameter)
{
    (void)pvParameter;
    while (1)
    {
        /* Delay */
        esp3d_hal::wait(1);
        if (!serialClient.started())
        {
            break;
        }
        serialClient.readSerial();
    }
    /* A task should NEVER return */
    vTaskDelete(NULL);
}

ESP3DSerialClient::ESP3DSerialClient()
{
    _started   = false;
    _xHandle   = NULL;
    _data      = NULL;
    _buffer    = NULL;
    _bufferPos = 0;
    _config    = NULL;
}

ESP3DSerialClient::~ESP3DSerialClient()
{
    end();
}

void ESP3DSerialClient::process(ESP3DMessage *msg)
{
    esp3d_log("Add message to queue");
    if (!addTxData(msg))
    {
        flush();
        if (!addTxData(msg))
        {
            esp3d_log_e("Cannot add msg to client queue");
            deleteMsg(msg);
        }
    }
    else
    {
        flush();
    }
}

bool ESP3DSerialClient::isEndChar(uint8_t ch)
{
    return ((char)ch == '\n' || (char)ch == '\r');
}

bool ESP3DSerialClient::begin()
{
    esp3d_log("Freeheap before BT release %u, %u",
              (unsigned int)esp_get_free_heap_size(),
              (unsigned int)heap_caps_get_free_size(MALLOC_CAP_8BIT | MALLOC_CAP_INTERNAL));
   
#if ESP3D_BT_FEATURE
    uint8_t byteValue = esp3dTftsettings.readByte(ESP3DSettingIndex::esp3d_radio_mode);
    if (byteValue != (uint8_t)ESP3DRadioMode::bluetooth_ble
        && byteValue != (uint8_t)ESP3DRadioMode::bluetooth_serial)
    {
        // Release BT memory if not used
        esp3d_log_d("Release BT memory");
        esp_bt_controller_mem_release(ESP_BT_MODE_BTDM);
    }
#endif  // ESP3D_BT_FEATURE

    end();
    configure(&esp3dSerialConfig);
    if (!_config)
    {
        esp3d_log_e("Serial not configured");
        return false;
    }

    // Allocate buffers using the configured buffer size
    _data = (uint8_t *)malloc(_config->rx_buffer_size);
    if (!_data)
    {
        esp3d_log_e("Failed to allocate memory for data buffer");
        return false;
    }

    _buffer = (uint8_t *)malloc(_config->rx_buffer_size);
    if (!_buffer)
    {
        free(_data);
        _data = NULL;
        esp3d_log_e("Failed to allocate memory for buffer");
        return false;
    }

    if (pthread_mutex_init(&_rx_mutex, NULL) != 0)
    {
        free(_data);
        _data = NULL;
        free(_buffer);
        _buffer = NULL;
        esp3d_log_e("Mutex creation for rx failed");
        return false;
    }
    setRxMutex(&_rx_mutex);

    if (pthread_mutex_init(&_tx_mutex, NULL) != 0)
    {
        free(_data);
        _data = NULL;
        free(_buffer);
        _buffer = NULL;
        pthread_mutex_destroy(&_rx_mutex);
        esp3d_log_e("Mutex creation for tx failed");
        return false;
    }
    setTxMutex(&_tx_mutex);

    // Load baud rate from settings
    uint32_t baudrate = esp3dTftsettings.readUint32(ESP3DSettingIndex::esp3d_baud_rate);
    if (!esp3dTftsettings.isValidIntegerSetting(baudrate, ESP3DSettingIndex::esp3d_baud_rate))
    {
        esp3d_log_w("Invalid baudrate use default");
        baudrate = esp3dTftsettings.getDefaultIntegerSetting(ESP3DSettingIndex::esp3d_baud_rate);
    }

    // Update baud rate in configuration
    _config->uart_config.baud_rate = (int)baudrate;

    esp3d_log("Use %ld Serial Baud Rate", baudrate);

    int intr_alloc_flags = 0;
#if CONFIG_UART_ISR_IN_IRAM
    intr_alloc_flags = ESP_INTR_FLAG_IRAM;
#endif

    // Install UART driver with configured parameters
    ESP_ERROR_CHECK(uart_driver_install(_config->port,
                                        _config->rx_buffer_size * 2,
                                        _config->tx_buffer_size,
                                        0,
                                        NULL,
                                        intr_alloc_flags));

    // Configure UART parameters
    ESP_ERROR_CHECK(uart_param_config(_config->port, &_config->uart_config));

    // Set UART pins
    ESP_ERROR_CHECK(uart_set_pin(_config->port,
                                 _config->tx_pin,
                                 _config->rx_pin,
                                 _config->rts_pin,
                                 _config->cts_pin));
    esp3d_log("Freeheap after BT release %u, %u",
              (unsigned int)esp_get_free_heap_size(),
              (unsigned int)heap_caps_get_free_size(MALLOC_CAP_8BIT | MALLOC_CAP_INTERNAL));

    // Create RX task
    _started       = true;
    BaseType_t res = xTaskCreatePinnedToCore(esp3d_serial_rx_task,
                                             "esp3d_serial_rx_task",
                                             _config->task_stack_size,
                                             NULL,
                                             _config->task_priority,
                                             &_xHandle,
                                             _config->task_core);

    if (res == pdPASS && _xHandle)
    {
        esp3d_log("Created Serial Task");
        esp3d_log("Serial client started");
        flush();
        return true;
    }
    else
    {
        esp3d_log_e("Serial Task creation failed");
        _started = false;
        return false;
    }
}

bool ESP3DSerialClient::pushMsgToRxQueue(const uint8_t *msg, size_t size)
{
    ESP3DMessage *newMsgPtr = newMsg();
    if (newMsgPtr)
    {
        if (ESP3DClient::setDataContent(newMsgPtr, msg, size))
        {
#if ESP3D_DISABLE_SERIAL_AUTHENTICATION_FEATURE
            newMsgPtr->authentication_level = ESP3DAuthenticationLevel::admin;
#endif  // ESP3D_DISABLE_SERIAL_AUTHENTICATION
            newMsgPtr->origin = ESP3DClientType::serial;
            newMsgPtr->type   = ESP3DMessageType::unique;
            if (!addRxData(newMsgPtr))
            {
                // delete message as cannot be added to the queue
                ESP3DClient::deleteMsg(newMsgPtr);
                esp3d_log_e("Failed to add message to rx queue");
                return false;
            }
        }
        else
        {
            // delete message as cannot be added partially filled to the queue
            free(newMsgPtr);
            esp3d_log_e("Message creation failed");
            return false;
        }
    }
    else
    {
        esp3d_log_e("Out of memory!");
        return false;
    }
    return true;
}

void ESP3DSerialClient::handle()
{
    if (_started)
    {
        if (getRxMsgsCount() > 0)
        {
            ESP3DMessage *msg = popRx();
            if (msg)
            {
                esp3dCommands.process(msg);
            }
        }
        if (getTxMsgsCount() > 0)
        {
            ESP3DMessage *msg = popTx();
            if (msg)
            {
                size_t len = uart_write_bytes(_config->port, msg->data, msg->size);
                if (len != msg->size)
                {
                    esp3d_log_e("Error writing message %s", msg->data);
                }
                deleteMsg(msg);
            }
        }
    }
}

void ESP3DSerialClient::flush()
{
    uint8_t loopCount = 10;
    while (loopCount && getTxMsgsCount() > 0)
    {
        // esp3d_log("flushing Tx messages");
        loopCount--;
        handle();
        uart_wait_tx_done(_config->port, pdMS_TO_TICKS(500));
    }
}

void ESP3DSerialClient::end()
{
    if (_started)
    {
        flush();
        _started = false;
        esp3d_log("Clearing queue Rx messages");
        clearRxQueue();
        esp3d_log("Clearing queue Tx messages");
        clearTxQueue();
        esp3d_hal::wait(1000);
        if (pthread_mutex_destroy(&_tx_mutex) != 0)
        {
            esp3d_log_w("Mutex destruction for tx failed");
        }
        if (pthread_mutex_destroy(&_rx_mutex) != 0)
        {
            esp3d_log_w("Mutex destruction for rx failed");
        }
        esp3d_log("Uninstalling Serial drivers");
        if (_config && uart_is_driver_installed(_config->port)
            && uart_driver_delete(_config->port) != ESP_OK)
        {
            esp3d_log_e("Error deleting serial driver");
        }
    }
    if (_xHandle)
    {
        vTaskDelete(_xHandle);
        _xHandle = NULL;
    }
    _bufferPos = 0;
    if (_data)
    {
        free(_data);
        _data = NULL;
    }
    if (_buffer)
    {
        free(_buffer);
        _buffer = NULL;
    }
}
