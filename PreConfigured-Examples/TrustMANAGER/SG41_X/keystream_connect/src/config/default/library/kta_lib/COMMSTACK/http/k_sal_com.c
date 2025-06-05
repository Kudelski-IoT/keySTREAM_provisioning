/*******************************************************************************
*************************keySTREAM Trusted Agent ("KTA")************************

* (c) 2023-2024 Nagravision S�rl

* Subject to your compliance with these terms, you may use the Nagravision S�rl
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

#include "k_sal_com.h"
/* -------------------------------------------------------------------------- */
/* IMPORTS                                                                    */
/* -------------------------------------------------------------------------- */
#include "cryptoauthlib.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
// #include "configuration.h"
#include "definitions.h" 
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include "KTALog.h"
#include "ktaFieldMgntHook.h"
#include "system/wifi/sys_wincs_wifi_service.h"
#include "system/net/sys_wincs_net_service.h"
/* -------------------------------------------------------------------------- */
/* LOCAL CONSTANTS, TYPES, ENUM                                               */
/* -------------------------------------------------------------------------- */

/** @brief Invalid Socket Id. */
#define C_SAL_COM_SOCKET_INVALID (-1)

/** @brief Connect wait time in ms. */
#define C_SAL_COM_CONNECT_DELAY_IN_MS       (500u)

/**
 * @brief
 * Reset read time out value in ms(this will override the read timeout
 * after first successful read. This is added to avoid more dealy in read because
 * read will always hit the timeout while checking any bytes are available or not.
 */
#define C_SAL_COM_RESET_READ_TIMEOUT_IN_MS  (200u)

/******************************************************************************/
/* LOCAL MACROS                                                               */
/******************************************************************************/
#ifdef DEBUG
/** @brief Enable sal com logs. */
/** 
 * SUPPRESS: MISRA_DEV_KTA_003 : misra_c2012_rule_21.6_violation
 * SUPPRESS: MISRA_DEV_KTA_001 : misra_c2012_rule_17.1_violation
 * Using printf for logging.
 * Not checking the return status of printf, since not required.
 **/
#define M_INTL_SAL_COM_DEBUG(__PRINT__)  do { \
                                              printf("\tCOM %d>", __LINE__); \
                                              printf __PRINT__; \
                                              printf("\r\n"); \
                                          } while (0)
#define M_INTL_SAL_COM_ERROR(__PRINT__)  do { \
                                              printf("\tCOM %d> ERROR ", __LINE__); \
                                              printf __PRINT__; \
                                              printf("\r\n"); \
                                          } while (0)
#else
#define M_INTL_SAL_COM_DEBUG(__PRINT__)
#define M_INTL_SAL_COM_ERROR(__PRINT__)
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
  /* Flag to check whether already connection estabilished with server or not. */
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
//static int appTcpRxSize = 2048;        // socket TX and RX buffer size
static TKComInfo gComInfo = {C_SAL_COM_SOCKET_INVALID, 0, 0, 0, 0};
volatile bool g_socketEventComplete = false;
volatile bool g_socketSendComplete = false;
volatile bool g_socketReadEvent = false;
volatile SYS_WINCS_NET_SOCK_EVENT_t g_lastSocketEvent;
extern char g_KTA_socketAddressIp[50];


// Define and initialize a TCP client socket configuration from MCC
SYS_WINCS_NET_SOCKET_t g_tcpClientSocket = {
    // Specify the type of binding for the socket
    .bindType = SYS_WINCS_NET_BIND_REMOTE,
    // Set the socket address to the predefined server address
    .sockAddr = g_KTA_socketAddressIp,
    // Define the type of socket (e.g., TCP, UDP)
    .sockType = SYS_WINCS_NET_SOCK_TYPE0,
    // Set the port number for the socket
    .sockPort = SYS_WINCS_NET_SOCK_PORT0,
    // Enable or disable TLS for the socket
    .tlsEnable = 0,
    // Specify the IP type (e.g., IPv4, IPv6)
    .ipType  = SYS_WINCS_NET_SOCK_TYPE_IPv4_0,
};

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
void APP_SOCKET_Callback
(
    uint32_t socket,
    SYS_WINCS_NET_SOCK_EVENT_t event,
    SYS_WINCS_NET_HANDLE_t netHandle
)
{
    M_INTL_SAL_COM_DEBUG(("%d START SOCKET event\r\n", event));
    M_INTL_SAL_COM_DEBUG((TERM_CYAN"[SOCKET] : SOCKET ID: %d\r\n"TERM_RESET, (int)socket));
    g_lastSocketEvent = event;
    
    switch(event)
    {
        case SYS_WINCS_NET_SOCK_EVENT_CONNECTED:
        {
            M_INTL_SAL_COM_DEBUG((TERM_GREEN"[K_SAL] : Connected to Server!\r\n"TERM_RESET));
            gComInfo.isConnected = 1;
            g_socketEventComplete = true;
            break;
        }

        case SYS_WINCS_NET_SOCK_EVENT_DISCONNECTED:
        {
            M_INTL_SAL_COM_DEBUG((TERM_RED"[K_SAL] : Socket Disconnected!\r\n"TERM_RESET));
            g_socketEventComplete = true;
            gComInfo.isConnected = 0;
            gComInfo.socketId = C_SAL_COM_SOCKET_INVALID;
            break;
        }

        case SYS_WINCS_NET_SOCK_EVENT_ERROR:
        {
            M_INTL_SAL_COM_DEBUG((TERM_RED"[K_SAL ERROR] : Socket Error\r\n"TERM_RESET));
            break;
        }

        case SYS_WINCS_NET_SOCK_EVENT_READ:
        {
            g_socketReadEvent = true;
            M_INTL_SAL_COM_DEBUG((TERM_GREEN"[K_SAL SOCKET] : READ EVENT - DATA AVAILABLE \r\n"TERM_RESET));
            break;
        }

        case SYS_WINCS_NET_SOCK_EVENT_SEND_COMPLETE:
        {
            M_INTL_SAL_COM_DEBUG((TERM_GREEN"[K_SAL SOCKET] : Socket SEND complete\r\n"TERM_RESET));
            g_socketSendComplete = true;
            break;
        }

        case SYS_WINCS_NET_SOCK_EVENT_CLOSED:
        {
            M_INTL_SAL_COM_DEBUG((TERM_GREEN"[K_SAL] : Socket CLOSED\r\n"TERM_RESET));
            g_socketEventComplete = true;
            break;
        }


        default:
            M_INTL_SAL_COM_DEBUG((TERM_GREEN"Un-handled SOCKET event\r\n"TERM_RESET));
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

  M_INTL_SAL_COM_DEBUG(("Start of %s", __func__));

  if (NULL == xppComInfo)
  {
    M_INTL_SAL_COM_ERROR(("Invalid parameter"));
    status = E_K_COMM_STATUS_PARAMETER;
  }
  else
  {
    if (gComInfo.ptrId == 2u)
    {
      M_INTL_SAL_COM_ERROR(("Already Initialized!!"));
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

  
  M_INTL_SAL_COM_DEBUG(("End of %s", __func__));
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
  uint16_t            port = 0;
  uint32_t            retVal = 0;
  char*               pEnd = NULL;

  M_INTL_SAL_COM_DEBUG(("Start of %s", __func__));

  if ((NULL == xpComInfo) || (NULL == xpHost) || (NULL == xpPort) || (pComInfo->ptrId != 2u))
  {
    M_INTL_SAL_COM_ERROR(("Invalid parameter"));
    status = E_K_COMM_STATUS_PARAMETER;
  }
  else
  {
    if (1u == pComInfo->isConnected)
    {
      M_INTL_SAL_COM_DEBUG(("Socket Already in Use"));
      status = E_K_COMM_STATUS_ERROR;
      goto end;
    }

    retVal = (uint32_t)strtol((const char *)xpPort, &pEnd, 10);
    if ((retVal > 0) && (retVal <= 0xFFFF))
    {
      port = (uint16_t)retVal;
    }
    else
    {
      M_INTL_SAL_COM_ERROR(("Invalid port number"));
      status = E_K_COMM_STATUS_PARAMETER;
      goto end;
    }

    if (C_SAL_COM_SOCKET_INVALID == pComInfo->socketId)
    {
      M_INTL_SAL_COM_DEBUG(("Connecting to IP: %s", g_KTA_socketAddressIp));      
      M_INTL_SAL_COM_DEBUG(("Connecting to PORT: %d", port));      
      /* Create socket. */
      // Create a TCP socket
      if (SYS_WINCS_FAIL == SYS_WINCS_NET_SockSrvCtrl(SYS_WINCS_NET_SOCK_TCP_OPEN, &g_tcpClientSocket))
      {
          M_INTL_SAL_COM_DEBUG(("Error in SOCKET TCP OPEN"));  
      }

    while (!g_socketEventComplete){
        /* Maintain Device Drivers */
        WDRV_WINC_Tasks(sysObj.drvWifiWinc);        
    }
    g_socketEventComplete = false;
    
    M_INTL_SAL_COM_DEBUG(("SOCKET ID: %d", (int)g_tcpClientSocket.sockID));  
    pComInfo->socketId = (int)g_tcpClientSocket.sockID;
        if (pComInfo->socketId <= 0)
        {
          M_INTL_SAL_COM_ERROR(("Invalid socket id"));
          status = E_K_COMM_STATUS_RESOURCE;
          goto end;
        }
    }
 
    pComInfo->isConnected = 1u;
    M_INTL_SAL_COM_DEBUG(("Socket connected"));
    status = E_K_COMM_STATUS_OK;
    goto end;
  }

end:
  M_INTL_SAL_COM_DEBUG(("End of %s", __func__));
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
  g_socketSendComplete = false;
  M_INTL_SAL_COM_DEBUG(("Start of %s", __func__));

  if ((NULL == xpComInfo) || (NULL == xpBuffer) || (0U == xBufferLen) || (pComInfo->ptrId != 2u))
  {
    M_INTL_SAL_COM_ERROR(("Invalid parameter"));
    status = E_K_COMM_STATUS_PARAMETER;
  }
  else
  {
    if (0 == pComInfo->isConnected)
    {
      M_INTL_SAL_COM_DEBUG(("Socket Not Connected"));
      status = E_K_COMM_STATUS_ERROR;
      goto end;
    }
    M_INTL_SAL_COM_DEBUG(("Writing to SOCKET ID: %d", (int)pComInfo->socketId));  
    // Write data to the TCP socket
    if (SYS_WINCS_FAIL == SYS_WINCS_NET_TcpSockWrite(pComInfo->socketId, (uint16_t)xBufferLen, (uint8_t*)xpBuffer))
    {
      status = E_K_COMM_STATUS_ERROR;
      M_INTL_SAL_COM_DEBUG(("Socket send ERROR"));
      goto end;
    }
    
    while (!g_socketSendComplete)
    {
      /* Maintain Device Drivers */
      WDRV_WINC_Tasks(sysObj.drvWifiWinc);
      SYS_CONSOLE_Flush(sysObj.sysConsole0);
    }
    g_socketSendComplete = false;    
    
    M_INTL_SAL_COM_DEBUG(("Socket send complete"));
    status = E_K_COMM_STATUS_OK;
    goto end;
  }

end:
  M_INTL_SAL_COM_DEBUG(("End of %s", __func__));
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
  size_t timeout = 4000;

  M_INTL_SAL_COM_DEBUG(("Start of %s", __func__));

  if ((NULL == xpComInfo) || (NULL == xpBuffer) || (NULL == xpBufferLen) ||
    (0 == *xpBufferLen) || (pComInfo->ptrId != 2u))
  {
    M_INTL_SAL_COM_ERROR(("Invalid parameter"));
    *xpBufferLen = 0;
    return E_K_COMM_STATUS_PARAMETER;
  }

  WDRV_WINC_Tasks(sysObj.drvWifiWinc);
  if (0 == pComInfo->isConnected)
  {
    M_INTL_SAL_COM_DEBUG(("Socket Not Connected"));
    *xpBufferLen = 0;
    return E_K_COMM_STATUS_ERROR;
  }
  // Read data from socket
  while (totalRead < totalToRead)
  {
    size_t currentTimeout = timeout;
    // Wait for data availability
    while (!g_socketReadEvent)
    {
      WDRV_WINC_Tasks(sysObj.drvWifiWinc);
      atca_delay_us(200);
      if (--currentTimeout < 1)
      {
        M_INTL_SAL_COM_ERROR(("Timeout while waiting for g_socketReadEvent"));
        goto end;
      }
    }
    g_socketReadEvent = false;

    // Read data from the socket
    int bytesRead = SYS_WINCS_NET_TcpSockRead(pComInfo->socketId,
                                              totalToRead - totalRead,
                                              readPtr);

    if (bytesRead <= 0)
    {
      M_INTL_SAL_COM_ERROR(("No more data or timeout occurred"));
      break;
    }

    totalRead += bytesRead;
    readPtr += bytesRead;

    currentTimeout = timeout;

    M_INTL_SAL_COM_DEBUG(("Bytes Read in this iteration: %d", bytesRead));
  }

end:

  *xpBufferLen = totalRead;
  if (totalRead > 0)
  {
    M_INTL_SAL_COM_DEBUG(("Total Bytes Read: %d", totalRead));
    status = E_K_COMM_STATUS_OK;
  }
  else
  {
    M_INTL_SAL_COM_ERROR(("No data received"));
    status = E_K_COMM_STATUS_ERROR;
  }
  M_INTL_SAL_COM_DEBUG(("End of %s", __func__));
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
  g_socketEventComplete = false; 
  M_INTL_SAL_COM_DEBUG(("Start of %s", __func__));


  if (NULL == xpComInfo)
  {
    M_INTL_SAL_COM_ERROR(("Invalid parameter"));
    status = E_K_COMM_STATUS_PARAMETER;
    goto end;
  }
  
  if ((pComInfo->isConnected == 0) || (pComInfo->socketId == C_SAL_COM_SOCKET_INVALID))
  {
      M_INTL_SAL_COM_DEBUG(("SOCKET already closed"));
      status = E_K_COMM_STATUS_OK;
      goto end;      
  }  
  else
  {
    // Create a TCP socket SYS_WINCS_NET_SOCK_CLOSE
    if (SYS_WINCS_FAIL == SYS_WINCS_NET_SockSrvCtrl(SYS_WINCS_NET_SOCK_CLOSE,  &(pComInfo->socketId)))
    {
      M_INTL_SAL_COM_ERROR(("SOCK Close error"));
    }

    while (!g_socketEventComplete){
        /* Maintain Device Drivers */
        WDRV_WINC_Tasks(sysObj.drvWifiWinc);        
    }
    
    WDRV_WINC_Tasks(sysObj.drvWifiWinc);     
    g_socketEventComplete = false;    
    
    pComInfo->socketId = C_SAL_COM_SOCKET_INVALID;
    pComInfo->isConnected = 0;
    pComInfo->ptrId = 0u;
    status = E_K_COMM_STATUS_OK;
  }

end:
  M_INTL_SAL_COM_DEBUG(("End of %s", __func__));
  return status;
}
/* -------------------------------------------------------------------------- */
/* LOCAL FUNCTIONS - IMPLEMENTATION                                           */
/* -------------------------------------------------------------------------- */

/* -------------------------------------------------------------------------- */
/* END OF FILE                                                                */
/* -------------------------------------------------------------------------- */
