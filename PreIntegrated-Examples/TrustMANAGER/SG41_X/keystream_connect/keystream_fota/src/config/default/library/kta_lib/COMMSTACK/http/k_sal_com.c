/*******************************************************************************
*************************keySTREAM Trusted Agent ("KTA")************************

* (c) 2023-2026 Nagravision S?rl

* Subject to your compliance with these terms, you may use the Nagravision S?rl
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
/** \brief  Interface for socket communication.
 *
 *  \author Kudelski Labs
 *
 *  \date 2025/06/12
 *
 *  \file k_sal_com.c
 ******************************************************************************/
/**
 * @brief Interface for socket communication.
 */

/* -------------------------------------------------------------------------- */
/* IMPORTS                                                                    */
/* -------------------------------------------------------------------------- */
#include "k_sal_com.h"
#include "definitions.h"
#include <errno.h>
#include <string.h>
#include "KTALog.h"
#include "ktaFieldMgntHook.h"
#include "system/wifi/sys_wincs_wifi_service.h"
#include "system/net/sys_wincs_net_service.h"

/* -------------------------------------------------------------------------- */
/* LOCAL CONSTANTS, TYPES, ENUM                                               */
/* -------------------------------------------------------------------------- */

/** @brief Invalid Socket Id. */
#define C_SAL_COM_SOCKET_INVALID            (-1)

/** @brief Timeout in mSec for WINC Tasks in SAL layer */
#define C_SAL_COM_WINC_TASKS_TIMEOUT_MSEC             1000
#define C_SAL_COM_SOCKET_READ_INITIAL_TIMEOUT_MSEC    15000
#define C_SAL_COM_SOCKET_READ_FOLLOWUP_TIMEOUT_MSEC   50

/** @brief Maximum iterations for WINC task polling loop.
 *  Safety guard against WDRV_WINC_Tasks() blocking or
 *  get_app_tick_ms() stalling. At ~1us per iteration this
 *  caps the loop at roughly 5 seconds wall time. */
#define C_SAL_COM_WINC_TASKS_MAX_ITERATIONS           5000000UL
/******************************************************************************/
/* LOCAL MACROS                                                               */
/******************************************************************************/

/**
 * SUPPRESS: MISRA_DEV_KTA_003 : misra_c2012_rule_21.6_violation
 * SUPPRESS: MISRA_DEV_KTA_001 : misra_c2012_rule_17.1_violation
 * Using printf for logging.
 * Not checking the return status of printf, since not required.
 **/
#define K_SALCOM__LOG                      printf


//#define K_SAL_COM_DEBUG_ENABLED
#ifdef K_SAL_COM_DEBUG_ENABLED
/** @brief Enable downloader debug logs. */
#define K_SAL_COM_DEBUG(fmt, ...)          printf("\t[K_SAL_COM]: " fmt "\r\n", ##__VA_ARGS__)
#define K_SAL_COM_DEBUG_GREEN(fmt, ...)    printf("\x1B[32m\t[K_SAL_COM]: " fmt "\r\n\x1B[0m", ##__VA_ARGS__)
#define K_SAL_COM_DEBUG_ERROR(fmt, ...)    printf("\x1B[31m\t[K_SAL_COM <%d>]: " fmt "\r\n\x1B[0m", __LINE__, ##__VA_ARGS__)
#else
#define K_SAL_COM_DEBUG(fmt, ...)
#define K_SAL_COM_DEBUG_GREEN(fmt, ...)
#define K_SAL_COM_DEBUG_ERROR(fmt, ...)
#endif /* DEBUG. */
/******************************************************************************/
/* TYPES & STRUCTURES                                                         */
/******************************************************************************/

/**
 * @ingroup g_k_types
 *
 * @brief Communication info.
 */
typedef struct
{
  uint32_t       socketId;
  /* ID of unique socket used. */
  uint32_t  connectTimeOut;
  /* Connection timeout. */
  uint32_t  readTimeOut;
  /* Read timeout. */
  uint32_t  isConnected;
  /* Flag to check whether already connection established with server or not. */
  uint8_t ptrId;
  /* Magic number to validate Pointer */
} TKComInfo;

/* -------------------------------------------------------------------------- */
/* LOCAL VARIABLES                                                            */
/* -------------------------------------------------------------------------- */

/** @brief Last socket event received from WINC callback */
static SYS_WINCS_NET_SOCK_EVENT_t gLastSocketEvent = SYS_WINCS_NET_SOCK_EVENT_UNDEFINED;

/** @brief Last socket ID that had an error */
static uint32_t gLastErrorSocketId = 0;

/** @brief Cumulative socket error count since init */
static uint32_t gSocketErrorCount = 0;

/* -------------------------------------------------------------------------- */
/* LOCAL FUNCTIONS - PROTOTYPE                                                */
/* -------------------------------------------------------------------------- */

/**
 * @brief  Print socket error diagnostic (RTOS equivalent of perror).
 *
 * @param[in] xpContext  Caller context string (e.g., function name).
 * @param[in] xStatus    The TKCommStatus error code.
 */
static void salComPerror(const char* xpContext, TKCommStatus xStatus);

/**
 * @brief  Get a human-readable string for TKCommStatus.
 *
 * @param[in] xStatus  The status code.
 * @return  String representation.
 */
static const char* salComStatusToString(TKCommStatus xStatus);

/**
 * @brief  Get a human-readable string for socket event.
 *
 * @param[in] xEvent  The socket event.
 * @return  String representation.
 */
static const char* salComSocketEventToString(SYS_WINCS_NET_SOCK_EVENT_t xEvent);

/* -------------------------------------------------------------------------- */
/* PUBLIC VARIABLES                                                           */
/* -------------------------------------------------------------------------- */
static TKComInfo gComInfo = {C_SAL_COM_SOCKET_INVALID, 0, 0, 0, 0};
static bool socketEventComplete;
static bool socketSendComplete;
static bool socketReadEvent;

extern char g_KTA_sock_ip_addr[25];

// Define and initialize a TCP client socket configuration from MCC
SYS_WINCS_NET_SOCKET_t k_sal_tcp_client = {
    .bindType = SYS_WINCS_NET_BIND_REMOTE,
    .sockAddr = g_KTA_sock_ip_addr,
    .sockType = SYS_WINCS_NET_SOCK_TYPE0,
    .sockPort = SYS_WINCS_NET_SOCK_PORT0,
    .tlsEnable = 0,
    .ipType  = SYS_WINCS_NET_SOCK_TYPE_IPv4_0,
};

static TKCommStatus salCommHandleWINCTasks(bool* event_flag, uint32_t timeout_ms);

/* -------------------------------------------------------------------------- */
/* PUBLIC FUNCTIONS - IMPLEMENTATION                                          */
/* -------------------------------------------------------------------------- */

/**
 * @brief Socket event callback function.
 *
 * Handles various socket events such as connection establishment and closure.
 *
 * @param sock Socket identifier
 * @param event Event type (connect, close, etc.)
 * @param netHandle Network handle (unused in this implementation)
 */
static void APP_SOCKET_Callback
(
    uint32_t socket,
    SYS_WINCS_NET_SOCK_EVENT_t event,
    SYS_WINCS_NET_HANDLE_t netHandle
)
{
    switch(event)
    {
        case SYS_WINCS_NET_SOCK_EVENT_CONNECTED:
        {
            K_SAL_COM_DEBUG("Socket(%ld) Connected", socket);
            gLastSocketEvent = event;
            gComInfo.isConnected = 1;
            socketEventComplete = true;
            break;
        }

        case SYS_WINCS_NET_SOCK_EVENT_DISCONNECTED:
        {
            K_SALCOM__LOG("[SOCK_DIAG] Socket(%ld) Disconnected unexpectedly\r\n", socket);
            gLastSocketEvent = event;
            gLastErrorSocketId = socket;
            socketEventComplete = true;
            socketSendComplete = true;
            socketReadEvent = true;
            gComInfo.isConnected = 0;
            gComInfo.socketId = C_SAL_COM_SOCKET_INVALID;
            break;
        }

        case SYS_WINCS_NET_SOCK_EVENT_ERROR:
        {
            gSocketErrorCount++;
            gLastSocketEvent = event;
            gLastErrorSocketId = socket;
            K_SALCOM__LOG("[SOCK_DIAG] Socket(%ld) ERROR event received "
                   "(total errors: %lu)\r\n", socket, gSocketErrorCount);
            socketEventComplete = true;
            socketSendComplete = true;
            socketReadEvent = true;
            gComInfo.isConnected = 0;
            break;
        }

        case SYS_WINCS_NET_SOCK_EVENT_READ:
        {
            gLastSocketEvent = event;
            socketReadEvent = true;
            K_SAL_COM_DEBUG("Socket(%ld) Read Event", socket);
            break;
        }

        case SYS_WINCS_NET_SOCK_EVENT_SEND_COMPLETE:
        {
            gLastSocketEvent = event;
            K_SAL_COM_DEBUG("Socket(%ld) Send Complete", socket);
            socketSendComplete = true;
            break;
        }

        case SYS_WINCS_NET_SOCK_EVENT_CLOSED:
        {
            gLastSocketEvent = event;
            K_SAL_COM_DEBUG("Socket(%ld) Closed", socket);
            socketEventComplete = true;
            break;
        }

        default:
            gLastSocketEvent = event;
            K_SALCOM__LOG("[SOCK_DIAG] Socket(%ld) Unhandled event(%d)\r\n", socket, event);
            break;
    }

}

/**
 * @brief  implement salComInit
 *
 */
K_SAL_API TKCommStatus salComInit
(
  uint32_t  xConnectTimeoutInMs,
  uint32_t  xReadTimeoutInMs,
  void**    xppComInfo
)
{
  TKCommStatus status = E_K_COMM_STATUS_ERROR;

  K_SAL_COM_DEBUG("Start of %s", __func__);

  if (NULL == xppComInfo)
  {
    K_SAL_COM_DEBUG_ERROR("Invalid parameter");
    status = E_K_COMM_STATUS_PARAMETER;
  }
  else
  {
    if (gComInfo.ptrId == 2u)
    {
      /* Force cleanup of stale state from a previous failed cycle
       * that did not call salComTerm properly. Without this, all
       * subsequent salComInit calls silently return ERROR and the
       * connection stack is permanently stuck. */
      K_SALCOM__LOG("[SOCK_DIAG] salComInit: stale ptrId=2, forcing cleanup"
             "(sock=%ld, conn=%lu)\r\n",
             (long)gComInfo.socketId, (unsigned long)gComInfo.isConnected);

      /* Close any lingering socket */
      if (k_sal_tcp_client.sockID != C_SAL_COM_SOCKET_INVALID)
      {
        socketEventComplete = false;
        SYS_WINCS_NET_SockSrvCtrl(SYS_WINCS_NET_SOCK_CLOSE,
                                  &(k_sal_tcp_client.sockID));
        (void)salCommHandleWINCTasks(&socketEventComplete,
                                    C_SAL_COM_WINC_TASKS_TIMEOUT_MSEC);
      }

      /* Reset all state so re-initialisation proceeds below */
      k_sal_tcp_client.sockID = C_SAL_COM_SOCKET_INVALID;
      gComInfo.socketId    = C_SAL_COM_SOCKET_INVALID;
      gComInfo.isConnected = 0;
      gComInfo.ptrId       = 0u;
    }

    /* Normal initialisation (also reached after forced cleanup above) */
    {
      gComInfo.socketId = C_SAL_COM_SOCKET_INVALID;
      gComInfo.connectTimeOut = xConnectTimeoutInMs;
      gComInfo.readTimeOut = xReadTimeoutInMs;
      gComInfo.isConnected = 0;
      gComInfo.ptrId = 2u;
      *xppComInfo = &gComInfo;
      socketEventComplete = false;
      socketSendComplete = false;
      socketReadEvent = false;
      status = E_K_COMM_STATUS_OK;
      SYS_WINCS_NET_SockSrvCtrl(SYS_WINCS_NET_SOCK_SET_CALLBACK, APP_SOCKET_Callback);
    }
  }

  K_SAL_COM_DEBUG("End of %s", __func__);
  return status;
}

/**
 * @brief  implement salComConnect
 *
 */
K_SAL_API TKCommStatus salComConnect
(
  void*          xpComInfo,
  const uint8_t* xpHost,
  const uint8_t* xpPort
)
{
  TKComInfo*          pComInfo = (TKComInfo *)xpComInfo;
  TKCommStatus        status = E_K_COMM_STATUS_ERROR;

  K_SAL_COM_DEBUG("Start of %s", __func__);

  if ((NULL == xpComInfo) || (NULL == xpHost) || (NULL == xpPort) || (pComInfo->ptrId != 2u))
  {
    K_SAL_COM_DEBUG_ERROR("Invalid parameter");
    status = E_K_COMM_STATUS_PARAMETER;
  }
  else
  {
    if (1u == pComInfo->isConnected)
    {
      K_SAL_COM_DEBUG_ERROR("Socket Already in Use");
      status = E_K_COMM_STATUS_ERROR;
      goto end;
    }

    if (C_SAL_COM_SOCKET_INVALID == pComInfo->socketId)
    {
      K_SALCOM__LOG("[SOCK_DIAG] salComConnect: opening TCP socket to %s\r\n",
             g_KTA_sock_ip_addr);

      // Reset event flag before socket operation to prevent stale callbacks
      socketEventComplete = false;

      // Create a TCP socket
      k_sal_tcp_client.sockID = C_SAL_COM_SOCKET_INVALID;
      if (SYS_WINCS_FAIL == SYS_WINCS_NET_SockSrvCtrl(SYS_WINCS_NET_SOCK_TCP_OPEN, &k_sal_tcp_client))
      {
          K_SALCOM__LOG("[SOCK_DIAG] SOCK_TCP_OPEN FAILED (sockID=%ld)\r\n",
                 (long)k_sal_tcp_client.sockID);
          status = E_K_COMM_STATUS_ERROR;
          salComPerror("Connect/TCP_OPEN", status);
          goto end;
      }
      K_SALCOM__LOG("[SOCK_DIAG] SOCK_TCP_OPEN ok, sockID=%ld, waiting for connect...\r\n",
             (long)k_sal_tcp_client.sockID);

      if(E_K_COMM_STATUS_TIMEOUT == salCommHandleWINCTasks(&socketEventComplete, C_SAL_COM_WINC_TASKS_TIMEOUT_MSEC))
      {
        K_SAL_COM_DEBUG_ERROR("Socket Open timedout");
        // Close the orphaned socket to prevent resource leak and stale callbacks
        if (k_sal_tcp_client.sockID != C_SAL_COM_SOCKET_INVALID)
        {
          K_SAL_COM_DEBUG("Closing timed-out socket(%ld)", k_sal_tcp_client.sockID);
          SYS_WINCS_NET_SockSrvCtrl(SYS_WINCS_NET_SOCK_CLOSE, &(k_sal_tcp_client.sockID));
          k_sal_tcp_client.sockID = C_SAL_COM_SOCKET_INVALID;
        }
        socketEventComplete = false;
        status = E_K_COMM_STATUS_TIMEOUT;
        salComPerror("Connect/OPEN_TIMEOUT", status);
        goto end;
      }
      socketEventComplete = false;

      pComInfo->socketId = (int)k_sal_tcp_client.sockID;
      if ((int)pComInfo->socketId <= 0)
      {
        K_SAL_COM_DEBUG_ERROR("Invalid Socket id");
        status = E_K_COMM_STATUS_RESOURCE;
        goto end;
      }
    }

    pComInfo->isConnected = 1u;
    status = E_K_COMM_STATUS_OK;
    goto end;
  }

end:
  K_SAL_COM_DEBUG("End of %s", __func__);
  return status;
}


/**
 * @brief  implement salComWrite
 *
 */
K_SAL_API TKCommStatus salComWrite
(
  void*          xpComInfo,
  const uint8_t* xpBuffer,
  size_t         xBufferLen
)
{
  TKCommStatus  status = E_K_COMM_STATUS_ERROR;
  TKComInfo* pComInfo  = (TKComInfo *)xpComInfo;

  K_SAL_COM_DEBUG("Start of %s", __func__);

  if ((NULL == xpComInfo) || (NULL == xpBuffer) || (0U == xBufferLen) || (pComInfo->ptrId != 2u))
  {
    K_SAL_COM_DEBUG_ERROR("Invalid parameter");
    status = E_K_COMM_STATUS_PARAMETER;
  }
  else
  {
    if (0 == pComInfo->isConnected)
    {
      K_SAL_COM_DEBUG_ERROR("Socket Not Connected");
      status = E_K_COMM_STATUS_ERROR;
      goto end;
    }

    if ((int)pComInfo->socketId <= 0)
    {
      K_SAL_COM_DEBUG_ERROR("Invalid Socket ID: %d", (int)pComInfo->socketId);
      status = E_K_COMM_STATUS_ERROR;
      goto end;
    }

    K_SAL_COM_DEBUG("Writing to SOCKET ID: %d", (int)pComInfo->socketId);
    socketSendComplete = false;
    if (SYS_WINCS_FAIL == SYS_WINCS_NET_TcpSockWrite(pComInfo->socketId, (uint16_t)xBufferLen, (uint8_t*)xpBuffer))
    {
      K_SAL_COM_DEBUG_ERROR("Socket send ERROR");
      status = E_K_COMM_STATUS_ERROR;
      salComPerror("Write/TcpSockWrite", status);
      goto end;
    }

    if(E_K_COMM_STATUS_TIMEOUT == salCommHandleWINCTasks(&socketSendComplete, C_SAL_COM_WINC_TASKS_TIMEOUT_MSEC))
    {
      status = E_K_COMM_STATUS_TIMEOUT;
      salComPerror("Write/SEND_TIMEOUT", status);
      goto end;
    }
    socketSendComplete = false;

    if (0 == pComInfo->isConnected)
    {
      K_SAL_COM_DEBUG_ERROR("Socket disconnected during write");
      status = E_K_COMM_STATUS_ERROR;
      salComPerror("Write/DISCONNECTED", status);
      goto end;
    }

    K_SAL_COM_DEBUG("Socket send complete");
    status = E_K_COMM_STATUS_OK;
    goto end;
  }

end:
  K_SAL_COM_DEBUG("End of %s", __func__);
  return status;
}

/**
 * @brief  implement salComRead
 *
 */
K_SAL_API TKCommStatus salComRead
(
    void*     xpComInfo,
    uint8_t*  xpBuffer,
    size_t*   xpBufferLen
)
{
  TKComInfo* pComInfo   = (TKComInfo *)xpComInfo;
  TKCommStatus  status  = E_K_COMM_STATUS_OK;
  size_t totalRead = 0;
  size_t totalToRead = *xpBufferLen;
  uint8_t *readPtr = xpBuffer;
  uint32_t socket_read_timeout = C_SAL_COM_SOCKET_READ_INITIAL_TIMEOUT_MSEC; // Initial read timeout in ms


  K_SAL_COM_DEBUG("Start of %s", __func__);

  if ((NULL == xpComInfo) || (NULL == xpBuffer) || (NULL == xpBufferLen) ||
    (0 == *xpBufferLen) || (pComInfo->ptrId != 2u))
  {
    K_SAL_COM_DEBUG_ERROR("Invalid parameter");
    *xpBufferLen = 0;
    return E_K_COMM_STATUS_PARAMETER;
  }

  if (0 == pComInfo->isConnected)
  {
    K_SAL_COM_DEBUG_ERROR("Socket Not Connected");
    *xpBufferLen = 0;
    return E_K_COMM_STATUS_ERROR;
  }

  if ((int)pComInfo->socketId <= 0)
  {
    K_SAL_COM_DEBUG_ERROR("Invalid Socket ID: %d", (int)pComInfo->socketId);
    *xpBufferLen = 0;
    return E_K_COMM_STATUS_ERROR;
  }

  // Read data from socket
  while (totalRead < totalToRead)
  {
    // Wait for data availability
    if(E_K_COMM_STATUS_TIMEOUT == salCommHandleWINCTasks(&socketReadEvent, socket_read_timeout))
    {
      status = E_K_COMM_STATUS_TIMEOUT;
      goto end;
    }
    socketReadEvent = false;

    if (0 == pComInfo->isConnected)
    {
      K_SAL_COM_DEBUG_ERROR("Socket disconnected during read");
      status = E_K_COMM_STATUS_ERROR;
      salComPerror("Read/DISCONNECTED", status);
      goto end;
    }

    // Read data from the socket
    int bytesRead = SYS_WINCS_NET_TcpSockRead(pComInfo->socketId,
                                              totalToRead - totalRead,
                                              readPtr);
    if (bytesRead <= 0)
    {
      K_SAL_COM_DEBUG_ERROR("No more data or timeout occurred");
      break;
    }

    totalRead += bytesRead;
    readPtr += bytesRead;
    socket_read_timeout = C_SAL_COM_SOCKET_READ_FOLLOWUP_TIMEOUT_MSEC; // After first read, set timeout in ms
    K_SAL_COM_DEBUG("Bytes Read in this iteration: %d", bytesRead);
  }

end:
  *xpBufferLen = totalRead;
  if (totalRead > 0)
  {
    K_SAL_COM_DEBUG("Total Bytes Read: %d", totalRead);
    status = E_K_COMM_STATUS_OK;
  }
  else
  {
    K_SAL_COM_DEBUG_ERROR("No data received");
    status = E_K_COMM_STATUS_ERROR;
  }
  K_SAL_COM_DEBUG("End of %s", __func__);
  return status;
}


/**
 * @brief  implement salComTerm
 *
 */
K_SAL_API TKCommStatus salComTerm
(
  void*  xpComInfo
)
{
  TKCommStatus status = E_K_COMM_STATUS_ERROR;
  TKComInfo* pComInfo = (TKComInfo *)xpComInfo;
  K_SAL_COM_DEBUG("Start of %s", __func__);

  if (NULL == xpComInfo)
  {
    K_SAL_COM_DEBUG_ERROR("Invalid parameter");
    return E_K_COMM_STATUS_PARAMETER;
  }

  if ((pComInfo->isConnected == 0) || (pComInfo->socketId == C_SAL_COM_SOCKET_INVALID))
  {
      // Check for orphaned socket from a timed-out open that was never tracked
      if (k_sal_tcp_client.sockID != C_SAL_COM_SOCKET_INVALID)
      {
          K_SAL_COM_DEBUG("Closing orphaned socket(%ld)", k_sal_tcp_client.sockID);
          socketEventComplete = false;
          SYS_WINCS_NET_SockSrvCtrl(SYS_WINCS_NET_SOCK_CLOSE, &(k_sal_tcp_client.sockID));
          salCommHandleWINCTasks(&socketEventComplete, C_SAL_COM_WINC_TASKS_TIMEOUT_MSEC);
      }
      else
      {
          K_SAL_COM_DEBUG("Socket already closed");
      }
  }
  else
  {
    // Create a TCP socket SYS_WINCS_NET_SOCK_CLOSE
    socketEventComplete = false;
    if (SYS_WINCS_FAIL == SYS_WINCS_NET_SockSrvCtrl(SYS_WINCS_NET_SOCK_CLOSE,  &(pComInfo->socketId)))
    {
      K_SAL_COM_DEBUG_ERROR("Socket Close error");
    }

    if(E_K_COMM_STATUS_TIMEOUT == salCommHandleWINCTasks(&socketEventComplete, C_SAL_COM_WINC_TASKS_TIMEOUT_MSEC))
    {
      status = E_K_COMM_STATUS_TIMEOUT;
    }
  }

  socketEventComplete = false;
  socketSendComplete = false;
  socketReadEvent = false;
  k_sal_tcp_client.sockID = C_SAL_COM_SOCKET_INVALID;
  pComInfo->socketId = C_SAL_COM_SOCKET_INVALID;
  pComInfo->isConnected = 0;
  pComInfo->ptrId = 0u;
  status = E_K_COMM_STATUS_OK;

  K_SAL_COM_DEBUG("End of %s", __func__);
  return status;
}
/* -------------------------------------------------------------------------- */
/* LOCAL FUNCTIONS - IMPLEMENTATION                                           */
/* -------------------------------------------------------------------------- */
static TKCommStatus salCommHandleWINCTasks(bool* event_flag, uint32_t timeout_ms)
{
    uint32_t startTick = get_app_tick_ms();
    uint32_t iterations = 0;

    while (!(*event_flag))
    {
      WDRV_WINC_Tasks(sysObj.drvWifiWinc);

      iterations++;

      if ((get_app_tick_ms() - startTick) >= timeout_ms)
      {
        K_SALCOM__LOG("[SOCK_DIAG] WINCTasks timeout after %lu ms (%lu iterations)\r\n",
               (unsigned long)(get_app_tick_ms() - startTick),
               (unsigned long)iterations);
        return E_K_COMM_STATUS_TIMEOUT;
      }

      /* Safety guard: if tick timer stopped or WDRV_WINC_Tasks is
       * returning instantly without advancing the clock, cap the
       * total number of iterations to avoid an infinite spin. */
      if (iterations >= C_SAL_COM_WINC_TASKS_MAX_ITERATIONS)
      {
        K_SALCOM__LOG("[SOCK_DIAG] WINCTasks MAX ITERATIONS reached "
               "(%lu) – tick stuck at %lu (start %lu)\r\n",
               (unsigned long)iterations,
               (unsigned long)get_app_tick_ms(),
               (unsigned long)startTick);
        return E_K_COMM_STATUS_TIMEOUT;
      }
    }
    return E_K_COMM_STATUS_OK;
}

/**
 * @brief  RTOS equivalent of perror() for socket diagnostics.
 *
 * Prints the caller context, error code name, last socket event,
 * socket ID, and cumulative error count to help diagnose network
 * latency and transient connection failures.
 */
static void salComPerror(const char* xpContext, TKCommStatus xStatus)
{
  K_SALCOM__LOG("[SOCK_DIAG] %s: %s | lastEvent=%s | socketId=%ld | "
         "connected=%lu | errCount=%lu\r\n",
         (xpContext != NULL) ? xpContext : "unknown",
         salComStatusToString(xStatus),
         salComSocketEventToString(gLastSocketEvent),
         (long)gComInfo.socketId,
         (unsigned long)gComInfo.isConnected,
         (unsigned long)gSocketErrorCount);
}

static const char* salComStatusToString(TKCommStatus xStatus)
{
  switch (xStatus)
  {
    case E_K_COMM_STATUS_OK:           return "OK";
    case E_K_COMM_STATUS_PARAMETER:    return "BAD_PARAMETER";
    case E_K_COMM_STATUS_DATA:         return "DATA_ERROR";
    case E_K_COMM_STATUS_ERROR:        return "GENERAL_ERROR";
    case E_K_COMM_STATUS_NOT_SUPPORTED:return "NOT_SUPPORTED";
    case E_K_COMM_STATUS_STATE:        return "INVALID_STATE";
    case E_K_COMM_STATUS_MEMORY:       return "MEMORY_ALLOC_FAILED";
    case E_K_COMM_STATUS_RESOURCE:     return "RESOURCE_UNAVAILABLE";
    case E_K_COMM_STATUS_TIMEOUT:      return "TIMEOUT";
    case E_K_COMM_STATUS_MISSING:      return "DATA_MISSING";
    case E_K_COMM_STATUS_REMOTE_STATE: return "REMOTE_STATE_ERROR";
    default:                           return "UNKNOWN_STATUS";
  }
}

static const char* salComSocketEventToString(SYS_WINCS_NET_SOCK_EVENT_t xEvent)
{
  switch (xEvent)
  {
    case SYS_WINCS_NET_SOCK_EVENT_CONNECTED:        return "CONNECTED";
    case SYS_WINCS_NET_SOCK_EVENT_TLS_DONE:         return "TLS_DONE";
    case SYS_WINCS_NET_SOCK_EVENT_DISCONNECTED:     return "DISCONNECTED";
    case SYS_WINCS_NET_SOCK_EVENT_SEND_COMPLETE:    return "SEND_COMPLETE";
    case SYS_WINCS_NET_SOCK_EVENT_READ:             return "READ";
    case SYS_WINCS_NET_SOCK_EVENT_ERROR:            return "SOCKET_ERROR";
    case SYS_WINCS_NET_SOCK_EVENT_UNDEFINED:        return "UNDEFINED";
    case SYS_WINCS_NET_SOCK_EVENT_CLIENT_CONNECTED: return "CLIENT_CONNECTED";
    case SYS_WINCS_NET_SOCK_EVENT_CLOSED:           return "CLOSED";
    default:                                        return "UNKNOWN_EVENT";
  }
}
/* -------------------------------------------------------------------------- */
/* END OF FILE                                                                */
/* -------------------------------------------------------------------------- */
