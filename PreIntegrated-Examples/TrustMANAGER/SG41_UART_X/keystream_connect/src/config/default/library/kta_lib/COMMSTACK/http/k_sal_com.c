/*******************************************************************************
*************************keySTREAM Trusted Agent ("KTA")************************

* (c) 2023-2024 Nagravision S?rl

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
 *  \author Kudelski IoT
 *
 *  \date 2023/06/12
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
#include "system/time/sys_time.h"

/* -------------------------------------------------------------------------- */
/* LOCAL CONSTANTS, TYPES, ENUM                                               */
/* -------------------------------------------------------------------------- */

/** @brief Invalid Socket Id. */
#define C_SAL_COM_SOCKET_INVALID            (-1)

/** @brief Timeout in mSec for WINC Tasks in SAL layer */
#define C_SAL_COM_WINC_TASKS_TIMEOUT_MSEC             1000
#define C_SAL_COM_SOCKET_READ_INITIAL_TIMEOUT_MSEC    15000
#define C_SAL_COM_SOCKET_READ_FOLLOWUP_TIMEOUT_MSEC   50
/******************************************************************************/
/* LOCAL MACROS                                                               */
/******************************************************************************/
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

/* -------------------------------------------------------------------------- */
/* LOCAL FUNCTIONS - PROTOTYPE                                                */
/* -------------------------------------------------------------------------- */


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
            gComInfo.isConnected = 1;
            socketEventComplete = true;
            break;
        }

        case SYS_WINCS_NET_SOCK_EVENT_DISCONNECTED:
        {
            K_SAL_COM_DEBUG("Socket(%ld) Disconnected", socket);
            socketEventComplete = true;
            gComInfo.isConnected = 0;
            gComInfo.socketId = C_SAL_COM_SOCKET_INVALID;
            break;
        }

        case SYS_WINCS_NET_SOCK_EVENT_ERROR:
        {
            K_SAL_COM_DEBUG("Socket(%ld) Error", socket);
            break;
        }

        case SYS_WINCS_NET_SOCK_EVENT_READ:
        {
            socketReadEvent = true;
            K_SAL_COM_DEBUG("Socket(%ld) Read Event", socket);
            break;
        }

        case SYS_WINCS_NET_SOCK_EVENT_SEND_COMPLETE:
        {
            K_SAL_COM_DEBUG("Socket(%ld) Send Complete", socket);
            socketSendComplete = true;
            break;
        }

        case SYS_WINCS_NET_SOCK_EVENT_CLOSED:
        {
            K_SAL_COM_DEBUG("Socket(%ld) Closed", socket);
            socketEventComplete = true;
            break;
        }

        default:
            K_SAL_COM_DEBUG("Socket(%ld) Unhandled event(%d)", socket, event);
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
      K_SAL_COM_DEBUG_ERROR("Already Initialized!!");
    }
    else
    {
      gComInfo.socketId = C_SAL_COM_SOCKET_INVALID;
      gComInfo.connectTimeOut = xConnectTimeoutInMs;
      gComInfo.readTimeOut = xReadTimeoutInMs;
      gComInfo.isConnected = 0;
      gComInfo.ptrId = 2u;
      *xppComInfo = &gComInfo;
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
      K_SAL_COM_DEBUG("Connecting to IP: %s", g_KTA_sock_ip_addr);

      // Create a TCP socket
      k_sal_tcp_client.sockID = C_SAL_COM_SOCKET_INVALID;
      if (SYS_WINCS_FAIL == SYS_WINCS_NET_SockSrvCtrl(SYS_WINCS_NET_SOCK_TCP_OPEN, &k_sal_tcp_client))
      {
          K_SAL_COM_DEBUG_ERROR("Error in Socket TCP Open");
      }

      if(E_K_COMM_STATUS_TIMEOUT == salCommHandleWINCTasks(&socketEventComplete, C_SAL_COM_WINC_TASKS_TIMEOUT_MSEC))
      {
        K_SAL_COM_DEBUG_ERROR("Socket Open timedout");
        status = E_K_COMM_STATUS_TIMEOUT;
        goto end;
      }
      socketEventComplete = false;

      pComInfo->socketId = (int)k_sal_tcp_client.sockID;
      if (pComInfo->socketId <= 0)
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

    K_SAL_COM_DEBUG("Writing to SOCKET ID: %d", (int)pComInfo->socketId);
    socketSendComplete = false;
    if (SYS_WINCS_FAIL == SYS_WINCS_NET_TcpSockWrite(pComInfo->socketId, (uint16_t)xBufferLen, (uint8_t*)xpBuffer))
    {
      K_SAL_COM_DEBUG_ERROR("Socket send ERROR");
      status = E_K_COMM_STATUS_ERROR;
      goto end;
    }

    if(E_K_COMM_STATUS_TIMEOUT == salCommHandleWINCTasks(&socketSendComplete, C_SAL_COM_WINC_TASKS_TIMEOUT_MSEC))
    {
      status = E_K_COMM_STATUS_TIMEOUT;
      goto end;
    }
    socketSendComplete = false;

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
      K_SAL_COM_DEBUG("Socket already closed");
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
    socketEventComplete = false;
  }

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
    DRV_HANDLE wdrvHandle = DRV_HANDLE_INVALID;
    uint32_t startTick = SYS_TIME_CounterGet();
    uint32_t timeoutTicks = SYS_TIME_MSToCount(timeout_ms);

    (void)SYS_WINCS_WIFI_SrvCtrl(SYS_WINCS_WIFI_GET_DRV_HANDLE, &wdrvHandle);

    while (!(*event_flag))
    {
      if (wdrvHandle != DRV_HANDLE_INVALID)
      {
        WDRV_WINC_Tasks(wdrvHandle);
      }

      if ((SYS_TIME_CounterGet() - startTick) >= timeoutTicks)
      {
        return E_K_COMM_STATUS_TIMEOUT;
      }
    }
    return E_K_COMM_STATUS_OK;
}
/* -------------------------------------------------------------------------- */
/* END OF FILE                                                                */
/* -------------------------------------------------------------------------- */
