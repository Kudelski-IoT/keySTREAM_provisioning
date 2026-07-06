/*******************************************************************************
* Copyright (C) 2025 Microchip Technology Inc. and its subsidiaries.
*
* Subject to your compliance with these terms, you may use Microchip software
* and any derivatives exclusively with Microchip products. It is your
* responsibility to comply with third party license terms applicable to your
* use of third party software (including open source software) that may
* accompany Microchip software.
*
* THIS SOFTWARE IS SUPPLIED BY MICROCHIP "AS IS". NO WARRANTIES, WHETHER
* EXPRESS, IMPLIED OR STATUTORY, APPLY TO THIS SOFTWARE, INCLUDING ANY IMPLIED
* WARRANTIES OF NON-INFRINGEMENT, MERCHANTABILITY, AND FITNESS FOR A
* PARTICULAR PURPOSE.
*
* IN NO EVENT WILL MICROCHIP BE LIABLE FOR ANY INDIRECT, SPECIAL, PUNITIVE,
* INCIDENTAL OR CONSEQUENTIAL LOSS, DAMAGE, COST OR EXPENSE OF ANY KIND
* WHATSOEVER RELATED TO THE SOFTWARE, HOWEVER CAUSED, EVEN IF MICROCHIP HAS
* BEEN ADVISED OF THE POSSIBILITY OR THE DAMAGES ARE FORESEEABLE. TO THE
* FULLEST EXTENT ALLOWED BY LAW, MICROCHIP'S TOTAL LIABILITY ON ALL CLAIMS IN
* ANY WAY RELATED TO THIS SOFTWARE WILL NOT EXCEED THE AMOUNT OF FEES, IF ANY,
* THAT YOU HAVE PAID DIRECTLY TO MICROCHIP FOR THIS SOFTWARE.
*******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include "app_sensors.h"
#include "definitions.h"
#include "app_wincs02.h"
#include "system/time/sys_time.h"


#define SENSORS_DEBUG_ENABLED
#ifdef SENSORS_DEBUG_ENABLED
/** @brief Enable downloader debug logs. */
#define APP_SENSORS_DEBUG(fmt, ...)         printf("[APP_SENSORS]: " fmt "\r\n", ##__VA_ARGS__)
#define APP_SENSORS_DEBUG_GREEN(fmt, ...)   printf("\x1B[32m[APP_SENSORS]: " fmt "\r\n\x1B[0m", ##__VA_ARGS__)
#define APP_SENSORS_DEBUG_ERROR(fmt, ...)   printf("\x1B[31m[APP_SENSORS <%d>]: " fmt "\r\n\x1B[0m", __LINE__, ##__VA_ARGS__)
#else
#define APP_SENSORS_DEBUG(fmt, ...)
#define APP_SENSORS_DEBUG_GREEN(fmt, ...)
#define APP_SENSORS_DEBUG_ERROR(fmt, ...)
#endif /* DEBUG. */

SYS_TIME_HANDLE sensors_timer_handle;

static SENSORS_APP_DATA sensors_app_data;
bool is_sensors_state_timedout;

static SENSORS_STATUS read_temperate_MCP9804(float *outTemp);
static void read_temp_sensor(void);
static SENSORS_STATUS write_register_OPT3001(uint8_t reg, uint16_t value);
static SENSORS_STATUS read_register_OPT3001(uint8_t reg, uint16_t *value);
static SENSORS_STATUS read_lux_OPT3001(float *outLux);
static void read_light_sensor(void);
static void sensors_timer_callback(uintptr_t context);

void APP_SENSORS_SetState(SENSORS_APP_STATES state)
{
    sensors_app_data.state = state;
}

void APP_SENSORS_Initialize(void)
{
    sensors_app_data.state = SENSORS_APP_STATE_INIT;
}

bool is_patch_version_even(void)
{
    const char *last_dot = strrchr(APP_COMPONENT_VERSION, '.');
    if (!last_dot || !*(last_dot + 1)) return false; // No patch part

    long patch = strtol(last_dot + 1, NULL, 10);

    // Check if patch is even
    return (patch % 2) == 0;
}

void APP_SENSORS_Tasks(void)
{
    switch(sensors_app_data.state)
    {
        case SENSORS_APP_STATE_INIT:
        {
            sensors_timer_handle = SYS_TIME_CallbackRegisterMS(sensors_timer_callback, 0, 10000, SYS_TIME_PERIODIC);
            sensors_app_data.state = SENSORS_APP_STATE_RUN;
        }

        case SENSORS_APP_STATE_RUN:
        {
            if (is_sensors_state_timedout) {
                is_sensors_state_timedout = false;

                // Alternate between reading light and temperature sensors based on patch version
                if (is_patch_version_even()) {
                    read_light_sensor();
                } else {
                    read_temp_sensor();
                }
            }
            break;
        }

        case SENSORS_APP_STATE_IDLE:
        {
            // Idle state; waiting for timer callback
            break;
        }

        /* The default state should never be executed. */
        default:
        {
            /* TODO: Handle error in application's state machine. */
            break;
        }
    }
}

static SENSORS_STATUS read_temperate_MCP9804(float *outTemp)
{
    if (outTemp == NULL) return SENSOR_STATUS_NULL_POINTER;

    uint8_t reg = 0x05;
    uint8_t tempData[2] = {0};

    if (!SERCOM2_I2C_Write(MCP9804_I2C_ADDR, &reg, 1))
    {
        APP_SENSORS_DEBUG_ERROR("MCP9804 I2C Write Error!\n");
        return SENSOR_STATUS_I2C_WRITE_ERROR;
    }

    while (SERCOM2_I2C_IsBusy());

    if (!SERCOM2_I2C_Read(MCP9804_I2C_ADDR, tempData, 2))
    {
        APP_SENSORS_DEBUG_ERROR("MCP9804 I2C Read Error!\n");
        return SENSOR_STATUS_I2C_READ_ERROR;
    }

    while (SERCOM2_I2C_IsBusy());

    uint16_t rawTemp = (tempData[0] << 8) | tempData[1];
    rawTemp &= 0x1FFF;

    float temperature = (rawTemp & 0x0FFF) / 16.0;
    if (rawTemp & 0x1000)
        temperature -= 256;

    *outTemp = temperature;
    return SENSOR_STATUS_OK;
}

static void read_temp_sensor(void)
{
    float temp = 0;
    SENSORS_STATUS status = read_temperate_MCP9804(&temp);
    if (status == SENSOR_STATUS_OK){
        APP_SENSORS_DEBUG_GREEN("Temperature: %.2f C", temp);
    }
    else
        APP_SENSORS_DEBUG_ERROR("Temperature Error(%d)", status);
}

static SENSORS_STATUS write_register_OPT3001(uint8_t reg, uint16_t value)
{
    uint8_t data[3] = {reg, (value >> 8) & 0xFF, value & 0xFF};

    if (!SERCOM2_I2C_Write(OPT3001_I2C_ADDR, data, 3))
    {
        APP_SENSORS_DEBUG_ERROR("OPT3001 I2C Write Error!\n");
        return SENSOR_STATUS_I2C_WRITE_ERROR;
    }

    while (SERCOM2_I2C_IsBusy());
    return SENSOR_STATUS_OK;
}

static SENSORS_STATUS read_register_OPT3001(uint8_t reg, uint16_t *value)
{
    if (value == NULL) return SENSOR_STATUS_NULL_POINTER;

    uint8_t data[2] = {0};

    if (!SERCOM2_I2C_Write(OPT3001_I2C_ADDR, &reg, 1))
    {
        APP_SENSORS_DEBUG_ERROR("OPT3001 I2C Write Error!\n");
        return SENSOR_STATUS_I2C_WRITE_ERROR;
    }

    while (SERCOM2_I2C_IsBusy());

    if (!SERCOM2_I2C_Read(OPT3001_I2C_ADDR, data, 2))
    {
        APP_SENSORS_DEBUG_ERROR("OPT3001 I2C Read Error!\n");
        return SENSOR_STATUS_I2C_READ_ERROR;
    }

    while (SERCOM2_I2C_IsBusy());

    *value = (data[0] << 8) | data[1];
    return SENSOR_STATUS_OK;
}

static SENSORS_STATUS read_lux_OPT3001(float *outLux)
{
    if (outLux == NULL) return SENSOR_STATUS_NULL_POINTER;

    uint16_t rawData;
    SENSORS_STATUS status = read_register_OPT3001(0x00, &rawData);
    if (status != SENSOR_STATUS_OK)
        return status;

    uint8_t exponent = (rawData >> 12) & 0x0F;
    uint16_t mantissa = rawData & 0x0FFF;

    *outLux = mantissa * 0.01 * (1 << exponent);
    return SENSOR_STATUS_OK;
}

static void read_light_sensor(void)
{
    write_register_OPT3001(0x01, 0xCA10);

    float lux = 0;
    SENSORS_STATUS status = read_lux_OPT3001(&lux);
    if (status == SENSOR_STATUS_OK){
        APP_SENSORS_DEBUG_GREEN("Light:  %.2f lux", lux);
    }
    else
        APP_SENSORS_DEBUG_ERROR("Light Error(%d)", status);
}

static void sensors_timer_callback(uintptr_t context)
{
    is_sensors_state_timedout = true;
}
