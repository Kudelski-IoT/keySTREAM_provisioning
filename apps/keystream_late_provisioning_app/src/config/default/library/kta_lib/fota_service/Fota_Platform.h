
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
/** \brief  Interface for Fota platform to be implemented by integrator as per 
            target platform.
 *
 *  \author Kudelski IoT
 *
 *  \date 2025/02/04
 *
 *  \file Fota_platform.h
 ******************************************************************************/

/**
 * @brief Interface API for Fota platform specific implementations.
 */

#ifndef FOTA_PLATFORM_H
#define FOTA_PLATFORM_H

/* -------------------------------------------------------------------------- */
/* IMPORTS                                                                    */
/* -------------------------------------------------------------------------- */
#include "k_sal_fota.h"

/* -------------------------------------------------------------------------- */
/* CONSTANTS, TYPES, ENUM                                                     */
/* -------------------------------------------------------------------------- */

/* -------------------------------------------------------------------------- */
/* FUNCTIONS                                                                  */
/* -------------------------------------------------------------------------- */

/**
 * @brief This API will get all component versions installed on platform.
 *
 *
 * @param[out] xpComponents
 *   Array of components.
 *   Should not be NULL.
 * 
 * @return
 * - E_K_FOTA_SUCCESS in case of success.
 * - E_K_FOTA_ERROR for other errors.
 */
 
TKFotaStatus fotaPlatformGetComponents
(
  TComponent *xpComponents
);


/**
 * @brief This API starts the installation of components.
 *        This API should return immediately by starting a fota thread function of target platform.
 *
 * Component array contains information like name, version and url.
 * This component information is to be stored by integrator based on platform capabilities.
 * Component data should be able to retrieved upon reboot of platform in case of tearing/power off,
 * so the it can install components after reboot.
 * Call FOTA thread function by passing Component information, then retunr from this function immediately.
 * FOTA Thread function responsibility is to download and install components.
 * Once component is installed, it should inform FOTA Agent by calling fotaUpdateComponent();
 * This process should be followed for each component.
 *
 * @param[out] xpComponents
 *   Array of target components.
 *   Should not be NULL.
 * 
 */
void fotaStartInstalltation
(
  const TTargetComponent *xpComponents
);

#endif // FOTA_PLATFORM_H

/* -------------------------------------------------------------------------- */
/* END OF FILE                                                                */
/* -------------------------------------------------------------------------- */
