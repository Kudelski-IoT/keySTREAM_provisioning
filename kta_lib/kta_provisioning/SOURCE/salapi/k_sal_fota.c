/*******************************************************************************
*************************keySTREAM Trusted Agent ("KTA")************************

* (c) 2023-2024 Nagravision SÃ rl

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
/** \brief  SAL Object for Mircrochip.
 *
 *  \author Kudelski IoT
 *
 *  \date 2023/06/12
 *
 *  \file k_sal_object.c
 ******************************************************************************/

/**
 * @brief SAL Object for Mircrochip.
 */

/* -------------------------------------------------------------------------- */
/* IMPORTS                                                                    */
/* -------------------------------------------------------------------------- */
#include <string.h>
#include <stdio.h>
#include "fotaagent.h"
#include "k_sal_fota.h"
#include "KTALog.h"



#ifndef K_SAL_API
#define K_SAL_API
#endif

/* -------------------------------------------------------------------------- */
/* LOCAL VARIABLES                                                            */
/* -------------------------------------------------------------------------- */
/**
 * SUPPRESS: MISRA_DEV_KTA_009 : misra_c2012_rule_5.9_violation
 * The identifier gpModuleName is intentionally defined as a common global for logging purposes
 */
#if LOG_KTA_ENABLE != C_KTA_LOG_LEVEL_NONE
static const char* gpModuleName = "SALFOTA";
#endif

/* -------------------------------------------------------------------------- */
/* PUBLIC FUNCTIONS - IMPLEMENTATION                                          */
/* -------------------------------------------------------------------------- */

K_SAL_API TKFotaStatus salFotaInstall
(
  const uint8_t          *xpFotaName,
  const size_t           xFotaNameLen,
  const uint8_t          *xpFotaMetadata,
  const size_t           xFotaMetadataLen,
  const TTargetComponent xTargetComponents[COMPONENTS_MAX],
  TFotaError           * xpFotaError,
  TComponent             xComponents[COMPONENTS_MAX]
)
{
  TKFotaStatus retStatus = E_K_FOTA_ERROR;

  M_KTALOG__START("Start");
  if ((NULL == xpFotaName) ||
      (NULL == xpFotaMetadata) ||
      (NULL == xTargetComponents))
  {
    M_KTALOG__ERR("FOTA installation Invalid input parameter");
    goto end;
  }


  retStatus = fotaAgentInstall(xpFotaName,
                               xFotaNameLen,
                               xpFotaMetadata,
                               xFotaMetadataLen,
                               xTargetComponents,
                               xpFotaError,
                               xComponents);
  // If FOTA installation is in progress, exit
  if (E_K_FOTA_IN_PROGRESS == retStatus)
  {
    M_KTALOG__INFO("FOTA installation in progress.");
    goto end;
  }

  // Fetch components if installation was successful
  if ((E_K_FOTA_SUCCESS == retStatus) &&
      (E_K_FOTA_SUCCESS != fotaAgentGetDeviceInfo(xComponents)))
  {
    M_KTALOG__ERR("Unable to fetch components from platform.");
  }

end:
  M_KTALOG__END("End, status : %d", retStatus);
  return retStatus;
}

/**
 * @brief   implement salFotaGetStatus
 *
 */
K_SAL_API TKFotaStatus salFotaGetStatus
(
  const uint8_t        *xpFotaName,
  size_t               xFotaNameLen,
  TFotaError         * xpFotaError,
  TComponent           xComponents[COMPONENTS_MAX]
)
{
  TKFotaStatus retStatus = E_K_FOTA_ERROR;

  M_KTALOG__START("Start");
  // Check for null or invalid parameters
  if ((NULL == xpFotaName) ||
      (0 == xFotaNameLen)  ||
      (NULL == xpFotaError))
  {
    M_KTALOG__ERR("Invalid parameters provided to salFotaGetStatus.");
   goto end;
  }

  // Call the fotaAgentGetStatus function
  retStatus = fotaAgentGetStatus(xpFotaName, xFotaNameLen, xpFotaError, xComponents);

  // If FOTA status is in progress, exit
  if (retStatus == E_K_FOTA_IN_PROGRESS)
  {
    M_KTALOG__INFO("FOTA installation in progress.");
    goto end;
  }

end:
  M_KTALOG__END("End, status : %d", retStatus);
  return retStatus;
}

/**
 * @brief   implement salDeviceGetInfo
 *
 */
K_SAL_API TKFotaStatus salDeviceGetInfo
(
  TComponent xComponents[COMPONENTS_MAX]
)
{
  TKFotaStatus retStatus = E_K_FOTA_ERROR;

  M_KTALOG__START("Start");
  // Check for null parameters
  if (NULL == xComponents)
  {
    M_KTALOG__ERR("Invalid parameters provided to salDeviceGetInfo.");
    goto end;
  }

  // Call the fotaAgentGetComponents function
  if (E_K_FOTA_SUCCESS != fotaAgentGetDeviceInfo(xComponents))
  {
    M_KTALOG__ERR("Unable to get components from platform.");
    goto end;
  }
  retStatus = E_K_FOTA_SUCCESS;

end:
  M_KTALOG__END("End, status : %d", retStatus);
  return retStatus;
}
