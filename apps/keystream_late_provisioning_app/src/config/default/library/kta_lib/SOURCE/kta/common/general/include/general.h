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
/** \brief keySTREAM Trusted Agent - General module to perform coding of data.
 *
 *  \author Kudelski IoT
 *
 *  \date 2023/06/12
 *
 *  \file general.h
 ******************************************************************************/

/**
 * @brief keySTREAM Trusted Agent - General module to perform coding of data.
 */

#ifndef GENERAL_H
#define GENERAL_H

#ifdef __cplusplus
extern "C" {
#endif /* C++ */

/* -------------------------------------------------------------------------- */
/* IMPORTS                                                                    */
/* -------------------------------------------------------------------------- */
#include "k_defs.h"
#include "icpp_parser.h"

#include <stdint.h>
#include <stddef.h>

/* -------------------------------------------------------------------------- */
/* CONSTANTS, TYPES, ENUM                                                     */
/* -------------------------------------------------------------------------- */
/** @brief Serialize the data. */
#define C_GEN__SERIALIZE (1u)

/** @brief Add padding to the data. */
#define C_GEN__PADDING   (2u)

/** @brief Encrypt the data. */
#define C_GEN__ENCRYPT   (4u)

/** @brief Sign the data. */
#define C_GEN__SIGNING   (8u)

/* -------------------------------------------------------------------------- */
/* VARIABLES                                                                  */
/* -------------------------------------------------------------------------- */

/* -------------------------------------------------------------------------- */
/* FUNCTIONS                                                                  */
/* -------------------------------------------------------------------------- */
/**
 * @brief
 *   To perform coding on the data, based on user's input.
 *   1. Seralize.
 *   2. Padding.
 *   3. Encrypt.
 *   4. Sign.
 *
 * @param[in] xTotalCodedSteps
 *   Coding level.
 * @param[in] xpSendProtoMessage
 *   Non serialized message to serialize.
 * @param[in,out] xpMessageToSend
 *   [in] Pointer to carry the serialized data buffer.
 *   [out] Actual serialized data.
 * @param [in,out] xpMessageToSendSize
 *   [in] Pointer to carry the serialized data buffer length.
 *   [out] Actual serialized data legth.
 *
 * @return
 * - E_K_STATUS_OK in case of success.
 * - E_K_STATUS_PARAMETER for wrong input parameter.
 * - E_K_STATUS_ERROR for other errors.
 */
TKStatus ktaGenerateResponse
(
  uint8_t                      xTotalCodedSteps,
  const TKIcppProtocolMessage* xpSendProtoMessage,
  uint8_t*                     xpMessageToSend,
  size_t*                      xpMessageToSendSize
);

#ifdef __cplusplus
}
#endif /* C++ */

#endif // GENERAL_H
/* -------------------------------------------------------------------------- */
/* END OF FILE                                                                */
/* -------------------------------------------------------------------------- */
