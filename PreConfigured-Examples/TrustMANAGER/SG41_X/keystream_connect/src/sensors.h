/**
 * @file sensors.h
 * @brief Interface for MCP9804 temperature and OPT3001 light sensors.
 *
 * This header defines functions and types to communicate with MCP9804 and OPT3001
 * I2C sensors for temperature and ambient light measurement respectively.
 *
 * Author: Adithya Vishnu Sankar
 */

#ifndef SENSORS_H
#define SENSORS_H

#include <stdint.h>
#include <stdbool.h>

/**
 * @brief I2C address for MCP9804 temperature sensor (7-bit format)
 */
#define MCP9804_I2C_ADDR  0x18

/**
 * @brief I2C address for OPT3001 light sensor (7-bit format)
 */
#define OPT3001_I2C_ADDR  0x44

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Status codes returned by sensor driver functions.
 */
typedef enum {
    SENSOR_STATUS_OK = 0,            /**< Operation successful */
    SENSOR_STATUS_I2C_WRITE_ERROR,  /**< I2C write failed */
    SENSOR_STATUS_I2C_READ_ERROR,   /**< I2C read failed */
    SENSOR_STATUS_NULL_POINTER,     /**< NULL pointer passed to function */
    SENSOR_STATUS_UNKNOWN_ERROR     /**< Unspecified failure */
} SensorStatus;

/**
 * @brief Reads the temperature from MCP9804.
 *
 * @param[out] outTemp Pointer to float where temperature in °C will be stored.
 * @return SENSOR_STATUS_OK if successful, error code otherwise.
 */
SensorStatus MCP9804_ReadTemperature(float *outTemp);

/**
 * @brief Test function that prints MCP9804 temperature reading.
 */
void test_temp_sensor(void);

/**
 * @brief Writes a 16-bit value to an OPT3001 register.
 *
 * @param reg Register address to write.
 * @param value Data to write.
 * @return SENSOR_STATUS_OK if successful, error code otherwise.
 */
SensorStatus OPT3001_WriteRegister(uint8_t reg, uint16_t value);

/**
 * @brief Reads a 16-bit value from an OPT3001 register.
 *
 * @param reg Register address to read from.
 * @param[out] value Pointer to store the read value.
 * @return SENSOR_STATUS_OK if successful, error code otherwise.
 */
SensorStatus OPT3001_ReadRegister(uint8_t reg, uint16_t *value);

/**
 * @brief Reads ambient light from OPT3001 in lux.
 *
 * @param[out] outLux Pointer to float where lux value will be stored.
 * @return SENSOR_STATUS_OK if successful, error code otherwise.
 */
SensorStatus OPT3001_ReadLux(float *outLux);

/**
 * @brief Test function that performs OPT3001 readings.
 */
void test_light_sensor(void);

/**
 * @brief Prints a formatted test header.
 *
 * @param header String to print as a section header.
 */
void print_test_header(const char *header);

#ifdef __cplusplus
}
#endif

#endif // SENSORS_H
