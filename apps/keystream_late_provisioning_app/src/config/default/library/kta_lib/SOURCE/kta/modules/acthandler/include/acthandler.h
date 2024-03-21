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
/** \brief keySTREAM Trusted Agent - Activation handler.
 *
 *  \author Kudelski IoT
 *
 *  \date 2023/06/12
 *
 *  \file acthandler.h
 ******************************************************************************/

/**
 * @brief keySTREAM Trusted Agent - Activation handler.
 */

#ifndef ACTHANDLER_H
#define ACTHANDLER_H

#ifdef __cplusplus
extern "C" {
#endif /* C++ */

/* --------------------------------------------------------------------------------------------- */
/* IMPORTS                                                                                       */
/* --------------------------------------------------------------------------------------------- */
#include "k_defs.h"
#include "icpp_parser.h"

#include <string.h>
#include <stdio.h>
#include <stddef.h>
#include <stdint.h>
/* --------------------------------------------------------------------------------------------- */
/* CONSTANTS, TYPES, ENUM                                                                        */
/* --------------------------------------------------------------------------------------------- */
/** @brief HMAC SHA256 size in bytes. */
#define C_KTA_ACT__HMACSHA256_SIZE (16u)

/* --------------------------------------------------------------------------------------------- */
/* VARIABLES                                                                                     */
/* --------------------------------------------------------------------------------------------- */

/* --------------------------------------------------------------------------------------------- */
/* FUNCTIONS                                                                                     */
/* --------------------------------------------------------------------------------------------- */
/**
 * @brief
 *   Derive L2 Keys.
 *
 * @return
 * - E_K_STATUS_OK in case of success.
 * - E_K_STATUS_PARAMETER for wrong input parameter.
 * - E_K_STATUS_ERROR for other errors.
 */
TKStatus ktaActDeriveL2Keys
(
  void
);

/**
 * @brief
 *   Build activation request.
 *
 * @param[out] xpMessageToSend
 *   Should not be NULL.
 *   [in] Pointer to the buffer carrying activation request message.
 *   [out] Buffer filled with activation request message.
 * @param[in,out] xpMessageToSendSize
 *   Should not be NULL.
 *   [in] Buffer holds the size of input xpMessageToSend buffer.
 *   [out] Actual activation request message size.
 *
 * @return
 * - E_K_STATUS_OK in case of success.
 * - E_K_STATUS_PARAMETER for wrong input parameter.
 * - E_K_STATUS_ERROR for other errors.
 */
TKStatus ktaActBuildActivationRequest
(
  uint8_t* xpMessageToSend,
  size_t*  xpMessageToSendSize
);

/**
 * @brief
 *   Build L1 field keys from the activation response.
 *
 * @param[in] xpRecvMsg
 *   Should not be NULL.
 *   Buffer with icpp Message.
 *
 * @return
 * - E_K_STATUS_OK in case of success.
 * - E_K_STATUS_PARAMETER for wrong input parameter.
 * - E_K_STATUS_ERROR for other errors.
 */
TKStatus ktaActResponseBuildL1Keys
(
  TKIcppProtocolMessage* xpRecvMsg
);

#ifdef __cplusplus
}
#endif /* C++ */

#endif // ACTHANDLER_H

/* -------------------------------------------------------------------------- */
/* END OF FILE                                                                */
/* -------------------------------------------------------------------------- */
