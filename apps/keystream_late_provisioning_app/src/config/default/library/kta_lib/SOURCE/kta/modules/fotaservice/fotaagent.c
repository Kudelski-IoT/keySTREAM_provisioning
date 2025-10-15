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
/** \brief  Fota Agent.
 *
 *  \author Kudelski IoT
 *
 *  \date 2025/01/22
 *
 *  \file fotaagent.c
 ******************************************************************************/

/**
 * @brief Fota Agent.
 */

#include "fotaagent.h"
/* -------------------------------------------------------------------------- */
/* IMPORTS                                                                    */
/* -------------------------------------------------------------------------- */

#include "k_sal_fota.h"
#include "fotaprocess.h"
#include "KTALog.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>


/* -------------------------------------------------------------------------- */
/* LOCAL VARIABLES                                                            */
/* -------------------------------------------------------------------------- */
/**
 * SUPPRESS: MISRA_DEV_KTA_009 : misra_c2012_rule_5.9_violation
 * The identifier gpModuleName is intentionally defined as a common global for logging purposes
 */
#if LOG_KTA_ENABLE != C_KTA_LOG_LEVEL_NONE
static const char* gpModuleName = "FOTAAGENT";
#endif

/* -------------------------------------------------------------------------- */
/* GLOBAL STATE                                                               */
/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */
/* PUBLIC FUNCTIONS - IMPLEMENTATION                                          */
/* -------------------------------------------------------------------------- */

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
  M_KTALOG__START("[INFO] fotaAgentInstall Start\r\n");

  if ((NULL == xpFotaName) ||
      (NULL == xpFotaMetadata) ||
      (NULL == xTargetComponents))
  {
    M_KTALOG__ERR("ERROR: Invalid input parameter\r\n");
    retStatus = E_K_FOTA_ERROR;
    goto end;
  }

  retStatus = validateComponents(xTargetComponents, xpFotaError, xComponents);
  if ((E_K_FOTA_SUCCESS != retStatus) &&
      (E_K_FOTA_IN_PROGRESS != retStatus))
  {
    M_KTALOG__ERR("ERROR: validateComponents Failed\r\n");
    goto end;
  }

  retStatus = storeMetadata(xpFotaName,
                            xFotaNameLen,
                            xpFotaMetadata,
                            xFotaMetadataLen,
                            xpFotaError);
  if (E_K_FOTA_SUCCESS != retStatus)
  {
    M_KTALOG__ERR("ERROR: storeMetadata Failed\r\n");
    goto end;
  }

  retStatus = fotaDownloadAndInstall(xpFotaName, xFotaNameLen, xpFotaMetadata, xFotaMetadataLen, xTargetComponents, xComponents);
  if ((E_K_FOTA_SUCCESS != retStatus) &&
      (E_K_FOTA_IN_PROGRESS != retStatus))
  {
    M_KTALOG__ERR("ERROR: fotaDownloadAndInstall Failed\r\n");
    retStatus = E_K_FOTA_ERROR;
  }

end:
  M_KTALOG__END("[INFO] fotaAgentInstall End, status [%d]\r\n", retStatus);
  return retStatus;
}

/**
 * @brief  implement fotaAgentGetStatus
 *
 */
TKFotaStatus fotaAgentGetStatus
(
  const uint8_t *xpFotaName,
  size_t         xFotaNameLen,
  TFotaError    *xpFotaError,
  TComponent     xComponents[COMPONENTS_MAX]
)
{
  TKFotaStatus retStatus = E_K_FOTA_ERROR;
  M_KTALOG__START("[INFO] fotaAgentGetStatus Start\r\n");

  // Validate input parameters
  if ((NULL == xpFotaName) ||
      (NULL == xpFotaError) ||
      (0 == xFotaNameLen))
  {
    M_KTALOG__ERR("ERROR: Invalid input parameters\n");
    goto end;
  }

  retStatus = fotaGetComponentsStatus(xpFotaName, xFotaNameLen, xpFotaError, xComponents);

end:
  M_KTALOG__END("[INFO] fotaAgentGetStatus End, status [%d]\r\n", retStatus);
  return retStatus;
}

/**
 * @brief  implement fotaAgentGetDeviceInfo
 *
 */
TKFotaStatus fotaAgentGetDeviceInfo
(
  TComponent xComponents[COMPONENTS_MAX]
)
{
  TKFotaStatus retStatus = E_K_FOTA_ERROR;

  M_KTALOG__START("[INFO] fotaAgentGetDeviceInfo Start\r\n");

  // Check for NULL parameters
  if (NULL == xComponents) {
    M_KTALOG__ERR("ERROR: Invalid input parameters\n");
    goto end;
  }

  // Call fotaGetInstalledComponents to get the components information
  retStatus = fotaGetInstalledComponents(xComponents);
  if (E_K_FOTA_SUCCESS != retStatus)
  {
    M_KTALOG__ERR("ERROR: Failed to get components information\n");
    goto end;
  }

end:
  M_KTALOG__END("[INFO] fotaAgentGetDeviceInfo End, status [%d]\r\n", retStatus);
  return retStatus;
}

/* -------------------------------------------------------------------------- */
/* END OF FILE                                                                */
/* -------------------------------------------------------------------------- */
