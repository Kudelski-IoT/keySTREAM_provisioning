
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
/** \brief  Interface for Fota Agent.
 *
 *  \author Kudelski IoT
 *
 *  \date 2025/02/04
 *
 *  \file Fota_Agent.h
 ******************************************************************************/

/**
 * @brief Interface for Fota operation.
 */

#ifndef FOTA_AGENT_H
#define FOTA_AGENT_H

/* -------------------------------------------------------------------------- */
/* IMPORTS                                                                    */
/* -------------------------------------------------------------------------- */
#include "k_sal_fota.h"
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>


/* -------------------------------------------------------------------------- */
/* FOTA LOG                                                                  */
/* -------------------------------------------------------------------------- */
#define C_KTA_APP__LOG                      printf

//#define APP_DebugPrintf printf
/* -------------------------------------------------------------------------- */
/* CONSTANTS, TYPES, ENUM                                                     */
/* -------------------------------------------------------------------------- */

/** @brief Number of FOTA error causes. */
#define FOTA_ERROR_COUNT                             (5u)

/** @brief Maximum length of the error cause. */
#define ERROR_CAUSE_MAX_LEN                          (16u)

// Maximum length of FOTA metadata
#define MAX_FOTA_META_DATA                           (32u)

// Maximum length of component name
#define MAX_COMPONENT_NAME_LEN                       (16u)

// Maximum length of component version
#define MAX_COMPONENT_VERSION_LEN                    (16u)

/** @brief Maximum size of the URL for components. */
#define MAX_SIZE_URL                                 (2048u)

/** @brief Enum for version comparison states */
typedef enum {
  E_FOTA_NEW_VERSION,
  E_FOTA_CURRENT_VERSION,
  E_FOTA_OLDER_VERSION
} TFotaVersionState;

/* -------------------------------------------------------------------------- */
/* FUNCTIONS                                                                  */
/* -------------------------------------------------------------------------- */

/**
 * @brief Initializes the FOTA agent.
 *
 * This function initializes the FOTA agent, setting up any necessary state or resources
 * required for the FOTA process. It should be called before any other FOTA functions
 * are used.
 *
 * @return void
 */
void fotaAgentInit
(
  void
);

/**
 * @brief Install the FOTA updates.
 *
 * @param[in] xFotaName
 *   Pointer to the FOTA name.
 *   Should not be NULL.
 * @param[in] xFotaNameLen
 *   Length of the FOTA name.
 * @param[in] xFotaMetadata
 *   Pointer to the FOTA metadata.
 *   Should not be NULL.
 * @param[in] xFotaMetadataLen
 *   Length of the FOTA metadata.
 * @param[in] xTargetComponents
 *   Array of target components for FOTA.
 *   Should not be NULL.
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
TKFotaStatus fotaAgentInstall
(
  const uint8_t         *xpFotaName,
  const size_t           xFotaNameLen,
  const uint8_t         *xpFotaMetadata,
  const size_t           xFotaMetadataLen,
  const TTargetComponent xTargetComponents[COMPONENTS_MAX],
  TFotaError            *xpFotaError,
  TComponent             xComponents[COMPONENTS_MAX]
);


/**
 * @brief Gets the status of the FOTA update.
 *
 * @param[in] xFotaName
 *   Pointer to the FOTA name to check status of installation.
 *   Should not be NULL.
 * @param[in] xFotaNameLen
 *   Length of the FOTA name.
 * @param[out] xFotaError
 *   Pointer to the FOTA error structure.
 *   Should not be NULL.
 * @param[out] components
 *   Array of currently installed components.
 *   It will be filled with error/installed components upon ERROR/SUCCESS cases.
 *   If any component installation is in progress, components not filled.
 * 
 * @return
 * - E_K_FOTA_SUCCESS in case of success.
 * - E_K_FOTA_IN_PROGRESS in case of FOTA in progress.
 * - E_K_FOTA_ERROR for other errors.
 */
TKFotaStatus fotaAgentGetStatus
(
  const uint8_t *xpFotaName,
  size_t         xFotaNameLen,
  TFotaError    *xpFotaError,
  TComponent     xComponents[COMPONENTS_MAX]
);


/**
 * @brief Retrieves the components information installed on the device.
 *
 * This function populates the provided array with information about the
 * components installed on the device.
 * It checks for NULL parameters and returns an error status if any are found.
 *
 * @param[out] components
 *   Array of currently installed components.
 * 
 * @return TKFotaStatus indicating the status of the operation.
 *         - E_K_FOTA_SUCCESS: Successfully retrieved the components information.
 *         - E_K_FOTA_ERROR: Failed to retrieve the components information.
 */
TKFotaStatus fotaAgentGetDeviceInfo
(
  TComponent components[COMPONENTS_MAX]
);

#endif // FOTA_AGENT_H

/* -------------------------------------------------------------------------- */
/* END OF FILE                                                                */
/* -------------------------------------------------------------------------- */
