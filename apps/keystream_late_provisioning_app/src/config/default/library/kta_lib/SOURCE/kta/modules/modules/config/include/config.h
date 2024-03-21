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
/** \brief keySTREAM Trusted Agent - Device configuration module.
 *
 *  \author Kudelski IoT
 *
 *  \date 2023/06/02
 *
 *  \file config.h
 *
 ******************************************************************************/

/**
 * @brief keySTREAM Trusted Agent - Device configuration module.
 */

#ifndef KTACONFIG_H
#define KTACONFIG_H

#ifdef __cplusplus
extern "C" {
#endif /* C++ */

/* --------------------------------------------------------------------------------------------- */
/* IMPORTS                                                                                       */
/* --------------------------------------------------------------------------------------------- */
#include "k_defs.h"
#include "k_kta.h"

#include <stddef.h>
#include <stdint.h>

/* --------------------------------------------------------------------------------------------- */
/* CONSTANTS, TYPES, ENUM                                                                        */
/* --------------------------------------------------------------------------------------------- */
/** @brief Maximum no of lifecycle state supported. */
#define C_KTA_CONFIG__LIFE_CYCLE_MAX_STATE        (6u)

/** @brief Maximum size of each life cycle state in bytes. */
#define C_KTA_CONFIG__LIFE_CYCLE_EACH_STATE_SIZE  (4u)

/** @brief Maximal size of device profile UID supported */
#define C_K__MAX_DEVICE_PROFILES                 (2u)

/**
 * @brief Maximum size of material in bytes.
 * L1SegSeed (16 bytes) || KeySetID ( 1 byte)
 */
#define C_KEY_CONFIG__MATERIAL_MAX_SIZE           (17u)

/** @brief keySTREAM Trusted Agent device info configuration. */
typedef struct
{
  uint8_t   deviceProfilePubUID[C_K__MAX_DEVICE_PROFILES][C_K__DEVICE_PROFILE_PUBLIC_UID_MAX_SIZE];
  /* Device profile public UID. */
  uint8_t   deviceProfilePubUIDLength[C_K__MAX_DEVICE_PROFILES];
  /* Length of the device profile public UID, in bytes. */
  uint8_t   deviceSerailNo[C_K__DEVICE_SERIAL_NUM_MAX_SIZE];
  /* Device serial no. */
  uint8_t   deviceSerailNoLength;
  /* Length of the device serial no, in bytes. */
} TKtaDeviceInfoConfig;

/** @brief keySTREAM Trusted Agent context info configuration. */
typedef struct
{
  uint8_t l1SegSeed[C_K__L1_SEGMENTATION_SEED_SIZE];
  /* L1 segmenation seed. */
  uint8_t ktaContextProfileUid[C_K__CONTEXT_PROFILE_UID_MAX_SIZE];
  /* keySTREAM Trusted Agent context profile uid. */
  uint8_t ktaContextProfileUidLength;
  /* keySTREAM Trusted Agent context profile uid length. */
  uint8_t ktaContexSerialNumber[C_K__CONTEXT_SERIAL_NUMBER_MAX_SIZE];
  /* keySTREAM Trusted Agent context serial number. */
  uint8_t ktaContexSerialNumberLength;
  /* keySTREAM Trusted Agent context serial number length. */
  uint8_t ktaContextVersion[C_K__CONTEXT_VERSION_MAX_SIZE];
  /* keySTREAM Trusted Agent context version. */
  uint8_t ktaContextVersionLength;
  /* keySTREAM Trusted Agent context version length. */
  uint8_t ktaVersion[C_KTA__VERSION_MAX_SIZE];
  /* keySTREAM Trusted Agent version. */
  uint8_t rotKeySetId;
  /* keySTREAM Trusted Agent rot key set id. */
} TKtaContextInfoConfig;

/** @brief keySTREAM Trusted Agent life cycle state. */
typedef enum
{
  /**
   * No keySTREAM Trusted Agent call has been made yet.
   */
  E_LIFE_CYCLE_STATE_INIT           = 0,
  /**
   * The keySTREAM Trusted Agent sealing is done.
   */
  E_LIFE_CYCLE_STATE_SEALED         = 1,
  /**
   * The  keySTREAM Trusted Agent activation is done.
   */
  E_LIFE_CYCLE_STATE_ACTIVATED      = 2,
  /**
   * The keySTREAM Trusted Agent provisioning is done.
   */
  E_LIFE_CYCLE_STATE_PROVISIONED    = 3,
  /**
   * Connection request.
   */
  E_LIFE_CYCLE_STATE_CON_REQ        = 4,
  /**
   * Invalid state.
   */
  E_LIFE_CYCLE_STATE_INVALID        = 0xFF
} TKtaLifeCycleState;

/* --------------------------------------------------------------------------------------------- */
/* VARIABLES                                                                                     */
/* --------------------------------------------------------------------------------------------- */

/* --------------------------------------------------------------------------------------------- */
/* FUNCTIONS                                                                                     */
/* --------------------------------------------------------------------------------------------- */

/**
 * @brief
 *   Sets Device info configuration data to platform.
 *
 * @param[in] xpDeviceProfilePublicUid
 *   Should not be NULL.
 *   Device profile public uid value should be provided by caller.
 *   Max value is C_K__DEVICE_PROFILE_PUBLIC_UID_MAX_SIZE.
 * @param[in] xDeviceProfilePublicUidSize
 *   Size of device profile public uid.
 * @param[in] xpDeviceSerialNum
 *   Should not be NULL.
 *   Device Serial number value should be provided by caller.
 *   Max value is C_K_DEVICE_SERAIL_NUM_MAX_SIZE.
 * @param[in] xDeviceSerialNumSize
 *   Size of device serial number.
 * @param[in] xState
 *   Life cycle state of keySTREAM Trusted Agent.
 *
 * @return
 * - E_K_STATUS_OK in case of success.
 * - E_K_STATUS_PARAMETER for wrong input parameter.
 * - E_K_STATUS_ERROR for other errors.
 */
TKStatus ktaSetDeviceInfoConfig
(
  const uint8_t* xpDeviceProfilePublicUid,
  size_t         xDeviceProfilePublicUidSize,
  const uint8_t* xpDeviceSerialNum,
  size_t         xDeviceSerialNumSize,
  const TKtaLifeCycleState xState
);

/**
 * @brief
 *   Gets device information.
 *
 * @param[out] xpDeviceInfoConfig
 *   Should not be NULL.
 *   Buffer should be provided by caller.
 *   Buffer will be filled with device specific info.
 *
 * @return
 * - E_K_STATUS_OK in case of success.
 * - E_K_STATUS_PARAMETER for wrong input parameter.
 * - E_K_STATUS_ERROR for other errors.
 */
TKStatus ktaGetDeviceInfoConfig
(
  TKtaDeviceInfoConfig* xpDeviceInfoConfig
);

/**
 * @brief
 *   Sets keySTREAM Trusted Agent context configuration data to the platform.
 *
 * @param[in] xpL1SegSeed
 *   Should not be NULL.
 *   L1 segmentation seed value should be provided by caller.
 *   Exact value is C_K__L1_SEGMENTATION_SEED_SIZE bytes.
 * @param[in] xpKtaContextProfileUid
 *   Should not be NULL.
 *   Context profile uid value should be provided by caller.
 * @param[in] xKtaContextProfileUidLen
 *   Should be provided by caller.
 *   Max value is C_K__CONTEXT_PROFILE_UID_MAX_SIZE bytes.
 * @param[in] xpKtaContexSerialNumber
 *   Should not be NULL.
 *   Context serial number value should be provided by caller.
 * @param[in] xKtaContexSerialNumberLen
 *   Should be provided by caller.
 *   Max value is C_K__CONTEXT_SERIAL_NUMBER_MAX_SIZE bytes.
 * @param[in] xpKtaContextVersion
 *   Should not be NULL.
 *   Context version value should be provided by caller.
 * @param[in] xKtaContextVersionLen
 *   Should be provided by caller.
 *   Max value is C_K__CONTEXT_VERSION_MAX_SIZE bytes.
 * @param[in] xState
 *   Life cycle state of keySTREAM Trusted Agent.
 *
 * @return
 * - E_K_STATUS_OK in case of success.
 * - E_K_STATUS_PARAMETER for wrong input parameter.
 * - E_K_STATUS_ERROR for other errors.
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
);

/**
 * @brief
 *   Gets context specific data from platform.
 *
 * @param[out] xpContextInfoConfig
 *   Should not be NULL.
 *   Buffer should be provided by caller.
 *   Buffer will be filled with keySTREAM Trusted Agent context info.
 *
 * @return
 * - E_K_STATUS_OK in case of success.
 * - E_K_STATUS_PARAMETER for wrong input parameter.
 * - E_K_STATUS_ERROR for other errors.
 */
TKStatus ktaGetContextInfoConfig
(
  TKtaContextInfoConfig* xpContextInfoConfig
);

/**
 * @brief
 *   Read rot key set id.
 *
 * @param[out] xpRotKeySetId
 *   Buffer filled with rot key set id value.
 *
 * @return
 * - E_K_STATUS_OK in case of success.
 * - E_K_STATUS_PARAMETER for wrong input parameter.
 * - E_K_STATUS_ERROR for other errors.
 */
TKStatus ktaGetRotKeySetId
(
  uint8_t* xpRotKeySetId
);

/**
 * @brief
 *   Get L1 segmentation seed set by application.
 *
 * @param[out] xpL1SegSeed
 *   Should not be NULL.
 *   Buffer filled with segmentation seed of C_K__L1_SEGMENTATION_SEED_SIZE bytes.
 *
 * @return
 * - E_K_STATUS_OK in case of success.
 * - E_K_STATUS_PARAMETER for wrong input parameter.
 * - E_K_STATUS_ERROR for other errors.
 */
TKStatus ktaGetL1SegSeed
(
  uint8_t* xpL1SegSeed
);

/**
 * @brief
 *   Set rot key set id in NVM.
 *
 * @param[in] xRotKeySetId
 *   Rot key set id value should be set.
 *
 * @return
 * - E_K_STATUS_OK in case of success.
 * - E_K_STATUS_PARAMETER for wrong input parameter.
 * - E_K_STATUS_ERROR for other errors.
 */
TKStatus ktaSetRotKeySetId
(
  uint8_t xRotKeySetId
);

#ifdef TEST_COVERAGE
void ktaResetConfig(void);
#endif

#ifdef __cplusplus
}
#endif /* C++ */

#endif // KTACONFIG_H
/* -------------------------------------------------------------------------- */
/* END OF FILE                                                                */
/* -------------------------------------------------------------------------- */
