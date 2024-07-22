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
/** \brief keySTREAM Trusted Agent - Public interface.
 *
 *  \author Kudelski IoT
 *
 *  \date 2023/06/13
 *
 *  \file k_kta.h
 ******************************************************************************/

/**
 * @brief keySTREAM Trusted Agent - Public interface.
 */

#ifndef K_KTA_H
#define K_KTA_H

#ifdef __cplusplus
extern "C" {
#endif /* C++ */

/* --------------------------------------------------------------------------------------------- */
/* IMPORTS                                                                                       */
/* --------------------------------------------------------------------------------------------- */
#include "k_defs.h"

#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>

/** @addtogroup g_kta_api
 * @{
*/

/* --------------------------------------------------------------------------------------------- */
/* CONSTANTS, TYPES, ENUM                                                                        */
/* --------------------------------------------------------------------------------------------- */
/** @brief Maximal size of device profile public uid field, in bytes. */
#define C_K__DEVICE_PROFILE_PUBLIC_UID_MAX_SIZE  (32u)

/** @brief Maximal size of device serial no field, in bytes. */
#define C_K__DEVICE_SERIAL_NUM_MAX_SIZE          (16u)

/** @brief Maximal size of L1 segmentation seed, in bytes. */
#define C_K__L1_SEGMENTATION_SEED_SIZE           (16u)

/** @brief Maximal size of context profile uid, in bytes. */
#define C_K__CONTEXT_PROFILE_UID_MAX_SIZE        (32u)

/** @brief Maximal size of context serial number, in bytes. */
#define C_K__CONTEXT_SERIAL_NUMBER_MAX_SIZE      (16u)

/** @brief Maximal size of context version, in bytes. */
#define C_K__CONTEXT_VERSION_MAX_SIZE            (16u)

/** @brief Maximal size of keySTREAM Trusted Agent version, in bytes. */
#define C_KTA__VERSION_MAX_SIZE                  (16u)

/** @brief Maximal size of connection request data, in bytes. */
#define C_K__CONN_REQ_MAX_SIZE                   (4u)

/** @brief keySTREAM Trusted Agent version Storage length. */
#define C_K__VERSION_STORAGE_LENGTH              (2u)

/** @brief keySTREAM command status. */
typedef enum
{
  /**
   * Initial command state.
   */
  E_K_KTA_KS_STATUS_NONE = -1,
  /**
   * Communication to keySTREAM is done and the device is in provisioned state,
   * the device has received no operation command in response, noting to be done.
   */
  E_K_KTA_KS_STATUS_NO_OPERATION,
  /**
   * Device received renewal command.
   */
  E_K_KTA_KS_STATUS_RENEW,
  /**
   * Device received refurbish command,
   * all the provsioning related data must be wiped off from the device.
   */
  E_K_KTA_KS_STATUS_REFURBISH,
} TKktaKeyStreamStatus;

/* --------------------------------------------------------------------------------------------- */
/* VARIABLES                                                                                     */
/* --------------------------------------------------------------------------------------------- */

/* --------------------------------------------------------------------------------------------- */
/* FUNCTIONS                                                                                     */
/* --------------------------------------------------------------------------------------------- */

/**
 * @brief
 *   Initialize the keySTREAM Trusted Agent.
 *
 * @return
 * - E_K_STATUS_OK in case of success.
 * - E_K_STATUS_ERROR for other errors.
 */
TKStatus ktaInitialize
(
  void
);

/**
 * @brief
 *   Set keySTREAM Trusted Agent context configuration data to the platform.
 *
 * @pre
 *   The function ktaInitialize() has been called.
 *
 * @param[in] xpL1SegSeed
 *   L1 segmentation seed value.
 *   Exact value is C_K__L1_SEGMENTATION_SEED_SIZE bytes
 *   Should be provided by the caller.
 * @param[in] xpKtaContextProfileUid
 *   Context profile uid value, max C_K__CONTEXT_PROFILE_UID_MAX_SIZE.
 *   Should not be NULL.
 *   Buffer must be provided by the caller.
 * @param[in] xKtaContextProfileUidLen
 *   Max value is C_K__CONTEXT_PROFILE_UID_MAX_SIZE bytes.
 *   Should be provided by the caller.
 * @param[in] xpKtaContexSerialNumber
 *   keySTREAM Trusted Agent context serial number,
 *   max C_K__CONTEXT_SERIAL_NUMBER_MAX_SIZE.
 *   Should not be NULL.
 *   Buffer must be provided by the caller.
 * @param[in] xKtaContexSerialNumberLen
 *   Max value is C_K__CONTEXT_SERIAL_NUMBER_MAX_SIZE bytes.
 *   Should be provided by the caller
 * @param[in] xpKtaContextVersion
 *   keySTREAM Trusted Agent context version,
 *   must not exceed C_K__CONTEXT_VERSION_MAX_SIZE.
 *   Should not be NULL.
 *   Buffer must be provided by the caller.
 * @param[in] xKtaContextVersionLen
 *   Max value is C_K__CONTEXT_VERSION_MAX_SIZE bytes.
 *   should be provided by caller.
 *
 * @return
 * - E_K_STATUS_OK in case of success.
 * - E_K_STATUS_PARAMETER for wrong input parameter.
 * - E_K_STATUS_ERROR for other errors.
 */
TKStatus ktaStartup
(
  const uint8_t*  xpL1SegSeed,
  const uint8_t*  xpKtaContextProfileUid,
  size_t          xKtaContextProfileUidLen,
  const uint8_t*  xpKtaContexSerialNumber,
  size_t          xKtaContexSerialNumberLen,
  const uint8_t*  xpKtaContextVersion,
  size_t          xKtaContextVersionLen
);

/**
 * @brief
 * Set the device specific info.
 *
 * @pre
 *   The function ktaStartup() has been called.
 *
 * @param[in] xpDeviceProfilePublicUid
 *   Device profile public uid, max C_K__DEVICE_PROFILE_PUBLIC_UID_MAX_SIZE bytes;
 *   Should not be NULL.
 *   Buffer must be provided by the caller.
 * @param[in] xDeviceProfilePublicUidSize
 *   Size of device profile public uid, max C_K__DEVICE_PROFILE_PUBLIC_UID_MAX_SIZE.
 *   Should be provided by caller.
 * @param[in] xpDeviceSerialNum
 *   Device Serial no, max C_K__DEVICE_SERIAL_NUM_MAX_SIZE bytes.
 *   Should not be NULL.
 *   Buffer must be provided by the caller.
 * @param[in] xDeviceSerialNumSize
 *   Size of device serial number, max C_K__DEVICE_SERIAL_NUM_MAX_SIZE bytes.
 *   Should be provided by caller.
 * @param[in,out] xpConnectionRequest
 *   [in] Pointer to buffer to carry keySTREAM status. max C_K__CONN_REQ_MAX_SIZE.
 *   [out] 0: Device is already provisioned in the keySTREAM,
 *            no communication is needed with the keySTREAM.
 *         1: Device is not yet provisioned in the keySTREAM,
 *            communicate with the keySTREAM for onboarding.
 * @remark
 *   The values pointed to by the parameters are dumped and the caller may release them
 *   after this call.
 *
 * @return
 * - E_K_STATUS_OK in case of success.
 * - E_K_STATUS_PARAMETER for wrong input parameter.
 * - E_K_STATUS_ERROR for other errors.
 */
TKStatus ktaSetDeviceInformation
(
  const uint8_t*  xpDeviceProfilePublicUid,
  size_t          xDeviceProfilePublicUidSize,
  const uint8_t*  xpDeviceSerialNum,
  size_t          xDeviceSerialNumSize,
  uint8_t*        xpConnectionRequest
);

/**
 * @brief
 *   keySTREAM Trusted Agent builds notification message, which must then be sent via application
 *   to keySTREAM.
 *
 * @pre
 *   Network stack is ready to communicate with the keySTREAM.
 *   The function ktaStartup() has been called.
 *
 * @param[in] xpKs2ktaMsg
 *   keySTREAM message to provide to keySTREAM Trusted Agent.
 *   Buffer should be at-least C_K__ICPP_MSG_MAX_SIZE size.
 *   Should not be NULL.
 *   Buffer must be provided by the caller.
 * @param[in] xKs2ktaMsgLen
 *   Should be size of message received from the keySTREAM otherwise 0.
 *   Max length C_K__ICPP_MSG_MAX_SIZE.
 *   should be provided by the caller.
 * @param[in,out] xpKta2ksMsg
 *   [in] Pointer to buffer to carry message to send to keySTREAM.
 *   [out] Actual message to provide to keySTREAM.
 *   Buffer should at-least C_K__ICPP_MSG_MAX_SIZE size.
 *   Should not be NULL.
 *   Buffer must be provided by the caller.
 * @param[in,out] xpKta2ksMsgLen
 *   [in] Max length C_K__ICPP_MSG_MAX_SIZE.
 *   [out] Actual data length containing message to send to keySTREAM.
 *   xKs2ktaMsgLen will be filled with actual size received from the keySTREAM.
 *   If it is 0, It indicates that no furhter operation to be performed.
 *   Should be provided by the caller
 *
 * @return
 * - E_K_STATUS_OK in case of success.
 * - E_K_STATUS_PARAMETER for wrong input parameter.
 * - E_K_STATUS_ERROR for other errors.
 */
TKStatus ktaExchangeMessage
(
  const uint8_t*  xpKs2ktaMsg,
  size_t          xKs2ktaMsgLen,
  uint8_t*        xpKta2ksMsg,
  size_t*         xpKta2ksMsgLen
);

#ifdef OBJECT_MANAGEMENT_FEATURE
/**
 * @brief
 *   Gets an object which includes key, device cerificate and assosciated info.
 *   This object has been provisioned and is managed by keySTREAM.
 *
 * @pre
 *   The function ktaExchangeMessage() is called and device is in provisioned state.
 *
 * @param[in] xObjWithAssociationId
 *   Should be a valid key identifier, It is pre defined in application and
 *   user is not expected to change its value.
 *   Object with Association identifier.
 * @param[in,out] xpAssociatedKeyId
 *   [in] Pointer to buffer to carry asscociated key ID.
 *   [out] Associated key identifier (e.g. private key associated to a certificate).
 *   Should not be NULL.
 *   Buffer must be provided by the caller.
 * @param[in, out] xpAssociatedObjId
 *   [in] Pointer to buffer to carry asscociated object ID.
 *   [out] Associated object identifier
 *   (e.g. signing certificate id info, which then be used by ktaGetAssociatedObject() API, to read
 *   the signer cetificate.
 *   Should not be NULL.
 *   Buffer must be provided by the caller.
 * @param[out] xpOutData
 *   [in] Pointer to buffer to carry the device ceritficate (in Bytes).
 *   [out] Device ceritifcate in der (e.g. x.509 device certificate).
 *   Should not be NULL.
 *   Buffer must be provided by the caller.
 * @param[in,out] xpOutDataLen
 *   [in] Pointer to buffer to carry length of the device ceritficate (in Bytes).
 *   [out] Actual output length.
 *   The caller set the maximum length expected (typ. the buffer size).
 *   Then, keySTREAM Trusted Agent writes the actual length of the data written
 *   to the buffer.
 *   Should be provided by the caller.
 *
 * @return
 * - E_K_STATUS_OK in case of success.
 * - E_K_STATUS_PARAMETER for wrong input parameter.
 * - E_K_STATUS_ERROR for other errors.
 */
TKStatus ktaGetObjectWithAssociation
(
  uint32_t   xObjWithAssociationId,
  uint32_t*  xpAssociatedKeyId,
  uint32_t*  xpAssociatedObjId,
  uint8_t*   xpOutData,
  size_t*    xpOutDataLen
);

/**
 * @brief
 *  Gets an object which includes signer cerificate and assosciated info.
 *  This object has been provisioned and is managed by keySTREAM.
 *
 * @pre
 *   The function ktaGetObjectWithAssociation() is called and device is in provisioned state.
 *
 * @param[in] xObjWithAssociationId
 *   Should be a valid key identifier, It is the same key Identifier which application receives
 *   from the parameter "associatedObjId", in ktaGetObjectWithAssociation() API.
 *   Object with Association identifier.
 *   Should be provided by the caller
 * @param[in,out] xpOutData
 *   [in] Pointer to buffer to carry the device ceritficate (in Bytes).
 *   [out] Signer ceritifcate in der (e.g. x.509 signer certificate).
 *   Should not be NULL.
 *   Buffer must be provided by the caller.
 * @param[in,out] xpOutDataLen
 *   [in] Pointer to buffer to carry the length of the signer ceritficate (in Bytes).
 *   [out] Actual output length.
 *   The caller set the maximum length expected (typ. the buffer size).
 *   Then, keySTREAM Trusted Agent writes the actual length of the data written
 *   to the buffer.
 *   Should be provided by the caller.
 *
 * @return
 * - E_K_STATUS_OK in case of success.
 * - E_K_STATUS_PARAMETER for wrong input parameter.
 * - E_K_STATUS_ERROR for other errors.
 */
TKStatus ktaGetAssociatedObject
(
  uint32_t  xObjWithAssociationId,
  uint8_t*  xpOutData,
  size_t*   xpOutDataLen
);

/**
 * @brief
 *    Sign the data with keySTREAM Trusted Agent provisioned key.
 *    The key is always located inside the secure platform and addressed by an identifier.
 *
 * @pre
 *   The function ktaExchangeMessage() is called and device is in provisioned state.
 *
 * @param[in] xKeyId
 *   Should be a valid key identifier, It is the same key Identifier which application receives
 *   from the parameter "xpAssociatedKeyId", in ktaGetObjectWithAssociation() API.
 *   Object with Association identifier.
 *   Should be provided by the caller
 * @param[in] xpHash
 *   Hash To Sign: pointer to buffer to compute hash.
 *   Should not be NULL.
 *   Buffer must be provided by the caller.
 * @param[in] xHashLen
 *   Length of buffer to compute hash.
 *   Should be provided by the caller.
 * @param[in,out] xpSignedHashOutBuff
 *   [in] Pointer to buffer carrying signed data.
 *   [out] SignHash output data: pointer to buffer.
 *   Should not be NULL.
 *   Buffer must be provided by the caller.
 * @param[in] xSignedHashOutBuffLen
 *   SignHash output data length, max size.
 * @param[out] xpActualSignedHashOutLen
 *   Length of filled signedHashOutBuffLen data.
 *   Should not be NULL.
 *   Buffer must be provided by the caller.
 *
 * @return
 * - E_K_STATUS_OK in case of success.
 * - E_K_STATUS_PARAMETER for wrong input parameter.
 * - E_K_STATUS_ERROR for other errors.
 */
TKStatus ktaSignHash
(
  uint32_t  xKeyId,
  uint8_t*  xpHash,
  size_t    xHashLen,
  uint8_t*  xpSignedHashOutBuff,
  uint32_t  xSignedHashOutBuffLen,
  size_t*   xpActualSignedHashOutLen
);
#endif

/**
 * @brief
 *   Provide command received from the keySTREAM
 *   There can be three valid command from keySTREAM
 *   1. If the no operation is received E_K_KTA_KS_STATUS_NO_OPERATION
 *   2. If the renewal is performend from keySTREAM E_K_KTA_KS_STATUS_RENEW
 *   3. If the refurbish is performned from keySTREAM E_K_KTA_KS_STATUS_REFURBISH
 *
 * @pre
 *   The function ktaExchangeMessage() is called and device is in provisioned state.
 *
 * @param[in,out] xpKtaKSCmdStatus
 *   [in] Buffer to carry command status.
 *   [out] Actual command status.
 *   Should not be NULL.
 *   Buffer must be provided by the caller.
 *
 * @return
 * - E_K_STATUS_OK in case of success.
 * - E_K_STATUS_PARAMETER for wrong input parameter.
 * - E_K_STATUS_ERROR for other errors.
 */
TKStatus ktaKeyStreamStatus
(
  TKktaKeyStreamStatus*  xpKtaKSCmdStatus
);

#ifdef TEST_COVERAGE
void ktaReset
(
  void
);
#endif
/** @} g_kta_api */
#ifdef __cplusplus
}
#endif /* C++ */

#endif // K_KTA_H

/* -------------------------------------------------------------------------- */
/* END OF FILE                                                                */
/* -------------------------------------------------------------------------- */
