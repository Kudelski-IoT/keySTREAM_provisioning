/**
 * @file sensors.c
 * @brief Implementation of MCP9804 and OPT3001 sensor communication.
 *
 * This file provides implementations for reading from and writing to
 * MCP9804 temperature sensor and OPT3001 ambient light sensor via I2C.
 *
 * Author: Adithya Vishnu Sankar
 */

#include "sensors.h"
#include <stdio.h>
#include "definitions.h"  
#include "app_wincs02.h"

SensorStatus MCP9804_ReadTemperature(float *outTemp)
{
    if (outTemp == NULL) return SENSOR_STATUS_NULL_POINTER;

    uint8_t reg = 0x05;
    uint8_t tempData[2] = {0};

    if (!SERCOM2_I2C_Write(MCP9804_I2C_ADDR, &reg, 1))
    {
        printf("MCP9804 I2C Write Error!\n");
        return SENSOR_STATUS_I2C_WRITE_ERROR;
    }

    while (SERCOM2_I2C_IsBusy());

    if (!SERCOM2_I2C_Read(MCP9804_I2C_ADDR, tempData, 2))
    {
        printf("MCP9804 I2C Read Error!\n");
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

void test_temp_sensor(void)
{
    print_test_header("Sampling Temperature sensor");

    float temp = 0;
    SensorStatus status = MCP9804_ReadTemperature(&temp);
    if (status == SENSOR_STATUS_OK)
        SYS_CONSOLE_PRINT("Temperature: %.2f C\n\r", temp);
    else
        SYS_CONSOLE_PRINT("Temperature sensor error: %d\n\r", status);
}

SensorStatus OPT3001_WriteRegister(uint8_t reg, uint16_t value)
{
    uint8_t data[3] = {reg, (value >> 8) & 0xFF, value & 0xFF};

    if (!SERCOM2_I2C_Write(OPT3001_I2C_ADDR, data, 3))
    {
        SYS_CONSOLE_PRINT("OPT3001 I2C Write Error!\n");
        return SENSOR_STATUS_I2C_WRITE_ERROR;
    }

    while (SERCOM2_I2C_IsBusy());
    return SENSOR_STATUS_OK;
}

SensorStatus OPT3001_ReadRegister(uint8_t reg, uint16_t *value)
{
    if (value == NULL) return SENSOR_STATUS_NULL_POINTER;

    uint8_t data[2] = {0};

    if (!SERCOM2_I2C_Write(OPT3001_I2C_ADDR, &reg, 1))
    {
        SYS_CONSOLE_PRINT("OPT3001 I2C Write Error!\n");
        return SENSOR_STATUS_I2C_WRITE_ERROR;
    }

    while (SERCOM2_I2C_IsBusy());

    if (!SERCOM2_I2C_Read(OPT3001_I2C_ADDR, data, 2))
    {
        SYS_CONSOLE_PRINT("OPT3001 I2C Read Error!\n");
        return SENSOR_STATUS_I2C_READ_ERROR;
    }

    while (SERCOM2_I2C_IsBusy());

    *value = (data[0] << 8) | data[1];
    return SENSOR_STATUS_OK;
}

SensorStatus OPT3001_ReadLux(float *outLux)
{
    if (outLux == NULL) return SENSOR_STATUS_NULL_POINTER;

    uint16_t rawData;
    SensorStatus status = OPT3001_ReadRegister(0x00, &rawData);
    if (status != SENSOR_STATUS_OK)
        return status;

    uint8_t exponent = (rawData >> 12) & 0x0F;
    uint16_t mantissa = rawData & 0x0FFF;

    *outLux = mantissa * 0.01 * (1 << exponent);
    return SENSOR_STATUS_OK;
}

void test_light_sensor(void)
{
    print_test_header("Sampling Light sensor");

    OPT3001_WriteRegister(0x01, 0xCA10);

    float lux = 0;
    SensorStatus status = OPT3001_ReadLux(&lux);
    if (status == SENSOR_STATUS_OK)
        SYS_CONSOLE_PRINT("Ambient Light: %.2f lux\n\n\r", lux);
    else
        SYS_CONSOLE_PRINT("Light sensor error: %d\n\n\r", status);
}

void print_test_header(const char *header)
{
    SYS_CONSOLE_PRINT("\r\n==== %s ====\n\r", header);
}
