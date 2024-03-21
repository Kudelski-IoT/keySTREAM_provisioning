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
 /** \brief  SAL - Interface for crypto operation.
 *
 * \author Kudelski IoT
 *
 * \date   2023/06/02
 *
 * \file   k_sal_crypto_ext.h
 *
 ******************************************************************************/

/**
 * @brief SAL interface for crypto operation. These APIs are only for internal testing and
 *        should not be exposed.
 */

#ifndef K_SAL_CRYPTO_EXT_H
#define K_SAL_CRYPTO_EXT_H

#ifdef __cplusplus
extern "C" {
#endif /* C++ */

/* -------------------------------------------------------------------------- */
/* IMPORTS                                                                    */
/* -------------------------------------------------------------------------- */
#include "k_defs.h"

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
 *   Set counter for generating key pair.
 *
 * @param[in]  xCounter
 *             Counter to generate key pair
 *
 * @return
 * - E_K_STATUS_OK in case of success.
 * - E_K_STATUS_PARAMETER for wrong input parameters.
 * - E_K_STATUS_ERROR for other errors.
 */
TKStatus salSetKeyStreamKey
(
  uint8_t  xCounter
);

/**
 * @brief
 *   Set counter for generating key pair.
 *
 * @param[in]  xCounter
 *             Counter to generate key pair
 *
 * @return
 * - E_K_STATUS_OK in case of success.
 * - E_K_STATUS_PARAMETER for wrong input parameters.
 * - E_K_STATUS_ERROR for other errors.
 */
TKStatus salSetRotKey
(
  uint8_t  xCounter
);

/**
 * @brief
 *   Set chipset Uid for test purpose
 *
 * @param[in]  xpChipUid
 *             Chipset Uid value.
 * @param[in]  xSize
 *             Chipset Uid size in bytes.
 *
 * @return
 * - E_K_STATUS_OK in case of success.
 * - E_K_STATUS_PARAMETER for wrong input parameters.
 * - E_K_STATUS_ERROR for other errors.
 */
TKStatus salSetChipUid
(
  uint8_t*  xpChipUid,
  size_t    xChipUidSize
);

/**
 * @brief
 *   Set chipset certificate for test purpose
 *
 * @param[in]  xpChipUid
 *             Chipset certificate value.
 * @param[in]  xChipCertSize
 *             Chipset certificate size in bytes.
 *
 * @return
 * - E_K_STATUS_OK in case of success.
 * - E_K_STATUS_PARAMETER for wrong input parameters.
 * - E_K_STATUS_ERROR for other errors.
 */
TKStatus salSetChipCert
(
  uint8_t*  xpChipCert,
  size_t    xChipCertSize
);

/**
 * @brief
 *   Provides generated L2 enc and auth keys.
 *
 * @param[in]  xpL2EncKey
 *             L2 enc key value.
 *             buffer should be allocated by caller.
 * @param[in,out]  xpL2EncKeySize
 *             L2 enc key size in bytes.
 *             Maximum size should be passed as 16-Bytes.
 * @param[in]  xpL2AuthKey
 *             L2 auth key value.
 *             buffer should be allocated by caller.
 * @param[in,out]  xpL2AuthKeySize
 *             L2 auth key size in bytes.
 *             Maximum size should be passed as 16-Bytes.
 * @return
 * - E_K_STATUS_OK in case of success.
 * - E_K_STATUS_PARAMETER for wrong input parameters.
 * - E_K_STATUS_ERROR for other errors.
 */
TKStatus salGetL2Keys
(
  uint8_t*  xpL2EncKey,
  size_t*   xpL2EncKeySize,
  uint8_t*  xpL2AuthKey,
  size_t*   xpL2AuthKeySize
);

/**
 * @brief
 *   Set the value using the ID.
 *
 * @param[in] xObjectId
 *   Key identifier.
 * @param[in] xpValue
 *   Key data buffer. Should not be NULL.
 * @param[in] xValueLen
 *   Length of the buffer xpValue.
 *
 * @return
 * - E_K_STATUS_OK in case of success.
 * - E_K_STATUS_ERROR for other errors.
 */
TKStatus setValueById
(
  uint32_t  xObjectId,
  uint8_t*  xpValue,
  uint8_t   xValueLen
);

void salCryptoUpdateObjIds
(
  void
);
#ifdef TEST_COVERAGE

void salCryptoResetGlobalStruct
(
  void
);
#endif

#ifdef __cplusplus
}
#endif /* C++ */

#endif // K_SAL_CRYPTO_EXT_H

/* -------------------------------------------------------------------------- */
/* END OF FILE                                                                */
/* -------------------------------------------------------------------------- */

