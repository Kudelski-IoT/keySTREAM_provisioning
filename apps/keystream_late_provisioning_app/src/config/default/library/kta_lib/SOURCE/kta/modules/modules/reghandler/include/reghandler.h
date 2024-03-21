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
/** \brief keySTREAM Trusted Agent - Registration module interface.
 *
 *  \author Kudelski IoT
 *
 *  \date 2023/06/12
 *
 *  \file reghandler.h
 ******************************************************************************/

/**
 * @brief keySTREAM Trusted Agent - Registration module interface.
 */

#ifndef KTAREG_H
#define KTAREG_H

#ifdef __cplusplus
extern "C" {
#endif /* C++ */

/* --------------------------------------------------------------------------------------------- */
/* IMPORTS                                                                                       */
/* --------------------------------------------------------------------------------------------- */
#include "k_defs.h"
#include "icpp_parser.h"

#include <stddef.h>
#include <stdint.h>

/* --------------------------------------------------------------------------------------------- */
/* CONSTANTS, TYPES, ENUM                                                                        */
/* --------------------------------------------------------------------------------------------- */

/* --------------------------------------------------------------------------------------------- */
/* VARIABLES                                                                                     */
/* --------------------------------------------------------------------------------------------- */

/* --------------------------------------------------------------------------------------------- */
/* FUNCTIONS                                                                                     */
/* --------------------------------------------------------------------------------------------- */
/**
 * @brief
 *   Build registration request.
 *
 * @param[in]  xpRecvdProtoMessage
 *   Should not be NULL.
 *   Message received from application.
 * @param[out]  xpMessageToSend
 *   Should not be NULL.
 *   Buffer should be provided by caller of at-least C_K__ICPP_MSG_MAX_SIZE size.
 *   Buffer will be filled with icpp Message to send to keySTREAM.
 * @param[in,out]  xpMessageToSendSize
 *   Should not be NULL.
 *   BufferSize will be filled with icpp Message size to be send to keySTREAM.
 *   If it is 0 then it means no data to send to server.
 *
 * @return
 * - E_K_STATUS_OK in case of success.
 * - E_K_STATUS_PARAMETER for wrong input parameters.
 * - E_K_STATUS_ERROR for other errors.
 */
TKStatus ktaregBuildRegistrationRequest
(
  TKIcppProtocolMessage* xpRecvdProtoMessage,
  uint8_t* xpMessageToSend,
  size_t*  xpMessageToSendSize
);

#ifdef __cplusplus
}
#endif /* C++ */

#endif // KTAREG_H
/* -------------------------------------------------------------------------- */
/* END OF FILE                                                                */
/* -------------------------------------------------------------------------- */
