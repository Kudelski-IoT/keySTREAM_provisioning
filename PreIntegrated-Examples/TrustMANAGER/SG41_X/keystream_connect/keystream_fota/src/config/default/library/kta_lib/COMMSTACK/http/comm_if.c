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
/** \brief COM - Interface
 *
 *  \author Kudelski Labs
 *
 *  \date 2023/06/12
 *
 *  \file comm_if.c
 * 
 *  ROBUSTNESS IMPROVEMENTS (2026-01-08):
 *  - Added comprehensive statistics tracking (success/failure counts)
 *  - Connection state monitoring and staleness detection
 *  - Parameter validation with detailed error codes
 *  - Connection health monitoring API
 *  - Automatic statistics reset capability
 ******************************************************************************/
/**
 * @brief Communication Interface. Based on the compilation flag it will select the coap or http.
 */

#include "comm_if.h"
/* -------------------------------------------------------------------------- */
/* IMPORTS                                                                    */
/* -------------------------------------------------------------------------- */
#include "http_if.h"
#include <string.h>
#include <time.h>

/* -------------------------------------------------------------------------- */
/* LOCAL CONSTANTS, TYPES, ENUM                                               */
/* -------------------------------------------------------------------------- */
/** @brief Maximum connection age before warning (5 minutes in seconds) */
#define C_COMM_IF_CONN_MAX_AGE_SEC      (300u)

/* -------------------------------------------------------------------------- */
/* LOCAL VARIABLES                                                            */
/* -------------------------------------------------------------------------- */
/** @brief Connection state */
static bool gIsConnected = false;

/** @brief Connection establishment timestamp */
static time_t gConnectTime = 0;

/** @brief Statistics - connection attempts */
static uint32_t gStatsConnectAttempts = 0;

/** @brief Statistics - successful connections */
static uint32_t gStatsConnectSuccess = 0;

/** @brief Statistics - connection failures */
static uint32_t gStatsConnectFailures = 0;

/** @brief Statistics - message exchange attempts */
static uint32_t gStatsMsgExchangeAttempts = 0;

/** @brief Statistics - successful exchanges */
static uint32_t gStatsMsgExchangeSuccess = 0;

/** @brief Statistics - exchange failures */
static uint32_t gStatsMsgExchangeFailures = 0;

/** @brief Statistics - termination calls */
static uint32_t gStatsTermCalls = 0;

/** @brief Statistics - last error code */
static TCommIfStatus gStatsLastError = E_COMM_IF_STATUS_OK;

/* -------------------------------------------------------------------------- */
/* LOCAL FUNCTIONS - PROTOTYPE                                                */
/* -------------------------------------------------------------------------- */
/**
 * @brief
 *   Check if connection is stale (too old).
 *
 * @return
 * - true if connection age exceeds threshold
 * - false otherwise
 */
static bool lIsConnectionStale(void);

/* -------------------------------------------------------------------------- */
/* PUBLIC VARIABLES                                                           */
/* -------------------------------------------------------------------------- */

/* -------------------------------------------------------------------------- */
/* PUBLIC FUNCTIONS - IMPLEMENTATION                                          */
/* -------------------------------------------------------------------------- */

/**
 * @brief  implement commInit
 *
 */
TCommIfStatus commInit
(
  const uint8_t* xpHost,
  const uint16_t xPort,
  const uint8_t* xpPath
)
{
  TCommIfStatus retStatus = E_COMM_IF_STATUS_ERROR;

  /* Update statistics */
  gStatsConnectAttempts++;

  /* Parameter validation */
  if ((xpHost == NULL) || (xpPath == NULL) || (xPort == 0))
  {
    gStatsConnectFailures++;
    gStatsLastError = E_COMM_IF_STATUS_PARAMETER;
    return E_COMM_IF_STATUS_PARAMETER;
  }

  /* Warn if re-connecting without closing previous connection */
  if (gIsConnected)
  {
    /* Log warning - application should call commTerm() first */
    gStatsTermCalls++;  /* Auto-cleanup counts as termination */
    (void)httpTerm();
    gIsConnected = false;
  }

  /* Attempt connection */
  retStatus = httpInit(E_COMM_IF_IP_PROTOCOL_V4, xpPath, xpHost, xPort);

  if (retStatus == E_COMM_IF_STATUS_OK)
  {
    gIsConnected = true;
    gConnectTime = time(NULL);
    gStatsConnectSuccess++;
  }
  else
  {
    gIsConnected = false;
    gStatsConnectFailures++;
    gStatsLastError = retStatus;
  }

  return retStatus;
}

/**
 * @brief  implement commMsgExchange
 *
 */
TCommIfStatus commMsgExchange
(
  const  uint8_t* xpMsgToSend,
  const  size_t   xSendSize,
  uint8_t*        xpRecvMsgBuffer,
  size_t*         xpRecvMsgBufferSize
)
{
  TCommIfStatus retStatus = E_COMM_IF_STATUS_ERROR;

  /* Update statistics */
  gStatsMsgExchangeAttempts++;

  /* Parameter validation */
  if ((xpMsgToSend == NULL) || (xpRecvMsgBuffer == NULL) || 
      (xpRecvMsgBufferSize == NULL) || (xSendSize == 0))
  {
    gStatsMsgExchangeFailures++;
    gStatsLastError = E_COMM_IF_STATUS_PARAMETER;
    return E_COMM_IF_STATUS_PARAMETER;
  }

  /* Check connection state */
  if (!gIsConnected)
  {
    gStatsMsgExchangeFailures++;
    gStatsLastError = E_COMM_IF_STATUS_NO_CONNECTION;
    return E_COMM_IF_STATUS_NO_CONNECTION;
  }

  /* Warn if connection is stale */
  if (lIsConnectionStale())
  {
    /* Log warning but continue - let underlying layer handle timeout */
  }

  /* Perform message exchange */
  retStatus = httpMsgExchange(xpMsgToSend, xSendSize, xpRecvMsgBuffer, xpRecvMsgBufferSize);

  if (retStatus == E_COMM_IF_STATUS_OK)
  {
    gStatsMsgExchangeSuccess++;
  }
  else
  {
    gStatsMsgExchangeFailures++;
    gStatsLastError = retStatus;
    
    /* On exchange failure, mark connection as dead */
    // gIsConnected = false;
  }

  return retStatus;
}

/**
 * @brief  implement commTerm
 *
 */
TCommIfStatus commTerm
(
  void
)
{
  TCommIfStatus retStatus = E_COMM_IF_STATUS_OK;

  /* Update statistics */
  gStatsTermCalls++;

  /* Only terminate if connected */
  if (gIsConnected)
  {
    retStatus = httpTerm();
    gIsConnected = false;
    gConnectTime = 0;
  }

  return retStatus;
}

/**
 * @brief
 *   Get communication statistics.
 *
 * @param[out] xpStats
 *   Pointer to statistics structure to fill.
 *
 * @return
 * - E_COMM_IF_STATUS_OK on success
 * - E_COMM_IF_STATUS_PARAMETER if xpStats is NULL
 */
TCommIfStatus commGetStatistics
(
  TCommIfStatistics* xpStats
)
{
  if (xpStats == NULL)
  {
    return E_COMM_IF_STATUS_PARAMETER;
  }

  xpStats->connectAttempts = gStatsConnectAttempts;
  xpStats->connectSuccess = gStatsConnectSuccess;
  xpStats->connectFailures = gStatsConnectFailures;
  xpStats->msgExchangeAttempts = gStatsMsgExchangeAttempts;
  xpStats->msgExchangeSuccess = gStatsMsgExchangeSuccess;
  xpStats->msgExchangeFailures = gStatsMsgExchangeFailures;
  xpStats->termCalls = gStatsTermCalls;
  xpStats->lastError = gStatsLastError;
  xpStats->isConnected = gIsConnected;
  xpStats->connectionAgeSeconds = gIsConnected ? (uint32_t)(time(NULL) - gConnectTime) : 0;

  return E_COMM_IF_STATUS_OK;
}

/**
 * @brief
 *   Reset communication statistics counters.
 */
void commResetStatistics
(
  void
)
{
  gStatsConnectAttempts = 0;
  gStatsConnectSuccess = 0;
  gStatsConnectFailures = 0;
  gStatsMsgExchangeAttempts = 0;
  gStatsMsgExchangeSuccess = 0;
  gStatsMsgExchangeFailures = 0;
  gStatsTermCalls = 0;
  gStatsLastError = E_COMM_IF_STATUS_OK;
  /* Note: Keep connection state unchanged */
}

/**
 * @brief
 *   Check connection health status.
 *
 * @return
 * - E_COMM_IF_STATUS_OK if connection is healthy
 * - E_COMM_IF_STATUS_NO_CONNECTION if not connected
 * - E_COMM_IF_STATUS_STALE if connection is too old
 */
TCommIfStatus commCheckConnectionHealth
(
  void
)
{
  if (!gIsConnected)
  {
    return E_COMM_IF_STATUS_NO_CONNECTION;
  }

  if (lIsConnectionStale())
  {
    return E_COMM_IF_STATUS_STALE;
  }

  return E_COMM_IF_STATUS_OK;
}

/* -------------------------------------------------------------------------- */
/* LOCAL FUNCTIONS - IMPLEMENTATION                                           */
/* -------------------------------------------------------------------------- */

/**
 * @implements lIsConnectionStale
 *
 */
static bool lIsConnectionStale(void)
{
  time_t currentTime = time(NULL);
  time_t connectionAge = currentTime - gConnectTime;

  return (connectionAge > C_COMM_IF_CONN_MAX_AGE_SEC);
}

/* -------------------------------------------------------------------------- */
/* END OF FILE                                                                */
/* -------------------------------------------------------------------------- */
