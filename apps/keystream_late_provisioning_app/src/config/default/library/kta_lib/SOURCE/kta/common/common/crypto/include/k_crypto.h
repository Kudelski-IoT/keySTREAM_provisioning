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
/** \brief keySTREAM Trusted Agent - Cipher module for cryptographic operations.
 *
 *  \author Kudelski IoT
 *
 *  \date 2023/06/12
 *
 *  \file k_crypto.h
 ******************************************************************************/

/**
 * @brief keySTREAM Trusted Agent - Cipher module for cryptographic operations.
 */

#ifndef KTACIPHER_H
#define KTACIPHER_H

#ifdef __cplusplus
extern "C" {
#endif /* C++ */

/* -------------------------------------------------------------------------- */
/* IMPORTS                                                                    */
/* -------------------------------------------------------------------------- */
#include "k_defs.h"

#include <stdint.h>
#include <stddef.h>

/* -------------------------------------------------------------------------- */
/* CONSTANTS, TYPES, ENUM                                                     */
/* -------------------------------------------------------------------------- */

/** @brief Block size to align the message. */
#define C_K_CRYPTO__AES_BLOCK_SIZE (16u)

/** @brief Padding start condition. */
#define C_K_CRYPTO__PAD_START_BYTE (0x80u)

/* -------------------------------------------------------------------------- */
/* VARIABLES                                                                  */
/* -------------------------------------------------------------------------- */

/* -------------------------------------------------------------------------- */
/* FUNCTIONS                                                                  */
/* -------------------------------------------------------------------------- */
/**
 * @brief
 *   Encrypt the given message.
 *
 * @param[in] xpClearMsg
 *   Clear message buffer to encrypt.
 * @param[in] xClearMsgLen
 *   Clear message buffer length to encrypt.
 * @param[in,out] xpEncryptedMsg
 *   [in] Pointer to buffer to carry encrypted message.
 *   [out] Actual encrypted message.
 * @param [in,out] xpEncryptedMsgLen
 *   [in] Pointer to buffer to carry encrypted message length.
 *   [out] Actual encrypted message length.
 *
 * @return
 * - E_K_STATUS_OK in case of success.
 * - E_K_STATUS_PARAMETER for wrong input parameter.
 * - E_K_STATUS_ERROR for other errors.
 */
TKStatus ktacipherEncrypt
(
  const uint8_t* xpClearMsg,
  size_t         xClearMsgLen,
  uint8_t*       xpEncryptedMsg,
  size_t*        xpEncryptedMsgLen
);

/**
 * @brief
 *   Decrypt the given encrypted message.
 *
 * @param[in] xpEncryptedMsg
 *   Encrypted message buffer to decrypt.
 * @param[in] xEncryptedMsgLen
 *   Length of encrypted message buffer to decrypt.
 * @param[in] xpClearMsg
 *   [in] Pointer to buffer to carry clear message.
 *   [out] Buffer updated with actual clear message.
 * @param[in,out] xpClearMsgLen
 *   [in] Length of clear message buffer.
 *   [out] Actual clear message length.
 *
 * @return
 * - E_K_STATUS_OK in case of success.
 * - E_K_STATUS_PARAMETER for wrong input parameter.
 * - E_K_STATUS_ERROR for other errors.
 */
TKStatus ktacipherDecrypt
(
  const uint8_t* xpEncryptedMsg,
  size_t         xEncryptedMsgLen,
  uint8_t*       xpClearMsg,
  size_t*        xpClearMsgLen
);

/**
 * @brief
 *   Sign the given message.
 *
 * @param[in] xpMsgToSign
 *   Clear message to sign.
 * @param[in] xMsgToSignLen
 *   Clear message to sign length.
 * @param[in,out] xpComputedMac
 *   [in] Pointer to buffer to carry computed MAC.
 *   [out] Actual MAC message.
 *   Data must be at least C_K_KTA__HMAC_MAX_SIZE
 *
 * @return
 * - E_K_STATUS_OK in case of success.
 * - E_K_STATUS_PARAMETER for wrong input parameter.
 * - E_K_STATUS_ERROR for other errors.
 */
TKStatus ktacipherSignMsg
(
  const uint8_t* xpMsgToSign,
  size_t         xMsgToSignLen,
  uint8_t*       xpComputedMac
);

/**
 * @brief
 *   Verfiy the signed message.
 *
 * @param[in] xpMsgToVerify
 *   Message buffer to verify the signature.
 * @param[in] xMsgToVerifyLen
 *   Message buffer length to verify the signature.
 * @param[in] xpMacToVerify
 *   Mac to verify with message buffer.
 *   Data must be at least C_K_KTA__HMAC_MAX_SIZE
 *
 * @return
 * - E_K_STATUS_OK in case of success.
 * - E_K_STATUS_PARAMETER for wrong input parameter.
 * - E_K_STATUS_ERROR for other errors.
 */
TKStatus ktacipherVerifySignedMsg
(
  const uint8_t* xpMsgToVerify,
  size_t         xMsgToVerifyLen,
  const uint8_t* xpMacToVerify
);

/**
 * @brief
 *   Add padding to data buffer.
 *
 * @param[in] xpData
 *   Non padded buffer.
 * @param[in] xDataLength
 *   Non padded buffer length.
 * @param[inout] xpPaddedData
 *   [in] Pointer to buffer to carry padded data.
 *   [out] Actual padded data.
 * @param[in,out] xpPaddedDataLength
 *   [in] Pointer to buffer to carry padded data length.
 *   [out] Actual padded data length.
 *
 * @return
 * - E_K_STATUS_OK in case of success.
 * - E_K_STATUS_PARAMETER for wrong input parameter.
 * - E_K_STATUS_ERROR for other errors.
 */
TKStatus ktacipherAddPadding
(
  const uint8_t* xpData,
  const size_t   xDataLength,
  uint8_t*       xpPaddedData,
  size_t*        xpPaddedDataLength
);

/**
 * @brief
 *   Remove padding from given data.
 *
 * @param[in, out] xpData
 *   [in] Buffer with padded data.
 *   [out] Buffer without padded data.
 * @param[in,out] xpDataLength
 *   [in] Padded data length.
 *   [out] Actual Non padded data length.
 *
 * @return
 * - E_K_STATUS_OK in case of success.
 * - E_K_STATUS_PARAMETER for wrong input parameter.
 * - E_K_STATUS_ERROR for other errors.
 */
TKStatus ktacipherRemovePadding
(
  const uint8_t* xpData,
  size_t*        xpDataLength
);

#ifdef __cplusplus
}
#endif /* C++ */

#endif // KTACIPHER_H
/* -------------------------------------------------------------------------- */
/* END OF FILE                                                                */
/* -------------------------------------------------------------------------- */
