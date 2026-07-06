/*******************************************************************************
*************************keySTREAM Trusted Agent ("KTA")************************

* (c) 2023-2025 Nagravision Sàrl

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
 *  \author Kudelski Labs
 *
 *  \date 2025/02/04
 *
 *  \file fotaplatform.h
 ******************************************************************************/

/**
 * @brief Interface API for Fota platform specific implementations.
 */

#ifndef FOTA_PLATFORM_H
#define FOTA_PLATFORM_H
#include <stdbool.h>
/* -------------------------------------------------------------------------- */
/* IMPORTS                                                                    */
/* -------------------------------------------------------------------------- */
#include "k_sal_fota.h"

/* -------------------------------------------------------------------------- */
/* CONSTANTS, TYPES, ENUM                                                     */
/* -------------------------------------------------------------------------- */

/** @brief Global flag indicating FOTA download completion */

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
  const void *xpFotaMetadata,
  size_t xFotaMetadataLen,
  const TTargetComponent *xpComponents
);

/**
 * @brief Function pointer type for getting all installed component information.
 *
 * This callback retrieves information about all components currently installed
 * on the platform, including component names, versions, and other metadata.
 *
 * @param[out] components  Pointer to array of TComponent structures to be filled
 *                         with installed component information.
 *                         Should not be NULL.
 *
 * @return Number of components populated in the components array.
 *         Returns 0 if no components are installed or on error.
 */
typedef uint8_t (*get_all_components_info_fp)(TComponent* components);

/**
 * @brief Check if SmartEEPROM is enabled on SAMD51.
 *
 * Reads NVMCTRL_SEESTAT register to determine SmartEEPROM configuration.
 * SmartEEPROM provides persistent storage across power cycles.
 *
 * @return
 * - bool true if SmartEEPROM is enabled, false if disabled.
 */
bool checkSmartEEPROMEnabled
(
  void
);

/**
 * @brief Busy-wait for FOTA download completion (SmartEEPROM disabled mode).
 *
 * When SmartEEPROM is disabled, FOTA must complete in single session (no persistence).
 * This function blocks and processes WiFi/download tasks until completion or timeout.
 * Polls g_fota_download_complete and g_fota_download_success flags set by download module.
 *
 * @return
 * - E_K_FOTA_SUCCESS: Download completed successfully
 * - E_K_FOTA_ERROR: Download failed or timeout (15 minutes)
 */
TKFotaStatus fotaBusyWaitDownload
(
  void
);

#endif // FOTA_PLATFORM_H

/* -------------------------------------------------------------------------- */
/* END OF FILE                                                                */
/* -------------------------------------------------------------------------- */
