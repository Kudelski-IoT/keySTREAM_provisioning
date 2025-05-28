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
/** \brief  Fota Agent for Mircrochip.
 *
 *  \author Kudelski IoT
 *
 *  \date 2025/01/22
 *
 *  \file Fota_Agent.c
 ******************************************************************************/

/**
 * @brief Fota Agent for Mircrochip.
 */

#include "Fota_Agent.h"
/* -------------------------------------------------------------------------- */
/* IMPORTS                                                                    */
/* -------------------------------------------------------------------------- */

#include "k_sal_fota.h"
#include "Fota_Process.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>


/* -------------------------------------------------------------------------- */
/* GLOBAL STATE                                                               */
/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */
/* PUBLIC FUNCTIONS - IMPLEMENTATION                                          */
/* -------------------------------------------------------------------------- */

/**
 * @brief  implement fotaAgentInit
 *
 */
void fotaAgentInit
(
  void
)
{
  C_KTA_APP__LOG("[INFO] fotaAgentInit Start\r\n");
  // Initialize the global state to IDLE
  initializefotaState();
  C_KTA_APP__LOG("[INFO] fotaAgentInit End\r\n");
}

/**
 * @brief  implement fotaAgentInstall
 *
 */
TKFotaStatus fotaAgentInstall
(
  const uint8_t *xpFotaName,
  const size_t xFotaNameLen,
  const uint8_t *xpFotaMetadata,
  const size_t xFotaMetadataLen,
  const TTargetComponent xTargetComponents[COMPONENTS_MAX],
  TFotaError *xpFotaError,
  TComponent xComponents[COMPONENTS_MAX]
)
{
  TKFotaStatus retStatus = E_K_FOTA_ERROR;
  //C_KTA_APP__LOG("[INFO] fotaAgentInstall Start\r\n");
  
  if ((NULL == xpFotaName) ||
      (NULL == xpFotaMetadata) ||
      (NULL == xTargetComponents))
  {
    C_KTA_APP__LOG("ERROR: Invalid input parameter\r\n");
    retStatus = E_K_FOTA_ERROR;
    goto end;
  }

  retStatus = validateComponents(xTargetComponents, xpFotaError, xComponents);
  if (retStatus != E_K_FOTA_SUCCESS && retStatus != E_K_FOTA_IN_PROGRESS)
  {
    C_KTA_APP__LOG("ERROR: validateComponents Failed\r\n");
    goto end;
  }

  retStatus = storeMetadata(xpFotaName, xFotaNameLen, xpFotaMetadata, xFotaMetadataLen, xpFotaError);
  if (retStatus != E_K_FOTA_SUCCESS)
  {
    C_KTA_APP__LOG("ERROR: storeMetadata Failed\r\n");
    goto end;
  }

  retStatus = fotaDownloadAndInstall(xpFotaName, xFotaNameLen, xTargetComponents, xComponents);
  if (retStatus != E_K_FOTA_SUCCESS && retStatus != E_K_FOTA_IN_PROGRESS)
  {
    C_KTA_APP__LOG("ERROR: fotaDownloadAndInstall Failed\r\n");
    retStatus = E_K_FOTA_ERROR;
  }

end:
  //C_KTA_APP__LOG("[INFO] fotaAgentInstall End, status [%d]\r\n", retStatus);
  return retStatus;
}

/**
 * @brief  implement fotaAgentGetStatus
 *
 */
TKFotaStatus fotaAgentGetStatus
(
  const uint8_t *xpFotaName,
  size_t fotaNameLen,
  TFotaError *xpFotaError,
  TComponent components[COMPONENTS_MAX]
)
{
  TKFotaStatus retStatus = E_K_FOTA_ERROR;
  
  //C_KTA_APP__LOG("[INFO] fotaAgentGetStatus Start\r\n");

  // Validate input parameters
  if ((NULL == xpFotaName ||
      NULL == xpFotaError||
      fotaNameLen == 0))
  {
    C_KTA_APP__LOG("ERROR: Invalid input parameters\n");
    goto end;
  }

  retStatus = fotaGetComponentsStatus(xpFotaName, fotaNameLen, xpFotaError, components);

end:
  //C_KTA_APP__LOG("[INFO] fotaAgentGetStatus End, status [%d]\r\n", retStatus);
  return retStatus;
}

/**
 * @brief  implement fotaAgentGetDeviceInfo
 *
 */
TKFotaStatus fotaAgentGetDeviceInfo
(
  TComponent components[COMPONENTS_MAX]
)
{
  TKFotaStatus retStatus = E_K_FOTA_ERROR;

  //C_KTA_APP__LOG("[INFO] fotaAgentGetDeviceInfo Start\r\n");
  
  // Check for NULL parameters
  if (components == NULL) {
    C_KTA_APP__LOG("ERROR: Invalid input parameters\n");
    goto end;
  }

  // Call fotaGetInstalledComponents to get the components information
  retStatus = fotaGetInstalledComponents(components);
  if (retStatus != E_K_FOTA_SUCCESS)
  {
    C_KTA_APP__LOG("ERROR: Failed to get components information\n");
    goto end;
  }

end:
  //C_KTA_APP__LOG("[INFO] fotaAgentGetDeviceInfo End, status [%d]\r\n", retStatus);
  return retStatus;
}

/* -------------------------------------------------------------------------- */
/* END OF FILE                                                                */
/* -------------------------------------------------------------------------- */
