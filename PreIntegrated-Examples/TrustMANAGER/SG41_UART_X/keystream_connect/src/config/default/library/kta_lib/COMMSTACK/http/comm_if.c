/**
 * @file  comm_if.c
 * @brief Communication interface — delegates to the original socket-based HTTP stack
 *        (http.c + k_sal_com.c) which is proven to work with the keySTREAM ICPP server.
 */

#include "comm_if.h"
#include "http_if.h"
#include <stdio.h>
#include <string.h>
#include <time.h>

/* -------------------------------------------------------------------------- */
/* LOCAL VARIABLES                                                            */
/* -------------------------------------------------------------------------- */
static bool     gIsConnected = false;
static time_t   gConnectTime = 0;
static uint32_t gStatsConnectAttempts = 0;
static uint32_t gStatsConnectSuccess  = 0;
static uint32_t gStatsConnectFailures = 0;
static uint32_t gStatsMsgExchangeAttempts = 0;
static uint32_t gStatsMsgExchangeSuccess  = 0;
static uint32_t gStatsMsgExchangeFailures = 0;
static uint32_t gStatsTermCalls = 0;
static TCommIfStatus gStatsLastError = E_COMM_IF_STATUS_OK;

/* Saved connection parameters for auto-reconnect */
static uint8_t  gSavedHost[256];
static uint16_t gSavedPort = 0;
static uint8_t  gSavedPath[256];

/* -------------------------------------------------------------------------- */
/* PUBLIC FUNCTIONS                                                           */
/* -------------------------------------------------------------------------- */

TCommIfStatus commInit(const uint8_t* xpHost, const uint16_t xPort, const uint8_t* xpPath)
{
    gStatsConnectAttempts++;

    if ((xpHost == NULL) || (xpPath == NULL) || (xPort == 0))
    {
        gStatsConnectFailures++;
        gStatsLastError = E_COMM_IF_STATUS_PARAMETER;
        return E_COMM_IF_STATUS_PARAMETER;
    }

    /* Close previous connection if any */
    if (gIsConnected)
    {
        (void)httpTerm();
        gIsConnected = false;
    }

    /* Save connection params for auto-reconnect */
    strncpy((char*)gSavedHost, (const char*)xpHost, sizeof(gSavedHost) - 1);
    gSavedHost[sizeof(gSavedHost) - 1] = '\0';
    gSavedPort = xPort;
    strncpy((char*)gSavedPath, (const char*)xpPath, sizeof(gSavedPath) - 1);
    gSavedPath[sizeof(gSavedPath) - 1] = '\0';

    TCommIfStatus st = httpInit(E_COMM_IF_IP_PROTOCOL_V4, xpPath, xpHost, xPort);
    if (E_COMM_IF_STATUS_OK != st)
    {
        (void)printf("[COMM] httpInit failed: %d\n", (int)st);
        gStatsConnectFailures++;
        gStatsLastError = st;
        return st;
    }

    gIsConnected = true;
    gConnectTime = time(NULL);
    gStatsConnectSuccess++;
    (void)printf("[COMM] Connected to %s:%u\n", (const char*)xpHost, xPort);
    return E_COMM_IF_STATUS_OK;
}

TCommIfStatus commMsgExchange(const uint8_t* xpMsgToSend, const size_t xSendSize,
                               uint8_t* xpRecvMsgBuffer, size_t* xpRecvMsgBufferSize)
{
    gStatsMsgExchangeAttempts++;

    if ((xpMsgToSend == NULL) || (xpRecvMsgBuffer == NULL) ||
        (xpRecvMsgBufferSize == NULL) || (xSendSize == 0))
    {
        gStatsMsgExchangeFailures++;
        gStatsLastError = E_COMM_IF_STATUS_PARAMETER;
        return E_COMM_IF_STATUS_PARAMETER;
    }

    if (!gIsConnected)
    {
        gStatsMsgExchangeFailures++;
        gStatsLastError = E_COMM_IF_STATUS_NO_CONNECTION;
        return E_COMM_IF_STATUS_NO_CONNECTION;
    }

    TCommIfStatus st = httpMsgExchange(xpMsgToSend, xSendSize,
                                        xpRecvMsgBuffer, xpRecvMsgBufferSize);
    if (E_COMM_IF_STATUS_OK != st)
    {
        /* Auto-reconnect: server may have closed connection (Connection: close) */
        (void)printf("[COMM] Exchange failed (%d), reconnecting...\n", (int)st);
        (void)httpTerm();
        gIsConnected = false;

        TCommIfStatus rc = httpInit(E_COMM_IF_IP_PROTOCOL_V4, gSavedPath, gSavedHost, gSavedPort);
        if (E_COMM_IF_STATUS_OK != rc)
        {
            (void)printf("[COMM] Reconnect failed: %d\n", (int)rc);
            gStatsMsgExchangeFailures++;
            gStatsLastError = rc;
            return rc;
        }
        gIsConnected = true;
        (void)printf("[COMM] Reconnected to %s:%u\n", (const char*)gSavedHost, gSavedPort);

        /* Retry the exchange on the fresh connection */
        st = httpMsgExchange(xpMsgToSend, xSendSize,
                             xpRecvMsgBuffer, xpRecvMsgBufferSize);
        if (E_COMM_IF_STATUS_OK != st)
        {
            (void)printf("[COMM] Exchange failed after reconnect: %d\n", (int)st);
            gStatsMsgExchangeFailures++;
            gStatsLastError = st;
            return st;
        }
    }

    (void)printf("[COMM] Exchange OK: sent %zu, received %zu bytes\n",
                 xSendSize, *xpRecvMsgBufferSize);
    gStatsMsgExchangeSuccess++;
    return E_COMM_IF_STATUS_OK;
}

TCommIfStatus commTerm(void)
{
    gStatsTermCalls++;
    if (gIsConnected)
    {
        (void)httpTerm();
        gIsConnected = false;
        gConnectTime = 0;
    }
    return E_COMM_IF_STATUS_OK;
}

TCommIfStatus commGetStatistics(TCommIfStatistics* xpStats)
{
    if (xpStats == NULL) return E_COMM_IF_STATUS_PARAMETER;
    xpStats->connectAttempts      = gStatsConnectAttempts;
    xpStats->connectSuccess       = gStatsConnectSuccess;
    xpStats->connectFailures      = gStatsConnectFailures;
    xpStats->msgExchangeAttempts  = gStatsMsgExchangeAttempts;
    xpStats->msgExchangeSuccess   = gStatsMsgExchangeSuccess;
    xpStats->msgExchangeFailures  = gStatsMsgExchangeFailures;
    xpStats->termCalls            = gStatsTermCalls;
    xpStats->lastError            = gStatsLastError;
    xpStats->isConnected          = gIsConnected;
    xpStats->connectionAgeSeconds = gIsConnected ? (uint32_t)(time(NULL) - gConnectTime) : 0;
    return E_COMM_IF_STATUS_OK;
}

void commResetStatistics(void)
{
    gStatsConnectAttempts = gStatsConnectSuccess = gStatsConnectFailures = 0;
    gStatsMsgExchangeAttempts = gStatsMsgExchangeSuccess = gStatsMsgExchangeFailures = 0;
    gStatsTermCalls = 0;
    gStatsLastError = E_COMM_IF_STATUS_OK;
}

TCommIfStatus commCheckConnectionHealth(void)
{
    if (!gIsConnected) return E_COMM_IF_STATUS_NO_CONNECTION;
    return E_COMM_IF_STATUS_OK;
}
