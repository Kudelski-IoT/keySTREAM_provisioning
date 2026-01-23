/*******************************************************************************
*************************keySTREAM Trusted Agent ("KTA")************************

* (c) 2023-2025 Nagravision Sàrl

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
/** \brief HTTP Public Interface.
 *
 *  \author Kudelski IoT
 *
 *  \date 2023/06/02
 *
 *  \file http_if.h
 *
 ******************************************************************************/
/**
 * @brief HTTP Public Interface.
 */

#ifndef HTTP_IF_H
#define HTTP_IF_H

/* --------------------------------------------------------------------------------------------- */
/* IMPORTS                                                                                       */
/* --------------------------------------------------------------------------------------------- */

#include "comm_if.h"

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
 *   Initialize Http module.
 * @param[in] xpUri
 *   Coap server uri; should not be NULL. It must have '\0' character at the end.
 * @param[in] xpHost
 *   IP address of server; should not be NULL. It must have '\0' character at the end.
 * @param[in] xPort
 *   Port of the server listening to client requests.
 *
 * @return
 * - E_COMM_IF_STATUS_OK or the error status, in particular.
 * - E_COMM_IF_STATUS_PARAMETER if wrong parameters received.
 * - E_COMM_IF_STATUS_NETWORK if any network issue.
 */
TCommIfStatus httpInit
(
  const TCommIfIpProtocol  xIpProtocol,
  const uint8_t*           xpUri,
  const uint8_t*           xpHost,
  const uint16_t           xPort
);

/**
 * @brief
 *   Sends message to server and return the response received from server.
 *
 * @param[in] xpMessageToSend
 *   Message to send to server; should not be NULL.
 * @param[in] xSendSize
 *   Size of the message, in bytes.
 * @param[out] xpReceiveMsgBuffer
 *   Data buffer to fill; must point to *xpReceiveMsgBufferLength bytes.
 *   Should not be NULL.
 * @param[in,out] xpReceiveMsgBufferLength
 *   [in] Size of the data buffer, in bytes.
 *   [out] Size of the received data, in bytes; this value may be larger than
 *         the input value, but only the first bytes are copied to xpReceiveMsgBuffer.
 *
 * @return
 * - E_COMM_IF_STATUS_OK or the error status, in particular.
 * - E_COMM_IF_STATUS_PARAMETER if wrong parameters received.
 * - E_COMM_IF_STATUS_NETWORK if any network issue.
 */
TCommIfStatus httpMsgExchange
(
  const  uint8_t*  xpMsgToSend,
  const  size_t    xSendSize,
  uint8_t*         xpRecvMsgBuffer,
  size_t*          xpRecvMsgBufferSize
);

/**
 * @brief
 *   Terminate Http Module.
 *
 * @return
 * - E_COMM_IF_STATUS_OK or the error status, in particular.
 */
TCommIfStatus httpTerm
(
  void
);

#endif // HTTP_IF_H
/* -------------------------------------------------------------------------- */
/* END OF FILE                                                                */
/* -------------------------------------------------------------------------- */

