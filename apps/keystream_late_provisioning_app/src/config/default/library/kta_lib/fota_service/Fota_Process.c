/* -------------------------------------------------------------------------- */
/* IMPORTS                                                                    */
/* -------------------------------------------------------------------------- */
#include "k_sal_fota.h"
#include "Fota_Agent.h"
#include "Fota_Process.h"
#include "Fota_Platform.h"

#include <stdio.h>
#include <string.h>
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>


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

/* Currently installed components */
TComponent installedComponents[COMPONENTS_MAX] =
{
  {{0x4D, 0x43, 0x48, 0x50, 0x2D, 0x43, 0x41, 0x4C}, 8, {0x31, 0x2E, 0x30, 0x2E, 0x30}, 5}, // "MCHP-CAL" -> "1.0.0"
  {{0}, 0, {0}, 0},
  {{0}, 0, {0}, 0},
  {{0}, 0, {0}, 0},
  {{0}, 0, {0}, 0},
  {{0}, 0, {0}, 0},
  {{0}, 0, {0}, 0},
  {{0}, 0, {0}, 0},
};

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
  TFotaError          *error,
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
 * 
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
  
  //C_KTA_APP__LOG("[INFO] initializefotaState Start\r\n");
 
  // Get installed component information from platform
  retStatus = fotaPlatformGetComponents(components);
  if(E_K_FOTA_SUCCESS != retStatus)
  {
    C_KTA_APP__LOG("ERROR: Unable to get components information from platform");
    goto end;
  }
  
  // Update platform components in global structure
  for(size_t i=0; i<COMPONENTS_MAX; i++)
  {
      if (0 > components[i].componentNameLen ||
          0 > components[i].componentVersionLen)
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
  for (size_t i=0; i < COMPONENTS_MAX; i++)
  {
    gFotaComponentStatus.state[i] = E_FOTA_STATE_IDLE;
  }

end:
  //C_KTA_APP__LOG("[INFO] fotaAgentGetDeviceInfo End\r\n");
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

  //C_KTA_APP__LOG("[INFO] fotaGetInstalledComponents Start\r\n");

  if (NULL == xpComponents)
  {
    goto end;
  }
  // Get name and versions of components installed and update xpComponents structure
  for(size_t i=0; i<COMPONENTS_MAX; i++)
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
  //C_KTA_APP__LOG("[INFO] fotaGetInstalledComponents End, status [%d]\r\n", retStatus);
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
  //C_KTA_APP__LOG("[INFO] validateComponents Start\r\n");
  
  for (size_t i = 0; i < COMPONENTS_MAX; i++)
  {
    if (0 < xTargetComponents[i].componentTargetNameLen &&
        0 < xTargetComponents[i].componentTargetVersionLen) 
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

        case E_FOTA_OLDER_VERSION:
        {
          /* Target version is older than the current installed version */
          fotaAgentUpdateFotaError(xFotaError, FOTA_ERROR_OLDER_VERSION);

          memcpy(xComponents[i].componentName, xTargetComponents[i].componentTargetName,
                  xTargetComponents[i].componentTargetNameLen);
          xComponents[i].componentNameLen = xTargetComponents[i].componentTargetNameLen;

          memcpy(xComponents[i].componentVersion, xTargetComponents[i].componentTargetVersion,
                  xTargetComponents[i].componentTargetVersionLen);
          xComponents[i].componentVersionLen = xTargetComponents[i].componentTargetVersionLen;
          retStatus = E_K_FOTA_ERROR;
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
          C_KTA_APP__LOG("ERROR: Invalid version %d", versionState);
        }
        break;
      }
    }
  }
  //C_KTA_APP__LOG("[INFO] validateComponents End, Status [%d]\r\n", retStatus);
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
  
  //C_KTA_APP__LOG("[INFO] storeMetadata Start\r\n");

  // Validate input parameters check
  if ((xFotaName == NULL ||
      xFotaMetadata == NULL ||
      xFotaNameLen == 0 ||
      xFotaMetadataLen == 0))
  {
    C_KTA_APP__LOG("ERROR: Invalid parameter passed\r\n");
    fotaAgentUpdateFotaError(xFotaError, FOTA_ERROR_INVALID_INPUT);
    goto end;
  }

  // Log metadata storage attempt
  C_KTA_APP__LOG("DEBUG: Storing FOTA metadata for %.*s\r\n", (int)xFotaNameLen, xFotaName);
  C_KTA_APP__LOG("DEBUG: Metadata: %.*s\r\n", (int)xFotaMetadataLen, xFotaMetadata);

  // Check metadata size limit
  if (xFotaMetadataLen > MAX_FOTA_META_DATA)
  {
    fotaAgentUpdateFotaError(xFotaError, FOTA_ERROR_INVALID_INPUT);
    goto end;
  }

  // Store metadata
  memcpy(gFotaMetadata, xFotaMetadata, xFotaMetadataLen);
  gFotaMetadataLen = xFotaMetadataLen;
  retStatus = E_K_FOTA_SUCCESS;
end:
  //C_KTA_APP__LOG("[INFO] storeMetadata End, Status [%d]\r\n", retStatus);
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

  //C_KTA_APP__LOG("[INFO] fotaGetComponentsStatus Start\r\n");

  // Validate FOTA name
  if (fotaNameLen != gFotaComponentStatus.xFotaNameLen ||
    memcmp(gFotaComponentStatus.xpFotaName, xpFotaName, fotaNameLen) != 0) 
  {
    C_KTA_APP__LOG("ERROR: FOTA name mismatch\r\n");
    goto end;
  }

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
  //C_KTA_APP__LOG("[INFO] fotaGetComponentsStatus End, Status [%d]\r\n", retStatus);
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
  const TTargetComponent *xpTargetComponents,
  TComponent              xComponents[COMPONENTS_MAX]
)
{
  TKFotaStatus retStatus = E_K_FOTA_ERROR; // Initial status

  //C_KTA_APP__LOG("[INFO] fotaDownloadAndInstall Start\r\n");

  if (xpFotaName == NULL)
  {
    C_KTA_APP__LOG("ERROR: xpFotaName is NULL\r\n");
    goto end;
  }
  memcpy(gFotaComponentStatus.xpFotaName, xpFotaName, xFotaNameLen);
  gFotaComponentStatus.xFotaNameLen = xFotaNameLen;

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
    }
  }
  // Start FOTA installation
  fotaStartInstalltation(xpTargetComponents);
  
  retStatus = E_K_FOTA_IN_PROGRESS;
end:
  //C_KTA_APP__LOG("[INFO] fotaDownloadAndInstall End, Status [%d]\r\n", retStatus);
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
  TKFotaStatus retStatus = E_K_FOTA_ERROR; // Initialize status to error

 // C_KTA_APP__LOG("[INFO] fotaDownloadAndInstall Start\r\n");

  // Check for NULL input
  if (NULL == componentName || NULL == componentVersion)
  {
    C_KTA_APP__LOG("ERROR: NULL input\n");
    goto end; // Return error if input is NULL
  }

  for (size_t i = 0; i < COMPONENTS_MAX; i++)
  {
    // Compare component names
    if ((componentNameLen > 0) &&
        (0 == memcmp(componentName,
                      gFotaComponentStatus.components[i].componentName,
                      componentNameLen)))
    {
      gFotaComponentStatus.state[i] = state;
      if (E_FOTA_STATE_SUCCESS == state)
      {
        for (size_t j = 0; j < COMPONENTS_MAX; j++)
        {
          /* Update installed components list with updated version*/
          if (0 == memcmp(componentName,
            installedComponents[j].componentName,
            componentNameLen))
          {
            memcpy(installedComponents[j].componentVersion,
                   componentVersion,
                  componentVersionLen);
            break;
          }
        }
      }
      retStatus = E_K_FOTA_SUCCESS;
      break; // Break inner loop once a match is found
    }
  }

end:
  //C_KTA_APP__LOG("[INFO] fotaUpdateComponent End, Status [%d]\r\n", retStatus);
  return retStatus; // Return success status
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

  if (result < 0)
  {
    retState = E_FOTA_NEW_VERSION;
  }
  else if (result > 0)
  {
    retState = E_FOTA_OLDER_VERSION;
  }
  else
  {
    retState = E_FOTA_CURRENT_VERSION;
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

  if (NULL == componentName  ||
      NULL == currentVersion  ||
      NULL == currentVersionLength)
  {
    status = E_K_FOTA_ERROR;
    goto end;
  }

  if (componentNameLen > 0)
  {
    // To be implemented by Integrator
    // Get version of currently installed target component
    for(size_t i = 0; i < COMPONENTS_MAX; i++)
    {
      if(0 == memcmp(componentName, installedComponents[i].componentName, installedComponents[i].componentNameLen))
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
    return;
  }

  if ((NULL == error->fotaErrorCode) ||
      (1 > error->fotaErrorCodeLen) ||
      (NULL == error->fotaErrorCause) ||
      (ERROR_CAUSE_MAX_LEN > error->fotaErrorCauseLen)
    )
  {
    return;
  }

  // Set error code
  error->fotaErrorCode[0] = (uint8_t)errorCode;

  // Set error cause //
  memcpy((void *)error->fotaErrorCause, aFotaErrorCauses[errorIndex], ERROR_CAUSE_MAX_LEN);
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
      C_KTA_APP__LOG("ERROR: NULL input\n");
      goto end;
  }

  // Iterate through the provided components
  for (size_t i = 0; i < COMPONENTS_MAX; i++)
  {
    // Copy component details to global struct
    if(compState == gFotaComponentStatus.state[i])
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
  }
end:
  return;
}

/* -------------------------------------------------------------------------- */
/* END OF FILE                                                                */
/* -------------------------------------------------------------------------- */
