#ifndef SYS_FLASH_TEST_H
#define SYS_FLASH_TEST_H

#include <stdint.h>
#include <stdbool.h>
#include "app_wincs02.h"
#include "driver/sst26/drv_sst26.h"

// *****************************************************************************
// Data Types
// *****************************************************************************

typedef enum {
    SYS_FLASH_STATE_INIT = 0,
    SYS_FLASH_STATE_OPEN,
    SYS_FLASH_STATE_ERASE,
    SYS_FLASH_STATE_WAIT_ERASE,
    SYS_FLASH_STATE_WRITE,
    SYS_FLASH_STATE_WAIT_WRITE,
    SYS_FLASH_STATE_READ,
    SYS_FLASH_STATE_WAIT_READ,
    SYS_FLASH_STATE_VERIFY,
    SYS_FLASH_STATE_DONE,
    SYS_FLASH_STATE_ERROR,
} SYS_FLASH_STATES;

// *****************************************************************************
// Function Declarations
// *****************************************************************************

/**
 * @brief Initializes the SST26 flash test routine.
 * 
 * Call this once during system init if needed.
 */
void SYS_Flash_Initialize(void);

/**
 * @brief Flash task state machine.
 * 
 * Call periodically from SYS_Tasks() or your app task to drive the flash test.
 */
void SYS_Flash_Tasks(void);

/**
 * @brief Returns true if flash operation was successful and complete.
 * 
 * @return true if done and successful.
 * @return false otherwise.
 */
bool SYS_Flash_IsDone(void);

/**
 * @brief Returns true if an error occurred during flash operation.
 * 
 * @return true if error occurred.
 * @return false otherwise.
 */
bool SYS_Flash_HasError(void);

#endif // SYS_FLASH_TEST_H
