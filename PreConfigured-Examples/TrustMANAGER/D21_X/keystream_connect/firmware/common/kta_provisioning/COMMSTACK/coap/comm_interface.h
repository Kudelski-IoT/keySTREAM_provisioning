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
/** \brief  Communication stack public interface.
*
*  \author Kudelski IoT
*
*  \date 2023/06/12
*
*  \file comm_interface.h
******************************************************************************/
/**
 * @brief Communication interface.
 */

#ifndef COMM_INTERFACE_H
#define COMM_INTERFACE_H

#ifdef __cplusplus
extern "C" {
#endif /* C++ */

/* -------------------------------------------------------------------------- */
/* IMPORTS                                                                    */
/* -------------------------------------------------------------------------- */
#include "comm_if.h"

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
 *   Initialize Communication stack.
 *
 * @param[in] xpUri
 *   keySTREAM server Uri, string encoded.
 *   Should not be NULL.
 * @param[in] xUriLength
 *   Length of the coap keySTREAM server uri, in bytes.
 *   Should not be NULL.
 * @param[in] xpHost
 *   keySTREAM server IP address, string encoded.
 *   Should not be NULL.
 * @param[in] xPort
 *   Port to connect to keySTREAM.
 *
 * @return
 * - E_COMM_IF_STATUS_OK in case of success.
 * - E_COMM_IF_STATUS_PARAMETER for wrong input parameter(s).
 * - E_COMM_IF_STATUS_NETWORK if any network issue.
 */
TCommIfStatus commInitProtocol
(
  const TCommIfIpProtocol xIpProtocol,
  const uint8_t*        xpUri,
  const uint8_t*        xpHost,
  const uint16_t        xPort
);

/**
 * @brief
 *   Sends the message to keySTREAM and receives response in return.
 *
 * @param[in]  xpMessageToSend
 *   Message to send to server.
 *   Should not be NULL.
 * @param[in]   xSendSize
 *   Size of the message, in bytes.
 * @param[in,out] xpReceiveMsgBuffer
 *   [in] Pointer to buffer receiving data from keySTREAM.
 *   [out] Actual data received from keySTREAM.
 *   Should not be NULL.
 * @param[in,out] xpReceiveMsgBufferLength
 *   [in] Pointer to length of buffer receiving data from keySTREAM.
 *   [out] Actual length of data received from keySTREAM.
 *   Should not be NULL.
 *
 * @return
 * - E_COMM_IF_STATUS_OK in case of success.
 * - E_COMM_IF_STATUS_PARAMETER for wrong input parameter(s).
 * - E_COMM_IF_STATUS_NETWORK if any network issue.
 */
TCommIfStatus commMessageExchange
(
  const  uint8_t*   xpMessageToSend,
  const  size_t     xSendSize,
  uint8_t*          xpReceiveMsgBuffer,
  size_t*           xpReceiveMsgBufferLength
);

/**
 * @brief
 *   Terminate Communication stack.
 *
 * @return
 * - E_COMM_IF_STATUS_OK in case of success.
 * - E_COMM_IF_STATUS_ERROR if communication stack not initialized.
 */
TCommIfStatus commTerminateProtocol
(
  void
);

#ifdef __cplusplus
}
#endif /* C++ */

#endif // COMM_INTERFACE_H

/* -------------------------------------------------------------------------- */
/* END OF FILE                                                                */
/* -------------------------------------------------------------------------- */
