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
/** \brief  Fota Process.
 *
 *  \author Kudelski IoT
 *
 *  \date 2025/01/22
 *
 *  \file fotaprocess.c
 ******************************************************************************/


/* -------------------------------------------------------------------------- */
/* IMPORTS                                                                    */
/* -------------------------------------------------------------------------- */
#include "k_sal_fota.h"
#include "fotaagent.h"
#include "fotaprocess.h"
#include "fotaplatform.h"
#include "k_sal_storage.h"
#include "k_sal_fotastorage.h"

#include <stdio.h>
#include <string.h>
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include "KTALog.h"

/* -------------------------------------------------------------------------- */
/* LOCAL VARIABLES                                                            */
/* -------------------------------------------------------------------------- */
/**
 * SUPPRESS: MISRA_DEV_KTA_009 : misra_c2012_rule_5.9_violation
 * The identifier gpModuleName is intentionally defined as a common global for logging purposes
 */
#if LOG_KTA_ENABLE != C_KTA_LOG_LEVEL_NONE
static const char* gpModuleName = "FOTAPROCESS";
#endif

/* -------------------------------------------------------------------------- */
/* LOCAL CONSTANTS, TYPES, ENUM                                               */
/* -------------------------------------------------------------------------- */
/** @brief Enum for FOTA error codes */
typedef enum
{
  FOTA_ERROR_INVALID_INPUT = 0x01,
  FOTA_ERROR_INVALID_URL = 0x02,
  FOTA_ERROR_OLDER_VERSION = 0x03,
  FOTA_ERROR_UNEXPECTED_STATE = 0x04,
  FOTA_ERROR_NO_COMPONENTS = 0x05
}
TFotaErrorCode;

/** @brief Array of 16-byte error causes corresponding to TFotaErrorCode. */
static const uint8_t aFotaErrorCauses[FOTA_ERROR_COUNT][ERROR_CAUSE_MAX_LEN] =
{
  {0x49, 0x4E, 0x56, 0x41, 0x4C, 0x49, 0x44, 0x20, 0x49, 0x4E, 0x50, 0x55, 0x54, 0x00, 0x00, 0x00}, // "INVALID INPUT"
  {0x49, 0x4E, 0x56, 0x41, 0x4C, 0x49, 0x44, 0x20, 0x55, 0x52, 0x4C, 0x00, 0x00, 0x00, 0x00, 0x00}, // "INVALID URL"
  {0x4F, 0x4C, 0x44, 0x45, 0x52, 0x20, 0x56, 0x45, 0x52, 0x53, 0x49, 0x4F, 0x4E, 0x00, 0x00, 0x00}, // "OLDER VERSION"
  {0x55, 0x4E, 0x45, 0x58, 0x50, 0x45, 0x43, 0x54, 0x45, 0x44, 0x20, 0x53, 0x54, 0x41, 0x54, 0x45}, // "UNEXPECTED STATE"
  {0x4E, 0x4F, 0x20, 0x43, 0x4F, 0x4D, 0x50, 0x4F, 0x4E, 0x45, 0x4E, 0x54, 0x53, 0x00, 0x00, 0x00}  // "NO COMPONENTS"
};

/* -------------------------------------------------------------------------- */
/* LOCAL VARIABLES                                                            */
/* -------------------------------------------------------------------------- */
/* Global buffer to store FOTA metadata */
static uint8_t gFotaMetadata[MAX_FOTA_META_DATA] = {0};

/* Global variable to store FOTA metadata length */
static size_t gFotaMetadataLen = 0;

/* Structure to maintain status of each components */
TFotaComponentStatus gFotaComponentStatus = {0};

/* To fix the misra-c2012-9.2 & misra-c2012-9.4-An element of an object shall not be initialized more than once */
TComponent installedComponents[COMPONENTS_MAX] = {
  { { }, 0, { }, 0 }
};

static bool fotaInialized = false;
/* -------------------------------------------------------------------------- */
/* LOCAL FUNCTIONS - PROTOTYPE                                                */
/* -------------------------------------------------------------------------- */

/**
 * @brief Calculate CRC32 checksum for data buffer.
 *
 * Uses standard CRC32 polynomial (0xEDB88320).
 *
 * @param[in] data
 *   Pointer to data buffer.
 * @param[in] length
 *   Length of data in bytes.
 *
 * @return
 * - uint32_t CRC32 checksum value.
 */
static uint32_t calculateCRC32
(
  const uint8_t *data,
  size_t         length
);

/**
 * @brief Validate CRC32 checksum of FOTA component record.
 *
 * @param[in] record
 *   Pointer to component record to validate.
 *
 * @return
 * - bool true if CRC is valid, false if corrupted.
 */
static bool validateComponentRecordCRC
(
  const TFotaComponentRecord *record
);

/**
 * @brief Calculate and set CRC32 for FOTA component record.
 *
 * @param[in,out] record
 *   Pointer to component record to update.
 */
static void setComponentRecordCRC
(
  TFotaComponentRecord *record
);

/**
 * @brief Validate CRC32 checksum of FOTA name record.
 *
 * @param[in] record
 *   Pointer to name record to validate.
 *
 * @return
 * - bool true if CRC is valid, false if corrupted.
 */
static bool validateNameRecordCRC
(
  const TFotaNameRecord *record
);

/**
 * @brief Calculate and set CRC32 for FOTA name record.
 *
 * @param[in,out] record
 *   Pointer to name record to update.
 */
static void setNameRecordCRC
(
  TFotaNameRecord *record
);


/**
 * @brief Calculate and set CRC32 for FOTA metadata record.
 *
 * @param[in,out] record
 *   Pointer to metadata record to update.
 */
static void setMetadataRecordCRC
(
  TFotaMetadataRecord *record
);

/**
 * @brief Validate CRC32 checksum of FOTA commit record.
 *
 * @param[in] record
 *   Pointer to commit record to validate.
 *
 * @return
 * - bool true if CRC and magic are valid, false if corrupted.
 */
static bool validateCommitRecord
(
  const TFotaCommitRecord *record
);

/**
 * @brief Set CRC32 checksum for FOTA commit record.
 *
 * @param[in,out] record
 *   Pointer to commit record to calculate and set CRC.
 */
static void setCommitRecord
(
  TFotaCommitRecord *record
);

/**
 * @brief Compare two version arrays.
 *
 * This function compares two version arrays.
 *
 * @param[in] currentVersion
 *   Current version as a byte array.
 * @param[in] targetVersion
 *   Target version as a byte array.
 * @param[in] length
 *   Length of the version arrays.
 *
 * @return
 * - TVersionState indicating comparison result.
 */
static TFotaVersionState compareVersion
(
  const uint8_t* currentVersion,
  const uint8_t* targetVersion,
  size_t         length
);

/**
 * @brief Retrieves the current version of a component.
 *
 * This function retrieves the current version of a specified component. It populates the
 * provided buffer with the version information and sets the length of the version.
 *
 * @param componentName Pointer to the component name.
 * @param componentNameLen Length of the component name.
 * @param versionBuffer Buffer to store the retrieved version.
 * @param versionLength Pointer to store the length of the retrieved version.
 * @return TKFotaStatus indicating the status of the operation.
 *         - E_K_FOTA_SUCCESS: Successfully retrieved the component version.
 *         - E_K_FOTA_ERROR: Failed to retrieve the component version.
 */
static TKFotaStatus getVersion
(
  const uint8_t *componentName,
  size_t         componentNameLen,
  uint8_t       *currentVersion,
  size_t        *currentVersionLength
);

/**
 * @brief Update Error cause and error code.
 *
 *
 * @param[in] error
 *   Pointer to the FOTA error structure.
 *   Should not be NULL.
 * @param[in] errorCode
 *   Error state of FOTA.
 */
static void fotaAgentUpdateFotaError
(
  TFotaError * error,
  const TFotaErrorCode errorCode
);

/**
 * @brief Fill component name and version details in structure based on state.
 *
 *
 * @param[in] compState
 *   State of the components for be filled

 * @param[OUT] components
 *   Components sturictre.
 *   Should not be NULL.
 */
static void fotaFillComponents
(
  TFotaState compState,
  TComponent components[COMPONENTS_MAX]
);

/* -------------------------------------------------------------------------- */
/* PUBLIC FUNCTIONS                                                           */
/* -------------------------------------------------------------------------- */


/**
 * @brief implement initializefotaState
 *
 */
void initializefotaState
(
  void
)
{
  TComponent components[COMPONENTS_MAX] = {0};
  TKFotaStatus retStatus = E_K_FOTA_ERROR;

  M_KTALOG__START("Start");

  // CRITICAL: Check commit flag FIRST before processing any FOTA data
  // If commit flag is not valid (0xAA), entire FOTA campaign is incomplete/corrupted
  TFotaCommitRecord commitRecord = {0};
  size_t commitRecordLen = sizeof(TFotaCommitRecord);
  bool readCommit = salFotaStorageRead(FOTA_STORAGE_COMMIT_ID, (uint8_t*)&commitRecord, &commitRecordLen);
  
  bool campaignValid = false;
  if (readCommit && validateCommitRecord(&commitRecord))
  {
    if (commitRecord.commitFlag == FOTA_COMMIT_VALID)
    {
      M_KTALOG__INFO("Commit flag VALID (0xAA) - resuming FOTA campaign with %u components (state=%u)\r\n",
                     (unsigned int)commitRecord.numComponents,
                     (unsigned int)commitRecord.campaignState);
      campaignValid = true;
    }
    else if (commitRecord.commitFlag == FOTA_COMMIT_INVALID && 
             commitRecord.campaignState == FOTA_CAMPAIGN_IN_PROGRESS)
    {
      // CRITICAL: Allow resume of campaigns that were updating when power lost
      // This preserves component progress (SUCCESS/ERROR states) after power cycles
      M_KTALOG__INFO("Resuming FOTA campaign in IN_PROGRESS state after power cycle (numComponents=%u)\r\n",
                     (unsigned int)commitRecord.numComponents);
      campaignValid = true;
    }
    else
    {
      M_KTALOG__WARN("Commit flag INVALID (0x%02X) with state=%u - FOTA campaign incomplete, resetting to IDLE\r\n",
                     (unsigned int)commitRecord.commitFlag,
                     (unsigned int)commitRecord.campaignState);
    }
  }
  else
  {
    M_KTALOG__WARN("Commit record not found or invalid - no active FOTA campaign\r\n");
  }

  // Get installed component information from platform
  retStatus = fotaPlatformGetComponents(components);
  if (E_K_FOTA_SUCCESS != retStatus)
  {
    M_KTALOG__ERR("ERROR: Unable to get components information from platform");
    goto end;
  }

  // Update platform components in global structure
  for (size_t i = 0; i < COMPONENTS_MAX; i++)
  {
    if ((0 < components[i].componentNameLen) ||
        (0 < components[i].componentVersionLen))
    {
      memcpy(installedComponents[i].componentName,
            components[i].componentName,
            components[i].componentNameLen);
      installedComponents[i].componentNameLen = components[i].componentNameLen;
      memcpy(installedComponents[i].componentVersion,
        components[i].componentVersion,
        components[i].componentVersionLen);
      installedComponents[i].componentVersionLen = components[i].componentVersionLen;
    }
  }

  // Initialize global state of fota target components list
  for (size_t i = 0; i < COMPONENTS_MAX; i++)
  {
    // If commit flag is invalid, reset all components to IDLE without reading EEPROM
    if (!campaignValid)
    {
      gFotaComponentStatus.state[i] = E_FOTA_STATE_IDLE;
      gFotaComponentStatus.components[i].componentNameLen = 0;
      gFotaComponentStatus.components[i].componentVersionLen = 0;
      continue;
    }
    
    // Read combined component record (name, version, state together)
    TFotaComponentRecord compRecord = {0};
    size_t compRecordLen = sizeof(TFotaComponentRecord);
    bool readComp = salFotaStorageRead(FOTA_STORAGE_COMPONENT_ID(i), (uint8_t*)&compRecord, &compRecordLen);

    if (true != readComp)
    {
      M_KTALOG__ERR("ERROR: Failed to read component record from storage.\r\n");
      // Initialize with default values instead of exiting
      gFotaComponentStatus.state[i] = E_FOTA_STATE_IDLE;
      gFotaComponentStatus.components[i].componentNameLen = 0;
      gFotaComponentStatus.components[i].componentVersionLen = 0;
      continue;  // Skip to next component
    }

    // Validate CRC32 checksum for power-cycle corruption detection
    if (!validateComponentRecordCRC(&compRecord))
    {
      M_KTALOG__ERR("[ERR] Component record %u failed CRC validation during init - treating as IDLE\r\n", (unsigned int)i);
      gFotaComponentStatus.state[i] = E_FOTA_STATE_IDLE;
      gFotaComponentStatus.components[i].componentNameLen = 0;
      gFotaComponentStatus.components[i].componentVersionLen = 0;
      continue;
    }

    // Validate lengths to prevent buffer overflow
    if (compRecord.componentNameLen > 16)
    {
      M_KTALOG__ERR("ERROR: Invalid componentNameLen (%u) for component %u\r\n", 
                     (unsigned int)compRecord.componentNameLen, (unsigned int)i);
      compRecord.componentNameLen = 0;
    }

    if (compRecord.componentVersionLen > 16)
    {
      M_KTALOG__ERR("ERROR: Invalid componentVersionLen (%u) for component %u\r\n", 
                     (unsigned int)compRecord.componentVersionLen, (unsigned int)i);
      compRecord.componentVersionLen = 0;
    }

    gFotaComponentStatus.state[i] = compRecord.state;

    if (compRecord.componentNameLen > 0)
    {
      memcpy(gFotaComponentStatus.components[i].componentName,
         compRecord.componentName,
         compRecord.componentNameLen);

      gFotaComponentStatus.components[i].componentNameLen = compRecord.componentNameLen;
      if (compRecord.componentVersionLen > 0)
      {
        memcpy(gFotaComponentStatus.components[i].componentVersion,
             compRecord.componentVersion,
             compRecord.componentVersionLen);
      }
    }
    gFotaComponentStatus.components[i].componentVersionLen = compRecord.componentVersionLen;

  }

  fotaInialized = true;
end:
  M_KTALOG__END("End, status : 0\r\n");
  return;
}

/**
 * @brief implement fotaGetInstalledComponents
 *
 */
TKFotaStatus fotaGetInstalledComponents
(
  TComponent *xpComponents
)
{
  TKFotaStatus retStatus = E_K_FOTA_ERROR;

  M_KTALOG__START("Start");

  if (NULL == xpComponents)
  {
    goto end;
  }
  if (!fotaInialized)
  {
    initializefotaState();
  }
  // Get name and versions of components installed and update xpComponents structure
  for (size_t i = 0; i < COMPONENTS_MAX; i++)
  {
    memcpy(xpComponents[i].componentName,
          installedComponents[i].componentName,
          installedComponents[i].componentNameLen);
    xpComponents[i].componentNameLen = installedComponents[i].componentNameLen;
    memcpy(xpComponents[i].componentVersion,
      installedComponents[i].componentVersion,
      installedComponents[i].componentVersionLen);
    xpComponents[i].componentVersionLen = installedComponents[i].componentVersionLen;
  }
  retStatus = E_K_FOTA_SUCCESS;
end:
  M_KTALOG__END("End, status : %d", retStatus);
  return retStatus;
}


/**
 * @brief implement validateComponents
 *
 */
TKFotaStatus validateComponents
(
  const TTargetComponent xTargetComponents[COMPONENTS_MAX],
  TFotaError *xFotaError,
  TComponent xComponents[COMPONENTS_MAX]
)
{
  TKFotaStatus retStatus = E_K_FOTA_SUCCESS;
  TFotaVersionState versionState = E_FOTA_NEW_VERSION;
  uint8_t currentVersion[CURRENT_MAX_LENGTH] = {0};
  size_t currentVersionLength = 0;
  bool hasValidComponent = false;

  M_KTALOG__START("Start");

  for (size_t i = 0; i < COMPONENTS_MAX; i++)
  {
    if ((0 < xTargetComponents[i].componentTargetNameLen) &&
        (0 < xTargetComponents[i].componentTargetVersionLen))
    {
      hasValidComponent = true;

      /* Validate component name  and version length */
      if ((xTargetComponents[i].componentTargetNameLen > MAX_COMPONENT_NAME_LEN)||
          (xTargetComponents[i].componentTargetVersionLen > MAX_COMPONENT_VERSION_LEN))
      {
        M_KTALOG__ERR("ERROR: Component name length %d or Component version length %d exceeded\n",
                      xTargetComponents[i].componentTargetNameLen,
                      xTargetComponents[i].componentTargetVersionLen);
        fotaAgentUpdateFotaError(xFotaError, FOTA_ERROR_INVALID_INPUT);
        retStatus = E_K_FOTA_ERROR;
        continue;
      }

      /* check URL for all components*/
      if ((0 == xTargetComponents[i].componentUrlLen) ||
          (NULL == xTargetComponents[i].componentUrl))
      {
        fotaAgentUpdateFotaError(xFotaError, FOTA_ERROR_INVALID_URL);
        memcpy(xComponents[i].componentName, xTargetComponents[i].componentTargetName,
                xTargetComponents[i].componentTargetNameLen);
        xComponents[i].componentNameLen = xTargetComponents[i].componentTargetNameLen;
        memcpy(xComponents[i].componentVersion, xTargetComponents[i].componentTargetVersion,
                xTargetComponents[i].componentTargetVersionLen);
        xComponents[i].componentVersionLen = xTargetComponents[i].componentTargetVersionLen;
        retStatus = E_K_FOTA_ERROR;
        continue;
      }

      memset(currentVersion, '\0', CURRENT_MAX_LENGTH);
      currentVersionLength = 0;
      retStatus = getVersion(xTargetComponents[i].componentTargetName,
                             xTargetComponents[i].componentTargetNameLen,
                             currentVersion, &currentVersionLength);

      if (E_K_FOTA_SUCCESS != retStatus)
      {
        fotaAgentUpdateFotaError(xFotaError, FOTA_ERROR_INVALID_INPUT);
	      memcpy(xComponents[i].componentName, xTargetComponents[i].componentTargetName,
               xTargetComponents[i].componentTargetNameLen);
        xComponents[i].componentNameLen = xTargetComponents[i].componentTargetNameLen;
        memcpy(xComponents[i].componentVersion, xTargetComponents[i].componentTargetVersion,
               xTargetComponents[i].componentTargetVersionLen);
        xComponents[i].componentVersionLen = xTargetComponents[i].componentTargetVersionLen;

        retStatus = E_K_FOTA_ERROR;
        continue;
      }

      versionState = compareVersion(currentVersion,
                             xTargetComponents[i].componentTargetVersion,
                             currentVersionLength);

      switch (versionState)
      {
        case E_FOTA_NEW_VERSION:
        {
          retStatus = E_K_FOTA_IN_PROGRESS;
        }
        break;

        case E_FOTA_CURRENT_VERSION:
        {
          /* Target version is older than the current installed version */
          fotaAgentUpdateFotaError(xFotaError, FOTA_ERROR_INVALID_INPUT);

          memcpy(xComponents[i].componentName, xTargetComponents[i].componentTargetName,
                  xTargetComponents[i].componentTargetNameLen);
          xComponents[i].componentNameLen = xTargetComponents[i].componentTargetNameLen;

          memcpy(xComponents[i].componentVersion, xTargetComponents[i].componentTargetVersion,
                  xTargetComponents[i].componentTargetVersionLen);
          xComponents[i].componentVersionLen = xTargetComponents[i].componentTargetVersionLen;
          retStatus = E_K_FOTA_ERROR;
        }
        break;

        default:
        {
          M_KTALOG__ERR("ERROR: Invalid version %d", versionState);
        }
        break;
      }
    }
  }

  /* Check if at least one valid component was provided */
  if (!hasValidComponent)
  {
    M_KTALOG__ERR("ERROR: No valid components provided\n");
    fotaAgentUpdateFotaError(xFotaError, FOTA_ERROR_NO_COMPONENTS);
    retStatus = E_K_FOTA_ERROR;
  }

  M_KTALOG__END("End, status : %d", retStatus);
  return retStatus;
}

/**
 * @brief implement storeMetadata
 *
 */
TKFotaStatus storeMetadata
(
  const uint8_t *xFotaName,
  size_t         xFotaNameLen,
  const uint8_t *xFotaMetadata,
  size_t         xFotaMetadataLen,
  TFotaError    *xFotaError
)
{
  TKFotaStatus retStatus = E_K_FOTA_ERROR;
  M_KTALOG__START("Start");

  // Validate input parameters check
  if ((NULL == xFotaName)     ||
      (NULL == xFotaMetadata) ||
      (0 == xFotaNameLen)     ||
      (0 == xFotaMetadataLen))
  {
    M_KTALOG__ERR("ERROR: Invalid parameter passed\r\n");
    fotaAgentUpdateFotaError(xFotaError, FOTA_ERROR_INVALID_INPUT);
    goto end;
  }

  // Check metadata size limit
  if (MAX_FOTA_META_DATA < xFotaMetadataLen)
  {
    fotaAgentUpdateFotaError(xFotaError, FOTA_ERROR_INVALID_INPUT);
    goto end;
  }

  // Store metadata
  memcpy(gFotaMetadata, xFotaMetadata, xFotaMetadataLen);
  gFotaMetadataLen = xFotaMetadataLen;
  retStatus = E_K_FOTA_SUCCESS;
end:
  M_KTALOG__END("End, status : %d", retStatus);
  return retStatus;
}

/**
 * @brief implement fotaGetTargetComponents
 *
 */
TKFotaStatus fotaGetTargetComponents
(
  TFotaComponentRecord *xpTargetComponents,
  uint8_t *xpNumComponents
)
{
  TKFotaStatus retStatus = E_K_FOTA_ERROR;
  uint8_t validComponentCount = 0;

  M_KTALOG__START("Start");

  // Validate input parameters
  if (xpTargetComponents == NULL)
  {
    M_KTALOG__ERR("ERROR: xpTargetComponents is NULL\r\n");
    goto end;
  }

  if (xpNumComponents == NULL)
  {
    M_KTALOG__ERR("ERROR: xpNumComponents is NULL\r\n");
    goto end;
  }

  if (!fotaInialized)
  {
    initializefotaState();
  }

  // Initialize output
  *xpNumComponents = 0;
  memset(xpTargetComponents, 0, sizeof(TFotaComponentRecord) * COMPONENTS_MAX);

  // Read all component records from storage
  for (size_t i = 0; i < COMPONENTS_MAX; i++)
  {
    size_t recordLen = sizeof(TFotaComponentRecord);
    bool readStatus = salFotaStorageRead(FOTA_STORAGE_COMPONENT_ID(i), (uint8_t*)&xpTargetComponents[i], &recordLen);

    if (true != readStatus)
    {
      M_KTALOG__ERR("ERROR: Failed to read component record %u from storage\r\n", (unsigned int)i);
      continue;  // Skip this component and try next
    }

    // Validate CRC before using the record
    if (!validateComponentRecordCRC(&xpTargetComponents[i]))
    {
      M_KTALOG__ERR("[ERR] Component record %u failed CRC validation - corrupted data\r\n", (unsigned int)i);
      memset(&xpTargetComponents[i], 0, sizeof(TFotaComponentRecord));  // Clear corrupted record
      continue;  // Skip this component
    }

    // Check if this component has valid data
    if (xpTargetComponents[i].componentNameLen > 0)
    {
      validComponentCount++;
      M_KTALOG__INFO("[INFO] Component[%u]: name=%.*s, version=%.*s, state=%u, urlLen=%u\r\n",
      (unsigned int)i,
      (int)xpTargetComponents[i].componentNameLen, xpTargetComponents[i].componentName,
      (int)xpTargetComponents[i].componentVersionLen, xpTargetComponents[i].componentVersion,
      (unsigned int)xpTargetComponents[i].state,
      (unsigned int)xpTargetComponents[i].componentUrlLen);
    }
  }

  *xpNumComponents = validComponentCount;

  if (validComponentCount > 0)
  {
    retStatus = E_K_FOTA_SUCCESS;
    M_KTALOG__INFO("[INFO] Successfully retrieved %u target component(s) from storage\r\n", validComponentCount);
  }
  else
  {
    M_KTALOG__INFO("[INFO] No valid target components found in storage\r\n");
    retStatus = E_K_FOTA_ERROR;
  }

end:
  M_KTALOG__END("End, status : %d", retStatus);
  return retStatus;
}

/**
 * @brief implement fotaGetComponentsStatus
 *
 */
TKFotaStatus fotaGetComponentsStatus
(
  const uint8_t *xpFotaName,
  size_t         fotaNameLen,
  TFotaError    *xpFotaError,
  TComponent     components[COMPONENTS_MAX]
)
{
  TKFotaStatus retStatus    = E_K_FOTA_ERROR;
  TFotaState   compState    = E_FOTA_STATE_IDLE;
  bool         state_error  = 0;
  bool         state_success = 0;
  TFotaNameRecord nameRecord = {0};
  size_t       nameRecordLen = sizeof(TFotaNameRecord);

  M_KTALOG__START("Start");

  // Read and validate FOTA name record with CRC
  if (!salFotaStorageRead(FOTA_STORAGE_NAME_ID, (uint8_t*)&nameRecord, &nameRecordLen) ||
      !validateNameRecordCRC(&nameRecord))
  {
    M_KTALOG__ERR("ERROR: Failed to read or validate FOTA name record\r\n");
    fotaAgentUpdateFotaError(xpFotaError, FOTA_ERROR_INVALID_INPUT);
    fotaFillComponents(E_FOTA_STATE_ERROR, components);
    retStatus = E_K_FOTA_ERROR;
    goto end;
  }

  // Compare name
  if (nameRecord.nameLen != fotaNameLen || 
      0 != memcmp(nameRecord.name, xpFotaName, fotaNameLen))
  {
    fotaAgentUpdateFotaError(xpFotaError, FOTA_ERROR_INVALID_INPUT);
    fotaFillComponents(E_FOTA_STATE_ERROR, components);
    retStatus = E_K_FOTA_ERROR;
    M_KTALOG__ERR("ERROR: FOTA name mismatch. Expected: %.*s, Found: %.*s\r\n",
                   (int)fotaNameLen, xpFotaName,
                   (int)nameRecord.nameLen, nameRecord.name);
    goto end;
  }

  // Iterate through all components to check their status
  // CRITICAL: Read state from EEPROM instead of RAM to ensure power-cycle consistency
  for (size_t i = 0; i < COMPONENTS_MAX; i++)
  {
    // Read component record from EEPROM to get current state
    TFotaComponentRecord compRecord = {0};
    size_t compRecordLen = sizeof(TFotaComponentRecord);
    bool readSuccess = salFotaStorageRead(FOTA_STORAGE_COMPONENT_ID(i), (uint8_t*)&compRecord, &compRecordLen);
    
    if (readSuccess && validateComponentRecordCRC(&compRecord))
    {
      // Use EEPROM state (source of truth after power cycle)
      compState = compRecord.state;
      
      // Sync RAM state with EEPROM state
      gFotaComponentStatus.state[i] = compState;
    }
    else
    {
      // If EEPROM read fails or CRC invalid, component is not active
      compState = E_FOTA_STATE_IDLE;
      gFotaComponentStatus.state[i] = E_FOTA_STATE_IDLE;
    }
    
    if (E_FOTA_STATE_IN_PROGRESS == compState)
    {
      retStatus = E_K_FOTA_IN_PROGRESS;
      break;
    }
    else if (E_FOTA_STATE_ERROR == compState)
    {
      state_error = 1;
    }
    else if (E_FOTA_STATE_SUCCESS == compState)
    {
      state_success = 1;
    }
  } // end of for()


  if (state_error)
  {
    fotaAgentUpdateFotaError(xpFotaError, FOTA_ERROR_UNEXPECTED_STATE);
    fotaFillComponents(E_FOTA_STATE_ERROR, components);
    retStatus = E_K_FOTA_ERROR;
    goto end;
  }

  if (state_success)
  {
    fotaFillComponents(E_FOTA_STATE_SUCCESS, components);
    retStatus = E_K_FOTA_SUCCESS;
    goto end;
  }

end:
  M_KTALOG__END("End, status : %d", retStatus);
  return retStatus;
}


/**
 * @brief implement fotaDownloadAndInstall
 *
 */
TKFotaStatus fotaDownloadAndInstall
(
  const uint8_t          *xpFotaName,
  const size_t            xFotaNameLen,
  const uint8_t           *xpFotaMetadata,
  const size_t            xFotaMetadataLen,
  const TTargetComponent *xpTargetComponents,
  TComponent              xComponents[COMPONENTS_MAX]
)
{
  (void)xComponents;
  TKFotaStatus retStatus = E_K_FOTA_ERROR; // Initial status
  uint8_t writeStatus          =     false;
  uint8_t numComponents        =     0;  // Count of components in this campaign

  M_KTALOG__START("Start");

  if (NULL == xpFotaName)
  {
    M_KTALOG__ERR("ERROR: xpFotaName is NULL\r\n");
    goto end;
  }
  memcpy(gFotaComponentStatus.xpFotaName, xpFotaName, xFotaNameLen);
  gFotaComponentStatus.xFotaNameLen = xFotaNameLen;

  // Store xpFotaName with CRC protection
  TFotaNameRecord nameRecord = {0};
  nameRecord.nameLen = (uint8_t)((xFotaNameLen < 27) ? xFotaNameLen : 27);
  memcpy(nameRecord.name, xpFotaName, nameRecord.nameLen);
  setNameRecordCRC(&nameRecord);
  
  writeStatus = salFotaStorageWrite(FOTA_STORAGE_NAME_ID, (const uint8_t*)&nameRecord, sizeof(TFotaNameRecord));
  if (true != writeStatus)
  {
    M_KTALOG__ERR("ERROR: Failed to write xpFotaName to SAL storage\r\n");
    retStatus = E_K_FOTA_ERROR;
    goto end;
  }

  // Store xpFotaMetadata with CRC protection
  if ((NULL != xpFotaMetadata) && (0 < xFotaMetadataLen))
  {
    TFotaMetadataRecord metadataRecord = {0};
    metadataRecord.metadataLen = (uint8_t)((xFotaMetadataLen < 59) ? xFotaMetadataLen : 59);
    memcpy(metadataRecord.metadata, xpFotaMetadata, metadataRecord.metadataLen);
    setMetadataRecordCRC(&metadataRecord);
    
    writeStatus = salFotaStorageWrite(FOTA_STORAGE_METADATA_ID, (const uint8_t*)&metadataRecord, sizeof(TFotaMetadataRecord));
    if (true != writeStatus)
    {
      M_KTALOG__ERR("ERROR: Failed to write FOTA metadata to SAL storage\r\n");
      retStatus = E_K_FOTA_ERROR;
      goto end;
    }
  }

  // Iterate through all target components
  for (size_t i = 0; i < COMPONENTS_MAX; i++)
  {
    if (0 < xpTargetComponents[i].componentTargetNameLen)
    {
      // Prepare combined component record
      TFotaComponentRecord compRecord = {0};
      compRecord.componentNameLen = xpTargetComponents[i].componentTargetNameLen;
      memcpy(compRecord.componentName, xpTargetComponents[i].componentTargetName,
             xpTargetComponents[i].componentTargetNameLen);
      compRecord.componentVersionLen = xpTargetComponents[i].componentTargetVersionLen;
      memcpy(compRecord.componentVersion, xpTargetComponents[i].componentTargetVersion,
             xpTargetComponents[i].componentTargetVersionLen);
      compRecord.componentUrlLen = xpTargetComponents[i].componentUrlLen;
      if (compRecord.componentUrlLen > 512)
      {
        compRecord.componentUrlLen = 512;  // Limit to structure size
      }
      memcpy(compRecord.componentUrl, xpTargetComponents[i].componentUrl,
             compRecord.componentUrlLen);
      compRecord.state = E_FOTA_STATE_IN_PROGRESS;

      // Calculate and set CRC32 checksum for power-cycle protection
      setComponentRecordCRC(&compRecord);

      // Store component record with single write operation
      writeStatus = salFotaStorageWrite(FOTA_STORAGE_COMPONENT_ID(i),
                                        (uint8_t*)&compRecord,
                                        sizeof(TFotaComponentRecord));
      if (!writeStatus)
      {
        M_KTALOG__ERR("ERROR: Failed to write component record to storage\n");
        retStatus = E_K_FOTA_ERROR;
        goto end;
      }

      // Update global state after successful write
      gFotaComponentStatus.state[i] = E_FOTA_STATE_IN_PROGRESS;
      memcpy(gFotaComponentStatus.components[i].componentName,
            xpTargetComponents[i].componentTargetName,
            xpTargetComponents[i].componentTargetNameLen);
      gFotaComponentStatus.components[i].componentNameLen = xpTargetComponents[i].componentTargetNameLen;
      memcpy(gFotaComponentStatus.components[i].componentVersion,
            xpTargetComponents[i].componentTargetVersion,
            xpTargetComponents[i].componentTargetVersionLen);
      gFotaComponentStatus.components[i].componentVersionLen = xpTargetComponents[i].componentTargetVersionLen;
      
      // Count this component for commit record
      numComponents++;
    }
  }

  // CRITICAL: Write commit flag as LAST operation to indicate campaign completion
  // This must be the final write - if power is lost before this, entire campaign is invalid
  TFotaCommitRecord commitRecord = {0};
  commitRecord.commitFlag = FOTA_COMMIT_VALID;  // 0xAA = valid/complete
  commitRecord.numComponents = numComponents;
  commitRecord.campaignState = FOTA_CAMPAIGN_INIT;  // Campaign initialized, components IN_PROGRESS
  setCommitRecord(&commitRecord);
  
  writeStatus = salFotaStorageWrite(FOTA_STORAGE_COMMIT_ID, (const uint8_t*)&commitRecord, sizeof(TFotaCommitRecord));
  if (true != writeStatus)
  {
    M_KTALOG__ERR("ERROR: Failed to write commit flag - FOTA campaign incomplete\r\n");
    retStatus = E_K_FOTA_ERROR;
    goto end;
  }
  
  M_KTALOG__INFO("SUCCESS: Commit flag written - FOTA campaign initialization complete (numComponents=%u)\r\n", 
                 (unsigned int)numComponents);

  // Start FOTA installation
  fotaStartInstalltation(xpFotaMetadata, xFotaMetadataLen, xpTargetComponents);
  retStatus = E_K_FOTA_IN_PROGRESS;
end:
  M_KTALOG__END("End, status : %d", retStatus);
  return retStatus;
}


/**
 * @brief implement fotaUpdateComponent
 */
TKFotaStatus fotaUpdateComponent
(
  const uint8_t *componentName,
  const size_t   componentNameLen,
  const uint8_t *componentVersion,
  const size_t   componentVersionLen,
  TFotaState     state
)
{
  TKFotaStatus retStatus                            =    E_K_FOTA_ERROR;
  uint8_t writeStatus                               =    false;

  M_KTALOG__START("Start");
  M_KTALOG__INFO("Comp[Name(%s)-Len(%d)]-Ver[(%s)-Len(%d)]-State[%d]\r\n",
    componentName, componentNameLen, componentVersion, componentVersionLen, state);

  // Defensive: check for valid input
  if ((NULL == componentName)    ||
      (NULL == componentVersion) ||
      (0 == componentNameLen)    ||
      (0 == componentVersionLen))
  {
    M_KTALOG__ERR("ERROR: Invalid input to fotaUpdateComponent\n");
    goto end;
  }

  // If in progress, just log and return
  if (E_FOTA_STATE_IN_PROGRESS == state)
  {
    retStatus = E_K_FOTA_IN_PROGRESS;
    goto end;
  }

  // Find the component index and update
  for (size_t i = 0; i < COMPONENTS_MAX; i++)
  {
    if ((componentNameLen == gFotaComponentStatus.components[i].componentNameLen) &&
        (0 == memcmp(componentName, gFotaComponentStatus.components[i].componentName, componentNameLen)))
    {
      // CRITICAL: Invalidate commit flag BEFORE updating component state
      // This protects against power loss during the update sequence
      TFotaCommitRecord commitRecord = {0};
      size_t commitRecordLen = sizeof(TFotaCommitRecord);
      
      // Read current commit record
      if (salFotaStorageRead(FOTA_STORAGE_COMMIT_ID, (uint8_t*)&commitRecord, &commitRecordLen) &&
          validateCommitRecord(&commitRecord))
      {
        // Only invalidate if not already in IN_PROGRESS state (optimization)
        if (commitRecord.campaignState != FOTA_CAMPAIGN_IN_PROGRESS)
        {
          // Mark campaign as IN_PROGRESS (updates happening)
          commitRecord.commitFlag = FOTA_COMMIT_INVALID;  // 0xFF = invalid during updates
          commitRecord.campaignState = FOTA_CAMPAIGN_IN_PROGRESS;
          setCommitRecord(&commitRecord);
          
          writeStatus = salFotaStorageWrite(FOTA_STORAGE_COMMIT_ID, (const uint8_t*)&commitRecord, sizeof(TFotaCommitRecord));
          if (!writeStatus)
          {
            M_KTALOG__ERR("ERROR: Failed to invalidate commit flag before component update\n");
            retStatus = E_K_FOTA_ERROR;
            goto end;
          }
          M_KTALOG__INFO("Commit flag invalidated for component update (transition to IN_PROGRESS)\n");
        }
        else
        {
          M_KTALOG__INFO("Commit already in IN_PROGRESS state, skipping invalidation\n");
        }
      }
      
      // Prepare combined component record
      // CRITICAL: Read existing record first to preserve URL field
      TFotaComponentRecord compRecord = {0};
      size_t compRecordLen = sizeof(TFotaComponentRecord);
      bool readExisting = salFotaStorageRead(FOTA_STORAGE_COMPONENT_ID(i), (uint8_t*)&compRecord, &compRecordLen);
      
      if (!readExisting || !validateComponentRecordCRC(&compRecord))
      {
        M_KTALOG__WARN("WARN: Could not read existing component record, creating new one\n");
        memset(&compRecord, 0, sizeof(TFotaComponentRecord));
        compRecord.componentNameLen = componentNameLen;
        memcpy(compRecord.componentName, componentName, componentNameLen);
      }
      
      // Update state and version (preserving URL from existing record)
      compRecord.componentVersionLen = componentVersionLen;
      memcpy(compRecord.componentVersion, componentVersion, componentVersionLen);
      compRecord.state = state;
      // Note: componentUrl and componentUrlLen preserved from existing record

      // Calculate and set CRC32 checksum for power-cycle protection
      setComponentRecordCRC(&compRecord);

      // Write entire component record with single operation
      writeStatus = salFotaStorageWrite(FOTA_STORAGE_COMPONENT_ID(i),
                                        (uint8_t*)&compRecord,
                                        sizeof(TFotaComponentRecord));
      if (true != writeStatus)
      {
        M_KTALOG__ERR("ERROR: Failed to write component record to fota storage\n");
        retStatus = E_K_FOTA_ERROR;
        goto end;
      }

      // Update state in global status
      gFotaComponentStatus.state[i] = state;

      // Check if ALL components have reached final state (SUCCESS or ERROR)
      // CRITICAL: Read from EEPROM to ensure we have accurate state (not stale RAM)
      bool allComponentsDone = true;
      uint8_t numComponentsComplete = 0;
      
      for (size_t j = 0; j < COMPONENTS_MAX; j++)
      {
        // Read component state from EEPROM (source of truth)
        TFotaComponentRecord checkRecord = {0};
        size_t checkRecordLen = sizeof(TFotaComponentRecord);
        bool readSuccess = salFotaStorageRead(FOTA_STORAGE_COMPONENT_ID(j), (uint8_t*)&checkRecord, &checkRecordLen);
        
        if (readSuccess && validateComponentRecordCRC(&checkRecord) && checkRecord.componentNameLen > 0)
        {
          numComponentsComplete++;
          
          // Sync RAM with EEPROM state
          gFotaComponentStatus.state[j] = checkRecord.state;
          
          if (checkRecord.state == E_FOTA_STATE_IN_PROGRESS)
          {
            allComponentsDone = false;
            // Don't break - continue to sync all component states
          }
        }
      }
      
      // If all components are done, re-validate commit flag with COMPLETE state
      if (allComponentsDone && numComponentsComplete > 0)
      {
        TFotaCommitRecord finalCommitRecord = {0};
        finalCommitRecord.commitFlag = FOTA_COMMIT_VALID;  // 0xAA = valid
        finalCommitRecord.numComponents = numComponentsComplete;
        finalCommitRecord.campaignState = FOTA_CAMPAIGN_COMPLETE;  // All components finished
        setCommitRecord(&finalCommitRecord);
        
        // Try to write final commit with retry on failure
        uint8_t retryCount = 0;
        const uint8_t maxRetries = 3;
        
        do {
          writeStatus = salFotaStorageWrite(FOTA_STORAGE_COMMIT_ID, 
                                            (const uint8_t*)&finalCommitRecord, 
                                            sizeof(TFotaCommitRecord));
          if (writeStatus)
          {
            M_KTALOG__INFO("SUCCESS: All components complete - commit flag validated (COMPLETE state)\n");
            break;
          }
          else
          {
            retryCount++;
            if (retryCount < maxRetries)
            {
              M_KTALOG__WARN("WARN: Final commit write failed (attempt %u/%u), retrying...\n", 
                            (unsigned int)retryCount, (unsigned int)maxRetries);
            }
            else
            {
              M_KTALOG__ERR("CRITICAL: Final commit write failed after %u attempts - components done but flag not set\n",
                           (unsigned int)maxRetries);
              M_KTALOG__ERR("CRITICAL: Campaign will need manual completion or will retry on next component query\n");
            }
          }
        } while (!writeStatus && retryCount < maxRetries);
      }

      // Update installed components if success
      if (E_FOTA_STATE_SUCCESS == state)
      {
        // Update version in global status
        memcpy(gFotaComponentStatus.components[i].componentVersion, componentVersion, componentVersionLen);
        gFotaComponentStatus.components[i].componentVersionLen = componentVersionLen;

        // Update installed components array
        for (size_t j = 0; j < COMPONENTS_MAX; j++)
        {
          if ((componentNameLen == installedComponents[j].componentNameLen) &&
          (0 == memcmp(componentName, installedComponents[j].componentName, componentNameLen)))
          {
            memcpy(installedComponents[j].componentVersion, componentVersion, componentVersionLen);
            installedComponents[j].componentVersionLen = componentVersionLen;
            break;
          }
        }
        retStatus = E_K_FOTA_SUCCESS;
      }
      else if (E_FOTA_STATE_ERROR == state)
      {
        retStatus = E_K_FOTA_ERROR;
      }
      else
      {
        M_KTALOG__ERR("ERROR: Invalid state %d\r\n", state);
        retStatus = E_K_FOTA_ERROR;
      }
      goto end;
    }
  }

  // Not found
  M_KTALOG__ERR("ERROR: Component not found for update\n");
  retStatus = E_K_FOTA_ERROR;

end:
  M_KTALOG__INFO("[INFO] fotaUpdateComponent End, Status [%d]\r\n", retStatus);
  return retStatus;
}



/* -------------------------------------------------------------------------- */
/* LOCAL FUNCTIONS - IMPLEMENTATION                                           */
/* -------------------------------------------------------------------------- */
/**
 * @implements calculateCRC32
 *
 */
static uint32_t calculateCRC32
(
  const uint8_t *data,
  size_t         length
)
{
  uint32_t crc = 0xFFFFFFFFUL;

  for (size_t i = 0; i < length; i++)
  {
    crc ^= data[i];
    for (uint8_t bit = 0; bit < 8; bit++)
    {
      if (crc & 1)
      {
        crc = (crc >> 1) ^ 0xEDB88320UL;
      }
      else
      {
        crc = crc >> 1;
      }
    }
  }

  return ~crc;
}

/**
 * @implements validateComponentRecordCRC
 *
 */
static bool validateComponentRecordCRC
(
  const TFotaComponentRecord *record
)
{
  if (record == NULL)
  {
    return false;
  }

  // Check magic number first
  if (record->magic != FOTA_RECORD_MAGIC)
  {
    M_KTALOG__ERR("[ERR] Invalid magic number: expected 0x%08X, got 0x%08X\r\n",
                  (unsigned int)FOTA_RECORD_MAGIC, (unsigned int)record->magic);
    return false;
  }

  // Calculate CRC over entire record except the CRC field itself
  size_t crcDataLen = sizeof(TFotaComponentRecord) - sizeof(uint32_t);
  uint32_t calculatedCRC = calculateCRC32((const uint8_t*)record, crcDataLen);

  if (calculatedCRC != record->crc32)
  {
    M_KTALOG__ERR("[ERR] CRC mismatch: expected 0x%08X, got 0x%08X\r\n",
                  (unsigned int)calculatedCRC, (unsigned int)record->crc32);
    return false;
  }

  return true;
}

/**
 * @implements setComponentRecordCRC
 *
 */
static void setComponentRecordCRC
(
  TFotaComponentRecord *record
)
{
  if (record == NULL)
  {
    return;
  }

  // Set magic number
  record->magic = FOTA_RECORD_MAGIC;

  // Calculate CRC over entire record except the CRC field itself
  size_t crcDataLen = sizeof(TFotaComponentRecord) - sizeof(uint32_t);
  record->crc32 = calculateCRC32((const uint8_t*)record, crcDataLen);
}

/**
 * @implements validateNameRecordCRC
 *
 */
static bool validateNameRecordCRC
(
  const TFotaNameRecord *record
)
{
  if (record == NULL)
  {
    return false;
  }

  // Check magic number first
  if (record->magic != FOTA_NAME_MAGIC)
  {
    M_KTALOG__ERR("[ERR] Invalid name magic: expected 0x%08X, got 0x%08X\r\n",
                  (unsigned int)FOTA_NAME_MAGIC, (unsigned int)record->magic);
    return false;
  }

  // Calculate CRC over entire record except the CRC field itself
  size_t crcDataLen = sizeof(TFotaNameRecord) - sizeof(uint32_t);
  uint32_t calculatedCRC = calculateCRC32((const uint8_t*)record, crcDataLen);

  if (calculatedCRC != record->crc32)
  {
    M_KTALOG__ERR("[ERR] Name CRC mismatch: expected 0x%08X, got 0x%08X\r\n",
                  (unsigned int)calculatedCRC, (unsigned int)record->crc32);
    return false;
  }

  return true;
}

/**
 * @implements setNameRecordCRC
 *
 */
static void setNameRecordCRC
(
  TFotaNameRecord *record
)
{
  if (record == NULL)
  {
    return;
  }

  // Set magic number
  record->magic = FOTA_NAME_MAGIC;

  // Calculate CRC over entire record except the CRC field itself
  size_t crcDataLen = sizeof(TFotaNameRecord) - sizeof(uint32_t);
  record->crc32 = calculateCRC32((const uint8_t*)record, crcDataLen);
}

/**
 * @implements setMetadataRecordCRC
 *
 */
static void setMetadataRecordCRC
(
  TFotaMetadataRecord *record
)
{
  if (record == NULL)
  {
    return;
  }

  // Set magic number
  record->magic = FOTA_METADATA_MAGIC;

  // Calculate CRC over entire record except the CRC field itself
  size_t crcDataLen = sizeof(TFotaMetadataRecord) - sizeof(uint32_t);
  record->crc32 = calculateCRC32((const uint8_t*)record, crcDataLen);
}

/**
 * @implements validateCommitRecord
 *
 */
static bool validateCommitRecord
(
  const TFotaCommitRecord *record
)
{
  if (record == NULL)
  {
    M_KTALOG__ERR("[ERR] validateCommitRecord: NULL pointer\r\n");
    return false;
  }

  // Check magic number first
  if (record->magic != FOTA_COMMIT_MAGIC)
  {
    M_KTALOG__ERR("[ERR] Commit record magic mismatch: expected 0x%08X, got 0x%08X\r\n",
                   (unsigned int)FOTA_COMMIT_MAGIC, (unsigned int)record->magic);
    return false;
  }

  // Calculate expected CRC over record (excluding crc32 field)
  size_t crcDataLen = sizeof(TFotaCommitRecord) - sizeof(uint32_t);
  uint32_t expectedCrc = calculateCRC32((const uint8_t*)record, crcDataLen);

  if (record->crc32 != expectedCrc)
  {
    M_KTALOG__ERR("[ERR] Commit record CRC mismatch: expected 0x%08X, got 0x%08X\r\n",
                   (unsigned int)expectedCrc, (unsigned int)record->crc32);
    return false;
  }

  return true;
}

/**
 * @implements setCommitRecord
 *
 */
static void setCommitRecord
(
  TFotaCommitRecord *record
)
{
  if (record == NULL)
  {
    return;
  }

  record->magic = FOTA_COMMIT_MAGIC;
  // Calculate CRC over entire record except the crc32 field itself
  size_t crcDataLen = sizeof(TFotaCommitRecord) - sizeof(uint32_t);
  record->crc32 = calculateCRC32((const uint8_t*)record, crcDataLen);
}

/**
 * @implements compareVersion
 *
 */
static TFotaVersionState compareVersion
(
  const uint8_t* currentVersion,
  const uint8_t* targetVersion,
  size_t length
)
{
  int result = 0;
  TFotaVersionState retState = E_FOTA_NEW_VERSION;

  result = memcmp(currentVersion, targetVersion, length);

  if (0 == result)
  {
    retState = E_FOTA_CURRENT_VERSION;
  }
  else
  {
    retState = E_FOTA_NEW_VERSION;
  }

  return retState;
}

/**
 * @implements getVersion
 *
 */
static TKFotaStatus getVersion
(
  const uint8_t *componentName,
  size_t componentNameLen,
  uint8_t *currentVersion,
  size_t *currentVersionLength
)
{
  TKFotaStatus status = E_K_FOTA_ERROR;

  if ((NULL == componentName) ||
      (NULL == currentVersion) ||
      (NULL == currentVersionLength))
  {
    status = E_K_FOTA_ERROR;
    goto end;
  }

  if (0 < componentNameLen)
  {
    // To be implemented by Integrator
    // Get version of currently installed target component
    for (size_t i = 0; i < COMPONENTS_MAX; i++)
    {
      if (0 == memcmp(componentName, installedComponents[i].componentName, installedComponents[i].componentNameLen))
      {
        memcpy(currentVersion, installedComponents[i].componentVersion,  installedComponents[i].componentVersionLen);
        *currentVersionLength = installedComponents[i].componentVersionLen;
        status = E_K_FOTA_SUCCESS;
        break;
      }
    }
  }

end:
  return status;
}

/**
 * @implements fotaAgentUpdateFotaError
 *
 */
static void fotaAgentUpdateFotaError
(
  TFotaError *error,
  const TFotaErrorCode errorCode
)
{

  uint8_t errorIndex = errorCode - 1;

  if ((FOTA_ERROR_INVALID_INPUT > errorCode)  ||
      (FOTA_ERROR_NO_COMPONENTS < errorCode))
  {
    goto end;
  }

  if ((NULL == error->fotaErrorCode) ||
      (1 > error->fotaErrorCodeLen) ||
      (NULL == error->fotaErrorCause) ||
      (ERROR_CAUSE_MAX_LEN > error->fotaErrorCauseLen)
    )
  {
    goto end;
  }

  // Set error code
  error->fotaErrorCode[0] = (uint8_t)errorCode;

  // Set error cause //
  memcpy((void *)error->fotaErrorCause, aFotaErrorCauses[errorIndex], ERROR_CAUSE_MAX_LEN);
end:
  return;
}

/**
 * @implements fotaFillComponents
 *
 */
static void fotaFillComponents
(
  TFotaState compState,
  TComponent components[COMPONENTS_MAX]
)
{
  size_t compIndex = 0;

  // Check for NULL input
  if (NULL == components)
  {
    M_KTALOG__ERR("ERROR: NULL input\n");
    goto end;
  }
  M_KTALOG__INFO("[INFO] fotaFillComponents Start\r\n");
  // Iterate through the provided components
  for (size_t i = 0; i < COMPONENTS_MAX; i++)
  {
    if ((compState == gFotaComponentStatus.state[i]) &&
        (0 < gFotaComponentStatus.components[i].componentNameLen))
    {
      // Bounds check to prevent buffer overflow
      if (compIndex >= COMPONENTS_MAX)
      {
        M_KTALOG__ERR("ERROR: Component index out of bounds\n");
        break;
      }

      // Bounds check for component name length
      size_t copyNameLen = gFotaComponentStatus.components[i].componentNameLen;
      if (copyNameLen > CURRENT_MAX_LENGTH)
      {
        M_KTALOG__ERR("ERROR: Component name length %zu exceeds max %u\n", 
                      copyNameLen, CURRENT_MAX_LENGTH);
        copyNameLen = CURRENT_MAX_LENGTH;
      }

      memcpy(components[compIndex].componentName,
             gFotaComponentStatus.components[i].componentName,
             copyNameLen);
      components[compIndex].componentNameLen = copyNameLen;

      if (0 < gFotaComponentStatus.components[i].componentVersionLen)
      {
        // Bounds check for component version length
        size_t copyVersionLen = gFotaComponentStatus.components[i].componentVersionLen;
        if (copyVersionLen > CURRENT_MAX_LENGTH)
        {
          M_KTALOG__ERR("ERROR: Component version length %zu exceeds max %u\n", 
                        copyVersionLen, CURRENT_MAX_LENGTH);
          copyVersionLen = CURRENT_MAX_LENGTH;
        }

        memcpy(components[compIndex].componentVersion,
               gFotaComponentStatus.components[i].componentVersion,
               copyVersionLen);
        components[compIndex].componentVersionLen = copyVersionLen;
      }
      compIndex++;
    }
  }
end:
  return;
}

/* -------------------------------------------------------------------------- */
/* END OF FILE                                                                */
/* -------------------------------------------------------------------------- */
