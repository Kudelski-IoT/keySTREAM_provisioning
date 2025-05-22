
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
/** \brief  Fota Agent Process.
 *
 *  \author Kudelski IoT
 *
 *  \date 2025/02/04
 *
 *  \file Fota_Process.h
 ******************************************************************************/

/**
 * @brief Interface for Fota processing.
 */
#ifndef FOTA_PROCESS_H
#define FOTA_PROCESS_H

/* -------------------------------------------------------------------------- */
/* IMPORTS                                                                    */
/* -------------------------------------------------------------------------- */
#include "Fota_Agent.h"
#include "k_sal_fota.h"
#include <stddef.h>
#include <stdint.h>

/* -------------------------------------------------------------------------- */
/* CONSTANTS, TYPES, ENUM                                                     */
/* -------------------------------------------------------------------------- */

/** @brief Enumeration for the FOTA state. */
typedef enum {
  E_FOTA_STATE_SUCCESS = 0x00,
  /* FOTA completed successfully. */
  E_FOTA_STATE_IN_PROGRESS = 0x01,
  /* FOTA is in progress. */
  E_FOTA_STATE_ERROR = 0x02,
  /* FOTA encountered an error. */
  E_FOTA_STATE_IDLE = 0xFF
  /* FOTA is idle. */
} TFotaState;

/** @brief Structure holds the status and information of FOTA components.
 */
typedef struct {
  uint8_t xpFotaName[MAX_COMPONENT_NAME_LEN];
  /**< Array to store the FOTA name. */

  size_t xFotaNameLen;
  /**< Length of the FOTA name. */

  TComponent components[COMPONENTS_MAX];
  /**< Array of components involved in the FOTA process. */

  TFotaState state[COMPONENTS_MAX];
  /**< Array of state values for each component. */
} TFotaComponentStatus;



/* -------------------------------------------------------------------------- */
/* FUNCTIONS                                                                  */
/* -------------------------------------------------------------------------- */

/**
 * @brief Set the global FOTA state.
 *
 * This function sets the global FOTA state to the provided state.
 *
 * @return None
 */
void initializefotaState
(
  void
);

/**
 * @brief Validates the components for the FOTA process.
 *
 * This function validates the components involved in the FOTA process.
 * Checks the versions of the components and updates their status accordingly.
 *
 * @param [in] xTargetComponents Array of target components to be validated.
 * @param [out] xFotaError Pointer to the FOTA error structure.
 * @param [out] xComponents Array to store the validated components information.
 * 
 * @return TKFotaStatus indicating the status of the validation.
 *         - E_K_FOTA_SUCCESS: Successfully validated the components.
 *         - E_K_FOTA_ERROR: Failed to validate the components.
 */
TKFotaStatus validateComponents
(
  const TTargetComponent xTargetComponents[COMPONENTS_MAX],
  TFotaError            *xFotaError,
  TComponent             xComponents[COMPONENTS_MAX]
);

/**
 * @brief Stores the FOTA metadata.
 *
 * This function stores the metadata associated with the FOTA process.
 * It validates the input parameters and updates the FOTA error structure.
 *
 * @param [in] xFotaName Pointer to the FOTA name.
 * @param [in] xFotaNameLen Length of the FOTA name.
 * @param [in] xFotaMetadata Pointer to the FOTA metadata.
 * @param [in] xFotaMetadataLen Length of the FOTA metadata.
 * @param [out] xFotaError Pointer to the FOTA error structure.
 * 
 * @return TKFotaStatus indicating the status of the operation.
 *         - E_K_FOTA_SUCCESS: Successfully stored the FOTA metadata.
 *         - E_K_FOTA_ERROR: Failed to store the FOTA metadata.
 */
TKFotaStatus storeMetadata
(
  const uint8_t *xFotaName,
  size_t         xFotaNameLen,
  const uint8_t *xFotaMetadata,
  size_t         xFotaMetadataLen,
  TFotaError    *xFotaError
);

/**
 * @brief Download and install the FOTA update.
 *
 * This function sets the global state to IN_PROGRESS. Saves the FotaName to a
 * global variable. Iterates over each component in the xTargetComponents array,
 * validates the length of the component name and version.
 * This Function calls thread function() to be implemented by Integrator as per
 * platform.
 * 
 *
 * @param[in] xpFotaName
 *   Pointer to the FOTA name.
 *   Should not be NULL.
 * @param[in] xFotaNameLen
 *   Length of the FOTA name.
 * @param[in] xpTargetComponents
 *   Array of target components.
 *   Should not be NULL.
  * @param[out] components
 *   Array of currently installed components.
 * 
 * @return
 * - E_K_FOTA_SUCCESS in case of success.
 * - E_K_FOTA_IN_PROGRESS in case of FOTA in progress.
 * - E_K_FOTA_ERROR for other errors.
 */
TKFotaStatus fotaDownloadAndInstall
(
  const uint8_t *xpFotaName,
  const size_t xFotaNameLen,
  const TTargetComponent *xpTargetComponents,
  TComponent xComponents[COMPONENTS_MAX]
);

/**
 * @brief Gets the status of the FOTA update.
 * 
 * Overall Fota update status will be provided to caller. Upon failed or successful
 * installation, structure components will be updated.
 *
 * @param[in] xFotaName
 *   Pointer to the FOTA name.
 *   Should not be NULL.
 * @param[in] xFotaNameLen
 *   Length of the FOTA name.
 * @param[out] xFotaError
 *   Pointer to the FOTA error structure.
 *   Should not be NULL.
 * @param[out] components
 *   Array of currently installed components.
 *
 * @return
 * - E_K_FOTA_SUCCESS in case of success.
 * - E_K_FOTA_IN_PROGRESS in case of FOTA in progress.
 * - E_K_FOTA_ERROR for other errors.
 */
TKFotaStatus fotaGetComponentsStatus
(
  const uint8_t *xpFotaName,
  size_t         fotaNameLen,
  TFotaError    *xpFotaError,
  TComponent     components[COMPONENTS_MAX]
);

/**
 * @brief This API will get all component versions installed on platform.
 *
 *
 * @param[out] xpComponents
 *   Array of target components.
 *   Should not be NULL.
 * 
 * @return
 * - E_K_FOTA_SUCCESS in case of success.
 * - E_K_FOTA_ERROR for other errors.
 */
TKFotaStatus fotaGetInstalledComponents
(
  TComponent *xpComponents
);

/**
 * @brief Updates the information of a specific component in the FOTA process.
 *
 * This function updates the name, version and fota status of a specific component
 * in the FOTA process. It iterates through the list of components and updates
 * the component that matches the provided name.
 *
 * @param [in] componentName Pointer to the component name to be updated.
 * @param [in] componentNameLen Length of the component name.
 * @param [in] componentVersion Pointer to the new component version.
 * @param [in] componentVersionLen Length of the new component version.
 * @param [in] state Fota installation state of component
 * 
 * @return TKFotaStatus indicating the status of the operation.
 *         - E_K_FOTA_SUCCESS: Successfully updated the component information.
 *         - E_K_FOTA_ERROR: Failed to update the component information.
 */
TKFotaStatus fotaUpdateComponent
(
    const uint8_t *componentName,
    const size_t componentNameLen,
    const uint8_t *componentVersion,
    const size_t componentVersionLen,
    TFotaState state
);


#endif // FOTA_PROCESS_H

/* -------------------------------------------------------------------------- */
/* END OF FILE                                                                */
/* -------------------------------------------------------------------------- */
