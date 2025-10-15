/*******************************************************************************
*************************keySTREAM Trusted Agent ("KTA")************************

* (c) 2023-2025 Nagravision Sï¿½?Â rl

* Subject to your compliance with these terms, you may use the Nagravision Sï¿½?Â rl
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
  FOTA_ERROR_INVALID_INPUT    = 0x01,
  FOTA_ERROR_INVALID_URL      = 0x02,
  FOTA_ERROR_OLDER_VERSION    = 0x03,
  FOTA_ERROR_UNEXPECTED_STATE = 0x04,
  FOTA_ERROR_NO_COMPONENTS    = 0x05
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
  {{0x4D, 0x43, 0x48, 0x50, 0x2D, 0x43, 0x41, 0x4C}, 8, {0x31, 0x2E, 0x30, 0x2E, 0x30}, 5},
  { { }, 0, { }, 0 },
  { { }, 0, { }, 0 },
  { { }, 0, { }, 0 },
  { { }, 0, { }, 0 },
  { { }, 0, { }, 0 },
  { { }, 0, { }, 0 },
  { { }, 0, { }, 0 }
};

static bool fotaInialized = false;
/* -------------------------------------------------------------------------- */
/* LOCAL FUNCTIONS - PROTOTYPE                                                */
/* -------------------------------------------------------------------------- */

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
  TComponent components[COMPONENTS_MAX]                   = {0};
  TKFotaStatus retStatus                                  = E_K_FOTA_ERROR;
  uint8_t targetVersion[CURRENT_MAX_LENGTH]               = {0};
  size_t targetVersionLen                                 = CURRENT_MAX_LENGTH;
  uint8_t stateBuffer[FOTA_STATE_LEN]                    = {0};
  size_t stateLen                                         = FOTA_STATE_LEN;


  M_KTALOG__START("[INFO] initializefotaState Start\r\n");

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
     // Read target component version
    bool readTarget = salFotaStorageRead(FOTA_STORAGE_TARGET_COMPONENT_ID, targetVersion, &targetVersionLen);
    // Read state
    bool readState = salFotaStorageRead(FOTA_STORAGE_STATE_ID, stateBuffer, &stateLen);

    if ((true != readTarget) ||
        (true != readState)  ||
        (FOTA_STATE_LEN != stateLen))
    {
      M_KTALOG__ERR("ERROR: Failed to read target component or state from storage.\r\n");
      goto end;
    }

    gFotaComponentStatus.state[i] = stateBuffer[0];

    memcpy(gFotaComponentStatus.components[i].componentName,
           installedComponents[i].componentName,
           installedComponents[i].componentNameLen);
    gFotaComponentStatus.components[i].componentNameLen = installedComponents[i].componentNameLen;
    memcpy(gFotaComponentStatus.components[i].componentVersion,
           targetVersion,
           targetVersionLen);

    gFotaComponentStatus.components[i].componentVersionLen = targetVersionLen;

  }

  fotaInialized = true;
end:  M_KTALOG__END("[INFO] fotaAgentGetDeviceInfo End\r\n");
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

  M_KTALOG__START("[INFO] fotaGetInstalledComponents Start\r\n");

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
  M_KTALOG__END("[INFO] fotaGetInstalledComponents End, status [%d]\r\n", retStatus);
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

  M_KTALOG__START("[INFO] validateComponents Start\r\n");

  for (size_t i = 0; i < COMPONENTS_MAX; i++)
  {
    if ((0 < xTargetComponents[i].componentTargetNameLen) &&
        (0 < xTargetComponents[i].componentTargetVersionLen))
    {
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
  M_KTALOG__END("[INFO] validateComponents End, Status [%d]\r\n", retStatus);
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
  M_KTALOG__START("[INFO] storeMetadata Start\r\n");

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

  // Log metadata storage attempt
  M_KTALOG__DEBUG("DEBUG: Storing FOTA metadata for %.*s\r\n", (int)xFotaNameLen, xFotaName);
  M_KTALOG__DEBUG("DEBUG: Metadata: %.*s\r\n", (int)xFotaMetadataLen, xFotaMetadata);

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
  M_KTALOG__END("[INFO] storeMetadata End, Status [%d]\r\n", retStatus);
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
  TKFotaStatus retStatus                             = E_K_FOTA_ERROR;
  TFotaState   compState                             = E_FOTA_STATE_IDLE;
  bool         state_error                           = 0;
  bool         state_success                         = 0;
  uint8_t      flashFotaNameBuf[CURRENT_MAX_LENGTH]  = {0};
  size_t       flashFotaNameLen                      = CURRENT_MAX_LENGTH;
  uint8_t      stateBuffer[1]                        = {0};
  size_t       stateLen                              = 1;
  bool         readStatus                            = false;

  M_KTALOG__START("[INFO] fotaGetComponentsStatus Start\r\n");

  // Read FOTA name from flash
  readStatus = salFotaStorageRead(FOTA_STORAGE_NAME_ID, flashFotaNameBuf, &flashFotaNameLen);

  // Simple: if not same, set error and exit
  if (0 != memcmp(flashFotaNameBuf, xpFotaName, fotaNameLen))
  {
    fotaAgentUpdateFotaError(xpFotaError, FOTA_ERROR_INVALID_INPUT);
    fotaFillComponents(E_FOTA_STATE_ERROR, components);
    retStatus = E_K_FOTA_ERROR;
    M_KTALOG__ERR("ERROR: FOTA name mismatch. Expected: %.*s, Found: %.*s\r\n",
                   (int)fotaNameLen, xpFotaName,
                   (int)flashFotaNameLen, flashFotaNameBuf);
    goto end;
  }

  // Read only one component state (1 byte) from FOTA_STORAGE_STATE_ID
  readStatus = salFotaStorageRead(FOTA_STORAGE_STATE_ID, stateBuffer, &stateLen);
  if ((true != readStatus) ||
      (FOTA_STATE_LEN != stateLen))
  {
    M_KTALOG__ERR("ERROR: Failed to read FOTA component state from storage\r\n");
    fotaAgentUpdateFotaError(xpFotaError, FOTA_ERROR_UNEXPECTED_STATE);
    fotaFillComponents(E_FOTA_STATE_ERROR, components);
    retStatus = E_K_FOTA_ERROR;
    M_KTALOG__ERR("ERROR: Failed to read FOTA component state from storage\r\n");
    goto end;
  }
  gFotaComponentStatus.state[0] = stateBuffer[0];

  // Iterate through all components to check their status
  for (size_t i = 0; i < COMPONENTS_MAX; i++)
  {
    compState = gFotaComponentStatus.state[i];
    if (E_FOTA_STATE_IN_PROGRESS == compState)
    {
      retStatus = E_K_FOTA_IN_PROGRESS;
      break;
    }
    else if (E_FOTA_STATE_ERROR == compState)
    {
      state_error = 1;
    }
    else
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
  M_KTALOG__END("[INFO] fotaGetComponentsStatus End, Status [%d]\r\n", retStatus);
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
  TKFotaStatus retStatus       =     E_K_FOTA_ERROR; // Initial status
  uint8_t writeStatus          =     false;

  M_KTALOG__START("[INFO] fotaDownloadAndInstall Start\r\n");

  if (NULL == xpFotaName)
  {
    M_KTALOG__ERR("ERROR: xpFotaName is NULL\r\n");
    goto end;
  }
  memcpy(gFotaComponentStatus.xpFotaName, xpFotaName, xFotaNameLen);
  gFotaComponentStatus.xFotaNameLen = xFotaNameLen;

  writeStatus = salFotaStorageWrite(FOTA_STORAGE_NAME_ID, xpFotaName, xFotaNameLen);
  if (true != writeStatus)
  {
    M_KTALOG__ERR("ERROR: Failed to write FOTA name to SAL storage\r\n");
    retStatus = E_K_FOTA_ERROR;
    goto end;
  }

  // Iterate through all target components
  for (size_t i = 0; i < COMPONENTS_MAX; i++)
  {
    if (0 < xpTargetComponents[i].componentTargetNameLen)
    {
      // Copy component details to global struct
      memcpy(gFotaComponentStatus.components[i].componentName,
            xpTargetComponents[i].componentTargetName,
            xpTargetComponents[i].componentTargetNameLen);
      gFotaComponentStatus.components[i].componentNameLen = xpTargetComponents[i].componentTargetNameLen;

      memcpy(gFotaComponentStatus.components[i].componentVersion,
            xpTargetComponents[i].componentTargetVersion,
            xpTargetComponents[i].componentTargetVersionLen);
      gFotaComponentStatus.components[i].componentVersionLen = xpTargetComponents[i].componentTargetVersionLen;

      // Simulating the component update process for each components (success or in_progress or failure) for downloading.
      // Store status in global struct for each components
      gFotaComponentStatus.state[i] = E_FOTA_STATE_IN_PROGRESS;
      writeStatus = salFotaStorageWrite(FOTA_STORAGE_STATE_ID,
                                        &gFotaComponentStatus.state[i],
                                        sizeof(gFotaComponentStatus.state[i]));
      if (true != writeStatus)
      {
        M_KTALOG__ERR("ERROR: Failed to write installed component version to SAL storage\n");
        retStatus = E_K_FOTA_ERROR;
        goto end;
      }

      writeStatus = salFotaStorageWrite(FOTA_STORAGE_TARGET_COMPONENT_ID,
                                        xpTargetComponents[i].componentTargetVersion,
                                        xpTargetComponents[i].componentTargetVersionLen);

      if (!writeStatus)
      {
        M_KTALOG__ERR("ERROR: Failed to write Target component version to storage\n");
        retStatus = E_K_FOTA_ERROR;
        goto end;
      }
    }
  }
  // Start FOTA installation
  fotaStartInstalltation(xpFotaMetadata, xFotaMetadataLen, xpTargetComponents);
  retStatus = E_K_FOTA_IN_PROGRESS;
end:
  M_KTALOG__END("[INFO] fotaDownloadAndInstall End, Status [%d]\r\n", retStatus);
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
  uint8_t stateFlashBuffer[FOTA_STATE_LEN]          =    {0};

  M_KTALOG__START("[INFO] fotaUpdateComponent Start\r\n");
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

  // Find the component index and update
  for (size_t i = 0; i < COMPONENTS_MAX; i++)
  {
    if ((componentNameLen == installedComponents[i].componentNameLen) &&
        (0 == memcmp(componentName, installedComponents[i].componentName, componentNameLen)))
    {
      // Update state in global status and flash
      gFotaComponentStatus.state[i] = state;
      // Set stateFlashBuffer using memcpy for standard compliance
      memcpy(stateFlashBuffer, (uint8_t*) &state, sizeof(stateFlashBuffer));
      writeStatus = salFotaStorageWrite(FOTA_STORAGE_STATE_ID,
                                        stateFlashBuffer,
                                        sizeof(stateFlashBuffer));
      if (true != writeStatus)
      {
        M_KTALOG__ERR("ERROR: Failed to write state to fota storage\n");
        retStatus = E_K_FOTA_ERROR;
        goto end;
      }

      // If in progress, just log and return
      if (E_FOTA_STATE_IN_PROGRESS == state)
      {
        M_KTALOG__INFO("DEBUG: Component [%s] is in progress\r\n", componentName);
        retStatus = E_K_FOTA_IN_PROGRESS;
        goto end;
      }

      // Write name to flash if success
      if (E_FOTA_STATE_SUCCESS == state)
      {
        memcpy(installedComponents[i].componentVersion, componentVersion, componentVersionLen);
        installedComponents[i].componentVersionLen = componentVersionLen;

        writeStatus = salFotaStorageWrite(FOTA_STORAGE_INSTALLED_COMPONENT_ID,
                                          componentVersion,
                                          componentVersionLen);
        if (true != writeStatus)
        {
          M_KTALOG__ERR("ERROR: Failed to write installed component version to SAL storage\n");
          retStatus = E_K_FOTA_ERROR;
          goto end;
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
  (void)compState;
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
      memcpy(components[compIndex].componentName,
              gFotaComponentStatus.components[i].componentName,
              gFotaComponentStatus.components[i].componentNameLen);
              components[compIndex].componentNameLen = gFotaComponentStatus.components[i].componentNameLen;

      memcpy(components[compIndex].componentVersion,
        gFotaComponentStatus.components[i].componentVersion,
        gFotaComponentStatus.components[i].componentVersionLen);
      components[compIndex].componentVersionLen = gFotaComponentStatus.components[i].componentVersionLen;
      compIndex++;
  }
end:
  return;
}

/* -------------------------------------------------------------------------- */
/* END OF FILE                                                                */
/* -------------------------------------------------------------------------- */
