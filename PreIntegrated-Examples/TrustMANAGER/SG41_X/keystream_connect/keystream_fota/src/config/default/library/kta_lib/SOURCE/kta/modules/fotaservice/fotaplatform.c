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
/** \brief  FOTA Platform for Microchip.
 *
 *  \author Kudelski Labs
 *
 *  \date 2025/01/22
 *
 *  \file fotaplatform.c
 ******************************************************************************/

/**
 * @brief FOTA Platform for Microchip.
 */
/* -------------------------------------------------------------------------- */
/* IMPORTS                                                                    */
/* -------------------------------------------------------------------------- */
#include "fotaplatform.h"
#include "k_sal_fota.h"
#include "KTALog.h"

#include "../../../../../app_wincs02.h"
#include "../../../../../metadata.h"
#include "../../../../../app_dldr.h"  // For busy-wait flags
#include "../../../../../definitions.h"  // For sysObj and WDRV_WINC_Tasks

/* -------------------------------------------------------------------------- */
/* LOCAL VARIABLES                                                            */
/* -------------------------------------------------------------------------- */
/**
 * SUPPRESS: MISRA_DEV_KTA_009 : misra_c2012_rule_5.9_violation
 * The identifier gpModuleName is intentionally defined as a common global for logging purposes
 */
#if LOG_KTA_ENABLE != C_KTA_LOG_LEVEL_NONE
static const char* gpModuleName = "FOTAPLATFORM";
#endif

/* -------------------------------------------------------------------------- */
/* GLOBAL STATE                                                               */
/* -------------------------------------------------------------------------- */


/* -------------------------------------------------------------------------- */
/* GLOBAL VARIABLES                                                          */
/* -------------------------------------------------------------------------- */
/** @brief Global buffer for FOTA image version string (null-terminated). */
static char gaFotaImageVersion[32] = {0};

/** @brief Global buffer for FOTA image URL string (null-terminated). */
extern char gaFotaImageUrl[2512] = {0};

/** @brief Global buffer for FOTA image name string (null-terminated). */
static char gaFotaImageName[64] = {0};

/** @brief Current FOTA component status. */
extern FOTA_Status gCurrentFOTAStatus;

/* -------------------------------------------------------------------------- */
/* LOCAL CONSTANTS, TYPES, ENUM                                               */
/* -------------------------------------------------------------------------- */

/** @brief Maximum timeout iterations for download busy-wait (15 minutes at ~1ms per iteration). */
#define C_MAX_TIMEOUT_ITERATIONS  (900000U)

/** @brief Progress log interval in iterations (30 seconds at ~1ms per iteration). */
#define C_LOG_INTERVAL_ITERATIONS (30000U)

/** @brief Delay loop iterations for approximately 1ms delay. */
#define C_DELAY_LOOP_ITERATIONS   (1000U)

/* -------------------------------------------------------------------------- */
/* PUBLIC FUNCTIONS - IMPLEMENTATION                                          */
/* -------------------------------------------------------------------------- */


/**
 * @brief
 *   Get FOTA component information from platform.
 *
 * @param[in,out] xpComponents
 *   [in] Pointer to component structure buffer.
 *   [out] Filled component structure with platform information.
 *
 * @return
 * - E_K_FOTA_SUCCESS on success.
 * - E_K_FOTA_ERROR on error.
 */
TKFotaStatus fotaPlatformGetComponents
(
  TComponent*  xpComponents
)
{
  TKFotaStatus status = E_K_FOTA_ERROR;

  M_KTALOG__START("Start");

  /* Validate input */
  if (xpComponents == NULL)
  {
    M_KTALOG__ERR("[ERROR] xpComponents is NULL\r\n");
    goto end;
  }

  M_KTALOG__INFO("[INFO] Reading component information from metadata...\r\n");

  /* Get all components at once using callback */
  get_all_components_info_fp fpGetAllComponents = get_all_components_information;
  uint8_t numComponents = fpGetAllComponents(xpComponents);
  
  if (numComponents > 0)
  {
    M_KTALOG__INFO("[PLATFORM] Retrieved %u component(s) using callback\r\n", numComponents);
    // Debug print for each component
    for (uint8_t i = 0; i < numComponents; i++)
    {
      M_KTALOG__INFO("[PLATFORM] Component[%u] name: %.*s\r\n", 
        i, 
        (int)xpComponents[i].componentNameLen, 
        xpComponents[i].componentName);
      M_KTALOG__INFO("[PLATFORM] Component[%u] version: %.*s\r\n", 
        i, 
        (int)xpComponents[i].componentVersionLen, 
        xpComponents[i].componentVersion);
    }
    
    status = E_K_FOTA_SUCCESS;
  }
  else
  {
    M_KTALOG__ERR("[ERROR] Failed to retrieve components from callback\r\n");
    status = E_K_FOTA_ERROR;
  }

end:
  return status;
}

/**
 * @brief
 *   Start FOTA installation process.
 *
 * @param[in] xpFotaMetadata
 *   Pointer to FOTA metadata buffer.
 * @param[in] xFotaMetadataLen
 *   Length of FOTA metadata in bytes.
 * @param[in] xpComponents
 *   Pointer to target component information.
 *
 * @return
 *   None.
 */
void fotaStartInstalltation
(
  const void*              xpFotaMetadata,
  size_t                   xFotaMetadataLen,
  const TTargetComponent*  xpComponents
)
{
  // Validate inputs
  if (xpFotaMetadata == NULL)
  {
    M_KTALOG__ERR("[ERROR] xpFotaMetadata is NULL, aborting installation.\r\n");
    return;
  }

  if (xFotaMetadataLen == 0)
  {
    M_KTALOG__ERR("[ERROR] xFotaMetadataLen is 0, aborting installation.\r\n");
    return;
  }

  if (xpComponents == NULL)
  {
    M_KTALOG__ERR("[ERROR] xpComponents is NULL, aborting installation.\r\n");
    return;
  }

  // The metadata provides details about the firmware
  // version, size, and other relevant information needed for update management.
  //  This can be used and stored by the application using its ownstorage mechanism
  // (e.g., flash memory, EEPROM, or file system) to persist
  // firmware update information across power cycles or update sessions.

  // Store FOTA image information (name, version, URL) using application-specific storage mechanism
  // This will help applications to track and manage firmware updates effectively during power cycles.

  // Prepare component data (bounds already validated)
  size_t nameLen = (xpComponents->componentTargetNameLen < sizeof(gaFotaImageName)) ? 
                   xpComponents->componentTargetNameLen : sizeof(gaFotaImageName) - 1u;
  size_t versionLen = (xpComponents->componentTargetVersionLen < sizeof(gaFotaImageVersion)) ? 
                      xpComponents->componentTargetVersionLen : sizeof(gaFotaImageVersion) - 1u;
  // Use source buffer size (512) not destination size (2512) to prevent buffer over-read
  size_t urlLen = (xpComponents->componentUrlLen < 512u) ? 
                  xpComponents->componentUrlLen : 511u;

  (void)memcpy(gaFotaImageName, xpComponents->componentTargetName, nameLen);
  gaFotaImageName[nameLen] = '\0';
  
  (void)memcpy(gaFotaImageVersion, xpComponents->componentTargetVersion, versionLen);
  gaFotaImageVersion[versionLen] = '\0';
  
  (void)memcpy(gaFotaImageUrl, xpComponents->componentUrl, urlLen);
  gaFotaImageUrl[urlLen] = '\0';

  printf(TERM_CYAN "\r\n\n--------------------------------------------------------\r\n");
  printf("Update Component Name   : %s\n\r", gaFotaImageName);
  printf("Update Component Version: %s\n\r", gaFotaImageVersion);
  printf("Update Component URL    : %s\n\r", gaFotaImageUrl);
  printf("Update Component URL size: %u\n\r", (unsigned int)urlLen);
  printf("--------------------------------------------------------\r\n\n" TERM_RESET);

  gCurrentFOTAStatus = APP_FOTA_START;
  M_KTALOG__INFO("[INFO] FOTA installation initiated successfully.\r\n");

  // ***** PLATFORM-SPECIFIC: Check SmartEEPROM and handle accordingly *****
  bool smartEEPROMEnabled = checkSmartEEPROMEnabled();
  
  if (!smartEEPROMEnabled)
  {
    // SmartEEPROM disabled - Use busy-wait to block until download completes
    // This is necessary because without persistent storage, FOTA must complete in single session
    M_KTALOG__INFO("[PLATFORM] SmartEEPROM disabled - Starting synchronous download with busy-wait\r\n");
    TKFotaStatus downloadResult = fotaBusyWaitDownload();
    
    if (E_K_FOTA_SUCCESS == downloadResult)
    {
      M_KTALOG__INFO("[PLATFORM] Download completed successfully\r\n");
    }
    else
    {
      M_KTALOG__ERR("[PLATFORM] Download failed or timed out\r\n");
    }
  }
  else
  {
    // SmartEEPROM enabled - Return immediately for async operation
    // Download will proceed in background, state persisted across power cycles
    M_KTALOG__INFO("[PLATFORM] SmartEEPROM enabled - Starting asynchronous download (non-blocking)\r\n");
  }
  
  return;
}
/**
 * @brief Check if SmartEEPROM is enabled.
 *
 * @implements checkSmartEEPROMEnabled
 *
 * @return
 * - true if SmartEEPROM is enabled (both SBLK and PSZ fuses are non-zero).
 * - false if SmartEEPROM is disabled.
 */
bool checkSmartEEPROMEnabled
(
  void
)
{
  // Read fuse configuration from USER_PAGE (same as boot banner)
  uint32_t NVMCTRL_SEESBLK_FuseConfig = ((*(uint32_t *)(USER_PAGE_ADDR + 4)) >> 0) & 0xF;
  uint32_t NVMCTRL_SEEPSZ_FuseConfig = ((*(uint32_t *)(USER_PAGE_ADDR + 4)) >> 4) & 0x7;
  
  // SmartEEPROM is enabled if BOTH fields are non-zero (configured in fuses)
  bool enabled = (NVMCTRL_SEESBLK_FuseConfig != 0 && NVMCTRL_SEEPSZ_FuseConfig != 0);
  
  M_KTALOG__INFO("[checkSmartEEPROMEnabled] USER_PAGE fuses: %s (SBLK=%u, PSZ=%u)\r\n", 
                 enabled ? "ENABLED" : "DISABLED",
                 (unsigned int)NVMCTRL_SEESBLK_FuseConfig, (unsigned int)NVMCTRL_SEEPSZ_FuseConfig);
  
  return enabled;
}

/**
 * @brief Busy-wait for FOTA download completion (SmartEEPROM disabled mode).
 *
 * @implements fotaBusyWaitDownload
 *
 * @return
 * - E_K_FOTA_SUCCESS if download completed successfully.
 * - E_K_FOTA_ERROR if download failed or timed out.
 */
TKFotaStatus fotaBusyWaitDownload
(
  void
)
{
  TKFotaStatus retStatus = E_K_FOTA_ERROR;
  
  M_KTALOG__INFO("SmartEEPROM disabled - Starting busy-wait for download completion\r\n");
  
  // Reset completion flags
  g_fota_download_complete = false;
  g_fota_update_complete = false;
  
  // Busy-wait loop with extended timeout (15 minutes for large files)
  uint32_t lTimeoutCounter = 0U;
  uint32_t lLastLogCounter = 0U;
  
  while (!g_fota_download_complete && (lTimeoutCounter < C_MAX_TIMEOUT_ITERATIONS))
  {
    // CRITICAL: Process WiFi driver or download freezes
    WDRV_WINC_Tasks(sysObj.drvWifiWinc);
    
    // CRITICAL: Process download state machine
    APP_DLDR_Tasks();
    
    // CRITICAL: Process other tasks to keep system responsive
    APP_WINCS02_Tasks();
    
    // Note: Watchdog is not enabled in this system configuration
    // If watchdog is enabled in your system, add: WDT_REGS->WDT_CLEAR = WDT_CLEAR_CLEAR_KEY;
    
    // Progress logging every 30 seconds
    if ((lTimeoutCounter - lLastLogCounter) >= C_LOG_INTERVAL_ITERATIONS)
    {
      M_KTALOG__INFO("Busy-wait progress: %u seconds elapsed\r\n", (unsigned int)(lTimeoutCounter / 1000U));
      lLastLogCounter = lTimeoutCounter;
    }
    
    // Small delay to prevent CPU 100% usage (approximately 1ms)
    for (volatile uint32_t i = 0U; i < C_DELAY_LOOP_ITERATIONS; i++);
    
    lTimeoutCounter++;
  }
  
  // Check results after busy-wait
  if (lTimeoutCounter >= C_MAX_TIMEOUT_ITERATIONS)
  {
    M_KTALOG__ERR("[PLATFORM] Download timeout after %u seconds, status = [E_K_FOTA_ERROR]\r\n", 
                  (unsigned int)(lTimeoutCounter / 1000U));
    retStatus = E_K_FOTA_ERROR;
  }
  else if (g_fota_download_complete)
  {
    M_KTALOG__INFO("[PLATFORM] Download completed successfully in %u seconds\r\n", 
                   (unsigned int)(lTimeoutCounter / 1000U));
    retStatus = E_K_FOTA_SUCCESS;
  }
  else
  {
    M_KTALOG__ERR("[PLATFORM] Download failed after %u seconds, status = [E_K_FOTA_ERROR]\r\n", 
                  (unsigned int)(lTimeoutCounter / 1000U));
    retStatus = E_K_FOTA_ERROR;
  }
  
  return retStatus;
}

/* -------------------------------------------------------------------------- */
/* END OF FILE                                                                */
/* -------------------------------------------------------------------------- */
