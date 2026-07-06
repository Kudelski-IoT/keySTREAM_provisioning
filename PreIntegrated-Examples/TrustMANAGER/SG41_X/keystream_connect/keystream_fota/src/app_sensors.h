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

#ifndef _APP_SENSORS_H
#define _APP_SENSORS_H

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
} SENSORS_STATUS;

typedef enum
{
    /* Application's state machine's initial state. */
    SENSORS_APP_STATE_IDLE,
    SENSORS_APP_STATE_INIT,
    SENSORS_APP_STATE_RUN,
    /* TODO: Define states used by the application state machine. */

} SENSORS_APP_STATES;

typedef struct
{
    /* The application's current state */
    SENSORS_APP_STATES state;

    /* TODO: Define any additional data used by the application. */

} SENSORS_APP_DATA;


/**
 * @brief Sensors application initialization.
 *
 * @param[in] context Pointer to timer content if any.
 */
void APP_SENSORS_Initialize(void);

/**
 * @brief Executes the Sensors application tasks.
 *
 * @param[in] context Pointer to timer content if any.
 */
void APP_SENSORS_Tasks(void);

void APP_SENSORS_SetState(SENSORS_APP_STATES state);

#ifdef __cplusplus
}
#endif

#endif // _APP_SENSORS_H
