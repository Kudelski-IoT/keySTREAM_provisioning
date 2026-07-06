/*******************************************************************************

  File Name:
    ICM_driver.c

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

#include <stdint.h>
#include <stdbool.h>
#include <string.h>

// ICM Register Base Address (this is base address for PIC32CX1025SG41)
#define ICM_BASE_ADDR           0x42002C00UL

// ICM Register Offsets
#define ICM_CFG_OFFSET          0x00
#define ICM_CTRL_OFFSET         0x04
#define ICM_SR_OFFSET           0x08
#define ICM_IER_OFFSET          0x10
#define ICM_IDR_OFFSET          0x14
#define ICM_IMR_OFFSET          0x18
#define ICM_ISR_OFFSET          0x1C
#define ICM_UASR_OFFSET         0x20
#define ICM_DSCR_OFFSET         0x30
#define ICM_HASH_OFFSET         0x34

// ICM Register Pointers
#define ICM_CFG                 (*(volatile uint32_t*)(ICM_BASE_ADDR + ICM_CFG_OFFSET))
#define ICM_CTRL                (*(volatile uint32_t*)(ICM_BASE_ADDR + ICM_CTRL_OFFSET))
#define ICM_SR                  (*(volatile uint32_t*)(ICM_BASE_ADDR + ICM_SR_OFFSET))
#define ICM_IER                 (*(volatile uint32_t*)(ICM_BASE_ADDR + ICM_IER_OFFSET))
#define ICM_IDR                 (*(volatile uint32_t*)(ICM_BASE_ADDR + ICM_IDR_OFFSET))
#define ICM_IMR                 (*(volatile uint32_t*)(ICM_BASE_ADDR + ICM_IMR_OFFSET))
#define ICM_ISR                 (*(volatile uint32_t*)(ICM_BASE_ADDR + ICM_ISR_OFFSET))
#define ICM_UASR                (*(volatile uint32_t*)(ICM_BASE_ADDR + ICM_UASR_OFFSET))
#define ICM_DSCR                (*(volatile uint32_t*)(ICM_BASE_ADDR + ICM_DSCR_OFFSET))
#define ICM_HASH                (*(volatile uint32_t*)(ICM_BASE_ADDR + ICM_HASH_OFFSET))

// ICM Control Register Bits
#define ICM_CTRL_ENABLE         (1UL << 0)
#define ICM_CTRL_DISABLE        (1UL << 1)
#define ICM_CTRL_SWRST          (1UL << 2)
#define ICM_CTRL_REHASH         (1UL << 4)

// ICM Configuration Register Bits
#define ICM_CFG_WBDIS           (1UL << 0)
#define ICM_CFG_EOMDIS          (1UL << 1)
#define ICM_CFG_SLBDIS          (1UL << 2)
#define ICM_CFG_BBC_POS         4
#define ICM_CFG_BBC_MASK        (0xFUL << ICM_CFG_BBC_POS)
#define ICM_CFG_ASCD            (1UL << 8)
#define ICM_CFG_DUALBUFF        (1UL << 9)
#define ICM_CFG_UIHASH          (1UL << 12)
#define ICM_CFG_UALGO_POS       13
#define ICM_CFG_UALGO_MASK      (0x7UL << ICM_CFG_UALGO_POS)
#define ICM_CFG_HAPROT_POS      16
#define ICM_CFG_HAPROT_MASK     (0x3FUL << ICM_CFG_HAPROT_POS)
#define ICM_CFG_DAPROT_POS      24
#define ICM_CFG_DAPROT_MASK     (0x3FUL << ICM_CFG_DAPROT_POS)

// ICM Status Register Bits
#define ICM_SR_ENABLE           (1UL << 0)
#define ICM_SR_RAWRINS_POS      1
#define ICM_SR_RAWRINS_MASK     (0xFUL << ICM_SR_RAWRINS_POS)
#define ICM_SR_RAWS_POS         8
#define ICM_SR_RAWS_MASK        (0xFUL << ICM_SR_RAWS_POS)

// ICM Interrupt Bits
#define ICM_INT_RHC_POS         0
#define ICM_INT_RHC_MASK        (0xFUL << ICM_INT_RHC_POS)
#define ICM_INT_RDM_POS         4
#define ICM_INT_RDM_MASK        (0xFUL << ICM_INT_RDM_POS)
#define ICM_INT_RBE_POS         8
#define ICM_INT_RBE_MASK        (0xFUL << ICM_INT_RBE_POS)
#define ICM_INT_RWC_POS         12
#define ICM_INT_RWC_MASK        (0xFUL << ICM_INT_RWC_POS)
#define ICM_INT_REC_POS         16
#define ICM_INT_REC_MASK        (0xFUL << ICM_INT_REC_POS)
#define ICM_INT_RSU_POS         20
#define ICM_INT_RSU_MASK        (0xFUL << ICM_INT_RSU_POS)
#define ICM_INT_URAD            (1UL << 24)

// Algorithm Selection
#define ICM_ALGO_SHA1           0x0
#define ICM_ALGO_SHA256         0x1
#define ICM_ALGO_SHA224         0x4

// ICM Region Descriptor Structure (must be 64-byte aligned)
typedef struct __attribute__((packed, aligned(64))) {
    uint32_t raddr;         // Region Start Address
    uint32_t rcfg;          // Region Configuration
    uint32_t rctrl;         // Region Control
    uint32_t rnext;         // Next Region Descriptor Address
} icm_region_desc_t;

// Region Configuration Register Bits
#define ICM_RCFG_CDWBN          (1UL << 0)
#define ICM_RCFG_WRAP           (1UL << 1)
#define ICM_RCFG_EOM            (1UL << 2)
#define ICM_RCFG_RHIEN          (1UL << 4)
#define ICM_RCFG_DMIEN          (1UL << 5)
#define ICM_RCFG_BEIEN          (1UL << 6)
#define ICM_RCFG_WCIEN          (1UL << 7)
#define ICM_RCFG_ECIEN          (1UL << 8)
#define ICM_RCFG_SUIEN          (1UL << 9)
#define ICM_RCFG_PROCDLY_POS    24
#define ICM_RCFG_PROCDLY_MASK   (0xFFUL << ICM_RCFG_PROCDLY_POS)

// Region Control Register Bits
#define ICM_RCTRL_TRSIZE_POS    0
#define ICM_RCTRL_TRSIZE_MASK   (0xFFFFUL << ICM_RCTRL_TRSIZE_POS)

// Error codes
typedef enum {
    ICM_SUCCESS = 0,
    ICM_ERROR_INVALID_PARAM,
    ICM_ERROR_NOT_INITIALIZED,
    ICM_ERROR_BUSY,
    ICM_ERROR_TIMEOUT,
    ICM_ERROR_ALIGNMENT
} icm_result_t;

// ICM Driver State
typedef struct {
    bool initialized;
    icm_region_desc_t* region_desc;
    uint32_t* hash_area;
    volatile bool operation_complete;
} icm_driver_t;

static icm_driver_t icm_driver = {0};

/**
 * @brief Initialize the ICM driver
 * @param region_desc Pointer to region descriptor (must be 64-byte aligned)
 * @param hash_area Pointer to hash storage area (must be 128-byte aligned)
 * @return ICM_SUCCESS on success, error code otherwise
 */
icm_result_t icm_init(icm_region_desc_t* region_desc, uint32_t* hash_area) {
    if (!region_desc || !hash_area) {
        return ICM_ERROR_INVALID_PARAM;
    }
    
    // Check alignment requirements
    if (((uint32_t)region_desc & 0x3F) != 0) {
        return ICM_ERROR_ALIGNMENT;
    }
    
    if (((uint32_t)hash_area & 0x7F) != 0) {
        return ICM_ERROR_ALIGNMENT;
    }
    
    // Store pointers
    icm_driver.region_desc = region_desc;
    icm_driver.hash_area = hash_area;
    
    // Perform software reset
    ICM_CTRL = ICM_CTRL_SWRST;
    
    // Wait for reset to complete
    while (ICM_SR & ICM_SR_ENABLE);
    
    // Configure ICM
    ICM_CFG = ICM_CFG_UALGO_MASK & (ICM_ALGO_SHA256 << ICM_CFG_UALGO_POS);
    
    // Set descriptor and hash area addresses
    ICM_DSCR = (uint32_t)region_desc;
    ICM_HASH = (uint32_t)hash_area;
    
    icm_driver.initialized = true;
    icm_driver.operation_complete = false;
    
    return ICM_SUCCESS;
}

/**
 * @brief Calculate SHA-256 digest of a memory region
 * @param start_addr Start address of memory region
 * @param length Length in bytes (must be multiple of 4)
 * @param digest Output buffer for 32-byte SHA-256 digest
 * @return ICM_SUCCESS on success, error code otherwise
 */
icm_result_t icm_calculate_sha256(uint32_t start_addr, uint32_t length, uint8_t* digest) {
    if (!icm_driver.initialized) {
        return ICM_ERROR_NOT_INITIALIZED;
    }
    
    if (!digest || length == 0 || (length & 0x3) != 0) {
        return ICM_ERROR_INVALID_PARAM;
    }
    
    if (ICM_SR & ICM_SR_ENABLE) {
        return ICM_ERROR_BUSY;
    }
    
    // Clear hash area
    memset(icm_driver.hash_area, 0, 32);
    
    // Configure region descriptor
    icm_driver.region_desc->raddr = start_addr;
    icm_driver.region_desc->rcfg = ICM_RCFG_EOM;  // End of monitoring
    icm_driver.region_desc->rctrl = (length >> 2) & ICM_RCTRL_TRSIZE_MASK;  // Size in 32-bit words
    icm_driver.region_desc->rnext = 0;  // No next region
    
    // Clear any pending interrupts
    ICM_ISR;
    
    icm_driver.operation_complete = false;
    
    // Enable ICM
    ICM_CTRL = ICM_CTRL_ENABLE;
    
    // Wait for completion (polling mode)
    uint32_t timeout = 1000000;  // Adjust timeout as needed
    while (--timeout && !icm_driver.operation_complete) {
        // Check for hash complete interrupt
        uint32_t isr = ICM_ISR;
        if (isr & (ICM_INT_RHC_MASK & (1UL << ICM_INT_RHC_POS))) {
            icm_driver.operation_complete = true;
            break;
        }
        
        // Check for errors
        if (isr & (ICM_INT_RDM_MASK | ICM_INT_RBE_MASK | ICM_INT_RWC_MASK | 
                   ICM_INT_REC_MASK | ICM_INT_RSU_MASK | ICM_INT_URAD)) {
            // Disable ICM on error
            ICM_CTRL = ICM_CTRL_DISABLE;
            return ICM_ERROR_TIMEOUT;
        }
    }
    
    if (!icm_driver.operation_complete) {
        ICM_CTRL = ICM_CTRL_DISABLE;
        return ICM_ERROR_TIMEOUT;
    }
    
    // Disable ICM
    ICM_CTRL = ICM_CTRL_DISABLE;
    
    // Copy digest from hash area
    memcpy(digest, icm_driver.hash_area, 32);
    
    return ICM_SUCCESS;
}

/**
 * @brief Get ICM status
 * @return Current ICM status register value
 */
uint32_t icm_get_status(void) {
    return ICM_SR;
}

/**
 * @brief Check if ICM is busy
 * @return true if ICM is currently enabled/busy
 */
bool icm_is_busy(void) {
    return (ICM_SR & ICM_SR_ENABLE) != 0;
}

/**
 * @brief Disable ICM module
 */
void icm_disable(void) {
    ICM_CTRL = ICM_CTRL_DISABLE;
    icm_driver.operation_complete = false;
}

/**
 * @brief Deinitialize ICM driver
 */
void icm_deinit(void) {
    icm_disable();
    icm_driver.initialized = false;
    icm_driver.region_desc = NULL;
    icm_driver.hash_area = NULL;
}