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
/** \brief keySTREAM Trusted Agent - Device configuration module.
 *
 *  \author Kudelski IoT
 *
 *  \date 2023/06/13
 *
 *  \file config.c
 ******************************************************************************/

/**
 * @brief keySTREAM Trusted Agent - Device configuration module.
 */
#include "config.h"
/* -------------------------------------------------------------------------- */
/* IMPORTS                                                                    */
/* -------------------------------------------------------------------------- */
#include "k_sal_storage.h"
#include "k_kta.h"
#include "kta_version.h"
#include "KTALog.h"
#include "k_sal.h"

#include <string.h>
#include <stdint.h>

/* -------------------------------------------------------------------------- */
/* LOCAL CONSTANTS, TYPES, ENUM                                               */
/* -------------------------------------------------------------------------- */

/* -------------------------------------------------------------------------- */
/* LOCAL VARIABLES                                                            */
/* -------------------------------------------------------------------------- */
/**
 * SUPPRESS: MISRA_DEV_KTA_009 : misra_c2012_rule_5.9_violation
 * The identifier gpModuleName is intentionally defined as a common global for logging purposes
 */
/* Module name used for logging. */
#if LOG_KTA_ENABLE != C_KTA_LOG_LEVEL_NONE
static const char* gpModuleName = "KTACONFIG";
#endif
/* Static variable to hold configuration info. */
static TKtaDeviceInfoConfig gKtaDeviceInfoConfig = {0};
static TKtaContextInfoConfig gKtaContextInfoConfig = {0};

/* -------------------------------------------------------------------------- */
/* LOCAL FUNCTIONS - PROTOTYPE                                                */
/* -------------------------------------------------------------------------- */
/**
 * @brief
 *   Load keySTREAM Trusted Agent information data from NVM.
 *
 * @param[in] xState
 *   keySTREAM Trusted Agent Life cycle state.
 *
 * @return
 * - E_K_STATUS_OK in case of success.
 * - E_K_STATUS_PARAMETER for wrong input parameter.
 * - E_K_STATUS_ERROR for other errors.
 */
static TKStatus lReadDeviceAndContextInfo
(
  const TKtaLifeCycleState xState
);

/* -------------------------------------------------------------------------- */
/* PUBLIC VARIABLES                                                           */
/* -------------------------------------------------------------------------- */

/* -------------------------------------------------------------------------- */
/* PUBLIC FUNCTIONS - IMPLEMENTATION                                          */
/* -------------------------------------------------------------------------- */

/**
 * @brief implement ktaSetDeviceInfoConfig
 *
 */
/**
 * SUPPRESS: MISRA_DEV_KTA_005 : misra_c2012_rule_15.4_violation
 * SUPPRESS: MISRA_DEV_KTA_004 : misra_c2012_rule_15.1_violation
 * Using goto for breaking during the error and return cases.
 **/
TKStatus ktaSetDeviceInfoConfig
(
  const uint8_t* xpDeviceProfilePublicUid,
  size_t         xDeviceProfilePublicUidSize,
  const uint8_t* xpDeviceSerialNum,
  size_t         xDeviceSerialNumSize,
  const TKtaLifeCycleState xState
)
{
  TKStatus status = E_K_STATUS_ERROR;
  uint8_t  aKtaInfo[C_K_KTA__SEALED_INFORMATION_MAX_SIZE] = {0};
  uint8_t  ktaInfoLen = 0;
  uint8_t  maxDevProfSize = 0;

  M_KTALOG__START("Start");

  // REQ RQ_M-KTA-STRT-CF-0100(1) : Device Profile Public UID Size
  // REQ RQ_M-KTA-STRT-CF-0110(1) : Device Serial Number Size
  if ((NULL == xpDeviceProfilePublicUid) ||
      (C_K__DEVICE_PROFILE_PUBLIC_UID_MAX_SIZE < xDeviceProfilePublicUidSize) ||
      (0u == xDeviceProfilePublicUidSize) ||
      (NULL == xpDeviceSerialNum) ||
      (0u == xDeviceSerialNumSize) ||
      (C_K__DEVICE_SERIAL_NUM_MAX_SIZE < xDeviceSerialNumSize))
  {
    status = E_K_STATUS_PARAMETER;
    M_KTALOG__ERR("Invalid parameter passed");
  }
  else
  {
    if (xState == E_LIFE_CYCLE_STATE_INIT)
    {
      M_KTALOG__DEBUG("Device is in init state, keeping KTA info into NVM...");
      (void)memcpy(&(aKtaInfo[ktaInfoLen]),
                   gKtaContextInfoConfig.ktaContextProfileUid,
                   C_K__CONTEXT_PROFILE_UID_MAX_SIZE);
      ktaInfoLen = C_K__CONTEXT_PROFILE_UID_MAX_SIZE;
      aKtaInfo[ktaInfoLen] = gKtaContextInfoConfig.ktaContextProfileUidLength;
      ktaInfoLen += (uint8_t)1;

      (void)memcpy(&(aKtaInfo[ktaInfoLen]),
                   gKtaContextInfoConfig.ktaContexSerialNumber,
                   C_K__CONTEXT_SERIAL_NUMBER_MAX_SIZE);
      ktaInfoLen += C_K__CONTEXT_SERIAL_NUMBER_MAX_SIZE;
      aKtaInfo[ktaInfoLen] = gKtaContextInfoConfig.ktaContexSerialNumberLength;
      ktaInfoLen += (uint8_t)1;

      (void)memcpy(&(aKtaInfo[ktaInfoLen]),
                   gKtaContextInfoConfig.ktaContextVersion,
                   C_K__CONTEXT_VERSION_MAX_SIZE);
      ktaInfoLen += C_K__CONTEXT_VERSION_MAX_SIZE;
      aKtaInfo[ktaInfoLen] = gKtaContextInfoConfig.ktaContextVersionLength;
      ktaInfoLen += (uint8_t)1;

      (void)memcpy(&(aKtaInfo[ktaInfoLen]), ktaGetVersion(), C_KTA__VERSION_MAX_SIZE);
      ktaInfoLen += C_KTA__VERSION_MAX_SIZE;
      // REQ RQ_M-KTA-ACTV-FN-0005_06(1) : Device Profile Public UID
      (void)memcpy(&(aKtaInfo[ktaInfoLen]), xpDeviceProfilePublicUid, xDeviceProfilePublicUidSize);
      ktaInfoLen += C_K__DEVICE_PROFILE_PUBLIC_UID_MAX_SIZE;
      aKtaInfo[ktaInfoLen] = (uint8_t)xDeviceProfilePublicUidSize;
      ktaInfoLen += (uint8_t)1;

      (void)memcpy(&(aKtaInfo[ktaInfoLen]), xpDeviceSerialNum, xDeviceSerialNumSize);
      ktaInfoLen += C_K__DEVICE_SERIAL_NUM_MAX_SIZE;
      aKtaInfo[ktaInfoLen] = (uint8_t)xDeviceSerialNumSize;
      ktaInfoLen += (uint8_t)1;

      status = salStorageSetAndLockValue(C_K_KTA__SEALED_DATA_STORAGE_ID, aKtaInfo, ktaInfoLen);

      if (status != E_K_STATUS_OK)
      {
        M_KTALOG__ERR("salStorageSetAndLockValue failed[%d]", status);
        goto end;
      }

      (void)memcpy(gKtaDeviceInfoConfig.deviceProfilePubUID[0],
                   xpDeviceProfilePublicUid,
                   xDeviceProfilePublicUidSize);
      gKtaDeviceInfoConfig.deviceProfilePubUIDLength[0] = (uint8_t)xDeviceProfilePublicUidSize;

      (void)memcpy(gKtaDeviceInfoConfig.deviceSerailNo,
                   xpDeviceSerialNum,
                   xDeviceSerialNumSize);
      gKtaDeviceInfoConfig.deviceSerailNoLength = (uint8_t)xDeviceSerialNumSize;
    }
    else if (xState > E_LIFE_CYCLE_STATE_INIT)
    {
      M_KTALOG__DEBUG("Device not in init state, reading KTA info from NVM...");
      status = lReadDeviceAndContextInfo(xState);

      if (status != E_K_STATUS_OK)
      {
        M_KTALOG__ERR("lReadDeviceAndContextInfo failed[%d]", status);
      }

      /* Find max size of devProfileUID to compare. */
      maxDevProfSize = gKtaDeviceInfoConfig.deviceProfilePubUIDLength[0];
      if (gKtaDeviceInfoConfig.deviceProfilePubUIDLength[1] > maxDevProfSize)
      {
        maxDevProfSize = gKtaDeviceInfoConfig.deviceProfilePubUIDLength[1];
      }

      if (0 != memcmp(xpDeviceProfilePublicUid, gKtaDeviceInfoConfig.deviceProfilePubUID[0],
                        maxDevProfSize))
      {
        /* Fill the mutable device profile info. */
        (void)memcpy(gKtaDeviceInfoConfig.deviceProfilePubUID[1],
                    xpDeviceProfilePublicUid,
                    xDeviceProfilePublicUidSize);

        gKtaDeviceInfoConfig.deviceProfilePubUIDLength[1] = (uint8_t)xDeviceProfilePublicUidSize;
      }
    }
    else
    {
      M_KTALOG__ERR("Invalid state[%d]", xState);
    }
  }

end:
  M_KTALOG__END("End, status : %d", status);
  return status;
}

/**
 * @brief implement ktaGetDeviceInfoConfig
 *
 */
TKStatus ktaGetDeviceInfoConfig
(
  TKtaDeviceInfoConfig* xpDeviceInfoConfig
)
{
  TKStatus status = E_K_STATUS_ERROR;

  M_KTALOG__START("Start");

  if (NULL == xpDeviceInfoConfig)
  {
    M_KTALOG__ERR("Invalid parameter passed");
    status = E_K_STATUS_PARAMETER;
  }
  else
  {
    /* Fill device specific info. */
    (void)memcpy(xpDeviceInfoConfig->deviceProfilePubUID[0],
                 gKtaDeviceInfoConfig.deviceProfilePubUID[0],
                 gKtaDeviceInfoConfig.deviceProfilePubUIDLength[0]);
    xpDeviceInfoConfig->deviceProfilePubUIDLength[0] =
        gKtaDeviceInfoConfig.deviceProfilePubUIDLength[0];

    if (gKtaDeviceInfoConfig.deviceProfilePubUIDLength[1] != 0u)
    {
      (void)memcpy(xpDeviceInfoConfig->deviceProfilePubUID[1],
                   gKtaDeviceInfoConfig.deviceProfilePubUID[1],
                   gKtaDeviceInfoConfig.deviceProfilePubUIDLength[1]);
      xpDeviceInfoConfig->deviceProfilePubUIDLength[1] =
          gKtaDeviceInfoConfig.deviceProfilePubUIDLength[1];
    }

    (void)memcpy(xpDeviceInfoConfig->deviceSerailNo,
                 gKtaDeviceInfoConfig.deviceSerailNo,
                 gKtaDeviceInfoConfig.deviceSerailNoLength);
    xpDeviceInfoConfig->deviceSerailNoLength = gKtaDeviceInfoConfig.deviceSerailNoLength;
    status = E_K_STATUS_OK;
  }

  M_KTALOG__END("End, status : %d", status);
  return status;
}

/**
 * @brief implement ktaSetContextInfoConfig
 *
 */
/**
 * SUPPRESS: MISRA_DEV_KTA_002 : misra_c2012_rule_17.7_violation
 * return value of non-void function not used
 */
TKStatus ktaSetContextInfoConfig
(
  const uint8_t* xpL1SegSeed,
  const uint8_t* xpKtaContextProfileUid,
  size_t         xKtaContextProfileUidLen,
  const uint8_t* xpKtaContexSerialNumber,
  size_t         xKtaContexSerialNumberLen,
  const uint8_t* xpKtaContextVersion,
  size_t         xKtaContextVersionLen,
  const TKtaLifeCycleState  xState
)
{
  TKStatus status = E_K_STATUS_ERROR;

  M_KTALOG__START("Start");

  /* Public function arguments validity checked by the calling function. */

  if (xState == E_LIFE_CYCLE_STATE_INIT)
  {
    M_KTALOG__DEBUG("Setting kta context configuration data to the platform");
    (void)memset(gKtaContextInfoConfig.ktaContextProfileUid,
                  0x00,
                  C_K__CONTEXT_PROFILE_UID_MAX_SIZE);
    (void)memcpy(gKtaContextInfoConfig.ktaContextProfileUid, xpKtaContextProfileUid,
                  xKtaContextProfileUidLen);
    gKtaContextInfoConfig.ktaContextProfileUidLength = (uint8_t)xKtaContextProfileUidLen;

    (void)memset(gKtaContextInfoConfig.ktaContexSerialNumber,
                  0x00,
                  C_K__CONTEXT_SERIAL_NUMBER_MAX_SIZE);
    (void)memcpy(gKtaContextInfoConfig.ktaContexSerialNumber, xpKtaContexSerialNumber,
                  xKtaContexSerialNumberLen);
    gKtaContextInfoConfig.ktaContexSerialNumberLength = (uint8_t)xKtaContexSerialNumberLen;

    (void)memset(gKtaContextInfoConfig.ktaContextVersion,
                  0x00,
                  C_K__CONTEXT_VERSION_MAX_SIZE);
    (void)memcpy(gKtaContextInfoConfig.ktaContextVersion,
                  xpKtaContextVersion,
                  xKtaContextVersionLen);
    gKtaContextInfoConfig.ktaContextVersionLength = (uint8_t)xKtaContextVersionLen;

    /* Loading kta version. */
    (void)memset(gKtaContextInfoConfig.ktaVersion, 0, C_KTA__VERSION_MAX_SIZE);
    (void)memcpy(gKtaContextInfoConfig.ktaVersion,
                  ktaGetVersion(),
                  strnlen((const char*)ktaGetVersion(), C_KTA__VERSION_MAX_SIZE));

    (void)memcpy(gKtaContextInfoConfig.l1SegSeed, xpL1SegSeed, C_K__L1_SEGMENTATION_SEED_SIZE);
    gKtaContextInfoConfig.rotKeySetId = 0;
    status = E_K_STATUS_OK;
  }
  else
  {
    status = lReadDeviceAndContextInfo(xState);
    /* memcpy done independtly of returned status as caller of ktaSetContextInfoConfig
    handles the error scenario. */
    if (xState == E_LIFE_CYCLE_STATE_SEALED)
    {
      (void)memcpy(gKtaContextInfoConfig.l1SegSeed,
                    xpL1SegSeed, C_K__L1_SEGMENTATION_SEED_SIZE);
    }
  }

  M_KTALOG__END("End, status : %d", status);
  return status;
}

/**
 * @brief implement ktaGetContextInfoConfig
 *
 */
TKStatus ktaGetContextInfoConfig
(
  TKtaContextInfoConfig* xpContextInfoConfig
)
{
  TKStatus status = E_K_STATUS_ERROR;

  M_KTALOG__START("Start");

  if (NULL == xpContextInfoConfig)
  {
    status = E_K_STATUS_PARAMETER;
    M_KTALOG__ERR("Invalid parameter passed");
  }
  else
  {
    /* Fill context specific platform data. */
    M_KTALOG__DEBUG("Getting kta context specific data");
    (void)memcpy(xpContextInfoConfig->l1SegSeed,
                 gKtaContextInfoConfig.l1SegSeed,
                 sizeof(gKtaContextInfoConfig.l1SegSeed));

    (void)memcpy(xpContextInfoConfig->ktaContextProfileUid,
                 gKtaContextInfoConfig.ktaContextProfileUid,
                 sizeof(gKtaContextInfoConfig.ktaContextProfileUid));

    (void)memcpy(xpContextInfoConfig->ktaContexSerialNumber,
                 gKtaContextInfoConfig.ktaContexSerialNumber,
                 sizeof(gKtaContextInfoConfig.ktaContexSerialNumber));

    (void)memcpy(xpContextInfoConfig->ktaContextVersion,
                 gKtaContextInfoConfig.ktaContextVersion,
                 sizeof(gKtaContextInfoConfig.ktaContextVersion));

    (void)memcpy(xpContextInfoConfig->ktaVersion,
                 gKtaContextInfoConfig.ktaVersion,
                 sizeof(gKtaContextInfoConfig.ktaVersion));

    xpContextInfoConfig->rotKeySetId = gKtaContextInfoConfig.rotKeySetId;

    status = E_K_STATUS_OK;
  }

  M_KTALOG__END("End, status : %d", status);
  return status;
}

/**
 * @brief implement ktaGetL1SegSeed
 *
 */
TKStatus ktaGetL1SegSeed
(
  uint8_t* xpL1SegSeed
)
{
  TKStatus status = E_K_STATUS_ERROR;

  M_KTALOG__START("Start");

  if (NULL == xpL1SegSeed)
  {
    status = E_K_STATUS_PARAMETER;
    M_KTALOG__ERR("Invalid parameter passed");
  }
  else
  {
    (void)memcpy(xpL1SegSeed, gKtaContextInfoConfig.l1SegSeed, C_K__L1_SEGMENTATION_SEED_SIZE);
    status = E_K_STATUS_OK;
  }

  M_KTALOG__END("End, status : %d", status);
  return status;
}

/**
 * @brief implement ktaGetRotKeySetId
 *
 */
TKStatus ktaGetRotKeySetId
(
  uint8_t* xpRotKeySetId
)
{
  TKStatus status = E_K_STATUS_ERROR;

  M_KTALOG__START("Start");

  if (NULL == xpRotKeySetId)
  {
    status = E_K_STATUS_PARAMETER;
    M_KTALOG__ERR("Invalid parameter passed");
  }
  else
  {
    *xpRotKeySetId = gKtaContextInfoConfig.rotKeySetId;
    status = E_K_STATUS_OK;
  }

  M_KTALOG__END("End, status : %d", status);
  return status;
}

/**
 * @brief implement ktaSetRotKeySetId
 *
 */
TKStatus ktaSetRotKeySetId
(
  uint8_t xRotKeySetId
)
{
  TKStatus status = E_K_STATUS_ERROR;

  M_KTALOG__START("Start");

  if ((uint8_t)0x00 == xRotKeySetId)
  {
    status = E_K_STATUS_PARAMETER;
    M_KTALOG__ERR("Invalid parameter passed");
  }
  else
  {
    gKtaContextInfoConfig.rotKeySetId = xRotKeySetId;
    status = E_K_STATUS_OK;
  }

  M_KTALOG__END("End, status : %d", status);
  return status;
}

#ifdef TEST_COVERAGE
void ktaResetConfig(void)
{
  memset(&gKtaDeviceInfoConfig, 0, sizeof(TKtaDeviceInfoConfig));
  memset(&gKtaContextInfoConfig, 0, sizeof(TKtaContextInfoConfig));
}
#endif
/* -------------------------------------------------------------------------- */
/* LOCAL FUNCTIONS - IMPLEMENTATION                                           */
/* -------------------------------------------------------------------------- */

/**
 * @implements lReadDeviceAndContextInfo
 *
 */
/**
 * SUPPRESS: MISRA_DEV_KTA_005 : misra_c2012_rule_15.4_violation
 * SUPPRESS: MISRA_DEV_KTA_004 : misra_c2012_rule_15.1_violation
 * Using goto for breaking during the error and return cases.
 **/
static TKStatus lReadDeviceAndContextInfo
(
  const TKtaLifeCycleState xState
)
{
  TKStatus status   = E_K_STATUS_ERROR;
  uint8_t  aKtaInfo[C_K_KTA__SEALED_INFORMATION_MAX_SIZE]  = {0};
  size_t   ktaInfoLen = C_K_KTA__SEALED_INFORMATION_MAX_SIZE;
  size_t   keyMaterialLen = C_KEY_CONFIG__MATERIAL_MAX_SIZE;

  if ((xState >= E_LIFE_CYCLE_STATE_SEALED) && (xState <= E_LIFE_CYCLE_STATE_PROVISIONED))
  {
    status = salStorageGetValue(C_K_KTA__SEALED_DATA_STORAGE_ID, aKtaInfo, &ktaInfoLen);

    if ((E_K_STATUS_OK != status) || (0u == ktaInfoLen))
    {
      M_KTALOG__ERR("SAL API failed while reading sealed data");
      goto end;
    }

    ktaInfoLen = 0;
    (void)memcpy(gKtaContextInfoConfig.ktaContextProfileUid,
                  &aKtaInfo[ktaInfoLen],
                  C_K__CONTEXT_PROFILE_UID_MAX_SIZE);
    ktaInfoLen += C_K__CONTEXT_PROFILE_UID_MAX_SIZE;
    gKtaContextInfoConfig.ktaContextProfileUidLength = aKtaInfo[ktaInfoLen];
    ktaInfoLen++;

    (void)memcpy(gKtaContextInfoConfig.ktaContexSerialNumber,
                  &(aKtaInfo[ktaInfoLen]),
                  C_K__CONTEXT_SERIAL_NUMBER_MAX_SIZE);
    ktaInfoLen += C_K__CONTEXT_SERIAL_NUMBER_MAX_SIZE;
    gKtaContextInfoConfig.ktaContexSerialNumberLength = aKtaInfo[ktaInfoLen];
    ktaInfoLen++;

    (void)memcpy(gKtaContextInfoConfig.ktaContextVersion,
                  &(aKtaInfo[ktaInfoLen]),
                  C_K__CONTEXT_VERSION_MAX_SIZE);
    ktaInfoLen += C_K__CONTEXT_VERSION_MAX_SIZE;
    gKtaContextInfoConfig.ktaContextVersionLength = aKtaInfo[ktaInfoLen];
    ktaInfoLen++;

    /* KTA version should not be read from NVM. */
    // REQ RQ_M-KTA-REGT-CF-0070(1) : KTA Version
    (void)memset(gKtaContextInfoConfig.ktaVersion, 0, C_KTA__VERSION_MAX_SIZE);
    (void)memcpy(gKtaContextInfoConfig.ktaVersion,
                  ktaGetVersion(),
                  strnlen((const char*)ktaGetVersion(), C_KTA__VERSION_MAX_SIZE));
    ktaInfoLen += C_KTA__VERSION_MAX_SIZE;

    (void)memcpy(gKtaDeviceInfoConfig.deviceProfilePubUID[0],
                  &aKtaInfo[ktaInfoLen],
                  C_K__DEVICE_PROFILE_PUBLIC_UID_MAX_SIZE);
    ktaInfoLen += C_K__DEVICE_PROFILE_PUBLIC_UID_MAX_SIZE;
    gKtaDeviceInfoConfig.deviceProfilePubUIDLength[0] = aKtaInfo[ktaInfoLen];
    ktaInfoLen++;

    (void)memcpy(gKtaDeviceInfoConfig.deviceSerailNo,
                  &aKtaInfo[ktaInfoLen],
                  C_K__DEVICE_SERIAL_NUM_MAX_SIZE);
    ktaInfoLen += C_K__DEVICE_SERIAL_NUM_MAX_SIZE;
    gKtaDeviceInfoConfig.deviceSerailNoLength = aKtaInfo[ktaInfoLen];
  }

  if ((xState == E_LIFE_CYCLE_STATE_ACTIVATED) ||
      (xState == E_LIFE_CYCLE_STATE_PROVISIONED) ||
      (xState == E_LIFE_CYCLE_STATE_CON_REQ))
  {
    uint8_t aL1KeyMaterial[C_KEY_CONFIG__MATERIAL_MAX_SIZE];

    /* Added to supress the misra-c2012-9.3 msg="Arrays shall not be partially initialized" */
    memset(aL1KeyMaterial , 0x00 , C_KEY_CONFIG__MATERIAL_MAX_SIZE);

    status = salStorageGetValue(C_K_KTA__L1_KEY_MATERIAL_DATA_ID,
                                aL1KeyMaterial,
                                &keyMaterialLen);

    if ((E_K_STATUS_OK != status) || (0u == keyMaterialLen))
    {
      M_KTALOG__ERR("SAL API failed while reading L1 key material");
      goto end;
    }

    (void)memcpy(gKtaContextInfoConfig.l1SegSeed, aL1KeyMaterial, C_K__L1_SEGMENTATION_SEED_SIZE);
    gKtaContextInfoConfig.rotKeySetId = aL1KeyMaterial[C_K__L1_SEGMENTATION_SEED_SIZE];
  }

end:
  return status;
}

/* -------------------------------------------------------------------------- */
/* END OF FILE                                                                */
/* -------------------------------------------------------------------------- */
