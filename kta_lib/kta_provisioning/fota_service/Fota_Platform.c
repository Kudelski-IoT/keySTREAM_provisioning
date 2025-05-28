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
/** \brief  Fota Installer for Mircrochip.
 *
 *  \author Kudelski IoT
 *
 *  \date 2025/01/22
 *
 *  \file Fota_Platform.c
 ******************************************************************************/

/**
 * @brief Fota Platform for Mircrochip.
 */

#include "Fota_Platform.h"

/* -------------------------------------------------------------------------- */
/* IMPORTS                                                                    */
/* -------------------------------------------------------------------------- */

/* -------------------------------------------------------------------------- */
/* PUBLIC FUNCTIONS - IMPLEMENTATION                                          */
/* -------------------------------------------------------------------------- */

/**
 * @brief  implement fotaPlatformGetComponents
 *
 */
TKFotaStatus fotaPlatformGetComponents
(
  TComponent *xpComponents
)
{
  (void)xpComponents;
  // Get each installed component version from platform.
  // Update xpComponents structure with component names and versions
  return E_K_FOTA_SUCCESS;
}

/**
 * @brief  implement fotaPlatformGetComponents
 *
 */
void fotaStartInstalltation
(
  const TTargetComponent *xpComponents
)
{
  (void)xpComponents;
  // xpComponents contains components name, version and URL information.
  // Call FOTA thread function by passing this information, and return immediately.
  // FOTA Thread function is responsibility to download and install components
  // Once component installation is completed, it should inform FOTA Agent by calling
  // fotaUpdateComponent();
  return;
}

/* -------------------------------------------------------------------------- */
/* END OF FILE                                                                */
/* -------------------------------------------------------------------------- */
