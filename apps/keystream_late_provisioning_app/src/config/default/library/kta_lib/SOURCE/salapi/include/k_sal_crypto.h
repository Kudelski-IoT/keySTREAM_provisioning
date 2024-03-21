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
/** \brief  Interface for crypto operation.
 *
 *  \author Kudelski IoT
 *
 *  \date 2023/06/12
 *
 *  \file k_sal_crypto.h
 ******************************************************************************/

/**
 * @brief Interface for crypto operation.
 */

#ifndef K_SAL_CRYPTO_H
#define K_SAL_CRYPTO_H

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
#include "k_sal.h"

#include <stdint.h>
#include <stddef.h>

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
 *   Generic function to compute HMAC SHA256.
 *   The key is always located inside the secure platform and addressed by an identifier.
 *   The computed MAC is always exported to Host.
 *
 * @param[in] xKeyId
 *   Key identifier.
 * @param[in] xpInputData
 *   Input data pointer to buffer; Should not be NULL.
 * @param[in] xInputDataLen
 *   Length of xpInputData.
 * @param[out] xpMac
 *   Computed MAC: pointer to buffer. Should not be NULL.
 *   Length is fixed to 16-Bytes(C_K_KTA__HMAC_MAX_SIZE),
 *   truncated to keep the 16 most significant bytes.
 *
 * @return
 * - E_K_STATUS_OK in case of success.
 * - E_K_STATUS_PARAMETER for wrong input parameter.
 * - E_K_STATUS_ERROR for other errors.
 */
K_SAL_API TKStatus salCryptoHmac
(
  uint32_t        xKeyId,
  const uint8_t*  xpInputData,
  size_t          xInputDataLen,
  uint8_t*        xpMac
);

/**
 * @brief
 *   Generic function to compute HMAC SHA256 and compare with the provided mac reference.
 *   The key is always located inside the secure platform and addressed by an identifier.
 *
 * @param[in] xKeyId
 *   Key identifier.
 * @param[in] xpInputData
 *   Input data pointer to buffer; Should not be NULL.
 * @param[in] xInputDataLen
 *   Length of xpInputData.
 * @param[in] xpMac
 *   Length is fixed to 16-Bytes(C_K_KTA__HMAC_MAX_SIZE),
 *   truncated to keep the 16 most significant bytes.
 *
 * @return
 * - E_K_STATUS_OK in case of success.
 * - E_K_STATUS_PARAMETER for wrong input parameter.
 * - E_K_STATUS_ERROR for other errors.
 */
K_SAL_API TKStatus salCryptoHmacVerify
(
  uint32_t        xKeyId,
  const uint8_t*  xpInputData,
  size_t          xInputDataLen,
  const uint8_t*  xpMac
);

/**
 * @brief
 *   Generic function to encrypt data based on AES-128 CBC.
 *   The key is always located inside the secure platform and addressed by an identifier.
 *
 * @param[in] xKeyId
 *   Key identifier.
 * @param[in] xpInputData
 *   Plain input data buffer pointer; Should not be NULL.
 * @param[in] xInputDataLen
 *   Length of xpInputData.
 * @param[out] xpOutputData
 *   Encrypted output data buffer pointer. Should not be NULL.
 * @param[in,out] xpOutputDataLen
 *   [in] Length of xpOutputData buffer.
 *   [out] Length of filled output data.
 *   Should not be NULL.
 *
 * @return
 * - E_K_STATUS_OK in case of success.
 * - E_K_STATUS_PARAMETER for wrong input parameter.
 * - E_K_STATUS_ERROR for other errors.
 */
K_SAL_API TKStatus salCryptoAesEnc
(
  uint32_t        xKeyId,
  const uint8_t*  xpInputData,
  size_t          xInputDataLen,
  uint8_t*        xpOutputData,
  size_t*         xpOutputDataLen
);

/**
 * @brief
 *   Generic function to decrypt data based on AES-128 CBC.
 *   The key is always located inside the secure platform and addressed by an identifier.
 *
 * @param[in] xKeyId
 *   Key identifier.
 * @param[in] xpInputData
 *   Encrypted input data buffer; Should not be NULL.
 * @param[in] xInputDataLen
 *   Length of xpInputData.
 * @param[out] xpOutputData
 *   Plain output data buffer. Should not be NULL.
 * @param[in,out] xpOutputDataLen
 *   [in] Length of xpOutputData buffer.
 *   [out] Length of filled output data.
 *   Should not be NULL.
 *
 * @return
 * - E_K_STATUS_OK in case of success.
 * - E_K_STATUS_PARAMETER for wrong input parameter.
 * - E_K_STATUS_ERROR for other errors.
 */
K_SAL_API TKStatus salCryptoAesDec
(
  uint32_t        xKeyId,
  const uint8_t*  xpInputData,
  size_t          xInputDataLen,
  uint8_t*        xpOutputData,
  size_t*         xpOutputDataLen
);

/**
 * @brief
 *   Get random value.
 *
 * @param[out] xpRandomData
 *   Address of buffer where the device platform will write the random data.
 *   This buffer must have a minimal size of xpRandomDataLen. Should not be NULL.
 *   MAX = 256 Bytes(C_K_KTA__RANDOM_MAX_SIZE).
 * @param[in,out] xpRandomDataLen
 *   [in] Length of xpRandomData buffer.
 *   [out] Length of filled output data.
 *   Should not be NULL.
 *
 * @return
 * - E_K_STATUS_OK in case of success.
 * - E_K_STATUS_PARAMETER for wrong input parameter.
 * - E_K_STATUS_ERROR for other errors.
 */
K_SAL_API TKStatus salCryptoGetRandom
(
  uint8_t*  xpRandomData,
  size_t*   xpRandomDataLen
);

/**
 * @brief
 *   Generic function to sign the hash data.
 *   The key is always located inside the secure platform and addressed by an identifier.
 *
 * @param[in] xKeyId
 *   Key identifier.
 * @param[in] xpMsgToHash
 *   Hash To Sign buffer; Should not be NULL.
 * @param[in] xMsgToHashLen
 *   Length of xpMsgToHash.
 * @param[out] xpSignedHashOutBuff
 *   SignHash output data buffer. Should not be NULL.
 * @param[in] xSignedHashOutBuffLen
 *   SignHash output data length. Should not be NULL.
 * @param[out] xpActualSignedHashOutLen
 *   Length of filled xpSignedHashOutBuff data. Should not be NULL.
 *
 * @return
 * - E_K_STATUS_OK in case of success.
 * - E_K_STATUS_ERROR for other errors.
 */
TKStatus salSignHash
(
  uint32_t  xKeyId,
  uint8_t*  xpMsgToHash,
  size_t    xMsgToHashLen,
  uint8_t*  xpSignedHashOutBuff,
  uint32_t  xSignedHashOutBuffLen,
  size_t*   xpActualSignedHashOutLen
);

/** @} g_sal_api */
#ifdef __cplusplus
}
#endif /* C++ */

#endif // K_SAL_CRYPTO_H

/* -------------------------------------------------------------------------- */
/* END OF FILE                                                                */
/* -------------------------------------------------------------------------- */

