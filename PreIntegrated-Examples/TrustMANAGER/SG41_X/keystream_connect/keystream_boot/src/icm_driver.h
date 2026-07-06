/*******************************************************************************

  File Name:
    ICM_driver.h

  Summary:
    Integrity Check Monitor (ICM) driver for PIC32CX1025SG41.

  Description:
    Provides SHA-256 digest calculation for memory regions

 *******************************************************************************/

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

#ifndef ICM_DRIVER_H
#define ICM_DRIVER_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// Forward declarations
typedef struct icm_region_desc icm_region_desc_t;

// Error codes
typedef enum {
    ICM_SUCCESS = 0,
    ICM_ERROR_INVALID_PARAM,
    ICM_ERROR_NOT_INITIALIZED,
    ICM_ERROR_BUSY,
    ICM_ERROR_TIMEOUT,
    ICM_ERROR_ALIGNMENT
} icm_result_t;

/**
 * @brief Initialize the ICM driver
 * @param region_desc Pointer to region descriptor (must be 64-byte aligned)
 * @param hash_area Pointer to hash storage area (must be 128-byte aligned)
 * @return ICM_SUCCESS on success, error code otherwise
 */
icm_result_t icm_init(icm_region_desc_t* region_desc, uint32_t* hash_area);

/**
 * @brief Calculate SHA-256 digest of a memory region
 * @param start_addr Start address of memory region
 * @param length Length in bytes (must be multiple of 4)
 * @param digest Output buffer for 32-byte SHA-256 digest
 * @return ICM_SUCCESS on success, error code otherwise
 */
icm_result_t icm_calculate_sha256(uint32_t start_addr, uint32_t length, uint8_t* digest);

/**
 * @brief Get ICM status
 * @return Current ICM status register value
 */
uint32_t icm_get_status(void);

/**
 * @brief Check if ICM is busy
 * @return true if ICM is currently enabled/busy
 */
bool icm_is_busy(void);

/**
 * @brief Disable ICM module
 */
void icm_disable(void);

/**
 * @brief Deinitialize ICM driver
 */
void icm_deinit(void);

#ifdef __cplusplus
}
#endif

#endif // ICM_DRIVER_H