/*******************************************************************************
*************************keySTREAM Trusted Agent ("KTA")************************

* (c) 2023-2026 Nagravision Sàrl

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
#include <stdbool.h>

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
  E_COMM_IF_STATUS_NO_CONNECTION,
  /* Not connected - call commInit first. [NEW] */
  E_COMM_IF_STATUS_STALE,
  /* Connection is too old/stale. [NEW] */
  E_COMM_IF_STATUS_RESOURCE,
  /* Resource unavailable (e.g., socket limit). [NEW] */
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

/** @brief Communication statistics structure [NEW] */
typedef struct
{
  uint32_t      connectAttempts;
  /* Total number of connection attempts. */
  uint32_t      connectSuccess;
  /* Number of successful connections. */
  uint32_t      connectFailures;
  /* Number of failed connections. */
  uint32_t      msgExchangeAttempts;
  /* Total number of message exchange attempts. */
  uint32_t      msgExchangeSuccess;
  /* Number of successful message exchanges. */
  uint32_t      msgExchangeFailures;
  /* Number of failed message exchanges. */
  uint32_t      termCalls;
  /* Number of times commTerm() was called. */
  TCommIfStatus lastError;
  /* Last error code encountered. */
  bool          isConnected;
  /* Current connection state. */
  uint32_t      connectionAgeSeconds;
  /* Age of current connection in seconds (0 if not connected). */
} TCommIfStatistics;

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
 *   IP address of server; should not be NULL. It must have '\0' character at the end.
 * @param[in] xPort
 *   Port of the server listening to client requests.
 * @param[in] xpPath
 *   Server path; should not be NULL. It must have '\0' character at the end.
 *
 * @return
 * - E_COMM_IF_STATUS_OK or the error status, in particular.
 * - E_COMM_IF_STATUS_PARAMETER if wrong parameters received.
 * - E_COMM_IF_STATUS_NETWORK if any network issue.
 */
TCommIfStatus commInit
(
  const uint8_t* xpHost,
  const uint16_t xPort,
  const uint8_t* xpPath
);

/**
 * @brief
 *   Sends message to server and return the response received from server.
 *
 * @param[in] xpMsgToSend
 *   Message to send to server; should not be NULL.
 * @param[in] xSendSize
 *   size of the message, in bytes.
 * @param[out] xpRecvMsgBuffer
 *   Data buffer to fill; must point to *xpRecvMsgBufferSize bytes.
 *   Should not be NULL.
 * @param[in,out] xpRecvMsgBufferSize
 *   [in] Size of the data buffer, in bytes.
 *   [out] Size of the received data, in bytes; this value may be larger than
 *        the input value, but only the first bytes are copied to xpRecvMsgBuffer.
 *
 * @return
 * - E_COMM_IF_STATUS_OK or the error status, in particular:
 * - E_COMM_IF_STATUS_PARAMETER if wrong parameters received.
 * - E_COMM_IF_STATUS_NO_CONNECTION if not connected (call commInit first).
 * - E_COMM_IF_STATUS_NETWORK if any network issue.
 * - E_COMM_IF_STATUS_TIMEOUT if operation timed out.
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
 * - E_COMM_IF_STATUS_OK or the error status, in particular.
 * - E_COMM_IF_STATUS_ERROR if communication stack not initialized.
 */
TCommIfStatus commTerm
(
  void
);

/**
 * @brief
 *   Get communication statistics. [NEW]
 *
 * @param[out] xpStats
 *   Pointer to statistics structure to fill; should not be NULL.
 *
 * @return
 * - E_COMM_IF_STATUS_OK on success.
 * - E_COMM_IF_STATUS_PARAMETER if xpStats is NULL.
 */
TCommIfStatus commGetStatistics
(
  TCommIfStatistics* xpStats
);

/**
 * @brief
 *   Reset communication statistics counters. [NEW]
 *   Note: Does not affect connection state.
 */
void commResetStatistics
(
  void
);

/**
 * @brief
 *   Check connection health status. [NEW]
 *
 * @return
 * - E_COMM_IF_STATUS_OK if connection is healthy.
 * - E_COMM_IF_STATUS_NO_CONNECTION if not connected.
 * - E_COMM_IF_STATUS_STALE if connection is too old (>5 minutes).
 */
TCommIfStatus commCheckConnectionHealth
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
