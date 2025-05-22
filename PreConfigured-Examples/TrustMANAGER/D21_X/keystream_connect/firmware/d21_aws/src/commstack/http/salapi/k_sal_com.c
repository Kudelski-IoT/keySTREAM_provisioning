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
#include "cloud_wifi_task.h"
#include "socket.h"
#include "cryptoauthlib.h"

#include <errno.h>
#include <unistd.h>
#include <string.h>

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
#define C_SAL_COM_RESET_READ_TIMEOUT_IN_MS  (100u)

/** @brief Length of an address in IP V6 protocol, in bytes. */
#define C_SAL_COM_V6_ADDRESS_NUM_BYTES      (16u)

/******************************************************************************/
/* LOCAL MACROS                                                               */
/******************************************************************************/

#ifdef DEBUG
/** @brief Enable sal com logs. */
#define M_INTL_SAL_COM_DEBUG(__PRINT__)  do { \
                                              printf("\n\tCOM %d>", __LINE__); \
                                              printf __PRINT__; \
                                              printf("\r\n"); \
                                            } while (0)
#define M_INTL_SAL_COM_ERROR(__PRINT__)  do { \
                                              printf("\n\tCOM %d> ERROR ", __LINE__); \
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

/** @brief Boolean Types. */
typedef enum
{
  E_FALSE,
  /* TRUE. */
  E_TRUE,
  /* FALSE. */
  E_UNDEF
  /* Undefined. */
} TBoolean;

/**
 * @ingroup g_k_types
 *
 * @brief Supported IP protocols.
 */
typedef enum
{
  E_K_IP_PROTOCOL_V4,
  /* IP V4. */
  E_K_IP_PROTOCOL_V6,
  /* IP V6. */
  E_K_IP_NUM_PROTOCOLS
  /* Number of supported IP protocols. */
} TKIpProtocol;

/**
 * @ingroup g_k_types
 *
 * @brief Socket IP address, expressed for protocol IP V4.
*/
typedef struct
{
  uint32_t       address;
  /* IP V4 address, MSBF; e.g. 192.168.15.4 = 0xC0A80F04 */
  uint16_t       port;
  /* IP port. */
} TKSocketAddressV4;

/**
 * @ingroup g_k_types
 *
 * @brief Socket IP.
 */
typedef struct
{
  TKIpProtocol         protocol;
  /* IP protocol. */
  TKSocketAddressV4    v4;
  /* IP address; the used field depends on protocol value. */
} TKSocketIp;

/**
 * @ingroup g_k_types
 *
 * @brief Communication info.
 */
typedef struct
{
  int       socketId;
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
static TKComInfo gComInfo = {C_SAL_COM_SOCKET_INVALID, 0, 0, 0, 0};
/* -------------------------------------------------------------------------- */
/* LOCAL FUNCTIONS - PROTOTYPE                                                */
/* -------------------------------------------------------------------------- */

/**
 * @brief
 *   Convert the ip to sal socket structure.
 *
 * @param[in] xpAddress
 *   Ip address of the server. Should not be NULL
 * @param[out] xpSocketIp
 *   Converted SAL socket ip.
 * @return
 * - E_TRUE for success
 * - E_FALSE for failure
 */
static TBoolean lConvertSocketIp
(
  const uint8_t*  xpAddress,
  TKSocketIp*     xpSocketIp
);

/* -------------------------------------------------------------------------- */
/* PUBLIC VARIABLES                                                           */
/* -------------------------------------------------------------------------- */

extern int32_t kta_gRxBufferLength;
extern enum wifi_status kta_gSockRecvStatus;
extern enum wifi_status kta_gSockSendStatus;
extern uint32_t kta_gTxSize;
extern uint8_t* kta_getHostByName(const char* xpUrl);

/* -------------------------------------------------------------------------- */
/* PUBLIC FUNCTIONS - IMPLEMENTATION                                          */
/* -------------------------------------------------------------------------- */

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
  TKComInfo* pComInfo = NULL;

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
      M_INTL_SAL_COM_ERROR(("Already Initialised!!"));
    }
    else
    {
      pComInfo = &gComInfo;
      pComInfo->socketId = C_SAL_COM_SOCKET_INVALID;
      pComInfo->connectTimeOut = xConnectTimeoutInMs;
      pComInfo->readTimeOut = xReadTimeoutInMs;
      pComInfo->isConnected = 0;
      pComInfo->ptrId = 2u;

      *xppComInfo = pComInfo;
      status = E_K_COMM_STATUS_OK;
    }
  }

  M_INTL_SAL_COM_DEBUG(("End of %s", __func__));
  return status;
}

/**
 * @brief  implement salComConnect
 *
 */
/**
 * SUPPRESS: MISRA_DEV_KTA_005 : misra_c2012_rule_15.4_violation
 * SUPPRESS: MISRA_DEV_KTA_004 : misra_c2012_rule_15.1_violation
 * Using goto for breaking during the error and return cases.
 **/
K_SAL_API TKCommStatus salComConnect
(
  void*          xpComInfo,
  const uint8_t* xpHost,
  const uint8_t* xpPort
)
{
  TKComInfo*          pComInfo = (TKComInfo *)xpComInfo;
  TKCommStatus        status = E_K_COMM_STATUS_ERROR;
  int                 socStatus = E_K_COMM_STATUS_ERROR;
  struct sockaddr_in  socketAddress = { 0 };
  TKSocketIp          socketIp = { 0 };
  uint16_t            port = 0;
  uint32_t            retVal = 0;
  uint8_t*            pHostIp = NULL;
  char*               pEnd = NULL;
  TBoolean            isValidAdd = E_FALSE;
  int8_t              sockRet = SOCK_ERR_NO_ERROR;

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
    pHostIp = kta_getHostByName((const char *)xpHost);
    isValidAdd = lConvertSocketIp(pHostIp, &socketIp);
    if (E_TRUE != isValidAdd)
    {
      status = E_K_COMM_STATUS_PARAMETER;
      M_INTL_SAL_COM_ERROR(("Invalid ip4 socket IP[%s]", pHostIp));
      goto end;
    }
    errno = 0;
    retVal = (uint32_t)strtol((const char *)xpPort, &pEnd, 10);
    if ((retVal > 0) && (retVal <= 0xFFFF))
    {
      port = (uint16_t)retVal;
    }
    if ((0 != errno)      ||
        (retVal > 0xFFFF) ||
        (retVal == 0))
    {
      M_INTL_SAL_COM_ERROR(("Error Occured %d", errno));
      goto end;
    }

    if (C_SAL_COM_SOCKET_INVALID == pComInfo->socketId)
    {
      /* Create socket. */
      pComInfo->socketId = socket(AF_INET, SOCK_STREAM, 0);
      if (pComInfo->socketId < 0)
      {
        M_INTL_SAL_COM_ERROR(("Invalid socket id"));
        status = E_K_COMM_STATUS_RESOURCE;
        goto end;
      }
    }
    M_INTL_SAL_COM_DEBUG(("IpAdd[%s] Port[%d]", pHostIp, port));

    socketAddress.sin_family      = AF_INET;
    socketAddress.sin_addr.s_addr = _htonl(socketIp.v4.address);
    socketAddress.sin_port        = _htons(port);

    M_INTL_SAL_COM_DEBUG(("Connect To Add [%d %x %x]",
                          socketAddress.sin_family,
                          socketAddress.sin_port,
                          (unsigned int)socketAddress.sin_addr.s_addr));
    socStatus = connect(pComInfo->socketId,
                        (struct sockaddr*)&socketAddress,
                        sizeof(socketAddress));
    if (socStatus != SOCK_ERR_NO_ERROR)
    {
      M_INTL_SAL_COM_ERROR(("Connect to server failed"));
      status = E_K_COMM_STATUS_RESOURCE;
      sockRet = shutdown(pComInfo->socketId);
      if (sockRet != SOCK_ERR_NO_ERROR)
      {
        M_INTL_SAL_COM_ERROR(("shutdown failed"));
        status = E_K_COMM_STATUS_ERROR;
        goto end;
      }
      pComInfo->socketId = C_SAL_COM_SOCKET_INVALID;
      goto end;
    }
    pComInfo->isConnected = 1u;
    atca_delay_ms(C_SAL_COM_CONNECT_DELAY_IN_MS);
    status = E_K_COMM_STATUS_OK;
  }

end:
  M_INTL_SAL_COM_DEBUG(("End of %s", __func__));
  return status;
}

/**
 * @brief  implement salComWrite
 *
 */
/**
 * SUPPRESS: MISRA_DEV_KTA_005 : misra_c2012_rule_15.4_violation
 * SUPPRESS: MISRA_DEV_KTA_004 : misra_c2012_rule_15.1_violation
 * Using goto for breaking during the error and return cases.
 **/
K_SAL_API TKCommStatus salComWrite
(
  void*          xpComInfo,
  const uint8_t* xpBuffer,
  size_t         xBufferLen
)
{
  TKCommStatus  status = E_K_COMM_STATUS_ERROR;
  TKComInfo* pComInfo  = (TKComInfo *)xpComInfo;
  int       sockStatus = 0;

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
    atca_delay_ms(5);
    kta_gSockSendStatus = WIFI_STATUS_UNKNOWN;

    sockStatus = send(pComInfo->socketId, (void *)xpBuffer, xBufferLen, 0);
    if (sockStatus != SOCK_ERR_NO_ERROR)
    {
      M_INTL_SAL_COM_ERROR(("send failed status[%d]", sockStatus));
      status = E_K_COMM_STATUS_RESOURCE;
      goto end;
    }
    kta_gTxSize = (uint32_t)xBufferLen;
    status = E_K_COMM_STATUS_OK;
    /* Number of bytes sent check is already taken care in MCHP Driver. */

    do
    {
      /* Wait until the outgoing message was sent. */
      m2m_wifi_handle_events();

      if (kta_gSockSendStatus == WIFI_STATUS_ERROR)
      {
        M_INTL_SAL_COM_ERROR(("send error status[%d]", kta_gSockSendStatus));
        status = E_K_COMM_STATUS_ERROR;
        /* Break the do/while loop on error. */
        break;
      }
      atca_delay_ms(1);
    } while ((kta_gSockSendStatus != WIFI_STATUS_MESSAGE_SENT));
  }

end:
  M_INTL_SAL_COM_DEBUG(("End of %s", __func__));
  return status;
}

/**
 * @brief  implement salComRead
 *
 */
/**
 * SUPPRESS: MISRA_DEV_KTA_005 : misra_c2012_rule_15.4_violation
 * SUPPRESS: MISRA_DEV_KTA_004 : misra_c2012_rule_15.1_violation
 * Using goto for breaking during the error and return cases.
 **/
K_SAL_API TKCommStatus salComRead
(
  void*     xpComInfo,
  uint8_t*  xpBuffer,
  size_t*   xpBufferLen
)
{
  TKComInfo* pComInfo   = (TKComInfo *)xpComInfo;
  TKCommStatus  status  = E_K_COMM_STATUS_OK;
  int       socStatus   = 0;
  uint32_t  readTimeOut = 0;

  M_INTL_SAL_COM_DEBUG(("Start of %s", __func__));

  if ((NULL == xpComInfo) || (NULL == xpBuffer) || (NULL == xpBufferLen) ||
      (0 == *xpBufferLen) || (pComInfo->ptrId != 2u))
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
    atca_delay_ms(2);
    kta_gRxBufferLength = 0;
    readTimeOut = pComInfo->readTimeOut;
    while (E_K_COMM_STATUS_ERROR != status)
    {
      kta_gSockRecvStatus = WIFI_STATUS_UNKNOWN;
      /* Receive the incoming message. */
      socStatus = recv(pComInfo->socketId,
                       &xpBuffer[kta_gRxBufferLength],
                       *xpBufferLen,
                       readTimeOut);
      if (socStatus != SOCK_ERR_NO_ERROR)
      {
        M_INTL_SAL_COM_ERROR(("recv failed status[%d]", socStatus));
        status = E_K_COMM_STATUS_RESOURCE;
        break;
      }

      while (kta_gSockRecvStatus != WIFI_STATUS_MESSAGE_RECEIVED)
      {
        if ((kta_gSockRecvStatus == WIFI_STATUS_TIMEOUT) ||
            (kta_gSockRecvStatus == WIFI_STATUS_ERROR))
        {
          status = E_K_COMM_STATUS_ERROR;
          break;
        }
        atca_delay_ms(1);
        m2m_wifi_handle_events();
      }
      readTimeOut = C_SAL_COM_RESET_READ_TIMEOUT_IN_MS;
      M_INTL_SAL_COM_DEBUG(("Reset Timeout[%d]", (int)readTimeOut));
      M_INTL_SAL_COM_DEBUG(("Bytes Read[%d]", (int)kta_gRxBufferLength));
    }

    if (kta_gRxBufferLength > 0)
    {
      *xpBufferLen = kta_gRxBufferLength;
      status = E_K_COMM_STATUS_OK;
    }
    else
    {
      M_INTL_SAL_COM_ERROR(("salTlsRead Failed %d", kta_gSockRecvStatus));
    }
  }

end:
  M_INTL_SAL_COM_DEBUG(("End of %s", __func__));

  return status;
}

/**
 * @brief  implement salComTerm
 *
 */
/**
 * SUPPRESS: MISRA_DEV_KTA_005 : misra_c2012_rule_15.4_violation
 * SUPPRESS: MISRA_DEV_KTA_004 : misra_c2012_rule_15.1_violation
 * Using goto for breaking during the error and return cases.
 **/
K_SAL_API TKCommStatus salComTerm
(
  void*  xpComInfo
)
{
  TKCommStatus status = E_K_COMM_STATUS_ERROR;
  TKComInfo* pComInfo = (TKComInfo *)xpComInfo;
  int8_t sockRet = SOCK_ERR_NO_ERROR;

  M_INTL_SAL_COM_DEBUG(("Start of %s", __func__));

  if (NULL == xpComInfo)
  {
    M_INTL_SAL_COM_ERROR(("Invalid parameter"));
    status = E_K_COMM_STATUS_PARAMETER;
  }
  else
  {
    if (pComInfo->socketId != C_SAL_COM_SOCKET_INVALID)
    {
      sockRet = shutdown(pComInfo->socketId);
      if (sockRet != SOCK_ERR_NO_ERROR)
      {
        M_INTL_SAL_COM_ERROR(("shutdown failed"));
        status = E_K_COMM_STATUS_ERROR;
        goto end;
      }
      pComInfo->socketId = C_SAL_COM_SOCKET_INVALID;
      pComInfo->isConnected = 0;
    }
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

/**
 * @implements lConvertSocketIp
 *
 **/
static TBoolean lConvertSocketIp
(
  const uint8_t*  xpAddress,
  TKSocketIp*     xpSocketIp
)
{
  size_t   value    = 0;
  uint32_t address  = 0;
  uint8_t  numDots  = 0;
  size_t   index    = 0;
  TBoolean  isPort  = E_FALSE;
  TBoolean  isEnd   = E_FALSE;
  TBoolean  isValid = E_FALSE;

  if (NULL != xpAddress)
  {
    isValid = E_TRUE;
    for (index = 0; isValid && !isEnd; index++)
    {
      switch (xpAddress[index])
      {
      case '\0':
        isEnd = E_TRUE;
        if (E_TRUE == isPort)
        {
          if ((value > 0U) && (value <= (uint16_t)0xFFFF))
          { /* format is a.b.c.d:port */
            xpSocketIp->v4.port = (uint16_t)value;
          }
          else
          {
            isValid = E_FALSE;
          }
        }
        else
        {
          if (3U == numDots)
          { /* format is a.b.c.d */
            xpSocketIp->v4.address = (address << 8U) + (uint32_t)value;
          }
          else
          {
            isValid = E_FALSE;
          }
        }
        break;

      case ':':
        isPort = E_TRUE;
        switch (numDots)
        {
        case 0: /* format is ":port" */
          break;

        case 3: /* format is "a.b.c.d:" */
          xpSocketIp->v4.address = (address << 8) + (uint32_t)value;
          value = 0;
          break;

        default:
          isValid = E_FALSE;
          break;
        }
        break;

      case '.':
        if ((isPort) || (numDots > 3U))
        {
          isValid = E_FALSE;
        }
        else
        {
          numDots++;
          address = (address << 8U) + (uint32_t)value;
          value = 0;
        }
        break;

      default:
        if ((xpAddress[index] >= (uint8_t)'0') && (xpAddress[index] <= (uint8_t)'9'))
        {
          value = (10U * value) + (size_t)(xpAddress[index] - (uint8_t)'0');
        }
        else
        {
          isValid = E_FALSE;
        }
        break;
      } /* switch. */
    } /* for. */
  } /* if. */

  if (E_TRUE == isValid)
  {
    xpSocketIp->protocol = E_K_IP_PROTOCOL_V4;
  }
  return isValid;
}

/* -------------------------------------------------------------------------- */
/* END OF FILE                                                                */
/* -------------------------------------------------------------------------- */
