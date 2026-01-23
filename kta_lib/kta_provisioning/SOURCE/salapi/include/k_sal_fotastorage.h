/*******************************************************************************
*************************keySTREAM Trusted Agent ("KTA")************************

* (c) 2023-2025 Nagravision SÃ rl

* Subject to your compliance with these terms, you may use the Nagravision SÃ rl
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
/** \brief  Interface for storage operation.
*
* \author Kudelski IoT
*
* \date 2025/07/04
*
* \file k_sal_fotastorage.h
*
******************************************************************************/

/**
 * @brief Interface for storage operation.
 */

#ifndef K_SAL_FOTASTORAGE_H
#define K_SAL_FOTASTORAGE_H

#ifdef __cplusplus
extern "C" {
#endif /* C++ */


/* -------------------------------------------------------------------------- */
/* MACROS                                                                     */
/* -------------------------------------------------------------------------- */
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

/* -------------------------------------------------------------------------- */
/* FOTA Storage ID's                                                          */
/* -------------------------------------------------------------------------- */
/**
 * @brief Persistent Storage_Ids.
 */
/** @brief FOTA name ID. */
#define FOTA_STORAGE_NAME_ID                                                       (0x2001u)

/** @brief FOTA metadata ID. */
#define FOTA_STORAGE_METADATA_ID                                                   (0x2003u)

/** @brief FOTA commit flag ID - used to validate campaign completion */
#define FOTA_STORAGE_COMMIT_ID                                                     (0x2005u)

/** @brief Magic number for validating FOTA component records */
#define FOTA_RECORD_MAGIC                                                          (0xF07A1234u)

/** @brief Magic number for validating FOTA commit record */
#define FOTA_COMMIT_MAGIC                                                          (0xF07AC0DEu)

/** @brief Commit flag states */
#define FOTA_COMMIT_VALID                                                          (0xAAu)
#define FOTA_COMMIT_INVALID                                                        (0xFFu)

/** @brief Campaign state values */
#define FOTA_CAMPAIGN_INIT                                                         (0x00u)  // Campaign initialized
#define FOTA_CAMPAIGN_IN_PROGRESS                                                  (0x01u)  // Component updates in progress
#define FOTA_CAMPAIGN_COMPLETE                                                     (0x02u)  // All components finished

/** @brief Magic number for validating FOTA name record */
#define FOTA_NAME_MAGIC                                                            (0xF07ACA3Eu)

/** @brief Magic number for validating FOTA metadata record */
#define FOTA_METADATA_MAGIC                                                        (0xF07ABE7Au)

/** @brief FOTA name record with CRC protection */
typedef struct {
    uint32_t magic;            // Magic number for validity check (0xF07ANA3E)
    uint8_t  nameLen;
    uint8_t  name[27];         // Reduced from 32 to accommodate CRC
    uint32_t crc32;            // CRC32 checksum of record (magic through name)
} TFotaNameRecord;

/** @brief FOTA metadata record with CRC protection */
typedef struct {
    uint32_t magic;            // Magic number for validity check (0xF07AME7A)
    uint8_t  metadataLen;
    uint8_t  metadata[59];     // Reduced from 64 to accommodate CRC
    uint32_t crc32;            // CRC32 checksum of record (magic through metadata)
} TFotaMetadataRecord;

/** @brief FOTA commit record - written LAST to indicate campaign completion */
typedef struct {
    uint32_t magic;            // Magic number for validity check (0xF07AC0DE)
    uint8_t  commitFlag;       // 0xAA = valid/complete, 0xFF = invalid/incomplete
    uint8_t  numComponents;    // Number of components in this campaign
    uint8_t  campaignState;    // 0=INIT, 1=IN_PROGRESS, 2=COMPLETE
    uint8_t  reserved;         // Padding for alignment
    uint32_t crc32;            // CRC32 checksum of record (magic through reserved)
} TFotaCommitRecord;

/** @brief FOTA component record structure for combined storage */
typedef struct {
    uint32_t magic;            // Magic number for validity check (0xF07A1234)
    uint8_t componentNameLen;
    uint8_t componentName[16];
    uint8_t componentVersionLen;
    uint8_t componentVersion[16];
    uint8_t state;
    uint8_t reserved1[3];      // Padding for alignment
    uint16_t componentUrlLen;
    uint8_t componentUrl[512];
    uint32_t crc32;            // CRC32 checksum of record (magic through componentUrl)
} TFotaComponentRecord;

/** @brief FOTA component IDs (indexed 0-7) - stores name, version, and state together */
#define FOTA_STORAGE_COMPONENT_ID_BASE                                             (0x2010u)
#define FOTA_STORAGE_COMPONENT_ID(index)                                           (FOTA_STORAGE_COMPONENT_ID_BASE + (index))



/* -------------------------------------------------------------------------- */
/* FUNCTIONS                                                                  */
/* -------------------------------------------------------------------------- */

/**
  * @brief
  *   To store the data based on ID.
  *
  * @param[in] xStorageDataId
  *   Storage data Identifier. Refer Storage_Ids.
  * @param[in] xpData
  *   Address of buffer containing the input data.
  *   Should not be NULL.
  * @param[in] xDataLen
  *   Length of xpData buffer in bytes.
  *
  * @return
  * - E_K_STATUS_OK in case of success.
  * - E_K_STATUS_PARAMETER for wrong input parameter(s).
  * - E_K_STATUS_ERROR for other errors.
  */
bool salFotaStorageWrite
 (
   uint32_t xStorageDataId,
   const uint8_t* xpData,
   size_t xDataLen
 );

 /**
  * @brief
  *   To get the data based on ID.
  *
  * @param[in] xStorageDataId
  *   Storage data Identifier. Refer Storage_Ids.
  * @param[out] xpData
  *   Address of buffer where the device platform will return the output data.
  *   Should not be NULL.
  * @param[in,out] xpDataLen
  *   Address of output buffer length (in Bytes).
  *   [in] Caller set the maximum output buffer length expected.
  *   [out] The function set the actual length of the output buffer.
  *
  * @return
  * - E_K_STATUS_OK in case of success.
  * - E_K_STATUS_PARAMETER for wrong input parameter(s).
  * - E_K_STATUS_ERROR for other errors.
  */
bool salFotaStorageRead
 (
   uint32_t  xStorageDataId,
   uint8_t*  xpData,
   size_t*	 xpDataLen
 );


 #ifdef __cplusplus
}
#endif /* C++ */

#endif // K_SAL_FOTASTORAGE_H

/* -------------------------------------------------------------------------- */
/* END OF FILE                                                                */
/* -------------------------------------------------------------------------- */
