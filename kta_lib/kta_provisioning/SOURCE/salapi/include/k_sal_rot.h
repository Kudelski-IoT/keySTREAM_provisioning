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
/** \brief  Interface for RoT operation.
*
* \author Kudelski IoT
*
* \date    2023/06/02
*
* \file k_sal_rot.h
*
******************************************************************************/

/**
 * @brief Interface for RoT operation.
 */

#ifndef K_SAL_ROT_H
#define K_SAL_ROT_H

#ifdef __cplusplus
extern "C" {
#endif /* C++ */

/** @defgroup g_sal_api SAL Interface */

/** @addtogroup g_sal_api
 * @{
*/

/* -------------------------------------------------------------------------- */
/* IMPORTS                                                                    */
/* -------------------------------------------------------------------------- */
#include "k_defs.h"
#include "k_sal_crypto.h"

#include <stdint.h>

/* -------------------------------------------------------------------------- */
/* CONSTANTS, TYPES, ENUM                                                     */
/* -------------------------------------------------------------------------- */

/* -------------------------------------------------------------------------- */
/* VARIABLES                                                                  */
/* -------------------------------------------------------------------------- */

/* -------------------------------------------------------------------------- */
/* FUNCTIONS                                                                  */
/* -------------------------------------------------------------------------- */

/**
 * @brief
 *   Generates a key pair based on ECC NIST P256. The public key is always exported to Host.
 *   The private key is always written to C_K_KTA__VOLATILE_ID key identifier.
 *
 * @param[out] xpPublicKey
 *   Generated public key: pointer to buffer (always exported in clear).
 *   Should not be NULL.
 *   Length is fixed to 64-Bytes(C_K_KTA__PUBLIC_KEY_MAX_SIZE);
 *
 * @return
 * - E_K_STATUS_OK in case of success.
 * - E_K_STATUS_PARAMETER for wrong input parameter.
 * - E_K_STATUS_ERROR for other errors.
 */
K_SAL_API TKStatus salRotKeyPairGeneration
(
  uint8_t*  xpPublicKey
);

/**
 * @brief
 *   Compute a shared secret based ECDH NIST P256.
 *   The private key is always located inside the secure platform and addressed by an identifier.
 *   The public key is always provided through a Host buffer. The computed shared secret is either
 *   exported to Host or kept inside the secure platform in key identifier(coherent with
 *   salRotHkdfExtractAndExpand () "secret" input).
 *
 * @param[in] xPrivateKeyId
 *   Local private key: identifier.
 *   The only valid values are C_K_KTA__CHIP_SK_ID and C_K_KTA__VOLATILE_ID.
 * @param[in] xpPeerPublicKey
 *   Peer public key: pointer to buffer. Should not be NULL.
 *   Length is fixed to 64-Bytes(ECC NIST P256 Public Key format with
 *   both coordinates x||y)(C_K_KTA__PUBLIC_KEY_MAX_SIZE).
 * @param[in] xSharedSecretTarget
 *   bit 31 = 1: Computed shared secret is exported to Host buffer.
 *   bit 31 = 0: Computed shared secret is written to provided key identifier.
 *   The only valid value is C_K_KTA__VOLATILE_2_ID (content: 32-Bytes).
 *   Key identifiers are encoded on 16-bits, cf. "Object ID Management".
 * @param[out] xpSharedSecret
 *   Computed shared secret: pointer buffer.
 *   Only applies if xSharedSecretTarget = 0.
 *   Length is fixed to 32-Bytes(C_K_KTA__SHARED_SECRET_KEY_MAX_SIZE).
 *
 * @return
 * - E_K_STATUS_OK in case of success.
 * - E_K_STATUS_PARAMETER for wrong input parameter.
 * - E_K_STATUS_ERROR for other errors.
 */
K_SAL_API TKStatus salRotKeyAgreement
(
  uint32_t        xPrivateKeyId,
  const uint8_t*  xpPeerPublicKey,
  uint32_t        xSharedSecretTarget,
  uint8_t*        xpSharedSecret
);

/**
 * @brief
 *   Derive a key based on HKDF HMAC-SHA256 (both Extract and Expand phases).
 *   The secret is either provided through a Host buffer or located inside the secure platform
 *   in key identifier (coherent with salRotKeyAgreement() "shared secret" output).
 *   The derived key is always kept inside the secure platform.
 *
 * @param[in] xMode
 *   mode = 0: "Activation" message use case(C_K_KTA__HKDF_ACT_MODE).
 *    Secret is located in C_K_KTA__VOLATILE_2_ID key identifier (content: 32-Bytes).
 *    Salt length = 64-Bytes(C_K_KTA__HKDF_ACT_SALT_MAX_SIZE).
 *    Derived key is written to C_K_KTA__VOLATILE_2_ID.
 *   mode = 1: "Generic" message use case(C_K_KTA__HKDF_GEN_MODE).
 *    Secret is provided through Host buffer. Length fixed to 64-Bytes.
 *    Salt length = 16-Bytes(C_K_KTA__HKDF_GEN_SALT_MAX_SIZE).
 *    Derived key is written to C_K_KTA__L1_FIELD_KEY_ID.
 * @param[in]   xpSecret
 *   Secret for HKDF-extraction phase: pointer to buffer. Only applies if mode=1.
 * @param[in]   xpSalt
 *   Salt for HKDF-extraction phase: pointer to buffer; Should not be NULL.
 *   Length is fixed to 64/16 Bytes based on mode.
 *   Should not be NULL.
 * @param[in]   xpInfo
 *   Info for HKDF-expansion phase: pointer to buffer; Should not be NULL.
 * @param[in]   xInfoLen
 *   Length of xpInfo.
 *
 * @return
 * - E_K_STATUS_OK in case of success.
 * - E_K_STATUS_PARAMETER for wrong input parameter.
 * - E_K_STATUS_ERROR for other errors.
 */
K_SAL_API TKStatus salRotHkdfExtractAndExpand
(
  uint32_t        xMode,
  const uint8_t*  xpSecret,
  const uint8_t*  xpSalt,
  const uint8_t*  xpInfo,
  size_t          xInfoLen
);

/**
 * @brief
 *   Derive a key based on HMAC SHA256 and truncate the derived key (128 most significant bits).
 *   The key is always located inside the secure platform and addressed by an identifier.
 *   The derived key is always kept inside secure platform and addressed by an identifier.
 *
 * @param[in] xKeyId
 *   Key identifier
 *   The only valid values are C_K_KTA__VOLATILE_2_ID and C_K_KTA__L1_FIELD_KEY_ID.
 * @param[in] xpInputData
 *   Input data pointer to buffer; Should not be NULL.
 * @param[in] xInputDataLen
 *   Length of xpInputData.
 * @param[in] xDerivedKeyId
 *   Derived key identifier.
 *   The only valid values are C_K_KTA__VOLATILE_2_ID and C_K_KTA__VOLATILE_3_ID.
 *
 * @return
 * - E_K_STATUS_OK in case of success.
 * - E_K_STATUS_PARAMETER for wrong input parameter.
 * - E_K_STATUS_ERROR for other errors.
 */
K_SAL_API TKStatus salRotKeyDerivation
(
  uint32_t        xKeyId,
  const uint8_t*  xpInputData,
  size_t          xInputDataLen,
  uint32_t        xDerivedKeyId
);

/**
 * @brief
 *   Get chip UID. Chip Platform writes chip_uid which can have a chip specific format and length.
 *
 * @param[out] xpChipUid
 *   Address of buffer where the device platform will write the chip_uid.
 *   MAX = 32 Bytes(C_K_KTA__CHIPSET_UID_MAX_SIZE). Should not be NULL.
 * @param[in,out] xpChipUidLen
 *   [in] Length of xpChipUid buffer.
 *   [out] Length of filled output data.
 *   Should not be NULL.
 *
 * @return
 * - E_K_STATUS_OK in case of success.
 * - E_K_STATUS_PARAMETER for wrong input parameter.
 * - E_K_STATUS_ERROR for other errors.
 */
K_SAL_API TKStatus salRotGetChipUID
(
  uint8_t*  xpChipUid,
  size_t*   xpChipUidLen
);

/**
 * @brief
 *   Get chip UID. Chip Platform writes chip_uid which can have a chip specific format and length.
 *
 * @param[out] xpChipCert
 *   Address of buffer where the device platform will write the chip certificate.
 *   MAX = 256 Bytes(C_K_KTA__CHIP_CERT_MAX_SIZE). Should not be NULL.
 * @param[in,out] xpChipCertLen
 *   [in] Length of xpChipCert buffer.
 *   [out] Length of filled output data.
 *   Should not be NULL.
 *
 * @return
 * - E_K_STATUS_OK in case of success.
 * - E_K_STATUS_PARAMETER for wrong input parameter.
 * - E_K_STATUS_ERROR for other errors.
 */
K_SAL_API TKStatus salRotGetChipCertificate
(
  uint8_t*  xpChipCert,
  size_t*   xpChipCertLen
);

/** @} g_sal_api */

#ifdef __cplusplus
}
#endif /* C++ */

#endif // K_SAL_ROT_H

/* -------------------------------------------------------------------------- */
/* END OF FILE                                                                */
/* -------------------------------------------------------------------------- */

