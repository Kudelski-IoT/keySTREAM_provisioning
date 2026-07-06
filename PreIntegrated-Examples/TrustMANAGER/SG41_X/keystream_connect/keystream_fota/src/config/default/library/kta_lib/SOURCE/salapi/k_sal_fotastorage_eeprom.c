/*******************************************************************************
*************************keySTREAM Trusted Agent ("KTA")************************

* (c) 2023-2024 Nagravision Sàrl

* Subject to your compliance with these terms, you may use the Nagravision Sàrl
* Software and any derivatives exclusively with Nagravision's products. It is your
* responsibility to comply with third party license terms applicable to your
* use of third party software (including open source software) that may accompany
* Nagravision Software.

* Redistribution of this Nagravision Software in source or binary form is allowed
* and must include the above terms of use and the following disclaimer with the
* distribution and accompanying materials.

* THIS SOFTWARE IS SUPPLIED BY NAGRAVISION "AS IS". NO WARRANTIES, WHETHER EXPRESS,
* IMPLIED OR STATUTORY, APPLY TO THIS SOFTWARE, INCLUDING ANY IMPLIED WARRANTIES OF
* NON-INFRINGEMENT, MERCHANTABILITY, AND FITNESS FOR A PARTICULAR PURPOSE. IN NO
* EVENT WILL NAGRAVISION BE LIABLE FOR ANY INDIRECT, SPECIAL, PUNITIVE, INCIDENTAL
* OR CONSEQUENTIAL LOSS, DAMAGE, COST OR EXPENSE OF ANY KIND WHATSOEVER RELATED TO
* THE SOFTWARE, HOWEVER CAUSED, EVEN IF NAGRAVISION HAS BEEN ADVISED OF THE
* POSSIBILITY OR THE DAMAGES ARE FORESEEABLE. TO THE FULLEST EXTENT ALLOWED BY LAW,
* NAGRAVISION 'S TOTAL LIABILITY ON ALL CLAIMS IN ANY WAY RELATED TO THIS
* SOFTWARE WILL NOT EXCEED THE AMOUNT OF FEES, IF ANY, THAT YOU HAVE PAID DIRECTLY
* TO NAGRAVISION FOR THIS SOFTWARE.
********************************************************************************/
/** \brief  SAL storage for Microchip.
 *
 *  \author Kudelski IoT
 *
 *  \date 2023/06/12
 *
 *  \file k_sal_storage.c
 ******************************************************************************/

/* -------------------------------------------------------------------------- */
/* INCLUDES                                                                   */
/* -------------------------------------------------------------------------- */
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "k_sal_fotastorage.h"
#include "KTALog.h"
#include "peripheral/nvmctrl/plib_nvmctrl.h"


/* -------------------------------------------------------------------------- */
/* LOCAL VARIABLES                                                            */
/* -------------------------------------------------------------------------- */
/**
 * SUPPRESS: MISRA_DEV_KTA_009 : misra_c2012_rule_5.9_violation
 * The identifier gpModuleName is intentionally defined as a common global for logging purposes
 */
#if LOG_KTA_ENABLE != C_KTA_LOG_LEVEL_NONE
static const char* gpModuleName = "KSALFOTASTORAGE";
#endif

/* -------------------------------------------------------------------------- */
/* MACROS                                                                     */
/* -------------------------------------------------------------------------- */

/** @brief Enable EEPROM write verification (read-back check) */
#define EEPROM_WRITE_VERIFICATION_ENABLED                                          (1)

/** @brief Maximum number of write retry attempts */
#define EEPROM_WRITE_MAX_RETRIES                                                   (3)

/** @brief Delay between write retries in microseconds (if needed) */
#define EEPROM_WRITE_RETRY_DELAY_US                                                (100)

/* -------------------------------------------------------------------------- */
/* LOCAL CONSTANTS, TYPES, ENUM                                               */
/* -------------------------------------------------------------------------- */


/* -------------------------------------------------------------------------- */



/* -------------------------------------------------------------------------- */
/* FOTA Storage Block Mapping (Current Implementation)                        */
/* -------------------------------------------------------------------------- */
/*
 * Current FOTA storage uses CRC-protected structures (defined in k_sal_fotastorage.h)
 * with indexed component IDs (0x2010-0x2017) for dynamic component storage.
 *
 * Storage Layout:
 * - 0x00-0x3F:   FOTA Name storage (TFotaNameRecord, 36 bytes with CRC)
 * - 0x40-0x7F:   FOTA Metadata storage (TFotaMetadataRecord, 68 bytes with CRC)
 * - 0x80-0x9F:   FOTA Commit Flag (TFotaCommitRecord, 12 bytes with CRC) - WRITTEN LAST
 * - 0x100+:      Component records (TFotaComponentRecord[8], 558 bytes each with CRC)
 *
 * Each record includes:
 *   - Magic number (4 bytes) - for validity check
 *   - Data fields (variable)
 *   - CRC32 checksum (4 bytes) - for corruption detection
 */

/**
 * @brief FOTA Storage Layer - Dual-Mode Implementation
 * 
 * Automatically detects SmartEEPROM hardware availability and adapts storage strategy.
 * 
 * Storage Modes:
 *   1. SmartEEPROM Mode (Hardware Configured):
 *      - Memory-mapped access to SmartEEPROM (0x44000000)
 *      - Persistent across resets and power cycles
 *      - FOTA campaigns resume after interruption
 *      - Requires SBLK and PSZ fuse configuration
 * 
 *   2. RAM Stub Mode (SmartEEPROM Disabled):
 *      - FOTA state maintained in RAM by fotaprocess.c
 *      - Lost on reset/power cycle - no resume capability
 *      - Read operations return false → KTA uses RAM stub
 *      - Write operations return true (no-op, RAM already updated)
 * 
 * Runtime Detection:
 *   - Reads USER_PAGE fuses for SmartEEPROM configuration
 *   - SBLK=0 and PSZ=0 → SmartEEPROM disabled
 *   - Detection cached after first call for performance
 * 
 * Production Notes:
 *   - Scales to millions of devices with or without SmartEEPROM
 *   - No code changes needed between configurations
 *   - Hardware fuse setting determines behavior
 */

/* Legacy TFotaStorageLayout structure removed - no longer used. 
 * Current implementation uses TFotaComponentRecord with indexed IDs. */

/**
 * @brief SmartEEPROM base pointer
 * 
 * Initialized to NULL. Set to SEEPROM_ADDR (0x44000000) when SmartEEPROM is enabled.
 */
uint8_t *SmartEEPROM8 = NULL;

/** @brief Flag indicating storage mode has been detected and initialized */
static bool gStorageModeInitialized = false;

/* -------------------------------------------------------------------------- */
/* FOTA Storage Address Defines                                               */
/* -------------------------------------------------------------------------- */

/**
 * @brief Detect SmartEEPROM availability from hardware fuses
 * 
 * Reads USER_PAGE fuse configuration to determine if SmartEEPROM is enabled.
 * Result is cached for subsequent calls.
 * 
 * @return true  - SmartEEPROM configured in fuses, persistent storage available
 * @return false - SmartEEPROM disabled, uses RAM stub (no persistence)
 * 
 * @note Detection result determines entire FOTA behavior:
 *       - true:  FOTA resumes after reset
 *       - false: FOTA state lost on reset
 */
static bool isSmartEEPROMAvailable(void)
{
    // Return cached result if already initialized
    if (gStorageModeInitialized)
    {
        return (SmartEEPROM8 != NULL);
    }
    
    // Read fuse configuration from USER_PAGE (same as boot banner)
    uint32_t sblk = ((*(uint32_t *)(USER_PAGE_ADDR + 4)) >> 0) & 0xF;
    uint32_t psz = ((*(uint32_t *)(USER_PAGE_ADDR + 4)) >> 4) & 0x7;
    
    // Check if SmartEEPROM is disabled in fuses (both fields zero)
    if (sblk == 0 && psz == 0)
    {
        M_KTALOG__INFO("SmartEEPROM disabled - using RAM stub only\r\n");
        SmartEEPROM8 = NULL;
    }
    else
    {
        M_KTALOG__INFO("SmartEEPROM enabled (SBLK=%u, PSZ=%u) - using EEPROM storage\r\n", 
                       (unsigned int)sblk, (unsigned int)psz);
        SmartEEPROM8 = (uint8_t *)SEEPROM_ADDR;
    }
    
    gStorageModeInitialized = true;
    return (SmartEEPROM8 != NULL);
}

/** @brief Base address for NAME storage in EEPROM */
#define FOTA_STORAGE_NAME_ADDR                              (0x00)

/** @brief Base address for METADATA storage in EEPROM */
#define FOTA_STORAGE_METADATA_ADDR                          (0x40)

/** @brief Base address for COMMIT FLAG storage in EEPROM */
#define FOTA_STORAGE_COMMIT_ADDR                            (0x80)

/* Legacy gaFotaStorageRecord table removed - no longer used.
 * Current implementation uses dynamic address calculation. */

/* -------------------------------------------------------------------------- */
/* LOCAL FUNCTIONS - IMPLEMENTATION                                           */
/* -------------------------------------------------------------------------- */

/**
 * @brief
 *   Get FOTA storage information (address and size) for given storage ID.
 *
 * @param[in] storageID
 *   Storage identifier (component index, name, metadata, or commit).
 * @param[out] pAddress
 *   Pointer to store the EEPROM address.
 * @param[out] pLength
 *   Pointer to store the storage field length.
 *
 * @return
 * - true if storage ID is valid and info retrieved.
 * - false if storage ID is invalid.
 */
static bool fotaGetStorageInfo
(
  uint32_t   storageID,
  uint32_t*  pAddress,
  size_t*    pLength
)
{
  // Handle indexed component records (0x2010-0x2017)
  if (storageID >= FOTA_STORAGE_COMPONENT_ID_BASE && 
      storageID < (FOTA_STORAGE_COMPONENT_ID_BASE + 8))
  {
    uint32_t compIndex = storageID - FOTA_STORAGE_COMPONENT_ID_BASE;
    // Calculate dynamic offset in EEPROM for component records
    // Store after metadata area at offset 0x100 (256 bytes)
    *pAddress = 0x100 + (compIndex * sizeof(TFotaComponentRecord));
    *pLength = sizeof(TFotaComponentRecord);
    return true;
  }

  // Handle NAME and METADATA storage IDs
  switch (storageID)
  {
    case FOTA_STORAGE_NAME_ID:
      *pAddress = FOTA_STORAGE_NAME_ADDR;
      *pLength = sizeof(TFotaNameRecord);  // CRC-protected name record
      return true;

    case FOTA_STORAGE_METADATA_ID:
      *pAddress = FOTA_STORAGE_METADATA_ADDR;
      *pLength = sizeof(TFotaMetadataRecord);  // CRC-protected metadata record
      return true;

    case FOTA_STORAGE_COMMIT_ID:
      *pAddress = FOTA_STORAGE_COMMIT_ADDR;
      *pLength = sizeof(TFotaCommitRecord);  // Commit flag record
      return true;

    default:
      return false;
  }
}

/**
 * @brief Write data to FOTA storage (SmartEEPROM or RAM stub)
 * 
 * SmartEEPROM Mode:
 *   - Direct memory write to SmartEEPROM (0x44000000)
 *   - Optional read-back verification with retry
 *   - Persistent across power cycles
 * 
 * RAM Stub Mode (SmartEEPROM Disabled):
 *   - Returns success immediately (no-op)
 *   - RAM stub in fotaprocess.c already contains the data
 *   - Data lost on reset - FOTA cannot resume
 * 
 * @param storageID  Storage location identifier
 * @param pData      Data buffer to write
 * @param dataLen    Number of bytes to write
 * 
 * @return true on success (both modes always return true)
 * @return false on invalid parameters or write failure
 * 
 * @note Production-ready: Works seamlessly with/without SmartEEPROM hardware
 */
bool salFotaStorageWrite
(
  uint32_t        storageID,
  const uint8_t*  pData,
  size_t          dataLen
)
{
  uint32_t address;
  size_t fieldLen;
  uint8_t retryCount = 0;
  bool writeSuccess = false;
  
  M_KTALOG__START("salFotaStorageWrite Start - ID: 0x%04X, Len: %u", (unsigned int)storageID, (unsigned int)dataLen);
  
  if (!fotaGetStorageInfo(storageID, &address, &fieldLen))
  {
    M_KTALOG__ERR("[ERR] Invalid FOTA storage ID: 0x%04X\r\n", (unsigned int)storageID);
    return false;
  }

  if (dataLen > fieldLen)
  {
    M_KTALOG__ERR("[ERR] Data length %u exceeds field length %u\r\n", (unsigned int)dataLen, (unsigned int)fieldLen);
    return false;
  }

  if (pData == NULL)
  {
    M_KTALOG__ERR("[ERR] NULL data pointer\r\n");
    return false;
  }

  // Check storage mode and use appropriate method
  if (isSmartEEPROMAvailable())
  {
    // SmartEEPROM mode - use direct memory access
    // Write with retry and verification
    while (retryCount < EEPROM_WRITE_MAX_RETRIES && !writeSuccess)
    {
      // Perform write operation
      memcpy(&SmartEEPROM8[address], pData, dataLen);
      
#if EEPROM_WRITE_VERIFICATION_ENABLED
      // Memory barrier to ensure write completes before read
      __DSB();
      __ISB();
      
      // Read-back verification
      bool verificationPassed = true;
    for (size_t i = 0; i < dataLen; i++)
    {
      if (SmartEEPROM8[address + i] != pData[i])
      {
        verificationPassed = false;
        M_KTALOG__ERR("[ERR] Write verification failed at offset %u: wrote 0x%02X, read 0x%02X\r\n",
                      (unsigned int)i, pData[i], SmartEEPROM8[address + i]);
        break;
      }
    }
    
    if (verificationPassed)
    {
      writeSuccess = true;
      if (retryCount > 0)
      {
        M_KTALOG__INFO("[INFO] Write successful after %u retries\r\n", retryCount);
      }
    }
    else
    {
      retryCount++;
      if (retryCount < EEPROM_WRITE_MAX_RETRIES)
      {
        M_KTALOG__ERR("[ERR] Write verification failed, retry %u/%u\r\n", 
                      retryCount, EEPROM_WRITE_MAX_RETRIES);
        // Optional: Add small delay before retry
        for (volatile uint32_t delay = 0; delay < EEPROM_WRITE_RETRY_DELAY_US * 10; delay++);
      }
    }
#else
    // No verification - assume write succeeded
    writeSuccess = true;
#endif
    }
    
    if (!writeSuccess)
    {
      M_KTALOG__ERR("[ERR] EEPROM write failed after %u retries for ID 0x%04X at address 0x%04X\r\n",
                    EEPROM_WRITE_MAX_RETRIES, (unsigned int)storageID, (unsigned int)address);
    }
  }
  else
  {
    // SmartEEPROM disabled - return success (RAM stub already updated in fotaprocess.c)
    M_KTALOG__INFO("[INFO] SmartEEPROM disabled - skipping storage write (using RAM stub)\r\n");
    writeSuccess = true;
  }

  M_KTALOG__END("salFotaStorageWrite End - Success: %d", writeSuccess);
  return writeSuccess;
}


/**
 * @brief Read data from FOTA storage (SmartEEPROM or RAM stub)
 * 
 * SmartEEPROM Mode:
 *   - Direct memory read from SmartEEPROM (0x44000000)
 *   - Returns true with persistent data
 * 
 * RAM Stub Mode (SmartEEPROM Disabled):
 *   - Returns false (no persistent storage exists)
 *   - KTA library uses RAM stub in fotaprocess.c instead
 *   - RAM stub contains current FOTA state (session-only)
 * 
 * @param storageID  Storage location identifier
 * @param xpData     Buffer to receive data
 * @param xDataLen   [in] Buffer size, [out] Bytes actually read
 * 
 * @return true  - Data read from SmartEEPROM successfully
 * @return false - No SmartEEPROM, use RAM stub instead
 * 
 * @note Production-ready: Caller handles both cases gracefully
 */
bool salFotaStorageRead
(
  uint32_t   storageID,
  uint8_t*   xpData,
  size_t*    xDataLen
)
{
  uint32_t address;
  size_t fieldLen;

  M_KTALOG__START("salFotaStorageRead Start");
  if (!fotaGetStorageInfo(storageID, &address, &fieldLen))
  {
    M_KTALOG__ERR("[ERR] Invalid FOTA storage ID: 0x%04X\r\n", (unsigned int)storageID);
    return false;
  }

  // Check storage mode and use appropriate method
  if (isSmartEEPROMAvailable())
  {
    // SmartEEPROM mode - direct memory access
    memcpy(xpData, &SmartEEPROM8[address], fieldLen);
    *xDataLen = fieldLen;
  }
  else
  {
    // SmartEEPROM disabled - return false (no persistent storage, rely on RAM stub)
    M_KTALOG__INFO("[INFO] SmartEEPROM disabled - no storage to read from (using RAM stub)\r\n");
    return false;
  }

  M_KTALOG__END("salFotaStorageRead End");
  return true;
}

/* -------------------------------------------------------------------------- */
/* END OF FILE                                                                */
/* -------------------------------------------------------------------------- */
