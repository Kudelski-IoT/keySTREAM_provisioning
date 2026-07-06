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
/** \brief Socket communication - Windows Winsock2 implementation.
 *
 *  \author Kudelski IoT
 *
 *  \date 2025/01/08
 *
 *  \file k_sal_com.c
 ******************************************************************************/
/**
 * @brief Windows socket communication implementation using Winsock2.
 */

/* -------------------------------------------------------------------------- */
/* IMPORTS                                                                    */
/* -------------------------------------------------------------------------- */
#include "k_sal_com.h"
#include <winsock2.h>
#include <ws2tcpip.h>
#include <string.h>
#include <stdio.h>

#pragma comment(lib, "ws2_32.lib")

/* -------------------------------------------------------------------------- */
/* LOCAL CONSTANTS, TYPES, ENUM                                               */
/* -------------------------------------------------------------------------- */

/** @brief Invalid Socket Id. */
#define C_SAL_COM_SOCKET_INVALID            ((SOCKET)INVALID_SOCKET)

/** @brief Magic number to validate pointer */
#define C_SAL_COM_MAGIC_NUMBER              (0x53414C32U)  /* "SAL2" */

/** @brief Connection state flags */
#define C_SAL_COM_STATE_INITIALIZED         (0x01U)
#define C_SAL_COM_STATE_CONNECTED           (0x02U)

/* -------------------------------------------------------------------------- */
/* TYPES & STRUCTURES                                                         */
/* -------------------------------------------------------------------------- */

/**
 * @brief Communication info structure.
 */
typedef struct
{
  SOCKET        socketId;           /**< Windows socket handle */
  uint32_t      connectTimeOut;     /**< Connection timeout in ms */
  uint32_t      readTimeOut;        /**< Read timeout in ms */
  uint32_t      state;              /**< Connection state flags */
  uint32_t      magicNumber;        /**< Validation magic number */
} TKComInfo;

/* -------------------------------------------------------------------------- */
/* LOCAL VARIABLES                                                            */
/* -------------------------------------------------------------------------- */

/** @brief Global communication info structure */
static TKComInfo g_comInfo = {C_SAL_COM_SOCKET_INVALID, 0U, 0U, 0U, 0U};

/** @brief WSA initialization state */
static bool g_wsaInitialized = false;

/* -------------------------------------------------------------------------- */
/* LOCAL FUNCTIONS - PROTOTYPE                                                */
/* -------------------------------------------------------------------------- */

/**
 * @brief Initialize Winsock library.
 *
 * @return E_K_COMM_STATUS_OK on success, error status otherwise.
 */
static TKCommStatus lInitializeWinsock(void);

/**
 * @brief Cleanup Winsock library.
 */
static void lCleanupWinsock(void);

/**
 * @brief Validate com info pointer.
 *
 * @param[in] xpComInfo Pointer to validate. Should not be NULL.
 *
 * @return true if valid, false otherwise.
 */
static bool lIsValidComInfo
(
  const void* xpComInfo
);

/* -------------------------------------------------------------------------- */
/* PUBLIC FUNCTIONS - IMPLEMENTATION                                          */
/* -------------------------------------------------------------------------- */

/**
 * @brief Initialize COM (Windows Winsock2 implementation).
 *
 * @param[in] xConnectTimeoutInMs Connection timeout in milliseconds.
 * @param[in] xReadTimeoutInMs Read timeout in milliseconds.
 * @param[out] xppComInfo Pointer to com info data. Should not be NULL.
 *
 * @return E_K_COMM_STATUS_OK or the error status.
 */
K_SAL_API TKCommStatus salComInit
(
  uint32_t  xConnectTimeoutInMs,
  uint32_t  xReadTimeoutInMs,
  void**    xppComInfo
)
{
  TKCommStatus xStatus = E_K_COMM_STATUS_ERROR;

  if (NULL == xppComInfo)
  {
    xStatus = E_K_COMM_STATUS_PARAMETER;
  }
  else if (C_SAL_COM_MAGIC_NUMBER == g_comInfo.magicNumber)
  {
    /* Already initialized */
    xStatus = E_K_COMM_STATUS_OK;
    *xppComInfo = &g_comInfo;
  }
  else
  {
    /* Initialize Winsock */
    xStatus = lInitializeWinsock();
    
    if (E_K_COMM_STATUS_OK == xStatus)
    {
      g_comInfo.socketId = C_SAL_COM_SOCKET_INVALID;
      g_comInfo.connectTimeOut = xConnectTimeoutInMs;
      g_comInfo.readTimeOut = xReadTimeoutInMs;
      g_comInfo.state = C_SAL_COM_STATE_INITIALIZED;
      g_comInfo.magicNumber = C_SAL_COM_MAGIC_NUMBER;
      *xppComInfo = &g_comInfo;
    }
  }

  return xStatus;
}

/**
 * @brief Establish connection with server (Windows implementation).
 *
 * @param[in] xpComInfo Com Info data. Should not be NULL.
 * @param[in] xpHost Server Host name. Should not be NULL. Must have '\0' at the end.
 * @param[in] xpPort Server Port. Should not be NULL. Must have '\0' at the end.
 *
 * @return E_K_COMM_STATUS_OK or the error status.
 */
K_SAL_API TKCommStatus salComConnect
(
  void*          xpComInfo,
  const uint8_t* xpHost,
  const uint8_t* xpPort
)
{
  TKComInfo*    xpInfo = (TKComInfo*)xpComInfo;
  TKCommStatus  xStatus = E_K_COMM_STATUS_ERROR;
  struct addrinfo  hints;
  struct addrinfo* xpResult = NULL;
  struct addrinfo* xpPtr = NULL;
  int           xResult = 0;
  DWORD         xTimeout = 0U;

  if ((NULL == xpComInfo) || (NULL == xpHost) || (NULL == xpPort) ||
      (false == lIsValidComInfo(xpComInfo)))
  {
    xStatus = E_K_COMM_STATUS_PARAMETER;
  }
  else if (0U != (xpInfo->state & C_SAL_COM_STATE_CONNECTED))
  {
    /* Already connected */
    xStatus = E_K_COMM_STATUS_ERROR;
  }
  else
  {
    /* Setup hints for getaddrinfo */
    (void)memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;      /* Allow IPv4 or IPv6 */
    hints.ai_socktype = SOCK_STREAM;  /* TCP socket */
    hints.ai_protocol = IPPROTO_TCP;

    /* Resolve server address */
    xResult = getaddrinfo((const char*)xpHost, (const char*)xpPort, &hints, &xpResult);
    
    if (0 != xResult)
    {
      xStatus = E_K_COMM_STATUS_RESOURCE;
    }
    else
    {
      /* Attempt to connect to server */
      for (xpPtr = xpResult; NULL != xpPtr; xpPtr = xpPtr->ai_next)
      {
        /* Create socket */
        xpInfo->socketId = socket(xpPtr->ai_family, xpPtr->ai_socktype, xpPtr->ai_protocol);
        
        if (C_SAL_COM_SOCKET_INVALID == xpInfo->socketId)
        {
          continue;
        }

        /* Set connect timeout */
        if (0U != xpInfo->connectTimeOut)
        {
          xTimeout = xpInfo->connectTimeOut;
          (void)setsockopt(xpInfo->socketId, SOL_SOCKET, SO_SNDTIMEO,
                          (const char*)&xTimeout, sizeof(xTimeout));
        }

        /* Set read timeout */
        if (0U != xpInfo->readTimeOut)
        {
          xTimeout = xpInfo->readTimeOut;
          (void)setsockopt(xpInfo->socketId, SOL_SOCKET, SO_RCVTIMEO,
                          (const char*)&xTimeout, sizeof(xTimeout));
        }

        /* Connect to server */
        xResult = connect(xpInfo->socketId, xpPtr->ai_addr, (int)xpPtr->ai_addrlen);
        
        if (SOCKET_ERROR == xResult)
        {
          (void)closesocket(xpInfo->socketId);
          xpInfo->socketId = C_SAL_COM_SOCKET_INVALID;
          continue;
        }

        /* Connection successful */
        xpInfo->state |= C_SAL_COM_STATE_CONNECTED;
        xStatus = E_K_COMM_STATUS_OK;
        break;
      }

      freeaddrinfo(xpResult);
      
      if (E_K_COMM_STATUS_OK != xStatus)
      {
        xStatus = E_K_COMM_STATUS_NETWORK;
      }
    }
  }

  return xStatus;
}

/**
 * @brief Send data to the server (Windows implementation).
 *
 * @param[in] xpComInfo Com info data. Should not be NULL.
 * @param[in] xpBuffer Data buffer to send. Should not be NULL.
 * @param[in] xBufferLen Size of the data buffer, in bytes.
 *
 * @return E_K_COMM_STATUS_OK or the error status.
 */
K_SAL_API TKCommStatus salComWrite
(
  void*          xpComInfo,
  const uint8_t* xpBuffer,
  size_t         xBufferLen
)
{
  TKComInfo*    xpInfo = (TKComInfo*)xpComInfo;
  TKCommStatus  xStatus = E_K_COMM_STATUS_ERROR;
  int           xBytesSent = 0;
  size_t        xTotalSent = 0U;

  if ((NULL == xpComInfo) || (NULL == xpBuffer) || (0U == xBufferLen) ||
      (false == lIsValidComInfo(xpComInfo)))
  {
    xStatus = E_K_COMM_STATUS_PARAMETER;
  }
  else if (0U == (xpInfo->state & C_SAL_COM_STATE_CONNECTED))
  {
    xStatus = E_K_COMM_STATUS_ERROR;
  }
  else
  {
    xStatus = E_K_COMM_STATUS_OK;
    
    /* Send data in loop until all bytes sent */
    while (xTotalSent < xBufferLen)
    {
      xBytesSent = send(xpInfo->socketId, 
                       (const char*)(xpBuffer + xTotalSent),
                       (int)(xBufferLen - xTotalSent), 0);
      
      if (SOCKET_ERROR == xBytesSent)
      {
        xStatus = E_K_COMM_STATUS_NETWORK;
        break;
      }
      
      xTotalSent += (size_t)xBytesSent;
    }
  }

  return xStatus;
}

/**
 * @brief Read data from the server (Windows implementation).
 *
 * @param[in] xpComInfo Com info data. Should not be NULL.
 * @param[out] xpBuffer Buffer to store received data. Should not be NULL.
 * @param[in,out] xpBufferLen In: Buffer size, Out: Bytes received.
 *
 * @return E_K_COMM_STATUS_OK or the error status.
 */
K_SAL_API TKCommStatus salComRead
(
  void*    xpComInfo,
  uint8_t* xpBuffer,
  size_t*  xpBufferLen
)
{
  TKComInfo*    xpInfo = (TKComInfo*)xpComInfo;
  TKCommStatus  xStatus = E_K_COMM_STATUS_ERROR;
  int           xBytesReceived = 0;

  if ((NULL == xpComInfo) || (NULL == xpBuffer) || (NULL == xpBufferLen) ||
      (0U == *xpBufferLen) || (false == lIsValidComInfo(xpComInfo)))
  {
    xStatus = E_K_COMM_STATUS_PARAMETER;
  }
  else if (0U == (xpInfo->state & C_SAL_COM_STATE_CONNECTED))
  {
    xStatus = E_K_COMM_STATUS_ERROR;
  }
  else
  {
    xBytesReceived = recv(xpInfo->socketId, (char*)xpBuffer, (int)*xpBufferLen, 0);
    
    if (SOCKET_ERROR == xBytesReceived)
    {
      int xError = WSAGetLastError();
      
      if (WSAETIMEDOUT == xError)
      {
        xStatus = E_K_COMM_STATUS_TIMEOUT;
      }
      else
      {
        xStatus = E_K_COMM_STATUS_NETWORK;
      }
      
      *xpBufferLen = 0U;
    }
    else if (0 == xBytesReceived)
    {
      /* Connection closed */
      xStatus = E_K_COMM_STATUS_NETWORK;
      *xpBufferLen = 0U;
    }
    else
    {
      *xpBufferLen = (size_t)xBytesReceived;
      xStatus = E_K_COMM_STATUS_OK;
    }
  }

  return xStatus;
}

/**
 * @brief Terminate COM and close connection (Windows implementation).
 *
 * @param[in] xpComInfo Com info data. Should not be NULL.
 *
 * @return E_K_COMM_STATUS_OK or the error status.
 */
K_SAL_API TKCommStatus salComTerm
(
  void* xpComInfo
)
{
  TKComInfo*    xpInfo = (TKComInfo*)xpComInfo;
  TKCommStatus  xStatus = E_K_COMM_STATUS_ERROR;

  if ((NULL == xpComInfo) || (false == lIsValidComInfo(xpComInfo)))
  {
    xStatus = E_K_COMM_STATUS_PARAMETER;
  }
  else
  {
    /* Close socket if connected */
    if ((0U != (xpInfo->state & C_SAL_COM_STATE_CONNECTED)) &&
        (C_SAL_COM_SOCKET_INVALID != xpInfo->socketId))
    {
      (void)closesocket(xpInfo->socketId);
    }

    /* Reset state */
    xpInfo->socketId = C_SAL_COM_SOCKET_INVALID;
    xpInfo->state = 0U;
    xpInfo->magicNumber = 0U;
    
    /* Cleanup Winsock */
    lCleanupWinsock();
    
    xStatus = E_K_COMM_STATUS_OK;
  }

  return xStatus;
}

/* -------------------------------------------------------------------------- */
/* LOCAL FUNCTIONS - IMPLEMENTATION                                           */
/* -------------------------------------------------------------------------- */

/**
 * @brief Initialize Winsock library.
 *
 * @return E_K_COMM_STATUS_OK on success, error status otherwise.
 */
static TKCommStatus lInitializeWinsock(void)
{
  TKCommStatus xStatus = E_K_COMM_STATUS_ERROR;
  WSADATA xWsaData;
  int xResult = 0;

  if (false == g_wsaInitialized)
  {
    xResult = WSAStartup(MAKEWORD(2, 2), &xWsaData);
    
    if (0 == xResult)
    {
      g_wsaInitialized = true;
      xStatus = E_K_COMM_STATUS_OK;
    }
    else
    {
      xStatus = E_K_COMM_STATUS_RESOURCE;
    }
  }
  else
  {
    xStatus = E_K_COMM_STATUS_OK;
  }

  return xStatus;
}

/**
 * @brief Cleanup Winsock library.
 */
static void lCleanupWinsock(void)
{
  if (true == g_wsaInitialized)
  {
    (void)WSACleanup();
    g_wsaInitialized = false;
  }
}

/**
 * @brief Validate com info pointer.
 *
 * @param[in] xpComInfo Pointer to validate. Should not be NULL.
 *
 * @return true if valid, false otherwise.
 */
static bool lIsValidComInfo
(
  const void* xpComInfo
)
{
  const TKComInfo* xpInfo = (const TKComInfo*)xpComInfo;
  bool xIsValid = false;

  if ((NULL != xpComInfo) && (C_SAL_COM_MAGIC_NUMBER == xpInfo->magicNumber))
  {
    xIsValid = true;
  }

  return xIsValid;
}

/* -------------------------------------------------------------------------- */
/* END OF FILE                                                                */
/* -------------------------------------------------------------------------- */
