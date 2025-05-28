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
/** \brief Communication stack public interface.
 *
 *  \author Kudelski IoT
 *
 *  \date 2023/06/12
 *
 *  \file comm_if.h
 ******************************************************************************/
/**
 * @brief Communication stack public interface.
 */

#ifndef COMM_IF_H
#define COMM_IF_H

#ifdef __cplusplus
extern "C" {
#endif /* C++ */

/* -------------------------------------------------------------------------- */
/* IMPORTS                                                                    */
/* -------------------------------------------------------------------------- */
#include <stdint.h>
#include <stddef.h>

/* -------------------------------------------------------------------------- */
/* CONSTANTS, TYPES, ENUM                                                     */
/* -------------------------------------------------------------------------- */

/**
 * @brief keySTREAM server details for HTTP.
 */

/** @brief Server host url. */
#define C_K_COMM__SERVER_HOST C_KTA_APP__KEYSTREAM_HOST_COAP_URL

/** @brief Server port. */
#define C_K_COMM__SERVER_PORT (38292u)

/** @brief Server mount path. */
#define C_K_COMM__SERVER_URI "/lp1"

/** @brief Communication interface return status codes. */
typedef enum
{
  E_COMM_IF_STATUS_OK,
  /* Status OK, everything is fine. */
  E_COMM_IF_STATUS_PARAMETER,
  /* Bad parameter. */
  E_COMM_IF_STATUS_ERROR,
  /* On error cases. */
  E_COMM_IF_STATUS_DATA,
  /* Inconsistent or unsupported data. */
  E_COMM_IF_STATUS_TIMEOUT,
  /* A timeout occurred. */
  E_COMM_IF_STATUS_NETWORK,
  /* The network connectivity is not available. */
  E_COMM_IF_STATUS_MEMORY,
  /* Failure on memory allocation. */
  E_COMM_IF_NUM_STATUS
  /* Number of status values. */
} TCommIfStatus;

/** @brief Supported IP protocols. */
typedef enum
{
  E_COMM_IF_IP_PROTOCOL_V4,
  /* IP V4. */
  E_COMM_IF_IP_PROTOCOL_V6,
  /* IP V6. */
  E_COMM_IF_IP_NUM_PROTOCOLS
  /* Number of supported IP protocols. */
} TCommIfIpProtocol;

/* -------------------------------------------------------------------------- */
/* VARIABLES                                                                  */
/* -------------------------------------------------------------------------- */

/* -------------------------------------------------------------------------- */
/* FUNCTIONS                                                                  */
/* -------------------------------------------------------------------------- */

/**
 * @brief
 *   Initialize communication stack.
 *
 * @param[in] xpHost
 *   IP address of keySTREAM, string encoded.
 *   Should not be NULL.
 * @param[in] xPort
 *   port to connect to keySTREAM, string encoded.
 *   Should not be NULL.
 * @param[in] xpPath
 *   Server mount path;
 *   Should not be NULL.
 *
 * @return
 * - E_COMM_IF_STATUS_OK in case of success.
 * - E_COMM_IF_STATUS_PARAMETER for wrong input parameter(s).
 * - E_COMM_IF_STATUS_NETWORK if any network related issue.
 */
TCommIfStatus commInit
(
  const uint8_t*  xpHost,
  const uint16_t  xPort,
  const uint8_t*  xpPath
);

/**
 * @brief
 *   Sends message to server and return the response received from server.
 *
 * @param[in] xpMsgToSend
 *   Message to send to keySTREAM.
 *   Should not be NULL.
 * @param[in] xSendSize
 *   Size of the message, in bytes.
 * @param[in,out] xpRecvMsgBuffer
 *   [in] Pointer to recevie data buffer from keySTREAM.
 *   [out] Actual data received from keySTREAM.
 *   Should not be NULL.
 * @param[in,out] xpRecvMsgBufferSize
 *   [in] Pointer to length for recevied data from keySTREAM.
 *   [out] Actual data length received from keySTREAM.
 *   Should not be NULL.
 *
 * @return
 * - E_COMM_IF_STATUS_OK in case of success.
 * - E_COMM_IF_STATUS_PARAMETER for wrong input parameter(s).
 * - E_COMM_IF_STATUS_NETWORK if any network issue.
 */
TCommIfStatus commMsgExchange
(
  const uint8_t*  xpMsgToSend,
  const size_t    xSendSize,
  uint8_t*        xpRecvMsgBuffer,
  size_t*         xpRecvMsgBufferSize
);

/**
 * @brief
 *   Terminate communication stack.
 *
 * @return
 * - E_COMM_IF_STATUS_OK in case of success.
 * - E_COMM_IF_STATUS_ERROR if communication stack not initialized.
 */
TCommIfStatus commTerm
(
  void
);

#ifdef __cplusplus
}
#endif /* C++ */

#endif // COMM_IF_H

/* -------------------------------------------------------------------------- */
/* END OF FILE                                                                */
/* -------------------------------------------------------------------------- */
